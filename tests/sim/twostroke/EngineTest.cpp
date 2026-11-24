// tests/sim/twostroke/EngineTest.cpp: the two-stroke in the engine (spec.engine.cycle 1): it starts and idles, revs,
// its Scope's cycle is 360 degrees, its sound finite and within the ceiling.
#include "Bench.h"

#include <cstdio>
#include <cstdlib>
#include <vector>

using tee::test::twostroke::Bench;

TEST_CASE("engine: a two-stroke single starts, idles and revs", "[twostroke][engine]")
{
    Bench b("two-stroke-mx125");
    if (!b.hooked())
        SKIP("sim/engine.fm has no two-stroke hook yet (card D)");
    b.start(3.0);
    const double idle = b.rpm();
    b.ctl().throttle = 1.0f;
    double top = 0;
    b.play(3.0, [&] { top = std::max(top, b.rpm()); });
    b.ctl().throttle = 0.0f;
    b.play(3.0);
    const double back = b.rpm();
    std::printf("mx125: idles at %.0f rpm, revs to %.0f (redline %.0f), back to %.0f; peak sample %.3f; %.2f of a core\n", idle, top,
                b.tel("engine.redline") * 30.0 / 3.14159265, back, b.peak(), b.load());
    CHECK(b.bad() == 0);
    CHECK(b.peak() <= 1.0);
    CHECK(idle > 1000);
    CHECK(idle < 3500);
    CHECK(top > 10000);
    CHECK(back < 4000);
}

TEST_CASE("engine: a two-stroke's Scope is one turn: 360 degrees", "[twostroke][engine]")
{
    Bench b("two-stroke-mx125");
    if (!b.hooked())
        SKIP("sim/engine.fm has no two-stroke hook yet (card D)");
    b.start(1.0);
    b.ctl().scopeOn = 1;
    b.ctl().throttle = 0.5f;
    b.play(1.0);
    double pmax = 0;
    for (int d = 0; d < 360; ++d)
        pmax = std::max(pmax, b.field("scope.pressure[" + std::to_string(d) + "]"));
    std::printf("scope: %d degrees, %d cycles, cylinder 1's peak %.1f bar\n", int(b.field("scope.degrees")), int(b.field("scope.cycles")),
                pmax / 1e5);
    CHECK(b.field("scope.degrees") == 360);
    CHECK(pmax > 10e5);
}

namespace
{
    // A full-throttle sweep on the engine dyno, held at each speed: (rpm, brake torque N m, power kW)
    struct Point { double rpm, torque, kw; };
    std::vector<Point> dynoSweep(Bench& b, double from, double to, double by)
    {
        std::vector<Point> out;
        b.ctl().mode = 2;
        b.ctl().throttle = 1.0f;
        for (double rpm = from; rpm <= to + 1; rpm += by)
        {
            b.ctl().dynoSpeed = float(rpm * 3.14159265358979 / 30.0);
            b.ctl().dynoRate = 0.0f;
            b.play(1.2);
            double tq = 0;
            int n = 0;
            b.play(0.5, [&] { tq += b.tel("engine.torque"); n += 1; });
            tq /= n;
            out.push_back({b.rpm(), tq, tq * b.rpm() * 3.14159265358979 / 30.0 / 1000.0});
        }
        b.ctl().mode = 0;
        b.ctl().throttle = 0.0f;
        return out;
    }
} // namespace

namespace
{
    struct BuildCase { const char* name; double from, to, by, peakRpm, publishedKw; };

    // One build: it starts and idles; it four-strokes at idle and not on the pipe; its best power where its class's is
    void checkBuild(const BuildCase& k)
    {
        Bench b(k.name);
        if (!b.hooked())
            SKIP("sim/engine.fm has no two-stroke hook yet (card D)");
        b.start(3.0);
        const double idle = b.rpm();
        const int m0 = int(b.field("two.cy[0].misfires")), c0 = int(b.field("two.cy[0].cycles"));
        b.play(2.0);
        const double idleMisfire = double(int(b.field("two.cy[0].misfires")) - m0) / std::max(int(b.field("two.cy[0].cycles")) - c0, 1);
        double best = 0, bestAt = 0, bandMisfire = 0;
        for (const Point& p : dynoSweep(b, k.from, k.to, k.by))
            if (p.kw > best)
            {
                best = p.kw;
                bestAt = p.rpm;
            }
        // on the pipe, held at its best: how often it misfires
        b.ctl().mode = 2;
        b.ctl().throttle = 1.0f;
        b.ctl().dynoSpeed = float(bestAt * 3.14159265358979 / 30.0);
        b.play(1.0);
        const int m1 = int(b.field("two.cy[0].misfires")), c1 = int(b.field("two.cy[0].cycles"));
        b.play(1.0);
        bandMisfire = double(int(b.field("two.cy[0].misfires")) - m1) / std::max(int(b.field("two.cy[0].cycles")) - c1, 1);
        std::printf("%-22s idle %4.0f rpm, misfiring %3.0f %% of cycles; best %5.1f kW at %5.0f rpm (the class: %5.1f kW near %5.0f: "
                    "%3.0f %%), misfiring there %2.0f %%; peak sample %.2f\n", k.name, idle, 100 * idleMisfire, best, bestAt, k.publishedKw,
                    k.peakRpm, 100 * best / k.publishedKw, 100 * bandMisfire, b.peak());
        CHECK(b.bad() == 0);
        CHECK(b.peak() <= 1.0);
        CHECK(idle > 1200);
        CHECK(idle < 4500);
        CHECK(idleMisfire > 0.2);
        CHECK(bandMisfire == 0.0);
        CHECK(std::abs(bestAt - k.peakRpm) < 0.1 * k.peakRpm);
    }} // namespace

