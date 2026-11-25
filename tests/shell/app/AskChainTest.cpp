// tests/shell/app/AskChainTest.cpp: the whole chain of a load the assistant asks for over unsaved work, through the
// application's own panel (app::makePanel over the simulator, as tee-headless makes it): the assistant's load_build
// (card A's tool, on a worker thread, as Claude's calls run) posts to the Session, the frame's tick drains it, the
// frame's dialog asks the owner (U1's AppFrame::ask: "Load" or "Keep"), a click answers, and the document is loaded or
// kept. Card A's assertions (25 Nov): LOAD loads the build asked for and leaves it clean; KEEP refuses with "said no"
// and leaves the document exactly as it was; the frame going while the question is open answers no. No reply carries
// a file's path. Each wait ends on the state it waits for (the dialog open, or the call over: a reply with no question
// is a fault, printed); kGuard only stops a hang, and at it the case prints what the call, the frame and the document
// saw, so a red explains itself (the first macOS red, f1f60ab8: the dialog not open within the old 10 s, nothing said).
#include "Engine.h"
#include "Offline.h"
#include "Options.h"

#include "files/BuildFiles.h"
#include "input/Controls.h"
#include "player/SimAdapter.h"

#include "app/AppFrame.h"
#include "tee/assistant/Session.h"
#include "tee/assistant/Tools.h"

#include <funkgui/panel/HeadlessHost.h>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <future>
#include <string>
#include <thread>

using namespace tee;
namespace fs = std::filesystem;

namespace
{
    using Clock = std::chrono::steady_clock;
    constexpr std::chrono::seconds kGuard{ 120 };   // a hang's stop, as the engine's own compile wait (Rig)

    bool clickOn(funkgui::HeadlessHost& host, const std::string& title)
    {
        for (const funkgui::A11yItem& it : host.accessibility())
            if (it.title == title && it.role == funkgui::A11yRole::button && !it.bounds.isEmpty())
            {
                host.click(it.bounds.centreX(), it.bounds.centreY());
                host.tick(20);
                return true;
            }
        return false;
    }

    // The application's panel over the simulator, and what a test drives it through.
    struct Rig
    {
        app::Options options;
        app::Engine engine;
        std::unique_ptr<input::Controls> controls;
        app::Shown shown;
        std::unique_ptr<funkgui::HeadlessHost> host;

        Rig()
        {
            options.engine = "sim";
            options.sim = TEE_SOURCE_SIM_DIR;
            engine = app::loadEngine(options, 48000.0);
            REQUIRE(engine.error.empty());
            REQUIRE(engine.sim != nullptr);
            REQUIRE(engine.sim->waitReady(120.0));
            controls = std::make_unique<input::Controls>(*engine.player, *engine.player);
            shown = app::makePanel(options, engine, *controls, false);
            REQUIRE(shown.problem.empty());
            REQUIRE(shown.assistant != nullptr);
            REQUIRE(shown.document != nullptr);
            host = std::make_unique<funkgui::HeadlessHost>(*shown.panel, 0, 1.0f);
            host->tick(2);
        }

        ui::AppFrame* frame() const { return dynamic_cast<ui::AppFrame*>(shown.panel.get()); }

        // One frame of the application: the engine, the assistant's inbox and the frame's tick.
        void tick()
        {
            engine.frame();
            if (shown.frame)
                shown.frame();
            if (host)
                host->tick(1);
        }

        // A tool called on a worker thread (as Claude's run), the frame ticked meanwhile; `whileWaiting` each frame
        // (true: stop ticking and leave the call running). The result, or none when the bound passed.
        std::future<assistant::Result> call(const std::string& tool, const nlohmann::json& args)
        {
            assistant::Session* s = shown.assistant;
            return std::async(std::launch::async, [s, tool, args] { return s->tools().call(tool, args); });
        }

