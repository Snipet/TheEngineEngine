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
#include <cmath>
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

// (marked [!mayfail] until 14890c0 and 3930eadb: its idle loped 640 to 860 rpm, the spark trim's look-ahead short of its
// manifold's filling time; causes.md L)
TEST_CASE("a warm start on p-idle-warm: the 7.0 runs steadily at 14 s", "[calibration][na]")
{
    warmStart("big-bore-70-pushrod-v8", true);
}

TEST_CASE("a warm start on p-idle-warm: every archetype runs at 14 s", "[nightly][calibration][na]")
{
    for (const auto& entry : std::filesystem::directory_iterator(kRoot / "content" / "builds" / "archetypes"))
        if (entry.is_regular_file() && entry.path().extension() == ".json")
            warmStart(entry.path().stem().string(), false);
}

// The turbo six's idle on the bench with the ECU, boost and intake models (the lead, 2026-11-25): its cam (0.9999, the
// wildest) lopes, the dilution recursion's period near 0.4 s, sd 113 rpm on the reference alone. The ECU's idle valve read
// that lope's swing as far off the target and doubled its gain on air that arrives half a lope late: sd 151 rpm, the valve
// 0.04 to 0.52. With the gain read over 0.5 s (idle.fm), 127. Held under 140: the fix, with a tenth's room, short of the
// 151 before. The speed every block (512 samples at 48 kHz) over 5 to 9 s (old 155, new 127). Nightly for now: its own
// process compiles the program (some 6 of its 7 s) and the fast tier is over its budget (the steward, 00:19); it moves to
// the fast tier when calibration-na's fast cases share one process
TEST_CASE("the turbo six's lope on the bench is not fed by the idle valve under the models", "[nightly][calibration][na]")
{
    std::string error;
    static const std::shared_ptr<Sim>* const kept = new std::shared_ptr<Sim>(Sim::compile(kRoot / "sim", {}, error));
    const std::shared_ptr<Sim>& sim = *kept;
    INFO(error);
    REQUIRE(sim != nullptr);
    Timeline t;
    t.rate = 48000.0;
    t.block = 512;
    t.seconds = 9.0;
    const auto at = [&](double time, const char* name, double value) {
        t.events.push_back({time, TimelineEvent::Kind::Control, name, value, {}, 0.0});
    };
    at(0.0, "mode", 0);
    at(0.0, "throttle", 0);
    at(0.0, "ignition", 1);
    at(0.2, "ignition", 2);
    at(2.0, "ignition", 1);
    t.sort();
    SimHost host;
    LoadOptions load;
    load.sampleRate = t.rate;
    load.maxBlock = t.block;
    load.worker = false;
    REQUIRE(host.attach(sim, load, error));
    Build build(sim->fields());
    std::vector<std::string> warnings;
    REQUIRE(build.loadFile(kRoot / "content" / "builds" / "combo" / "turbo-six.json", BuildPart::Whole, warnings, error));
    for (const char* f : {"ecu.model", "boost.model", "intake.model"})
        REQUIRE(build.set(f, 1.0));
    build.attach(&host);
    RunOptions options;
    options.telemetry = {"engine.speed"};
    RunResult run;
    REQUIRE(runTimeline(host, build, t, options, run, error));
    const int cs = run.column("engine.speed");
    REQUIRE(cs >= 0);
    double s = 0.0, q = 0.0;
    int n = 0;
    for (std::size_t r = 0; r < run.rowCount(); ++r)
        if (run.rowTime[r] >= 5.0)
        {
            const double rpm = run.at(r, std::size_t(cs)) * 9.5492966;
            s += rpm, q += rpm * rpm, ++n;
        }
    REQUIRE(n > 100);
    const double mean = s / n, sd = std::sqrt(std::max(q / n - mean * mean, 0.0));
    INFO("mean " << mean << " rpm, sd " << sd << " rpm");
    CHECK(mean > 1000.0);
    CHECK(sd < 140.0);
}
