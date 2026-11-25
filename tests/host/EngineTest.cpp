// tests/host/EngineTest.cpp: SimHost on the simulator itself (sim/engine.fm), once it is there: it plays a build and a
// scenario to the bit as K3a's single-threaded EnginePlayer does, and the test library drives it.
#include "Support.h"

#include "EngineTwin.h"
#include "tee/Bake.h"
#include "tee/Dyno.h"
#include "tee/Timeline.h"
#include "tee/EnginePlayer.h"

#include <chrono>
#include <limits>
#include <set>
#include <thread>
#include <catch2/catch_approx.hpp>
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

TEST_CASE("the dyno runner starts a common-rail diesel and holds a twin turbo's boost on steps",
          "[engine][sim][dyno][nightly]")
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
    REQUIRE(r.advances.size() >= 3);   // the grid's three, and the half steps beside each speed's best
    for (const double cell : {0.0, 25.0, 50.0})
        CHECK(std::find(r.advances.begin(), r.advances.end(), cell) != r.advances.end());
    CHECK((r.halfSteps > 0) == (r.advances.size() > 3));
    CHECK(std::is_sorted(r.advances.begin(), r.advances.end()));
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
    CHECK(map[16 + 16 + 256 + 1] == 4.0);   // ny: the load axis 0, 0.75, 0.95, 3
    CHECK(map[16 + 16 + 0 * 16 + 0] == 0.0);   // parked at light load (idle)
    CHECK(map[16 + 16 + 0 * 16 + 1] == 0.0);
    CHECK(map[16 + 16 + 0 * 16 + 2] == r.best[0]);   // the bake's advance from 0.95 up
    CHECK(map[16 + 16 + 0 * 16 + 3] == r.best[0]);
    CHECK(build.canUndo());
    // its key: current as applied; stale after an edit that moves the cam; current again when undone; never moved by a
    // dyno's heat-settle counter (hidden) nor by the cam map it wrote
    CHECK(r.key == intakeCamKey(build, *sim));
    CHECK(build.get("ecu.maps.camInKey") == static_cast<double>(r.key));
    CHECK(intakeCamCurrent(build, *sim));
    const double centre = build.get("valve.exCentre");
    build.set("valve.exCentre", centre + 4.0);
    CHECK_FALSE(intakeCamCurrent(build, *sim));
    build.set("valve.exCentre", centre);
    CHECK(intakeCamCurrent(build, *sim));
    build.set("exhaust.heatSettle", build.get("exhaust.heatSettle") + 1.0);
    CHECK(intakeCamCurrent(build, *sim));
    WARN("v12-65's intake cam baked in " << r.wallSeconds << " s: " << r.best[0] << ", " << r.best[1] << ", " << r.best[2]
                                         << " deg at 2000, 4000, 6000 rpm");
}

