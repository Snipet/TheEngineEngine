// shell/input/g29/G29Joystick.h: a G29 read as a plain joystick (Windows: RacingWheelWin.cpp fills a Seen from SDL's
// joystick API each frame), in plain C++ (no SDL): which mode the wheel is in, the table of its axes, buttons and hat by
// SDL's index for that mode, and the state and the status line they make. docs/controls.md, "A wheel and pedals".
//
// Why only reading: on Windows every output report goes through hid.dll at the descriptor's full length (17 bytes),
// and the wheel takes its commands (the switch to its own mode, the range, the spring, the lights) as 7-byte reports
// only: SDL 3.4's own Logitech driver is off there for that reason (SDL_hidapi_lg4ff.c). Without Logitech's software
// nothing can be sent to it; with Logitech G HUB, G HUB switches it to its own mode and sets the range, the spring and
// the lights itself. So here nothing is ever sent.
//
// The tables (joystickTable) are EMPTY until the owner has run `TheEngineEngine --probe-input` on Windows in a mode and
// pasted it: a mode with no table is seen and said ("G29 seen ...: its controls are not yet mapped"), and nothing is
// driven from an axis nobody has checked. A table filled in is the whole change; tests/shell/G29JoystickTest.cpp
// drives the rules with a made-up one.
//
// Never run with a real wheel in this repository.
#pragma once

#include "input/g29/G29.h"

#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace tee::input::g29
{
    // One mode's controls, by SDL's index on the joystick (the probe's "axis 2", "button 5", "hat 0").
    struct JoystickTable
    {
        std::array<int, kAxes> axis{ { -1, -1, -1, -1 } };      // steering, throttle, brake, clutch; -1: none
        std::array<bool, kAxes> restAtMax{ { false, true, true, true } };   // a pedal's rest at the axis's top
        std::vector<std::pair<int, int>> buttons;               // SDL button -> the mask's bit (G29.h's kBits)
        std::vector<std::pair<int, int>> gears;                 // SDL button -> gear (-1 reverse, 1..6)
        int hat = -1;                                           // the d-pad's hat; -1: none
        bool mapped() const noexcept;                           // any control at all
    };

    // The table for a mode; null: none yet (every mode, until the owner's probe on Windows fills one in).
    const JoystickTable* joystickTable(Kind kind) noexcept;

    // What SDL shows of a joystick in one frame: its ids, and its axes (-32768..32767), buttons and hats as indexed.
    struct Seen
    {
        int vendor = 0, product = 0, version = 0;
        std::vector<std::int16_t> axes;
        std::vector<bool> buttons;
        std::vector<std::uint8_t> hats;                         // SDL's hat bits: up 1, right 2, down 4, left 8
    };

    // A Seen made a State through a table (an index the joystick does not have reads as at rest).
    State joystickState(const JoystickTable& table, const Seen& seen) noexcept;

    // The reader's decisions, a frame at a time: which G29 is seen, whether it is read, its state and its line.
    class JoystickReader
    {
    public:
        using TableFor = const JoystickTable* (*)(Kind) noexcept;
        explicit JoystickReader(TableFor tableFor = joystickTable) : tableFor_(tableFor) {}

        // Once a frame: the G29 seen now (null: none, or the reader is off). True when present(), reading() or
        // status() changed.
        bool update(const Seen* seen);

        bool present() const noexcept { return kind_ != Kind::none; }
        bool reading() const noexcept { return reading_; }
        Kind kind() const noexcept { return kind_; }
        const State& state() const noexcept { return state_; }
        const std::string& status() const noexcept { return status_; }

        // The status lines, as the WHEEL row and the status bar show them.
        static std::string statusFor(Kind kind, bool mapped);

    private:
        TableFor tableFor_;
        Kind kind_ = Kind::none;
        bool reading_ = false;
        State state_;
        std::string status_;
    };
}
