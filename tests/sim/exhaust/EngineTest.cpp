// tests/sim/exhaust/EngineTest.cpp: card G's tests in the engine (sim/engine.fm): the builder behind spec.exhaust.model.
// Test 1 (model 0 is the reference, bit for bit, every layout) is the null test's (tests/sim/NullTest.cpp: every layout
// with a wild cam among its pairs); these are tests 2 and 11.
#include "sim/Twin.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <cstdlib>
#include <random>

using namespace tee;

namespace
{
    // One player for the file (its Program lives to the process's end, never destroyed by a static's destructor)
    EnginePlayer& player()
    {
        static EnginePlayer* p = test::enginePlayer(48000.0).release();
        return *p;
    }

    Audio render(const EngineBuild& build, const EngineScenario& scenario, double seconds)
    {
        RenderOptions options;
        options.seconds = seconds;
        RenderResult result;
        std::string error;
        const bool ok = renderEngine(player(), build, scenario, options, result, error);
        INFO(error);
        REQUIRE(ok);
        return result.audio;
    }
} // namespace

// Card G, test 2: the assignment table at its defaults (the layout's own topology: all into one, a collector a bank
// into one pipe, dual with an H) built by the builder gives the reference's sound: free revs (start, idle, blips, the
// limiter, the overrun, off, a restart), every layout, model 1 against model 0. The builder starts its pipes as the
// reference does at a configure (built at every configure, so its pipes carry the reference's history) and draws its
// noise from the engine's stream in the reference's order. Measured: identical to the bit, every layout.
TEST_CASE("model 1 at its defaults is model 0 in the engine, every layout", "[exhaust][engine]")
{
    const EngineScenario scenario = test::engineScenario("free-revs");
    double worst = 1e9;
    for (int layout = 0; layout < 17; ++layout)
    {
        EngineBuild b = test::engineBuild(player(), "v8");
        b.values["engine.layout"] = {static_cast<double>(layout)};
        b.values["exhaust.model"] = {0.0};
        const Audio a = render(b, scenario, 8.0);
        b.values["exhaust.model"] = {1.0};
        const Audio m = render(b, scenario, 8.0);
        const double dl = test::nullDepth(a.left, m.left);
        const double dr = test::nullDepth(a.right, m.right);
        worst = std::min(worst, std::min(dl, dr));
        std::printf("engine, layout %2d: model 1 against model 0: %s / %s dB under the signal (peak %.3f)\n", layout,
                    std::isinf(dl) ? "identical" : std::to_string(dl).c_str(),
                    std::isinf(dr) ? "identical" : std::to_string(dr).c_str(), test::peak(a.left));
        INFO("layout " << layout);
        CHECK(test::peak(a.left) > 0.05);
        CHECK(dl >= 60.0);
        CHECK(dr >= 60.0);
    }
    std::printf("engine: model 1 at its defaults against model 0, the worst depth %s\n",
                worst > 1.0e8 || std::isinf(worst) ? "none: identical to the bit" : (std::to_string(worst) + " dB").c_str());
}

namespace
{
    // A random description of the exhaust: tables, sides and every slot's kind (spec.exhaust's paths)
    void randomExhaust(EnginePlayer& p, std::mt19937& g)
    {
        std::uniform_int_distribution<int> any(0, 1 << 20);
        auto pick = [&](int n) { return any(g) % n; };
        auto set = [&](const std::string& path, double v, std::size_t element = 0) {
            const int i = p.specIndex(path);
            REQUIRE(i >= 0);
            p.setSpec(static_cast<std::size_t>(i), v, element);
        };
        for (std::size_t i = 0; i < 16; ++i)
            set("exhaust.primaryTo", pick(10) == 0 ? -1 : (pick(3) == 0 ? -2 : pick(4)), i);
        for (std::size_t c = 0; c < 8; ++c)
        {
            set("exhaust.collectorTo", pick(5) == 0 ? 6 + pick(4) : (pick(3) == 0 ? -1 : pick(2)), c);
            set("exhaust.colVol", pick(5) == 0 ? 0.001 : 0.0, c);
        }
        set("exhaust.sides", pick(3));
        set("exhaust.crossKind", pick(3));
        set("exhaust.crossAt", pick(2));
        set("exhaust.primStepAt", pick(4) == 0 ? 0.4 : 0.0);
        for (std::size_t s = 0; s < 2; ++s)
        {
            set("exhaust.converter", pick(3), s);
            set("exhaust.resonator", pick(4), s);
            set("exhaust.resHz", 80.0 + pick(500), s);
            set("exhaust.mufKind", pick(4), s);
            set("exhaust.bypass", pick(4), s);
            set("exhaust.tips", 1 + pick(2), s);
        }
    }
} // namespace

