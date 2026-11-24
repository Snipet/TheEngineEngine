// tests/sim/combo/ComboTest.cpp: package X's combinations. Each property has a quick form in the default run (under 30 s
// a case: 01-rules.md section 3) and its full form under the [nightly] tag (ctest --preset nightly; CI's soak tier):
//   - every archetype with nothing switched on and with everything: the tour (quick); every scenario (nightly);
//   - each switch alone on the V8's tour and the turbo six's pull (quick); with each switch after it (nightly);
//   - the archetype corpus: six builds, one of each class (quick); all thirty (nightly);
//   - every triple of switches (nightly).
// Each run is judged by combo::judge (Combo.cpp states every bound).
#include "Combo.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <vector>

using namespace combo;

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
        const Verdict v = judge(r, tm, archetype(build), true, mask == 0 ? v0(r, tm) : referenceStep(build, kind));
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

TEST_CASE("the switches the program has", "[combo]")
{
    const auto& s = switches();
    std::string list;
    for (const Switch& x : s)
        list += " " + x.path;
    std::printf("combo: %zu switches:%s\n", s.size(), list.c_str());
    // car.model (I) and engine.custom (D) were in at 14:15; the others' switches were added as hooks by D
    CHECK(s.size() >= 2);
}

#define ARCHETYPE_CASES(name)                                                                                          \
    TEST_CASE("archetype " name " through the tour with nothing switched on and with every switch on",                \
              "[combo][archetype]")                                                                                    \
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

ARCHETYPE_CASES("small-four")
ARCHETYPE_CASES("v8")
ARCHETYPE_CASES("turbo-six")
ARCHETYPE_CASES("large-diesel")
ARCHETYPE_CASES("turbo-v8-dual")

#define PAIRS_CASE(i)                                                                                                  \
    TEST_CASE("switch " #i " alone", "[combo][pairs]")                                                                  \
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

TEST_CASE("nightly: every triple of switches on every archetype through the tour and the pull", "[nightly][triples]")
{
    for (std::uint32_t m : subsetsOfSize(3))
        for (const Archetype& a : archetypes())
        {
            play(m, a.name, Kind::Tour);
            play(m, a.name, Kind::Pull);
        }
}

TEST_CASE("the scenario files of content/scenarios/combo are the ones the tests play", "[combo]")
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
    // One build of the corpus through the tour, judged against its own reference (every switch off).
    void corpusBuild(const std::filesystem::path& f)
    {
        const Archetype generic{"corpus", 36.6, 209.4, false};
        Times tm;
        tee::test::Scenario off = scenario(Kind::Tour, f.string(), 0u, tm);
        off.at(0.0);
        for (const Switch& w : switches())
            off.spec(w.path, 0);
        const tee::test::Run r0 = off.run(tm.end);
        const double from = tm.started + 0.5, to = tm.off;
        const double ref = std::max(r0.left().during(from, to).largestStep(), r0.right().during(from, to).largestStep());
        const tee::test::Run r = scenario(Kind::Tour, f.string(), 0u, tm).run(tm.end);
        const Verdict v = judge(r, tm, &generic, true, ref);
        report(v, "as built", f.stem().string(), "tour");
    }

    std::filesystem::path corpusDir()
    {
        return tee::test::contentDirectory() / "builds" / "archetypes";
    }
} // namespace

// the content session's thirty archetypes, each with every model on as the build says: a plausible idle (here 350 to
// 2000 rpm, bikes and diesels among them), revs, stops, and the judge's every other measure; the click bound twice the
// build's own reference (a big diesel's own blip steps 18, the V8's 12)
#define CORPUS_CASE(stem)                                                                                              \
    TEST_CASE("the archetype corpus: " stem " through the tour", "[combo][corpus]")                                   \
    {                                                                                                                  \
        const auto f = corpusDir() / (std::string(stem) + ".json");                                                    \
        if (!std::filesystem::exists(f))                                                                               \
            SKIP("no " << f.string());                                                                                 \
        corpusBuild(f);                                                                                                \
    }
// the quick form: one of each class (a small four, a pushrod V8, a turbo V8, a diesel, a rotary, a V12)
CORPUS_CASE("economy-four-15")
CORPUS_CASE("small-block-57-carb")
CORPUS_CASE("twin-turbo-hot-vee-v8-40")
CORPUS_CASE("diesel-20-common-rail")
CORPUS_CASE("rotary-twin-13")
CORPUS_CASE("v12-65")

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
