// app/InputProbe.cpp: InputProbe.h.
#include "InputProbe.h"

#include "input/g29/G29.h"
#include "input/g29/G29Input.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

namespace tee::app
{
    namespace
    {
        const char* typeName(SDL_JoystickType t)
        {
            switch (t)
            {
                case SDL_JOYSTICK_TYPE_GAMEPAD: return "gamepad";
                case SDL_JOYSTICK_TYPE_WHEEL: return "WHEEL";
                case SDL_JOYSTICK_TYPE_ARCADE_STICK: return "arcade stick";
                case SDL_JOYSTICK_TYPE_FLIGHT_STICK: return "flight stick";
                case SDL_JOYSTICK_TYPE_DANCE_PAD: return "dance pad";
                case SDL_JOYSTICK_TYPE_GUITAR: return "guitar";
                case SDL_JOYSTICK_TYPE_DRUM_KIT: return "drum kit";
                case SDL_JOYSTICK_TYPE_ARCADE_PAD: return "arcade pad";
                case SDL_JOYSTICK_TYPE_THROTTLE: return "throttle";
                default: return "unknown";
            }
        }

        std::string hapticFeatures(Uint32 f)
        {
            static const std::pair<Uint32, const char*> names[] = {
                { SDL_HAPTIC_CONSTANT, "constant" },   { SDL_HAPTIC_SINE, "sine" },
                { SDL_HAPTIC_SQUARE, "square" },       { SDL_HAPTIC_TRIANGLE, "triangle" },
                { SDL_HAPTIC_SAWTOOTHUP, "sawtooth-up" }, { SDL_HAPTIC_SAWTOOTHDOWN, "sawtooth-down" },
                { SDL_HAPTIC_RAMP, "ramp" },           { SDL_HAPTIC_SPRING, "spring" },
                { SDL_HAPTIC_DAMPER, "damper" },       { SDL_HAPTIC_INERTIA, "inertia" },
                { SDL_HAPTIC_FRICTION, "friction" },   { SDL_HAPTIC_LEFTRIGHT, "left-right" },
                { SDL_HAPTIC_CUSTOM, "custom" },       { SDL_HAPTIC_GAIN, "gain" },
                { SDL_HAPTIC_AUTOCENTER, "autocentre" }, { SDL_HAPTIC_STATUS, "status" },
                { SDL_HAPTIC_PAUSE, "pause" },
            };
            std::string out;
            for (const auto& [bit, name] : names)
                if ((f & bit) != 0)
                    out += (out.empty() ? "" : ", ") + std::string(name);
            return out.empty() ? std::string("none") : out;
        }

        struct Axis
        {
            int rest = 0, low = 0, high = 0, printed = 0;
        };

        struct Device
        {
            SDL_Joystick* joystick = nullptr;
            int number = 0;                     // as printed: #1, #2, ...
            std::string name;
            std::vector<Axis> axes;
            std::map<int, int> presses;         // button -> times pressed
            std::map<int, int> hats;            // hat -> last value
            std::map<int, std::string> hatsSeen; // hat -> the directions it went
        };

        std::string hatName(Uint8 v)
        {
            switch (v)
            {
                case SDL_HAT_CENTERED: return "centred";
                case SDL_HAT_UP: return "up";
                case SDL_HAT_DOWN: return "down";
                case SDL_HAT_LEFT: return "left";
                case SDL_HAT_RIGHT: return "right";
                case SDL_HAT_LEFTUP: return "left-up";
                case SDL_HAT_LEFTDOWN: return "left-down";
                case SDL_HAT_RIGHTUP: return "right-up";
                case SDL_HAT_RIGHTDOWN: return "right-down";
                default: return std::to_string(v);
            }
        }