namespace
{
    // The predicted curve against the stepped dyno with the exhaust's heat settled (both the steady state) at the
    // speeds given, on an archetype: within `bound` of the steps' largest torque.
    void predictedMatchesSteps(const char* archetype, const std::vector<double>& speeds, double bound,
                               double settle = 4.0)
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
        plan.settle = settle;
        plan.measure = 2.0;   // whole teeth of knock control's sawtooth, where it acts
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

TEST_CASE("predicted points are the stepped dyno's settled torque within 3 %: the V12 by its resonance, the economy four, the turbo four",
          "[engine][sim][predict][dyno][nightly]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    predictedMatchesSteps("v12-65.json", {680.68, 698.79, 717.33}, 0.03);   // 6500, 6673, 6850 rpm (D's)
    predictedMatchesSteps("economy-four-15.json", {560.0, 569.6, 579.0}, 0.03);
    // knock-limited (its retard a sawtooth): each side's mean over whole teeth of it. Not 2910 rpm: there the turbo's
    // spool follows the exhaust's heat over minutes, and neither dyno has one answer within a minute (the stepped
    // dyno 220 to 281 N m by its settle's timing, 235 and 247 after 60 s without and with the settles)
    predictedMatchesSteps("turbo-four-20-gdi.json", {423.8, 542.9}, 0.03);
}

TEST_CASE("the knock limit's key moves with what moves the knock limit, and not with the air or the person's timing",
          "[engine][sim][bake]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    const auto sim = compiled(simDir(), {}, error);
    REQUIRE(sim != nullptr);
    Build build(sim->fields());
    std::vector<std::string> warnings;
    REQUIRE(build.loadFile(std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/archetypes/turbo-four-20-gdi.json",
                           BuildPart::Whole, warnings, error));
    const std::int64_t key = knockLimitKey(build, *sim);
    CHECK(key >= 0);
    CHECK(key <= 0x7fffffff);
    CHECK(knockLimitKey(build, *sim) == key);   // the same build, the same key
    CHECK(sim->sourceHash() != 0);
    // not the air's temperature (baked at 300 K), nor ecu.timing (the person's offset on top)
    if (build.fieldOf("env.temperature") >= 0)
    {
        build.set("env.temperature", build.get("env.temperature") + 20.0);
        CHECK(knockLimitKey(build, *sim) == key);
    }
    if (build.fieldOf("ecu.timing") >= 0)
    {
        build.set("ecu.timing", build.get("ecu.timing") + 3.0);
        CHECK(knockLimitKey(build, *sim) == key);
    }
    // the base key (the cam bake's): the cam maps move the key and not the base
    const std::int64_t base = knockLimitBaseKey(build, *sim);
    if (build.fieldOf("ecu.maps.camInAuto") >= 0)
    {
        const double was = build.get("ecu.maps.camInAuto");
        build.set("ecu.maps.camInAuto", was == 0.0 ? 1.0 : 0.0);
        CHECK(knockLimitKey(build, *sim) != key);
        CHECK(knockLimitBaseKey(build, *sim) == base);
        build.set("ecu.maps.camInAuto", was);
    }
    // the hidden counters (a dyno's heatSettle presses) move neither
    if (build.fieldOf("boost.heatSettle") >= 0)
    {
        build.set("boost.heatSettle", build.get("boost.heatSettle") + 1.0);
        CHECK(knockLimitKey(build, *sim) == key);
        CHECK(knockLimitBaseKey(build, *sim) == base);
    }
    // the compression, the fuel and the boost's section do
    const double compression = build.get("engine.compression");
    build.set("engine.compression", compression - 0.5);
    CHECK(knockLimitKey(build, *sim) != key);
    CHECK(knockLimitBaseKey(build, *sim) != base);
    build.set("engine.compression", compression);
    CHECK(knockLimitKey(build, *sim) == key);
    for (const FieldInfo& f : build.fields().all())
        if (f.path.rfind("spec.boost.", 0) == 0 && !f.hidden && f.kind == FieldKind::Number && f.maximum && f.minimum
            && build.get(f.path) < *f.maximum)
        {
            INFO(f.path);
            const double was = build.get(f.path);
            build.set(f.path, std::min(was + 0.25 * (*f.maximum - *f.minimum), *f.maximum));
            CHECK(knockLimitKey(build, *sim) != key);
            build.set(f.path, was);
            break;
        }
    // a program without the table is never current
    if (build.fieldOf("ecu.maps.knockLimitAuto") < 0)
    {
        CHECK_FALSE(knockLimitCurrent(build, *sim));
        return;
    }
    // a bake applied is current; saved and loaded it still is (its key, a hidden fingerprint, is in the file); a
    // change to what moves the knock limit makes it stale
    CHECK_FALSE(knockLimitCurrent(build, *sim));
    KnockBakeResult r;
    r.speeds = {200.0, 400.0, 600.0};
    r.loads = {0.5, 1.0, 2.0};
    r.limit = {{0.0, 0.0, 3.0}, {0.0, 1.0, 6.0}, {0.0, 0.0, 2.5}};
    r.key = knockLimitKey(build, *sim);
    REQUIRE(applyKnockLimit(build, r, error));
    CHECK(knockLimitCurrent(build, *sim));
    Build again(sim->fields());
    REQUIRE(again.load(build.save(), BuildPart::Whole, warnings, error));
    CHECK(static_cast<std::int64_t>(again.get("ecu.maps.knockLimitKey")) == r.key);
    CHECK(knockLimitCurrent(again, *sim));
    again.set("engine.compression", again.get("engine.compression") - 0.5);
    CHECK_FALSE(knockLimitCurrent(again, *sim));
}

TEST_CASE("the knock limit baked on the turbo four: a knocking point's cell its mean retard and 2, none 0, lower "
          "pedals run only where it knocks",
          "[engine][sim][bake][nightly]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    const auto sim = compiled(simDir(), {}, error);
    REQUIRE(sim != nullptr);
    Build build(sim->fields());
    std::vector<std::string> warnings;
    REQUIRE(build.loadFile(std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/archetypes/turbo-four-20-gdi.json",
                           BuildPart::Whole, warnings, error));
    KnockBakeOptions options;
    options.speeds = 5;
    options.pedals = {1.0, 0.5};
    options.boostSteps = 0;   // the pedals' rules here; the boost targets' below
    KnockBakeResult r;
    REQUIRE(bakeKnockLimit(sim, build, options, r, error));
    REQUIRE(r.speeds.size() == 5);
    REQUIRE(r.limit.size() == 5);
    CHECK(r.key == knockLimitKey(build, *sim));
    bool someKnock = false;
    for (std::size_t s = 0; s < r.speeds.size(); ++s)
    {
        INFO(s);
        REQUIRE_FALSE(r.measured[s].empty());
        const KnockBakeCell& full = r.measured[s].front();
        CHECK(full.pedal == 1.0);
        // the second pedal only where full throttle knocked
        CHECK((r.measured[s].size() == 2) == (full.valid && full.retardMean >= options.knockLimit));
        someKnock = someKnock || full.retardMean >= options.knockLimit;
        // at the load full throttle gave, the table is its cell (by straight lines between the measured loads)
        double want = full.retardMean >= options.knockLimit ? full.retardMean + options.margin : 0.0;
        for (std::size_t j = 0; j < r.loads.size(); ++j)
        {
            CHECK(r.limit[s][j] >= 0.0);
            if (std::fabs(r.loads[j] - full.load) < 1e-9)
                CHECK(r.limit[s][j] == want);
        }
    }
    CHECK(someKnock);   // a turbo four on its archetype's boost knocks somewhere at full throttle
}

TEST_CASE("the knock limit's loads on a turbo: boost targets from its level down to its spring, spreading the loads",
          "[engine][sim][bake][nightly]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    const auto sim = compiled(simDir(), {}, error);
    REQUIRE(sim != nullptr);
    Build build(sim->fields());
    std::vector<std::string> warnings;
    REQUIRE(build.loadFile(std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/archetypes/turbo-four-20-gdi.json",
                           BuildPart::Whole, warnings, error));
    KnockBakeOptions options;
    options.speeds = 3;
    options.pedals = {1.0};
    options.boostSteps = 3;
    KnockBakeResult r;
    REQUIRE(bakeKnockLimit(sim, build, options, r, error));
    CHECK(r.notes.find("boost targets") != std::string::npos);
    const double spring = build.get("boost.gateSpring");
    bool spread = false;
    for (const auto& column : r.measured)
    {
        REQUIRE_FALSE(column.empty());
        CHECK(column.front().boostTarget >= spring);   // the build's level first
        for (std::size_t i = 1; i < column.size(); ++i)
        {
            CHECK(column[i].boostTarget < column[i - 1].boostTarget);   // down to the spring
            CHECK(column[i].boostTarget >= spring - 1.0);
            if (column[i].valid && column[i - 1].valid && column[i - 1].load - column[i].load > 0.1)
                spread = true;   // a lower target, a lower load
        }
    }
    CHECK(spread);
}

TEST_CASE("the background bake: a stale table set aside at once, baked when the edits rest, taken, current; an edit "
          "of what moves the knock limit makes it stale again",
          "[engine][sim][bake]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    const auto sim = compiled(simDir(), {}, error);
    REQUIRE(sim != nullptr);
    Build build(sim->fields());
    std::vector<std::string> warnings;
    REQUIRE(build.loadFile(std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/archetypes/turbo-four-20-gdi.json",
                           BuildPart::Whole, warnings, error));
    if (build.fieldOf("ecu.maps.knockLimitAuto") < 0)
        SKIP("the program has no knock-limit table yet");
    KnockBakeOptions options;   // small, for the test: two speeds at full throttle
    options.speeds = 2;
    options.pedals = {1.0};
    options.predict.threads = 2;
    KnockBakeService bakes(sim, options, 0.05);

    // a table of another build (a key not this one's), in use: set aside at once, a bake asked
    build.set("ecu.maps.knockLimitAuto", 0.0);
    build.set("ecu.maps.knockLimitKey", 12345.0);
    const KnockBakeState asked = bakes.consider(build);
    CHECK((asked == KnockBakeState::Waiting || asked == KnockBakeState::Baking));
    CHECK(build.get("ecu.maps.knockLimitAuto") == 1.0);   // the engine on its knock control alone meanwhile
    CHECK(bakes.consider(build) != KnockBakeState::Current);   // asked once: a second consider asks nothing new

    // taken and applied (the UI's each frame); its progress counted in points while it baked
    int mostDone = 0, total = 0;
    bool applied = false;
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(120);
    while (!applied && std::chrono::steady_clock::now() < until)
    {
        REQUIRE(bakes.state() != KnockBakeState::Failed);
        const CalibrationBakeService::Progress p = bakes.progress();
        mostDone = std::max(mostDone, p.done);
        total = std::max(total, p.total);
        CHECK(p.done <= p.total);
        applied = bakes.takeAndApply(build, error);
        INFO(error);
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    REQUIRE(applied);
    CHECK(total >= 2);   // two speeds at full throttle: about two points
    CHECK(knockLimitCurrent(build, *sim));
    CHECK(bakes.state() == KnockBakeState::Current);   // so it says once applied, without a consider()
    CHECK(std::string(knockBakeStateName(bakes.state())) == "baked");
    BakedFields r;
    REQUIRE(bakes.lastSpark(r));   // the table applied, kept for TUNE
    CHECK(r.key == knockLimitKey(build, *sim));
    CHECK(bakes.stillKnockingCells() >= 0);
    CHECK(bakes.consider(build) == KnockBakeState::Current);

    // a control is not an edit of the build: nothing to consider. The compression is: stale, set aside, asked again
    build.set("engine.compression", build.get("engine.compression") - 0.5);
    CHECK(bakes.consider(build) != KnockBakeState::Current);
    CHECK(build.get("ecu.maps.knockLimitAuto") == 1.0);
}

namespace
{
    // A host of the real program started warm, not held (LoadOptions::initial), running at idle; `blocks` of 512
    double oilAfter(SimHost& host, int blocks)
    {
        std::vector<float> l(512), r(512);
        for (int i = 0; i < blocks; ++i)
        {
            host.process(l.data(), r.data(), 512);
            host.service();
        }
        TelemetryFrame t;
        REQUIRE(host.telemetry(t));
        return tel(host, t, "tel.engine.oilTemperature");
    }
} // namespace

TEST_CASE("an engine started warm (LoadOptions::initial): its working temperatures at the first block, then its own",
          "[engine][sim][simhost]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    const auto sim = compiled(simDir(), {}, error);
    REQUIRE(sim != nullptr);
    SimHost host;
    LoadOptions load = offline(48000.0, 512);
    load.initial = warmEngine();
    REQUIRE(host.attach(sim, load, error));
    SimControls c;
    c.mode = 1;
    c.ignition = 2;
    host.setControls(c);
    const double first = oilAfter(host, 1);
    CHECK(first == Catch::Approx(378.0).margin(0.5));   // the thermostat's 368 and 10
    c.ignition = 1;
    host.setControls(c);
    const double later = oilAfter(host, static_cast<int>(30.0 * 48000.0 / 512));
    INFO("oil " << first << " K at the first block, " << later << " K after 30 s at idle");
    CHECK(std::fabs(later - first) > 0.01);   // not held (a held value stays 378 exactly): it moved by itself
    CHECK(later > 340.0);                     // and from warm, not from cold
}

TEST_CASE("a quality change keeps the engine's heat as it was, not its initial temperatures", "[engine][sim][simhost][nightly]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    const auto sim = compiled(simDir(), {}, error);
    REQUIRE(sim != nullptr);
    SimHost host;
    LoadOptions load = offline(48000.0, 512);
    load.initial = warmEngine();
    REQUIRE(host.attach(sim, load, error));
    SimControls c;
    c.mode = 1;
    c.ignition = 2;
    host.setControls(c);
    oilAfter(host, 100);
    c.ignition = 1;
    host.setControls(c);
    const double before = oilAfter(host, static_cast<int>(30.0 * 48000.0 / 512));
    if (!host.setQuality(SimHost::Quality::Economy, error))
        SKIP("the program does not take TEE_ENGINE_RATE_48000: " << error);
    const double after = oilAfter(host, 20);
    INFO("oil " << before << " K before the swap, " << after << " K after it");
    CHECK(after == Catch::Approx(before).margin(0.2));
    CHECK(std::fabs(after - 378.0) > 0.05);   // not its initial value again
}

TEST_CASE("the knock limit's corrections: a cell whose mean retard was over 12 degrees measured again with the table "
          "applied, its residual added, said when it still knocks",
          "[engine][sim][bake][nightly]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    const auto sim = compiled(simDir(), {}, error);
    REQUIRE(sim != nullptr);
    Build build(sim->fields());
    std::vector<std::string> warnings;
    REQUIRE(build.loadFile(std::filesystem::path(TEE_SOURCE_DIR)
                               / "content/builds/archetypes/straight-six-twin-turbo-30.json",
                           BuildPart::Whole, warnings, error));
    if (build.fieldOf("ecu.maps.knockLimitAuto") < 0)
        SKIP("the program has no knock-limit table yet");
    KnockBakeOptions options;
    options.speeds = 8;       // (2686 rpm among them: N2's 2450)
    options.pedals = {1.0};
    options.boostSteps = 1;   // the build's own level: where the retard sits at its ceiling
    KnockBakeResult r;
    REQUIRE(bakeKnockLimit(sim, build, options, r, error));
    int corrected = 0;
    for (std::size_t s = 0; s < r.measured.size(); ++s)
        for (const KnockBakeCell& c : r.measured[s])
        {
            INFO("speed " << r.speeds[s] << ": mean " << c.retardMean << ", corrections " << c.corrections
                          << ", residual " << c.residual << ", cell " << c.limit);
            if (c.valid && c.retardMean > options.correctAbove)
            {
                CHECK(c.corrections >= 1);
                CHECK(c.corrections <= options.corrections);
                CHECK(c.limit >= c.retardMean + options.margin);
                CHECK(c.stillKnocking == (c.residual >= options.knockLimit));
                ++corrected;
            }
            else
            {
                CHECK(c.corrections == 0);
                CHECK_FALSE(c.stillKnocking);
            }
        }
    CHECK(corrected >= 1);   // the six at its full boost has a cell at the ceiling (N2's proof: 2450 rpm)
    REQUIRE(r.stillKnocking.size() == r.speeds.size());
}

TEST_CASE("the first frame of every telemetry field is a value the program wrote: finite, and no denormal garbage",
          "[engine][sim][simhost]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    const auto sim = compiled(simDir(), {}, error);
    REQUIRE(sim != nullptr);
    const auto written = [](double v) {
        return std::isfinite(v) && (v == 0.0 || std::fabs(v) >= static_cast<double>(std::numeric_limits<float>::min()));
    };
    // the host's: a timeline's rows, every number of the tel block (no columns asked), its first block a short one
    SimHost host;
    REQUIRE(host.attach(sim, offline(48000.0, 512), error));
    Build build(sim->fields());
    build.attach(&host);
    Timeline timeline;
    timeline.seconds = 0.05;
    timeline.block = 512;
    timeline.events.push_back({0.0, TimelineEvent::Kind::Control, "ignition", 2.0, {}, 0.0});
    timeline.events.push_back({0.001, TimelineEvent::Kind::Control, "mode", 1.0, {}, 0.0});
    RunResult run;
    REQUIRE(runTimeline(host, build, timeline, {}, run, error));
    REQUIRE(run.rowTime.size() >= 2);
    for (std::size_t c = 0; c < run.columns.size(); ++c)
    {
        INFO(run.columns[c] << " at the first frame: " << run.at(0, c));
        CHECK(written(run.at(0, c)));
    }
    // K3's EnginePlayer's: each telemetry field before and after its first call
    EnginePlayer::Options po;
    po.directory = simDir();
    auto player = EnginePlayer::create(po, error);
    REQUIRE(player != nullptr);
    for (std::size_t k = 0; k < player->telemetryNames().size(); ++k)
    {
        INFO(player->telemetryNames()[k] << " before the first call: " << player->telemetry(k));
        CHECK(written(player->telemetry(k)));
    }
}

TEST_CASE("the calibration baked in the background: the spark, the cam on that spark, the spark again; applied, both "
          "current; an edit makes both stale",
          "[engine][sim][bake][nightly]")
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
    if (build.fieldOf("ecu.maps.camInKey") < 0 || build.fieldOf("ecu.maps.knockLimitKey") < 0)
        SKIP("the program has no knock-limit or cam key yet");
    REQUIRE(build.get("valve.inPhaseRange") > 0.0);
    CalibrationBakeOptions options;   // small, for the test
    options.spark.speeds = 3;
    options.spark.pedals = {1.0};
    options.cam.step = 25.0;
    options.cam.predict.points = options.cam.predict.coarse = 3;
    options.debounceSeconds = 0.05;
    CalibrationBakeService bakes(sim, options);

    CHECK(bakes.consider(build) != KnockBakeState::Current);   // nothing baked: stale, asked
    CHECK(build.get("ecu.maps.knockLimitAuto") == 1.0);
    std::set<std::string> stages;
    bool applied = false;
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(300);
    while (!applied && std::chrono::steady_clock::now() < until)
    {
        REQUIRE(bakes.state() != KnockBakeState::Failed);
        if (const std::string st = bakes.stage(); !st.empty())
            stages.insert(st);
        applied = bakes.takeAndApply(build, error);
        INFO(error);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    REQUIRE(applied);
    CHECK(intakeCamCurrent(build, *sim));
    CHECK(knockLimitCurrent(build, *sim));
    CHECK(bakes.consider(build) == KnockBakeState::Current);   // spark, cam, spark: and it ends
    INFO("stages seen: " << stages.size());
    CHECK(stages.count("cam") == 1);

    // an edit of what moves both (the compression): both stale, both set aside, a bake asked
    build.set("engine.compression", build.get("engine.compression") - 0.5);
    CHECK(bakes.consider(build) != KnockBakeState::Current);
    CHECK(build.get("ecu.maps.knockLimitAuto") == 1.0);
    CHECK(build.get("ecu.maps.camInAuto") == 1.0);
}

TEST_CASE("the calibration bake's cache across launches: a table baked once is taken by its key, not measured again",
          "[engine][sim][bake]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    const auto sim = compiled(simDir(), {}, error);
    REQUIRE(sim != nullptr);
    Build build(sim->fields());
    std::vector<std::string> warnings;
    REQUIRE(build.loadFile(std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/archetypes/turbo-four-20-gdi.json",
                           BuildPart::Whole, warnings, error));
    if (build.fieldOf("ecu.maps.knockLimitAuto") < 0)
        SKIP("the program has no knock-limit table yet");
    // a folder of the test's own, emptied first (nothing written outside it)
    const std::filesystem::path cache = std::filesystem::path(TEE_SOURCE_DIR) / "out" / "tests" / "bake-cache";
    std::filesystem::remove_all(cache);
    CalibrationBakeOptions options;
    options.spark.speeds = 2;
    options.spark.pedals = {1.0};
    options.spark.boostSteps = 0;
    options.bakeCam = false;
    options.debounceSeconds = 0.0;
    options.cacheDirectory = cache;
    const auto bakeOnce = [&](Build& b, bool& measured) {
        CalibrationBakeService bakes(sim, options);
        bakes.consider(b);
        CalibrationBakeResult r;
        const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(120);
        while (!bakes.take(r) && std::chrono::steady_clock::now() < until)
        {
            REQUIRE(bakes.state() != KnockBakeState::Failed);
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        REQUIRE(r.hasSpark);
        measured = !r.spark.cached;
        return r;
    };
    Build first(sim->fields()), second(sim->fields());
    REQUIRE(first.load(build.save(), BuildPart::Whole, warnings, error));
    REQUIRE(second.load(build.save(), BuildPart::Whole, warnings, error));
    bool measured = false;
    const CalibrationBakeResult a = bakeOnce(first, measured);
    CHECK(measured);   // nothing kept yet: baked
    CHECK(std::filesystem::exists(cache / ("spark-" + std::to_string(a.spark.key) + ".json")));
    // another service (another launch), the same build: taken from the cache at once, the same values
    const auto t0 = std::chrono::steady_clock::now();
    const CalibrationBakeResult b = bakeOnce(second, measured);
    const double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    CHECK_FALSE(measured);
    CHECK(seconds < 1.0);
    CHECK(b.spark.key == a.spark.key);
    CHECK(b.spark.fields == a.spark.fields);
    CHECK(b.spark.stillKnocking == a.spark.stillKnocking);
    std::filesystem::remove_all(cache);
}

TEST_CASE("a prediction asked while a calibration bake runs goes first: its time within a stated factor of its time "
          "alone",
          "[engine][sim][bake][predict]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    const auto sim = compiled(simDir(), {}, error);
    REQUIRE(sim != nullptr);
    std::vector<std::string> warnings;
    Build econ(sim->fields());
    REQUIRE(econ.loadFile(std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/archetypes/economy-four-15.json",
                          BuildPart::Whole, warnings, error));
    Build turbo(sim->fields());
    REQUIRE(turbo.loadFile(std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/archetypes/turbo-four-20-gdi.json",
                           BuildPart::Whole, warnings, error));
    if (turbo.fieldOf("ecu.maps.knockLimitAuto") < 0)
        SKIP("the program has no knock-limit table yet");
    PredictOptions po;
    po.points = 4;
    po.coarse = 2;
    po.threads = 2;
    po.cacheSize = 0;   // every request measured
    const auto refinedAfter = [&](PredictService& predict, Build& b) {
        const auto t0 = std::chrono::steady_clock::now();
        predict.request(b);
        PredictCurve curve;
        const auto until = t0 + std::chrono::seconds(120);
        while (std::chrono::steady_clock::now() < until)
        {
            if (predict.take(curve) && curve.refined)
                break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        REQUIRE(curve.refined);
        return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    };
    PredictService predict(sim, po);
    const double alone = refinedAfter(predict, econ);
    econ.set("engine.compression", econ.get("engine.compression") - 0.1);   // another curve: not from the cache

    CalibrationBakeOptions bo;
    bo.debounceSeconds = 0.0;
    bo.bakeCam = false;
    bo.spark.predict.threads = 2;
    CalibrationBakeService bakes(sim, bo);
    bakes.yieldTo(predict);
    bakes.consider(turbo);
    const auto until = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (bakes.state() != KnockBakeState::Baking && std::chrono::steady_clock::now() < until)
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    REQUIRE(bakes.state() == KnockBakeState::Baking);
    std::this_thread::sleep_for(std::chrono::milliseconds(300));   // the bake's workers under way
    const double during = refinedAfter(predict, econ);
    INFO("the curve alone " << alone << " s, with a bake running " << during << " s");
    std::printf("predicted curve alone %.2f s, with a calibration bake running (yielding) %.2f s\n", alone, during);
    // the bake waits between its points: the curve waits at most for the points under way (some 0.3 s each)
    CHECK(during <= 2.5 * alone + 1.0);
    // the PredictService may go first (a reload that makes it again): the bake holds its flag, not the service
    {
        PredictService other(sim, po);
        bakes.yieldTo(other);
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));   // the bake, past points, reads a flag still valid
    CHECK(bakes.state() != KnockBakeState::Failed);
    bakes.stopYielding();
}

TEST_CASE("a slowly spooling turbo diesel's predicted points at its lug are its long hold's within 4 %",
          "[engine][sim][predict][dyno][nightly]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    // the tractor's VGT takes 6 to 8 s to arrive: its stepped points held 20 s. Not about 1060 rpm: there the turbo
    // spools or not by the path to it (held 30 s: 1544 N m at 1058 rpm, 1917 to 2023 at 1061), a knife's edge of
    // the engine's own, not a reading of the predictor's
    predictedMatchesSteps("tractor-15-inline-six.json", {115.19, 136.14}, 0.04, 20.0);   // 1100, 1300 rpm
}
