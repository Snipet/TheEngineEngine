// tests/host/EngineTest.cpp: SimHost on the simulator itself (sim/engine.fm), once it is there: it plays a build and a
// scenario to the bit as K3a's single-threaded EnginePlayer does, and the test library drives it.
#include "Support.h"

#include "EngineTwin.h"

#include <set>
#include <catch2/catch_test_macros.hpp>

#include <cstring>

using namespace tee;
using namespace tee::test;

namespace
{
    std::filesystem::path simDir()
    {
        return std::filesystem::path(TEE_SOURCE_DIR) / "sim";
    }

    bool haveSim()
    {
        return std::filesystem::exists(simDir() / "engine.fm");
    }
} // namespace

TEST_CASE("SimHost plays the simulator to the bit as EnginePlayer does", "[engine][sim]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    const std::filesystem::path root = TEE_SOURCE_DIR;
    for (const char* buildName : {"v8", "large-diesel"})
        for (const char* scenarioName : {"free-revs", "drive-car"})
        {
            CAPTURE(buildName, scenarioName);
            const std::string buildFile = (root / "content" / "builds" / (std::string(buildName) + ".json")).string();
            const std::string scenarioFile =
                (root / "tests" / "sim" / "scenarios" / (std::string(scenarioName) + ".json")).string();
            std::string error;
            std::vector<float> theirsL, theirsR;
            REQUIRE(enginePlayerRender(buildFile, scenarioFile, 6.0, 512, theirsL, theirsR, error));

            // SimHost, through the same files
            Timeline t;
            REQUIRE(loadTimeline(root / "tests" / "sim" / "scenarios" / (std::string(scenarioName) + ".json"), t, error));
            SimHost host;
            REQUIRE(host.attach(compiled(simDir(), {}, error), offline(48000.0, 512), error));
            Build build(host.fields());
            std::vector<std::string> buildWarnings;
            REQUIRE(build.loadFile(root / "content" / "builds" / (std::string(buildName) + ".json"), BuildPart::Whole,
                                   buildWarnings, error));
            build.attach(&host);
            SimControls start;
            start.ignition = 1;   // EnginePlayer's Controls start with the key on
            host.setControls(start);
            RunOptions options;
            options.seconds = 6.0;
            RunResult ours;
            REQUIRE(runTimeline(host, build, t, options, ours, error));

            REQUIRE(ours.left.size() == theirsL.size());
            const double depth = measure::nullDepth(theirsL, ours.left);
            CAPTURE(depth, measure::peak(ours.left));
            CHECK(std::memcmp(ours.left.data(), theirsL.data(), ours.left.size() * sizeof(float)) == 0);
            CHECK(std::memcmp(ours.right.data(), theirsR.data(), ours.right.size() * sizeof(float)) == 0);
            CHECK(measure::peak(ours.left) > 0.01);
        }
}

TEST_CASE("the test library drives the simulator: a start, an idle, a rev to the limiter", "[engine][sim][scenario]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    const Run r = tee::test::Scenario("v8")
                      .at(0.0).mode(1).ignition(tee::On)
                      .at(0.25).ignition(tee::Start).until(2.0).ignition(tee::On)
                      .at(4.0).throttle(1.0).ramp(0.3)
                      .run(9.0);
    INFO(r.error());
    REQUIRE(r.ok());
    CHECK(r.tel("engine.state").at(3.5) == 2);
    CHECK(r.tel("engine.speed").during(3.0, 3.9).mean() > 0.5 * r.tel("engine.idle").last());
    CHECK(r.tel("engine.speed").during(7.0, 9.0).max() > 0.95 * r.tel("engine.redline").last());
    CHECK(r.audio().finite());
    CHECK(r.audio().peak() <= 1.0f);
    CHECK(r.host().guardTrips == 0);
    const Signal held = r.audio().during(7.0, 9.0);
    WARN("v8 on the limiter: firing " << r.firingHz(7.0, 9.0) << " Hz, its line " << held.line(r.firingHz(7.0, 9.0)).db
                                      << " dB over a floor of " << held.floor().db << " dB; largest step "
                                      << r.audio().largestStep() << "; " << r.speed() << " times real time; the host's own"
                                      << " work a block " << r.host().hostMicrosMean << " us mean, "
                                      << r.host().hostMicrosPeak << " us peak; load mean "
                                      << r.host().loadMean);
    CHECK(held.line(r.firingHz(7.0, 9.0)).above(held.floor(), 12));
    std::string steps;
    for (double t = 0.0; t < 9.0; t += 1.0)
        steps += std::to_string(static_cast<int>(t)) + " s: " + std::to_string(r.audio().during(t, t + 1.0).largestStep()) + "  ";
    WARN("the largest step each second: " << steps);
}

TEST_CASE("a telemetry array's elements are columns, assertions and dyno fields by name; a wrong index is said",
          "[engine][sim][scenario]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    const auto sim = compiled(simDir(), {}, error);
    REQUIRE(sim != nullptr);
    const FieldInfo* f = nullptr;
    std::size_t index = 0;
    CHECK(telemetryElement(sim->fields(), "engine.cylPeak[3]", f, index, error));
    CHECK(index == 3);
    CHECK(telemetryElement(sim->fields(), "tel.engine.speed", f, index, error));
    CHECK_FALSE(telemetryElement(sim->fields(), "engine.cylPeak[16]", f, index, error));
    CHECK(error.find("has 16 elements, 0 to 15") != std::string::npos);
    CHECK_FALSE(telemetryElement(sim->fields(), "engine.cylPeak", f, index, error));
    CHECK(error.find("name one") != std::string::npos);
    CHECK_FALSE(telemetryElement(sim->fields(), "engine.speed[0]", f, index, error));
    CHECK(error.find("no array") != std::string::npos);

    const Run r = tee::test::Scenario("v8")
                      .at(0.0).mode(1).ignition(tee::On)
                      .at(0.25).ignition(tee::Start).until(2.0).ignition(tee::On)
                      .run(3.0);
    INFO(r.error());
    REQUIRE(r.ok());
    // a running V8: each of its eight cylinders peaks well above the air, the ninth slot is empty
    for (int c = 0; c < 8; ++c)
        CHECK(r.tel("engine.cylPeak[" + std::to_string(c) + "]").during(2.5, 3.0).max() > 5.0e5);
    CHECK(r.tel("engine.cylPeak[8]").during(2.5, 3.0).max() == 0.0);
}

