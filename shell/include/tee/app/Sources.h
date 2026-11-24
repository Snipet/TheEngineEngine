// shell/include/tee/app/Sources.h: the four faces of whatever is playing, as the shell, the application and the
// interface see it, and what the application gives a Panel besides. Plain C++20: no SDL, no FunkGui, no FMinus. The
// shell adapts the reference (tee::RefPlayer, the keel's) to these now and the new simulator's host (tee::SimHost,
// card C) later; nothing above this header knows which of the two it is talking to (docs/plan/w1-b-shell.md, "The
// interfaces you define"; docs/plan/02-contracts.md section 7).
//
// Threads: "UI thread" below is the one thread that runs the window (or the headless script); "audio thread" is the
// device's callback (or, offline, whoever renders). A ControlSink takes a frame from any thread.
#pragma once

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace tee
{
    // One thing a person can set.
    struct ParamInfo
    {
        std::string id, label, unit, group;
        enum class Kind { real, integer, toggle, choice } kind = Kind::real;
        double min = 0, max = 1, def = 0;
        std::vector<std::string> choices;   // a choice's names, from min up
        bool logTaper = false;              // its control's travel is logarithmic (min above 0)
        int decimals = 2;                   // digits shown after the point
        double scale = 1;                   // shown = value * scale (a 0 to 1 value shown as 0 to 100 %)
    };

    // The parameters. UI thread.
    struct ParamSource
    {
        virtual ~ParamSource() = default;
        virtual int count() const = 0;
        virtual const ParamInfo& info(int i) const = 0;
        virtual double get(int i) const = 0;
        virtual void set(int i, double value) = 0;              // reaches the audio thread safely; never blocks
        virtual int findParam(std::string_view id) const = 0;   // -1: none
    };

    // What the machine is doing: the newest whole block's values. UI thread.
    struct TelemetrySource
    {
        virtual ~TelemetrySource() = default;
        virtual int findValue(std::string_view name) const = 0;   // a handle for value(); -1: none (the names: below)
        virtual double value(int handle) const = 0;               // 0 for a handle that is none
        virtual float outputPeak() const = 0;                     // the output's level over the newest block, 0 to 1
        // The output's newest samples, mono (the two channels' mean), oldest first, into mono[0..max): returns how
        // many were written (0: none). A source keeps at least 4096; a spectrum or a scope reads them each frame.
        // (Every Player of the shell implements it. It has a body, answering none, only so that a Player written
        // before it was added still builds.)
        virtual int outputTail(float* mono, int max) const
        {
            (void) mono;
            (void) max;
            return 0;
        }
    };

    // What the driver is doing (docs/plan/02-contracts.md section 4, the part a person's hands and feet give).
    struct ControlsFrame
    {
        int ignition = 0;     // 0 off, 1 on, 2 start (held): a key, as a car's
        float throttle = 0;   // the pedals, 0 up to 1 down
        float brake = 0;
        float clutch = 0;     // 1 is the pedal on the floor
        float handbrake = 0;
        float steer = 0;      // -1 left to 1 right
        int lever = 0;        // an H-pattern's position: -1 R, 0 N, 1 up
        int shiftUp = 0;      // each counts presses: a press is an increase
        int shiftDown = 0;
        int buttons = 0;      // held switches, a bit each
        int mode = 0;
    };

    // Any thread; the newest frame wins.
    struct ControlSink
    {
        virtual ~ControlSink() = default;
        virtual void setControls(const ControlsFrame&) = 0;
    };

    struct AudioSource
    {
        virtual ~AudioSource() = default;
        virtual void prepare(double rate, int maxBlock) = 0;               // audio stopped
        virtual void process(float* l, float* r, int n) noexcept = 0;      // the audio thread: writes n samples to each
    };

    // One engine, all four faces.
    struct Player : ParamSource, TelemetrySource, ControlSink, AudioSource
    {
    };

    // What a Panel's own controls ask for this frame. The shell merges it with the keyboard's and the devices' into
    // the one ControlsFrame it sends: a Panel never calls Player::setControls itself.
    struct PanelControls
    {
        bool startHeld = false;             // its start button is down
        float throttle = -1;                // 0 to 1 while its throttle strip is held; negative: not held
        float brake = -1;
        int shiftUp = 0, shiftDown = 0;     // counts of presses, as ControlsFrame's
        bool leverSet = false;              // its gear control asks for `lever` (-1 R, 0 N, 1 up)
        int lever = 0;
    };

    // What the application gives a Panel besides the Player. UI thread; any function may be empty.
    struct AppServices
    {
        // The Panel's controls, each frame they change.
        std::function<void(const PanelControls&)> panelControls;
        // The merged frame as last sent (to draw pedals).
        std::function<ControlsFrame()> controls;
        // A line for the top bar: what is playing and the audio device. When the program did not compile (or was
        // stopped), the text starts with "error:" and may run to several lines, the compiler's own words after the
        // first: a Panel with one line of room shows the text up to the first newline.
        std::function<std::string()> status;
    };

    // The names a TelemetrySource is asked for, the same whichever engine plays (docs/plan/02-contracts.md section 5,
    // in SI). A source that has no such value answers -1 to findValue().
    namespace telemetry
    {
        inline constexpr std::string_view kState = "engine.state";             // 0 off, 1 cranking, 2 running, 3 running down
        inline constexpr std::string_view kSpeed = "engine.speed";             // the crank, rad/s
        inline constexpr std::string_view kRedline = "engine.redline";         // rad/s
        inline constexpr std::string_view kThrottle = "engine.throttle";       // the plate, 0 to 1
        inline constexpr std::string_view kManifold = "engine.manifold";       // Pa absolute
        inline constexpr std::string_view kCoolant = "engine.coolant";         // K
        inline constexpr std::string_view kOilPressure = "engine.oilPressure"; // Pa over the air outside
        inline constexpr std::string_view kFlags = "engine.flags";             // bit 0 the limiter, bit 1 the fuel cut
        inline constexpr std::string_view kCycle = "engine.cycle";             // a slowed copy of the cycle, 0 to 1
        inline constexpr std::string_view kCarSpeed = "car.speed";             // m/s
        inline constexpr std::string_view kGear = "car.gear";                  // -1 R, 0 N, 1 up
        inline constexpr std::string_view kDriven = "car.driven";              // 1 while the engine is in a vehicle
        inline constexpr std::string_view kCarBrake = "car.brake";             // the pedals as the car has them, 0 to 1
        inline constexpr std::string_view kCarClutch = "car.clutch";
        inline constexpr std::string_view kBoost = "boost.pressure";           // Pa over the air outside
    }
}
