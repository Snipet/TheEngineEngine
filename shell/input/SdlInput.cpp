// shell/input/SdlInput.cpp: SdlInput.h.
#include "input/SdlInput.h"

#include "input/g29/G29.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cmath>

namespace tee::input
{
    namespace
    {
        // A trigger's travel, 0 to 1 (SDL gives 0 to 32767), with a little slack at rest.
        float trigger(Sint16 v) noexcept
        {
            const float t = static_cast<float>(v) / 32767.0f;
            return std::clamp((t - 0.03f) / 0.94f, 0.0f, 1.0f);
        }

        // A stick's axis, -1 to 1, with a dead zone about the centre.
        float stick(Sint16 v) noexcept
        {
            const float t = std::clamp(static_cast<float>(v) / 32767.0f, -1.0f, 1.0f);
            const float a = std::fabs(t);
            if (a < 0.08f)
                return 0.0f;
            return std::copysign((a - 0.08f) / 0.92f, t);
        }

        Action buttonAction(Uint8 button) noexcept
        {
            switch (button)
            {
                case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
                case SDL_GAMEPAD_BUTTON_SOUTH: return Action::shiftUp;
                case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
                case SDL_GAMEPAD_BUTTON_WEST: return Action::shiftDown;
                case SDL_GAMEPAD_BUTTON_START: return Action::start;
                default: return Action::none;
            }
        }
    }

    SdlInput::SdlInput(Controls& controls) : controls_(controls) {}

    SdlInput::~SdlInput()
    {
        drop();
        if (gamepads_)
            SDL_QuitSubSystem(SDL_INIT_GAMEPAD);
    }

    bool SdlInput::openGamepads(std::string& why)
    {
        if (gamepads_)
            return true;
        // The G29 is the racing wheel's (input/g29/RacingWheel.h), never a gamepad here. SDL's own HIDAPI Logitech
        // driver (SDL_hidapi_lg4ff.c, on by default but on Windows) would set its autocentre to 0 the moment it merely
        // detected the wheel, which would undo the wheel's spring: it is turned off before SDL's joysticks start.
        SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_LG4FF, "0");
        if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD))
        {
            why = SDL_GetError();
            return false;
        }
        gamepads_ = true;
        return true;
    }

    std::string SdlInput::gamepadName() const
    {
        const char* name = pad_ != nullptr ? SDL_GetGamepadName(pad_) : nullptr;
        return name != nullptr ? name : "";
    }

    void SdlInput::adopt(unsigned id)
    {
        if (pad_ != nullptr)
            return;
        // a G29 (any of its three ids) is the racing wheel's: never opened as a gamepad, so it cannot drive twice
        const auto sid = static_cast<SDL_JoystickID>(id);
        if (g29::isG29(SDL_GetGamepadVendorForID(sid), SDL_GetGamepadProductForID(sid)))
            return;
        pad_ = SDL_OpenGamepad(sid);
        padId_ = pad_ != nullptr ? id : 0;
    }

    void SdlInput::drop()
    {
        if (pad_ != nullptr)
            SDL_CloseGamepad(pad_);
        pad_ = nullptr;
        padId_ = 0;
        controls_.driver().release(Source::gamepad);
    }

    void SdlInput::analog()
    {
        if (pad_ == nullptr)
            return;
        controls_.driver().setAnalog(trigger(SDL_GetGamepadAxis(pad_, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)),
                                     trigger(SDL_GetGamepadAxis(pad_, SDL_GAMEPAD_AXIS_LEFT_TRIGGER)),
                                     stick(SDL_GetGamepadAxis(pad_, SDL_GAMEPAD_AXIS_LEFTX)));
    }

    bool SdlInput::handle(const SDL_Event& e)
    {
        switch (e.type)
        {
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                // a repeat of a held key changes nothing for the driver, and is still the driver's
                return controls_.key(static_cast<int>(e.key.scancode), e.key.down);

            case SDL_EVENT_WINDOW_FOCUS_LOST:
                controls_.focusLost();
                return false;                       // the host wants it too

            case SDL_EVENT_GAMEPAD_ADDED:
                adopt(static_cast<unsigned>(e.gdevice.which));
                return true;

            case SDL_EVENT_GAMEPAD_REMOVED:
                if (pad_ != nullptr && static_cast<unsigned>(e.gdevice.which) == padId_)
                {
                    drop();
                    // another one that is still there
                    int count = 0;
                    if (SDL_JoystickID* ids = SDL_GetGamepads(&count))
                    {
                        for (int i = 0; i < count && pad_ == nullptr; ++i)
                            adopt(static_cast<unsigned>(ids[i]));
                        SDL_free(ids);
                    }
                }
                return true;

            case SDL_EVENT_GAMEPAD_AXIS_MOTION:
                if (pad_ != nullptr && static_cast<unsigned>(e.gaxis.which) == padId_)
                    analog();
                return true;

            case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
            case SDL_EVENT_GAMEPAD_BUTTON_UP:
                if (pad_ != nullptr && static_cast<unsigned>(e.gbutton.which) == padId_)
                {
                    const Action action = buttonAction(e.gbutton.button);
                    if (action != Action::none)
                        controls_.driver().set(Source::gamepad, action, e.gbutton.down);
                }
                return true;

            default:
                return false;
        }
    }
}
