// tests/sim/diesel/GovernorTest.cpp: card D2: the governors (spec.ecu.governor): the droop line, the hunting idle of a
// mechanical governor against an electronic one's steadiness, the truck six at 300 K, 101325 Pa.
#include "Diesel.h"

#include <algorithm>
#include <cstdio>

using d2::Engine;

TEST_CASE("diesel governor: the droop line: full rack at the rated speed, none at high idle", "[diesel][governor]")
{
    // The rack on the engine dyno, the pedal down, at speeds across the droop: rated speed (the redline) gives full
    // rack, high idle (rated x (1 + droop)) none, half way half (the electronic governor's droop line, no lag)
    for (int kind : {1, 2})
    {
        Engine e("truck-15l-six");
        e.spec("ecu.governor", kind);
        e.start();
        // (the ECU's redline moves a little as the engine warms: read it after a few seconds at full load)
        e.dyno(e.tel("engine.redline") * 0.9 / 0.10471976, 1.0f);
        REQUIRE(e.run(4.0));
        const double droop = 0.06;
        double rack[3] = {};
        int k = 0;
        for (double x : {-1.1, -0.5, 0.05})
        {
            // (the ECU's redline, the rated speed, moves a little with the engine's state: the governor's high idle read
            // each time, hd.highIdle = rated x (1 + droop))
            const double hi = e.get("hd.highIdle");
            const double rated = hi / (1.0 + droop);
            e.dyno((hi + x * droop * rated) / 0.10471976, 1.0f);
            REQUIRE(e.run(2.0));
            rack[k] = e.mean("ecu.rack", 0.5);
            std::printf("  high idle %.0f rpm; at %.0f rpm, rack %.2f\n", hi / 0.10471976, e.tel("engine.speed") / 0.10471976, rack[k]);
            ++k;
        }
        const double red = e.tel("engine.redline");
        std::printf("%s governor, rated %.0f rpm: rack %.2f at 0.97 of it, %.2f half way up the droop, %.2f past high idle\n",
                    kind == 1 ? "mechanical" : "electronic", red / 0.10471976, rack[0], rack[1], rack[2]);
        CHECK(rack[0] > 0.95);
        CHECK(rack[1] > 0.35);
        CHECK(rack[1] < 0.65);
        CHECK(rack[2] < 0.05);
    }
}

TEST_CASE("diesel governor: a mechanical governor's idle hunts, an electronic one's is steady", "[diesel][governor]")
{
    double spread[3] = {};
    for (int kind : {1, 2})
    {
        Engine e("truck-15l-six");
        e.spec("ecu.governor", kind);
        e.start(2.0, 6.0);
        double lo = 1e9, hi = 0.0, sum = 0.0;
        const int calls = static_cast<int>(4.0 * e.rate / 512.0);
        for (int i = 0; i < calls; ++i)
        {
            REQUIRE(e.run(512.0 / e.rate));
            const double rpm = e.get("ecu.rpmAvg");     // (the ECU's smoothed speed: the firings' ripple out)
            lo = std::min(lo, rpm);
            hi = std::max(hi, rpm);
            sum += rpm;
        }
        spread[kind] = hi - lo;
        std::printf("%s governor at idle: %.0f rpm mean, %.0f to %.0f (%.0f rpm swing)\n", kind == 1 ? "mechanical" : "electronic",
                    sum / calls, lo, hi, hi - lo);
        CHECK(e.tel("engine.state") == 2.0);
    }
    // (a mechanical governor's idle: tens of rpm of slow swing, "galloping"; an electronic one's a few)
    CHECK(spread[1] > 15.0);
    CHECK(spread[1] > 2.0 * spread[2]);
    CHECK(spread[2] < 25.0);
}
