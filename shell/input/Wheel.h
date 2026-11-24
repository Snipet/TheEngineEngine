// shell/input/Wheel.h: the seam for a steering wheel and pedals (docs/plan/w1-b-shell.md, slice 2, item 10). The
// mapping is here (input/WheelMap.h: a table a device, a learn mode; tested with a made-up device); no real device is
// opened yet (SDL's joystick and haptic interfaces, the settings' tables, a window to learn in are still to do).
//
// A wheel is an analogue device like a gamepad, with more of everything: a steering axis with a range the device can
// be told, three pedals that may be combined or inverted, an H-pattern shifter that reports a position, paddles, and
// many buttons. It feeds the same Driver the keys do:
//
//   pedals and steering   Driver::setAnalog (a clutch pedal joins it when the wheel has one)
//   the shifter           the frame's lever, when the automatic clutch is off
//   paddles and buttons   Driver::set(Source::gamepad, Action::..., down)
//
// so nothing downstream of the Driver changes when a wheel arrives.
#pragma once

#include <string>

namespace tee::input
{
    class Driver;

    // One wheel, open. No implementation yet: wheels() below finds none.
    struct Wheel
    {
        virtual ~Wheel() = default;
        virtual std::string name() const = 0;
        virtual void poll(Driver&) = 0;                    // once a frame: its axes and buttons into the Driver
        virtual bool setRange(float degrees) = 0;          // false: the device cannot be told
        virtual bool setAutocentre(float strength) = 0;    // 0 to 1; false: the device has no spring
    };

    // How many wheels the shell can open now. Always 0 until slice 2.
    inline int wheels() noexcept { return 0; }
}
