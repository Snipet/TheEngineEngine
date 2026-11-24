// tests/ui/standin/tee/app/Sources.h: a stand-in for shell/include/tee/app/Sources.h, the interfaces card B defines
// (docs/plan/w1-b-shell.md, "The interfaces you define"), copied from the card's text word for word so that ui/ builds
// and tests alone before the shell's header exists. ui/CMakeLists.txt puts this folder on the include path only when
// shell/include/tee/app/Sources.h is not in the tree; once it is, this file is not used. It is never a second
// definition in one build.
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace tee
{
    struct ParamInfo            // one thing a person can set
    {
        std::string id, label, unit, group;
        enum class Kind { real, integer, toggle, choice } kind = Kind::real;
        double min = 0, max = 1, def = 0;
        std::vector<std::string> choices;
        bool logTaper = false; int decimals = 2; double scale = 1;
    };
    struct ParamSource          // UI thread
    {
        virtual int count() const = 0;
        virtual const ParamInfo& info(int i) const = 0;
        virtual double get(int i) const = 0;
        virtual void set(int i, double value) = 0;       // reaches the audio thread safely; never blocks
        virtual int find(std::string_view id) const = 0; // -1: none
    };
    struct TelemetrySource      // UI thread: the newest whole block's values
    {
        virtual int find(std::string_view name) const = 0;
        virtual double value(int handle) const = 0;
        virtual float outputPeak() const = 0;            // the output's level, 0 to 1
    };
    struct ControlsFrame { int ignition = 0; float throttle = 0, brake = 0, clutch = 0, handbrake = 0, steer = 0;
                           int lever = 0, shiftUp = 0, shiftDown = 0, buttons = 0, mode = 0; };
    struct ControlSink { virtual void setControls(const ControlsFrame&) = 0; };       // any thread; newest wins
    struct AudioSource { virtual void prepare(double rate, int maxBlock) = 0;         // audio stopped
                         virtual void process(float* l, float* r, int n) noexcept = 0; }; // the audio thread
    struct Player : ParamSource, TelemetrySource, ControlSink, AudioSource {};        // one engine, all four faces
}