        // Opens and describes one; its axes' rest values read now.
        void describe(SDL_JoystickID id, std::map<SDL_JoystickID, Device>& open, int& counter)
        {
            if (open.count(id) != 0)
                return;
            Device d;
            d.number = ++counter;
            const char* name = SDL_GetJoystickNameForID(id);
            d.name = name != nullptr ? name : "(no name)";
            char guid[64] = {};
            SDL_GUIDToString(SDL_GetJoystickGUIDForID(id), guid, sizeof guid);
            std::printf("\njoystick #%d: \"%s\"\n", d.number, d.name.c_str());
            std::printf("  vendor 0x%04x, product 0x%04x, guid %s\n", SDL_GetJoystickVendorForID(id),
                        SDL_GetJoystickProductForID(id), guid);
            std::printf("  SDL's type: %s%s\n", typeName(SDL_GetJoystickTypeForID(id)),
                        SDL_IsGamepad(id) ? "; SDL also takes it for a gamepad (it has a mapping)" : "");
            // a G29: which of its modes (the application's table is by mode: input/g29/G29Joystick.h on Windows)
            const int vendor = SDL_GetJoystickVendorForID(id), product = SDL_GetJoystickProductForID(id);
            const int version = SDL_GetJoystickProductVersionForID(id);
            if (tee::input::g29::isG29(vendor, product))
            {
                using tee::input::g29::Kind;
                const Kind k = tee::input::g29::kindOf(vendor, product, version);
                std::printf("  a Logitech G29 (version 0x%04x): %s\n", version,
                            k == Kind::g29      ? "in its own mode (0xC24F; on Windows: as Logitech G HUB sets it)"
                            : k == Kind::compat ? "in compatibility mode (0xC294: its switch at PS3, no Logitech software)"
                            : k == Kind::ps4    ? "with its switch at PS4 (0xC260)"
                                                : "an id of the G29's, but not told apart as one (another Logitech wheel?)");
            }
            d.joystick = SDL_OpenJoystick(id);
            if (d.joystick == nullptr)
            {
                std::printf("  could not be opened: %s\n", SDL_GetError());
                open[id] = std::move(d);
                return;
            }
            const int axes = SDL_GetNumJoystickAxes(d.joystick);
            const int buttons = SDL_GetNumJoystickButtons(d.joystick);
            const int hats = SDL_GetNumJoystickHats(d.joystick);
            std::printf("  axes %d, buttons %d, hats %d\n", axes, buttons, hats);
            SDL_UpdateJoysticks();
            for (int a = 0; a < std::max(axes, 0); ++a)
            {
                Axis x;
                Sint16 initial = 0;
                const bool known = SDL_GetJoystickAxisInitialState(d.joystick, a, &initial);
                x.rest = known ? initial : SDL_GetJoystickAxis(d.joystick, a);
                x.low = x.high = x.printed = SDL_GetJoystickAxis(d.joystick, a);
                std::printf("  axis %d: now %d (at rest: %d%s)\n", a, x.printed, x.rest, known ? "" : ", as read now");
                d.axes.push_back(x);
            }
            if (SDL_IsJoystickHaptic(d.joystick))
            {
                if (SDL_Haptic* h = SDL_OpenHapticFromJoystick(d.joystick))
                {
                    const char* hn = SDL_GetHapticName(h);
                    std::printf("  haptic: yes (\"%s\"), axes %d, effects at once %d\n", hn != nullptr ? hn : "",
                                SDL_GetNumHapticAxes(h), SDL_GetMaxHapticEffects(h));
                    std::printf("  haptic features: %s\n", hapticFeatures(SDL_GetHapticFeatures(h)).c_str());
                    std::printf("  rumble: %s\n", SDL_HapticRumbleSupported(h) ? "supported" : "no");
                    SDL_CloseHaptic(h);
                }
                else
                    std::printf("  haptic: it says it has one, but it did not open: %s\n", SDL_GetError());
            }
            else
                std::printf("  haptic: none reported by SDL for this joystick\n");
            open[id] = std::move(d);
        }
    }

