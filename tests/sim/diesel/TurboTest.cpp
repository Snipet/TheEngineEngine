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
        e.spec("boost.target", 260000.0);   // (the fixture's boost target: the tractor archetype's, the same 15 L class on the same valve events; that archetype's judgement. A VGT on the spring law with no target limit-cycles, N2: nothing shipped uses it)
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

TEST_CASE("diesel turbo: at 1300 rpm variable vanes hold a fixed big single's boost, and get there sooner", "[diesel][turbo]")
{
    // (marked [!mayfail] for an hour on 24 Nov: with the exhaust's mean flow closed (G aaedbf4) the VGT took 4.35 s to
    // the single's 90 % against 2.80 s, N2's surge guard parking it on the surge line; N2's guard giving back 0.1 of
    // its rate in place of 0.3: 2.60 against 2.80 s. Plain again.)
    // (marked [!mayfail] while the vanes lagged the single on H's fuel model: 3.1 against 2.1 s to 187 kPa; N2's
    // e12b230, the surge guard retuned, gives 1.75 against 1.95 s, 211 against 208 kPa. The measure, agreed with N2:
    // the same absolute boost for both, 90 % of the single's final)
    // Was "twice the single's" while the turbine's back pressure did not reach the cylinders (59 against 192 kPa); then
    // failed on card F's vane loop settling low (N2's back-pressure limit, 5eaac6f: lead's ruling 18:31). N2's surge
    // guard (cf888e2) fixed the loop: VGT 209 kPa, 90 % in 4.1 s; the single 184 kPa in 4.4 s. Now plain.
    const Spool single = spoolOnce(0);
    const Spool vgt = spoolOnce(1);
    // Both machines are closed-loop on one boost target, so two controllers holding it agree to the loop's tolerance:
    // 2 % here, a stated judgement (lead, 2026-11-24; N2: the VGT's surge guard holds the vanes near 0.8, 0.25-0.5 %
    // under fully open at 1300 rpm). It was ">= single" until D's gamma (8b20b176) made it a 0.6 % near-tie, 210.1
    // against 211.4 kPa. The VGT's advantage is the time, which the next line holds.
    CHECK(vgt.boost >= 0.98 * single.boost);
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
    // The property (two steady states of card F's vane loop, fixed by N2's surge guard, cf888e2): full load held at
    // 1300 rpm after coming from below and from above ends at the same boost and torque. On the footing of card N2's
    // GateTest (its short form, the lead's ruling: one state, so each run is held until settled): the cooler and pipes
    // the truck's size (icFlow 0.8, chargeVolume 0.02), the exhaust's and the cooler's heat settled 1 s into each speed,
    // held warm, 8 s of warm-up and 10 s at each of 1100 (or 2000) and 1300 rpm, read over 2 s: 79 against 77 kPa (N2's
    // 78.1 / 75.9). (From 800 the climb takes some 11 s to 90 %: the
    // surge guard holds the vanes open while the compressor sits left of its flag; N2's [nightly] case holds that.)
    // (Marked [!mayfail] 24-25 Nov since G's 15fcdd84: read 6 s after the change of speed with the exhaust's walls
    // cold, it read the climb: 11 against 63 kPa, then 14 against 46.)
    double boost[2] = {0.0, 0.0}, torque[2] = {0.0, 0.0};
    int k = 0;
    for (double from : {1100.0, 2000.0})
    {
        Engine e("truck-15l-six");
        e.spec("boost.model", 1);
        e.spec("boost.kind", 1);
        e.spec("boost.layout", 1);
        e.spec("boost.vanes", 1);
        e.spec("boost.gateSpring", 220000.0);
        e.spec("boost.target", 260000.0);   // (the fixture's boost target: the tractor archetype's, the same 15 L class on the same valve events; that archetype's judgement. A VGT on the spring law with no target limit-cycles, N2: nothing shipped uses it)
        e.spec("boost.icFlow", 0.8);
        e.spec("boost.chargeVolume", 0.02);
        e.start();
        // (held warm each block, as the dyno harness holds an engine (host/include/tee/Warm.h)), read over 2 s
        auto play = [&](double sec) {
            const int calls = static_cast<int>(sec * e.rate / 512.0 + 0.5);
            for (int c = 0; c < calls; ++c)
            {
                e.holdWarm();
                REQUIRE(e.run(512.0 / e.rate));
            }
        };
        int press = 0;
        e.dyno(from, 1.0f);
        play(8.0);      // (the harness's warm-up)
        for (double rpm : {from, 1300.0})
        {
            e.dyno(rpm, 1.0f);
            play(1.0);
            ++press;
            e.spec("exhaust.heatSettle", press);
            e.spec("boost.heatSettle", press);
            play(9.0);
        }
        double b = 0.0, t = 0.0;
        const int calls = static_cast<int>(2.0 * e.rate / 512.0 + 0.5);
        for (int c = 0; c < calls; ++c)
        {
            e.holdWarm();
            REQUIRE(e.run(512.0 / e.rate));
            b += e.tel("boost.pressure") / calls;
            t += e.tel("engine.torque") / calls;
        }
        boost[k] = b;
        torque[k] = t;
        std::printf("VGT at 1300 rpm from %.0f rpm: %.0f kPa, %.0f N m, vanes %.2f\n", from, boost[k] / 1000.0, torque[k], e.tel("boost.vanes[0]"));
        ++k;
    }
    // (within 15 %, the lead's ruling for N2's form of this check, 25 Nov: the two states it guards against were 25 against
    // 58 kPa, more than a factor of two apart; a tenth of that separation, three times the scatter N2's form read between
    // machines and timings (3.0 to 5.4 kPa). It was 5 %, which a different machine turns red)
    CHECK(std::fabs(boost[0] - boost[1]) < 0.15 * boost[1]);
    CHECK(std::fabs(torque[0] - torque[1]) < 0.15 * torque[1]);
}
