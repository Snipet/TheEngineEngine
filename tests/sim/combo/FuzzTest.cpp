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
    void fuzz(std::uint64_t first, int count, double seconds, bool corners)
    {
        REQUIRE(fuzzFields().size() > 50);
        for (std::uint64_t seed = first; seed < first + static_cast<std::uint64_t>(count); ++seed)
        {
            Times tm;
            const auto spec = randomSpec(seed, corners);
            // with FMinus's bounds checks and loop budget: an access out of bounds stops the call with its words
            // (and is reported) instead of taking the test process down
            const tee::test::Run r = fuzzScenario(spec, seed, seconds, tm).checks().run(tm.end);
            const Verdict v = judgeFuzz(r, tm);
            const std::string name = std::string(corners ? "corners " : "random ") + std::to_string(seed);
            report(v, name, "fuzz", corners ? "corners" : "random");
        }
    }
} // namespace

TEST_CASE("fuzz: random Spec seed 2 (the one that once crashed) with six seconds of random driving", "[combo][fuzz]")
{
    fuzz(2, 1, 6.0, false);
}

TEST_CASE("fuzz: the ranges' corners seed 3 (the one that once overshot) with six seconds of random driving",
          "[combo][fuzz]")
{
    fuzz(3, 1, 6.0, true);
}

TEST_CASE("fuzz: random Specs inside the ranges with seeds 1 to 6 and six seconds of random driving each",
          "[nightly][fuzz]")
{
    fuzz(1, 6, 6.0, false);
}

TEST_CASE("fuzz: the ranges' corners with seeds 1 to 6 and six seconds of random driving each",
          "[nightly][fuzz]")
{
    fuzz(1, 6, 6.0, true);
}

TEST_CASE("nightly fuzz: seeds 100 to 299 random and at the corners for ten seconds each", "[nightly][fuzz-long]")
{
    fuzz(100, 200, 10.0, false);
    fuzz(100, 200, 10.0, true);
}
