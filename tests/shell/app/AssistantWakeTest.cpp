// tests/shell/app/AssistantWakeTest.cpp: the assistant's wake (app/AssistantWake.h) as the window serves it, with no
// window: a post pushes one SDL event (at most one pending), the event taken lets the next post wake again, a drain that
// left jobs asks again a little later (never at once), and over the application's own panel a tool's call is answered by
// the wake's drains alone, no frame ticked (a minimised window: DesktopHost runs no tick, so shown.frame never runs), and
// a job that says "not yet" is drained again by the wake's timer alone.
#include "AssistantWake.h"
#include "Engine.h"
#include "Offline.h"
#include "Options.h"

#include "input/Controls.h"
#include "player/SimAdapter.h"

#include "tee/Build.h"
#include "tee/Fields.h"
#include "tee/SimHost.h"
#include "tee/assistant/Session.h"
#include "tee/assistant/Tools.h"

#include <SDL3/SDL.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <future>
#include <memory>
#include <string>
#include <thread>

using namespace tee;

namespace
{
    using Clock = std::chrono::steady_clock;

    struct Events
    {
        Events() { REQUIRE(SDL_InitSubSystem(SDL_INIT_EVENTS)); }
        ~Events() { SDL_QuitSubSystem(SDL_INIT_EVENTS); }
    };

    int queued(Uint32 type)
    {
        SDL_PumpEvents();
        return SDL_PeepEvents(nullptr, 0, SDL_PEEKEVENT, type, type);
    }

    // The next event of `type` within `ms`, taken off the queue; false: none came.
    bool next(Uint32 type, int ms, SDL_Event& e)
    {
        const auto until = Clock::now() + std::chrono::milliseconds(ms);
        while (Clock::now() < until)
        {
            SDL_PumpEvents();
            if (SDL_PeepEvents(&e, 1, SDL_GETEVENT, type, type) == 1)
                return true;
            SDL_Delay(1);
        }
        return false;
    }
}

TEST_CASE("the assistant's wake: one event pending, taken, and again a little later", "[app][assistant]")
{
    Events events;
    app::AssistantWake wake;
    REQUIRE(wake.usable());
    SDL_FlushEvent(wake.type());

    const std::function<void()> post = wake.wake();
    post();
    post();
    post();
    CHECK(queued(wake.type()) == 1);   // three posts, one event: at most one pending
    SDL_Event e{};
    REQUIRE(next(wake.type(), 100, e));
    CHECK(wake.take(e));
    SDL_Event other{};
    other.type = SDL_EVENT_KEY_DOWN;
    CHECK_FALSE(wake.take(other));
    post();                            // taken: the next post wakes again
    CHECK(queued(wake.type()) == 1);
    REQUIRE(next(wake.type(), 100, e));
    CHECK(wake.take(e));

    // again(): not at once, about kAgainMs on, one timer however often asked
    const auto asked = Clock::now();
    wake.again();
    wake.again();
    CHECK(wake.againPending());
    CHECK(queued(wake.type()) == 0);
    REQUIRE(next(wake.type(), 2000, e));
    const double ms = std::chrono::duration<double, std::milli>(Clock::now() - asked).count();
    CHECK(ms >= 50.0);
    CHECK(wake.take(e));
    CHECK_FALSE(wake.againPending());
    CHECK_FALSE(next(wake.type(), 200, e));   // one timer: one event

    // a timer still pending when the wake goes fires into the state it holds, not into a freed wake
    {
        app::AssistantWake gone;
        gone.again();
    }
    SDL_Delay(150);
}

namespace
{
    // The application's panel over the simulator (as tee-headless makes it) and the wake set on its assistant; the
    // window's loop, minimised, is serve(): events only, as app/Main.cpp's rawEvent has them, never shown.frame.
    struct Minimised
    {
        app::Options options;
        app::Engine engine;
        std::unique_ptr<input::Controls> controls;
        app::Shown shown;
        std::unique_ptr<Events> events;
        std::unique_ptr<app::AssistantWake> wake;
        int drains = 0;

        // `adopted` false: the simulator's compile is waited for but not adopted (a window minimised before its first
        // frame after the compile: only a drain can adopt it)
        explicit Minimised(bool adopted = true)
        {
            options.engine = "sim";
            options.sim = TEE_SOURCE_SIM_DIR;
            options.background = true;   // compiled on a worker, as the window's application has it
            engine = app::loadEngine(options, 48000.0);
            REQUIRE(engine.error.empty());
            REQUIRE(engine.sim != nullptr);
            if (adopted)
                REQUIRE(engine.sim->waitReady(120.0));
            else
            {
                const auto until = Clock::now() + std::chrono::seconds(120);
                while (!engine.sim->compiled() && Clock::now() < until)
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                REQUIRE(engine.sim->compiled());
                REQUIRE_FALSE(engine.sim->ready());
            }
            controls = std::make_unique<input::Controls>(*engine.player, *engine.player);
            shown = app::makePanel(options, engine, *controls, false);
            REQUIRE(shown.problem.empty());
            REQUIRE(shown.assistant != nullptr);
            REQUIRE(shown.drainAssistant);
            events = std::make_unique<Events>();
            wake = std::make_unique<app::AssistantWake>();
            REQUIRE(wake->usable());
            SDL_FlushEvent(wake->type());
            shown.assistant->setWake(wake->wake());
        }
        ~Minimised()
        {
            if (shown.assistant != nullptr)
                shown.assistant->setWake({});
        }

        std::future<assistant::Result> call(const std::string& tool, nlohmann::json args)
        {
            assistant::Session* s = shown.assistant;
            return std::async(std::launch::async, [s, tool, args] { return s->tools().call(tool, args); });
        }

