// tests/sim/exhaust/ExhaustTest.cpp: card G's tests of the exhaust builder, on made-up port flows (no cylinders).
#include "Rig.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdio>

using namespace tee::test::exhaust;

TEST_CASE("the harness compiles", "[exhaust]")
{
    Rig rig(12);
    rig.configure();
    CHECK(rig.ends() == 2);
}

// Card G, test 2 (standalone form): the builder with every field at its default (the assignment table meaning the
// layout's own topology) against the reference's exhaust code (refexhaust.fm, copied from sim/ref/engine.fm), both
// on the same port flows, the noise off (its random numbers are drawn in another order). Every layout: topologies 0
// (all into one), 1 (a collector a bank, one pipe) and 2 (dual with an H).
TEST_CASE("model 1 at its defaults is the reference's exhaust", "[exhaust][reference]")
{
    // every layout at the reference's dials, and three at others (HEADERS, PIPE, X-PIPE, MUFFLER)
    const float dialSets[4][4] = {{0.5f, 0.5f, 0.0f, 0.5f}, {0.1f, 0.9f, 0.7f, 0.95f}, {0.9f, 0.2f, 0.3f, 0.15f},
                                  {0.5f, 0.5f, 1.0f, 0.0f}};
    for (int run = 0; run < 17 + 9; ++run)
    {
        const int layout = run < 17 ? run : (run - 17) % 3 == 0 ? 5 : ((run - 17) % 3 == 1 ? 10 : 12);
        const int ds = run < 17 ? 0 : 1 + (run - 17) / 3;
        Rig rig(layout);
        rig.noise(0.0);
        rig.dials(dialSets[ds][0], dialSets[ds][1], dialSets[ds][2], dialSets[ds][3]);
        rig.configure();
        std::vector<float> q = pulses(rig, 3000.0, 1.0, 0.15, 0.01);
        std::vector<float> bl, br, rl, rr;
        rig.run(q, bl, br, false);
        rig.run(q, rl, rr, true);
        double es = 0.0;
        double ss = 0.0;
        for (std::size_t i = 0; i < bl.size(); ++i)
        {
            es += (bl[i] - rl[i]) * (bl[i] - rl[i]) + (br[i] - rr[i]) * (br[i] - rr[i]);
            ss += rl[i] * rl[i] + rr[i] * rr[i];
        }
        const double err = db(es / ss);
        std::printf("layout %2d, dials %d: builder - reference %.1f dB (reference rms %.3g)\n", layout, ds, err,
                    std::sqrt(ss / (2.0 * bl.size())));
        INFO("layout " << layout);
        CHECK(ss > 0.0);
        CHECK(err < -60.0);
    }
}

