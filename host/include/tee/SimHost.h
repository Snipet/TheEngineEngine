// host/include/tee/SimHost.h: the threaded host of 02-contracts.md, section 7. It plays one instance of the
// simulator's main module (Sim.h) on the audio thread, and takes what every other thread asks of it without making
// the audio thread wait for anybody:
//
//   writes to the Spec     a ring of commands (Commands.h), drained before each block, each section's `rev` bumped
//                          once a drain; a full ring refuses the write (set() false), and the caller tries again;
//   the driver's controls  a seqlock: the newest frame wins;
//   telemetry              after each block, the `tel` block's bytes and the host's numbers through a seqlock;
//   the scope              copied, at most about 60 times a second, only while someone asks (ctl.scopeOn says so).
//
// Safety, on the audio thread: flush-to-zero; an output guard (a sample that is not finite or is beyond the limit,
// full scale by default: the block is not played, it fades out from the last sample played, and the instance is made
// afresh off the audio thread, with a flag raised: never a blast to the speakers); with a Sim compiled with the checks
// (development), a call FMinus's loop budget or bounds checks stopped is handled the same way, with the compiler's
// words kept (stopReason()). A load meter: the callback's time over its buffer's, mean and peak, and the host's own
// time besides Engine::process, in the telemetry; and a verdict on it (HostStats::fits, Fit.h: 1 fits, 0 tight, -1
// over, with hysteresis; lateCallbacks counts the callbacks that took longer than their buffer), worked out by
// service(), which never acts on it: the interface shows it and names the Economy quality.
//
//   tee::SimHost host;
//   std::string errors;
//   if (!host.load("sim", {}, errors)) report(errors);
//   auto speed = host.fields().ref("tel.engine.speed", errors);
//   host.set(host.fields().ref("spec.engine.idle", errors), 90.0);   // UI thread: queued, clamped to its range
//   host.prepare(48000, 512);                                        // audio stopped
//   host.process(left, right, n);                                    // the audio thread
//   tee::TelemetryFrame t;  host.telemetry(t);  t.get(speed);        // UI thread
//
// Instances are the host's: made, init'd and ended off the audio thread (by service(), which a thread of the host's
// own calls every few milliseconds, or the caller when LoadOptions::worker is off: an offline render calls it between
// blocks), handed to the audio thread through an atomic pointer, and handed back the same way.
#pragma once

