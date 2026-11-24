// host/include/tee/RefPlayer.h: the reference engine (sim/ref/program.fm), played as TestingPlayground's synth plays
// it. This is deliberately the old contract, kept so that the port to the new one can be held to it: nothing new is
// designed here.
//
// The contract (TestingPlayground's Source/fminus/Patch.h, lines 1 to 45), as this class keeps it:
//   - one Shared, made once: its init(rate) runs before the voice's, its end() after;
//   - one Host (host.fm), which the player fills before every call of start, stop and process;
//   - one Voice (the synth has sixteen; the engine is played by the key that went down last, so one is the engine),
//     whose fields of type Shared* and Host* point at those two;
//   - the 32 dials are the Voice's `param`s, set by name through FMinus's setValue;
//   - start(note, velocity), stop(), and bool process(left, right, n) into buffers that arrive at 0; process is called
//     only while the voice sounds: from a start until a process returns false;
//   - what the panel shows is Shared's show* fields, read after each call.
// The synth calls each method through a function of its own, appended to the program and marked
// [[clang::nonblocking]], so that FMinus proves the method cannot reach malloc, calloc or free. So does this: a
// RefPlayer that was created is that proof for start, process and stop.
//
// Two things the synth does that this does not: it smooths a moving float dial into a stream of samples (here a dial
// is a knob, read when the next call starts), and it calls process in parts of at most 16 samples (here a call is as
// long as the caller's block).
//
// Threads: create(), reset() and the destructor compile, allocate and run init and end: off the audio thread. start,
// stop, process, setDial by index, input() and show() allocate nothing and take no lock.
#pragma once

