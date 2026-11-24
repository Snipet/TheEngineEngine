// host/include/tee/EnginePlayer.h: the simulator (sim/engine.fm, module Engine) played through its contract
// (docs/plan/02-contracts.md, sections 1 to 5; sim/README.md has the exact fields), single-threaded: for tools and
// tests. Card C's SimHost is the threaded one; this is the plain one it can be checked against.
//
// One Program (sim/engine.fm and what it includes), one Instance of Engine. Every write goes straight into the
// instance, between calls of process: a Spec field (by its path under spec, "engine.displacement"), clamped to its
// fminus::range, with its section's rev moved; the Controls, as a struct written whole before each call; the
// Telemetry read after a call, by path under tel ("engine.speed"). A new player, and reset(), put every Spec and
// Controls field at its fminus::default (FMinus initialises nothing) and run Engine::init.
//
// Also here, until card C grows them: a build (02-contracts.md, section 8: the Spec's values by field name, in SI,
// choices by their names) read from and written to JSON; an engine scenario (a timeline of Controls and Spec changes,
// tests/sim/scenarios/*.json: its format is below); and the offline render of one through a player, into Render.h's
// RenderResult (its columns the Telemetry's paths).
//
// Threads: create(), reset() and the destructor compile, allocate and run init and end: off the audio thread. process
// and the writes and reads by index allocate nothing and take no lock.
#pragma once

