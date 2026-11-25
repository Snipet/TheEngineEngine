// tests/assistant/ToolsTest.cpp: the tools (assistant/.../Tools.h) on a real simulator, document and library, the
// frame run as tee-headless runs it: 60 frames a second on a simulated clock, each frame draining the inbox, then the
// adapter's frame(), then that frame's audio. The tools are called from a worker thread, as the MCP link and the chat
// call them. Properties, not examples: docs/plan/w3-a-detailed.md, "Proof".
#define TEE_REGISTER_SHARED_SIM
#include "Rig.h"
#include "tee/Predict.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace tee;
using namespace tee::assistant;
using nlohmann::json;
namespace fs = std::filesystem;

using namespace tee::test;

TEST_CASE("assistant: a batch of changes is one undo step, and undo puts every value back", "[assistant][sharedsim]")
{
    Rig rig("batch");
    const auto fields = rig.settable(3);
    REQUIRE(fields.size() == 3);
    std::vector<double> before;
    json changes = json::array();
    for (const auto& [f, v] : fields)
    {
        before.push_back(rig.build().get(f->path));
        changes.push_back({{"path", shortPath(*f)}, {"value", v}});
    }
    const std::uint64_t stepBefore = rig.build().undoStep();
    const Result r = rig.call("set_fields", {{"changes", changes}});
    INFO(r.text());
    REQUIRE_FALSE(r.isError);
    const std::uint64_t step = r.value["undoStep"].get<std::uint64_t>();
    CHECK(step != 0);
    CHECK(step != stepBefore);
    CHECK(rig.build().undoStep() == step);
    for (const auto& [f, v] : fields)
        CHECK((rig.build().get(f->path) == static_cast<double>(static_cast<float>(v)) || rig.build().get(f->path) == v));
    // one undo takes back the whole batch
    REQUIRE(rig.build().undo());
    for (std::size_t i = 0; i < fields.size(); ++i)
        CHECK(rig.build().get(fields[i].first->path) == before[i]);
    CHECK(rig.build().undoStep() == stepBefore);
    CHECK(rig.build().redoStep() == step);
    REQUIRE(rig.build().redo());
    CHECK(rig.build().undoStep() == step);
    // a step id is never reused
    rig.build().set(fields[0].first->path, before[0]);
    CHECK(rig.build().undoStep() > step);
}

TEST_CASE("assistant: a value outside its range is refused, the stored value returned, nothing changed",
          "[assistant][sharedsim]")
{
    Rig rig("range");
    const auto fields = rig.settable(2);
    REQUIRE(fields.size() == 2);
    const FieldInfo& good = *fields[0].first;
    const FieldInfo& f = *fields[1].first;
    const double stored = rig.build().get(f.path);
    const double goodStored = rig.build().get(good.path);
    const std::uint64_t step = rig.build().undoStep();
    const bool dirty = rig.file->dirty();
    for (const double v : {*f.maximum + std::abs(*f.maximum) * 0.5 + 1.0, *f.minimum - std::abs(*f.minimum) * 0.5 - 1.0})
    {
        // a good change beside the bad one: all or none
        const Result r = rig.call("set_fields", {{"changes", {{{"path", shortPath(good)}, {"value", fields[0].second}},
                                                              {{"path", shortPath(f)}, {"value", v}}}}});
        INFO(r.text());
        CHECK(r.isError);
        CHECK(r.value["error"].get<std::string>().find("outside its range") != std::string::npos);
        CHECK(r.value["stored"].get<double>() == stored);
        CHECK(r.value["min"].get<double>() == *f.minimum);
        CHECK(r.value["max"].get<double>() == *f.maximum);
        CHECK(rig.build().get(f.path) == stored);
        CHECK(rig.build().get(good.path) == goodStored);
        CHECK(rig.build().undoStep() == step);
        CHECK(rig.file->dirty() == dirty);
    }
    // not a number, a fraction for a whole number, a switch given 2, a choice not among its choices: refused the same
    const Result s = rig.call("set_fields", {{"changes", {{{"path", shortPath(f)}, {"value", "lots"}}}}});
    CHECK(s.isError);
    CHECK(s.value["stored"].get<double>() == stored);
    for (const FieldInfo& g : rig.build().fields().all())
        if (g.block == "spec" && !g.hidden && g.kind == FieldKind::Choice)
        {
            const Result c = rig.call("set_fields", {{"changes", {{{"path", shortPath(g)}, {"value", "no such choice"}}}}});
            CHECK(c.isError);
            CHECK(c.value.contains("choices"));
            break;
        }
    CHECK(rig.build().undoStep() == step);
}

