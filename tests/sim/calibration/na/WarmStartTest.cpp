// tests/sim/calibration/na/WarmStartTest.cpp: card N1's hold on a warm start (the lead, 19:14 and 19:56): every
// archetype on card P's p-idle-warm (in the car in neutral, held warm, the key from 0.25 s to 2 s, then idle) is running
// at 14 s, at a speed; the quick ones also idle steadily. Played as tee-render plays it (the timeline's runner, the warm
// hold, 512-sample blocks). The quick tier: the two that stopped starting (a 3.6 L flat six and a 7.0 L V8: ab65e06's
// added correction drained the plenum through the flare; the ECU's proportional part shut the idle valve at the catch)
// and a 5.7 L carburetted V8 as the control. Steady: the speed's least to most over 8 to 14 s within 100 rpm (P's idle
// steadiness: a warm idle wanders some 20 to 50 rpm, a rule of thumb; twice the top of that is judgement).
//
// The start's dip (the steward and the lead, 26 Nov 05:00): the lowest speed over 2 to 8 s, between the key's release
// and the idle the steady check reads, no more than 25 rpm under main's own (half the top of that same 20 to 50 rpm
// wander: judgement). An idle change read only on the settled idle deepened it unseen (the reverted 98d0dc4d: the 7.4
// 545 -> 457 rpm against 650 asked, the V12 879 -> 827, the twin six 639 -> 579, the flat six 822 -> 792). main's
// figures below are its own, read here on main d586682c (sim f23337cc); an archetype without one fails until its
// figure is added, and a change that deepens a dip past the bound restates the figure with its reason, never the bound.
// TEE_SIM_DIR reads another sim tree, as the harness does (the steward's acceptance read for the idle's cure).
#include "tee/Timeline.h"
#include "tee/Warm.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <filesystem>
#include <string>

using namespace tee;

namespace
{
    const std::filesystem::path kRoot = TEE_SOURCE_DIR;

    std::filesystem::path simDirectory()
    {
        const char* other = std::getenv("TEE_SIM_DIR");
        return other != nullptr && *other != '\0' ? std::filesystem::path(other) : kRoot / "sim";
    }

    // main's lowest speed over 2 to 8 s on p-idle-warm, rpm (main d586682c, sim f23337cc; 26 Nov)
    const std::map<std::string, double> kMainDip = {
        {"air-cooled-flat-six-36", 822.3},      {"big-bore-70-pushrod-v8", 717.2},
        {"big-block-74-pushrod-v8", 544.9},     {"boxer-four-turbo-unequal", 788.4},
        {"diesel-20-common-rail", 730.2},       {"economy-four-15", 736.2},
        {"flat-plane-v8-39-twin-turbo", 896.7}, {"flat-plane-v8-45", 987.2},
        {"four-cam-v8-50", 680.8},              {"hot-hatch-16-16v", 835.8},
        {"inline-five-turbo-rally", 988.8},     {"kei-660-triple-turbo", 816.0},
        {"pickup-diesel-67-inline-six", 714.5}, {"pushrod-v6-38", 618.5},
        {"rotary-twin-13", 775.5},              {"rotary-twin-na-side-port", 744.0},
        {"small-block-57-carb", 680.5},         {"straight-six-32-high-rev", 791.6},
        {"straight-six-twin-turbo-30", 639.2},  {"supercharged-v8-62", 638.6},
        {"three-cylinder-10-turbo", 775.4},     {"tractor-15-inline-six", 655.3},
        {"turbo-four-20-gdi", 736.5},           {"twin-cam-roadster-18", 837.4},
        {"twin-turbo-hot-vee-v8-40", 644.2},    {"two-stage-cam-20", 888.4},
        {"v10-52", 896.0},                      {"v12-65", 878.6},
        {"v6-35-na", 670.2},                    {"v6-35-twin-turbo-pickup", 621.6},
        {"vr6-28", 742.6},
    };
    constexpr double kDipRoom = 25.0;

    // The process's compiled simulator, one for every case here (never deleted: a compiled program destroyed after the
    // JIT at exit crashes on macOS; its compile is some 6 s of a case)
    const std::shared_ptr<Sim>& keptSim(std::string& error)
    {
        static const std::shared_ptr<Sim>* const kept = new std::shared_ptr<Sim>(Sim::compile(simDirectory(), {}, error));
        return *kept;
    }