#include "tee/Commands.h"
#include "tee/Fields.h"
#include "tee/Fit.h"
#include "tee/Instance.h"
#include "tee/Sim.h"
#include "tee/Warm.h"
#include "tee/Telemetry.h"

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace tee
{
    // What the driver does: 02-contracts.md, section 4, field for field. Each is written to the `ctl` field of its
    // name, when the program has one, before every block.
    struct SimControls
    {
        std::int32_t ignition = 0;     // 0 off, 1 on, 2 start (held)
        float throttle = 0.0f;
        float brake = 0.0f;
        float clutch = 0.0f;           // 1: the pedal on the floor
        float handbrake = 0.0f;
        float steer = 0.0f;
        std::int32_t lever = 0;        // -1 R, 0 N, 1 up
        std::int32_t shiftUp = 0;      // counters: a press is an increase
        std::int32_t shiftDown = 0;
        std::int32_t buttons = 0;
        std::int32_t mode = 0;         // MODE_BENCH, MODE_DRIVE, MODE_DYNO_ENGINE, MODE_DYNO_CHASSIS
        float dynoSpeed = 0.0f;        // rad/s
        float dynoRate = 0.0f;         // rad/s^2
        float holdSpeed = 0.0f;        // rad/s
        std::int32_t strip = 0;
        std::int32_t scopeOn = 0;      // the host sets it too while someone asks for the scope
        std::int32_t bakeMaps = 0;     // a counter: the ECU bakes its maps at each increase (the TUNE workspace's BAKE)

        bool operator==(const SimControls&) const = default;
    };

    // The names of SimControls' fields, in order, and each one's place in it, for whoever reads or writes them by name.
    struct ControlField
    {
        const char* name;
        std::size_t offset;    // in SimControls
        bool isFloat;
    };
    const std::vector<ControlField>& controlFields();
    // A control by name, as a double (false when there is none of that name).
    bool setControl(SimControls& controls, std::string_view name, double value) noexcept;
    bool getControl(const SimControls& controls, std::string_view name, double& value) noexcept;

    // An event of the session log (Replay.h): what reached the simulator, at which sample.
    struct LogEvent
    {
        enum class Kind : std::uint8_t
        {
            Spec,       // `bytes` written at `offset` (a section's rev bumped after, as the host does)
            Controls,   // the controls became `controls`
        };
        std::int64_t sample = 0;       // the session's sample at which it took effect (a block's first)
        Kind kind = Kind::Spec;
        std::uint32_t offset = 0;
        std::int16_t section = -1;
        std::vector<unsigned char> bytes;
        SimControls controls;
    };

    struct LoadOptions
    {
        Sim::Options sim;
        double sampleRate = 48000.0;
        int maxBlock = 1024;                    // the longest call of Engine::process (a longer block is cut)
        std::size_t commandSlots = 16384;
        std::size_t poolBytes = std::size_t{1} << 21;
        // The output's safety stage. A sample beyond guardLimit (the contract's +-1) but finite and within absurdLimit
        // is a loud moment, not a fault: clamped to guardLimit, its block counted (HostStats::clippedBlocks) and the
        // largest sample kept (HostStats::largestSample); nothing restarts. A sample not finite, or beyond absurdLimit
        // (16: +24 dB over full scale), is a fault: the block silenced and a fresh instance made off the audio thread.
        float guardLimit = 1.0f;
        float absurdLimit = 16.0f;
        // After a fault's fresh instance, an ignition the driver left on cranks it (ctl.ignition 2, written by the
        // host) until it runs (tel.engine.state 2) or for at most crankSeconds: the engine comes back as the person
        // left it, not silently off. 0: it does not (the instance waits, stopped, for the driver's start).
        double restartCrankSeconds = 3.0;
        bool worker = true;                     // a thread of the host's own calls service()
        bool log = false;                       // keep the session log (takeLog)
        double crossfadeSeconds = 0.02;         // a reload's crossfade
        double fadeInSeconds = 0.02;            // a fresh instance's fade in, after a trip
        // State written into the instance before each call of Engine::process (Warm.h: a warm engine, held): for
        // offline runs, tests and tools, never the application's engine. A path the program lacks is skipped.
        std::vector<HeldValue> hold;
    };

    class SimHost
    {
    public:
        SimHost();
        ~SimHost();
        SimHost(const SimHost&) = delete;
        SimHost& operator=(const SimHost&) = delete;

        // Compiles the simulator in `simDir` and makes an instance of it, its Spec at the defaults, init run at the
        // options' rate. False, with `errors` holding clang's words (or what the program lacks of the contract).
        bool load(const std::filesystem::path& simDir, const LoadOptions& options, std::string& errors);
        // The same over a Sim already compiled (one shared with other hosts).
        bool attach(std::shared_ptr<Sim> sim, const LoadOptions& options, std::string& errors);

        bool loaded() const noexcept { return sim_ != nullptr; }
        const Schema& schema() const noexcept { return sim_->schema(); }
        const Fields& fields() const noexcept { return sim_->fields(); }
        std::shared_ptr<Sim> sim() const { return sim_; }
        const LoadOptions& options() const noexcept { return options_; }
        double sampleRate() const noexcept { return rate_; }
        int maxBlock() const noexcept { return maxBlock_; }

        // ---- any thread but the audio thread ---------------------------------------------------------------------
        // Queues a write of a Spec field (or an element of one), clamped to its range, applied before the next block.
        // False when the ref is of another program (a reload made it stale), names no Spec field, or the queue is
        // full (counted: the caller tries again next frame).
        bool set(FieldRef specField, double value, std::size_t index = 0);
        // The bytes of a field as the ref names it (a whole array, a curve, a map), written as they are: n must be
        // the ref's size. Not clamped.
        bool setBytes(FieldRef specField, const void* data, std::size_t n);
        // The driver's controls: the newest frame wins.
        void setControls(const SimControls& controls);
        SimControls controls() const;
        // The newest whole block's telemetry; false while no block has been played (or a frame could not be read
        // whole, which a reader takes as "keep the last").
        bool telemetry(TelemetryFrame& out) const;
        // Asks for the scope (for about half a second from now), and gives the newest one copied, if any.
        bool scope(ScopeFrame& out);
        // What the Spec is as the host has sent it: the bytes of the main module the commands have written so far
        // (every other byte 0), for a document that wants to read back, and what a fresh instance starts from.
        bool readSpec(FieldRef specField, std::vector<unsigned char>& bytes) const;
        double readSpec(FieldRef specField, std::size_t index = 0) const;

        // The words of the last stop (a check, with FMinus's message) or guard trip; empty when there was none.
        std::string stopReason() const;

        // A reload (development): `next`'s instance made and init'd here, the playing instance's state carried over
        // by field name and type (Reload.h), the Spec bytes the host sent written over it, each section's rev bumped;
        // the audio thread crossfades to it, and the old one is freed off it. False, with `errors`, when the new
        // program's tel or scope block outgrew the room the host gave them at load.
        bool reload(std::shared_ptr<Sim> next, std::string& errors);

        // The engine's quality (the lead's decision, Q's measures): Full, the simulator at 96 kHz inside (the
        // default); Economy, at 48 kHz (sim/engine.fm with TEE_ENGINE_RATE_48000: 0.49 of the work, torque within
        // some 1.2 %, the top octave lost: bands from about 10 kHz 13 to 24 dB down). A change compiles the program
        // again (seconds: call it off the UI thread too) and swaps it in under the reload's crossfade: the Spec is
        // carried, the engine's state is not (its rates differ), and an ignition left on cranks it back. Never
        // switched by the host itself. False, with `errors`, when the program does not compile, was compiled from
        // memory, or does not take the define. A development reload (Reload.h) compiles at the options it was given.
        // A build on the edge of lighting (a diesel's charge that only just lights, the owner's large diesel between
        // 1300 and 1650 rpm) may sit on the other branch (lit or misfiring) after a quality change, as after any
        // reload: the rate does not decide it, the history does.
        enum class Quality
        {
            Full,
            Economy
        };
        bool setQuality(Quality quality, std::string& errors);
        Quality quality() const noexcept { return quality_; }
        // The engine's own rate inside (Engine::rate) as the last quality change found it; 96000 until one.
        double innerRate() const noexcept { return innerRate_; }

        // The session log since load (or the last take), when LoadOptions::log is on. Off the audio thread.
        std::vector<LogEvent> takeLog();
        // The Spec as the log began: the module's bytes the host had written when it was loaded.
        const std::vector<unsigned char>& logStart() const noexcept { return logStart_; }

        // Frees what the audio thread handed back, makes a fresh instance after a trip or a stop, drains the log.
        // Off the audio thread; the host's own thread calls it unless LoadOptions::worker is off.
        void service();

        // ---- the audio thread ------------------------------------------------------------------------------------
        // For a new rate or longest block: with the audio stopped (no process() meanwhile). The instance is made
        // afresh at the new rate, its Spec what the host has sent.
        void prepare(double sampleRate, int maxBlock);
        // Writes `frames` samples to both buffers: the queue drained, the controls written, Engine::process (in calls
        // of at most maxBlock), the guard, the telemetry published. No allocation, no lock, no wait.
        void process(float* left, float* right, int frames) noexcept;

    private:
        struct Live;
        Live* makeLive(const std::shared_ptr<Sim>& sim, std::string& errors);
        void destroyLive(Live* live);
        void retire(Live* live) noexcept;
        bool runChunk(Live& live, float* left, float* right, int n) noexcept;
        void publish(Live& live) noexcept;
        void writeControls(Live& live) noexcept;
        void applyCommands(Live& live) noexcept;
        void logEvent(LogEvent::Kind kind, std::uint32_t offset, std::int16_t section, const void* bytes,
                      std::uint32_t size) noexcept;
        void workerLoop();
        void stopWorker();

        std::shared_ptr<Sim> sim_;
        LoadOptions options_;
        double rate_ = 48000.0;
        int maxBlock_ = 1024;

        // the producers' side
        mutable std::mutex producers_;
        std::unique_ptr<CommandQueue> commands_;
        std::vector<unsigned char> shadow_;   // the module's bytes as the commands wrote them
        std::uint32_t generation_ = 0;        // the Fields commands are resolved against
        mutable std::mutex controlsWriters_;
        SimControls controlsSent_;
        SeqBuffer controlsBuffer_;

        // the audio thread's
        Live* live_ = nullptr;
        Live* fading_ = nullptr;              // a reload's old instance while the crossfade runs
        int fadeTotal_ = 0;
        int fadeLeft_ = 0;
        int fadeInTotal_ = 0;
        int fadeInLeft_ = 0;
        SimControls controlsApplied_;
        float lastLeft_ = 0.0f, lastRight_ = 0.0f;
        bool reloadWith(std::shared_ptr<Sim> next, bool carry, std::string& errors);
        Quality quality_ = Quality::Full;
        double innerRate_ = 96000.0;
        std::int64_t crankLeft_ = 0;          // samples the host still cranks a fresh instance after a fault
        bool clippedNow_ = false;             // a sample of this chunk was clamped
        std::atomic<std::uint32_t> engineFaultsNow_{0};   // HostStats::engineFaults, for service()'s words
        std::uint32_t engineFaultsTold_ = 0;              // service()'s: the count stopReason last said
        std::vector<float> scratchLeft_, scratchRight_;
        std::vector<unsigned char> publishBuffer_;
        HostStats stats_;
        double peakWindow_ = 0.0;             // seconds into the load peak's window
        double windowPeak_ = 0.0;
        std::int64_t lastScopeSample_ = -1000000;
        std::uint32_t sectionsTouched_ = 0;   // bits, during a drain

        // handovers
        std::atomic<Live*> pending_{nullptr};
        static constexpr int kRetireSlots = 8;
        std::atomic<Live*> retired_[kRetireSlots] = {};
        Live* retireLater_ = nullptr;         // the audio thread's, when every slot is full
        std::atomic<bool> restartWanted_{false};
        std::atomic<int> pendingIsReload_{0};

        // published
        SeqBuffer telemetryBuffer_;
        std::size_t telemetryRoom_ = 0;       // tel bytes the buffer has room for
        SeqBuffer scopeBuffer_;
        std::size_t scopeRoom_ = 0;
        std::atomic<std::int64_t> scopeAskedAt_{-1};   // the sample a reader last asked at
        std::atomic<std::int64_t> samplesNow_{0};
        // the load verdict (Fit.h): the audio thread's peak and late count in, service()'s verdict out
        std::atomic<double> loadPeakNow_{0.0};
        std::atomic<std::uint32_t> lateNow_{0};
        std::atomic<std::int32_t> fitsNow_{1};
        FitMeter fitMeter_;                      // service()'s
        std::int64_t fitSamples_ = 0;            // service()'s: the samples at its last update

        // a reload's snapshot of the playing instance, taken by the audio thread at a block's end
        std::vector<unsigned char> snapshot_;
        std::atomic<int> snapshotState_{0};   // 1 asked, 2 taken

        // the log
        struct LogRing;
        std::unique_ptr<LogRing> logRing_;
        std::vector<LogEvent> log_;
        std::mutex logMutex_;
        std::vector<unsigned char> logStart_;

        // the stop's words (set off the audio thread)
        mutable std::mutex reasonMutex_;
        std::string stopReason_;
        std::atomic<int> tripKind_{0};        // 1 guard, 2 loop budget, 3 bounds
        std::atomic<std::uint32_t> refused_{0};
        int restartsInARow_ = 0;
        double lastRestart_ = -1e9;

        std::thread worker_;
        std::atomic<bool> workerStop_{false};
        std::mutex serviceMutex_;
    };
} // namespace tee
