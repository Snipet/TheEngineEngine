// tests/sim/core/EngineExplore.cpp: the engine on the dyno, printed (hidden: "[.dynosweep]").
#include "Engine.h"

#include <algorithm>
#include <cstdio>

using tee::test::core::Bench;

TEST_CASE("dyno sweep printed", "[.dynosweep]")
{
    for (const char* build : {"default", "engines/ref-12-v8-cross"})
    {
        Bench b(build);
        b.start();
        b.ctl().mode = 2;
        b.ctl().throttle = 1.0f;
        std::printf("%s\n", build);
        for (double rpm : {1500.0, 2500.0, 3500.0, 4500.0, 5500.0, 6200.0})
        {
            b.ctl().dynoSpeed = float(rpm / 9.5492966);
            double sp = 0, n = 0, bm = 0, im = 0, pm = 0, fm = 0, ve = 0, vm = 0, tq = 0, tb = 0;
            b.play(2.0);
            b.play(1.0, [&] {
                sp += b.tel("engine.speed"); bm += b.tel("engine.bmep"); im += b.tel("engine.imep"); pm += b.tel("engine.pmep");
                fm += b.tel("engine.fmep"); ve += b.tel("engine.veAir"); vm += b.tel("engine.veManifold");
                tq += b.tel("engine.dynoTorque"); tb += b.field("tqBrake"); n += 1; });
            std::printf("  %5.0f rpm: held %7.1f, BMEP %5.2f IMEP %5.2f PMEP %5.2f FMEP %4.2f bar, VE air %.3f man %.3f, dyno %6.1f N m, gas-fric %6.1f, peak %5.1f bar, redline %.0f\n",
                        rpm, sp / n * 9.5492966, bm / n / 1e5, im / n / 1e5, pm / n / 1e5, fm / n / 1e5, ve / n, vm / n, tq / n, tb / n,
                        b.tel("engine.peakPressure") / 1e5, b.tel("engine.redline") * 9.5492966);
        }
    }
}

TEST_CASE("dyno trace", "[.dynotrace]")
{
    Bench b("v8");
    b.start();
    b.ctl().mode = 2;
    b.ctl().throttle = 1.0f;
    b.ctl().dynoSpeed = float(2000.0 / 9.5492966);
    double t = 0;
    int k = 0;
    b.play(4.0, [&] {
        t += 512.0 / 48000.0;
        if (++k % 8 == 0)
            std::printf("%5.2f s: %7.1f rpm, absorber %7.1f, dynoOn %g, state %g, integ %g, knock %.1f, flags %g, throttle %.2f\n", t,
                        b.tel("engine.speed") * 9.5492966, b.field("tqAbs"), b.field("dynoOn"), b.tel("engine.state"), b.field("dyno.integ"),
                        b.tel("ecu.knockRetard"), b.tel("engine.flags"), b.tel("engine.throttle"));
    });
}

TEST_CASE("cost of builds", "[.cost]")
{
    for (const char* build : {"default", "engines/ref-12-v8-cross", "engines/v16-45"})
    {
        double best = 1e9;
        for (int r = 0; r < 3; ++r)
        {
            Bench b(build);
            b.start();
            b.ctl().throttle = 0.6f;
            b.play(6.0);
            best = std::min(best, b.load());
        }
        std::printf("%-24s %.1f %% of a core (48 kHz, 512 blocks; start, then 6 s at 60 %% throttle)\n", build, 100.0 * best);
    }
}