// Each build: it starts and idles; it four-strokes at idle (misfire after misfire, unscripted: the poorly scavenged
// charge at the reference's limit of dilution) and not on the pipe; on the dyno at full throttle its best power comes
// where its class's does (published, within 10 %), and its power is printed against the published figure (the model's
// crankcase pump delivers less than a real one's: no inertia in the inlet tract or the transfers; docs/models/twostroke.md).
// The quick form (the default run, under 30 s): the 125 alone, on a coarser sweep; the five builds nightly.
TEST_CASE("engine: a build: the 125 starts, idles four-stroking, its band where the class has it", "[twostroke][engine][builds]")
{
    checkBuild(BuildCase{"two-stroke-mx125", 9000, 13000, 1000, 11500, 28.0});
}

TEST_CASE("engine: the builds: start, idle four-stroking, the band where the class has it", "[twostroke][engine][builds][nightly]")
{
    for (BuildCase k : {BuildCase{"two-stroke-mx125", 7000, 13000, 500, 11500, 28.0}, BuildCase{"two-stroke-road-twin", 5000, 10000, 500, 8500, 26.0},
                        BuildCase{"two-stroke-triple", 4500, 8000, 500, 7500, 44.0}, BuildCase{"two-stroke-kart", 10000, 17500, 500, 14500, 20.0},
                        BuildCase{"two-stroke-utility", 5000, 10000, 500, 9250, 2.4}})
        checkBuild(k);
}

TEST_CASE("debug start", "[.dbgstart]")
{
    for (const char* name : {"two-stroke-utility", "two-stroke-mx125"})
    {
        Bench b(name);
        b.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
        for (int i = 0; i < 8; ++i)
        {
            b.play(0.25);
            std::printf("%s %.2f s: %.0f rpm, state %.0f, cyl %.2f bar, cc %.3f bar, tPipe %.0f, misfires %d, cBurn %.2f, purity %.2f\n", name,
                        0.25 * (i + 1), b.rpm(), b.tel("engine.state"), b.field("two.cy[0].p") / 1e5, b.field("two.cy[0].pc") / 1e5,
                        b.field("two.tPipe"), int(b.field("two.cy[0].misfires")), b.field("two.cy[0].cBurn"), b.field("two.cy[0].purity"));
        }
        b.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
        b.ctl().throttle = 1.0f;
        b.play(3.0);
        std::printf("%s at full throttle: %.0f rpm, tPipe %.0f K, hEx %.4g mEx %.4g (%.0f K), cyl t %.0f, tAmb %.1f\n", name, b.rpm(),
                    b.field("two.tPipe"), b.field("two.hEx"), b.field("two.mEx"), b.field("two.hEx") / b.field("two.mEx"), b.field("two.cy[0].t"),
                    b.field("heat.tAmb"));
    }
}

TEST_CASE("engine: a two-stroke's work loop: IMEP and BMEP on its 360 degrees", "[twostroke][engine]")
{
    Bench b("two-stroke-mx125");
    if (!b.hooked())
        SKIP("sim/engine.fm has no two-stroke hook yet (card D)");
    b.start(1.0);
    b.ctl().mode = 2;
    b.ctl().throttle = 1.0f;
    b.ctl().dynoSpeed = float(11000 * 3.14159265358979 / 30.0);
    b.play(2.0);
    const double imep = b.tel("engine.imep"), bmep = b.tel("engine.bmep"), tq = b.tel("engine.torque");
    std::printf("mx125 at 11 000 rpm, full throttle: IMEP %.2f bar, BMEP %.2f bar (2 pi T / V: %.2f), peak %.1f bar\n", imep / 1e5,
                bmep / 1e5, 2 * 3.14159265358979 * tq / 124.8e-6 / 1e5, b.tel("engine.peakPressure") / 1e5);
    const double bmepTwo = 2 * 3.14159265358979 * tq / 124.8e-6;     // (a two-stroke's: 2 pi T / V)
    CHECK(std::abs(bmep - bmepTwo) < 0.05 * std::abs(bmepTwo) + 1e4);
    CHECK(imep > bmepTwo);
    CHECK(imep < bmepTwo + 3e5);      // (friction by part: some 1 to 3 bar at 11 000 rpm)
    CHECK(b.tel("engine.peakPressure") > 20e5);
}
