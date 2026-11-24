// tests/sim/boost/SafetyTest.cpp: card F, standalone test 9: the turbine's housing is passive (with the wheel locked
// and no fuel it never returns more wave energy than it took, for random sizes), and ten minutes of random throttle
// chops on random sizes stay finite and bounded. Also what the machine costs a sample.
#include "Rig.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>
#include <memory>
#include <random>
#include <string>
#include <vector>

using tee::test::boost::Module;
using tee::test::boost::Rig;

TEST_CASE("boost: the housing with the wheel locked and no fuel returns no more wave energy than it took", "[boost][passive]")
{
    Module pass("PassRig");
    pass.defaults("spec");
    auto* init = pass.method<void(void*)>("init");
    auto* start = pass.method<void(void*)>("start");
    auto* run = pass.method<void(void*, int)>("run");
    {
        const tee::ScopedFlushDenormals flush;
        init(pass.data());
    }
    std::mt19937 random(20261124);
    auto between = [&](double lo, double hi) { return std::uniform_real_distribution<double>(lo, hi)(random); };
    double worst = 0.0;
    for (int trial = 0; trial < 40; ++trial)
    {
        pass.set("spec.kind", 1);
        pass.set("spec.model", 1);
        pass.set("spec.scrolls", 1 + trial % 2);
        pass.set("spec.housingVolume", std::exp(between(std::log(5.0e-5), std::log(2.0e-3))));
        pass.set("spec.turbAR", between(0.3, 1.6));
        pass.set("spec.turbWheel", between(0.03, 0.1));
        pass.set("spec.gate", trial % 3 == 0 ? 1 : 0);
        pass.set("z", std::exp(between(std::log(2.0e4), std::log(2.0e6))));
        pass.set("zd", std::exp(between(std::log(2.0e4), std::log(2.0e6))));
        pass.set("level", std::exp(between(std::log(1.0e3), std::log(2.0e5))));
        pass.set("tGas", 293.15);
        {
            const tee::ScopedFlushDenormals flush;
            start(pass.data());
            run(pass.data(), 24000);
        }
        const double in = pass.get("eIn");
        const double out = pass.get("eOut");
        INFO("trial " << trial << ": in " << in << ", out " << out);
        REQUIRE(std::isfinite(out));
        CHECK(in > 0.0);
        CHECK(out <= in);
        worst = std::fmax(worst, out / in);
    }
    std::printf("passivity: the most a housing returned was %.3f of what it took\n", worst);
}