// Card G, test 11: the engine running on random pedals for ten minutes (TEE_EXHAUST_SOAK_SECONDS: another length),
// its exhaust's description changed to a random one every 4 s (rebuilt under the exhaust's own fade), now and then the
// model switched: every sample finite and within full scale; after the key is off and 6 s have passed, no pressure
// stays in the pipes (the means at the collectors and the heads under 50 Pa) and the output is silent.
TEST_CASE("random exhaust topologies changed while the engine runs", "[exhaust][engine][soak]")
{
    double seconds = 600.0;
    if (const char* env = std::getenv("TEE_EXHAUST_SOAK_SECONDS"); env != nullptr && std::atof(env) > 0.0)
        seconds = std::atof(env);
    EnginePlayer& p = player();
    p.reset();
    applyBuild(p, test::engineBuild(p, "v8"));
    std::string error;
    REQUIRE(p.setSpec("exhaust.model", 1.0, error));
    std::mt19937 g(4242u);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    EnginePlayer::Controls& c = p.controls();
    c.mode = EnginePlayer::kModeBench;
    c.ignition = EnginePlayer::kIgnitionStart;
    std::vector<float> l(512), r(512);
    bool finite = true;
    double peakAll = 0.0;
    int rebuilds = 0;
    const int calls = static_cast<int>(seconds * 48000.0 / 512.0);
    for (int k = 0; k < calls; ++k)
    {
        if (k == 40)
            c.ignition = EnginePlayer::kIgnitionOn;
        if (k % 25 == 0)
            c.throttle = static_cast<float>(unit(g) < 0.3 ? 0.0 : unit(g));
        if (k % 375 == 200)
        {
            randomExhaust(p, g);
            if (unit(g) < 0.1)
                REQUIRE(p.setSpec("exhaust.model", unit(g) < 0.5 ? 0.0 : 1.0, error));
            ++rebuilds;
        }
        p.process(l.data(), r.data(), 512);
        for (int i = 0; i < 512; ++i)
        {
            finite = finite && std::isfinite(l[i]) && std::isfinite(r[i]);
            peakAll = std::max(peakAll, static_cast<double>(std::max(std::fabs(l[i]), std::fabs(r[i]))));
        }
    }
    // the key off, then quiet
    REQUIRE(p.setSpec("exhaust.model", 1.0, error));
    c.ignition = EnginePlayer::kIgnitionOff;
    c.throttle = 0.0f;
    double tail = 0.0;
    for (int k = 0; k < static_cast<int>(6.0 * 48000.0 / 512.0); ++k)
    {
        p.process(l.data(), r.data(), 512);
        if (k > static_cast<int>(5.0 * 48000.0 / 512.0))
            for (int i = 0; i < 512; ++i)
                tail = std::max(tail, static_cast<double>(std::max(std::fabs(l[i]), std::fabs(r[i]))));
    }
    const double pt = std::fabs(test::field(p, "exh.b.pTurb"));
    const double pc = std::max(std::fabs(test::field(p, "exh.b.pConv[0]")), std::fabs(test::field(p, "exh.b.pConv[1]")));
    std::printf("engine soak: %.0f s, %d descriptions, every sample finite %s, largest %.3f; 6 s after the key is off: "
                "output %.3g, mean pressure at the collectors %.3g Pa and the heads %.3g Pa\n",
                seconds, rebuilds, finite ? "yes" : "no", peakAll, tail, pt, pc);
    CHECK(finite);
    CHECK(peakAll <= 1.0);
    CHECK(tail < 1.0e-3);
    CHECK(pt < 50.0);
    CHECK(pc < 50.0);
}

