// tests/host/EngineTest.cpp: SimHost on the simulator itself (sim/engine.fm), once it is there: it plays a build and a
// scenario to the bit as K3a's single-threaded EnginePlayer does, and the test library drives it.
#include "Support.h"

#include "EngineTwin.h"

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
                                      << r.audio().largestStep() << "; " << r.speed() << " times real time; load mean "
                                      << r.host().loadMean);
    CHECK(held.line(r.firingHz(7.0, 9.0)).above(held.floor(), 12));
    std::string steps;
    for (double t = 0.0; t < 9.0; t += 1.0)
        steps += std::to_string(static_cast<int>(t)) + " s: " + std::to_string(r.audio().during(t, t + 1.0).largestStep()) + "  ";
    WARN("the largest step each second: " << steps);
}
