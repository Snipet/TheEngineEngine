// host/include/tee/Replay.h: a session log played again (card C, section 9). SimHost keeps the log when
// LoadOptions::log is on: every change of the controls and every Spec write, each at the sample it took effect, and
// the Spec's bytes when the log began. Rendering a log again through a fresh host of the same program at the same rate
// gives the same samples: the events land on the same samples, and the simulator's output does not depend on where the
// blocks end. Wave 2's cameras and exports are built on it.
#pragma once

#include "tee/SimHost.h"

#include <filesystem>
#include <string>
#include <vector>

namespace tee
{
    struct SessionLog
    {
        double rate = 48000.0;
        std::int64_t samples = 0;                  // how long the session was
        std::vector<unsigned char> start;          // the main module's bytes the host had written when the log began
        std::vector<LogEvent> events;              // by sample

        // As JSON ({"format": "tee-session", "version": 1, ...}, the bytes in hex), and back.
        std::string json() const;
        static bool parse(std::string_view text, SessionLog& log, std::string& error);
        bool saveFile(const std::filesystem::path& file, std::string& error) const;
        static bool loadFile(const std::filesystem::path& file, SessionLog& log, std::string& error);
    };

    // The log of `host` so far (its LoadOptions::log on), taken from it, `samples` long.
    SessionLog takeSession(SimHost& host, std::int64_t samples);

    // Plays `log` through `host` (fresh, of the program the log was made with, at its rate; LoadOptions::worker off),
    // in blocks of at most `block`, cut where the events fall. False, with `error`, when the log does not fit it.
    bool replay(SimHost& host, const SessionLog& log, int block, std::vector<float>& left, std::vector<float>& right,
                std::string& error);
} // namespace tee
