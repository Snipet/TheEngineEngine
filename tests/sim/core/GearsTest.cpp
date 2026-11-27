// tests/sim/core/GearsTest.cpp: the gearbox kinds from rest, and the failing cases of record for X-056 (queue item 18)
// and X-057 (queue item 19), each marked [!shouldfail] on its finding by name until its cure lands, when the mark comes
// off. Every case prints its figures each run. Whole-engine cases ([engine]): one process, the kept player (Engine.h).
#include "Engine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using tee::test::core::Bench;

namespace
{
constexpr double kRpm = 9.5492966;
constexpr const char* kRally = "archetypes/inline-five-turbo-rally";   // the shipped Sequential

// a Bench in DRIVE at rest, warm from start, nothing pressed
void atRest(Bench& b)
{
    b.start();
    b.ctl().mode = 1;
    b.play(0.2);
}
} // namespace

// Each kind leaves neutral at rest by the input its gearbox reads (sim/vehicle/gearbox.fm): the manual's paddles, the
// sequential's and the dual clutch's counters, the automatic's and the CVT's lever. (The eighteen-speed reads the lever
// with the clutch and its buttons: not here.)
TEST_CASE("each gearbox kind leaves rest by the input it reads", "[core][engine][gears]")
{
    struct K { const char* build; const char* kind; bool byLever; };
    for (const K& k : {K{"archetypes/hot-hatch-16-16v", "manual", false}, K{kRally, "sequential", false},
                       K{"archetypes/turbo-four-20-gdi", "dual clutch", false}, K{"archetypes/v6-35-na", "automatic", true},
                       K{"archetypes/economy-four-15", "CVT", true}})
    {
        Bench b(k.build);
        atRest(b);
        const double g0 = b.tel("car.gear");
        if (k.byLever)
            b.ctl().lever = 1;
        else
            b.ctl().shiftUp += 1;
        b.play(0.5);
        std::printf("%-12s by %s: gear %g at rest, %g after it\n", k.kind, k.byLever ? "the lever  " : "the counter", g0, b.tel("car.gear"));
        CHECK(g0 == 0.0);
        CHECK(b.tel("car.gear") == 1.0);
    }
}

// X-056 (B's trace; the lead's drive of 35a733d1): the sequential's dog shift set the robotised clutch to 1.0 whatever
// the launch held, so at rest it closed a clutch the launch had open. Cured by queue item 18 (27 Nov): the dog shift
// holds the clutch where it was as the shift began (gearbox.fm, robotHeld); before it, 386 rpm and 3.27 km/h here.
TEST_CASE("X-056: at rest, two up presses: the engine stays above 0.85 x idle and the car stays put", "[core][engine][gears]")
{
    Bench b(kRally);
    atRest(b);
    b.ctl().shiftUp += 1;
    b.play(0.5);
    const double idle = b.field("ecu.idleRpm");
    b.ctl().shiftUp += 1;
    double minRpm = 1e9, maxRobot = 0, maxKmh = 0;
    b.play(1.5, [&] {
        minRpm = std::min(minRpm, b.tel("engine.speed") * kRpm);
        maxRobot = std::max(maxRobot, b.field("car.m1.box.robot"));
        maxKmh = std::max(maxKmh, b.tel("car.speed") * 3.6);
    });
    std::printf("X-056 rest, up, up: gear %g; engine down to %.0f rpm (0.85 x idle %.0f), robot up to %.2f, car up to %.2f km/h\n",
                b.tel("car.gear"), minRpm, 0.85 * idle, maxRobot, maxKmh);
    CHECK(minRpm > 0.85 * idle);
    CHECK(maxKmh < 0.5);
}

