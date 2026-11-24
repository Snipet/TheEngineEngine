// host/include/tee/Render.h: the offline renderer: a preset and a scenario played through the reference player into
// memory, with what the panel showed after every call and how long the calls took. tee-render writes its result to
// files; tee-bench reads its times; the tests assert on its samples and its rows.
//
// A render depends on the preset, the scenario, the player's sample rate and the block size, and on nothing else: it
// starts from a reset player, and an event takes effect at its own sample (its time times the rate, rounded) whatever
// the block size, because a call is cut short where the next event falls. So the block size is the longest call, not
// every call's length.
#pragma once

#include "tee/RefPlayer.h"
#include "tee/Scenario.h"
#include "tee/Wav.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace tee
{
    struct RenderOptions
    {
        int blockSize = 512;       // the longest call, in samples at the player's rate
        double seconds = 0.0;      // how long to render; 0: the scenario's own length
        bool keepAudio = true;     // false: time the calls and keep the rows, but not the samples (tee-bench)
    };

    struct RenderResult
    {
        Audio audio;                         // the samples (empty with keepAudio off)

        // What the panel showed: a row after every call. Row r's values are rows[r * columns.size() + c].
        std::vector<std::string> columns;    // Shared's show* fields: "showRpm", ...
        std::vector<double> rowTime;         // seconds from the start, at the row's call's end
        std::vector<float> rows;
        std::size_t rowCount() const noexcept { return rowTime.size(); }
        float at(std::size_t row, std::size_t column) const noexcept { return rows[row * columns.size() + column]; }
        // The column of a name ("showRpm"), or -1.
        int column(std::string_view name) const noexcept;

        // The calls, timed: by the steady clock (wall time, which a busy machine lengthens) and by the thread's own
        // processor time (Clock.h), which is what the code cost.
        std::int64_t frames = 0;             // samples rendered
        std::int64_t calls = 0;              // calls of process
        std::int64_t soundingFrames = 0;     // samples of the calls in which the engine ran (the voice sounded)
        double soundingSeconds = 0.0;        // the wall time those calls took
        double soundingCpuSeconds = 0.0;     // the processor time they took
        double worstCallShare = 0.0;         // the slowest such call: its wall time over its samples' duration
        double peak = 0.0;                   // the largest sample's magnitude, either channel

        // Set when a check stopped the program (Options::checks): the render ends there, silent from then on.
        bool stopped = false;
        std::string stopReason;
    };

    // Renders `scenario` with `preset`'s dials through `player`, which is reset first. False, with `error` set, when
    // the preset or the scenario names a dial the program does not have.
    bool render(RefPlayer& player, const Preset& preset, const Scenario& scenario, const RenderOptions& options,
                RenderResult& result, std::string& error);
} // namespace tee
