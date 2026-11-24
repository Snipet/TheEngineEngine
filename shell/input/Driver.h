// shell/input/Driver.h: the driver's hands and feet, whatever they are on: keys (a pedal eases down while its key is
// held and back when it is let go), a gamepad (analogue, no easing), the Panel's throttle strip and start button. It
// merges them into one ControlsFrame a frame of the application. No SDL and no clock of its own: time comes in through
// advance(dt), so the same inputs give the same frames in a test (docs/controls.md says what a person sees).
//
// The start key. While it is held the frame says ignition 2 (start), and for at least startLeast seconds from its
// press however briefly it was held: a tap shorter than a frame of the window, or than a block of the audio, still
// reaches the engine. Let go, it says 1 (on); let go after it was held
// for offHold seconds on an engine that was running when it went down, it says 0 (off). The reference engine applies
// the same second itself while its start key is held (sim/ref/engine.fm, "the start key held a second switches it
// off"), which is why the frame goes on saying 2 until the key comes up: the two clocks never race.
//
// The gears. The shift keys count presses (shiftUp, shiftDown). With the automatic clutch on (the default) a gear key
// asks for that gear through the same presses, one at a time until the gearbox says it is there, so the lever stays in
// neutral and the gearbox works its own clutch: the keys alone can drive. With it off, a gear key moves the H-pattern
// lever and the clutch is the driver's (its key).
//
// UI thread only.
#pragma once

#include "input/Keys.h"
#include "tee/app/Sources.h"

#include <array>
#include <cstdint>

namespace tee::input
{
    // ctl.buttons' bits (sim/spec/controls.fm, BTN_*) the driver's toggles set.
    inline constexpr int kButtonSplitter = 16, kButtonRange = 32, kButtonEngineBrake = 64;

    // Who holds an action: each source lets go of only what it pressed.
    enum class Source : std::uint8_t { keyboard, gamepad, panel, script, count };

    // Seconds for a pedal's whole travel, down and back.
    struct Easing
    {
        float rise = 0.25f;
        float fall = 0.15f;
    };

    struct DriverSettings
    {
        Easing throttle{ 0.30f, 0.18f };
        Easing brake{ 0.20f, 0.12f };
        Easing clutch{ 0.10f, 0.30f };
        Easing handbrake{ 0.10f, 0.10f };
        bool autoClutch = true;
        float offHold = 1.0f;       // seconds the start key is held on a running engine to switch it off
        float startLeast = 0.15f;   // seconds the frame says start after a press of the start key, at the least
        float crankMax = 2.0f;      // seconds a press on a stopped engine cranks it, at the most, until it runs
        float seekInterval = 0.3f;  // seconds between a gear key's presses of the shift, while the gear is not there
        int seekTries = 12;         // presses a gear key makes before it gives up (a refused downshift)
        int mode = 1;               // the frame's mode: 1 is MODE_DRIVE (docs/plan/02-contracts.md section 4)
    };

    // What the engine says, for the start key's rule and the gear keys.
    struct EngineSeen
    {
        bool running = false;
        bool gearKnown = false;     // false: no gearbox to ask (the gear keys then do nothing with the automatic clutch)
        int gear = 0;
    };

    class Driver
    {
    public:
        explicit Driver(const DriverSettings& = {});

        void setSettings(const DriverSettings&);
        const DriverSettings& settings() const noexcept { return settings_; }

        // A key or a button went down or came up. A repeat of a key that is down changes nothing.
        void set(Source, Action, bool down);

        // A press with no hold: a shift counted, or a gear asked for (the Panel's own controls report counts).
        void press(Action);

        // The gear a control asks for by its number (-1 R, 0 N, 1 up), as a gear key does.
        void requestGear(int gear);

        // A gamepad's triggers and stick: the pedals are at least these.
        void setAnalog(float throttle, float brake, float steer);

        // A wheel's clutch pedal (0 up, 1 on the floor): the pedal is at least this. Let go with release(gamepad).
        void setAnalogClutch(float clutch);

        // The Panel's throttle and brake strips: while held, the pedal is at least value.
        void setStrip(float value, bool held);
        void setBrakeStrip(float value, bool held);
        // The clutch pedal from the mouse (the frame's pedal bar): while held, the pedal is at least value.
        void setClutchStrip(float value, bool held);
        void setHandbrakeStrip(float value, bool held);

        // The toggles that are on, as ctl.buttons' bits (BTN_SPLITTER 16, BTN_RANGE 32, BTN_ENGINE_BRAKE 64).
        int latchedButtons() const noexcept { return latched_; }

        // Whether the start key does anything (false while the engine is not ready to play: its compile). Off: a press
        // from any source is dropped, and a crank under way stops.
        void setStartEnabled(bool on) noexcept;
        bool startEnabled() const noexcept { return startEnabled_; }

        // The frame's mode from now on (ControlsFrame::mode: MODE_BENCH 0, MODE_DRIVE 1, MODE_DYNO_ENGINE 2).
        void setMode(int mode);

        // Everything a source holds is let go (the window lost the keyboard; a gamepad was unplugged).
        void release(Source);

        // dt seconds on. Returns the frame to send.
        const ControlsFrame& advance(float dt, const EngineSeen&);

        const ControlsFrame& frame() const noexcept { return frame_; }

        // The keys' own share of the throttle (eased), for a Panel that shows it.
        float keyThrottle() const noexcept { return throttle_; }

    private:
        bool held(Action) const noexcept;
        static float ease(float x, bool down, const Easing&, float dt) noexcept;
        void pressed(Action);

        DriverSettings settings_;
        std::array<std::uint8_t, static_cast<std::size_t>(Action::count)> held_{};   // a bit a Source
        ControlsFrame frame_{};
        float throttle_ = 0, brake_ = 0, clutch_ = 0, handbrake_ = 0;                // the keys' eased pedals
        float padThrottle_ = 0, padBrake_ = 0, padSteer_ = 0, padClutch_ = 0;
        float strip_ = 0, brakeStrip_ = 0;
        float clutchStrip_ = 0, handbrakeStrip_ = 0;
        bool stripHeld_ = false, brakeStripHeld_ = false, clutchStripHeld_ = false, handbrakeStripHeld_ = false;
        // the start key
        bool startWas_ = false;
        bool startOnRunning_ = false;
        float startHeld_ = 0;
        float startLatch_ = 0;
        bool startLatchFresh_ = false;
        float crankLeft_ = 0;
        bool startEnabled_ = true;
        int latched_ = 0;               // the toggles' ctl.buttons bits (splitter, range, engine brake)           // seconds the starter still cranks a stopped engine after a press  // set by a press: the frame that sees it first does not count down      // seconds the start is still said after a press, whatever the key does
        int ignition_ = 0;
        // the gears
        int shiftUp_ = 0, shiftDown_ = 0;
        int lever_ = 0;
        int seek_ = 0;              // the gear a key asked for (seeking_ says whether one did)
        bool seeking_ = false;
        float seekWait_ = 0;
        int seekLeft_ = 0;
    };
}
