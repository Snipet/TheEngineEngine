// shell/input/g29/RacingWheelWin.cpp: RacingWheel.h on Windows: a G29 read as a plain joystick through SDL's joystick
// API (RawInput, DirectInput or Windows.Gaming.Input, as SDL finds it), its controls through g29/G29Joystick.h's
// table for the mode it is in. Nothing is ever sent to the wheel: on Windows the wheel's commands cannot reach it
// without Logitech's software (G29Joystick.h says why), so the range, the spring and the shift lights are G HUB's or
// none; Hooks::lights is never called. Off: the joystick is closed and nothing is read.
//
// Never run with a real wheel in this repository: CI compiles it; tests/shell/G29JoystickTest.cpp drives the reader.
#include "input/g29/RacingWheel.h"

#include "input/g29/G29Joystick.h"

#include <SDL3/SDL.h>

#include <algorithm>

namespace tee::input::g29
{
    struct RacingWheel::Impl
    {
        Hooks hooks;
        bool on = false;
        bool sdl = false;                       // SDL's joystick system started for us
        SDL_Joystick* joystick = nullptr;
        Uint64 nextLook = 0;                    // when to look for a G29 again (ms; while none is open)
        JoystickReader reader;
        Seen seen;

        void close()
        {
            if (joystick != nullptr)
                SDL_CloseJoystick(joystick);
            joystick = nullptr;
        }

        // A G29 among SDL's joysticks, opened; false: none.
        bool look()
        {
            int count = 0;
            SDL_JoystickID* ids = SDL_GetJoysticks(&count);
            if (ids == nullptr)
                return false;
            for (int i = 0; i < count && joystick == nullptr; ++i)
                if (isG29(SDL_GetJoystickVendorForID(ids[i]), SDL_GetJoystickProductForID(ids[i])))
                    joystick = SDL_OpenJoystick(ids[i]);
            SDL_free(ids);
            return joystick != nullptr;
        }

        void read()
        {
            seen.vendor = SDL_GetJoystickVendor(joystick);
            seen.product = SDL_GetJoystickProduct(joystick);
            seen.version = SDL_GetJoystickProductVersion(joystick);
            seen.axes.resize(static_cast<std::size_t>(std::max(0, SDL_GetNumJoystickAxes(joystick))));
            for (std::size_t a = 0; a < seen.axes.size(); ++a)
                seen.axes[a] = SDL_GetJoystickAxis(joystick, static_cast<int>(a));
            seen.buttons.resize(static_cast<std::size_t>(std::max(0, SDL_GetNumJoystickButtons(joystick))));
            for (std::size_t b = 0; b < seen.buttons.size(); ++b)
                seen.buttons[b] = SDL_GetJoystickButton(joystick, static_cast<int>(b));
            seen.hats.resize(static_cast<std::size_t>(std::max(0, SDL_GetNumJoystickHats(joystick))));
            for (std::size_t h = 0; h < seen.hats.size(); ++h)
                seen.hats[h] = SDL_GetJoystickHat(joystick, static_cast<int>(h));
        }

        void pump()
        {
            if (!on || !sdl)
            {
                close();
                if (reader.update(nullptr) && hooks.changed)
                    hooks.changed();
                return;
            }
            if (joystick != nullptr && !SDL_JoystickConnected(joystick))
                close();
            if (joystick == nullptr && SDL_GetTicks() >= nextLook)
            {
                nextLook = SDL_GetTicks() + 500;   // twice a second while none is open
                look();
            }
            if (joystick != nullptr)
                read();
            if (reader.update(joystick != nullptr ? &seen : nullptr) && hooks.changed)
                hooks.changed();
        }
    };

    RacingWheel::RacingWheel(Hooks hooks, bool on) : impl_(std::make_unique<Impl>())
    {
        impl_->hooks = std::move(hooks);
        impl_->on = on;
        impl_->sdl = SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    }

    RacingWheel::~RacingWheel()
    {
        impl_->close();
        if (impl_->sdl)
            SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
    }

    bool RacingWheel::supported() noexcept { return true; }
    bool RacingWheel::present() const noexcept { return impl_->reader.present(); }
    bool RacingWheel::on() const noexcept { return impl_->on; }
    void RacingWheel::setOn(bool on)
    {
        impl_->on = on;
        impl_->pump();
    }
    bool RacingWheel::reading() const noexcept { return impl_->on && impl_->reader.reading(); }
    State RacingWheel::state() const { return reading() ? impl_->reader.state() : State{}; }
    Presses RacingWheel::takePresses() { return {}; }   // sampled a frame at a time: Input counts the frame's edge
    std::string RacingWheel::status() const { return impl_->on ? impl_->reader.status() : std::string(); }
    void RacingWheel::pump() { impl_->pump(); }
}
