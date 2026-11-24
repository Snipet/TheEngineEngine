// shell/input/Keys.h: the keys the shell knows, what each can ask for, and the table between the two. No SDL: a key
// is its USB HID usage number, which is what SDL calls a scancode (the key's place on the board, whatever the layout
// prints on it), so W, A, S, D are where a player's hand expects them on any layout. Settings will keep a table of
// these (slice 2); the defaults are docs/controls.md's.
#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

namespace tee::input
{
    // What a key, a gamepad's button or the Panel can ask of the driver.
    enum class Action : std::uint8_t
    {
        none,
        throttle,     // held: the pedal eases down; let go: back up
        brake,
        clutch,
        handbrake,
        start,        // the start key (docs/controls.md, "The start key")
        shiftUp,      // a press each
        shiftDown,
        neutral,
        gear1,
        gear2,
        gear3,
        gear4,
        gear5,
        gear6,
        reverse,
        count
    };

    // A key by its place: the USB HID keyboard usage (SDL_Scancode has the same numbers).
    namespace key
    {
        inline constexpr int kA = 4, kD = 7, kE = 8, kN = 17, kQ = 20, kR = 21, kS = 22, kW = 26;
        inline constexpr int k1 = 30, k2 = 31, k3 = 32, k4 = 33, k5 = 34, k6 = 35;
        inline constexpr int kReturn = 40, kEscape = 41, kBackspace = 42, kTab = 43, kSpace = 44;
        inline constexpr int kPageUp = 75, kPageDown = 78;
        inline constexpr int kRight = 79, kLeft = 80, kDown = 81, kUp = 82;
        inline constexpr int kKeypadEnter = 88;
        inline constexpr int kLeftShift = 225, kRightShift = 229;
    }

    struct Binding
    {
        int key = 0;
        Action action = Action::none;
    };

    // The defaults (docs/controls.md).
    std::vector<Binding> defaultBindings();

    // The action a key is bound to, or Action::none.
    Action actionOf(const std::vector<Binding>&, int key) noexcept;

    // A key's and an action's name, for the docs, settings and the headless script ("w", "up", "return", "lshift";
    // "throttle", "start", "gear3"). keyFromName answers 0 and actionFromName Action::none for a name it does not know.
    std::string_view keyName(int key) noexcept;
    int keyFromName(std::string_view name) noexcept;
    std::string_view actionName(Action) noexcept;
    Action actionFromName(std::string_view name) noexcept;
}
