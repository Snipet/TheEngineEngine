// shell/input/g29/RacingWheel.h: a Logitech G29 racing wheel (its pedals and the Driving Force Shifter on its one
// cable), read by the application. macOS: RacingWheelMac.cpp, IOKit's IOHIDManager (C API) on the main run loop; every
// other system: RacingWheelNone.cpp (none is ever present; docs/controls.md says so). Ported from TestingPlayground's
// Source/plugin/standalone/RacingWheel.{h,cpp} (47bac40; SEED.tsv), the MIDI taken out.
//
// It watches for a G29 (g29/G29.h's kindOf: vendor 0x046D, any of its three product ids) and hands what it sees to a
// g29::Driver (g29/G29Driver.h), which decides; this is its transport (the reports, the values, two timers). While on:
//   - a G29 in compatibility mode (0xC294, its switch at PS3 without Logitech's software) is switched to its own mode
//     with hid-lg4ff's two reports, at most once each time it is switched on; it leaves and comes back as 0xC24F
//     ("switching to its full mode" meanwhile; after 8 s without it, or back in its compatibility mode, what to do);
//   - a G29 in its own mode (0xC24F) is set up as a game does (900 degrees and a moderate centring spring) and read
//     (never IOKit's cache before the first report): state() is its pedals, steering, gear and buttons;
//   - a G29 at PS4 (0xC260) is not read ("set the switch on the wheel to PS3").
// Its five shift lights follow Hooks::lights (the DRIVE view's count), looked at 40 times a second while it is read,
// sent only when they change. Switched off, or the wheel leaving: the spring stops and the lights go off, and state()
// is at rest. Destroyed (the application quitting): the spring stops and the lights go off. A crash or a force-quit
// sends nothing: the spring stays until the wheel is unplugged. Off, the device is open (shared: a game may read it
// too) but nothing is read from it or sent to it.
//
// Main thread only (the window's: where SDL pumps the system's events). pump() once a frame.
//
// Never run with a real wheel in this repository: the macOS body is compiled by CI and run by no one but the owner;
// tests/shell/G29Test.cpp drives the decisions (G29Driver) with fake devices.
#pragma once

#include "input/g29/G29.h"

#include <functional>
#include <memory>
#include <string>

namespace tee::input::g29
{
    class RacingWheel
    {
    public:
        struct Hooks
        {
            std::function<void()> changed;       // present() or status() may have changed
            std::function<int()> lights;         // the shift lights asked for: their mask (0 to kAllLights), -1 none
        };

        RacingWheel(Hooks hooks, bool on);
        ~RacingWheel();
        RacingWheel(const RacingWheel&) = delete;
        RacingWheel& operator=(const RacingWheel&) = delete;

        // Whether this build can read a G29 at all (macOS: yes; elsewhere: not yet).
        static bool supported() noexcept;

        bool present() const noexcept;           // a G29 is connected, in any mode, or a switch has news
        bool on() const noexcept;
        void setOn(bool on);
        bool reading() const noexcept;           // a G29 is read now
        State state() const;                     // its state (at rest while not reading)
        std::string status() const;              // a short line, "" when there is none

        // Once a frame: the system's run loop serviced once, so the device's sources and the timers run whatever the
        // window's event pump does.
        void pump();

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
