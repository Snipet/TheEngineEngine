// tests/sim/combo/ComboTest.cpp: package X's combinations. Each property has a quick form in the default run (under 30 s
// a case: 01-rules.md section 3) and its full form under the [nightly] tag (ctest --preset nightly; CI's soak tier):
//   - the V8 with nothing switched on and with everything through the tour (fast); every archetype, every scenario
//     (nightly);
//   - a handful of pairs where packages meet (fast); each switch alone, every pair, every triple (nightly);
//   - the archetype corpus: the V12 (fast); five more of other classes and all thirty (nightly);
// The fast tier's budget is 600 CPU-s on CI's runner (01-rules.md section 3).
//   - every triple of switches (nightly).
// Each run is judged by combo::judge (Combo.cpp states every bound).
#include "Combo.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <sstream>
#include <string>
#include <vector>

using namespace combo;


#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <catch2/catch_test_case_info.hpp>

namespace
{
    // Marks each case's tier at its start, for the nightly bound (combo::referenceExcess)
    class TierListener : public Catch::EventListenerBase
    {
    public:
        using Catch::EventListenerBase::EventListenerBase;
        void testCaseStarting(const Catch::TestCaseInfo& info) override
        {
            bool nightly = false;
            for (const auto& tag : info.tags)
                if (tag.original == Catch::StringRef("nightly"))
                    nightly = true;
            combo::setNightlyCase(nightly);
        }
    };
} // namespace

CATCH_REGISTER_LISTENER(TierListener)

namespace
{
    // the reference judged against itself: its own step is its bound
    double v0(const tee::test::Run& r, const Times& tm)
    {
        const double from = tm.started + 0.5, to = tm.off > 0.0 ? tm.off : tm.end;
        return std::max(r.left().during(from, to).largestStep(), r.right().during(from, to).largestStep());
    }

    void play(std::uint32_t mask, const std::string& build, Kind kind)
    {
        Times tm;
        const tee::test::Run r = scenario(kind, build, mask, tm).run(tm.end);
        const double from = tm.started + 0.5, to = tm.off > 0.0 ? tm.off : tm.end;
        const Verdict v = judge(r, tm, archetype(build), true, mask == 0 ? v0(r, tm) : referenceStep(build, kind),
                                mask == 0 ? stepExcess(r, from, to) : referenceExcess(build, kind));
        report(v, comboName(mask), build, kindName(kind));
    }

    std::uint32_t allMask()
    {
        const std::size_t n = switches().size();
        return n >= 32 ? 0xffffffffu : (1u << n) - 1u;
    }

    void archetypeCase(const char* build, bool all, bool every)
    {
        REQUIRE(!switches().empty());
        for (Kind k : every ? allKinds() : std::vector<Kind>{Kind::Tour})
        {
            INFO(build << " / " << kindName(k));
            play(all ? allMask() : 0u, build, k);
        }
    }

    // Switch i alone, then i with every j after it: the V8 through the tour, and the turbo six through the pull.
    void pairsCase(std::size_t i, bool pairs)
    {
        const auto& s = switches();
        if (i >= s.size())
            SKIP("the program has " << s.size() << " switches");
        const std::uint32_t one = 1u << i;
        if (!pairs)
        {
            play(one, "v8", Kind::Tour);
            play(one, "turbo-six", Kind::Pull);
            return;
        }
        for (std::size_t j = i + 1; j < s.size(); ++j)
        {
            play(one | (1u << j), "v8", Kind::Tour);
            play(one | (1u << j), "turbo-six", Kind::Pull);
        }
    }
} // namespace

TEST_CASE("the switches the program has", "[fast][fast-1][combo]")
{
    const auto& s = switches();
    std::string list;
    for (const Switch& x : s)
        list += " " + x.path;
    std::printf("combo: %zu switches:%s\n", s.size(), list.c_str());
    // car.model (I) and engine.custom (D) were in at 14:15; the others' switches were added as hooks by D
    CHECK(s.size() >= 2);
}

#define ARCHETYPE_CASES(name, tier)                                                                                    \
    TEST_CASE("archetype " name " through the tour with nothing switched on and with every switch on",                \
              tier "[archetype]")                                                                                    \
    {                                                                                                                  \
        archetypeCase(name, false, false);                                                                             \
        archetypeCase(name, true, false);                                                                              \
    }                                                                                                                  \
    TEST_CASE("archetype " name " with nothing switched on through every scenario", "[nightly][archetype]")           \
    {                                                                                                                  \
        archetypeCase(name, false, true);                                                                              \
    }                                                                                                                  \
    TEST_CASE("archetype " name " with every switch on through every scenario", "[nightly][archetype]")               \
    {                                                                                                                  \
        archetypeCase(name, true, true);                                                                               \
    }