TEST_CASE("assistant: hidden fields, revs, the controls and the telemetry are unreachable", "[assistant][sharedsim]")
{
    Rig rig("hidden");
    const Fields& fields = rig.build().fields();
    std::vector<std::string> unreachable;
    for (const FieldInfo& f : fields.all())
        if ((f.block == "spec" && (f.hidden || f.kind == FieldKind::Rev)) || f.block != "spec")
            unreachable.push_back(f.path);
    REQUIRE(unreachable.size() > 10);
    // a sample (each kind: a rev, a hidden working field, a control, a gauge, the scope) asked by both spellings
    std::vector<std::string> asked;
    for (const char* p : {"spec.engine.rev", "engine.rev", "ctl.throttle", "ctl.ignition", "tel.engine.speed", "rev"})
        asked.push_back(p);
    for (const std::string& p : unreachable)
        if (p.rfind("spec.", 0) == 0 && asked.size() < 12)
            asked.push_back(p), asked.push_back(p.substr(5));
    for (const std::string& p : unreachable)
        if (p.rfind("scope.", 0) == 0)
        {
            asked.push_back(p);
            break;
        }
    const std::uint64_t step = rig.build().undoStep();
    for (const std::string& p : asked)
    {
        INFO(p);
        const Result d = rig.call("describe_field", {{"path", p}});
        CHECK(d.isError);
        CHECK(d.value["error"].get<std::string>().find("has no field") != std::string::npos);
        const Result s = rig.call("set_fields", {{"changes", {{{"path", p}, {"value", 1}}}}});
        CHECK(s.isError);
        CHECK_FALSE(s.value.contains("stored"));   // nothing told about it
    }
    CHECK(rig.build().undoStep() == step);
    // and never listed: every section read whole names none of them
    const Result all = rig.call("get_build");
    REQUIRE_FALSE(all.isError);
    for (auto it = all.value["sections"].begin(); it != all.value["sections"].end(); ++it)
    {
        const Result sec = rig.call("get_build", {{"section", it.key()}});
        REQUIRE_FALSE(sec.isError);
        for (const json& e : sec.value["fields"])
        {
            const FieldInfo* f = fields.find("spec." + e["path"].get<std::string>());
            REQUIRE(f != nullptr);
            CHECK_FALSE(f->hidden);
            CHECK(f->kind != FieldKind::Rev);
        }
    }
    // the status's gauges are a fixed list read from the telemetry, never written
    const Result st = rig.call("get_status");
    REQUIRE_FALSE(st.isError);
    CHECK(st.value["engine"] == "playing");
    CHECK(st.value["gauges"].contains("engine.speed"));
}

TEST_CASE("assistant: a path is never accepted, and never returned", "[assistant][sharedsim]")
{
    Rig rig("paths");
    const std::string userDir = rig.user.string();
    for (const char* bad : {"../v8", "/etc/passwd", "builds/v8", "..\\v8", "C:\\v8", ".v8", "~/v8"})
    {
        INFO(bad);
        CHECK(rig.call("load_build", {{"name", bad}}).isError);
        CHECK(rig.call("save_build_as", {{"name", bad}}).isError);
    }
    // a tool that takes no path given one anyway, or an argument it does not know: refused before it runs
    CHECK(rig.call("list_builds", {{"directory", "/tmp"}}).isError);
    CHECK(rig.call("get_status", {{"file", "/tmp/x"}}).isError);
    CHECK(rig.call("no_such_tool").isError);
    // nothing in the user's folder was written by any of it
    CHECK(fs::is_empty(rig.user));
    // what the tools return across a session's work: no directory, no file name
    rig.call("get_status");
    rig.call("list_builds");
    rig.call("get_build");
    rig.call("get_build", {{"section", "engine"}});
    rig.call("load_build", {{"name", "v8"}});
    rig.call("save_build_as", {{"name", "assistant-paths"}});
    rig.call("save_build_as", {{"name", "assistant-paths"}});
    rig.call("load_build", {{"name", "no such build"}});
    rig.call("describe_field", {{"path", "engine.bore"}});
    for (const std::string& text : rig.replies)
    {
        INFO(text);
        CHECK(text.find(userDir) == std::string::npos);
        CHECK(text.find(TEE_SOURCE_DIR) == std::string::npos);
        CHECK(text.find("content/builds") == std::string::npos);
        CHECK(text.find(".json") == std::string::npos);
        CHECK(text.find("/home") == std::string::npos);
    }
    CHECK(fs::exists(rig.user / "assistant-paths.json"));
}

