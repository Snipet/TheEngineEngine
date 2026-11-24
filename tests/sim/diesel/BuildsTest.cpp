// tests/sim/diesel/BuildsTest.cpp: card D2's builds (content/builds/truck-*, diesel-*; each at 300 K, 101325 Pa): each
// starts, idles and makes its power on the engine dyno; and the tuned pickup rolls coal while its turbo spools.
#include "Diesel.h"

#include <algorithm>
#include <cstdio>

using d2::Engine;

TEST_CASE("diesel builds: each starts, idles, and makes power on the dyno", "[diesel][builds]")
{
    for (const char* name : {"truck-15l-six", "truck-15l-six-bobtail", "diesel-v8-truck", "diesel-mechanical-classic",
                             "diesel-pickup-tuned", "diesel-marine-v12"})
    {
        INFO(name);
        Engine e(name);
        CHECK(e.get("spec.env.temperature") == 300.0);
        e.start(2.0, 3.0);
        REQUIRE(e.tel("engine.state") == 2.0);
        const double idle = e.get("ecu.rpmAvg");
        const double red = e.tel("engine.redline") / 0.10471976;
        double best = 0.0, bestAt = 0.0, tqMax = 0.0;
        for (double x : {0.55, 0.7, 0.85, 0.97})
        {
            e.dyno(red * x, 1.0f);
            REQUIRE(e.run(5.0));
            const double tq = e.mean("engine.torque", 0.5);
            tqMax = std::max(tqMax, tq);
            if (tq * red * x > best)
            {
                best = tq * red * x;
                bestAt = red * x;
            }
        }
        best *= 0.10471976 / 1000.0;
        std::printf("%s: idles at %.0f rpm, redline %.0f; most torque %.0f N m, most power %.0f kW at %.0f rpm\n", name, idle, red,
                    tqMax, best, bestAt);
        CHECK(idle > 400.0);
        CHECK(idle < 900.0);
        CHECK(best > 100.0);
    }
}

TEST_CASE("diesel builds: the tuned pickup smokes while its turbo spools; with the limiter it does not", "[diesel][builds][smoke]")
{
    double peak[2] = {0.0, 0.0}, after[2] = {0.0, 0.0};
    for (int k = 0; k < 2; ++k)
    {
        Engine e("diesel-pickup-tuned");
        if (k == 1)
        {
            e.spec("ecu.afc", 0.8);
            e.spec("fuel.overFuel", 0.0);
        }
        e.start();
        e.dyno(1600.0, 0.0f);
        REQUIRE(e.run(3.0));
        e.ctl().throttle = 1.0f;     // floored
        double boostAtPeak = 0.0;
        for (int i = 0; i < 40; ++i)
        {
            REQUIRE(e.run(0.05));
            if (e.tel("ecu.smoke") > peak[k])
            {
                peak[k] = e.tel("ecu.smoke");
                boostAtPeak = e.tel("boost.pressure");
            }
        }
        REQUIRE(e.run(4.0));
        after[k] = e.tel("ecu.smoke");
        std::printf("pickup %s, floored at 1600 rpm: smoke %.1f FSN at %.0f kPa of boost as it spools, %.1f FSN once spooled (%.0f kPa)\n",
                    k == 0 ? "tuned (no limiter, +30 %)" : "stock limiter", peak[k], boostAtPeak / 1000.0, after[k],
                    e.tel("boost.pressure") / 1000.0);
    }
    CHECK(peak[0] > 4.0);
    CHECK(peak[0] > after[0] + 1.0);
    CHECK(peak[1] < 2.0);
}
