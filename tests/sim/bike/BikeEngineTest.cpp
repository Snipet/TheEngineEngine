// tests/sim/bike/BikeEngineTest.cpp: card B2's integrated tests: the motorcycle builds (content/builds/bike-*.json)
// through the whole engine (sim/engine.fm) with card C's tee::test::Scenario.
#include "tee/test/Scenario.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    std::string slurp(const std::filesystem::path& p)
    {
        std::ifstream in(p, std::ios::binary);
        std::ostringstream s;
        s << in.rdbuf();
        return s.str();
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

    // The share of the samples in [from, to] whose flags have `bit`
    double share(const tee::test::Series& flags, double from, double to, int bit)
    {
        const auto& t = flags.times();
        const auto& f = flags.values();
        int n = 0, k = 0;
        for (std::size_t i = 0; i < t.size(); ++i)
            if (t[i] >= from && t[i] <= to)
            {
                ++n;
                k += (bits(f[i]) & bit) != 0 ? 1 : 0;
            }
        return n > 0 ? static_cast<double>(k) / n : 0.0;
    }

    // A dumped clutch: in first with the lever in, the throttle open (the limiter holds the revs), the lever out over
    // `release` s at 3.5 s
    tee::test::Run launch(const std::string& build, double release, double seconds)
    {
        tee::test::Scenario s = started(build);
        s.at(2.3).clutch(1.0);
        s.at(2.5).shiftUp();
        s.at(2.7).throttle(1.0).ramp(0.2);
        s.at(3.5).clutch(0.0).ramp(release);
        return s.run(seconds);
    }
} // namespace

TEST_CASE("bike.engine.launch: through the engine, the superbike's dumped clutch all but lifts its front; the cruiser's "
          "spins its rear with the front down", "[bike][engine]")
{
    const tee::test::Run sb = launch("bike-superbike", 0.15, 6.0);
    REQUIRE(sb.ok());
    const tee::test::Run cr = launch("bike-cruiser", 0.15, 6.0);
    REQUIRE(cr.ok());
    const double sbLift = share(sb.tel("car.flags"), 3.55, 4.5, 2);
    const double crLift = share(cr.tel("car.flags"), 3.55, 4.5, 2);
    const double crGrip = share(cr.tel("car.flags"), 3.55, 4.5, 1 | 128);
    std::printf("engine launch, superbike: lifted %.0f %% of 3.55-4.5 s, lift at most %.1f deg, front load least %.0f N, "
                "wheelie limit %.1f m/s^2, accel at most %.1f, engine %.0f to %.0f rad/s; bike flags up to %d\n",
                100.0 * sbLift, sb.tel("car.liftAngle").during(3.5, 4.5).max(),
                sb.tel("car.wheelLoad[0]").during(3.6, 4.5).min(), sb.tel("car.wheelieLimit").last(),
                sb.tel("car.accel").during(3.5, 4.5).max(), sb.tel("engine.speed").during(3.5, 4.5).min(),
                sb.tel("engine.speed").during(3.5, 4.5).max(), bits(sb.tel("car.bikeFlags").during(3.5, 4.5).max()));
    std::printf("engine launch, cruiser: lifted %.0f %%, traction-limited or spinning %.0f %% of 3.55-4.5 s; front load "
                "least %.0f N; accel at most %.1f m/s^2 (wheelie limit %.1f); speed at 6 s %.1f m/s\n", 100.0 * crLift,
                100.0 * crGrip, cr.tel("car.wheelLoad[0]").during(3.6, 4.5).min(), cr.tel("car.accel").during(3.5, 4.5).max(),
                cr.tel("car.wheelieLimit").last(), cr.tel("car.speed").last());
    // (the engine model gives this 998 cc four about 74 kW where a superbike's makes 140: in first its tyre cannot push
    // the bike to its wheelie limit, so here the front only comes near lifting. With the class's power the launch is
    // the wheelie's: BikeTest.cpp's bike.launch, ClassTest.cpp's superbike)
    const double frontStatic = 0.5 * 9.81 * (201.0 + 80.0);
    CHECK(sb.tel("car.wheelLoad[0]").during(3.6, 4.5).min() < 0.3 * frontStatic);
    CHECK(sb.tel("car.accel").during(3.5, 4.5).max() > 0.8 * sb.tel("car.wheelieLimit").last());
    CHECK(sb.tel("car.liftAngle").during(3.5, 6.0).max() < 20.0);
    CHECK((bits(sb.tel("car.bikeFlags").during(3.5, 6.0).max()) & 2) == 0);     // never looped
    CHECK(crLift == 0.0);
    CHECK(crGrip > 0.5);
    CHECK(cr.tel("car.wheelLoad[0]").during(3.6, 4.5).min() > 500.0);
    CHECK(sb.audio().finite());
    CHECK(cr.audio().finite());
    CHECK(sb.audio().peak() <= 1.0f);
}

