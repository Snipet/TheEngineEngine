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

TEST_CASE("explore: steps at each change", "[.explore]")
{
    Rig rig;
    rig.set("ss.model", 1);
    rig.set("ln.sc.windOff", 1);
    rig.set("src", kTails);
    rig.set("sig", 1);
    rig.set("freq", 180.0);
    rig.set("amp", 0.002);
    rig.reset();
    rig.play(0.3, false);
    for (int a = 0; a < 7; ++a)
    {
        rig.clear();
        rig.set("ss.camera", a);
        const bool pl = std::getenv("PLACE") != nullptr;
        if (pl)
            rig.set("ss.place", a % 5);
        if (std::getenv("DIST") != nullptr)
            rig.set("distance", 0.15 * double(a % 4));
        rig.play(0.4);
        const auto& l = rig.left();
        double worst = 0.0;
        std::size_t at = 0;
        for (std::size_t i = 960; i + 960 < l.size(); ++i)
        {
            double s = 0.0;
            for (std::size_t j = i - 960; j < i + 960; j += 4)
                s += double(l[j]) * l[j];
            const double rms = std::sqrt(s / 480.0);
            const double st = std::fabs(double(l[i]) - l[i - 1]) / std::max(rms, 1e-12);
            if (st > worst)
            {
                worst = st;
                at = i;
            }
        }
        std::printf("to camera %d: worst %.4f at %.1f ms\n", a, worst, double(at) / 96.0);
    }
}