// X-056, the lead's case (27 Nov): at rest, up, up, then down. Through the application (tee-headless, 35a733d1) the
// shipped rally five stalls 0.15 s after the down (2 to 1, 100 ms shut with its blip) and stays stalled. On this Bench
// (measured 27 Nov) it does not stall: it sagged to 287 rpm and recovered. So the case asks for both: still running a
// second after the down, and never under 0.85 x idle. Cured by queue item 18 (as above).
TEST_CASE("X-056: at rest, up, up, down: the engine still runs a second after the down", "[core][engine][gears]")
{
    Bench b(kRally);
    atRest(b);
    b.ctl().shiftUp += 1;
    b.play(0.7);
    b.ctl().shiftUp += 1;
    b.play(1.1);
    b.ctl().shiftDown += 1;
    int least = 9;
    double minRpm = 1e9, maxRobot = 0;
    b.play(1.0, [&] {
        least = std::min(least, int(b.field("ecu.state")));
        minRpm = std::min(minRpm, b.tel("engine.speed") * kRpm);
        maxRobot = std::max(maxRobot, b.field("car.m1.box.robot"));
    });
    std::printf("X-056 rest, up, up, down: gear %g; through the second after the down: engine down to %.0f rpm, robot up to %.2f, "
                "least state %d, now %g\n", b.tel("car.gear"), minRpm, maxRobot, least, b.field("ecu.state"));
    CHECK(least == 2);
    CHECK(b.field("ecu.state") == 2.0);
    CHECK(minRpm > 0.85 * b.field("ecu.idleRpm"));
}

// X-056's guard: a synced 1-2 on the overrun at about 1500 rpm keeps the robotised clutch closed WHILE IT SHIFTS.
// Passes today, and must still pass with the cure. (After the shift, synced in second at about 1000 rpm on the
// overrun, the robot slips between 0.28 and 0.57 (measured 27 Nov, "[.x056guard]"): another law's, not asserted here.)
TEST_CASE("X-056: a synced 1-2 at about 1500 rpm on the overrun keeps the clutch closed", "[core][engine][gears]")
{
    Bench b(kRally);
    atRest(b);
    b.ctl().shiftUp += 1;
    b.play(0.2);
    b.ctl().throttle = 0.2f;
    b.play(3.0);
    b.ctl().throttle = 0.0f;
    b.play(5.0);
    const double before = b.tel("engine.speed") * kRpm, synced = b.field("car.m1.box.synced");
    b.ctl().shiftUp += 1;
    double minRobot = 1;
    int steps = 0;
    b.play(0.5, [&] {
        if (b.field("car.m1.box.shifting") != 0.0)
        {
            minRobot = std::min(minRobot, b.field("car.m1.box.robot"));
            ++steps;
        }
    });
    std::printf("X-056 overrun 1-2: before %.0f rpm, synced %g; gear %g after; while shifting (%d steps) robot down to %.3f; after, %.3f\n",
                before, synced, b.tel("car.gear"), steps, minRobot, b.field("car.m1.box.robot"));
    CHECK(steps > 0);
    CHECK(synced == 1.0);
    CHECK(b.tel("car.gear") == 2.0);
    CHECK(minRobot == 1.0);
}

// X-057's rule (queue item 19): a governed engine's redline is its governor's high idle (heavy.fm:203, rated x
// (1 + droop) here, none states highIdle). On the reference ECU redlineFor floors it at idle + 1500 (ecu.fm:125-130)
// and a table engine's base sits above it, so these three cannot reach it in content. Fails today on all three.
TEST_CASE("X-057: the three governed builds' redline is their high idle", "[core][engine][gears][!shouldfail]")
{
    for (const char* build : {"diesel-v8-truck", "diesel-mechanical-classic", "diesel-marine-v12"})
    {
        Bench b(build);
        b.start();
        b.play(0.5);
        const double hi = b.field("spec.ecu.highIdle") > 0.0 ? b.field("spec.ecu.highIdle") * kRpm
                                                              : b.field("spec.ecu.ratedSpeed") * kRpm * (1.0 + b.field("spec.ecu.droop"));
        std::printf("X-057 %s: governed high idle %.1f rpm, ecu.redRpm %.1f (idle %.1f)\n", build, hi, b.field("ecu.redRpm"), b.field("ecu.idleRpm"));
        CHECK(std::abs(b.field("ecu.redRpm") - hi) < 0.5);
    }
}
