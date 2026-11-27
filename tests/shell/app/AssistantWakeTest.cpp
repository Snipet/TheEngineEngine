// tests/shell/app/AssistantWakeTest.cpp: the assistant's wake (app/AssistantWake.h) as the window serves it, with no
// window: a post pushes one SDL event (at most one pending), the event taken lets the next post wake again, a drain that
// left jobs asks again a little later (never at once), and a tool's call over the application's own panel is answered by
// the wake's drains alone, no frame ticked (a minimised window: DesktopHost runs no tick, so shown.frame never runs).
#include "AssistantWake.h"
#include "Engine.h"
#include "Offline.h"
#include "Options.h"

#include "input/Controls.h"
#include "player/SimAdapter.h"

#include "tee/assistant/Session.h"
#include "tee/assistant/Tools.h"

#include <SDL3/SDL.h>

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <future>
#include <memory>
#include <string>

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

    // a timer still pending when the wake goes is removed with it (nothing fires into a freed wake)
    {
        app::AssistantWake gone;
        gone.again();
    }
    SDL_Delay(150);
}

TEST_CASE("the assistant's wake: a tool's call answered with no frame ticked", "[app][assistant]")
{
    app::Options options;
    options.engine = "sim";
    options.sim = TEE_SOURCE_SIM_DIR;
    app::Engine engine = app::loadEngine(options, 48000.0);
    REQUIRE(engine.error.empty());
    REQUIRE(engine.sim != nullptr);
    REQUIRE(engine.sim->waitReady(120.0));
    auto controls = std::make_unique<input::Controls>(*engine.player, *engine.player);
    app::Shown shown = app::makePanel(options, engine, *controls, false);
    REQUIRE(shown.problem.empty());
    REQUIRE(shown.assistant != nullptr);
    REQUIRE(shown.drainAssistant);

    Events events;
    app::AssistantWake wake;
    REQUIRE(wake.usable());
    SDL_FlushEvent(wake.type());
    shown.assistant->setWake(wake.wake());

    // the window's loop, minimised: events only (rawEvent's handling, as app/Main.cpp has it), never shown.frame
    assistant::Session* s = shown.assistant;
    auto call = std::async(std::launch::async, [s] { return s->tools().call("get_build", nlohmann::json::object()); });
    int drains = 0;
    const auto until = Clock::now() + std::chrono::seconds(30);
    while (call.wait_for(std::chrono::seconds(0)) != std::future_status::ready && Clock::now() < until)
    {
        SDL_Event e{};
        if (!SDL_WaitEventTimeout(&e, 100) || !wake.take(e))
            continue;
        shown.drainAssistant();
        ++drains;
        if (shown.assistant->waiting() > 0)
            wake.again();
    }
    REQUIRE(call.wait_for(std::chrono::seconds(0)) == std::future_status::ready);
    const assistant::Result r = call.get();
    INFO(r.value.dump());
    CHECK_FALSE(r.isError);
    CHECK(drains >= 1);
    shown.assistant->setWake({});
}
