// tests/sim/vehicle/CarsTest.cpp: the car builds (content/builds/cars/*.json): each names only the vehicle's fields,
// within their ranges, and uses the vehicle model; and, on the rig's made-up engine at equal power and weight, the
// three archetypes launch in the order traction says (all-wheel, rear, front) when grip is the limit. (The same with
// real engines, and their quarter-mile times, are the integrated tests: they wait for the split's Car seam.)
#include "Rig.h"

#include <catch2/catch_approx.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

using namespace tee::vehicle;

namespace
{
    std::filesystem::path carDir() { return std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "cars"; }

    nlohmann::json loadCar(const std::string& name)
    {
        auto doc = nlohmann::json::parse(readFile(carDir() / (name + ".json")), nullptr, false);
        INFO(name);
        REQUIRE(!doc.is_discarded());
        return doc;
    }

    // A build's car section into the rig's car.spec: numbers as numbers, switches as switches, choices by name
    void applyCar(Rig& rig, const tee::Schema& schema, const nlohmann::json& car)
    {
        for (const auto& [key, value] : car.items())
        {
            if (key == "kind")
                continue;   // (the reference's Car or Truck: spec.car's own, not the model's)
            const std::string path = "car.spec." + key;
            tee::Schema::Handle h;
            std::string error;
            INFO(path);
            REQUIRE(schema.find(path, h, error));
            if (value.is_string())
            {
                const auto* choices = h.node->attribute("ui::choices");
                REQUIRE(choices != nullptr);
                const auto* range = h.node->attribute("fminus::range");
                const double lo = range != nullptr ? range->arguments[0].number : 0.0;
                int index = -1;
                for (std::size_t i = 0; i < choices->arguments.size(); ++i)
                    if (choices->arguments[i].text == value.get<std::string>())
                        index = static_cast<int>(i);
                REQUIRE(index >= 0);
                rig.set(path, lo + index);
            }
            else if (value.is_boolean())
                rig.set(path, value.get<bool>() ? 1.0 : 0.0);
            else if (value.is_array())
            {
                for (std::size_t i = 0; i < value.size(); ++i)
                    rig.set(path + "[" + std::to_string(i) + "]", value[i].get<double>());
            }
            else
            {
                const double v = value.get<double>();
                const auto* range = h.node->attribute("fminus::range");
                if (range != nullptr)
                {
                    CHECK(v >= range->arguments[0].number);
                    CHECK(v <= range->arguments[1].number);
                }
                rig.set(path, v);
            }
        }
    }

    const tee::Schema& schema()
    {
        static std::unique_ptr<tee::Schema> s;
        if (!s)
        {
            std::string error;
            s = tee::Schema::of(compiled().program->fminus(), "Rig", error);
        }
        return *s;
    }

    const std::vector<std::string> kArchetypes = {"light-fwd-four", "rwd-v8-coupe", "awd-turbo-six"};
} // namespace

TEST_CASE("cars.files: every car build uses the vehicle model and names only its fields, within range",
          "[vehicle][cars]")
{
    int files = 0;
    for (const auto& entry : std::filesystem::directory_iterator(carDir()))
    {
        if (entry.path().extension() != ".json")
            continue;
        files += 1;
        const auto doc = loadCar(entry.path().stem().string());
        INFO(entry.path().filename().string());
        CHECK(doc["format"] == "tee-build");
        CHECK(doc["version"] == 1);
        CHECK(doc["spec"].size() == 1);
        REQUIRE(doc["spec"].contains("car"));
        CHECK(doc["spec"]["car"]["model"] == "Vehicle");
        Rig rig;
        applyCar(rig, schema(), doc["spec"]["car"]);
        rig.settle();
        CHECK(rig.get("car.spec.model") == 1.0);
    }
    CHECK(files >= 3);
}

TEST_CASE("cars.traction: at equal power and weight on equal tyres, all-wheel drive pulls hardest, then rear, "
          "then front", "[vehicle][cars]")
{
    // Each archetype at 1500 kg on street tyres with a manual gearbox and the same engine (220 kW, 450 N m), rolling
    // at 4 m/s in first: the most it accelerates at any throttle (a sweep of fixed throttles a hundredth apart) is
    // what its traction allows. The hand calculation for mu = 1 (h = 0.5 m, L = 2.6 to 2.7 m) says about 1 g for all
    // four wheels, 0.46 / (1 - mu h / L) = 0.57 g for the rear with 46 % on it, 0.62 / (1 + mu h / L) = 0.52 g for the
    // front with 62 %.
    std::map<std::string, double> best;
    for (const auto& name : kArchetypes)
    {
        best[name] = 0.0;
        double bestThrottle = 0.0;
        for (int step = 0; step <= 80; ++step)
        {
            Rig rig;
            applyCar(rig, schema(), loadCar(name)["spec"]["car"]);
            rig.set("car.spec.mass", 1500.0);
            rig.set("car.spec.tyre", Street);
            rig.set("car.spec.gearbox", Manual);
            rig.set("curve", 1);
            rig.set("power", 220000.0);
            rig.set("peak", 450.0);
            rig.set("noStall", 1);
            // geared short enough that the engine can outpull every tyre: 14 to 1 overall
            rig.set("car.spec.autoGearing", 0);
            rig.set("car.spec.ratio[0]", 3.5);
            rig.set("car.spec.finalDrive", 4.0);
            rig.settle();
            rig.set("clutch", 1.0);
            rig.seconds(0.2);
            rig.set("lever", 1);
            rig.seconds(0.2);
            // rolling at 4 m/s, the clutch in
            const double v = 4.0, r = rig.get("car.radius");
            rig.set("car.v", v);
            rig.set("car.vPrev", v);
            for (int i = 0; i < 4; ++i)
                rig.set("car.ww[" + std::to_string(i) + "]", v / r);
            rig.set("car.wo", v / r);
            rig.set("w", rig.get("car.gA") * v / r);
            rig.set("clutch", 0.0);
            rig.set("throttle", 0.2 + 0.01 * step);
            rig.seconds(0.6);
            const double v0 = rig.get("car.v");
            rig.seconds(0.4);
            const double a = (rig.get("car.v") - v0) / 0.4;
            if (a / 9.81 > best[name])
            {
                best[name] = a / 9.81;
                bestThrottle = 0.2 + 0.01 * step;
            }
        }
        std::printf("traction: %s accelerates at up to %.3f g (at throttle %.2f)\n", name.c_str(), best[name], bestThrottle);
    }
    CHECK(best["awd-turbo-six"] > best["rwd-v8-coupe"]);
    CHECK(best["rwd-v8-coupe"] > best["light-fwd-four"]);
    // (less what the wheels' own inertia and the rolling take, and a fixed throttle never sits exactly at the peak)
    CHECK(best["awd-turbo-six"] == Catch::Approx(0.95).margin(0.07));
    CHECK(best["rwd-v8-coupe"] == Catch::Approx(0.55).margin(0.04));
    CHECK(best["light-fwd-four"] == Catch::Approx(0.50).margin(0.04));
}
