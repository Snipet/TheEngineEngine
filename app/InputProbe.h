// app/InputProbe.h: TheEngineEngine --probe-input [--probe-seconds S]: what SDL sees of the game controllers on this
// machine, for a person to read and paste back (a wheel's facts before the wheel is driven: docs/controls.md).
//
// For every joystick SDL sees: its name, vendor and product ids, SDL's type (a wheel, a gamepad, ...), whether SDL
// takes it for a gamepad, its counts of axes, buttons and hats, each axis's value at rest, and whether a haptic device
// opens from it with the features it reports (constant, sine, spring, damper, autocentre, gain, ...). Then, for S
// seconds (60), every change: which axis moved and to what, which button went down or up, which hat moved; a device
// plugged in or out. At the end, each axis's rest and the extremes seen. Prints to the console only: nothing is sent
// anywhere, nothing is written. With no device it says "none seen" and what may be why.
//
// The probe opens no window and no audio device, and changes nothing on the device (no force is played).
#pragma once

namespace tee::app
{
    // Runs the probe for `seconds`; 0 when it ran (devices or none), 2 when SDL's joystick system would not start.
    int probeInput(double seconds);
}
