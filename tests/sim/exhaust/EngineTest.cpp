// tests/sim/exhaust/EngineTest.cpp: card G's tests in the engine (sim/engine.fm): the builder behind spec.exhaust.model.
// Test 1 (model 0 is the reference, bit for bit, every layout) is the null test's (tests/sim/NullTest.cpp: every layout
// with a wild cam among its pairs); these are tests 2 and 11.
#include "sim/Twin.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <filesystem>
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


// Card G, test 10: with card F's machine (boost.model 1, a 2 L four with one turbo; and the V8 with two, a bank each)
// and the builder (exhaust.model 1), the collectors end in the turbines' housings and the main pipes begin at their
// outlets. It runs (every sample finite, within full scale), it makes boost, the junction is passive (the key off, the
// pressures in the pipes and the housings die away: the engine stops computing once it falls silent, so what is left
// is where the decay had reached then, a few tens of Pa, falling), and the downpipe's head swings less than the
// collector's end at the housing, by itself (the Scope's pipe-end pressures, sampled every 128 samples at full
// throttle).
TEST_CASE("the turbine's housing: card F's machine on the builder's collectors", "[exhaust][engine][turbine]")
{
    struct Case
    {
        const char* name;
        int layout;
        double displacement;
        int kind;
    };
    for (const Case& k : {Case{"2 L four, one turbo", 5, 0.002, 1}, Case{"V8, two turbos", 12, 0.0058, 2}})
    {
        EnginePlayer& p = player();
        p.reset();
        applyBuild(p, test::engineBuild(p, "v8"));
        std::string error;
        REQUIRE(p.setSpec("engine.layout", k.layout, error));
        REQUIRE(p.setSpec("engine.displacement", k.displacement, error));
        REQUIRE(p.setSpec("boost.kind", k.kind, error));
        REQUIRE(p.setSpec("boost.model", 1, error));
        REQUIRE(p.setSpec("exhaust.model", 1, error));
        EnginePlayer::Controls& c = p.controls();
        c.mode = EnginePlayer::kModeBench;
        c.ignition = EnginePlayer::kIgnitionStart;
        c.scopeOn = 1;
        std::vector<float> l(512), r(512);
        bool finite = true;
        auto run = [&](double seconds, std::vector<double>* coll, std::vector<double>* down) {
            const int calls = static_cast<int>(seconds * 48000.0 / 512.0);
            int lastRefresh = -1;
            for (int n = 0; n < calls; ++n)
            {
                p.process(l.data(), r.data(), 512);
                for (int i = 0; i < 512; ++i)
                    finite = finite && std::isfinite(l[i]) && std::fabs(l[i]) <= 1.0f;
                const int refresh = static_cast<int>(test::field(p, "scope.exhaustRefreshes"));
                if (coll != nullptr && refresh != lastRefresh)
                {
                    lastRefresh = refresh;
                    coll->push_back(test::field(p, "scope.ductFar[32]"));
                    down->push_back(test::field(p, "scope.ductNear[48]"));
                }
            }
        };
        run(1.5, nullptr, nullptr);
        c.ignition = EnginePlayer::kIgnitionOn;
        c.throttle = 1.0f;
        c.holdSpeed = static_cast<float>(4500.0 * 0.10471976);
        run(4.0, nullptr, nullptr);
        std::vector<double> coll, down;
        run(2.0, &coll, &down);
        const double fedNow = test::field(p, "exh.fed");
        const double boost = test::field(p, "tel.boost.pressure");
        auto swing = [](const std::vector<double>& x) {
            double m = 0.0, s = 0.0;
            for (double v : x)
                m += v;
            m /= static_cast<double>(x.size());
            for (double v : x)
                s += (v - m) * (v - m);
            return std::sqrt(s / static_cast<double>(x.size()));
        };
        const double sc = swing(coll);
        const double sd = swing(down);
        // the key off, the engine stopped: what is left
        c.ignition = EnginePlayer::kIgnitionOff;
        c.throttle = 0.0f;
        c.holdSpeed = 0.0f;
        run(6.0, nullptr, nullptr);
        const double pt = std::fabs(test::field(p, "exh.b.pTurb"));
        std::printf("turbine, %s at 4500 rpm full throttle: the housings fed %s, boost %.1f kPa; pressure swing at the "
                    "collector's end %.0f Pa, at the downpipe's head %.0f Pa (%.1f dB); 6 s after the key is off the "
                    "collectors' mean %.2f Pa; finite %s\n",
                    k.name, fedNow != 0.0 ? "yes" : "no", boost / 1000.0, sc, sd, 20.0 * std::log10(sd / sc), pt,
                    finite ? "yes" : "no");
        INFO(k.name);
        CHECK(fedNow != 0.0);
        CHECK(finite);
        CHECK(boost > 5000.0);
        CHECK(coll.size() > 100);
        CHECK(sd < sc);
        CHECK(pt < 100.0);
    }
}

// content/builds/exhausts: every part build loads (every field known), and the V8 with it starts and plays 3 s of
// free revs, every sample finite and within full scale, on the builder (tel.exhaust.model 1)
TEST_CASE("every exhaust build plays", "[exhaust][engine][builds]")
{
    const std::filesystem::path dir = std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "exhausts";
    int n = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir))
    {
        if (entry.path().extension() != ".json")
            continue;
        EnginePlayer& p = player();
        EngineBuild part;
        std::string error, warnings;
        const bool ok = loadBuild(p, entry.path(), part, error, warnings);
        INFO(entry.path().filename().string() << ": " << error << warnings);
        REQUIRE(ok);
        CHECK(warnings.empty());
        EngineBuild b = test::engineBuild(p, "v8");
        for (const auto& [path, v] : part.values)
            b.values[path] = v;
        const Audio a = render(b, test::engineScenario("free-revs"), 3.0);
        bool finite = true;
        for (std::size_t i = 0; i < a.left.size(); ++i)
            finite = finite && std::isfinite(a.left[i]) && std::isfinite(a.right[i]) && std::fabs(a.left[i]) <= 1.0f;
        const double model = test::field(p, "tel.exhaust.model");
        std::printf("build %-36s: peak %.3f, model %.0f, radiating ends %.0f\n", entry.path().filename().string().c_str(),
                    test::peak(a.left), model, test::field(p, "tel.exhaust.ends"));
        CHECK(finite);
        CHECK(test::peak(a.left) > 0.05);
        CHECK(model == 1.0);
        ++n;
    }
    CHECK(n >= 8);
}
