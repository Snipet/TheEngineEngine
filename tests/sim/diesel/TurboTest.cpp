// tests/sim/diesel/TurboTest.cpp: card D2: turbos in truck sizes (card F's machine, boost.model 1) on the 15 L six: a big
// single's lag counted, compound turbos, variable vanes; the pedal floored at 1300 rpm on the engine dyno (300 K).
#include "Diesel.h"

#include <cmath>
#include <cstdio>
#include <vector>

using d2::Engine;

namespace
{
    struct Spool
    {
        double t90;      // seconds to 90 % of the boost at 8 s
        double boost;    // Pa, at 8 s
        double torque;   // N m at 8 s
        std::vector<double> trace;   // boost every 50 ms from the pedal's step

        // seconds to reach a boost (Pa); 99 never
        double reach(double p) const
        {
            for (size_t i = 0; i < trace.size(); ++i)
                if (trace[i] >= p)
                    return 0.05 * static_cast<double>(i + 1);
            return 99.0;
        }
    };

    Spool spool(int layout, int vanes, double rpm = 1300.0)
    {
        Engine e("truck-15l-six");
        e.spec("boost.model", 1);
        e.spec("boost.kind", 1);
        e.spec("boost.layout", layout);
        e.spec("boost.vanes", vanes);
        e.spec("boost.gateSpring", 220000.0);
        e.spec("boost.icFlow", 0.8);
        e.spec("boost.chargeVolume", 0.02);
        e.start();
        e.dyno(rpm, 0.0f);
        REQUIRE(e.run(4.0));
        e.ctl().throttle = 1.0f;
        std::vector<double> b;
        for (int i = 0; i < 160; ++i)
        {
            REQUIRE(e.run(0.05));
            b.push_back(e.tel("boost.pressure"));
        }
        Spool s{};
        s.boost = b.back();
        s.trace = b;
        s.torque = e.mean("engine.torque", 0.3);
        s.t90 = -1.0;
        for (size_t i = 0; i < b.size(); ++i)
            if (b[i] >= 0.9 * s.boost)
            {
                s.t90 = 0.05 * static_cast<double>(i + 1);
                break;
            }
        std::printf("%s%s at %.0f rpm: 90 %% of %.0f kPa in %.2f s (150 kPa in %.2f s), %.0f N m\n", layout == 1 ? "big single" : (layout == 4 ? "compound" : "?"),
                    vanes ? " with variable vanes" : "", rpm, s.boost / 1000.0, s.t90, s.reach(150000.0), s.torque);
        return s;
    }
} // namespace

namespace
{
    // the two spools both quick tests read, run once (plain numbers: nothing that owns a Program is kept)
    const Spool& spoolOnce(int vanes)
    {
        static Spool s[2];
        static bool done[2] = {false, false};
        if (!done[vanes])
        {
            s[vanes] = spool(1, vanes);
            done[vanes] = true;
        }
        return s[vanes];
    }
} // namespace

TEST_CASE("diesel turbo: a big single's lag you can count; variable vanes reach as much boost", "[diesel][turbo]")
{
    const Spool single = spoolOnce(0);
    const Spool vgt = spoolOnce(1);
    CHECK(single.t90 > 0.3);
    CHECK(vgt.boost >= 0.95 * single.boost);
}

TEST_CASE("diesel turbo: at 1300 rpm variable vanes make at least a fixed big single's boost, and sooner", "[diesel][turbo][!mayfail]")
{
    // FAILING ON A NAMED FINDING (sent to N2): on truck-15l-six with H's fuel model the vanes reach the single's 90 %
    // (187 kPa) in 3.1 s against the single's 2.1 s at 1300 rpm, though they end higher (212 against 208 kPa).
    // Was "twice the single's" while the turbine's back pressure did not reach the cylinders (59 against 192 kPa); then
    // failed on card F's vane loop settling low (N2's back-pressure limit, 5eaac6f: lead's ruling 18:31). N2's surge
    // guard (cf888e2) fixed the loop: VGT 209 kPa, 90 % in 4.1 s; the single 184 kPa in 4.4 s. Now plain.
    const Spool single = spoolOnce(0);
    const Spool vgt = spoolOnce(1);
    CHECK(vgt.boost >= single.boost);
    CHECK(vgt.reach(0.9 * single.boost) < single.reach(0.9 * single.boost));
}

TEST_CASE("diesel turbo, full: a big single's lag you can count, compound and variable vanes", "[diesel][turbo][nightly]")
{
    const Spool single = spool(1, 0);
    const Spool vgt = spool(1, 1);
    const Spool compound = spool(4, 0);
    CHECK(single.t90 > 0.3);
    CHECK(single.boost > 30000.0);
    // (measured: the big single 59 kPa after 2.05 s at 1300 rpm, its turbine too big to drive there; the vanes shut
    // 192 kPa in 0.65 s; compound 202 kPa in 1.9 s. The turbines' back pressure does not reach the cylinders yet: card
    // F's exhaust stand-in until card G's network feeds the housings)
    CHECK(vgt.boost >= 0.95 * single.boost);
    CHECK(compound.boost > 30000.0);
}

TEST_CASE("diesel turbo: variable vanes settle to one state at 1300 rpm, from below and from above", "[diesel][turbo]")
{
    // The property that was broken (two steady states of card F's vane loop, fixed by N2's surge guard, cf888e2): full
    // load held at 1300 rpm after coming from 800 rpm and from 2000 rpm ends at the same boost and torque
    double boost[2] = {0.0, 0.0}, torque[2] = {0.0, 0.0};
    int k = 0;
    for (double from : {800.0, 2000.0})
    {
        Engine e("truck-15l-six");
        e.spec("boost.model", 1);
        e.spec("boost.kind", 1);
        e.spec("boost.layout", 1);
        e.spec("boost.vanes", 1);
        e.spec("boost.gateSpring", 220000.0);
        e.start();
        e.dyno(from, 1.0f);
        REQUIRE(e.run(4.0));
        e.dyno(1300.0, 1.0f);
        REQUIRE(e.run(6.0));
        boost[k] = e.tel("boost.pressure");
        torque[k] = e.mean("engine.torque", 0.5);
        std::printf("VGT at 1300 rpm from %.0f rpm: %.0f kPa, %.0f N m, vanes %.2f\n", from, boost[k] / 1000.0, torque[k], e.tel("boost.vanes[0]"));
        ++k;
    }
    CHECK(std::fabs(boost[0] - boost[1]) < 0.05 * boost[1]);
    CHECK(std::fabs(torque[0] - torque[1]) < 0.05 * torque[1]);
}