// Random builds driven by random chops and stabs for `seconds` of simulated time, from `seed`.
static void chops(double seconds, unsigned seed)
{
    std::mt19937 random(seed);
    auto between = [&](double lo, double hi) { return std::uniform_real_distribution<double>(lo, hi)(random); };
    double simulated = 0.0;
    double peakBoost = 0.0;
    double peakShare = 0.0;
    int builds = 0;
    const auto t0 = std::chrono::steady_clock::now();
    while (simulated < seconds)
    {
        // a build: a turbo (any layout), or a supercharger; random sizes; no blow-off valve half the time
        Rig rig;
        const int kind = 1 + static_cast<int>(between(0.0, 3.999));
        rig.set("spec.kind", kind == 4 ? 4 : 1);
        rig.set("spec.layout", kind == 4 ? 0 : static_cast<int>(between(1.0, 5.999)));
        rig.set("spec.charger", static_cast<int>(between(0.0, 2.999)));
        rig.set("spec.model", 1);
        rig.set("spec.size", between(0.6, 1.8));
        rig.set("spec.scrolls", between(0.0, 1.0) < 0.5 ? 1 : 2);
        rig.set("spec.blowOffKind", static_cast<int>(between(0.0, 2.999)));
        rig.set("spec.gate", between(0.0, 1.0) < 0.5 ? 0 : 1);
        rig.set("spec.vanes", between(0.0, 1.0) < 0.3 ? 1 : 0);
        rig.set("spec.target", between(0.0, 1.0) < 0.5 ? 0.0 : between(40000.0, 200000.0));
        rig.set("spec.gateSpring", between(30000.0, 150000.0));
        rig.set("spec.antiLag", between(0.0, 1.0) < 0.3 ? 1 : 0);
        rig.set("spec.assist", between(0.0, 1.0) < 0.3 ? 4000.0 : 0.0);
        rig.set("pulsed", between(0.0, 1.0) < 0.7 ? 1 : 0);
        rig.spec("rev", 0);
        ++builds;
        std::string what;
        for (const char* f : {"kind", "layout", "charger", "size", "scrolls", "blowOffKind", "gate", "vanes", "target",
                              "gateSpring", "antiLag", "assist"})
            what += std::string(f) + " " + std::to_string(rig.get(std::string("spec.") + f)) + ", ";
        double rpm = between(1500.0, 7500.0);
        for (int seg = 0; seg < 40 && simulated < seconds; ++seg)
        {
            if (between(0.0, 1.0) < 0.3)
                rpm = between(1000.0, 8000.0);
            rig.set("rpm", rpm);
            // a chop, a stab, or anything between
            const double r = between(0.0, 1.0);
            rig.set("throttle", r < 0.4 ? 0.0 : (r < 0.8 ? 1.0 : between(0.0, 1.0)));
            rig.set("boost.cmdAntiLag", rig.get("throttle") < 0.1 ? 0.004 : 0.0);
            const double dt = between(0.05, 1.0);
            rig.run(dt);
            simulated += dt;
            INFO("build " << builds << " (" << what << "pulsed " << rig.get("pulsed") << "), " << simulated << " s, "
                 << rpm << " rpm, throttle " << rig.get("throttle"));
            REQUIRE(rig.get("bad") == 0.0);
            const double boost = rig.boost();
            REQUIRE(std::isfinite(boost));
            REQUIRE(boost < 400000.0);
            REQUIRE(boost > -100000.0);
            for (int t = 0; t < 4; ++t)
            {
                const std::string s = std::to_string(t);
                const double w = rig.get("boost.shaft[" + s + "].omega");
                const double wmax = rig.get("boost.omegaMax[" + s + "]");
                REQUIRE(std::isfinite(w));
                if (wmax > 0.0)
                {
                    REQUIRE(w < 1.5 * wmax);
                    peakShare = std::fmax(peakShare, w / wmax);
                }
            }
            for (int h = 0; h < 8; ++h)
            {
                const double p = rig.get("boost.house[" + std::to_string(h) + "].p");
                REQUIRE(std::isfinite(p));
                REQUIRE(p < 2.0e6);
            }
            for (const char* f : {"boost.outInlet", "boost.outVent", "boost.outCase", "boost.outBang", "boost.v1.t",
                                  "boost.v2.t", "boost.tCore"})
                REQUIRE(std::isfinite(rig.get(f)));
            REQUIRE(std::fabs(rig.get("boost.outInlet")) < 100.0);
            peakBoost = std::fmax(peakBoost, boost);
        }
    }
    const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("soak: %.0f s simulated on %d builds in %.1f s; the most boost %.0f kPa, the fastest shaft %.2f of its "
                "limit\n", simulated, builds, wall, peakBoost / 1000.0, peakShare);
}

// The quick form (the default run, 01-rules.md section 3): the same property on a minute, the same seed, so its first
// builds are the soak's first.
TEST_CASE("boost: a minute of random throttle chops stays finite and bounded", "[boost][soak]")
{
    chops(60.0, 424242);
}

TEST_CASE("boost: ten minutes of random throttle chops stay finite and bounded", "[boost][soak][nightly]")
{
    chops(600.0, 424242);
}

TEST_CASE("boost: what the machine costs a sample", "[boost][cost]")
{
    // the rig alone (natural, nothing to do: the machine returns at once), then with the machine in several builds,
    // timed in turns (the least of five a build, so that the others' load on a shared machine counts least); the
    // difference is the machine's
    struct Build
    {
        const char* name;
        int kind;
        int layout;
        int scrolls;
    };
    const Build builds[] = {{"the rig alone", 0, 0, 1}, {"one turbo", 1, 1, 1}, {"one twin-scroll turbo", 1, 1, 2},
                            {"two turbos", 1, 2, 1}, {"two twin-scroll turbos", 1, 2, 2}, {"a Roots blower", 4, 0, 1}};
    const int n = sizeof(builds) / sizeof(builds[0]);
    std::vector<std::unique_ptr<Rig>> rigs;
    std::vector<double> best(n, 1.0e9);
    for (const Build& b : builds)
    {
        auto rig = std::make_unique<Rig>();
        rig->set("spec.kind", b.kind);
        rig->set("spec.layout", b.layout);
        rig->set("spec.scrolls", b.scrolls);
        rig->set("spec.model", 1);
        rig->spec("rev", 0);
        rig->set("rpm", 5000.0);
        rig->set("pulsed", 1);
        rig->run(1.0);
        rigs.push_back(std::move(rig));
    }
    for (int round = 0; round < 5; ++round)
        for (int i = 0; i < n; ++i)
        {
            const auto t0 = std::chrono::steady_clock::now();
            rigs[i]->run(1.0);
            const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
            best[i] = std::fmin(best[i], s / 96000.0 * 1.0e9);
        }
    std::printf("cost a sample (ns, the least of five; the machine's is over the rig alone):\n");
    for (int i = 0; i < n; ++i)
        std::printf("  %-24s %7.1f%s\n", builds[i].name, i == 0 ? best[0] : best[i] - best[0], i == 0 ? " (the rig)" : "");
    CHECK(best[3] - best[0] < 2000.0);
}