#include "tee/Instance.h"
#include "tee/Program.h"
#include "tee/Schema.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tee
{
    class RefPlayer
    {
    public:
        struct Options
        {
            std::filesystem::path directory;   // where program.fm and its files are: sim/ref
            double sampleRate = 48000.0;       // the host's rate (the engine runs at 96 kHz inside, whatever this is)
            int optimisation = 2;              // Program::Options::optimisation
            bool checks = false;               // Program::Options::checks: the loop budget and the bounds checks
        };

        // What the host and the player are doing: host.fm's Host, written into it before each call.
        struct Input
        {
            float throttle = 0.0f;     // the pedals, 0 released to 1
            float brake = 0.0f;
            float clutch = 0.0f;
            float steering = 0.0f;     // -1 left to 1 right (the reference does not read it)
            int gear = 0;              // the lever: -1 reverse, 0 neutral, 1 to 7
            int buttons = 0;           // a bit each (Buttons, below)
            float modwheel = 0.0f;     // 0 to 1: the throttle by hand
            float pitchbend = 0.0f;    // -1 to 1
            float aftertouch = 0.0f;   // 0 to 1
            bool sustain = false;
        };

        // The wheel's buttons the reference reads, as bits of Input::buttons (host.fm lists all of them).
        struct Buttons
        {
            static constexpr int paddleUp = 1 << 4;     // the right paddle: a gear up
            static constexpr int paddleDown = 1 << 5;   // the left paddle: a gear down
            static constexpr int start = 1 << 17;       // the PS button; the synth also holds note 36 while it is down
        };

        // A dial: one of the Voice's parameters.
        struct Dial
        {
            std::string name;          // "size"
            std::string type;          // "float" or "int"
            double minimum = 0.0;      // its fminus::range
            double maximum = 0.0;
            double defaultValue = 0.0; // its fminus::default
        };

        // Compiles the reference in `options.directory` and makes it ready to play: instances made, pointers set,
        // dials at their defaults, Shared's init run. nullptr, with `errors` holding FMinus's messages (or what is
        // missing of the contract), when it cannot.
        static std::unique_ptr<RefPlayer> create(const Options& options, std::string& errors);
        ~RefPlayer();
        RefPlayer(const RefPlayer&) = delete;
        RefPlayer& operator=(const RefPlayer&) = delete;

        // As create() left it, without compiling again: end run, the instances zeroed, the dials at their defaults,
        // the input cleared, init run at `sampleRate` (0: the rate it has).
        void reset(double sampleRate = 0.0);

        double sampleRate() const noexcept { return sampleRate_; }

        // ---- the dials ------------------------------------------------------------------------------------------
        const std::vector<Dial>& dials() const noexcept { return dials_; }
        // The dial's index in dials(), or -1.
        int dialIndex(std::string_view name) const noexcept;
        // Sets a dial: an atomic store the next call reads. FMinus neither clamps to the range nor checks it, and
        // neither does this. By name: false when the reference has no such dial.
        bool setDial(std::string_view name, double value) noexcept;
        void setDial(std::size_t index, double value) noexcept;
        double dial(std::size_t index) const noexcept;
        void setDefaults() noexcept;

        // ---- playing --------------------------------------------------------------------------------------------
        // The pedals, lever, buttons and wheels: change it between calls; each call reads it as it starts.
        Input& input() noexcept { return input_; }

        // A key goes down (a MIDI note number, velocity 0 to 1): the voice sounds from here on. A second start while
        // a key is held is the voice taken for a new note, as the synth does it.
        void start(float note, float velocity) noexcept;
        // The key goes up.
        void stop() noexcept;
        // The next n samples at the host's rate, written to left and right (which need not arrive at 0: they are
        // zeroed here). True while the voice sounds; once the reference says it is silent for good, process writes
        // zeros and calls nothing until the next start.
        bool process(float* left, float* right, int n) noexcept;
        bool sounding() const noexcept { return sounding_; }
        std::int64_t samplesPlayed() const noexcept { return samples_; }

        // ---- what the panel shows -------------------------------------------------------------------------------
        // Shared's show* fields, in declaration order, as they stood after the last call.
        const std::vector<std::string>& showNames() const noexcept { return showNames_; }
        float show(std::size_t index) const noexcept { return shared_->get(showFields_[index]); }
        // The show field of that name ("showRpm"), or -1.
        int showIndex(std::string_view name) const noexcept;

        // ---- the checks (Options::checks) -----------------------------------------------------------------------
        // True once a call ran past its loop budget or failed a bounds check: the program plays no more (process
        // writes zeros) until reset(). stopReason() says which call, and FMinus's message.
        bool stopped() const noexcept { return stopped_; }
        const std::string& stopReason() const noexcept { return stopReason_; }

        // ---- for tools and tests --------------------------------------------------------------------------------
        Program& program() noexcept { return *program_; }
        const Schema& sharedSchema() const noexcept { return *sharedSchema_; }
        const Schema& voiceSchema() const noexcept { return *voiceSchema_; }
        Instance& shared() noexcept { return *shared_; }
        Instance& voice() noexcept { return *voice_; }

    private:
        RefPlayer() = default;
        bool bind(std::string& errors);
        void begin();
        void end() noexcept;
        void writeHost() noexcept;
        void budget(int samples) noexcept;
        void check(const char* call) noexcept;

        std::unique_ptr<Program> program_;
        std::unique_ptr<Schema> sharedSchema_;
        std::unique_ptr<Schema> voiceSchema_;
        std::unique_ptr<Schema> hostSchema_;
        std::unique_ptr<Instance> shared_;
        std::unique_ptr<Instance> voice_;
        std::unique_ptr<Instance> host_;

        // the reference's methods, by their C types
        void (*sharedInit_)(void*, float) = nullptr;
        void (*sharedEnd_)(void*) = nullptr;
        void (*voiceInit_)(void*, float) = nullptr;    // optional (the reference's Voice has none)
        void (*voiceEnd_)(void*) = nullptr;            // optional
        void (*start_)(void*, float, float) = nullptr;
        bool (*process_)(void*, float*, float*, int) = nullptr;
        void (*stop_)(void*) = nullptr;

        struct HostFields
        {
            Field<float> bpm, beat, barStart, modwheel, pitchbend, aftertouch, steering, throttle, brake, clutch;
            Field<std::int32_t> beatsPerBar, beatUnit, gear, buttons;
            Field<bool> playing, sustain;
        };
        HostFields hostFields_;

        std::vector<Dial> dials_;
        std::vector<const fminus::Program::Parameter*> dialParameters_;
        std::vector<std::string> showNames_;
        std::vector<Field<float>> showFields_;

        Input input_;
        double sampleRate_ = 48000.0;
        std::int64_t samples_ = 0;
        std::int64_t budgetPerSample_ = 0;
        bool checks_ = false;
        bool begun_ = false;
        bool sounding_ = false;
        bool stopped_ = false;
        std::string stopReason_;
    };
} // namespace tee
