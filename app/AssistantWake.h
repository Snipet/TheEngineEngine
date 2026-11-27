// app/AssistantWake.h: the assistant's wake (card A's Session::setWake) as the app's window serves it. A tool's job is
// drained on the main thread by the frame's tick, but a window that does not tick (minimised, hidden) never drains it,
// and the call waits out its limit. So a post wakes the main thread: one SDL user event, at most one pending (as
// DesktopHost::requestFrame does it); the window's rawEvent takes it (take()) and drains the inbox there, frame or no
// frame (Shown::drainAssistant). A drain that leaves jobs waiting (one that said "not yet") asks again a little later
// (again(): kAgainMs on, at most one timer pending), never at once, so a waiting job does not spin the main thread.
//
// Any thread may call the function wake() gives; take() and again() are the main thread's. What the wake holds is
// shared: a post after the window has gone pushes an event nothing reads, and a timer outliving this keeps it alive
// until it fires.
#pragma once

#include <SDL3/SDL.h>

#include <atomic>
#include <functional>
#include <memory>

namespace tee::app
{
    class AssistantWake
    {
    public:
        static constexpr Uint32 kAgainMs = 75;   // a drain that left jobs: the next this long after (card A: 50 to 100)

        AssistantWake() : s_(std::make_shared<State>()) { s_->type = SDL_RegisterEvents(1); }
        // A timer still pending is left to fire (within kAgainMs): only its callback lets go of its hold, so no instant
        // of firing races a removal here; its one event finds no reader. SDL quitting first frees it unfired.
        ~AssistantWake() = default;
        AssistantWake(const AssistantWake&) = delete;
        AssistantWake& operator=(const AssistantWake&) = delete;

        // No event type was free: nothing wakes (the window drains when it ticks, as before).
        bool usable() const noexcept { return s_->type != 0; }
        Uint32 type() const noexcept { return s_->type; }

        // For Session::setWake: any thread, lock free.
        std::function<void()> wake() const
        {
            return [s = s_] { post(*s); };
        }

        // The main thread, each event first: true when `e` is the wake (taken: the next post wakes again).
        bool take(const SDL_Event& e) const noexcept
        {
            if (s_->type == 0 || e.type != s_->type)
                return false;
            s_->posted.store(false);
            return true;
        }

        // The main thread, after a drain that left jobs waiting: the wake again kAgainMs on (one timer at most).
        void again()
        {
            if (s_->type == 0 || s_->armed.exchange(true))
                return;
            auto* hold = new std::shared_ptr<State>(s_);   // the timer's own: its callback deletes it
            if (SDL_AddTimer(kAgainMs, &fire, hold) == 0)
            {
                delete hold;
                s_->armed.store(false);
            }
        }

        bool againPending() const noexcept { return s_->armed.load(); }

    private:
        struct State
        {
            Uint32 type = 0;
            std::atomic<bool> posted{ false };
            std::atomic<bool> armed{ false };   // a timer pending
        };

        static void post(State& s)
        {
            if (s.type == 0 || s.posted.exchange(true))
                return;
            SDL_Event e{};
            e.type = s.type;
            if (!SDL_PushEvent(&e))
                s.posted.store(false);
        }

        static Uint32 fire(void* user, SDL_TimerID, Uint32)
        {
            auto* hold = static_cast<std::shared_ptr<State>*>(user);
            State& s = **hold;
            s.armed.store(false);
            post(s);
            delete hold;
            return 0;
        }

        std::shared_ptr<State> s_;
    };
}