    int probeInput(double seconds)
    {
        // events from devices while no window of ours has the focus (there is none)
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
        // SDL's own Logitech wheel driver would send a wheel its commands as it opens it (the spring off, a mode
        // switch): the probe sends nothing, as the application does (input/SdlInput.cpp turns it off the same way)
        SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI_LG4FF, "0");
        if (!SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC | SDL_INIT_EVENTS))
        {
            // the haptic system may be what is missing: try without it
            std::printf("probe: SDL's joystick and haptic systems did not start (%s); trying without haptics\n",
                        SDL_GetError());
            if (!SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS))
            {
                std::fprintf(stderr, "probe: SDL's joystick system did not start: %s\n", SDL_GetError());
                return 2;
            }
        }
        int v = SDL_GetVersion();
        std::printf("probe: SDL %d.%d.%d on %s; joystick drivers this SDL was built with are used as the system offers "
                    "them\n",
                    SDL_VERSIONNUM_MAJOR(v), SDL_VERSIONNUM_MINOR(v), SDL_VERSIONNUM_MICRO(v), SDL_GetPlatform());
        std::printf("probe: this prints to this window only; nothing is sent anywhere and nothing is written\n");

        // A test's stand-in (TEE_PROBE_VIRTUAL=1): SDL's own virtual joystick, described as a wheel, moved by the probe
        // itself below. Never a person's device; the probe says it is one.
        const char* virtualAsked = std::getenv("TEE_PROBE_VIRTUAL");
        SDL_JoystickID virtualId = 0;
        if (virtualAsked != nullptr && virtualAsked[0] == '1')
        {
            SDL_VirtualJoystickDesc desc;
            SDL_INIT_INTERFACE(&desc);
            desc.type = SDL_JOYSTICK_TYPE_WHEEL;
            desc.vendor_id = 0x046D;            // a G29's ids, in its own mode: the probe's mode line is checked too
            desc.product_id = 0xC24F;
            desc.naxes = 4;
            desc.nbuttons = 12;
            desc.nhats = 1;
            desc.name = "virtual wheel (a test's stand-in, not a device)";
            virtualId = SDL_AttachVirtualJoystick(&desc);
            std::printf("probe: TEE_PROBE_VIRTUAL: a virtual wheel attached for a test (%s)\n",
                        virtualId != 0 ? "ok" : SDL_GetError());
        }

        std::map<SDL_JoystickID, Device> open;
        int counter = 0;
        int count = 0;
        if (SDL_JoystickID* ids = SDL_GetJoysticks(&count))
        {
            for (int i = 0; i < count; ++i)
                describe(ids[i], open, counter);
            SDL_free(ids);
        }
        std::printf("haptic devices SDL sees on their own (not through a joystick): ");
        int hapticCount = 0;
        if (SDL_HapticID* hs = SDL_GetHaptics(&hapticCount))
        {
            if (hapticCount == 0)
                std::printf("none\n");
            for (int i = 0; i < hapticCount; ++i)
            {
                const char* n = SDL_GetHapticNameForID(hs[i]);
                std::printf("%s\"%s\"", i == 0 ? "" : ", ", n != nullptr ? n : "(no name)");
            }
            if (hapticCount > 0)
                std::printf("\n");
            SDL_free(hs);
        }
        else
            std::printf("none (the haptic system is not available)\n");

        if (count == 0)
            std::printf("\nnone seen: SDL sees no joystick, wheel or gamepad. Is it plugged in (and powered, if it has its "
                        "own supply)? Some wheels start in a mode for one console: a switch on the wheel, or the maker's "
                        "app (Logitech G HUB) running, may be what makes the computer see it. It may also be plugged in "
                        "during the next %.0f s: this keeps listening.\n",
                        seconds);

        std::printf("\nlistening for %.0f s: press each pedal fully and let it go, pull each paddle, move the shifter "
                    "through every gate, turn the wheel to each stop and back, press each button. Changes:\n",
                    seconds);
        std::fflush(stdout);
        const auto start = std::chrono::steady_clock::now();
        const auto elapsed = [&] {
            return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        };
        double lastNote = 0.0;
        int virtualStep = 0;
        while (elapsed() < seconds)
        {
            // the test's stand-in: a pedal pressed fully and let go, a button pressed, the hat moved
            if (virtualId != 0 && open.count(virtualId) != 0 && open[virtualId].joystick != nullptr)
            {
                SDL_Joystick* j = open[virtualId].joystick;
                const double t = elapsed();
                if (virtualStep == 0 && t > 0.1)
                {
                    SDL_SetJoystickVirtualAxis(j, 1, 32767);
                    SDL_SetJoystickVirtualButton(j, 4, true);
                    ++virtualStep;
                }
                else if (virtualStep == 1 && t > 0.3)
                {
                    SDL_SetJoystickVirtualAxis(j, 1, -32768);
                    SDL_SetJoystickVirtualButton(j, 4, false);
                    SDL_SetJoystickVirtualHat(j, 0, SDL_HAT_UP);
                    ++virtualStep;
                }
            }
            SDL_Event e;
            while (SDL_PollEvent(&e))
            {
                const double t = elapsed();
                switch (e.type)
                {
                    case SDL_EVENT_JOYSTICK_ADDED:
                        if (open.count(e.jdevice.which) == 0)
                        {
                            std::printf("%6.2f s: a joystick was plugged in\n", t);
                            describe(e.jdevice.which, open, counter);
                        }
                        break;
                    case SDL_EVENT_JOYSTICK_REMOVED:
                        if (auto it = open.find(e.jdevice.which); it != open.end())
                        {
                            std::printf("%6.2f s: joystick #%d (\"%s\") was unplugged\n", t, it->second.number,
                                        it->second.name.c_str());
                            if (it->second.joystick != nullptr)
                                SDL_CloseJoystick(it->second.joystick);
                            it->second.joystick = nullptr;
                        }
                        break;
                    case SDL_EVENT_JOYSTICK_AXIS_MOTION:
                        if (auto it = open.find(e.jaxis.which); it != open.end() && e.jaxis.axis < it->second.axes.size())
                        {
                            Axis& a = it->second.axes[e.jaxis.axis];
                            const int value = e.jaxis.value;
                            a.low = std::min(a.low, value);
                            a.high = std::max(a.high, value);
                            // a line each eighth of the range, and at either end (not every sample)
                            if (std::abs(value - a.printed) >= 4096 || value == -32768 || value == 32767)
                            {
                                if (value != a.printed)
                                    std::printf("%6.2f s: #%d axis %d = %d\n", t, it->second.number,
                                                static_cast<int>(e.jaxis.axis), value);
                                a.printed = value;
                            }
                        }
                        break;
                    case SDL_EVENT_JOYSTICK_BUTTON_DOWN:
                    case SDL_EVENT_JOYSTICK_BUTTON_UP:
                        if (auto it = open.find(e.jbutton.which); it != open.end())
                        {
                            const bool down = e.type == SDL_EVENT_JOYSTICK_BUTTON_DOWN;
                            if (down)
                                ++it->second.presses[e.jbutton.button];
                            std::printf("%6.2f s: #%d button %d %s\n", t, it->second.number,
                                        static_cast<int>(e.jbutton.button), down ? "down" : "up");
                        }
                        break;
                    case SDL_EVENT_JOYSTICK_HAT_MOTION:
                        if (auto it = open.find(e.jhat.which); it != open.end())
                        {
                            it->second.hats[e.jhat.hat] = e.jhat.value;
                            if (e.jhat.value != SDL_HAT_CENTERED)
                            {
                                std::string& seen = it->second.hatsSeen[e.jhat.hat];
                                const std::string dir = hatName(e.jhat.value);
                                if (("," + seen + ",").find("," + dir + ",") == std::string::npos)
                                    seen += (seen.empty() ? "" : ",") + dir;
                            }
                            std::printf("%6.2f s: #%d hat %d %s\n", t, it->second.number, static_cast<int>(e.jhat.hat),
                                        hatName(e.jhat.value).c_str());
                        }
                        break;
                    default:
                        break;
                }
            }
            if (elapsed() - lastNote >= 15.0 && elapsed() + 1.0 < seconds)
            {
                lastNote = elapsed();
                std::printf("        (%.0f s left)\n", seconds - lastNote);
            }
            std::fflush(stdout);
            SDL_Delay(5);
        }

        std::printf("\nsummary:\n");
        if (open.empty())
            std::printf("  none seen\n");
        for (auto& [id, d] : open)
        {
            std::printf("  #%d \"%s\":\n", d.number, d.name.c_str());
            for (std::size_t a = 0; a < d.axes.size(); ++a)
            {
                const Axis& x = d.axes[a];
                std::printf("    axis %zu: rest %d, seen %d to %d%s\n", a, x.rest, x.low, x.high,
                            x.high > x.low ? "" : " (did not move)");
            }
            std::string pressed;
            for (const auto& [b, n] : d.presses)
                pressed += (pressed.empty() ? "" : ", ") + std::to_string(b) + (n > 1 ? " (x" + std::to_string(n) + ")" : "");
            std::printf("    buttons pressed: %s\n", pressed.empty() ? "none" : pressed.c_str());
            for (const auto& [h, dirs] : d.hatsSeen)
                std::printf("    hat %d went: %s\n", h, dirs.c_str());
            if (d.joystick != nullptr)
                SDL_CloseJoystick(d.joystick);
        }
        // a G29 seen: the application's meaning of each of its buttons by default (the probe's indices are SDL's, not
        // these: on Windows the table that joins them comes from this probe's lines)
        bool g29 = false;
        for (const auto& [id, d] : open)
            g29 = g29 || tee::input::g29::isG29(SDL_GetJoystickVendorForID(id), SDL_GetJoystickProductForID(id));
        if (g29)
        {
            std::printf("\nthe application's G29 buttons, by default (F9, Wheel buttons, moves them):\n");
            const tee::input::g29::Buttons defaults = tee::input::g29::defaultButtons();
            for (int b = 0; b < static_cast<int>(std::size(tee::input::g29::kBits)); ++b)
            {
                std::string meaning;
                for (const tee::input::g29::Binding& x : defaults)
                    if (x.bit == b)
                        meaning += (meaning.empty() ? "" : ", ") + tee::input::g29::targetLabel({ x.action, x.edit });
                if (b == tee::input::g29::kBitShare)
                    meaning = "SHARE held: a value's edit is fine";
                std::printf("  %-12s %s\n", tee::input::g29::kBits[b].name, meaning.empty() ? "(free)" : meaning.c_str());
            }
        }
        std::printf("probe: done. Paste everything above into the conversation.\n");
        if (virtualId != 0)
            SDL_DetachVirtualJoystick(virtualId);
        SDL_Quit();
        return 0;
    }
}
