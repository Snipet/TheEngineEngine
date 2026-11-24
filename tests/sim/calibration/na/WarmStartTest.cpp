// tests/sim/calibration/na/WarmStartTest.cpp: card N1's hold on a warm start (the lead, 19:14 and 19:56): every
// archetype on card P's p-idle-warm (in the car in neutral, held warm, the key from 0.25 s to 2 s, then idle) is running
// at 14 s, at a speed; the quick ones also idle steadily. Played as tee-render plays it (the timeline's runner, the warm
// hold, 512-sample blocks). The quick tier: the two that stopped starting (a 3.6 L flat six and a 7.0 L V8: ab65e06's
// added correction drained the plenum through the flare; the ECU's proportional part shut the idle valve at the catch)
// and a 5.7 L carburetted V8 as the control. Steady: the speed's least to most over 8 to 14 s within 100 rpm (P's idle
// steadiness: a warm idle wanders some 20 to 50 rpm, a rule of thumb; twice the top of that is judgement).
#include "tee/Timeline.h"
#include "tee/Warm.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <filesystem>
#include <string>

using namespace tee;

namespace
{
    const std::filesystem::path kRoot = TEE_SOURCE_DIR;

    void warmStart(const std::string& slug, bool steady)
    {
        CAPTURE(slug);
        std::string error;
        // (never deleted: a compiled program destroyed after the JIT at exit crashes on macOS)
        static const std::shared_ptr<Sim>* const kept = new std::shared_ptr<Sim>(Sim::compile(kRoot / "sim", {}, error));
        const std::shared_ptr<Sim>& sim = *kept;
        INFO(error);
        REQUIRE(sim != nullptr);
        Timeline t;
        REQUIRE(loadTimeline(kRoot / "content" / "validation" / "scenarios" / "p-idle-warm.json", t, error));
        SimHost host;
        LoadOptions load;
        load.sampleRate = t.rate;
        load.maxBlock = t.block;
        load.worker = false;
        if (t.warm)
            load.hold = warmEngine();
        REQUIRE(host.attach(sim, load, error));
        Build build(sim->fields());
        std::vector<std::string> warnings;
        REQUIRE(build.loadFile(kRoot / "content" / "builds" / "archetypes" / (slug + ".json"), BuildPart::Whole, warnings,
                               error));
        build.attach(&host);
        RunOptions options;
        options.telemetry = {"engine.speed", "engine.state"};
        RunResult run;
        REQUIRE(runTimeline(host, build, t, options, run, error));
        const int cs = run.column("engine.speed"), cst = run.column("engine.state");
        REQUIRE(cs >= 0);
        REQUIRE(cst >= 0);
        REQUIRE(run.rowCount() > 0);
        double sum = 0.0, lo = 1e9, hi = 0.0;
        int n = 0;
        for (std::size_t r = 0; r < run.rowCount(); ++r)
        {
            const double rpm = run.at(r, std::size_t(cs)) * 9.5492966;
            if (run.rowTime[r] >= 13.0)
                sum += rpm, ++n;
            if (run.rowTime[r] >= 8.0)
                lo = std::min(lo, rpm), hi = std::max(hi, rpm);
        }
        const double rpm = n > 0 ? sum / n : 0.0;
        const int state = int(run.at(run.rowCount() - 1, std::size_t(cst)));
        INFO("state at 14 s " << state << ", mean speed over the last second " << rpm << " rpm, 8 to 14 s " << lo
                              << " to " << hi << " rpm");
        CHECK(state == 2);
        CHECK(rpm > 300.0);
        if (steady)
            CHECK(hi - lo <= 100.0);
    }
} // namespace

TEST_CASE("a warm start on p-idle-warm: the flat six that stopped starting and the 5.7 control run steadily at 14 s",
          "[calibration][na]")
{
    for (const char* slug : {"air-cooled-flat-six-36", "small-block-57-carb"})
        warmStart(slug, true);
}

// Marked (the lead, 19:56): the 7.0 starts (ca76cd6) but its idle lopes, 641 to 846 rpm over 8 to 14 s at c40f041 (sd 45;
// 8 at c8f2d54, 107 under b6676a5's ratio): the idle loop is lightly damped on this engine (causes.md L). Turns plain
// when that is fixed.
TEST_CASE("a warm start on p-idle-warm: the 7.0 runs steadily at 14 s", "[calibration][na][!mayfail]")
{
    warmStart("big-bore-70-pushrod-v8", true);
}

TEST_CASE("a warm start on p-idle-warm: every archetype runs at 14 s", "[nightly][calibration][na]")
{
    for (const auto& entry : std::filesystem::directory_iterator(kRoot / "content" / "builds" / "archetypes"))
        if (entry.is_regular_file() && entry.path().extension() == ".json")
            warmStart(entry.path().stem().string(), false);
}
