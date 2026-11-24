// tests/sim/combo/FirstBlockTest.cpp: every telemetry field inside its range from the first block, on the default V8,
// every switch off and every switch on: X's own tel-range judge (Combo.cpp, judgeTelemetry), on half a second only.
// The lead's guard (2026-11-24): twice that day one new telemetry field without a sane first value (0 K at 0.01 s)
// turned 65 combo cases red on every platform for half an hour. scripts/lint.sh runs this case before a push (about
// 8 s: most of it compiling sim/), so a new field is caught where it is written. A known finding (Known.h) is
// reported, not failed, as in every combo case.
#include "Combo.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>

TEST_CASE("telemetry: every field inside its range from the first block, every switch off and every switch on",
          "[fast][fast-4][combo][telemetry]")
{
    const std::uint32_t all = combo::switches().size() >= 32
                                  ? 0xffffffffu
                                  : (1u << combo::switches().size()) - 1u;
    for (const std::uint32_t mask : {0u, all})
    {
        const std::string name = combo::comboName(mask);
        combo::Times times;
        tee::test::Scenario s = combo::scenario(combo::Kind::Tour, "v8", mask, times);
        const tee::test::Run run = s.run(0.5);
        REQUIRE(run.ok());
        combo::Verdict v;
        combo::judgeTelemetry(run, v);
        for (const combo::Failure& f : v.failures)
        {
            if (const combo::Known* k = combo::known(name, "v8", "first-block", f.check))
                WARN(name << ": " << f.check << ": " << f.detail << " (known: " << k->id << ", " << k->owner << ")");
            else
                FAIL_CHECK(name << ": " << f.check << ": " << f.detail);
        }
    }
}
