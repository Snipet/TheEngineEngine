// host/include/tee/Validation.h: what a build must measure as, and the run that checks it (card C, section 8).
//
// content/validation/<build>.json (docs/formats.md, "A validation"):
//
//   {"format": "tee-validation", "version": 1, "build": "v8", "description": "...",
//    "targets": [
//      {"name": "Warm idle", "scenario": "idle", "tel": "engine.speed", "measure": "mean", "during": [8, 10],
//       "range": [62.8, 94.2], "unit": "rad/s", "fast": true,
//       "source": "a warm V8's idle, 600 to 900 rpm: what makers specify for their V8s"},
//      {"name": "Peak torque", "dyno": {"from": 150, "to": 650, "rate": 60}, "measure": "maxTorque",
//       "range": [500, 680], "unit": "N m", "source": "..."}]}
//
// A target is measured on a scenario (a name under content/scenarios/, or a path) by a telemetry field (max, min,
// mean, last over `during`) or by the audio (peak, rms in dB, step), or on the dyno (a DynoPlan's fields) by maxTorque,
// maxPower, speedAtMaxTorque, speedAtMaxPower or torqueAt (with "at": a speed). Each says where its range comes from in
// words (a textbook's figure, a rule of thumb, a published specification of the kind of engine it stands for). A
// scenario's target may be "relativeTo" a telemetry field (its last value in the same run): a limiter's hold is a
// fraction of the build's own redline. A target
// the program cannot be measured for yet (a telemetry field it lacks) is "not measurable", which is no failure. Each
// scenario and dyno plan is run once for all the targets that share it.
#pragma once

#include "tee/Dyno.h"
#include "tee/Timeline.h"

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace tee
{
    struct ValidationTarget
    {
        std::string name;
        std::string scenario;          // a scenario's name or path (one of scenario and dyno)
        bool dyno = false;
        DynoPlan plan;                 // the dyno's
        std::string tel;               // a scenario's telemetry field, or empty for the audio
        std::string audio;             // peak, rms, step
        std::string measure;           // tel: max, min, mean, last; dyno: maxTorque, maxPower, speedAtMaxTorque, ...
        double from = 0.0, to = -1.0;  // a scenario's window, seconds (to < 0: its end)
        double at = 0.0;               // torqueAt's speed
        std::string relativeTo;        // a tel field: the measure is divided by its last value in the same run
        double low = 0.0, high = 0.0;  // the range
        std::string unit;
        std::string source;
        bool fast = false;             // in ctest's subset
    };

    struct Validation
    {
        std::string build;             // a build's name under content/builds/, or a path
        std::string description;
        std::vector<ValidationTarget> targets;

        static bool parse(std::string_view json, std::string_view fileName, Validation& validation, std::string& error);
        static bool loadFile(const std::filesystem::path& file, Validation& validation, std::string& error);
    };

    struct ValidationRow
    {
        const ValidationTarget* target = nullptr;
        std::optional<double> measured;   // empty: not measurable (why says why)
        bool pass = false;
        std::string why;
    };

    // Runs `validation` (only its fast targets when `fastOnly`) on `sim`, its build and scenarios found under
    // `content`. False, with `error`, when the build or a scenario cannot be read.
    bool runValidation(const std::shared_ptr<Sim>& sim, const Validation& validation, const std::filesystem::path& content,
                       bool fastOnly, std::vector<ValidationRow>& rows, std::string& error);

    // The rows as a table: target, range, measured, pass, source.
    std::string validationTable(const std::vector<ValidationRow>& rows);
} // namespace tee
