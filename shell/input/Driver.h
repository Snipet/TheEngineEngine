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
// Which gears a press reaches depends on the gearbox (setGearbox: the Controls read it from the build each frame;
// the reference's vehicle, car.model 0, reads no gearbox and is always `shifts`):
//   shifts     a manual's paddles, a sequential box, a dual clutch: as above (the shift presses)
//   selector   an automatic and a CVT, which read only the lever as R, N, D (sim/vehicle/gearbox.fm), held on the
//              lever whatever the automatic clutch says. A shift up is a step towards D (R to N, N to D); a shift down
//              a step towards R, and out of D only at a crawl (under kCrawlKmh: above it a shift down does nothing, so
//              a press hoping for a kickdown never drops the car into neutral at speed). The N key: N at any speed; the
//              reverse key: R (the box refuses it above a crawl); 1 to 6 and the shifter's forward gates: D; its
//              reverse gate R; out of every gate N. No gear key seeks.
//   eighteen   the eighteen-speed (sim/vehicle/roadranger.fm), which reads only the lever's positions (1 to 4, 5 the
//              L slot, -1 reverse), the splitter and range bits and the clutch pedal: the lever is sent whatever the
//              automatic clutch says, and the clutch is the driver's (nothing here moves a pedal or matches revs: a
//              driver who does neither grinds, as the box says). Keys and gates 1 to 4: the lever's positions; 5: the
//              L slot; 6: nothing; reverse -1; N 0. A shift up or down: the next gear of the eighteen in order (the
//              lever, the splitter and the range that gear wants: roadranger.fm's gearOf), refused at 1 and 18; Z and
//              X then toggle from where the press left them.
// Before 27 Nov the lever was zeroed for every box while the automatic clutch was on, so the automatic, the CVT and
// the eighteen-speed sat in neutral whatever was pressed (the owner's report; tests/shell/GearboxDrivingTest.cpp).
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

    // What a press of the gears reaches (the comment above).
    enum class Gearbox : std::uint8_t { shifts, selector, eighteen };
    inline constexpr float kCrawlKmh = 5.0f;   // the automatic's own crawl (gearbox.fm: into R only under it)

    // Who holds an action: each source lets go of only what it pressed.
    enum class Source : std::uint8_t { keyboard, gamepad, panel, script, wheel, count };

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
        float kmh = 0.0f;           // the car's speed (car.speed), for the selector's step out of D
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

        // A racing wheel's pedals (input/g29/G29Input.h), 0 up to 1 on the floor: each pedal is at least its own, beside
        // a gamepad's (neither overwrites the other). Its steering is not the car's (a straight line). Let go with
        // release(wheel).
        void setWheel(float throttle, float brake, float clutch);

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

        // The gearbox the presses drive (a change: the selector and the lever back to neutral, as the box goes).
        void setGearbox(Gearbox g);
        Gearbox gearbox() const noexcept { return gearbox_; }

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
        float wheelThrottle_ = 0, wheelBrake_ = 0, wheelClutch_ = 0;
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
        Gearbox gearbox_ = Gearbox::shifts;
        int select_ = 0;            // the selector's R -1, N 0, D 1 (Gearbox::selector)
        float kmh_ = 0.0f;          // the car's speed as last seen (advance)
        void eighteenStep(int by);  // the next gear of the eighteen, up (1) or down (-1)
        int seek_ = 0;              // the gear a key asked for (seeking_ says whether one did)
        bool seeking_ = false;
        float seekWait_ = 0;
        int seekLeft_ = 0;
    };
}