TEST_CASE("bike.engine.shift: through the engine, a quickshift cuts the ECU's fuel for its 50 ms and the engine drops "
          "to the next gear's speed; an auto-blipped downshift raises it; no stall, no over-rev", "[bike][engine]")
{
    tee::test::Scenario s = started("bike-superbike");
    s.at(0.0).spec("car.wheelieControl", "By the clutch");
    s.at(2.3).clutch(1.0);
    s.at(2.5).shiftUp();
    s.at(2.7).throttle(0.5);
    s.at(3.0).clutch(0.0).ramp(0.8);
    s.at(3.0).throttle(1.0).ramp(0.5);
    s.at(4.5).shiftUp();
    s.at(5.5).shiftUp();
    s.at(7.0).throttle(0.0).ramp(0.05);
    s.at(7.4).shiftDown();
    const tee::test::Run r = s.run(8.5);
    REQUIRE(r.ok());
    const auto w = r.tel("engine.speed");
    // (the engine's speed over the road's, before the upshift and once the shunt has settled)
    const auto v = r.tel("car.speed");
    const double wBefore = w.at(5.45) / v.at(5.45);
    const double wAfter = w.at(5.85) / v.at(5.85);
    const double wDown0 = w.during(7.25, 7.395).min();
    const double wDown1 = w.during(7.40, 7.60).max();
    const double red = r.tel("engine.redline").last();
    std::printf("engine shift: gears %.0f -> %.0f -> %.0f; up 2-3: engine over road %.1f -> %.1f rad/m (ratio 1.67/2.00 = 0.835: "
                "%.3f); car flags %d, engine flags %d in the cut; quickshifts %.0f; down 3-2: %.0f -> %.0f rad/s (2.00/1.67 "
                "= 1.198: %.3f); blips %.0f; engine %.0f to %.0f rad/s from 3.5 s, redline %.0f rad/s\n",
                r.tel("car.gear").at(5.4), r.tel("car.gear").at(6.9), r.tel("car.gear").last(), wBefore, wAfter,
                wAfter / wBefore, bits(r.tel("car.flags").during(5.5, 5.56).max()),
                bits(r.tel("engine.flags").during(5.5, 5.56).max()), r.tel("car.quickshifts").last(), wDown0, wDown1,
                wDown1 / wDown0, r.tel("car.blips").last(), w.during(3.5, 8.5).min(), w.during(3.5, 8.5).max(), red);
    CHECK(r.tel("car.gear").at(6.9) == 3.0);
    CHECK(r.tel("car.gear").last() == 2.0);
    CHECK(r.tel("car.quickshifts").last() == 2.0);
    CHECK(r.tel("car.blips").last() == 1.0);
    CHECK((bits(r.tel("car.flags").during(5.5, 5.56).max()) & 32) != 0);    // the cut asked for
    CHECK((bits(r.tel("engine.flags").during(5.5, 5.56).max()) & 2) != 0);  // and the ECU's fuel cut on
    CHECK((bits(r.tel("engine.flags").during(5.62, 6.9).max()) & 2) == 0);  // and off again
    // (in second the clutch was slipping to hold the rear to its grip; settled in third the engine turns as the gear and
    // the road say, but for the tyre's few per cent of slip)
    CHECK(wAfter * 0.31 / r.tel("car.ratio").at(5.85) == Catch::Approx(1.0).margin(0.08));
    CHECK(r.tel("car.ratio").at(5.85) / r.tel("car.ratio").at(5.45) == Catch::Approx(1.67 / 2.00).margin(0.001));
    CHECK(wDown1 / wDown0 > 1.1);
    CHECK(w.during(3.5, 8.5).min() > 100.0);                                // no stall
    CHECK(w.during(3.5, 8.5).max() < 1.03 * red);                          // no over-rev
}

