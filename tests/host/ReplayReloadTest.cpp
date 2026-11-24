// tests/host/ReplayReloadTest.cpp: a session log played again gives the same samples (card C, section 9); a reload
// carries a running engine's state across, by field name and type (section 4).
#include "Support.h"

#include "tee/Reload.h"
#include "tee/Replay.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <cstring>
#include <thread>

using namespace tee;
using namespace tee::test;

TEST_CASE("a replayed log's samples equal the original's", "[replay]")
{
    const auto sim = standinSim();
    LoadOptions options = offline(48000.0, 256);
    options.log = true;
    SimHost host;
    std::string error;
    REQUIRE(host.attach(sim, options, error));
    Build build(host.fields());
    build.attach(&host);

    // a session: a person starts it, revs it, turns knobs and draws a curve as it plays
    Timeline t;
    t.rate = 48000.0;
    t.block = 256;
    t.seconds = 3.0;
    t.events = {
        {0.0, TimelineEvent::Kind::Control, "ignition", 2, {}, 0.0},
        {0.7, TimelineEvent::Kind::Control, "ignition", 1, {}, 0.0},
        {1.0, TimelineEvent::Kind::Control, "throttle", 0.8, {}, 0.3},
        {1.4, TimelineEvent::Kind::Spec, "sound.level", 0.7, {}, 0.0},
        {1.9, TimelineEvent::Kind::Spec, "engine.idle", 120, {}, 0.4},
        {2.2, TimelineEvent::Kind::Press, "shiftUp", 1, {}, 0.0},
        {2.5, TimelineEvent::Kind::Control, "throttle", 0.0, {}, 0.0},
    };
    RunResult original;
    REQUIRE(runTimeline(host, build, t, {}, original, error));
    build.setCurve("engine.torqueShape", {0, 500}, {1, 0.5});   // between two blocks, after the timeline
    std::vector<float> tailL(1000), tailR(1000);
    host.process(tailL.data(), tailR.data(), 1000);
    const std::int64_t samples = static_cast<std::int64_t>(original.left.size()) + 1000;
    SessionLog log = takeSession(host, samples);
    CHECK(log.events.size() > 20);

    // through JSON and back, then through a fresh host in blocks of another size
    SessionLog again;
    REQUIRE(SessionLog::parse(log.json(), again, error));
    SimHost fresh;
    REQUIRE(fresh.attach(sim, offline(48000.0, 333), error));
    std::vector<float> left, right;
    REQUIRE(replay(fresh, again, 333, left, right, error));
    REQUIRE(left.size() == static_cast<std::size_t>(samples));
    const bool same = std::memcmp(left.data(), original.left.data(), original.left.size() * sizeof(float)) == 0
                      && std::memcmp(right.data(), original.right.data(), original.right.size() * sizeof(float)) == 0
                      && std::memcmp(left.data() + original.left.size(), tailL.data(), 1000 * sizeof(float)) == 0;
    CHECK(same);
    CHECK(measure::peak(left) > 0.01);
}

namespace
{
    std::shared_ptr<Sim> edited(Program::Files files, std::string& error)
    {
        return Sim::compileFiles(std::move(files), {}, error);
    }
} // namespace

