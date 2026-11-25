// shell/input/g29/G29Input.h: a G29's state (g29/G29.h) into the Driver, a frame at a time, as the keys and a gamepad
// go in (input/Driver.h). docs/controls.md, "A wheel and pedals", says it to a person.
//
//   the pedals             Driver::setWheel: throttle, brake and clutch, each the larger of the wheel's and the
//                          others' (a gamepad's, the keys', the Panel's strips)
//   the steering           not the car's (it runs in a straight line): RacingWheel::state() publishes it, nothing
//                          drives with it
//   the right paddle       shift up (a press each); the left paddle: shift down
//   the PS button          the start key, held as long as it is held (a short press starts a stopped engine; held
//                          on a running one, off: as the key)
//   the shifter            the gate it is in, asked for as a gear key asks (Driver::requestGear): with the automatic
//                          clutch off, the H-pattern lever itself (the clutch pedal is yours); with it on, the gear
//                          sought through the gearbox's own shifts (a sequential or automatic box too); out of every
//                          gate: neutral
//   other buttons          nothing yet (the learn mode comes later)
//
// Reading stops (the wheel off or gone): everything the wheel held is let go, once (Driver::release(Source::wheel)).
#pragma once

#include "input/g29/G29.h"

namespace tee::input
{
    class Driver;
}

namespace tee::input::g29
{
    class Input
    {
    public:
        // Once a frame. `reading`: the wheel is read (RacingWheel::reading()); false: let go, the first time.
        // (tee::input::Driver, the keys' and the gamepad's: not g29::Driver, the wheel's own decisions)
        void apply(const State& now, bool reading, tee::input::Driver& driver);

    private:
        State was_;
        bool active_ = false;
    };
}
