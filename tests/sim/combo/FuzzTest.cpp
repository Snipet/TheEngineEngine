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
            Verdict v = judgeFuzz(r, tm);
            // the contract's heavy case (02-contracts section 9: reported, not required in real time): a custom rotary of
            // two or more rotors (each face solved twice a sample on a shared port) or of twelve or more cylinders.
            // Its cost is X-016's, reported here, not failed (Q measured 2 to 10 R on such builds).
            double custom = 0.0, rotors = 0.0, cylinders = 0.0;
            for (const Value& x : spec)
            {
                if (x.path == "engine.custom")
                    custom = x.value;
                else if (x.path == "engine.rotors")
                    rotors = x.value;
                else if (x.path == "engine.cylinders")
                    cylinders = x.value;
            }
            if (custom != 0.0 && (rotors >= 2.0 || cylinders >= 12.0))
            {
                for (auto it = v.failures.begin(); it != v.failures.end();)
                {
                    if (it->check == "cost")
                    {
                        WARN("known X-016 (the heavy case): seed " << seed << ": " << it->detail << " (custom, "
                                                                 << rotors << " rotors, " << cylinders << " cylinders)");
                        it = v.failures.erase(it);
                    }
                    else
                        ++it;
                }
            }
            const std::string name = std::string(corners ? "corners " : "random ") + std::to_string(seed);
            report(v, name, "fuzz", corners ? "corners" : "random");
        }
    }
} // namespace

TEST_CASE("fuzz: random Spec seed 2 (the one that once crashed) with six seconds of random driving", "[fast][fast-1][combo][fuzz]")
{
    fuzz(2, 1, 6.0, false, true);
}

TEST_CASE("fuzz: the ranges' corners seed 3 (the one that once overshot) with six seconds of random driving",
          "[fast][fast-2][combo][fuzz]")
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