TEST_CASE("a reload carries a running engine's speed across and crossfades", "[reload]")
{
    std::string error;
    auto first = edited(standinFiles(), error);
    REQUIRE(first != nullptr);
    SimHost host;
    REQUIRE(host.attach(first, offline(), error));
    Build build(host.fields());
    build.set("engine.idle", 100);
    build.attach(&host);
    SimControls c;
    c.ignition = 2;
    c.throttle = 0.3f;
    host.setControls(c);
    std::vector<float> l(512), r(512);
    for (int i = 0; i < 150; ++i)
    {
        host.process(l.data(), r.data(), 512);
        host.service();
    }
    TelemetryFrame t;
    REQUIRE(host.telemetry(t));
    const double before = tel(host, t, "tel.engine.speed");
    REQUIRE(before > 100.0);

    // the edit: a new state field, a new Spec field, the harmonics' array resized (started afresh), the sound halved
    Program::Files files = standinFiles();
    files["engine.fm"] = replaced(files["engine.fm"], "    float heard;", "    float added;\n    float heard;");
    files["engine.fm"] = replaced(files["engine.fm"], "const float out = loud * s;", "const float out = 0.5f * loud * s;");
    files["spec.fm"] = replaced(files["spec.fm"], "[[ui::quantity(\"fraction\")]] [4];", "[[ui::quantity(\"fraction\")]] [5];");
    files["spec.fm"] = replaced(files["spec.fm"], "    int tap [[", "    float extra [[fminus::range(0, 1)]] [[fminus::default(0.25)]] [[ui::label(\"Extra\")]] [[ui::quantity(\"fraction\")]];\n    int tap [[");
    files["engine.fm"] = replaced(files["engine.fm"], "for (int k = 0; k < 4; ++k)", "for (int k = 0; k < 4; ++k) // five now, the fifth silent\n           ");
    auto second = edited(files, error);
    INFO(error);
    REQUIRE(second != nullptr);
    REQUIRE(host.reload(second, error));   // nobody plays meanwhile: the host takes the snapshot itself
    build.rebind(host.fields());
    host.process(l.data(), r.data(), 512);
    host.service();
    REQUIRE(host.telemetry(t));
    CHECK(t.generation == second->fields().generation());
    const double after = tel(host, t, "tel.engine.speed");
    CHECK(std::fabs(after - before) < 0.05 * before);
    CHECK(tel(host, t, "tel.engine.idle") == 100.0);                 // the build's value, sent again
    CHECK(tel(host, t, "tel.probe.calls") > 150);                    // the state carried
    CHECK(tel(host, t, "tel.probe.engineRev") > 1);                  // every section bumped: the simulator reapplies
    CHECK(t.host.reloads == 1);
    CHECK(build.get("sound.extra") == 0.25);                         // a new field at its default
    CHECK(build.get("sound.harmonics", 4) == 0.5);
    CHECK(measure::finite(l));
}

TEST_CASE("a reload while a made-up audio thread plays: the audio thread takes the snapshot", "[reload][threads]")
{
    std::string error;
    auto first = edited(standinFiles(), error);
    REQUIRE(first != nullptr);
    LoadOptions options = offline(48000.0, 256);
    options.worker = true;
    SimHost host;
    REQUIRE(host.attach(first, options, error));
    SimControls c;
    c.ignition = 2;
    c.throttle = 0.5f;
    host.setControls(c);
    std::atomic<bool> done{false};
    std::thread audio([&] {
        std::vector<float> l(256), r(256);
        while (!done)
        {
            host.process(l.data(), r.data(), 256);
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    TelemetryFrame t;
    REQUIRE(host.telemetry(t));
    const double calls = tel(host, t, "tel.probe.calls");
    auto second = edited(standinFiles(), error);
    REQUIRE(second != nullptr);
    REQUIRE(host.reload(second, error));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    REQUIRE(host.telemetry(t));
    done = true;
    audio.join();
    CHECK(t.generation == second->fields().generation());
    CHECK(tel(host, t, "tel.probe.calls") > calls);
    CHECK(tel(host, t, "tel.engine.speed") > 50.0);
}

TEST_CASE("the reloader compiles an edited directory and reports clang's words for a broken one", "[reload]")
{
    const std::filesystem::path dir = std::filesystem::path(TEE_SOURCE_DIR) / "out" / "tests" / "reload";
    std::filesystem::remove_all(dir);
    std::filesystem::create_directories(dir);
    for (const auto& entry : std::filesystem::directory_iterator(standin()))
        std::filesystem::copy_file(entry.path(), dir / entry.path().filename());
    SimHost host;
    std::string error;
    REQUIRE(host.load(dir, offline(), error));
    Reloader reloader(dir, {});
    const auto edit = [&](const std::string& from, const std::string& to) {
        std::ifstream in(dir / "engine.fm");
        std::ostringstream text;
        text << in.rdbuf();
        in.close();
        std::ofstream(dir / "engine.fm", std::ios::binary) << replaced(text.str(), from, to);
    };
    const auto waitFor = [&](int n) {
        for (int i = 0; i < 400 && reloader.compiles() < n; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        return reloader.compiles() >= n;
    };
    edit("const float out = loud * s;", "const float out = loud * s * 0.9f;");
    REQUIRE(waitFor(1));
    CHECK(reloader.poll(host));
    CHECK(reloader.errors().empty());
    edit("const float out = loud * s * 0.9f;", "const float out = loud * s * undefinedThing;");
    REQUIRE(waitFor(2));
    CHECK_FALSE(reloader.poll(host));
    CHECK(reloader.errors().find("undefinedThing") != std::string::npos);
    CHECK(reloader.errors().find("error:") != std::string::npos);
}
