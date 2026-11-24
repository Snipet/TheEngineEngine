// tests/sim/vehicle/EngineTest.cpp: the vehicle model (car.model 1) driven by the whole engine (sim/engine.fm) through
// card C's tee::test::Scenario: card I's integrated tests. (car.model 0 is the reference to the bit: the null test,
// tests/sim/NullTest.cpp, runs with it.)
//
// The fast tier's budget (01-rules.md §3; moved by the build steward at the lead's leave, no assertion or bound changed):
// the quarter mile's quick form runs the light front-drive four alone, its full form every archetype [nightly].
#include "tee/test/Scenario.h"

#include <initializer_list>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <vector>
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
    s.at(0.0).spec("env.temperature", 300.0);     // (the v8 build stands at -40 C: the launch is in 300 K air)
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
    s.at(0.0).spec("env.temperature", 300.0);     // (the v8 build stands at -40 C: the coast-down is in 300 K air)
    s.at(2.5).lever(1);
    s.at(2.8).throttle(1.0).ramp(0.1);
    s.at(12.0).throttle(0.0).lever(0);
    const tee::test::Run r = s.run(22.0);
    REQUIRE(r.ok());
    const auto v = r.tel("car.speed");
    const double v0 = v.at(13.0), v1 = v.at(21.0);
    // the hatch: 1150 kg, CdA 0.63, street tyres (0.010, rising with v^2 / 19600), wheels of 0.9 kg m^2 at 0.31 m
    // the air's density from spec.env (101325 Pa, 300 K): 1.177 kg/m^3
    const double rho = 101325.0 / (287.05 * 300.0);
    const double m = 1150.0, c = 0.010 * m * 9.81, k = 0.5 * rho * 0.63 + c * 0.00005102;
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

namespace
{
    // the body of "engine.quartermile:...": every case of `cases` (the quick form takes the first, the full form all)
    void caseOf_engine_quartermile_(std::initializer_list<const char*> cases)
{
    // Each archetype at its own mass with an engine of its kind (the reference's layouts), on drag radials (tyres that hook), through an automatic
    // (the rule of thumb's cars shift themselves): a burnout on the line lock to warm them, staged at 6 s, the green at
    // 8.5 s, the brakes off and the pedal brought down over 0.6 s (a driver who keeps them hooked). The rule of thumb
    // (ET = 5.825 (lb/hp)^(1/3) s, trap = 234 (hp/lb)^(1/3) mph) takes the engine's power: here the 90th percentile of
    // what the wheels saw from the green to the finish, over the gears' 0.97 (a peak would take the shafts' swing).
    for (const std::string car : cases)
    {
        tee::test::Scenario s = started("default");
        withCar(s, car);
        // each with the engine its kind has: a 1.6 L four, a 5.7 L V8, a 3 L six with a turbo
        if (car == "light-fwd-four")
            s.at(0.0).spec("engine.layout", "Inline 4").spec("engine.displacement", 0.0016);
        else if (car == "rwd-v8-coupe")
            s.at(0.0).spec("engine.layout", "V8 crossplane").spec("engine.displacement", 0.0057);
        else
            s.at(0.0).spec("engine.layout", "Inline 6").spec("engine.displacement", 0.003).spec("boost.kind", "Turbo")
                .spec("boost.level", 0.6);
        s.at(0.0).spec("car.tyre", "Drag radial").spec("car.gearbox", "Automatic").spec("car.abs", 0.0);
        s.at(2.3).lever(1).buttons(1);
        s.at(2.5).throttle(1.0);
        s.at(5.0).throttle(0.0).buttons(0).brake(1.0);
        s.at(6.0).strip();
        s.at(8.5).brake(0.0).throttle(0.15);
        s.at(8.5).throttle(1.0).ramp(0.6);
        const tee::test::Run r = s.run(28.0);
        REQUIRE(r.ok());
        const auto doc = nlohmann::json::parse(
            slurp(std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "cars" / (car + ".json")));
        const double kg = doc["spec"]["car"]["mass"].get<double>();
        const double t = r.tel("car.t1320").last(), v = r.tel("car.v1320").last() / 0.44704;
        std::vector<double> p = r.tel("car.wheelPower").during(8.5, 8.5 + t).values();
        REQUIRE(!p.empty());
        std::sort(p.begin(), p.end());
        const double watts = p[static_cast<std::size_t>(0.9 * static_cast<double>(p.size() - 1))] / 0.97;
        const double lb = kg * 2.20462, hp = watts / 745.7;
        const double et = 5.825 * std::cbrt(lb / hp), mph = 234.0 * std::cbrt(hp / lb);
        std::printf("quarter mile, %s (%.0f kg, %.0f kW): rt %.3f, 60 ft %.3f, ET %.3f s (rule %.3f), trap %.1f mph (rule "
                    "%.1f)\n",
                    car.c_str(), kg, watts / 1000.0, r.tel("car.tReaction").last(), r.tel("car.t60").last(), t, et, v, mph);
        CHECK(r.tel("car.stripState").last() == 5.0);
        CHECK(t == Catch::Approx(et).epsilon(0.08));
        CHECK(v == Catch::Approx(mph).epsilon(0.08));
    }
}
} // namespace

// The quick form, in every run (01-rules.md §3, the build steward at the lead's leave): the light front-drive four alone.
TEST_CASE("engine.quartermile: each archetype's quarter mile within 8 % of what weight and power predict (quick: the light front-drive four alone)", "[vehicle][engine]")
{
    caseOf_engine_quartermile_({"light-fwd-four"});
}

// The full form, nightly: every case.
TEST_CASE("engine.quartermile: each archetype's quarter mile within 8 % of what weight and power predict", "[vehicle][engine][nightly]")
{
    caseOf_engine_quartermile_({"light-fwd-four", "rwd-v8-coupe", "awd-turbo-six"});
}
