// host/include/tee/Timeline.h: a scenario for the simulator (docs/formats.md, "A scenario"): a build, a rate and a
// block size, a timeline of the driver's controls and Spec changes at times in seconds (with ramps), and assertions;
// and the offline run that plays it through a SimHost into memory. tee-render, tee-validate, the test library
// (tee/test/Scenario.h) and the dyno runner are built on it.
//
// A timeline is flat once it is made: each event sets one control or one Spec field at one time. A ramp is unrolled
// into steps every kStep seconds from the value before, so two runs of one timeline see the same values at the same
// samples whatever their block size: a block is cut where the next event falls.
#pragma once

#include "tee/Build.h"
#include "tee/SimHost.h"

#include <filesystem>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace tee
{
    struct TimelineEvent
    {
        enum class Kind : std::uint8_t
        {
            Control,   // a control of SimControls by `name` takes `value`
            Press,     // a counter control (shiftUp, shiftDown, strip) goes up by one
            Spec,      // a Spec field by `name` ("engine.bore", or an element "sound.harmonics[2]") takes `value`
            Choice,    // a Spec choice by `name` takes the option named `text`
        };
        double time = 0.0;
        Kind kind = Kind::Control;
        std::string name;
        double value = 0.0;
        std::string text;
        double ramp = 0.0;   // seconds: it moves there in a straight line from the value before, in kStep steps
    };

    // One assertion of a scenario file, checked on a run (checkAssertions).
    struct Assertion
    {
        std::string what;          // "tel" or "audio"
        std::string field;         // tel's path ("engine.speed")
        std::string measure;       // tel: max, min, mean, last, crossing; audio: finite, peak, rms, step
        double from = 0.0, to = -1.0;   // the window, seconds (to < 0: the end)
        std::string op;            // "<", "<=", ">", ">=", "within"
        double value = 0.0;
        double value2 = 0.0;       // within: [value, value2]
        std::string source;        // why: a sentence, a reference
    };

    struct Timeline
    {
        std::string name;
        std::string description;
        std::string build;         // a build file (a path, or a name under content/builds), empty: the defaults
        double rate = 48000.0;
        int block = 512;
        double seconds = 0.0;
        // The engine held warm (Warm.h) from the start: whoever attaches the host for it sets LoadOptions::hold to
        // warmEngine() (tee-render, tee-validate do).
        bool warm = false;
        std::vector<TimelineEvent> events;   // by time; of one time in the order written
        std::vector<Assertion> assertions;

        static constexpr double kStep = 0.005;   // a ramp's steps

        // Sorts by time, keeping the order of events of one time.
        void sort();
    };

    // Reads a scenario file ({"format": "tee-scenario", "version": 1, ...}). False, with `error`, when it is not one.
    bool parseTimeline(std::string_view json, std::string_view fileName, Timeline& timeline, std::string& error);
    bool loadTimeline(const std::filesystem::path& file, Timeline& timeline, std::string& error);

    struct RunOptions
    {
        double seconds = 0.0;      // 0: the timeline's
        bool keepAudio = true;
        // Telemetry kept after every block: these tel paths ("engine.speed" or "tel.engine.speed"); empty: every
        // number of the tel block (an array's elements are not kept).
        std::vector<std::string> telemetry;
        // Called after every block with the newest frame (the dyno reads its points through it). May be empty.
        std::function<void(double time, const TelemetryFrame&)> afterBlock;
        // Checked before every block: true stops the run where it is (a cancelled dyno run).
        std::function<bool()> cancelled;
    };

    struct RunResult
    {
        double rate = 48000.0;
        std::vector<float> left, right;
        std::vector<std::string> columns;   // tel paths, without "tel."
        std::vector<double> rowTime;        // seconds at each block's end
        std::vector<float> rows;            // row r, column c: rows[r * columns.size() + c]
        HostStats host;                     // the last block's
        double wallSeconds = 0.0;           // how long the run took
        double simulatedSeconds = 0.0;
        std::string stopReason;             // a trip or a stop's words, if any
        bool cancelled = false;

        int column(std::string_view path) const noexcept;
        std::size_t rowCount() const noexcept { return rowTime.size(); }
        float at(std::size_t row, std::size_t col) const noexcept { return rows[row * columns.size() + col]; }
    };

    // Plays `timeline` through `host` (made with LoadOptions::worker off, so that the run calls service() itself and
    // is the same every time), its Spec changes through `build` (attached to it). The host should be fresh (or just
    // prepared): the run starts at its present state. False, with `error`, when an event names nothing.
    bool runTimeline(SimHost& host, Build& build, const Timeline& timeline, const RunOptions& options, RunResult& result,
                     std::string& error);

    // The assertions' outcomes, a line each ("pass engine.speed max 6..8 > 600: 652.1"), and whether all passed.
    bool checkAssertions(const Timeline& timeline, const RunResult& run, std::vector<std::string>& lines);
} // namespace tee