TEST_CASE("bike.engine.redline: the 600 revs to its 16,000 rpm limiter at 96 kHz, its sound finite and within range",
          "[bike][engine]")
{
    tee::test::Scenario s = started("bike-supersport-600");
    s.at(2.5).throttle(1.0).ramp(0.1);
    s.at(5.0).throttle(0.0);
    const tee::test::Run r = s.rate(96000.0).run(6.0);
    REQUIRE(r.ok());
    const double top = r.tel("engine.speed").during(3.0, 5.0).max() * 9.5492966;
    std::printf("600 in neutral: %.0f rpm at most (redline %.0f rad/s); audio peak %.3f; firing %.0f Hz\n", top,
                r.tel("engine.redline").last(), r.audio().peak(), r.firingHz(4.0, 5.0));
    CHECK(top > 0.97 * 16000.0);
    CHECK(top < 1.03 * 16000.0);
    CHECK(r.audio().finite());
    CHECK(r.audio().peak() <= 1.0f);
}

TEST_CASE("bike.engine.firing: each bike engine, running, fires at the angles its geometry's arithmetic gives",
          "[bike][engine]")
{
    const std::filesystem::path dir = std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "engines";
    int seen = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir))
    {
        const std::string name = entry.path().stem().string();
        if (name.rfind("bike-", 0) != 0)
            continue;
        const auto doc = nlohmann::json::parse(slurp(entry.path()));
        std::vector<double> want = doc["expect"]["firingAngles"].get<std::vector<double>>();
        tee::test::Scenario s = started("builds/engines/" + name + ".json");
        const tee::test::Run r = s.run(3.0);
        REQUIRE(r.ok());
        std::vector<double> got;
        for (std::size_t i = 0; i < want.size(); ++i)
            got.push_back(r.tel("engine.firingAngle[" + std::to_string(i) + "]").last());
        std::sort(got.begin(), got.end());
        std::sort(want.begin(), want.end());
        std::string line;
        for (double g : got)
            line += " " + std::to_string(static_cast<int>(std::lround(g)));
        std::printf("firing, %s:%s (%.0f rpm idling)\n", name.c_str(), line.c_str(),
                    r.tel("engine.speed").last() * 9.5492966);
        INFO(name);
        for (std::size_t i = 0; i < want.size(); ++i)
            CHECK(got[i] == Catch::Approx(want[i]).margin(0.01));
        CHECK(r.tel("engine.speed").last() > 50.0);     // it runs
        CHECK(r.audio().finite());
        ++seen;
    }
    CHECK(seen == 13);
}

TEST_CASE("bike.engine.lash: through the engine, the driveline's play closes as an event on and off the throttle",
          "[bike][engine]")
{
    tee::test::Scenario s = started("bike-naked-twin");
    s.at(2.3).clutch(1.0);
    s.at(2.5).shiftUp();
    s.at(2.7).throttle(0.3);
    s.at(3.0).clutch(0.0).ramp(1.0);
    s.at(4.5).shiftUp();
    s.at(5.5).throttle(0.0);
    s.at(6.5).throttle(0.4);
    s.at(7.5).throttle(0.0);
    s.at(8.5).throttle(0.4);
    const tee::test::Run r = s.run(9.5);
    REQUIRE(r.ok());
    const auto lash = r.tel("car.chainLash");
    const auto side = r.tel("car.chainLashSide");
    std::printf("engine lash: events %.0f by 5.4 s, %.0f by 6.4, %.0f by 7.4, %.0f by 8.4, %.0f at the end; sides %.0f %.0f "
                "%.0f %.0f; last closing %.2f rad/s\n", lash.at(5.4), lash.at(6.4), lash.at(7.4), lash.at(8.4),
                lash.last(), side.at(6.4), side.at(7.4), side.at(8.4), side.last(), r.tel("car.chainLashSpeed").last());
    CHECK(lash.at(6.4) > lash.at(5.4));
    CHECK(lash.at(7.4) > lash.at(6.4));
    CHECK(lash.at(8.4) > lash.at(7.4));
    CHECK(lash.last() > lash.at(8.4));
    CHECK(side.at(6.4) == 1.0);
    CHECK(side.at(7.4) == 0.0);
    CHECK(side.at(8.4) == 1.0);
    CHECK(side.last() == 0.0);
}
