// shell/player/SimAdapter.h: the simulator (sim/engine.fm, played by card C's tee::SimHost) as a Player.
//
//   Parameters   the Spec's fields a person sets one number for (a number, an integer, a switch, a choice; not an
//                array, a curve or a map: those are the interface package's, through tee::Build), by their path
//                without "spec." ("engine.displacement"), with what the program says of each (02-contracts.md
//                section 3). The values live in a tee::Build, the UI thread's document, which sends each change to
//                the host; a quantity is shown in a usual unit (litres, rpm, bar, percent) by the ParamInfo's scale.
//   Telemetry    the tel block's fields by their path without "tel." ("engine.speed": Sources.h's names are these),
//                from the newest whole block the host published.
//   Controls     the frame as the simulator's Controls (the same fields).
//
// Threads: the UI thread sets and reads parameters and telemetry and calls frame() once a frame; process() is the
// audio thread's; prepare() runs with the audio stopped. setControls() may come from any thread (the host guards it).
#pragma once

#include "player/Boxes.h"
#include "tee/app/Sources.h"

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace tee
{
    class SimHost;
    struct FieldInfo;
    struct TelemetryFrame;
    struct HostStats;
    struct LoadOptions;
    class Program;
    class Schema;
    class Fields;
    class Sim;
    class Build;
}

namespace tee::player
{
    class SimAdapter final : public Player
    {
    public:
        struct Options
        {
            std::filesystem::path directory;    // sim/ (it holds engine.fm)
            bool checks = false;                // the loop budget and the bounds checks (a development build)
            int optimisation = 2;
            double sampleRate = 48000.0;
            // The machine code made on a worker thread: create() returns as soon as the front end has read the program
            // (a tenth of a second), with the document and the parameters ready to read and edit, and the adapter
            // plays silence until ready(). False: create() returns ready, as it always did.
            bool background = false;
        };

        // Reads the simulator's program and makes it ready to play (Options::background: ready later). nullptr, and the
        // compiler's words in `errors`, when it does not compile (the front end finds a program's errors, so a
        // background compile fails here too, at once).
        static std::unique_ptr<SimAdapter> create(const Options&, std::string& errors);
        ~SimAdapter() override;

        // The machine code is made and the host plays it. Until then process() writes silence, the telemetry reads 0,
        // the controls are not sent, and the document's edits wait in it (sent whole when it is ready).
        bool ready() const noexcept { return ready_.load(std::memory_order_acquire); }
        // The background compile failed where the front end did not (the code generator, the linker): its words.
        const std::string& compileError() const noexcept { return compileError_; }
        // Seconds since create() while not ready (the status line's "compiling the engine: 3 s").
        double compilingSeconds() const;
        // Waits (the caller's thread: the UI's) until ready or failed, or `seconds` pass; then takes the program as
        // frame() would. True: ready.
        bool waitReady(double seconds);
        // A test's hook: while held, a finished compile is not taken (frame() and waitReady() leave it waiting).
        void holdCompile(bool held) noexcept { held_ = held; }
        // The machine code's size, in bytes (0 until ready).
        std::size_t codeBytes() const noexcept { return codeBytes_; }

        // Once a frame, on the UI thread: what the document still has waiting goes to the host, and the telemetry
        // read next is the newest block's (between two calls of frame() it is fetched at most every few milliseconds,
        // so that a frame's values are of one block).
        void frame();

        // The host, and the document its Spec is a copy of (the interface package binds its pages to the second).
        SimHost& host() noexcept { return *host_; }
        tee::Build& build() noexcept { return *build_; }

        double compileSeconds() const noexcept { return compileSeconds_; }
        const std::string& warnings() const noexcept { return warnings_; }
        // Not empty: a check stopped the program (a build with checks); FMinus's words.
        std::string why() const;

        // ParamSource
        int count() const override { return static_cast<int>(infos_.size()); }
        const ParamInfo& info(int i) const override;
        double get(int i) const override;
        void set(int i, double value) override;
        int findParam(std::string_view id) const override;

        // TelemetrySource
        int findValue(std::string_view name) const override;
        double value(int handle) const override;
        float outputPeak() const override;
        // The simulator's share of its time: the audio callback's time over its buffer's, averaged over about a
        // second (HostStats::loadMean); 0 before the first block.
        double load() const;
        // The host's numbers about the newest block (restarting, clippedBlocks, largestSample, the load); zero before the
        // first block.
        HostStats hostStats() const;
        int outputTail(float* mono, int max) const override { return tail_.read(mono, max); }

        // ControlSink
        void setControls(const ControlsFrame&) override;
        // A counter of the simulator's Controls the frame does not carry ("bakeMaps", "strip") pressed once: one more.
        // False when the host's Controls have no such counter.
        bool pressCounter(std::string_view name);
        bool hasControl(std::string_view name) const;

        // AudioSource
        void prepare(double rate, int maxBlock) override;
        void process(float* l, float* r, int n) noexcept override;

    private:
        SimAdapter();
        void adopt();               // UI thread: the finished compile attached to the host
        void refresh() const;

        // the program as the front end read it: its Fields are the document's and the parameters' until adopt(), and
        // stay alive with the adapter (what was resolved against them stays valid: the layout is the same)
        std::unique_ptr<Program> preview_;
        std::unique_ptr<Schema> previewSchema_;
        std::unique_ptr<Fields> previewFields_;
        // the compile on a worker thread, and what it made
        std::thread worker_;
        mutable std::mutex compiled_;
        std::shared_ptr<Sim> sim_;
        std::string workerErrors_;
        std::atomic<int> workerState_{0};       // 0 compiling, 1 compiled, 2 failed
        std::atomic<bool> ready_{false};
        bool held_ = false;
        std::string compileError_;
        std::chrono::steady_clock::time_point started_{};
        std::size_t codeBytes_ = 0;
        double rate_ = 0.0;                     // prepare()'s, kept for the host until it is ready
        int maxBlock_ = 0;
        std::unique_ptr<LoadOptions> load_;
        std::unique_ptr<SimHost> host_;
        std::unique_ptr<tee::Build> build_;
        std::vector<ParamInfo> infos_;
        std::vector<std::string> paths_;                    // each parameter's path in the Build ("spec....")
        // the tel block's numbers by handle: its scalar fields, then each array element asked for by name ("car.wheelSlip[2]")
        // (findValue adds those, on the UI thread)
        mutable std::vector<const FieldInfo*> tel_;
        mutable std::vector<std::size_t> telIndex_;         // the element's flat index (0 for a scalar)
        mutable std::vector<std::string> telNames_;         // their names ("engine.speed", "car.wheelSlip[2]")
        double compileSeconds_ = 0.0;
        std::string warnings_;
        TailRing tail_;

        // the newest telemetry, fetched when a value is asked and the last fetch is a few milliseconds old
        mutable std::unique_ptr<TelemetryFrame> frame_;
        mutable std::chrono::steady_clock::time_point fetched_{};
        mutable bool have_ = false;
        mutable bool stale_ = true;
    };
}
