// tests/host/EngineTest.cpp: SimHost on the simulator itself (sim/engine.fm), once it is there: it plays a build and a
// scenario to the bit as K3a's single-threaded EnginePlayer does, and the test library drives it.
#include "Support.h"

#include "EngineTwin.h"
#include "tee/Bake.h"
#include "tee/Dyno.h"

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

namespace
{
    // SimHost against K3's EnginePlayer, through the same files, to the bit (each build and scenario named)
    void playsToTheBit(const std::vector<std::pair<const char*, const char*>>& cases, double seconds)
    {
        const std::filesystem::path root = TEE_SOURCE_DIR;
        for (const auto& [buildName, scenarioName] : cases)
            {
                CAPTURE(buildName, scenarioName);
                const std::string buildFile = (root / "content" / "builds" / (std::string(buildName) + ".json")).string();
                const std::string scenarioFile =
                    (root / "tests" / "sim" / "scenarios" / (std::string(scenarioName) + ".json")).string();
                std::string error;
                std::vector<float> theirsL, theirsR;
                REQUIRE(enginePlayerRender(buildFile, scenarioFile, seconds, 512, theirsL, theirsR, error));

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
                options.seconds = seconds;
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
} // namespace

TEST_CASE("SimHost plays the simulator to the bit as EnginePlayer does", "[engine][sim]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    playsToTheBit({{"v8", "free-revs"}}, 3.0);   // the quick form; every build and scenario: [nightly] below
}

TEST_CASE("SimHost plays the simulator to the bit as EnginePlayer does: both builds, both scenarios, 6 s",
          "[engine][sim][nightly]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    playsToTheBit({{"v8", "free-revs"}, {"v8", "drive-car"}, {"large-diesel", "free-revs"}, {"large-diesel", "drive-car"}},
                  6.0);
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
          "[engine][sim][simhost][nightly]")
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

TEST_CASE("the dyno runner starts a common-rail diesel and holds a twin turbo's boost on steps", "[engine][sim][dyno]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    const auto sim = compiled(simDir(), {}, error);
    REQUIRE(sim != nullptr);
    const auto run = [&](const char* name, std::vector<double> steps) {
        Build build(sim->fields());
        std::vector<std::string> warnings;
        REQUIRE(build.loadFile(std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/archetypes" / name, BuildPart::Whole,
                               warnings, error));
        DynoPlan plan;
        plan.steps = std::move(steps);
        plan.settle = 3.0;
        plan.measure = 0.5;
        plan.fields = {"engine.state", "boost.pressure"};
        DynoCurve curve;
        REQUIRE(runDyno(sim, build, plan, curve, error));
        INFO(error);
        return curve;
    };
    // P's two: from the key off straight to start, the diesel never cranked, and the twin turbo's boost wandered
    const DynoCurve diesel = run("diesel-20-common-rail.json", {300.0});
    REQUIRE(diesel.points.size() == 1);
    CHECK(diesel.points[0].fields[0] == 2.0);
    CHECK(diesel.points[0].torque > 50.0);
    const DynoCurve six = run("straight-six-twin-turbo-30.json", {300.0, 450.0});
    REQUIRE(six.points.size() == 2);
    for (const DynoPoint& p : six.points)
    {
        CHECK(p.fields[0] == 2.0);
        // no runaway (P measured 2.99 bar at 4297 rpm when the key went from off to start); how much boost the build
        // makes is its own (the content's maps and springs), not the runner's
        CHECK(p.fields[1] > 30000.0);
        CHECK(p.fields[1] < 130000.0);
    }
}

TEST_CASE("the intake cam baked by a sweep of the fast dyno: the best advance by speed, into ecu.maps.camIn",
          "[engine][sim][predict]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    const auto sim = compiled(simDir(), {}, error);
    REQUIRE(sim != nullptr);
    Build build(sim->fields());
    std::vector<std::string> warnings;
    REQUIRE(build.loadFile(std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/archetypes/v12-65.json",
                           BuildPart::Whole, warnings, error));
    CamBakeOptions options;
    options.step = 25.0;   // three cells (0, 25, 50) and three speeds: the machine, not N1's whole table
    options.predict.from = 209.44;
    options.predict.to = 628.32;
    options.predict.points = options.predict.coarse = 3;
    CamBakeResult r;
    REQUIRE(bakeIntakeCam(sim, build, options, r, error));
    INFO(error);
    REQUIRE(r.advances.size() == 3);
    REQUIRE(r.speeds.size() == 3);
    for (std::size_t s = 0; s < r.speeds.size(); ++s)
    {
        CHECK(std::find(r.advances.begin(), r.advances.end(), r.best[s]) != r.advances.end());
        CHECK(r.bestTorque[s] >= r.torque.front()[s] - 1e-9);   // never worse than parked
    }
    REQUIRE(applyIntakeCam(build, r, error));
    CHECK(build.get("ecu.maps.camInAuto") == 0.0);
    // Map16's values in its fields' order: x[16], y[16], z[16][16], nx, ny
    const std::vector<double>& map = build.values(build.fieldOf("ecu.maps.camIn"));
    REQUIRE(map.size() == 16 + 16 + 256 + 2);
    CHECK(std::fabs(map[0] - r.speeds[0]) < 1e-3 * r.speeds[0]);
    CHECK(map[16 + 16 + 256] == 3.0);   // nx
    CHECK(map[16 + 16 + 0 * 16 + 0] == r.best[0]);
    CHECK(build.canUndo());
    WARN("v12-65's intake cam baked in " << r.wallSeconds << " s: " << r.best[0] << ", " << r.best[1] << ", " << r.best[2]
                                         << " deg at 2000, 4000, 6000 rpm");
}

namespace
{
    // The predicted curve against the stepped dyno with the exhaust's heat settled (both the steady state) at the
    // speeds given, on an archetype: within `bound` of the steps' largest torque.
    void predictedMatchesSteps(const char* archetype, const std::vector<double>& speeds, double bound)
    {
        std::string error;
        const auto sim = compiled(simDir(), {}, error);
        REQUIRE(sim != nullptr);
        Build build(sim->fields());
        std::vector<std::string> warnings;
        REQUIRE(build.loadFile(std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/archetypes" / archetype,
                               BuildPart::Whole, warnings, error));
        DynoPlan plan;
        plan.steps = speeds;
        plan.settle = 4.0;
        plan.measure = 1.0;
        DynoCurve steps;
        REQUIRE(runDyno(sim, build, plan, steps, error));
        INFO(error);
        REQUIRE(steps.settled);
        REQUIRE(steps.points.size() == speeds.size());
        PredictOptions options;
        options.from = speeds.front();
        options.to = speeds.back();
        options.points = options.coarse = std::max(2, static_cast<int>(speeds.size()));   // (two at one speed)
        PredictCurve predicted;
        REQUIRE(predictCurve(sim, build, options, predicted, error));
        REQUIRE(predicted.points.size() >= speeds.size());
        double peak = 0.0;
        for (const DynoPoint& p : steps.points)
            peak = std::max(peak, std::fabs(p.torque));
        for (std::size_t i = 0; i < speeds.size(); ++i)
        {
            CAPTURE(archetype, speeds[i], predicted.points[i].torque, steps.points[i].torque);
            CHECK(std::fabs(predicted.points[i].torque - steps.points[i].torque) <= bound * peak);
        }
    }
} // namespace

TEST_CASE("a predicted point is the stepped dyno's settled torque within 3 %: the economy four at 5439 rpm",
          "[engine][sim][predict][dyno]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    predictedMatchesSteps("economy-four-15.json", {569.6}, 0.03);   // P saw 186 N m between steps of about 160
}

TEST_CASE("predicted points are the stepped dyno's settled torque within 3 %: the V12 by its resonance, the economy four",
          "[engine][sim][predict][dyno][nightly]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    predictedMatchesSteps("v12-65.json", {680.68, 698.79, 717.33}, 0.03);   // 6500, 6673, 6850 rpm (D's)
    predictedMatchesSteps("economy-four-15.json", {560.0, 569.6, 579.0}, 0.03);
}
