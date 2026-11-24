// shell/input/SdlInput.h: SDL's events into the Controls: the keyboard's keys down and up (by scancode, which is
// Keys.h's key number), a gamepad's triggers, stick and buttons through SDL's gamepad interface, and the window
// losing the keyboard. The application hands it every SDL event before the window's host reads it (DesktopHost's
// Options::rawEvent): a key the driver takes is not the Panel's.
//
// A gamepad (docs/controls.md): the right trigger is the throttle and the left the brake, analogue, with no easing;
// the left stick steers; the right bumper or A shifts up, the left bumper or X down; Start is the start key. The first
// gamepad plugged in is the one read; when it goes, the next.
//
// Main thread only.
#pragma once

#include "input/Controls.h"

#include <string>

struct SDL_Gamepad;
union SDL_Event;

namespace tee::input
{
    class SdlInput
    {
    public:
        explicit SdlInput(Controls&);
        ~SdlInput();
        SdlInput(const SdlInput&) = delete;
        SdlInput& operator=(const SdlInput&) = delete;

        // SDL's gamepad subsystem (the events arrive once it runs). False and why; the keyboard works without it.
        bool openGamepads(std::string& why);

        // True: the event was the driver's, and the window's host should not read it.
        bool handle(const SDL_Event&);

        // The gamepad read now, for --info and the status line; empty: none.
        std::string gamepadName() const;

    private:
        void adopt(unsigned id);
        void drop();
        void analog();

        Controls& controls_;
        SDL_Gamepad* pad_ = nullptr;
        unsigned padId_ = 0;
        bool gamepads_ = false;
    };
}