    void warmStart(const std::string& slug, bool steady)
    {
        CAPTURE(slug);
        std::string error;
        const std::shared_ptr<Sim>& sim = keptSim(error);
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
        double sum = 0.0, lo = 1e9, hi = 0.0, dip = 1e9;
        int n = 0;
        for (std::size_t r = 0; r < run.rowCount(); ++r)
        {
            const double rpm = run.at(r, std::size_t(cs)) * 9.5492966;
            if (run.rowTime[r] >= 13.0)
                sum += rpm, ++n;
            if (run.rowTime[r] >= 8.0)
                lo = std::min(lo, rpm), hi = std::max(hi, rpm);
            else if (run.rowTime[r] >= 2.0)
                dip = std::min(dip, rpm);
        }
        const double rpm = n > 0 ? sum / n : 0.0;
        const int state = int(run.at(run.rowCount() - 1, std::size_t(cst)));
        const auto mainDip = kMainDip.find(slug);
        INFO("state at 14 s " << state << ", mean speed over the last second " << rpm << " rpm, 8 to 14 s " << lo
                              << " to " << hi << " rpm; lowest over 2 to 8 s " << dip << " rpm, main's "
                              << (mainDip != kMainDip.end() ? mainDip->second : 0.0));
        CHECK(state == 2);
        CHECK(rpm > 300.0);
        if (steady)
            CHECK(hi - lo <= 100.0);
        REQUIRE(mainDip != kMainDip.end());                 // a new archetype: add main's figure above
        CHECK(dip >= mainDip->second - kDipRoom);
    }
} // namespace

TEST_CASE("a warm start on p-idle-warm: the flat six that stopped starting and the 5.7 control run steadily at 14 s",
          "[calibration][na]")
{
    for (const char* slug : {"air-cooled-flat-six-36", "small-block-57-carb"})
        warmStart(slug, true);
}

namespace
{
    void turboSixLope();
}

// (marked [!mayfail] until 14890c0 and 3930eadb: its idle loped 640 to 860 rpm, the spark trim's look-ahead short of its
// manifold's filling time; causes.md L). With it, on the same compiled simulator, the turbo six's lope on the bench
// (below): back in the fast tier now that one compile serves both (some 1 s of its own; the steward, 26 Nov)
TEST_CASE("a warm start on p-idle-warm: the 7.0 runs steadily at 14 s and the turbo six's lope on the bench is not fed "
          "by the idle valve", "[calibration][na]")
{
    SECTION("the 7.0's warm start")
    warmStart("big-bore-70-pushrod-v8", true);
    SECTION("the turbo six's lope on the bench is not fed by the idle valve under the models")
    turboSixLope();
}

// The deepest of the start's dips (the 7.4's, 105 rpm under the 650 asked) in the quick tier, beside the three above
TEST_CASE("a warm start on p-idle-warm: the 7.4's dip after the key holds against main's", "[calibration][na]")
{
    warmStart("big-block-74-pushrod-v8", false);
}

TEST_CASE("a warm start on p-idle-warm: every archetype runs at 14 s, its dip after the key held against main's",
          "[nightly][calibration][na]")
{
    for (const auto& entry : std::filesystem::directory_iterator(kRoot / "content" / "builds" / "archetypes"))
        if (entry.is_regular_file() && entry.path().extension() == ".json")
            warmStart(entry.path().stem().string(), false);
}

// The turbo six's idle on the bench with the ECU, boost and intake models (the lead, 2026-11-25): its cam (0.9999, the
// wildest) lopes, the dilution recursion's period near 0.4 s, sd 113 rpm on the reference alone. The ECU's idle valve read
// that lope's swing as far off the target and doubled its gain on air that arrives half a lope late: sd 151 rpm, the valve
// 0.04 to 0.52. With the gain read over 0.5 s (idle.fm), 127. Held under 140: the fix, with a tenth's room, short of the
// 151 before. The speed every block (512 samples at 48 kHz) over 5 to 9 s (old 155, new 127). A section of the 7.0's
// warm-start case above, on its compiled simulator (it was nightly while its own process compiled the program: some 6 of
// its 7 s; the steward, 00:19 and 26 Nov)
namespace
{
void turboSixLope()
{
    std::string error;
    const std::shared_ptr<Sim>& sim = keptSim(error);
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
} // namespace