TEST_CASE("assistant: loading over unsaved changes asks the owner first", "[assistant][sharedsim]")
{
    Limits limits;
    limits.askSeconds = 0.3;
    Rig rig("ask", limits);
    // a clean document loads without a question
    REQUIRE_FALSE(rig.call("load_build", {{"name", "v8"}}).isError);
    CHECK(rig.questions.empty());
    CHECK_FALSE(rig.file->dirty());
    // made dirty, by the assistant itself
    const auto fields = rig.settable(1);
    REQUIRE_FALSE(rig.call("set_fields", {{"changes", {{{"path", shortPath(*fields[0].first)}, {"value", fields[0].second}}}}})
                      .isError);
    REQUIRE(rig.file->dirty());
    const std::uint64_t hash = rig.build().hash();
    const std::string name = rig.build().name;

    rig.answer = false;   // no
    Result r = rig.call("load_build", {{"name", "v10"}});
    CHECK(r.isError);
    CHECK(r.value["error"].get<std::string>().find("said no") != std::string::npos);
    CHECK(rig.questions.size() == 1);
    CHECK(rig.build().hash() == hash);
    CHECK(rig.build().name == name);

    rig.answer.reset();   // silence
    r = rig.call("load_build", {{"name", "v10"}});
    CHECK(r.isError);
    CHECK(r.value["error"].get<std::string>().find("did not answer") != std::string::npos);
    CHECK(rig.build().hash() == hash);

    rig.answer = true;    // yes, but the build is changed while the owner is asked
    const FieldInfo& f0 = *fields[0].first;
    rig.whileAsked = [&] { rig.build().set(f0.path, fields[0].second == *f0.maximum ? *f0.minimum : *f0.maximum); };
    r = rig.call("load_build", {{"name", "v10"}});
    CHECK(r.isError);
    CHECK(r.value["error"].get<std::string>().find("changed while") != std::string::npos);
    CHECK(rig.build().name == name);

    rig.whileAsked = nullptr;   // yes
    r = rig.call("load_build", {{"name", "v10"}});
    INFO(r.text());
    CHECK_FALSE(r.isError);
    CHECK(rig.questions.size() == 4);
    CHECK(rig.build().name != name);
    CHECK_FALSE(rig.file->dirty());
    // a part is not a whole build
    for (const files::BuildSummary& s : rig.library->list())
        CHECK(s.part.empty());
}

TEST_CASE("assistant: save-as refuses a name that is already a build's, and overwrites nothing", "[assistant][sharedsim]")
{
    Rig rig("saveas");
    REQUIRE_FALSE(rig.call("save_build_as", {{"name", "mine"}}).isError);
    const fs::path mine = rig.user / "mine.json";
    REQUIRE(fs::exists(mine));
    const std::string was = readAll(mine);
    // changed, then saved again over its own name, any case, or over a shipped build's: refused
    const auto fields = rig.settable(1);
    REQUIRE_FALSE(rig.call("set_fields", {{"changes", {{{"path", shortPath(*fields[0].first)}, {"value", fields[0].second}}}}})
                      .isError);
    for (const char* taken : {"mine", "MINE", "Mine", "v8", "default", "bike-cruiser"})
    {
        INFO(taken);
        const Result r = rig.call("save_build_as", {{"name", taken}});
        CHECK(r.isError);
        CHECK(r.value["error"].get<std::string>().find("exists") != std::string::npos);
    }
    CHECK(readAll(mine) == was);
    // the folder holds the one file, by its own name (fs::exists would not tell MINE.json from mine.json on a disk
    // that does not tell cases apart: macOS's)
    std::vector<std::string> names;
    for (const auto& e : fs::directory_iterator(rig.user))
        names.push_back(e.path().filename().string());
    CHECK(names == std::vector<std::string>{"mine.json"});
    // a name that differs only in case is the same name, on every system: refused above, and loaded by it here
    REQUIRE_FALSE(rig.call("save_build_as", {{"name", "assistant-other"}}).isError);
    const Result loaded = rig.call("load_build", {{"name", "MINE"}});
    INFO(loaded.text());
    REQUIRE_FALSE(loaded.isError);
    CHECK(loaded.value["loaded"] == "mine");   // the library's own spelling
    // a new name in anything but plain ASCII is refused (names must compare alike everywhere)
    const Result accented = rig.call("save_build_as", {{"name", "mi\xC3\xA9n"}});
    CHECK(accented.isError);
    CHECK(accented.value["error"].get<std::string>().find("ASCII") != std::string::npos);
    // no tool removes a build
    for (const ToolInfo& t : rig.tools->list())
    {
        CHECK(t.name.find("delete") == std::string::npos);
        CHECK(t.name.find("remove") == std::string::npos);
    }
}

