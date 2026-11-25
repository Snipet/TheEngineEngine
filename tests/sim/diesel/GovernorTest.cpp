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

TEST_CASE("diesel governor: the boost compensator caps the fuel; the telemetry reads what is injected and its lambda", "[diesel][governor]")
{
    // Floored at 1300 rpm two seconds into the reference turbo's climb the rack is full and the air a little short
    // (the rack asks some phi 1.0): with the compensator (ecu.afc 0.8) the fuel injected is capped at an equivalence
    // ratio of 0.8, lambda 1.25; with it removed (0, the tuner's stop) the rack's whole ask goes in and what burns levels
    // toward phi 1 (measured 25 Nov: 16.3 against 20.2 g/s; lambda 1.25 against 1.11). tel.ecu.fuelFlow is the fuel
    // injected (it read the rack's ask before 25 Nov), tel.ecu.lambda the charge's on the reference's ECU (it read 0)
    double flow[2] = {0.0, 0.0}, lambda[2] = {0.0, 0.0};
    int k = 0;
    for (double afc : {0.8, 0.0})
    {
        Engine e("truck-15l-six");
        e.spec("ecu.afc", afc);
        e.start();
        e.dyno(1300.0, 1.0f);
        REQUIRE(e.run(2.0));
        CHECK(e.tel("ecu.rack") > 0.99);
        flow[k] = e.mean("ecu.fuelFlow", 0.2);
        lambda[k] = e.mean("ecu.lambda", 0.2);
        ++k;
    }
    std::printf("floored at 1300 rpm, 2 s in: with the compensator %.2f g/s, lambda %.2f; without it %.2f g/s, lambda %.2f\n",
                flow[0] * 1000.0, lambda[0], flow[1] * 1000.0, lambda[1]);
    CHECK(flow[0] < 0.9 * flow[1]);
    CHECK(lambda[0] > 1.2);
    CHECK(lambda[0] < 1.3);
    CHECK(lambda[1] > 0.95);
    CHECK(lambda[1] < 1.2);
}