        // The loop for `ms`, or until `f` is answered.
        void serve(std::future<assistant::Result>& f, int ms)
        {
            const auto until = Clock::now() + std::chrono::milliseconds(ms);
            while (f.wait_for(std::chrono::seconds(0)) != std::future_status::ready && Clock::now() < until)
            {
                SDL_Event e{};
                if (!SDL_WaitEventTimeout(&e, 20) || !wake->take(e))
                    continue;
                shown.drainAssistant();
                ++drains;
                if (shown.assistant->waiting() > 0)
                    wake->again();
            }
        }
    };

    bool answered(std::future<assistant::Result>& f)
    {
        return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
    }
}

TEST_CASE("the assistant's wake: a tool's call answered with no frame ticked", "[app][assistant]")
{
    Minimised m;
    auto call = m.call("get_build", nlohmann::json::object());
    m.serve(call, 30000);
    REQUIRE(answered(call));
    const assistant::Result r = call.get();
    INFO(r.value.dump());
    CHECK_FALSE(r.isError);
    CHECK(m.drains >= 1);
}

// A job that says "not yet" (set_fields while a control is held: Tools.cpp's gesture wait) is drained again by the
// wake's own timer (again(), kAgainMs on), with nothing posted after the first: the window minimised, no frame. The
// frames clock does not move while no frame ticks, so the batch waits for the control, not for its limit.
TEST_CASE("the assistant's wake: a job that says not yet is drained again, no frame ticked", "[app][assistant]")
{
    Minimised m;
    Build& build = m.engine.sim->build();
    const FieldInfo* field = nullptr;
    double want = 0.0;
    for (const FieldInfo& f : build.fields().all())
        if (f.block == "spec" && !f.hidden && f.kind == FieldKind::Number && f.hasRange() && *f.maximum > *f.minimum
            && f.section >= 0 && f.path.rfind("spec.engine.", 0) == 0)
        {
            const double now = build.get(f.path);
            want = now < (*f.minimum + *f.maximum) / 2.0 ? *f.maximum : *f.minimum;
            field = &f;
            break;
        }
    REQUIRE(field != nullptr);

    build.beginGesture();   // the owner holds a knob
    auto call = m.call("set_fields", { { "changes", { { { "path", field->path.substr(5) }, { "value", want } } } } });
    m.serve(call, 400);
    CHECK_FALSE(answered(call));
    CHECK(m.drains >= 3);   // the first wake, then the timer's: about 5 in 400 ms
    build.endGesture();     // let go: only the timer can drain it now (nothing posts)
    m.serve(call, 5000);
    REQUIRE(answered(call));
    const assistant::Result r = call.get();
    INFO(r.value.dump());
    CHECK_FALSE(r.isError);
    CHECK(build.get(field->path) == want);
}

// X-066: a window that does not tick never ran the engine's frame, so its simulator was never adopted and a tool's change
// never reached it until the window came back. The drain runs the engine's frame around the jobs: no frame ticked here.
TEST_CASE("the assistant's wake: the simulator adopted through drains alone, no frame ticked", "[app][assistant]")
{
    Minimised m(/*adopted*/ false);
    auto call = m.call("get_build", nlohmann::json::object());
    m.serve(call, 30000);
    REQUIRE(answered(call));
    CHECK_FALSE(call.get().isError);
    CHECK(m.engine.sim->ready());
}

// The change made before the simulator is adopted: only the drain's frame (adopting it, the document attached and sent)
// puts it in the engine; once adopted, a set goes straight to the host, and the frame after the jobs sends what it held.
TEST_CASE("the assistant's wake: a tool's change reaches the engine, no frame ticked", "[app][assistant]")
{
    Minimised m(/*adopted*/ false);
    Build& build = m.engine.sim->build();
    const FieldInfo* field = nullptr;
    double want = 0.0;
    for (const FieldInfo& f : build.fields().all())
        if (f.block == "spec" && !f.hidden && f.kind == FieldKind::Number && f.hasRange() && *f.maximum > *f.minimum
            && f.section >= 0 && f.path.rfind("spec.engine.", 0) == 0)
        {
            const double now = build.get(f.path);
            want = now < (*f.minimum + *f.maximum) / 2.0 ? *f.maximum : *f.minimum;
            field = &f;
            break;
        }
    REQUIRE(field != nullptr);
    SimHost& host = m.engine.sim->host();
    std::string why;
    const auto inEngine = [&] { return host.readSpec(host.fields().ref(field->path, why)); };

    auto call = m.call("set_fields", { { "changes", { { { "path", field->path.substr(5) }, { "value", want } } } } });
    m.serve(call, 30000);
    REQUIRE(answered(call));
    const assistant::Result r = call.get();
    INFO(r.value.dump());
    REQUIRE_FALSE(r.isError);
    CHECK(build.get(field->path) == want);
    REQUIRE(m.engine.sim->ready());
    CHECK(inEngine() == Catch::Approx(want));   // in the engine within the drain that ran the job
}

// The frame before the jobs: a tool that needs the simulator itself (predict_curve takes it in its job) finds it adopted
// in the same drain, not "not ready yet" until the window ticks once.
TEST_CASE("the assistant's wake: a tool that needs the simulator finds it adopted, no frame ticked", "[app][assistant]")
{
    Minimised m(/*adopted*/ false);
    auto call = m.call("predict_curve", nlohmann::json::object());
    m.serve(call, 300000);
    REQUIRE(answered(call));
    const assistant::Result r = call.get();
    INFO(r.value.dump().substr(0, 400));
    CHECK(r.value.dump().find("not ready") == std::string::npos);
    CHECK_FALSE(r.isError);
}
