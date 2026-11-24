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
            const tee::test::Run r = fuzzScenario(spec, seed, seconds, tm).run(tm.end);
            const Verdict v = judgeFuzz(r, tm);
            const std::string name = std::string(corners ? "corners " : "random ") + std::to_string(seed);
            report(v, name, "fuzz", corners ? "corners" : "random");
        }
    }
} // namespace

TEST_CASE("fuzz: random Specs inside the ranges with seeds 1 to 6 and six seconds of random driving each", "[combo][fuzz]")
{
    fuzz(1, 6, 6.0, false);
}

TEST_CASE("fuzz: the ranges' corners with seeds 1 to 6 and six seconds of random driving each", "[combo][fuzz]")
{
    fuzz(1, 6, 6.0, true);
}

TEST_CASE("nightly fuzz: seeds 100 to 299 random and at the corners for ten seconds each", "[.][nightly][fuzz-long]")
{
    if (std::getenv("TEE_NIGHTLY") == nullptr)
        SKIP("set TEE_NIGHTLY to run");
    fuzz(100, 200, 10.0, false);
    fuzz(100, 200, 10.0, true);
}