        template <class Done>
        bool tickUntil(Done done)
        {
            const auto start = Clock::now();
            while (!done())
            {
                if (Clock::now() - start > kGuard)
                    return false;
                tick();
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            return true;
        }

        // A load over unsaved work, waited for until its question is open or the call is over. True: asked. False:
        // `seen` says what each part saw (the call's reply, if any; the frame; the document; the engine; the time).
        bool waitAsked(std::future<assistant::Result>& f, std::string& seen)
        {
            ui::AppFrame* fr = frame();
            const auto start = Clock::now();
            int frames = 0;
            const auto over = [&] { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; };
            while (!fr->askOpen() && !over() && Clock::now() - start < kGuard)
            {
                tick();
                ++frames;
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            if (fr->askOpen())
                return true;
            const double secs = std::chrono::duration<double>(Clock::now() - start).count();
            seen = "no question after " + std::to_string(secs) + " s, " + std::to_string(frames) + " frames; the call: ";
            if (over())
            {
                const assistant::Result r = f.get();
                seen += std::string(r.isError ? "an error " : "a reply ") + r.value.dump();
            }
            else
                seen += "still running";
            seen += "; the frame: its ask dialog " + std::string(fr->askDialog() != nullptr ? "made" : "none")
                    + (fr->keyDialogOpen() ? ", the key dialog open (a question waits behind it)" : "");
            seen += "; the document: " + shown.document->file().stem().string()
                    + (shown.document->dirty() ? ", unsaved changes" : ", clean");
            seen += "; the engine: " + std::string(engine.sim->ready() ? "ready" : "compiling");
            return false;
        }

        assistant::Result finish(std::future<assistant::Result>& f)
        {
            const bool ready = tickUntil([&] { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; });
            REQUIRE(ready);
            return f.get();
        }

        // The document made dirty, as an edit makes it.
        void dirty()
        {
            tee::Build& b = shown.document->build();
            REQUIRE(b.set("engine.compression", b.get("engine.compression") + 0.5));
            REQUIRE(shown.document->dirty());
        }
    };

    bool noPath(const nlohmann::json& v)
    {
        const std::string text = v.dump();
        return text.find(".json") == std::string::npos && text.find(TEE_SOURCE_DIR) == std::string::npos;
    }
}

TEST_CASE("app: the assistant's load over unsaved work asks through the frame's dialog: Load loads, Keep keeps, gone is no",
          "[shell][app][assistant]")
{
    Rig rig;
    ui::AppFrame* f = rig.frame();
    REQUIRE(f != nullptr);
    files::BuildFile& doc = *rig.shown.document;

    // a clean build loads with no question
    {
        auto clean = rig.call("load_build", { { "name", "v8" } });
        const assistant::Result r = rig.finish(clean);
        INFO(r.value.dump());
        REQUIRE_FALSE(r.isError);
        CHECK_FALSE(f->askOpen());
        CHECK(doc.file().stem() == "v8");
        CHECK(noPath(r.value));
    }

    // LOAD: asked, held until the press, then loaded and clean
    rig.dirty();
    const fs::path fileBefore = doc.file();
    const std::uint64_t hashBefore = doc.build().hash();
    auto load = rig.call("load_build", { { "name", "v10" } });
    {
        std::string seen;
        const bool asked = rig.waitAsked(load, seen);
        INFO(seen);
        REQUIRE(asked);
    }
    CHECK(f->askDialog()->question().rfind("Claude asks to load \"v10\"", 0) == 0);
    CHECK(f->askDialog()->question().find("unsaved changes") != std::string::npos);
    CHECK_FALSE(f->askDialog()->focusOnYes());
    CHECK(load.wait_for(std::chrono::seconds(0)) != std::future_status::ready);
    CHECK(doc.build().hash() == hashBefore);
    CHECK(doc.dirty());
    CHECK(doc.file() == fileBefore);
    REQUIRE(clickOn(*rig.host, "Load"));
    {
        const assistant::Result r = rig.finish(load);
        INFO(r.value.dump());
        CHECK_FALSE(r.isError);
        CHECK(r.value.value("loaded", std::string()) == "v10");
        CHECK(r.value.contains("undoStep"));
        CHECK(noPath(r.value));
    }
    CHECK(doc.file().stem() == "v10");
    CHECK_FALSE(doc.dirty());
    CHECK(doc.build().hash() != hashBefore);
    CHECK_FALSE(f->askOpen());

    // KEEP: asked, refused with "said no", the document exactly as it was
    rig.dirty();
    const fs::path keptFile = doc.file();
    const std::uint64_t keptHash = doc.build().hash();
    auto keep = rig.call("load_build", { { "name", "v8" } });
    {
        std::string seen;
        const bool asked = rig.waitAsked(keep, seen);
        INFO(seen);
        REQUIRE(asked);
    }
    REQUIRE(clickOn(*rig.host, "Keep"));
    {
        const assistant::Result r = rig.finish(keep);
        INFO(r.value.dump());
        CHECK(r.isError);
        CHECK(r.value.value("error", std::string()).find("said no") != std::string::npos);
        CHECK(noPath(r.value));
    }
    CHECK(doc.file() == keptFile);
    CHECK(doc.build().hash() == keptHash);
    CHECK(doc.dirty());
    CHECK_FALSE(f->askOpen());


    // THE FRAME GOING while the question is open answers no, the document unchanged (one engine's compile for the
    // three passes: this last, since the frame is gone after it)
    const fs::path file = doc.file();
    const std::uint64_t hash = doc.build().hash();
    auto ask = rig.call("load_build", { { "name", "v10" } });
    {
        std::string seen;
        const bool asked = rig.waitAsked(ask, seen);
        INFO(seen);
        REQUIRE(asked);
    }
    // the frame goes (as the window closing makes it go): the Panel first, its asker cleared
    if (rig.shown.panelGoing)
        rig.shown.panelGoing();
    rig.host.reset();
    rig.shown.panel.reset();
    const auto start = Clock::now();
    while (ask.wait_for(std::chrono::milliseconds(5)) != std::future_status::ready && Clock::now() - start < kGuard)
    {
        rig.engine.frame();
        if (rig.shown.frame)
            rig.shown.frame();
    }
    REQUIRE(ask.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
    const assistant::Result gone = ask.get();
    INFO(gone.value.dump());
    CHECK(gone.isError);
    CHECK(doc.file() == file);
    CHECK(doc.build().hash() == hash);
    CHECK(doc.dirty());
}

// The hook that stops the questions moves with the Panel: a Shown moved from never calls it. The chain above was red on
// macOS alone (c06258e5: "the call: an error {"error":"the owner said no; nothing was loaded"}" within one frame): the
// rig's `shown = makePanel(...)` left a moved-from Shown whose std::function libc++ keeps, and its destructor cleared
// the live frame's asker. libstdc++ and MSVC empty a moved-from std::function, so this case was green there either way;
// on macOS it is the line between the two.
TEST_CASE("app: a moved-from Shown never calls the panel's going hook; a move-assignment calls the replaced one",
          "[shell][app]")
{
    int calls = 0;
    {
        app::Shown made;
        made.panelGoing = [&calls] { ++calls; };
        app::Shown kept(std::move(made));
        CHECK_FALSE(static_cast<bool>(made.panelGoing));
        app::Shown assigned;
        assigned = std::move(kept);                 // over a Shown with no hook: nothing called
        CHECK(calls == 0);
        CHECK_FALSE(static_cast<bool>(kept.panelGoing));
        CHECK(static_cast<bool>(assigned.panelGoing));

        app::Shown other;
        other.panelGoing = [&calls] { calls += 10; };
        assigned = std::move(other);                // the replaced Panel's hook runs, once
        CHECK(calls == 1);
    }
    CHECK(calls == 11);                             // the last one, as its Shown went; the moved-from ones none
}
