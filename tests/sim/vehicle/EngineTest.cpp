// tests/sim/vehicle/EngineTest.cpp: the vehicle model (car.model 1) driven by the whole engine (sim/engine.fm) through
// card C's tee::test::Scenario: card I's integrated tests. (car.model 0 is the reference to the bit: the null test,
// tests/sim/NullTest.cpp, runs with it.)
#include "tee/test/Scenario.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <nlohmann/json.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
    std::string slurp(const std::filesystem::path& p)
    {
        std::ifstream in(p, std::ios::binary);
        std::ostringstream s;
        s << in.rdbuf();
        return s.str();
    }

    // A car build's fields written into a Scenario's Spec at time 0 (content/builds/cars/<name>.json)
    tee::test::Scenario& withCar(tee::test::Scenario& s, const std::string& name)
    {
        const auto doc = nlohmann::json::parse(
            slurp(std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "cars" / (name + ".json")));
        s.at(0.0);
        for (const auto& [key, value] : doc["spec"]["car"].items())
        {
            if (value.is_string())
                s.spec("car." + key, value.get<std::string>());
            else if (value.is_boolean())
                s.spec("car." + key, value.get<bool>() ? 1.0 : 0.0);
            else
                s.spec("car." + key, value.get<double>());
        }
        return s;
    }

    // Started, in drive mode, in neutral: the engine idling by 2.5 s
    tee::test::Scenario started(const std::string& build)
    {
        tee::test::Scenario s(build);
        s.at(0.0).mode(1).lever(0).clutch(0.0).brake(0.0).throttle(0.0).ignition(tee::On);
        s.at(0.25).ignition(tee::Start).until(2.0).ignition(tee::On);
        return s;
    }

    int bits(double flags) { return static_cast<int>(flags); }
} // namespace

TEST_CASE("engine.launch: a strong rear-drive car on street tyres spins them in first, and hooks up in the higher gears",
          "[vehicle][engine]")
{
    // the reference's V8 (its 6.2 L) in the rear-drive coupe on street tyres, the paddles' robotised clutch
    tee::test::Scenario s = started("v8");
    withCar(s, "rwd-v8-coupe");
    s.at(0.0).spec("car.tyre", "Street");
    s.at(2.5).shiftUp();
    s.at(2.8).throttle(1.0).ramp(0.1);
    s.at(5.5).shiftUp();
    s.at(7.5).shiftUp();
    const tee::test::Run r = s.run(10.0);
    REQUIRE(r.ok());
    // (a run keeps telemetry's scalars only, so the spin is read from the flags: bit 1, a driven tyre past 1.5 times
    // its peak slip)
    const auto flags = r.tel("car.flags");
    REQUIRE(!flags.empty());
    double spinning = 0.0, third = 0.0;
    const auto& t = flags.times();
    const auto& f = flags.values();
    for (std::size_t i = 1; i < t.size(); ++i)
    {
        if (t[i] > 3.0 && t[i] < 5.4 && (bits(f[i]) & 1) != 0)
            spinning += t[i] - t[i - 1];
        if (t[i] > 8.5 && (bits(f[i]) & 1) != 0)
            third += t[i] - t[i - 1];
    }
    std::printf("launch: spinning %.2f s of first's 2.4, %.2f s of third's last 1.5; speed %.1f m/s; peak wheel power "
                "%.0f kW\n", spinning, third, r.tel("car.speed").last(), r.tel("car.wheelPower").max() / 1000.0);
    CHECK(r.tel("car.gear").during(3.0, 5.4).max() == 1.0);
    CHECK(spinning > 0.3);
    CHECK(r.tel("car.gear").last() == 3.0);
    CHECK(third == 0.0);
    CHECK(r.tel("car.speed").last() > 20.0);
    CHECK(r.audio().finite());
    CHECK(r.audio().peak() <= 1.0f);
}

