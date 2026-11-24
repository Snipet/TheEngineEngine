// tests/sim/combo/FuzzTest.cpp: package X's fuzzing inside the declared ranges. Random Specs (every field within its
// fminus::range, seeded: the seed reproduces it, tee-fuzz --seed N), each run for seconds of random pedals; then the
// same at the ranges' corners. Judged by combo::judgeFuzz: finite, no guard, no blast, not silent while running, not
// far over budget. A finding is reduced by tee-fuzz --reduce to the fields that matter.
#include "Combo.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>

using namespace combo;

namespace
{
    // checks: FMinus's bounds checks and loop budget on (a stop is reported); off in the nightly tier, as the
    // application ships, so that a crash there is a crash (01-rules.md section 4)
    void fuzz(std::uint64_t first, int count, double seconds, bool corners, bool checks)
    {
        REQUIRE(fuzzFields().size() > 50);
        for (std::uint64_t seed = first; seed < first + static_cast<std::uint64_t>(count); ++seed)
        {
            Times tm;
            const auto spec = randomSpec(seed, corners);
            tee::test::Scenario sc = fuzzScenario(spec, seed, seconds, tm);
            if (checks)
                sc.checks();
            const tee::test::Run r = sc.run(tm.end);
            const Verdict v = judgeFuzz(r, tm);
            const std::string name = std::string(corners ? "corners " : "random ") + std::to_string(seed);
            report(v, name, "fuzz", corners ? "corners" : "random");
        }
    }
} // namespace

TEST_CASE("fuzz: random Spec seed 2 (the one that once crashed) with six seconds of random driving", "[combo][fuzz]")
{
    fuzz(2, 1, 6.0, false, true);
}

TEST_CASE("fuzz: the ranges' corners seed 3 (the one that once overshot) with six seconds of random driving",
          "[combo][fuzz]")
{
    fuzz(3, 1, 6.0, true, true);
}

TEST_CASE("fuzz: random Specs inside the ranges with seeds 1 to 6 and six seconds of random driving each",
          "[nightly][fuzz]")
{
    fuzz(1, 6, 6.0, false, false);
}

TEST_CASE("fuzz: the ranges' corners with seeds 1 to 6 and six seconds of random driving each",
          "[nightly][fuzz]")
{
    fuzz(1, 6, 6.0, true, false);
}

TEST_CASE("nightly fuzz: seeds 100 to 299 random and at the corners for ten seconds each", "[nightly][fuzz-long]")
{
    fuzz(100, 200, 10.0, false, false);
    fuzz(100, 200, 10.0, true, false);
}