TEST_CASE("assistant: a batch waits while a control is held, and never joins its gesture", "[assistant][sharedsim]")
{
    Rig rig("gesture");
    const auto fields = rig.settable(2);
    REQUIRE(fields.size() == 2);
    const FieldInfo& held = *fields[0].first;
    const FieldInfo& asked = *fields[1].first;
    // the owner holds a knob: a gesture open on the frame's thread
    rig.build().beginGesture();
    rig.build().set(held.path, fields[0].second);
    const double heldValue = rig.build().get(held.path);
    auto pending = std::async(std::launch::async, [&] {
        return rig.tools->call("set_fields", {{"changes", {{{"path", shortPath(asked)}, {"value", fields[1].second}}}}});
    });
    const double from = rig.clock;
    while (rig.clock - from < 1.0)
    {
        rig.frame();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    CHECK(pending.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready);
    CHECK(rig.build().get(asked.path) != fields[1].second);
    rig.build().endGesture();                       // the knob let go: its own step
    const std::uint64_t knob = rig.build().undoStep();
    while (pending.wait_for(std::chrono::milliseconds(0)) != std::future_status::ready)
        rig.frame();
    const Result r = pending.get();
    INFO(r.text());
    REQUIRE_FALSE(r.isError);
    CHECK(r.value["undoStep"].get<std::uint64_t>() != knob);
    // undo the assistant's step: the knob's change stays
    REQUIRE(rig.build().undo());
    CHECK(rig.build().get(held.path) == heldValue);
    CHECK(rig.build().undoStep() == knob);

    // held for longer than the batch waits: refused, nothing changed
    rig.build().beginGesture();
    const double stored = rig.build().get(asked.path);
    const Result late = rig.whileFrames([&] {
        return rig.tools->call("set_fields", {{"changes", {{{"path", shortPath(asked)}, {"value", fields[1].second}}}}});
    });
    rig.build().endGesture();
    CHECK(late.isError);
    CHECK(rig.build().get(asked.path) == stored);
}

TEST_CASE("assistant: a turn's limits hold, and one driver at a time", "[assistant][sharedsim]")
{
    Limits limits;
    limits.turnCalls = 3;
    limits.turnSeconds = 1.0;
    limits.resultBytes = 1024;
    Rig rig("limits", limits);
    Drivers drivers;
    const auto clock = [&] { return rig.clock; };
    {
        Turn turn(*rig.tools, drivers, Driver::Desktop, clock);
        REQUIRE(turn.active());
        for (int i = 0; i < 3; ++i)
            CHECK_FALSE(rig.whileFrames([&] { return turn.call("get_status"); }).isError);
        const Result fourth = rig.whileFrames([&] { return turn.call("get_status"); });
        CHECK(fourth.isError);
        CHECK(fourth.value["error"].get<std::string>().find("tool calls") != std::string::npos);
        // the chat, while Claude Desktop drives: refused at once
        Turn chat(*rig.tools, drivers, Driver::Chat, clock);
        CHECK_FALSE(chat.active());
        CHECK(chat.call("get_status").isError);
    }
    {
        // the frame's clock: a turn past its seconds is refused
        Turn turn(*rig.tools, drivers, Driver::Chat, clock);
        REQUIRE(turn.active());
        CHECK_FALSE(rig.whileFrames([&] { return turn.call("get_status"); }).isError);
        const double from = rig.clock;
        while (rig.clock - from < 1.1)
            rig.frame();
        const Result late = rig.whileFrames([&] { return turn.call("get_status"); });
        CHECK(late.isError);
        CHECK(late.value["error"].get<std::string>().find("out of time") != std::string::npos);
    }
    {
        // a result larger than its limit is cut, and says so; the turn's results add up
        Turn turn(*rig.tools, drivers, Driver::Chat, clock);
        const Result big = rig.whileFrames([&] { return turn.call("list_builds"); });
        CHECK(big.isError);
        CHECK(big.value["error"].get<std::string>().find("too large") != std::string::npos);
        CHECK(turn.bytes() > 0);
        CHECK(turn.bytes() < 1024);
    }
    CHECK_FALSE(drivers.held());
}

TEST_CASE("assistant: the curves run on a private copy, and the document and the engine playing are untouched",
          "[assistant][sharedsim]")
{
    Rig rig("curves");
    const std::uint64_t hash = rig.build().hash();
    const std::uint64_t step = rig.build().undoStep();
    const Result r = rig.call("predict_curve");
    INFO(r.text());
    REQUIRE_FALSE(r.isError);
    CHECK(r.value["points"].size() >= 4);
    CHECK(r.value["peakTorque"]["Nm"].get<double>() > 50.0);
    CHECK(r.value["peakPower"]["kW"].get<double>() > 20.0);
    CHECK(rig.build().hash() == hash);
    CHECK(rig.build().undoStep() == step);
    // cancelled: answers at once, nothing made
    std::atomic<bool> cancel{true};
    const Result c = rig.whileFrames([&] { return rig.tools->call("predict_curve", json::object(), &cancel); });
    CHECK(c.isError);
    // the dyno's plan is bounded before it runs
    CHECK(rig.call("run_dyno", {{"from_rpm", 3000}, {"to_rpm", 2000}}).isError);
    CHECK(rig.call("run_dyno", {{"from_rpm", 100}, {"to_rpm", 2000}}).isError);
    CHECK(rig.call("run_dyno", {{"from_rpm", 1000}, {"to_rpm", 90000}}).isError);
}

TEST_CASE("assistant: the curves yield to the app's own: never with the ribbon's or the bake's, never deadlocked",
          "[assistant][sharedsim]")
{
    Rig rig("yield");
    // the ribbon's predictor, a real one, asked first: the assistant's curve waits for it
    PredictService ribbon(rig.sim->host().sim());
    rig.busyToo = [&] { return ribbon.busy(); };
    ribbon.request(rig.build());
    while (!ribbon.busy())
        std::this_thread::yield();
    std::uint64_t pointsWhileRibbon = 0;
    bool sawRibbon = false;
    // and the bake, as its flag: raised once the assistant's curve has begun, for a while, then let go
    bool raised = false, lowered = false;
    std::uint64_t atRaise = 0, atLower = 0;
    const Result r = rig.whileFrames([&] {
        sawRibbon = ribbon.busy();   // (read here, not only between polls: the ribbon may end inside the first)
        auto f = std::async(std::launch::async, [&] { return rig.tools->call("predict_curve"); });
        while (f.wait_for(std::chrono::milliseconds(5)) != std::future_status::ready)
        {
            if (ribbon.busy())
            {
                sawRibbon = true;
                pointsWhileRibbon = std::max(pointsWhileRibbon, rig.tools->pointsRun());
            }
            else if (!raised && rig.tools->pointsRun() > 0)
            {
                raised = true;
                atRaise = rig.tools->pointsRun();
                rig.othersBusy = true;
            }
            else if (raised && !lowered)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(800));
                atLower = rig.tools->pointsRun();
                lowered = true;
                rig.othersBusy = false;
            }
        }
        return f.get();
    });
    INFO(r.text());
    CHECK(sawRibbon);
    CHECK(pointsWhileRibbon == 0);          // nothing of the assistant's ran beside the ribbon's curve
    REQUIRE(raised);
    REQUIRE(lowered);
    CHECK(atLower - atRaise <= 3);          // at most the points already in flight (one a worker) when the bake began
    CHECK_FALSE(r.isError);                 // and it finished once the bake let go: no deadlock
    CHECK_FALSE(rig.tools->curveRunning());
    rig.busyToo = nullptr;

    // the app busy longer than the assistant may wait: refused, nothing run
    Limits quick;
    quick.predictSeconds = 0.5;
    Rig late("yield-late", quick);
    late.othersBusy = true;
    const Result busy = late.call("predict_curve");
    CHECK(busy.isError);
    CHECK(busy.value["error"].get<std::string>().find("busy") != std::string::npos);
    CHECK(late.tools->pointsRun() == 0);
}
