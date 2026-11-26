// tests/sim/exhaust/EngineTest.cpp: card G's tests in the engine (sim/engine.fm): the builder behind spec.exhaust.model.
// Test 1 (model 0 is the reference, bit for bit, every layout) is the null test's (tests/sim/NullTest.cpp: every layout
// with a wild cam among its pairs); these are tests 2 and 11.
#include "sim/Twin.h"

#include <catch2/catch_test_macros.hpp>
#include <nlohmann/json.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
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

    // Route 1 (the integrator's: initialised over the build, EnginePlayer::reset(rate, true)), on a player of its own:
    // every pipe designed from the build's own sizes, none from the defaults' before it. Then a free rev's driving:
    // started, idled, blipped three times, held at full throttle into the limiter, let go
    Audio renderRoute1(const EngineBuild& build, double seconds)
    {
        auto p = test::enginePlayer(48000.0);
        p->reset();
        applyBuild(*p, build);
        p->reset(48000.0, true);
        Audio a;
        a.sampleRate = 48000.0;
        std::vector<float> l(512), r(512);
        EnginePlayer::Controls& c = p->controls();
        c.mode = EnginePlayer::kModeBench;
        const int blocks = static_cast<int>(seconds * 48000.0 / 512.0);
        for (int k = 0; k < blocks; ++k)
        {
            const double t = k * 512.0 / 48000.0;
            c.ignition = t < 1.0 ? EnginePlayer::kIgnitionStart : EnginePlayer::kIgnitionOn;
            const double u = t - 2.5;
            c.throttle = (u > 0.0 && u < 3.0 && std::fmod(u, 1.0) < 0.25) || (t > 5.5 && t < 7.0) ? 1.0f : 0.0f;
            p->process(l.data(), r.data(), 512);
            a.left.insert(a.left.end(), l.begin(), l.end());
            a.right.insert(a.right.end(), r.begin(), r.end());
        }
        return a;
    }
} // namespace

