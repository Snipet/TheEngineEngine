// tests/sim/boost/MapTest.cpp: card F, standalone test 1: the compressor's map made from its sizes.
#include "Rig.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>

using tee::test::boost::Rig;

namespace
{
    // a turbo of given compressor sizes on the made-up engine
    void sizes(Rig& rig, double inducer, double exducer)
    {
        rig.set("spec.kind", 1);
        rig.set("spec.compInducer", inducer);
        rig.set("spec.compExducer", exducer);
        rig.spec("model", 1);
        rig.run(1.0 / 96000.0);
    }

    double ratio(Rig& rig, double phi, double omega)
    {
        auto* f = rig.method<float(void*, float, float)>("mapRatio");
        return f(rig.data(), static_cast<float>(phi), static_cast<float>(omega));
    }

    double eta(Rig& rig, double phi, double omega)
    {
        auto* f = rig.method<float(void*, float, float)>("mapEta");
        return f(rig.data(), static_cast<float>(phi), static_cast<float>(omega));
    }

    double choke(Rig& rig, double omega)
    {
        auto* f = rig.method<float(void*, float)>("mapChoke");
        return f(rig.data(), static_cast<float>(omega));
    }

    double knee(Rig& rig, double omega)
    {
        auto* f = rig.method<float(void*, float)>("mapKnee");
        return f(rig.data(), static_cast<float>(omega));
    }

    double bestFlow(Rig& rig, double omega)
    {
        auto* f = rig.method<float(void*, float)>("mapBest");
        return f(rig.data(), static_cast<float>(omega));
    }
} // namespace

TEST_CASE("boost: the choke flow scales with the inducer's area", "[boost][map]")
{
    Rig small;
    sizes(small, 0.040, 0.060);
    Rig big;
    sizes(big, 0.050, 0.060);
    // the same tip speed (the same exducer), at full and at part speed
    for (double omega : {6000.0, 16000.0})
    {
        const double ratioArea = (0.050 * 0.050) / (0.040 * 0.040);
        CHECK(choke(big, omega) / choke(small, omega) == Catch::Approx(ratioArea).epsilon(0.002));
    }
    // and it grows with speed to the inducer's limit
    CHECK(choke(small, 16000.0) > choke(small, 6000.0));
}

TEST_CASE("boost: the knee's pressure ratio goes with the tip speed squared times the fitted rise", "[boost][map]")
{
    Rig rig;
    sizes(rig, 0.045, 0.060);
    // tau = PR^(1/3.5) - 1 at the knee, against the tip speed's square times the rise coefficient fitted to every cited
    // map, c(M) = 0.608 - 0.157 (M - 1) (map-shape-fit.md: the published lines' peak ratios fall below a flat
    // coefficient's as the tip speed rises), the inlet at 293.15 K as the rig's
    const double u1 = 300.0;
    const double u2 = 450.0;
    const double r = 0.030;
    const double t1 = std::pow(ratio(rig, knee(rig, u1 / r), u1 / r), 1.0 / 3.5) - 1.0;
    const double phiK = knee(rig, u2 / r);
    const double t2 = std::pow(ratio(rig, phiK, u2 / r), 1.0 / 3.5) - 1.0;
    const double a01 = std::sqrt(1.4 * 287.0 * 293.15);
    const auto c = [&](double u) { return 0.608 - 0.157 * (u / a01 - 1.0); };
    CHECK(t2 / t1 == Catch::Approx((c(u2) * u2 * u2) / (c(u1) * u1 * u1)).epsilon(0.002));
    // the knee is the line's highest point: lower toward surge and toward choke
    const double top = ratio(rig, phiK, u2 / r);
    CHECK(ratio(rig, 0.5 * phiK, u2 / r) < top);
    CHECK(ratio(rig, 0.0, u2 / r) < top);
    CHECK(ratio(rig, phiK + 0.5 * (1.0 - phiK), u2 / r) < top);
    CHECK(ratio(rig, 1.0, u2 / r) < ratio(rig, phiK + 0.5 * (1.0 - phiK), u2 / r));
    // a 60 mm wheel at 150,000 rpm (471 m/s at the tip) makes at its knee what the cited heavy-duty lines make at that tip
    // speed: the S400SX-E 3.21 at 461 m/s and 3.62 at 491, the S300SX3 3.14 at 443 and 3.94 at 505 (sources section 21,
    // read off the makers' maps; their peaks interpolated to 471 m/s: 3.35 and 3.50); the generic map is their mean
    // within its stated 3 % (map-shape-fit.md). The earlier 2.6 to 3.0 was the five passenger maps' class, which state
    // no per-line peaks; the all-seven fit replaced the knee's law
    const double pr150 = ratio(rig, knee(rig, 150000.0 * 6.283185307 / 60.0), 150000.0 * 6.283185307 / 60.0);
    INFO("PR at 150 krpm: " << pr150);
    CHECK(pr150 > 3.35 * 0.97);
    CHECK(pr150 < 3.50 * 1.03);
}

TEST_CASE("boost: efficiency peaks at the best flow and a tip Mach number of 1.1", "[boost][map]")
{
    Rig rig;
    sizes(rig, 0.045, 0.060);
    const double etaMax = rig.get("spec.compEta");
    const double a0 = std::sqrt(1.4 * 287.0 * 293.15);
    const double r = rig.get("boost.comp[0].rTip");
    double best = 0.0;
    double bestPhi = 0.0;
    double bestMach = 0.0;
    for (int i = 0; i <= 100; ++i)
        for (int j = 0; j <= 60; ++j)
        {
            const double phi = 0.01 * i;
            const double mach = 0.3 + 0.02 * j;
            const double e = eta(rig, phi, mach * a0 / r);
            if (e > best)
            {
                best = e;
                bestPhi = phi;
                bestMach = mach;
            }
        }
    CHECK(best == Catch::Approx(etaMax).margin(1e-4));
    // (the island's flow at the peak's own speed: it moves with the knee)
    const double phiB = bestFlow(rig, bestMach * a0 / r);
    CHECK(bestPhi == Catch::Approx(phiB).margin(0.011));
    CHECK(bestMach == Catch::Approx(1.1).margin(0.021));
    // and falls away: off the island it is lower
    CHECK(eta(rig, phiB, 0.5 * a0 / r) < best - 0.05);
    CHECK(eta(rig, 0.2, 1.1 * a0 / r) < best - 0.05);
}