#include "tee/Instance.h"
#include "tee/Program.h"
#include "tee/RefPlayer.h"
#include "tee/Render.h"
#include "tee/Schema.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tee
{
    class EnginePlayer
    {
    public:
        struct Options
        {
            std::filesystem::path directory;   // where engine.fm and its files are: sim
            std::string entry = "engine.fm";
            double sampleRate = 48000.0;       // the host's rate (the engine runs at 96 kHz inside, whatever this is)
            int optimisation = 2;              // Program::Options::optimisation
            bool checks = false;               // Program::Options::checks: the loop budget and the bounds checks
        };

        // sim/spec/controls.fm's Controls, field for field: written into the instance before every call.
        struct Controls
        {
            int ignition = 1;          // 0 off, 1 on, 2 start (held)
            float throttle = 0.0f;
            float brake = 0.0f;
            float clutch = 0.0f;       // 1 is the pedal on the floor
            float handbrake = 0.0f;
            float steer = 0.0f;
            int lever = 0;             // -1 R, 0 N, 1 up
            int shiftUp = 0;           // counters: the simulator acts on each increase
            int shiftDown = 0;
            int buttons = 0;
            int mode = 0;              // 0 bench, 1 drive, 2 engine dyno, 3 chassis dyno
            float dynoSpeed = 0.0f;
            float dynoRate = 0.0f;
            float holdSpeed = 0.0f;    // rad/s; MODE_BENCH: above 0, a driver holds this speed
            int strip = 0;
            int scopeOn = 0;
            int bakeMaps = 0;          // a counter: the ECU bakes its maps into the Scope on each increase
        };
        static constexpr int kIgnitionOff = 0, kIgnitionOn = 1, kIgnitionStart = 2;
        static constexpr int kModeBench = 0, kModeDrive = 1;

        // One number of the Spec (an array's elements are one SpecField, `count` of them).
        struct SpecField
        {
            std::string path;                  // under spec: "engine.displacement"
            std::string section;               // "engine"
            const Schema::Node* node = nullptr;
            std::size_t offset = 0;            // in the Engine instance
            std::size_t count = 1;
            std::size_t revOffset = 0;         // its section's rev, in the Engine instance
            bool hasRange = false;
            double minimum = 0.0, maximum = 0.0;
            double defaultValue = 0.0;         // its fminus::default (0 without one)
            std::vector<std::string> choices;  // ui::choices: the names of minimum, minimum + 1, ...
            bool isInt() const noexcept { return node != nullptr && node->scalar == Scalar::Int; }
            bool isBool() const noexcept { return node != nullptr && node->scalar == Scalar::Bool; }
        };

        // Compiles the simulator in `options.directory` and makes it ready: the instance made, the Spec and Controls at
        // their defaults, init run. nullptr, with `errors` holding FMinus's messages or what the contract misses.
        static std::unique_ptr<EnginePlayer> create(const Options& options, std::string& errors);
        ~EnginePlayer();
        EnginePlayer(const EnginePlayer&) = delete;
        EnginePlayer& operator=(const EnginePlayer&) = delete;

        // As create() left it, without compiling again: end run, the instance zeroed, the Spec and the Controls at their
        // defaults, init run at `sampleRate` (0: the rate it has). keepSpec: the Spec as it was, not the defaults.
        void reset(double sampleRate = 0.0, bool keepSpec = false);
        double sampleRate() const noexcept { return sampleRate_; }

        // ---- the Spec -----------------------------------------------------------------------------------------------
        const std::vector<SpecField>& specFields() const noexcept { return specFields_; }
        int specIndex(std::string_view path) const noexcept;
        // Writes a value: clamped to the range, an int rounded, a bool true unless 0; its section's rev moved.
        void setSpec(std::size_t index, double value, std::size_t element = 0) noexcept;
        bool setSpec(std::string_view path, double value, std::string& error);
        double spec(std::size_t index, std::size_t element = 0) const noexcept;
        // Every Spec field at its default, every section's rev moved.
        void setSpecDefaults() noexcept;

        // ---- the Controls -------------------------------------------------------------------------------------------
        Controls& controls() noexcept { return controls_; }

        // ---- playing ------------------------------------------------------------------------------------------------
        // The next n samples at the host's rate into left and right: Engine::process.
        void process(float* left, float* right, int n) noexcept;
        std::int64_t samplesPlayed() const noexcept { return samples_; }

        // ---- the Telemetry ------------------------------------------------------------------------------------------
        const std::vector<std::string>& telemetryNames() const noexcept { return telNames_; }   // "engine.speed"
        double telemetry(std::size_t index) const noexcept;
        int telemetryIndex(std::string_view name) const noexcept;

        // ---- the checks (Options::checks) --------------------------------------------------------------------------
        bool stopped() const noexcept { return stopped_; }
        const std::string& stopReason() const noexcept { return stopReason_; }

        // ---- for tools and tests ------------------------------------------------------------------------------------
        Program& program() noexcept { return *program_; }
        const Schema& schema() const noexcept { return *schema_; }
        Instance& instance() noexcept { return *instance_; }
        // Any field of the Engine instance by path ("mono.set.size", "scope.pressure[3]"), as a double: tests.
        bool read(std::string_view path, double& value, std::string& error) const;

    private:
        EnginePlayer() = default;
        bool bind(std::string& errors);
        void begin(bool keepSpec);
        void end() noexcept;
        void writeControls() noexcept;
        void check(const char* call) noexcept;

        std::unique_ptr<Program> program_;
        std::unique_ptr<Schema> schema_;
        std::unique_ptr<Instance> instance_;
        void (*init_)(void*, float) = nullptr;
        void (*process_)(void*, float*, float*, int) = nullptr;
        void (*end_)(void*) = nullptr;

        std::vector<SpecField> specFields_;
        std::vector<Schema::Handle> ctlHandles_;     // in Controls' order, as writeControls writes them
        std::vector<std::pair<std::string, double>> ctlDefaults_;
        std::vector<std::string> telNames_;
        std::vector<Schema::Handle> telHandles_;
        std::vector<unsigned char> savedSpec_;
        std::size_t specOffset_ = 0, specSize_ = 0;

        Controls controls_;
        double sampleRate_ = 48000.0;
        std::int64_t samples_ = 0;
        std::int64_t budgetPerSample_ = 0;
        bool checks_ = false;
        bool begun_ = false;
        bool stopped_ = false;
        std::string stopReason_;
    };

    // ---- builds (02-contracts.md, section 8) ---------------------------------------------------------------------------
    //
    //   {"format": "tee-build", "version": 1, "name": "V8", "notes": "...",
    //    "spec": {"engine": {"layout": "V8 crossplane", "displacement": 0.0062099, ...}, "boost": {...}, ...}}
    //
    // Values by field name under their section, in SI, numbers as numbers, choices by their names (or numbers), bools
    // as true or false, arrays as arrays. A field the file lacks takes its default; a field the Schema lacks is kept
    // (EngineBuild::unknown) and written back, with a warning.
    struct EngineBuild
    {
        std::string name;
        std::string notes;
        std::map<std::string, std::vector<double>> values;   // by Spec path ("engine.layout"): its elements
        std::map<std::string, std::string> unknown;          // by path: the JSON text of what the Schema lacks
    };

    // Reads a build against the player's Spec, as tee::Build reads it (one reading of a build file: nested objects such
    // as spec.ecu.knock.on are fields, not unknowns). False, with `error` set, when it is no build or a value is no
    // value of its field. A field the program lacks: with `strict` (the default: the test rigs' and tools' call) an
    // error naming each; without, kept (EngineBuild::unknown) and written back, with a line in `warnings` (a newer
    // file on an older program, as the application reads it). `warnings` also gets what was out of its range.
    bool parseBuild(const EnginePlayer& player, std::string_view json, std::string_view fileName, EngineBuild& build,
                    std::string& error, std::string& warnings, bool strict = true);
    bool loadBuild(const EnginePlayer& player, const std::filesystem::path& file, EngineBuild& build, std::string& error,
                   std::string& warnings, bool strict = true);
    // The build as JSON: every Spec field (its value, or its default), choices by name; what was unknown written back.
    std::string buildJson(const EnginePlayer& player, const EngineBuild& build);
    // Every Spec field at its default, then the build's values.
    void applyBuild(EnginePlayer& player, const EngineBuild& build);
    // The player's Spec as a build: every field.
    EngineBuild currentBuild(const EnginePlayer& player, std::string name = {}, std::string notes = {});

    // ---- engine scenarios ----------------------------------------------------------------------------------------------
    //
    //   {"format": "tee-engine-scenario/1", "name": "...", "description": "...", "seconds": 12, "build": "v8",
    //    "events": [
    //      {"t": 0, "ctl": {"mode": 1, "ignition": 2}, "spec": {"car.mass": 1500}},
    //      {"t": 4, "ctl": {"throttle": 0.6}, "ramp": 0.08},        a straight line there, a step each kRampStep s
    //      {"t": 6, "press": ["shiftUp"]},                          a counter's increase: shiftUp, shiftDown, strip
    //      {"t": 9, "note": "a remark for the reader"}]}
    //
    // "build" names a file of content/builds (or is a path); none: the defaults. Events of one time apply in the file's
    // order, before the call that starts at that time.
    struct EngineEvent
    {
        enum class Kind
        {
            Control,   // ctl.<path> takes value
            Spec,      // spec.<path> takes value
            Count,     // ctl.<path>, a counter, goes up by one
        };
        double time = 0.0;
        Kind kind = Kind::Control;
        std::string path;
        double value = 0.0;
    };

    struct EngineScenario
    {
        std::string name;
        std::string description;
        double seconds = 0.0;
        std::string build;                  // as the file names it; empty: the defaults
        std::vector<EngineEvent> events;    // by time
    };

    bool parseEngineScenario(std::string_view json, std::string_view fileName, EngineScenario& scenario,
                             std::string& error);
    bool loadEngineScenario(const std::filesystem::path& file, EngineScenario& scenario, std::string& error);

    // Renders `scenario` from `build` through `player` (reset first, then the build applied): a row of the Telemetry
    // after every call. False, with `error` set, when an event names no field of the program's.
    bool renderEngine(EnginePlayer& player, const EngineBuild& build, const EngineScenario& scenario,
                      const RenderOptions& options, RenderResult& result, std::string& error);

    // ---- the twin: the simulator and the reference from the same inputs (the null test, tee-bench --engine) -----------
    //
    // `engine` takes the build and the scenario as they are. `ref` (sim/ref through RefPlayer) takes them as
    // sim/README.md ("The null test, both ways") says the reference's Voice would have been given them: before each
    // call, Engine::process is called for no samples (it applies the Spec, the Controls and the ignition, and renders
    // nothing), the adapter's Set (the reference's Shared's dials, in its Engine) is read back, and the reference's 32
    // dials and its Host's pedals, lever and buttons are set from it, so both programs' Set hold the same bits; the
    // ignition's start is the reference's start key going down (letting go of it, the key coming up), and what the
    // adapter does to Shared's state without a key (off: running down; the start let go while it cranks: stopped) is
    // written into the reference's Shared the same way. Then both are called for the same samples, each timed.
    struct TwinOptions
    {
        int blockSize = 512;       // the longest call, cut where an event falls
        double seconds = 0.0;      // 0: the scenario's length
        bool keepAudio = true;     // false: time the calls, keep no samples
    };

    struct TwinResult
    {
        Audio engine;              // the simulator's samples
        Audio reference;           // the reference's
        std::int64_t frames = 0;
        std::int64_t soundingFrames = 0;     // samples of the calls in which the reference's engine sounded
        double engineSeconds = 0.0;          // the wall time of those calls, each side's
        double referenceSeconds = 0.0;
        int keyDowns = 0;                    // the reference's start key went down this often
    };

    bool playTwin(EnginePlayer& engine, RefPlayer& ref, const EngineBuild& build, const EngineScenario& scenario,
                  const TwinOptions& options, TwinResult& result, std::string& error);
} // namespace tee
