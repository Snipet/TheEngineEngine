// tests/sim/vehicle/ShiftCutTest.cpp: an upshift's cut under ecu.model 1 (finding X-047, docs/plan/x047-shift-cut.md,
// item 16). The engine is cut while the vehicle asks, and no longer: every kind that asks gets the window its own
// gearbox model wants, measured a control step at a time (16 samples) on ecu.flags' fuel and spark cuts. The paddle
// robot (car.model 0) 60 ms, the truck's 150; car.model 1's manual on the paddles and its sequential 60 ms; the bike its
// quickshiftCut as set, no longer twice it. And the guard: every asker's window lasts at least two control steps, so a
// one-step asker fails here and not in a shift (the lead's ruling, 26 Nov: no floor in launch.fm).
#include "tee/test/Scenario.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <string>

namespace
{
    constexpr double kStep = 16.0 / 48000.0;   // the ECU's control step

    struct Window
    {
        double start = -1.0, seconds = 0.0;
    };

    // The first run of samples in [from, to] whose flags have any of `bits`: its start and its length (a sample a step)
    Window firstRun(const tee::test::Series& flags, double from, double to, int bits)
    {
        const auto& t = flags.times();
        const auto& f = flags.values();
        Window w;
        int n = 0;
        for (std::size_t i = 0; i < t.size(); ++i)
        {
            if (t[i] < from || t[i] > to)
                continue;
            const bool on = (static_cast<int>(f[i]) & bits) != 0;
            if (on && w.start < 0.0)
                w.start = t[i];
            if (on)
                ++n;
            else if (w.start >= 0.0)
                break;
        }
        w.seconds = n * kStep;
        return w;
    }

    struct Cut
    {
        Window ecu;     // ecu.flags: the fuel or the spark cut
        Window asked;   // car.flags 32 (the vehicle's own cut; car.model 1 and the bike: the paddle robot shows none)
        int flags = 0;  // every ecu.flags bit seen in the window
    };

    // Started in drive, into first, pulling away at `throttle`; an upshift at 4.5 s. `setup` sets the kind.
    template <class Setup>
    Cut upshift(const std::string& build, double throttle, Setup setup)
    {
        tee::test::Scenario s(build);
        s.block(16);
        s.at(0.0).mode(1).lever(0).clutch(0.0).brake(0.0).throttle(0.0).ignition(tee::On);
        s.at(0.0).spec("ecu.model", 1.0);
        setup(s);
        s.at(0.25).ignition(tee::Start).until(2.0).ignition(tee::On);
        s.at(2.5).shiftUp();
        s.at(2.7).throttle(throttle).ramp(0.3);
        s.at(4.5).shiftUp();
        const tee::test::Run r = s.run(5.2);
        INFO(r.error());
        REQUIRE(r.ok());
        Cut c;
        c.ecu = firstRun(r.tel("ecu.flags"), 4.5, 5.2, 2 | 4);
        c.asked = firstRun(r.tel("car.flags"), 4.5, 5.2, 32);
        c.flags = static_cast<int>(r.tel("ecu.flags").during(4.5, 5.2).max());
        std::printf("shift cut, %s: ecu cut %.2f ms from %.4f s; the vehicle's cut flag %.2f ms from %.4f s; gear %g -> %g; "
                    "ecu flags up to %d\n", build.c_str(), 1000.0 * c.ecu.seconds, c.ecu.start, 1000.0 * c.asked.seconds,
                    c.asked.start, r.tel("car.gear").at(4.45), r.tel("car.gear").last(), c.flags);
        return c;
    }

    void wants(const Cut& c, double seconds)
    {
        CHECK((c.flags & 1) == 0);                                     // not the limiter's
        CHECK(c.ecu.start >= 0.0);
        CHECK(std::fabs(c.ecu.seconds - seconds) <= kStep + 1.0e-9);   // within one control step
        CHECK(c.ecu.seconds >= 2.0 * kStep);                           // the guard
    }
} // namespace

TEST_CASE("shift.cut: the paddle robot's upshift is cut for its 60 ms, the truck's for its 150", "[vehicle][engine]")
{
    wants(upshift("v8", 0.5, [](tee::test::Scenario&) {}), 0.06);
    wants(upshift("truck-15l-six", 0.3, [](tee::test::Scenario& s) { s.at(0.0).spec("car.model", 0.0); }), 0.15);
}

TEST_CASE("shift.cut: car.model 1's paddle manual and its sequential are cut for 60 ms, as the vehicle asks",
          "[vehicle][engine]")
{
    for (double box : {0.0, 1.0})
    {
        const Cut c = upshift("v8", 0.5, [box](tee::test::Scenario& s) {
            s.at(0.0).spec("car.model", 1.0).spec("car.gearbox", box);
        });
        wants(c, 0.06);
        CHECK(std::fabs(c.ecu.seconds - c.asked.seconds) <= kStep + 1.0e-9);
    }
}

TEST_CASE("shift.cut: the bike's quickshift is cut for its quickshiftCut as set, not twice it; at its 20 ms floor, still "
          "at least two control steps", "[vehicle][engine][bike]")
{
    for (double cut : {0.05, 0.08, 0.02})
    {
        const Cut c = upshift("bike-superbike", 0.5, [cut](tee::test::Scenario& s) {
            s.at(0.0).spec("car.wheelieControl", "By the clutch").spec("car.quickshiftCut", cut);
        });
        wants(c, cut);
        CHECK(std::fabs(c.ecu.seconds - c.asked.seconds) <= kStep + 1.0e-9);
    }
}
