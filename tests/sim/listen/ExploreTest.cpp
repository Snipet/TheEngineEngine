// ExploreTest.cpp: tables for the fit (hidden: run with "[.explore]"): each source's level in each of the reference's
// views and in each of model 1's cameras, white noise in.
#include "Rig.h"

#include <cstdio>

using namespace tee::test::listen;

TEST_CASE("explore: each source in each view and camera", "[.explore]")
{
    Rig rig;
    const int srcs[] = {kTails, kIntake, kMech, kBlock, kShell, kTurbo, kStarter, kComb, kRoad, kTorque};
    const char* names[] = {"tails", "intake", "mech", "block", "shell", "turbo", "starter", "comb", "road", "torque"};
    std::printf("%-8s", "src");
    for (int v = 0; v < 5; ++v)
        std::printf("   ref%d  new%d  diff", v, v);
    std::printf("\n");
    for (int k = 0; k < 10; ++k)
    {
        std::printf("%-8s", names[k]);
        rig.set("driven", srcs[k] == kRoad ? 1 : 0);
        rig.set("shake", 0.0);
        for (int v = 0; v < 5; ++v)
        {
            rig.set("view", v);
            rig.set("ss.camera", v);
            rig.set("ss.model", 0);
            const double a = dB(heard(rig, srcs[k]));
            rig.set("ss.model", 1);
            const double b = dB(heard(rig, srcs[k]));
            std::printf(" %6.1f %6.1f %5.1f", a, b, b - a);
        }
        std::printf("\n");
    }
}

TEST_CASE("explore: the cabin's boom by frequency", "[.explore]")
{
    Rig rig;
    rig.set("view", 4);
    rig.set("ss.camera", 4);
    rig.set("src", kTorque);
    for (double f : {30.0, 50.0, 100.0, 200.0, 400.0})
    {
        rig.set("sig", 1);
        rig.set("freq", f);
        double p[2];
        for (int m = 0; m < 2; ++m)
        {
            rig.set("ss.model", m);
            rig.reset();
            rig.clear();
            rig.play(0.5, false);
            rig.play(1.0);
            p[m] = rig.power();
        }
        std::printf("%6.0f Hz  ref %6.1f  new %6.1f  diff %5.1f\n", f, dB(p[0]), dB(p[1]), dB(p[1] / p[0]));
    }
}