ARCHETYPE_CASES("small-four", "[nightly]")
ARCHETYPE_CASES("v8", "[fast][fast-2][combo]")
ARCHETYPE_CASES("turbo-six", "[nightly]")
ARCHETYPE_CASES("large-diesel", "[nightly]")
ARCHETYPE_CASES("turbo-v8-dual", "[nightly]")

#define PAIRS_CASE(i)                                                                                                  \
    TEST_CASE("switch " #i " alone", "[nightly][pairs]")                                                                  \
    {                                                                                                                  \
        pairsCase(i, false);                                                                                           \
    }                                                                                                                  \
    TEST_CASE("switch " #i " with each switch after it", "[nightly][pairs]")                                          \
    {                                                                                                                  \
        pairsCase(i, true);                                                                                            \
    }
PAIRS_CASE(0)
PAIRS_CASE(1)
PAIRS_CASE(2)
PAIRS_CASE(3)
PAIRS_CASE(4)
PAIRS_CASE(5)
PAIRS_CASE(6)
PAIRS_CASE(7)
PAIRS_CASE(8)
PAIRS_CASE(9)
PAIRS_CASE(10)
PAIRS_CASE(11)
PAIRS_CASE(12)
PAIRS_CASE(13)
PAIRS_CASE(14)

// Every triple of switches on every archetype through the tour and the pull: about 2200 runs (5051 s as one ctest
// entry, 84 of the soak's 92 minutes on CI's 4-core Linux runner). Sixteen shards, each its own ctest entry so the
// runner's cores share them: shard k takes the triples whose index in subsetsOfSize(3) is k mod 16, so together they
// cover exactly the triples the one case did.
namespace
{
    constexpr std::size_t kTripleShards = 16;
    void triples(std::size_t shard)
    {
        const auto all = subsetsOfSize(3);
        REQUIRE(all.size() >= kTripleShards);
        for (std::size_t i = shard; i < all.size(); i += kTripleShards)
            for (const Archetype& a : archetypes())
            {
                play(all[i], a.name, Kind::Tour);
                play(all[i], a.name, Kind::Pull);
            }
    }
} // namespace

#define TRIPLES_SHARD(k)                                                                                               \
    TEST_CASE("nightly: every triple of switches on every archetype through the tour and the pull: shard " #k          \
              " of 16",                                                                                                \
              "[nightly][triples]")                                                                                    \
    {                                                                                                                  \
        triples(k - 1);                                                                                                \
    }
TRIPLES_SHARD(1)
TRIPLES_SHARD(2)
TRIPLES_SHARD(3)
TRIPLES_SHARD(4)
TRIPLES_SHARD(5)
TRIPLES_SHARD(6)
TRIPLES_SHARD(7)
TRIPLES_SHARD(8)
TRIPLES_SHARD(9)
TRIPLES_SHARD(10)
TRIPLES_SHARD(11)
TRIPLES_SHARD(12)
TRIPLES_SHARD(13)
TRIPLES_SHARD(14)
TRIPLES_SHARD(15)
TRIPLES_SHARD(16)

TEST_CASE("the scenario files of content/scenarios/combo are the ones the tests play", "[fast][fast-1][combo]")
{
    for (Kind k : allKinds())
    {
        const auto path = scenarioDirectory() / (std::string(kindName(k)) + ".json");
        std::FILE* f = std::fopen(path.string().c_str(), "rb");
        std::string text;
        if (f != nullptr)
        {
            char b[4096];
            std::size_t n;
            while ((n = std::fread(b, 1, sizeof b, f)) > 0)
                text.append(b, n);
            std::fclose(f);
        }
        INFO(path.string() << ": regenerate with tee-fuzz --write-scenarios");
        CHECK(text == scenarioJson(k, 0));
    }
}

namespace
{
    // One build of the corpus as built through a scenario (the tour unless named), judged against its own reference
    // (every switch off).
    void corpusBuild(const std::filesystem::path& f, Kind kind = Kind::Tour)
    {
        const Archetype generic{"corpus", 36.6, 209.4, false};
        Times tm;
        double ref = 0.0, from = 0.0, to = 0.0;
        // (nightly: the reference's lowest excess over its copies at kBoundNudges, as referenceExcess; the default
        // tier: the 293.15 K copy alone)
        std::vector<double> excess;
        // (kept across the job's processes as reference() keeps its own: a corpus build with every switch off is its
        // own key, the build's file named in it and its bytes in the content tree's hash)
        const std::string kept = referenceKey("corpus, every switch off: " + f.generic_string(), kind, nightlyCase(),
                                             leverDrives(f.string()));
        const bool fromFile = readKeptReference(kept, ref, excess);
        {
            Times t0;
            (void)scenario(kind, f.string(), 0u, t0);
            from = t0.started + 0.5;
            to = t0.off > 0.0 ? t0.off : t0.end;
        }
        for (double dT : kBoundNudges)
        {
            if (fromFile)
                break;
            tee::test::Scenario off = scenario(kind, f.string(), 0u, tm);
            off.at(0.0);
            for (const Switch& w : switches())
                off.spec(w.path, 0);
            if (dT != 0.0)
                off.spec("env.temperature", 293.15 + dT);
            const tee::test::Run r0 = off.run(tm.end);
            from = tm.started + 0.5;
            to = tm.off > 0.0 ? tm.off : tm.end;
            if (dT == 0.0)
                ref = std::max(r0.left().during(from, to).largestStep(), r0.right().during(from, to).largestStep());
            excess.push_back(stepExcess(r0, from, to));
            if (!nightlyCase())
                break;
        }
        if (!fromFile)
            keepReference(kept, ref, excess);
        if (nightlyCase())
        {
            std::printf("reference | %-28s | %s | excess over its copies%s:", f.stem().string().c_str(), kindName(kind),
                        fromFile ? " (kept)" : "");
            for (double e : excess)
                std::printf(" %.2f", e);
            std::printf("; the bound twice the lowest, %.2f\n", 2.0 * lowestExcess(excess));
        }
        const tee::test::Run r = scenario(kind, f.string(), 0u, tm).run(tm.end);
        const Verdict v = judge(r, tm, &generic, true, ref, lowestExcess(excess));
        report(v, "as built", f.stem().string(), kindName(kind));
    }

    std::filesystem::path corpusDir()
    {
        return tee::test::contentDirectory() / "builds" / "archetypes";
    }
} // namespace

// the content session's thirty archetypes, each with every model on as the build says: a plausible idle (here 350 to
// 2000 rpm, bikes and diesels among them), revs, stops, and the judge's every other measure; the click bound twice the
// build's own reference (a big diesel's own blip steps 18, the V8's 12)
#define CORPUS_CASE(stem, tier)                                                                                        \
    TEST_CASE("the archetype corpus: " stem " through the tour", tier "[corpus]")                                   \
    {                                                                                                                  \
        const auto f = corpusDir() / (std::string(stem) + ".json");                                                    \
        if (!std::filesystem::exists(f))                                                                               \
            SKIP("no " << f.string());                                                                                 \
        corpusBuild(f);                                                                                                \
    }
// one of each class (a small four, a pushrod V8, a turbo V8, a diesel, a rotary, a V12); the V12 in the fast tier
CORPUS_CASE("economy-four-15", "[nightly]")
CORPUS_CASE("small-block-57-carb", "[nightly]")
CORPUS_CASE("twin-turbo-hot-vee-v8-40", "[nightly]")
CORPUS_CASE("diesel-20-common-rail", "[nightly]")
CORPUS_CASE("rotary-twin-13", "[nightly]")
CORPUS_CASE("v12-65", "[nightly][combo]")

TEST_CASE("the archetype corpus: every build of content/builds/archetypes through the tour", "[nightly][corpus]")
{
    if (!std::filesystem::is_directory(corpusDir()))
        SKIP("no content/builds/archetypes yet");
    std::vector<std::filesystem::path> files;
    for (const auto& e : std::filesystem::directory_iterator(corpusDir()))
        if (e.is_regular_file() && e.path().extension() == ".json")
            files.push_back(e.path());
    std::sort(files.begin(), files.end());
    for (const auto& f : files)
        corpusBuild(f);
}

// Shipped builds on the pull (the lead's ruling, 26 Nov 15:00: builds an owner drives, read per event on the pull, the
// upshifts over where the tour read clean): three of the corpus as built, each against its own every-switch-off
// reference, judged per event. The three-cylinder and the kei carry X-048's rows; the boxer is the build that reads
// clean. The whole corpus on the pull is a proposal in docs/handoffs/x-combo.md, not a case.
// The fourth (the lead's ruling, 26 Nov 18:11): the flat-plane twin-turbo V8, whose box is a dual clutch (no shift cut:
// item 16 does not reach it), over at its limiter on the pull (X-055); the corpus nightly plays only the tour.
TEST_CASE("shipped builds on the pull: the kei 660 then the three-cylinder 1.0 then the boxer four then the flat-plane V8",
          "[nightly][corpus]")
{
    for (const char* stem :
         {"kei-660-triple-turbo", "three-cylinder-10-turbo", "boxer-four-turbo-unequal", "flat-plane-v8-39-twin-turbo"})
    {
        const auto f = corpusDir() / (std::string(stem) + ".json");
        if (!std::filesystem::exists(f))
        {
            WARN("no " << f.string());
            continue;
        }
        corpusBuild(f, Kind::Pull);
    }
}

TEST_CASE("a handful of pairs where packages meet: fuel and ECU then boost and exhaust then ECU and car", "[nightly][combo][pairs]")
{
    // the fast tier's pairs (every pair is nightly): those whose meeting has produced findings today (X-003, X-006,
    // X-014, X-015, X-011, X-018)
    const auto& sw = switches();
    auto bit = [&](const char* path) {
        for (std::size_t i = 0; i < sw.size(); ++i)
            if (sw[i].path == path)
                return 1u << i;
        return 0u;
    };
    play(bit("fuel.model") | bit("ecu.model"), "v8", Kind::Tour);
    play(bit("boost.model") | bit("exhaust.model"), "turbo-six", Kind::Pull);
    {
        // the new ECU cuts on a shift and the cut's fuel bangs in the pipe: the bang rule (half of it: judge() doubles)
        // The cut's bang depends on where it ends among the firings: the worst of six phases, the upshift moved 0 to
        // 5 ms (the steward's ask; a single draw read 31.7 against the ruled 42)
        const std::uint32_t m = bit("ecu.model") | bit("car.model");
        const double ref = 0.5 * referenceStepWithPops("v8", Kind::Launch);
        const double refExcess = referenceExcess("v8", Kind::Launch);
        Verdict v;
        for (int k = 0; k < 6; ++k)
        {
            Times tm;
            const tee::test::Run r = scenario(Kind::Launch, "v8", m, tm, 0.001 * k).run(tm.end);
            const Verdict one = judge(r, tm, archetype("v8"), true, ref, refExcess);
            std::printf("pairs: ecu.model+car.model / v8 / launch, the upshift %d ms later: largest step %.1f\n", k,
                        one.stepMax);
            if (k == 0 || one.stepMax > v.stepMax)
                v = one;
        }
        report(v, comboName(m), "v8", kindName(Kind::Launch));
    }
}

// The nightly bound's proof, on demand (hidden): one case judged as a [nightly] case is, against the reference's lowest
// excess over its copies. BOUNDPROOF="<archetype> <tour|pull|launch|cold> <switches>" or "<corpus stem>" (the corpus's
// own route) or "corpus <stem> <scenario>" (a corpus build as built through any scenario); BOUNDPROOF_SET and
// BOUNDPROOF_REF with BOUNDPROOF_DT: below; with TEE_SIM_DIR, on another tree (X-014 and X-015 on 62a6a7fd: the lead's condition, 26 Nov 08:07)
TEST_CASE("the nightly bound's proof: one case judged against its reference's lowest copy", "[.boundproof][nightly]")
{
    const char* spec = std::getenv("BOUNDPROOF");
    REQUIRE(spec != nullptr);
    std::istringstream in(spec);
    std::string build, kindArg, comboArg;
    in >> build >> kindArg >> comboArg;
    if (kindArg.empty())
    {
        corpusBuild(corpusDir() / (build + ".json"));
        return;
    }
    // "corpus <stem> <scenario>": a corpus build as built through any scenario, against its every-switch-off
    // reference's lowest excess over its copies (as corpusBuild judges the tour), per event at each BOUNDPROOF_DT copy
    if (build == "corpus")
    {
        const std::string file = (corpusDir() / (kindArg + ".json")).string();
        Kind kind = Kind::Tour;
        for (Kind k : allKinds())
            if (comboArg == kindName(k))
                kind = k;
        std::vector<double> excess;
        double ref = 0.0;
        for (double dT : kBoundNudges)
        {
            Times tm;
            tee::test::Scenario off = scenario(kind, file, 0u, tm);
            off.at(0.0);
            // BOUNDPROOF_OFF="boost.model exhaust.model": the reference turns only those off (X-014's own reference, the
            // build with boost and exhaust off); unset, every switch
            if (const char* only = std::getenv("BOUNDPROOF_OFF"); only != nullptr)
            {
                std::istringstream names(only);
                std::string name;
                while (names >> name)
                    off.spec(name, 0);
            }
            else
                for (const Switch& w : switches())
                    off.spec(w.path, 0);
            off.spec("env.temperature", 293.15 + dT);
            const tee::test::Run r0 = off.run(tm.end);
            const double from = tm.started + 0.5, to = tm.off > 0.0 ? tm.off : tm.end;
            if (dT == 0.0)
                ref = std::max(r0.left().during(from, to).largestStep(), r0.right().during(from, to).largestStep());
            excess.push_back(stepExcess(r0, from, to));
        }
        std::printf("reference | %s | %s | excess over its copies:", kindArg.c_str(), kindName(kind));
        for (double e : excess)
            std::printf(" %.2f", e);
        std::printf("; the bound twice the lowest, %.2f\n", 2.0 * lowestExcess(excess));
        const Archetype generic{"corpus", 36.6, 209.4, false};
        const char* dts = std::getenv("BOUNDPROOF_DT");
        std::istringstream list(dts != nullptr ? dts : "0");
        double dT = 0.0;
        while (list >> dT)
        {
            Times tm;
            tee::test::Scenario sc = scenario(kind, file, 0u, tm);
            sc.at(0.0).spec("env.temperature", 293.15 + dT);
            const tee::test::Run r = sc.run(tm.end);
            const Verdict v = judge(r, tm, &generic, true, ref, lowestExcess(excess));
            for (const auto& w : v.windows)
                std::printf("window | %+.2f | %s | %.2f | %.3f | %s\n", dT, w.event.c_str(), w.excess, w.at,
                            v.excessBound >= 0.0 && w.excess > v.excessBound ? "over" : "under");
        }
        return;
    }
    Kind kind = Kind::Tour;
    for (Kind k : allKinds())
        if (kindArg == kindName(k))
            kind = k;
    std::uint32_t mask = 0;
    const auto& sw = switches();
    std::string rest = comboArg;
    while (!rest.empty() && rest != "none")
    {
        const auto plus = rest.find('+');
        const std::string one = rest.substr(0, plus);
        rest = plus == std::string::npos ? std::string() : rest.substr(plus + 1);
        for (std::size_t k = 0; k < sw.size(); ++k)
            if (sw[k].path == one)
                mask |= 1u << k;
    }
    // BOUNDPROOF_DT="dT dT ...": a spread, every window's excess at each copy (the reference's copies played once);
    // each line "window | dT | event | excess | at | over" against the nightly bound
    // BOUNDPROOF_SET="path=value ...": Spec values (under spec) set on the case AND on its reference (part C: the reference
    // off its limiter); the reference is then played here at kBoundNudges with the same values, and at each copy its own
    // windows are printed too ("refwindow | dT | event | excess | at"): each side's excess per event. BOUNDPROOF_REF=1:
    // the same with no values set (the reference's own windows beside the case's)
    std::vector<std::pair<std::string, double>> sets;
    const bool withRef = std::getenv("BOUNDPROOF_SET") != nullptr || std::getenv("BOUNDPROOF_REF") != nullptr;
    if (const char* st = std::getenv("BOUNDPROOF_SET"); st != nullptr)
    {
        std::istringstream in2(st);
        std::string kv;
        while (in2 >> kv)
            if (const auto eq = kv.find('='); eq != std::string::npos)
                sets.push_back({kv.substr(0, eq), std::stod(kv.substr(eq + 1))});
    }
    if (const char* dts = std::getenv("BOUNDPROOF_DT"); dts != nullptr)
    {
        double refStep = referenceStep(build, kind), refExcess = referenceExcess(build, kind);
        auto playRef = [&](double dT, Times& tm) {
            tee::test::Scenario off = scenario(kind, build, 0u, tm);
            off.at(0.0).spec("env.temperature", 293.15 + dT);
            for (const auto& [path, value] : sets)
                off.spec(path, value);
            return off.run(tm.end);
        };
        if (withRef)
        {
            std::vector<double> excess;
            for (double dT : kBoundNudges)
            {
                Times tm;
                const tee::test::Run r0 = playRef(dT, tm);
                const double from = tm.started + 0.5, to = tm.off > 0.0 ? tm.off : tm.end;
                if (dT == 0.0)
                    refStep = std::max(r0.left().during(from, to).largestStep(), r0.right().during(from, to).largestStep());
                excess.push_back(stepExcess(r0, from, to));
            }
            refExcess = lowestExcess(excess);
            std::printf("reference | %s | %s | with BOUNDPROOF_SET | excess over its copies:", build.c_str(), kindName(kind));
            for (double e : excess)
                std::printf(" %.2f", e);
            std::printf("; the bound twice the lowest, %.2f\n", 2.0 * refExcess);
        }
        std::istringstream list(dts);
        double dT = 0.0;
        while (list >> dT)
        {
            Times tm;
            tee::test::Scenario sc = scenario(kind, build, mask, tm);
            sc.at(0.0).spec("env.temperature", 293.15 + dT);
            for (const auto& [path, value] : sets)
                sc.spec(path, value);
            const tee::test::Run r = sc.run(tm.end);
            const Verdict v = judge(r, tm, archetype(build), true, refStep, refExcess);
            for (const auto& w : v.windows)
                std::printf("window | %+.2f | %s | %.2f | %.3f | %s\n", dT, w.event.c_str(), w.excess, w.at,
                            v.excessBound >= 0.0 && w.excess > v.excessBound ? "over" : "under");
            if (withRef)
            {
                Times t0;
                const tee::test::Run r0 = playRef(dT, t0);
                const Verdict v0 = judge(r0, t0, archetype(build), true, refStep, -1.0);
                for (const auto& w : v0.windows)
                    std::printf("refwindow | %+.2f | %s | %.2f | %.3f\n", dT, w.event.c_str(), w.excess, w.at);
            }
        }
        return;
    }
    Times tm;
    const tee::test::Run r = scenario(kind, build, mask, tm).run(tm.end);
    const Verdict v = judge(r, tm, archetype(build), true, referenceStep(build, kind), referenceExcess(build, kind));
    std::printf("boundproof | %s / %s / %s: excess %.2f at %.3f s against %.2f\n", comboName(mask).c_str(), build.c_str(),
                kindName(kind), v.stepExcess, v.stepAt, v.excessBound);
    report(v, comboName(mask), build, kindName(kind));
}

// GEARPROBE="<corpus stem> <tour|pull|launch|cold>": a corpus build as built through a scenario, unjudged; one line per
// change of car.gear ("gear | t | from | to") and a line a second ("state | t | gear | engine rpm | km/h"), so that which
// gear a scenario really drives in is read, not assumed. GEARPROBE_LEVER="t position ...": the lever moved at those times
// (on top of the scenario's own controls).
TEST_CASE("gear probe: car.gear through a scenario", "[.gearprobe]")
{
    const char* arg = std::getenv("GEARPROBE");
    if (arg == nullptr)
        SKIP("GEARPROBE not set");
    std::istringstream in(arg);
    std::string stem, kindArg;
    in >> stem >> kindArg;
    Kind kind = Kind::Pull;
    for (Kind k : allKinds())
        if (kindArg == kindName(k))
            kind = k;
    Times tm;
    tee::test::Scenario sc = scenario(kind, (corpusDir() / (stem + ".json")).string(), 0u, tm);
    if (const char* lv = std::getenv("GEARPROBE_LEVER"))
    {
        std::istringstream l(lv);
        double t = 0.0;
        int pos = 0;
        while (l >> t >> pos)
            sc.at(t).lever(pos);
    }
    const tee::test::Run r = sc.run(tm.end);
    const auto gear = r.tel("car.gear");
    const auto rpm = r.tel("engine.speed");
    const auto kmh = r.tel("car.speed");
    const auto& t = gear.times();
    const auto& x = gear.values();
    std::printf("gearprobe | %s | %s\n", stem.c_str(), kindName(kind));
    for (std::size_t i = 1; i < t.size(); ++i)
        if (x[i] != x[i - 1])
            std::printf("gear | %.3f | %.0f | %.0f\n", t[i], x[i - 1], x[i]);
    for (double s = 1.0; s <= tm.end; s += 1.0)
        std::printf("state | %.1f | %.0f | %.0f | %.1f\n", s, gear.at(s), rpm.at(s), kmh.at(s));
}
