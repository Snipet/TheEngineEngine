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
static void engineIdentity(bool full);

TEST_CASE("model 1 at its defaults is model 0 in the engine, every layout", "[exhaust][engine]")
{
    engineIdentity(false);
}

TEST_CASE("model 1 at its defaults is model 0 in the engine, every layout, in full", "[exhaust][engine][nightly]")
{
    engineIdentity(true);
}

static void engineIdentity(bool full)
{
    const EngineScenario scenario = test::engineScenario("free-revs");
    double worst = 1e9;
    // (quick: a four's, a V8's single and dual layouts; in full every layout)
    const std::vector<int> layouts = full ? std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}
                                          : std::vector<int>{5, 10, 12};
    for (int layout : layouts)
    {
        EngineBuild b = test::engineBuild(player(), "v8");
        b.values["engine.layout"] = {static_cast<double>(layout)};
        b.values["exhaust.model"] = {0.0};
        const Audio a = render(b, scenario, 8.0);
        b.values["exhaust.model"] = {1.0};
        b.values["exhaust.portHeat"] = {0.0};   // (the port's cooling off: the reference has none)
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
static void engineSoak(bool full);

TEST_CASE("random exhaust topologies changed while the engine runs", "[exhaust][engine][soak]")
{
    engineSoak(false);
}

TEST_CASE("random exhaust topologies changed while the engine runs, in full", "[exhaust][engine][soak][nightly]")
{
    engineSoak(true);
}

static void engineSoak(bool full)
{
    double seconds = full ? 600.0 : 6.0;
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
static void turbineHousing(bool full);

TEST_CASE("the turbine's housing: card F's machine on the builder's collectors", "[exhaust][engine][turbine]")
{
    turbineHousing(false);
}

TEST_CASE("the turbine's housing: card F's machine on the builder's collectors, in full", "[exhaust][engine][turbine][nightly]")
{
    turbineHousing(true);
}

static void turbineHousing(bool full)
{
    struct Case
    {
        const char* name;
        int layout;
        double displacement;
        int kind;
        int gate;   // spec.boost.gate: 1 external (its own pipe, to the air at the front)
    };
    // (quick: the two with external gates, which feed the housings and the gates' pipes; in full the internal too)
    std::vector<Case> cases = {Case{"2 L four, one turbo, an external gate", 5, 0.002, 1, 1},
                               Case{"V8, two turbos, external gates", 12, 0.0058, 2, 1}};
    if (full)
    {
        cases.push_back(Case{"2 L four, one turbo", 5, 0.002, 1, 0});
        cases.push_back(Case{"V8, two turbos", 12, 0.0058, 2, 0});
    }
    for (const Case& k : cases)
    {
        EnginePlayer& p = player();
        p.reset();
        applyBuild(p, test::engineBuild(p, "v8"));
        std::string error;
        REQUIRE(p.setSpec("engine.layout", k.layout, error));
        REQUIRE(p.setSpec("engine.displacement", k.displacement, error));
        REQUIRE(p.setSpec("boost.kind", k.kind, error));
        REQUIRE(p.setSpec("boost.model", 1, error));
        REQUIRE(p.setSpec("boost.gate", k.gate, error));
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
        const int gates = static_cast<int>(test::field(p, "exh.b.nGates"));
        double gateMass = 0.0;
        for (int t = 0; t < gates; ++t)
            gateMass += test::field(p, "boost.mach.gateMass[" + std::to_string(t) + "]");
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
                    "collectors' mean %.2f Pa; external gates' pipes %d, through them %.4f kg/s; finite %s\n",
                    k.name, fedNow != 0.0 ? "yes" : "no", boost / 1000.0, sc, sd, 20.0 * std::log10(sd / sc), pt,
                    gates, gateMass, finite ? "yes" : "no");
        INFO(k.name);
        CHECK(fedNow != 0.0);
        CHECK(finite);
        CHECK(boost > 5000.0);
        CHECK(coll.size() > 100);
        CHECK(sd < sc);
        CHECK(pt < 100.0);
        CHECK(gates == (k.gate != 0 ? k.kind : 0));
    }
}

// content/builds/exhausts: every part build loads (every field known), and the V8 with it starts and plays 3 s of
// free revs, every sample finite and within full scale, on the builder (tel.exhaust.model 1)
static void buildsPlay(bool full);

TEST_CASE("every exhaust build plays", "[exhaust][engine][builds]")
{
    buildsPlay(false);
}

TEST_CASE("every exhaust build plays, in full", "[exhaust][engine][builds][nightly]")
{
    buildsPlay(true);
}

static void buildsPlay(bool full)
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
        const Audio a = render(b, test::engineScenario("free-revs"), full ? 3.0 : 1.5);
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


// Card C's settle (spec.exhaust.heatSettle): at full load, a settle puts the exhaust's walls where the load holds
// them: over the next 20 s they move by under 3 %, where unsettled they move by much more. Both models.
static void heatSettles(bool full);

TEST_CASE("the exhaust's heat settles on request", "[exhaust][engine][settle]")
{
    heatSettles(false);
}

TEST_CASE("the exhaust's heat settles on request, in full", "[exhaust][engine][settle][nightly]")
{
    heatSettles(true);
}

static void heatSettles(bool full)
{
    // (quick: the builder, 10 s after; in full both models, 20 s)
    for (int model : full ? std::vector<int>{0, 1} : std::vector<int>{1})
    {
        double moved[2] = {0.0, 0.0};
        for (int settle : {0, 1})
        {
            EnginePlayer& p = player();
            p.reset();
            applyBuild(p, test::engineBuild(p, "v8"));
            std::string error;
            REQUIRE(p.setSpec("exhaust.model", model, error));
            EnginePlayer::Controls& c = p.controls();
            c.mode = EnginePlayer::kModeBench;
            c.ignition = EnginePlayer::kIgnitionStart;
            std::vector<float> l(512), r(512);
            auto run = [&](double s) {
                for (int k = 0; k < static_cast<int>(s * 48000.0 / 512.0); ++k)
                    p.process(l.data(), r.data(), 512);
            };
            run(1.5);
            c.ignition = EnginePlayer::kIgnitionOn;
            c.throttle = 1.0f;
            c.holdSpeed = static_cast<float>(4000.0 * 0.10471976);
            run(4.0);
            if (settle != 0)
            {
                REQUIRE(p.setSpec("exhaust.heatSettle", 1, error));
                run(0.1);
            }
            const double w0 = test::field(p, "tel.exhaust.wallTemperature[3]");
            run(full ? 20.0 : 10.0);
            const double w1 = test::field(p, "tel.exhaust.wallTemperature[3]");
            moved[settle] = std::fabs(w1 - w0) / w0;
            std::printf("settle, model %d, %s: main pipe's wall %.0f K, 20 s later %.0f K (%.1f %%)\n", model,
                        settle != 0 ? "settled" : "not settled", w0, w1, 100.0 * moved[settle]);
        }
        INFO("model " << model);
        CHECK(moved[1] < 0.03);
        CHECK(moved[0] > 3.0 * moved[1]);
    }
}

// Finding X-013: the exhaust's telemetry is physical from the first block (written at every configure, not only
// every 16 samples)
TEST_CASE("the exhaust's telemetry is physical from the first block", "[exhaust][engine][telemetry]")
{
    for (int model : {0, 1})
    {
        EnginePlayer& p = player();
        p.reset();
        applyBuild(p, test::engineBuild(p, "v8"));
        std::string error;
        REQUIRE(p.setSpec("exhaust.model", model, error));
        std::vector<float> l(512), r(512);
        p.process(l.data(), r.data(), 512);
        const double th = test::field(p, "tel.exhaust.headerTemperature");
        const double tw = test::field(p, "tel.exhaust.wallTemperature[0]");
        INFO("model " << model << ": header gas " << th << " K, wall " << tw << " K");
        CHECK(th > 200.0);
        CHECK(th < 2000.0);
        CHECK(tw > 200.0);
    }
}

// After the passivity fix: the card's typical dual system (content/builds/exhausts/dual-x-resonated-valved.json: an X
// after the main pipes, quarter-wave stubs, chambered mufflers with valves by speed and load) on the V8, 60 s of
// random throttle and dyno speeds (the valves opening and shutting): every sample finite and within full scale, and
// after the key is off the output dies away
static void dualXSoak(bool full);

TEST_CASE("the dual X with stubs and valves soaks", "[exhaust][engine][soak]")
{
    dualXSoak(false);
}

TEST_CASE("the dual X with stubs and valves soaks, in full", "[exhaust][engine][soak][nightly]")
{
    dualXSoak(true);
}

static void dualXSoak(bool full)
{
    EnginePlayer& p = player();
    p.reset();
    EngineBuild part;
    std::string error, warnings;
    REQUIRE(loadBuild(p, std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/exhausts/dual-x-resonated-valved.json",
                      part, error, warnings));
    EngineBuild b = test::engineBuild(p, "v8");
    for (const auto& [path, v] : part.values)
        b.values[path] = v;
    applyBuild(p, b);
    std::mt19937 g(99u);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    EnginePlayer::Controls& c = p.controls();
    c.mode = EnginePlayer::kModeBench;
    c.ignition = EnginePlayer::kIgnitionStart;
    std::vector<float> l(512), r(512);
    bool finite = true;
    double peakAll = 0.0;
    double valveMoves = 0.0, lastValve = 0.0;
    const int calls = static_cast<int>((full ? 60.0 : 10.0) * 48000.0 / 512.0);
    for (int k = 0; k < calls; ++k)
    {
        // started on the bench (1.5 s on the starter), then on the dyno (mode 2: the crank held at a speed, the
        // throttle where it is put): a new speed and throttle every second
        if (k == 140)
            c.ignition = EnginePlayer::kIgnitionOn;
        if (k >= 240 && k % 90 == 0)
        {
            c.mode = 2;
            c.throttle = static_cast<float>(unit(g));
            c.dynoSpeed = static_cast<float>((1500.0 + 5000.0 * unit(g)) * 0.10471976);
        }
        p.process(l.data(), r.data(), 512);
        for (int i = 0; i < 512; ++i)
        {
            finite = finite && std::isfinite(l[i]) && std::isfinite(r[i]);
            peakAll = std::max(peakAll, static_cast<double>(std::max(std::fabs(l[i]), std::fabs(r[i]))));
        }
        const double v = test::field(p, "tel.exhaust.valve[0]");
        valveMoves += std::fabs(v - lastValve);
        lastValve = v;
    }
    c.mode = EnginePlayer::kModeBench;
    c.ignition = EnginePlayer::kIgnitionOff;
    c.throttle = 0.0f;
    double tail = 0.0;
    for (int k = 0; k < static_cast<int>(6.0 * 48000.0 / 512.0); ++k)
    {
        p.process(l.data(), r.data(), 512);
        if (k > static_cast<int>(5.0 * 48000.0 / 512.0))
            for (int i = 0; i < 512; ++i)
                tail = std::max(tail, static_cast<double>(std::fabs(l[i])));
    }
    std::printf("dual X soak: 60 s, finite %s, largest %.3f, the valve travelled %.1f (open to shut and back counts 2), "
                "6 s after the key is off %.3g\n", finite ? "yes" : "no", peakAll, valveMoves, tail);
    CHECK(finite);
    CHECK(peakAll <= 1.0);
    CHECK(valveMoves > 2.0);
    CHECK(tail < 1.0e-3);
}

// The lead's ruling: the sizes a slider drags glide (XBuild::glide) instead of rebuilding under the fade. The V8 on
// the dyno at 3000 rpm, half throttle, its main pipes 1.5 m: the main pipes dragged to 3 m and their bore from 60 to
// 80 mm in 20 steps over a second. Measured: the fade never moves (no rebuild), the largest step between samples while
// they glide against the second before, and the glide within 0.1 % of its target a second after the last step (0.2 s
// its time constant). A kind (the
// mufflers' change) still rebuilds: the fade goes down.
TEST_CASE("lengths and bores dragged glide, kinds rebuild", "[exhaust][engine][glide]")
{
    EnginePlayer& p = player();
    p.reset();
    EngineBuild b = test::engineBuild(p, "v8");
    b.values["exhaust.model"] = {1.0};
    b.values["exhaust.mainLen"] = {1.5, 1.5};
    b.values["exhaust.mainD"] = {0.06, 0.06};
    applyBuild(p, b);
    EnginePlayer::Controls& c = p.controls();
    c.mode = EnginePlayer::kModeBench;
    c.ignition = EnginePlayer::kIgnitionStart;
    std::vector<float> l(512), r(512);
    auto block = [&](double& step, double& fadeMin) {
        p.process(l.data(), r.data(), 512);
        for (int i = 1; i < 512; ++i)
            step = std::max(step, static_cast<double>(std::fabs(l[i] - l[i - 1])));
        fadeMin = std::min(fadeMin, test::field(p, "exh.fadeG"));
    };
    double unused = 0.0, fadeUnused = 1.0;
    for (int k = 0; k < 140; ++k)
        block(unused, fadeUnused);
    c.ignition = EnginePlayer::kIgnitionOn;
    c.mode = 2;
    c.dynoSpeed = static_cast<float>(3000.0 * 0.10471976);
    c.throttle = 0.5f;
    for (int k = 0; k < 300; ++k)
        block(unused, fadeUnused);
    double stepBefore = 0.0, fadeBefore = 1.0;
    for (int k = 0; k < 94; ++k)
        block(stepBefore, fadeBefore);
    // (tel.exhaust.portPressure, N1's: the ports' mean, absolute, between the air and a few bar)
    const double port = test::field(p, "tel.exhaust.portPressure");
    std::printf("glide: before, at 3000 rpm half throttle, the ports' mean pressure %.0f Pa absolute\n", port);
    CHECK(port > 101325.0);
    CHECK(port < 400000.0);
    double stepGlide = 0.0, fadeGlide = 1.0;
    for (int k = 0; k < 100; ++k)
    {
        if (k % 5 == 0)
        {
            const double x = (k / 5 + 1) / 20.0;
            for (std::size_t s = 0; s < 2; ++s)
            {
                p.setSpec(static_cast<std::size_t>(p.specIndex("exhaust.mainLen")), 1.5 + 1.5 * x, s);
                p.setSpec(static_cast<std::size_t>(p.specIndex("exhaust.mainD")), 0.06 + 0.02 * x, s);
            }
        }
        block(stepGlide, fadeGlide);
    }
    for (int k = 0; k < 94; ++k)
        block(unused, fadeGlide);
    const double lenNow = test::field(p, "exh.b.gs.mainLen[0]");
    const double dNow = test::field(p, "exh.b.gs.mainD[1]");
    for (std::size_t s = 0; s < 2; ++s)
        p.setSpec(static_cast<std::size_t>(p.specIndex("exhaust.mufKind")), 1, s);
    double fadeKind = 1.0;
    for (int k = 0; k < 20; ++k)
        block(unused, fadeKind);
    std::printf("glide: the main pipes 1.5 to 3 m, 60 to 80 mm in a second: the fade's least %.3f (before %.3f), the "
                "largest step between samples %.4f against %.4f the second before; a second after, %.4f m and %.4f m; a "
                "muffler's kind changed: the fade's least %.3f\n", fadeGlide, fadeBefore, stepGlide, stepBefore, lenNow,
                dNow, fadeKind);
    CHECK(fadeGlide == 1.0);
    CHECK(stepGlide <= 1.5 * stepBefore);
    CHECK(std::fabs(lenNow - 3.0) < 0.003);
    CHECK(std::fabs(dNow - 0.08) < 0.00008);
    CHECK(fadeKind < 0.5);
}

// The exhaust port's cooling (spec.exhaust.portHeat, Caton and Heywood's correlation): the V8 on the engine dyno, the
// throttle open, warm, at 1500, 3000 and 5000 rpm (8 s each): the gas at the valve against the gas leaving the port,
// and the heat the port walls take against the brake power. Measured and printed; checked: the port cools the gas
// (never warms it), by less than 400 K, and the header reads under the valve.
static void portCools(bool full);

TEST_CASE("the exhaust port's walls cool the gas", "[exhaust][engine][port]")
{
    portCools(false);
}

TEST_CASE("the exhaust port's walls cool the gas, in full", "[exhaust][engine][port][nightly]")
{
    portCools(true);
}

static void portCools(bool full)
{
    EnginePlayer& p = player();
    p.reset();
    EngineBuild b = test::engineBuild(p, "v8");
    b.values["exhaust.model"] = {1.0};
    applyBuild(p, b);
    EnginePlayer::Controls& c = p.controls();
    c.mode = EnginePlayer::kModeBench;
    c.ignition = EnginePlayer::kIgnitionStart;
    std::vector<float> l(512), r(512);
    auto run = [&](double s) {
        for (int k = 0; k < static_cast<int>(s * 48000.0 / 512.0); ++k)
            p.process(l.data(), r.data(), 512);
    };
    run(1.5);
    c.ignition = EnginePlayer::kIgnitionOn;
    run(1.0);
    for (double rpm : full ? std::vector<double>{1500.0, 3000.0, 5000.0} : std::vector<double>{1500.0, 5000.0})
    {
        c.mode = 2;
        c.dynoSpeed = static_cast<float>(rpm * 0.10471976);
        c.throttle = 1.0f;
        run(full ? 8.0 : 6.0);
        const double tv = test::field(p, "tel.exhaust.portTemperature");
        const double te = test::field(p, "tel.exhaust.portExitTemperature");
        const double th = test::field(p, "tel.exhaust.headerTemperature");
        const double q = test::field(p, "tel.exhaust.portHeat");
        const double power = test::field(p, "tel.engine.torque") * rpm * 0.10471976;
        std::printf("port at %.0f rpm full throttle: at the valve %.0f K, leaving the port %.0f K (%.0f K less), the "
                    "header %.0f K; the port walls take %.1f kW (brake %.1f kW)\n", rpm, tv, te, tv - te, th, q / 1000.0,
                    power / 1000.0);
        CHECK(te <= tv);
        CHECK(tv - te < 400.0);
        CHECK(q > 0.0);
        CHECK(th < tv);
    }
}

// A twin-turbo inline six into ONE exhaust (content/builds/archetypes/straight-six-twin-turbo-30.json, its exhaust
// sides 1, the front three cylinders' collector and the rear three's): each collector in its own turbo's housing
// (F's housingOf on one bank: by group), and both turbos' outlets into the one downpipe (Boost::exhaustSides, called
// at every configure). Before it, the second turbo's outlet had no pipe: its shaft stood. Measured: both shafts' speeds
// at 4300 rpm full throttle on the dyno, and the boost.
TEST_CASE("a twin-turbo six into one pipe spins both turbos", "[exhaust][engine][turbine]")
{
    EnginePlayer& p = player();
    p.reset();
    EngineBuild b;
    std::string error, warnings;
    REQUIRE(loadBuild(p, std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/archetypes/straight-six-twin-turbo-30.json",
                      b, error, warnings));
    b.values["exhaust.model"] = {1.0};
    b.values["exhaust.sides"] = {1.0};
    b.values["exhaust.primaryTo"] = {0, 0, 0, 1, 1, 1};
    b.values["exhaust.collectorTo"] = {0, 0};
    applyBuild(p, b);
    EnginePlayer::Controls& c = p.controls();
    c.mode = EnginePlayer::kModeBench;
    c.ignition = EnginePlayer::kIgnitionStart;
    std::vector<float> l(512), r(512);
    bool finite = true;
    auto run = [&](double s) {
        for (int k = 0; k < static_cast<int>(s * 48000.0 / 512.0); ++k)
        {
            p.process(l.data(), r.data(), 512);
            for (int i = 0; i < 512; ++i)
                finite = finite && std::isfinite(l[i]) && std::fabs(l[i]) <= 1.0f;
        }
    };
    run(1.5);
    c.ignition = EnginePlayer::kIgnitionOn;
    c.mode = 2;
    c.dynoSpeed = static_cast<float>(4300.0 * 0.10471976);
    c.throttle = 1.0f;
    run(6.0);
    const double w0 = test::field(p, "tel.boost.shaftSpeed[0]");
    const double w1 = test::field(p, "tel.boost.shaftSpeed[1]");
    const double boost = test::field(p, "tel.boost.pressure");
    const double torque = test::field(p, "tel.engine.torque");
    std::printf("twin six into one pipe at 4300 rpm full throttle: shafts %.0f and %.0f rad/s, boost %.1f kPa, %.0f N m, "
                "the exhaust's sides %d, finite %s\n", w0, w1, boost / 1000.0, torque,
                static_cast<int>(test::field(p, "exh.b.nSides")), finite ? "yes" : "no");
    CHECK(finite);
    CHECK(w0 > 3000.0);
    CHECK(w1 > 3000.0);
    CHECK(std::fabs(w0 - w1) < 0.2 * std::max(w0, w1));
    CHECK(torque > 100.0);
}
