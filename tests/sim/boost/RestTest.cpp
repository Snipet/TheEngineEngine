// tests/sim/boost/RestTest.cpp: at rest, nothing flows. Every volume of the machine started a little off the air
// outside, the engine stopped: the flows between the volumes (orifices solved implicitly, inertial ducts with their
// friction) must die away and stay away, not chatter about zero.
#include "Rig.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using tee::test::boost::Rig;

TEST_CASE("boost: at rest, with the volumes a little apart, the flows die away and stay away", "[boost][rest]")
{
    struct Build
    {
        const char* name;
        int kind;
        int layout;
        int scrolls;
        int charger;
        int gate;
    };
    const Build builds[] = {{"one turbo", 1, 1, 1, 0, 0},           {"twin-scroll, external gate", 1, 1, 2, 0, 1},
                            {"sequential", 1, 3, 1, 0, 0},          {"compound", 1, 4, 2, 0, 0},
                            {"four", 1, 5, 1, 0, 0},                {"Roots", 4, 0, 1, 0, 0},
                            {"twin-screw", 4, 0, 1, 1, 0},          {"centrifugal", 4, 0, 1, 2, 0}};
    for (const Build& b : builds)
    {
        Rig rig;
        rig.set("spec.kind", b.kind);
        rig.set("spec.layout", b.layout);
        rig.set("spec.scrolls", b.scrolls);
        rig.set("spec.charger", b.charger);
        rig.set("spec.gate", b.gate);
        rig.set("spec.model", 1);
        rig.spec("rev", 0);
        rig.set("rpm", 0.0);
        rig.set("throttle", 0.3);
        rig.run(1.0 / 96000.0);
        // a little apart: each volume's gas 1 to 3 % more or less
        double k = 0.97;
        for (const char* v : {"boost.v1", "boost.v2", "boost.vInter", "boost.vSec", "plenum"})
        {
            rig.set(std::string(v) + ".m", rig.get(std::string(v) + ".m") * k);
            rig.set(std::string(v) + ".u", rig.get(std::string(v) + ".u") * k);
            k += 0.015;
        }
        for (int h = 0; h < 8; ++h)
        {
            const std::string at = "boost.house[" + std::to_string(h) + "]";
            rig.set(at + ".ps", rig.get(at + ".ps") * k);
            rig.set(at + ".m", rig.get(at + ".m") * k);
            k = 2.0 - k;
        }
        const std::vector<std::string> flows = {"boost.comp[0].mdot", "boost.comp[1].mdot", "boost.mIc", "boost.mBov",
                                                "boost.house[0].mOutPipe", "boost.house[1].mOutPipe",
                                                "boost.house[0].mOutInt", "boost.house[2].mOutPipe", "boost.pd.mLeak",
                                                "boost.house[0].mOutGate", "mThr"};
        const bool turbo = b.kind != 4;
        const int housings = turbo ? (b.layout == 5 ? 8 : (b.layout == 1 ? 2 : 4)) : 0;
        std::vector<std::string> volumes = {"boost.v1.p", "boost.v2.p", "plenum.p"};
        for (int h = 0; h < housings; h += 3 - b.scrolls)
            volumes.push_back("boost.house[" + std::to_string(h) + "].p");
        if (b.layout == 3)
            volumes.push_back("boost.vSec.p");
        if (b.layout == 4)
            volumes.push_back("boost.vInter.p");
        // two windows of 20 ms, a sample at a time, three seconds and six seconds on: what still moves must be small
        // and dying, not a chatter that keeps its size
        double worst[2] = {0.0, 0.0};
        double worstP[2] = {0.0, 0.0};
        std::string whichFlow;
        std::string whichP;
        for (int w = 0; w < 2; ++w)
        {
            rig.run(3.0);
            for (int i = 0; i < 1920; ++i)
            {
                rig.run(1.0 / 96000.0);
                for (const auto& f : flows)
                    if (std::fabs(rig.get(f)) > worst[w])
                    {
                        worst[w] = std::fabs(rig.get(f));
                        whichFlow = f;
                    }
                for (const auto& v : volumes)
                    if (std::fabs(rig.get(v) - 101325.0) > worstP[w])
                    {
                        worstP[w] = std::fabs(rig.get(v) - 101325.0);
                        whichP = v;
                    }
            }
        }
        std::printf("at rest, %s: the most any flow was %.2e kg/s at 3 s, %.2e at 6 s (%s); the most any pressure was "
                    "off %.3f Pa, %.3f (%s)\n", b.name, worst[0], worst[1], whichFlow.c_str(), worstP[0], worstP[1],
                    whichP.c_str());
        std::fflush(stdout);
        INFO(b.name);
        CHECK(worstP[1] < 1.0);
        CHECK((worst[1] < 1.0e-6 || worst[1] < 0.3 * worst[0]));
    }
}
