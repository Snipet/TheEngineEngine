// tests/sim/vehicle/BenchTest.cpp: what the vehicle model costs a sample (card I's budget: 0.05 of the reference's
// cost, R), measured in processor time over the rig's run, less the same rig with the vehicle's step left out. Hidden
// from ctest ("[.bench]": a busy machine makes it noisy); run it by hand: tee-tests-vehicle "[.bench]".
#include "Rig.h"

#include <cstdio>
#include <ctime>

using namespace tee::vehicle;

namespace
{
    double cpuSeconds()
    {
        timespec ts{};
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &ts);
        return static_cast<double>(ts.tv_sec) + 1e-9 * static_cast<double>(ts.tv_nsec);
    }

    double nsPerSample(Rig& rig, double seconds)
    {
        double best = 1e30;
        for (int k = 0; k < 3; ++k)
        {
            const double t0 = cpuSeconds();
            rig.seconds(seconds);
            const double t1 = cpuSeconds();
            best = std::fmin(best, 1e9 * (t1 - t0) / (seconds * 96000.0));
        }
        return best;
    }
} // namespace

TEST_CASE("bench: the model's cost a sample, every part in use", "[.bench]")
{
    for (int box = 0; box < 5; ++box)
    {
        Rig rig;
        rig.set("car.spec.gearbox", box);
        rig.set("car.spec.drive", AWD);
        rig.set("car.spec.rearPreload", 50.0);
        rig.set("car.spec.frontKSpeed", 10.0);
        rig.set("car.spec.centreKTorque", 0.2);
        rig.set("car.spec.liveAxle", 1);
        rig.set("car.spec.clA", 1.0);
        rig.settle();
        if (box == 0)
            rig.pullAway(1, 0.6, 250.0);
        else if (box <= 2)
            rig.set("up", 1);
        else
            rig.set("lever", 1);
        rig.set("throttle", 0.6);
        rig.seconds(2.0);
        const double driving = nsPerSample(rig, 4.0);
        const double v = rig.get("car.v");
        rig.set("bare", 1);
        const double bare = nsPerSample(rig, 4.0);
        std::printf("bench (gearbox %d, all-wheel drive, every differential, %.1f m/s): %.1f ns a sample, %.1f without "
                    "the rig's engine\n", box, v, driving, driving - bare);
    }
}