TEST_CASE("SimControls carries every field of the program's Controls, and nothing it lacks", "[engine][sim][simhost]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    const auto sim = compiled(simDir(), {}, error);
    REQUIRE(sim != nullptr);
    std::set<std::string> carried;
    for (const ControlField& c : controlFields())
        carried.insert(c.name);
    std::set<std::string> program;
    for (const FieldInfo& f : sim->fields().all())
        if (f.block == "ctl")
            program.insert(f.path.substr(4));
    // a field added to sim/spec/controls.fm must be added to SimControls and controlFields() (SimHost.h)
    for (const std::string& name : program)
    {
        CAPTURE(name);
        CHECK(carried.count(name) == 1);
    }
    for (const std::string& name : carried)
    {
        CAPTURE(name);
        CHECK(program.count(name) == 1);
    }
    SimControls c;
    CHECK(setControl(c, "bakeMaps", 3));
    CHECK(c.bakeMaps == 3);
}

TEST_CASE("an engine the simulator found diverged (FLAG_FAULT) is counted, said in words, and cranked back",
          "[engine][sim][simhost][guard]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    const auto sim = compiled(simDir(), {}, error);
    REQUIRE(sim != nullptr);
    SimHost host;
    LoadOptions load = offline(48000.0, 512);
    REQUIRE(host.attach(sim, load, error));
    SimControls c;
    c.mode = 1;
    c.ignition = 2;
    host.setControls(c);
    std::vector<float> l(512), r(512);
    const auto play = [&](int blocks) {
        for (int i = 0; i < blocks; ++i)
        {
            host.process(l.data(), r.data(), 512);
            host.service();
        }
    };
    play(200);   // some 2 s: started
    c.ignition = 1;
    host.setControls(c);
    play(50);
    TelemetryFrame t;
    REQUIRE(host.telemetry(t));
    REQUIRE(tel(host, t, "tel.engine.state") == 2);
    CHECK(t.host.engineFaults == 0);

    // a crank speed past five times the redline, held before every call (LoadOptions::hold): the simulator puts the
    // engine at rest each time, and says so
    SimHost broken;
    LoadOptions held = offline(48000.0, 512);
    held.hold = {{"crank.omega", 1.0e7}};
    REQUIRE(broken.attach(sim, held, error));
    broken.setControls(c);
    for (int i = 0; i < 20; ++i)
    {
        broken.process(l.data(), r.data(), 512);
        broken.service();
    }
    REQUIRE(broken.telemetry(t));
    CHECK(t.host.engineFaults > 0);
    CHECK(broken.stopReason().find("the engine diverged and was restarted") != std::string::npos);
    for (const float v : l)
        CHECK(std::isfinite(v));
}

TEST_CASE("a quality change swaps the engine at 48 kHz inside under a crossfade, and the key left on runs it again",
          "[engine][sim][simhost]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    const auto sim = compiled(simDir(), {}, error);
    REQUIRE(sim != nullptr);
    SimHost host;
    REQUIRE(host.attach(sim, offline(48000.0, 512), error));
    SimControls c;
    c.mode = 1;
    c.ignition = 2;
    host.setControls(c);
    std::vector<float> l(512), r(512);
    double peak = 0.0;
    bool finite = true;
    const auto play = [&](int blocks) {
        peak = 0.0;
        for (int i = 0; i < blocks; ++i)
        {
            host.process(l.data(), r.data(), 512);
            host.service();
            for (int k = 0; k < 512; ++k)
            {
                finite = finite && std::isfinite(l[k]) && std::isfinite(r[k]);
                peak = std::max(peak, static_cast<double>(std::fabs(l[k])));
            }
        }
    };
    play(200);
    c.ignition = 1;
    host.setControls(c);
    play(50);
    CHECK(host.quality() == SimHost::Quality::Full);

    if (!host.setQuality(SimHost::Quality::Economy, error))
        SKIP("the program does not take TEE_ENGINE_RATE_48000: " << error);
    CHECK(host.quality() == SimHost::Quality::Economy);
    CHECK(host.innerRate() == 48000.0);
    play(250);   // the crossfade, the crank (some 0.3 s), then running
    TelemetryFrame t;
    REQUIRE(host.telemetry(t));
    CHECK(tel(host, t, "tel.engine.state") == 2);
    CHECK(t.host.reloads >= 1);
    CHECK(finite);
    CHECK(peak > 0.01);
    CHECK(host.controls().ignition == 1);   // the person's key untouched

    REQUIRE(host.setQuality(SimHost::Quality::Full, error));
    CHECK(host.innerRate() == 96000.0);
    play(250);
    REQUIRE(host.telemetry(t));
    CHECK(tel(host, t, "tel.engine.state") == 2);
    CHECK(finite);
}