TEST_CASE("engine.shift: a dog box's upshift cuts the ignition through the ECU; a downshift blips the engine up",
          "[vehicle][engine]")
{
    tee::test::Scenario s = started("v8");
    withCar(s, "rwd-v8-coupe");
    s.at(0.0).spec("car.gearbox", "Sequential");
    s.at(2.5).shiftUp();
    s.at(2.8).throttle(0.6).ramp(0.2);
    s.at(5.0).shiftUp();
    s.at(7.0).throttle(0.0).ramp(0.05);
    s.at(7.3).shiftDown();
    const tee::test::Run r = s.run(8.5);
    REQUIRE(r.ok());
    const auto cut = r.tel("car.flags").during(4.95, 5.2);
    const auto fuel = r.tel("engine.flags").during(4.95, 5.2);
    std::printf("shift: car flags up to %d, engine flags up to %d during the upshift; engine %.0f rad/s before the "
                "downshift, %.0f during it\n",
                bits(cut.max()), bits(fuel.max()), r.tel("engine.speed").during(7.1, 7.3).min(),
                r.tel("engine.speed").during(7.3, 7.5).max());
    CHECK((bits(cut.max()) & 32) != 0);           // the cut asked for
    CHECK((bits(fuel.max()) & 2) != 0);           // and the ECU's fuel cut on
    CHECK(r.tel("car.gear").during(5.3, 7.0).min() == 2.0);
    CHECK((bits(r.tel("engine.flags").during(5.3, 6.9).max()) & 2) == 0);   // released
    CHECK(r.tel("engine.speed").during(7.3, 7.5).max() > 1.1 * r.tel("engine.speed").during(7.1, 7.3).min());
    CHECK(r.tel("car.gear").last() == 1.0);
}

TEST_CASE("engine.coastdown: in neutral the car slows as its drag and rolling say", "[vehicle][engine]")
{
    tee::test::Scenario s = started("v8");
    withCar(s, "light-fwd-four");
    s.at(0.0).spec("car.gearbox", "Automatic");
    s.at(2.5).lever(1);
    s.at(2.8).throttle(1.0).ramp(0.1);
    s.at(12.0).throttle(0.0).lever(0);
    const tee::test::Run r = s.run(22.0);
    REQUIRE(r.ok());
    const auto v = r.tel("car.speed");
    const double v0 = v.at(13.0), v1 = v.at(21.0);
    // the hatch: 1150 kg, CdA 0.63, street tyres (0.010, rising with v^2 / 19600), wheels of 0.9 kg m^2 at 0.31 m
    const double m = 1150.0, c = 0.010 * m * 9.81, k = 0.5 * 1.2 * 0.63 + c * 0.00005102;
    const double mEff = m + 4.0 * 0.9 / (0.31 * 0.31);
    const double rr = std::sqrt(c / k);
    const double analytic = rr * std::tan(std::atan(v0 / rr) - std::sqrt(k * c) * 8.0 / mEff);
    std::printf("coast-down in the engine: %.3f m/s to %.3f in 8 s (analytic %.3f)\n", v0, v1, analytic);
    CHECK(v0 > 25.0);
    CHECK(v1 == Catch::Approx(analytic).epsilon(0.01));
}

TEST_CASE("engine.stall: a clutch dumped at idle in a tall gear stalls the engine, as the reference does",
          "[vehicle][engine]")
{
    tee::test::Scenario s = started("v8");
    withCar(s, "rwd-v8-coupe");
    s.at(2.5).clutch(1.0);
    s.at(3.0).lever(4);
    s.at(3.5).clutch(0.0);
    const tee::test::Run r = s.run(6.0);
    REQUIRE(r.ok());
    std::printf("stall: engine state %g, speed %.1f rad/s, gear %g\n", r.tel("engine.state").last(),
                r.tel("engine.speed").last(), r.tel("car.gear").last());
    CHECK(r.tel("engine.state").during(2.4, 3.4).min() == 2.0);
    CHECK(r.tel("engine.state").last() != 2.0);
    CHECK(r.tel("engine.speed").last() < 10.0);
}