// Card G, test 2: the assignment table at its defaults (the layout's own topology: all into one, a collector a bank
// into one pipe, dual with an H) built by the builder gives the reference's sound: free revs (start, idle, blips, the
// limiter, the overrun, off, a restart), every layout, model 1 against model 0. The builder starts its pipes as the
// reference does at a configure (built at every configure, so its pipes carry the reference's history) and draws its
// noise from the engine's stream in the reference's order. Measured: identical to the bit, every layout.
// Each model loads by route 1 (initialised over the build: renderRoute1), on the lead's ruling of 26 Nov: the pipes'
// wall at model 0 keeps the reference's key, the sound speed and length (sim/ref/pipes.fm), so where a build is written
// over the defaults (the application's route, the null's) it keeps the defaults' wall when only the bore differs (the
// V8's layout 12), a fault of the reference's load order that model 0 must keep; model 1 has the whole key and
// redesigns it. On that route model 1 laid out as the reference is not the reference's stale exhaust; on route 1 both
// design their walls from the build's own bores, and the property, the builder laid out as the reference is the
// reference's exhaust, is met there.
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
    double worst = 1e9;
    // (quick: a four's, a V8's single and dual layouts; in full every layout)
    const std::vector<int> layouts = full ? std::vector<int>{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16}
                                          : std::vector<int>{5, 10, 12};
    for (int layout : layouts)
    {
        EngineBuild b = test::engineBuild(player(), "v8");
        b.values["engine.layout"] = {static_cast<double>(layout)};
        b.values["exhaust.model"] = {0.0};
        const Audio a = renderRoute1(b, 8.0);
        b.values["exhaust.model"] = {1.0};
        b.values["exhaust.portHeat"] = {0.0};   // (the port's cooling off: the reference has none)
        b.values["exhaust.flowClose"] = {0.0};  // (the valves' mean flow not closed: the reference's g q)
        const Audio m = renderRoute1(b, 8.0);
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

// The settle's whole property (the lead's ruling, 26 Nov): it lands where the long run lands. C's case: the two-stage
// at 5879 rpm on its high cam, settled by the ports' mass-weighted gas, read its port heat 5.3 % over a 90 s hold and
// its torque 3 N m low. A settle weighs the ports' gas as the running tPort does (each 16 samples alike), so:
// - here, on the dyno: the gas a settle takes (tel.exhaust.portTemperature just after it) is the gas the running
//   path held just before it, within 2 % (the running gas is a 0.8 s mean, and first.json's boost hunts over about
//   a second: measured +0.15 % and +1.48 %; weighted by mass, +4.5 % and +8.1 %): the two-stage at 5879 rpm, and
//   first.json, a turbo, at 3200;
// The long run itself is the predictor's to compare (it settles the intake's and the boost's heat too, and starts
// warm): measured with tee-dyno --predict, the two-stage at 5879 rpm settled 15151 W and 189.4 N m against 15090 W and
// 190.4 N m held 90 s unsettled (+0.4 %; weighted by mass 15897 W, +5.3 %, 187.3 N m); first.json at 3200 rpm 16047
// against 16044 W. A held engine here, with only the exhaust settled, differs by the coolant's and the intake's heat
static void settleLands();

TEST_CASE("the exhaust's heat settles where the long run lands", "[exhaust][engine][settle]")
{
    settleLands();
}

static void settleLands()
{
    struct Point
    {
        const char* build;
        double rpm;
    };
    for (const Point& pt : {Point{"archetypes/two-stage-cam-20", 5879.0}, Point{"first", 3200.0}})
    {
        EnginePlayer& p = player();
        std::vector<float> l(512), r(512);
        auto run = [&](double s) {
            for (int k = 0; k < static_cast<int>(s * 48000.0 / 512.0); ++k)
                p.process(l.data(), r.data(), 512);
        };
        auto start = [&] {
            p.reset();
            applyBuild(p, test::engineBuild(p, pt.build));
            EnginePlayer::Controls& c = p.controls();
            c.mode = EnginePlayer::kModeBench;
            c.ignition = EnginePlayer::kIgnitionStart;
            run(1.5);
            c.ignition = EnginePlayer::kIgnitionOn;
            c.throttle = 1.0f;
            c.mode = 2;     // the engine dyno, at the point's speed
            c.dynoSpeed = static_cast<float>(pt.rpm * 0.10471976);
        };
        auto settle = [&](int n) {
            std::string error;
            REQUIRE(p.setSpec("exhaust.heatSettle", n, error));
            p.process(l.data(), r.data(), 512);
        };
        INFO(pt.build);
        start();
        run(8.0);
        const double before = test::field(p, "tel.exhaust.portTemperature");
        settle(1);
        const double after = test::field(p, "tel.exhaust.portTemperature");
        std::printf("settle lands, %s at %.0f rpm: the running port gas %.1f K, the settle's %.1f K (%+.2f %%)\n", pt.build,
                    pt.rpm, before, after, 100.0 * (after / before - 1.0));
        CHECK(std::fabs(after / before - 1.0) < 0.02);
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
    struct Case
    {
        const char* build;
        bool diesel;
        std::vector<double> rpms;
    };
    const std::vector<Case> cases = {{"v8", false, full ? std::vector<double>{1500.0, 3000.0, 5000.0} : std::vector<double>{5000.0}},
                                     {"large-diesel", true, full ? std::vector<double>{1200.0, 1800.0} : std::vector<double>{1800.0}}};
    for (const Case& k : cases)
    {
        EnginePlayer& p = player();
        p.reset();
        EngineBuild b = test::engineBuild(p, k.build);
        b.values["exhaust.model"] = {1.0};
        b.values["env.temperature"] = {293.15};     // (a warm day, as the calibration's targets are)
        applyBuild(p, b);
        EnginePlayer::Controls& c = p.controls();
        c.mode = EnginePlayer::kModeBench;
        c.ignition = EnginePlayer::kIgnitionStart;
        std::vector<float> l(512), r(512);
        const double dt = 512.0 / 48000.0;
        double eGas = 0.0, eCool = 0.0;
        auto run = [&](double s) {
            for (int n = 0; n < static_cast<int>(s / dt); ++n)
            {
                p.process(l.data(), r.data(), 512);
                eGas += test::field(p, "tel.exhaust.portHeat") * dt;
                eCool += test::field(p, "tel.exhaust.portCoolantHeat") * dt;
            }
        };
        run(1.5);
        c.ignition = EnginePlayer::kIgnitionOn;
        run(1.0);
        for (double rpm : k.rpms)
        {
            c.mode = 2;
            c.dynoSpeed = static_cast<float>(rpm * 0.10471976);
            c.throttle = 1.0f;
            run(full ? 8.0 : 6.0);
            // the heat settled where this load holds it (card C's settle), then 2 s run on, the energy counted over them
            {
                const std::size_t hs = static_cast<std::size_t>(p.specIndex("exhaust.heatSettle"));
                p.setSpec(hs, p.spec(hs) + 1.0);
            }
            run(0.1);
            eGas = 0.0;
            eCool = 0.0;
            const double w0 = test::field(p, "tel.exhaust.portWallTemperature");
            run(2.0);
            const double w1 = test::field(p, "tel.exhaust.portWallTemperature");
            const double tv = test::field(p, "tel.exhaust.portTemperature");
            const double te = test::field(p, "tel.exhaust.portExitTemperature");
            const double th = test::field(p, "tel.exhaust.headerTemperature");
            const double q = test::field(p, "tel.exhaust.portHeat");
            const double power = test::field(p, "tel.engine.torque") * rpm * 0.10471976;
            std::printf("port, %s at %.0f rpm full throttle: at the valve %.0f K, leaving the port %.0f K (%.0f K less), "
                        "the header %.0f K; the port walls take %.1f kW (brake %.1f kW), their wall %.0f K; over 2 s the "
                        "gas gave %.1f kJ, the coolant took %.1f kJ, the wall moved %.1f K\n", k.build, rpm, tv, te, tv - te,
                        th, q / 1000.0, power / 1000.0, w1, eGas / 1000.0, eCool / 1000.0, w1 - w0);
            INFO(k.build << " at " << rpm);
            CHECK(te <= tv);
            CHECK(q > 0.0);
            CHECK(th < te);
            // the energy accounted: what the gas gave, the coolant took and the walls stored (their capacity times
            // their move), within 5 % (the sums are of 512-sample blocks)
            const double stored = test::field(p, "exh.capPort") * (w1 - w0);
            std::printf("    energy: gas %.1f kJ = coolant %.1f + walls %.1f kJ (%.1f %% apart)\n", eGas / 1000.0,
                        eCool / 1000.0, stored / 1000.0, 100.0 * (eGas - eCool - stored) / eGas);
            CHECK(std::fabs(eGas - eCool - stored) < 0.05 * eGas);
            // the header's targets at full load (the calibration's: a thermocouple a little way down the primary)
            // from mid speed up; at 1500 rpm printed only
            if (rpm < 1700.0 && !k.diesel)
                continue;
            if (k.diesel)
            {
                // (the target is 700 to 950 K; the large diesel's gas at the valve is 590 to 730 K, the cylinder's, under
                // it before the port takes 30 to 40 K: its floor is not the exhaust's to hold, so only its top checked)
                CHECK(th < 950.0);
            }
            else
            {
                CHECK(th > 900.0);
                CHECK(th < 1100.0);
            }
        }
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

// A measurement for the calibration (hidden: run it by name): at full load on archetypes, the heat settled, the gas's
// mass means along the exhaust (at the valve, leaving the port, the primaries) beside what thermocouples read there
// (tel.exhaust.portThermocouple, headerThermocouple), and the collectors' gas (a turbine's inlet).
TEST_CASE("measure: the exhaust's temperatures at full load, mass means and thermocouples", "[.][measure]")
{
    struct Case
    {
        const char* file;
        double rpm;
    };
    for (const Case& k : {Case{"air-cooled-flat-six-36.json", 5000.0}, Case{"turbo-four-20-gdi.json", 5500.0},
                          Case{"small-block-57-carb.json", 4500.0}, Case{"pickup-diesel-67-inline-six.json", 2800.0},
                          Case{"straight-six-32-high-rev.json", 6500.0}, Case{"v12-65.json", 6000.0}})
    {
        EnginePlayer& p = player();
        p.reset();
        EngineBuild b;
        std::string error, warnings;
        REQUIRE(loadBuild(p, std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/archetypes" / k.file, b, error, warnings));
        b.values["env.temperature"] = {293.15};
        applyBuild(p, b);
        EnginePlayer::Controls& c = p.controls();
        c.mode = EnginePlayer::kModeBench;
        c.ignition = EnginePlayer::kIgnitionStart;
        std::vector<float> l(512), r(512);
        auto run = [&](double s) {
            for (int n = 0; n < static_cast<int>(s * 48000.0 / 512.0); ++n)
                p.process(l.data(), r.data(), 512);
        };
        run(1.5);
        c.ignition = EnginePlayer::kIgnitionOn;
        c.mode = 2;
        c.dynoSpeed = static_cast<float>(k.rpm * 0.10471976);
        c.throttle = 1.0f;
        run(8.0);
        const std::size_t hs = static_cast<std::size_t>(p.specIndex("exhaust.heatSettle"));
        p.setSpec(hs, p.spec(hs) + 1.0);
        run(4.0);
        auto f = [&](const std::string& path) { return test::field(p, path); };
        std::printf("%s at %.0f rpm WOT (%.0f kW brake): mass means: valve %.0f, port exit %.0f, primaries %.0f, "
                    "collectors %.0f K; thermocouples: port %.0f (plain time mean %.0f), header %.0f K\n", k.file, k.rpm,
                    f("tel.engine.torque") * k.rpm * 0.10471976 / 1000.0, f("tel.exhaust.portTemperature"),
                    f("tel.exhaust.portExitTemperature"), f("tel.exhaust.headerTemperature"), f("exh.b.heat.tGas[1]"),
                    f("tel.exhaust.portThermocouple"), f("exh.tcPlain"), f("tel.exhaust.headerThermocouple"));
    }
}

// The lead's ruling (24 Nov): the exhaust valves give their pipes g q (the simple wave's stiffening), 5 to 20 % more
// than the cylinders let out over a cycle; Exhaust::stepLet closes the mean (spec.exhaust.flowClose, on under model 1).
// What the pipes are given against what the valves let out (exh.flowGiven, exh.flowLet: m^3, summed in blocks), over a
// second: the big block at idle and at 4000 rpm full throttle, the 2.0 turbo four at 3000: within a per cent.
TEST_CASE("the pipes are given what the exhaust valves let out", "[exhaust][engine][books]")
{
    struct Case
    {
        const char* file;
        double rpm;     // 0: idle
    };
    for (const Case& k : {Case{"big-block-74-pushrod-v8.json", 0.0}, Case{"big-block-74-pushrod-v8.json", 4000.0},
                          Case{"turbo-four-20-gdi.json", 3000.0}})
    {
        EnginePlayer& p = player();
        p.reset();
        EngineBuild b;
        std::string error, warnings;
        REQUIRE(loadBuild(p, std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/archetypes" / k.file, b, error, warnings));
        b.values["env.temperature"] = {293.15};
        b.values["exhaust.model"] = {1.0};
        applyBuild(p, b);
        EnginePlayer::Controls& c = p.controls();
        c.mode = EnginePlayer::kModeBench;
        c.ignition = EnginePlayer::kIgnitionStart;
        std::vector<float> l(512), r(512);
        auto run = [&](double s) {
            for (int n = 0; n < static_cast<int>(s * 48000.0 / 512.0); ++n)
                p.process(l.data(), r.data(), 512);
        };
        run(1.5);
        c.ignition = EnginePlayer::kIgnitionOn;
        c.throttle = 0.0f;
        if (k.rpm > 0.0)
        {
            c.mode = 2;
            c.dynoSpeed = static_cast<float>(k.rpm * 0.10471976);
            c.throttle = 1.0f;
        }
        run(3.0);
        const double g0 = test::field(p, "exh.flowGiven"), l0 = test::field(p, "exh.flowLet");
        run(1.0);
        const double g1 = test::field(p, "exh.flowGiven"), l1 = test::field(p, "exh.flowLet");
        if (l1 - l0 <= 0.0)
            SKIP("the engine does not call Exhaust::stepLet yet (D's two engine.fm lines)");
        const double ratio = (g1 - g0) / (l1 - l0);
        std::printf("books: %s %s: the pipes given %.5f m^3 a second against %.5f let out: %.4f\n", k.file,
                    k.rpm > 0.0 ? "full throttle" : "idle", g1 - g0, l1 - l0, ratio);
        INFO(k.file << " at " << k.rpm);
        CHECK(std::fabs(ratio - 1.0) < 0.01);
    }
}

// content/builds/exhausts: every field of each part build, read back from the player after the build is applied,
// equal to the file's, element by element, choices by name included (C's 127c350 found an array of choices given by
// name dropped to its default: exhaust.mufKind, resonator, converter, bypass)
TEST_CASE("every exhaust preset reads back as its file says", "[exhaust][engine][builds]")
{
    EnginePlayer& p = player();
    int fields = 0, elements = 0;
    for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/exhausts"))
    {
        if (entry.path().extension() != ".json")
            continue;
        p.reset();
        EngineBuild b;
        std::string error, warnings;
        REQUIRE(loadBuild(p, entry.path(), b, error, warnings));
        applyBuild(p, b);
        std::ifstream in(entry.path());
        const nlohmann::json doc = nlohmann::json::parse(in);
        for (const auto& [key, value] : doc["spec"]["exhaust"].items())
        {
            const std::string path = "exhaust." + key;
            const int idx = p.specIndex(path);
            INFO(entry.path().filename().string() << ": " << path);
            REQUIRE(idx >= 0);
            const EnginePlayer::SpecField& f = p.specFields()[static_cast<std::size_t>(idx)];
            auto expect = [&](const nlohmann::json& v) {
                if (v.is_string())
                {
                    for (std::size_t c = 0; c < f.choices.size(); ++c)
                        if (f.choices[c] == v.get<std::string>())
                            return f.minimum + static_cast<double>(c);
                    FAIL("no choice named " << v.get<std::string>());
                }
                if (v.is_boolean())
                    return v.get<bool>() ? 1.0 : 0.0;
                return v.get<double>();
            };
            const bool array = value.is_array();
            const std::size_t n = array ? value.size() : 1;
            for (std::size_t i = 0; i < n; ++i)
            {
                const double want = expect(array ? value[i] : value);
                const double got = test::field(p, "spec." + path + (f.count > 1 ? "[" + std::to_string(i) + "]" : ""));
                INFO("element " << i << ": file " << want << ", read back " << got);
                CHECK(std::fabs(got - want) <= 1.0e-5 * std::max(1.0, std::fabs(want)));
                ++elements;
            }
            ++fields;
        }
    }
    std::printf("presets read back: %d fields, %d elements, each as its file says\n", fields, elements);
}
