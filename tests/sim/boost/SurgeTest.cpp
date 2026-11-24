// tests/sim/boost/SurgeTest.cpp: card F, standalone tests 5 and 6: surge as the compression system's own
// oscillation when the throttle shuts on boost, and what a twin-scroll housing does with pulses.
#include "Rig.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdio>

using tee::test::boost::Rig;

namespace
{
    // On boost at 5000 rpm, then the throttle shut over 60 ms (a pedal let up), then `seconds` sampled every 0.5 ms:
    // how many times the compressor's flow turned back (crossed zero going down), and the last time it did
    struct Chop
    {
        int reversals = 0;
        double firstReverse = -1.0;
        double lastReverse = -1.0;
        int reversalsEarly = 0;   // in the first half second
        double minFlow = 0.0;
        double swingEarly = 0.0;  // the flow's swing (most less least), the first half second after the throttle shut
        double swingLate = 0.0;   // and the last half second
    };

    Chop chop(int blowOff, double seconds)
    {
        Rig rig;
        rig.set("spec.kind", 1);
        rig.set("spec.model", 1);
        rig.set("spec.blowOffKind", blowOff);
        rig.set("spec.gateSpring", 100000.0);
        rig.spec("rev", 0);
        rig.set("rpm", 5000);
        rig.set("throttle", 1.0);
        rig.set("pulsed", 1);
        rig.run(3.0);
        REQUIRE(rig.boost() > 80000.0);
        Chop c;
        double t = 0.0;
        double last = rig.get("boost.comp[0].mdot");
        for (int i = 0; i < 60; ++i)
        {
            rig.set("throttle", 1.0 - (i + 1) / 60.0);
            rig.run(0.001);
            t += 0.001;
        }
        double lo[2] = {1.0e9, 1.0e9};
        double hi[2] = {-1.0e9, -1.0e9};
        while (t < seconds)
        {
            rig.run(0.0005);
            t += 0.0005;
            const double f = rig.get("boost.comp[0].mdot");
            c.minFlow = f < c.minFlow ? f : c.minFlow;
            const int w = t < 0.56 ? 0 : (t > seconds - 0.5 ? 1 : -1);
            if (w >= 0)
            {
                lo[w] = f < lo[w] ? f : lo[w];
                hi[w] = f > hi[w] ? f : hi[w];
            }
            if (last >= 0.0 && f < 0.0)
            {
                ++c.reversals;
                if (c.firstReverse < 0.0)
                    c.firstReverse = t;
                c.lastReverse = t;
                if (t < 0.56)
                    ++c.reversalsEarly;
            }
            last = f;
        }
        c.swingEarly = hi[0] - lo[0];
        c.swingLate = hi[1] - lo[1];
        return c;
    }
} // namespace

TEST_CASE("boost: a shut throttle on boost with no blow-off valve surges between 5 and 40 Hz, and it dies away",
          "[boost][surge]")
{
    const Chop c = chop(0, 6.0);
    // the cycles' rate over the first half second after the throttle shut
    const double hz = c.reversalsEarly / 0.5;
    std::printf("no valve: %d reversals, %.1f a second at first, the first at %.3f s, the last at %.3f s, least flow "
                "%.4f kg/s; the flow's swing %.4f kg/s at first, %.4f six seconds on\n",
                c.reversals, hz, c.firstReverse, c.lastReverse, c.minFlow, c.swingEarly, c.swingLate);
    CHECK(c.reversals >= 4);
    CHECK(c.minFlow < -0.02);
    CHECK(hz >= 5.0);
    CHECK(hz <= 40.0);
    // and it dies away as the rotor slows (Greitzer's B falls with the tip speed): six seconds on, the swing is a tenth
    // of what it was
    CHECK(c.swingLate < 0.1 * c.swingEarly);
}

TEST_CASE("boost: a shut throttle on boost with a blow-off valve does not reverse the flow", "[boost][surge]")
{
    for (int kind : {1, 2})
    {
        const Chop c = chop(kind, 2.0);
        std::printf("valve %s: %d reversals, least flow %.4f kg/s\n", kind == 1 ? "to the air" : "recirculating",
                    c.reversals, c.minFlow);
        INFO("blow-off valve kind " << kind);
        CHECK(c.reversals == 0);
        CHECK(c.minFlow >= 0.0);
    }
}

TEST_CASE("boost: pulses into a twin-scroll housing give more shaft power at low speed than the same flow into one",
          "[boost][scroll]")
{
    double power[2] = {0.0, 0.0};
    double flow[2] = {0.0, 0.0};
    for (int scrolls : {1, 2})
    {
        Rig rig;
        rig.set("spec.kind", 1);
        rig.set("spec.model", 1);
        rig.set("spec.scrolls", scrolls);
        rig.set("spec.gateSpring", 300000.0);
        rig.spec("rev", 0);
        rig.set("rpm", 2000);
        rig.set("throttle", 1.0);
        rig.set("pulsed", 1);
        // the rotor held at one speed, so both see the same compressor and the same boost
        rig.set("boost.shaft[0].hold", 6000.0);
        rig.run(2.0);
        const double e0 = rig.get("boost.sumPt[0]");
        rig.clear();
        rig.run(1.0);
        power[scrolls - 1] = rig.get("boost.sumPt[0]") - e0;
        flow[scrolls - 1] = rig.mean("sMEx");
        std::printf("%s scroll at 2000 rpm: turbine %.1f W from %.5f kg/s; boost %.1f kPa\n",
                    scrolls == 1 ? "single" : "twin", power[scrolls - 1], flow[scrolls - 1], rig.mean("sBoost") / 1000.0);
    }
    CHECK(flow[1] == Catch::Approx(flow[0]).epsilon(0.01));
    CHECK(power[1] > 1.05 * power[0]);
}
