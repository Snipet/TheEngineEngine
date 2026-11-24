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

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace tee
{
    class SimHost;
    struct FieldInfo;
    struct TelemetryFrame;
    inline namespace document
    {
        class Build;
    }
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
        };

        // Compiles the simulator and makes it ready to play. nullptr, and the compiler's words in `errors`, when it
        // does not compile.
        static std::unique_ptr<SimAdapter> create(const Options&, std::string& errors);
        ~SimAdapter() override;

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
        int outputTail(float* mono, int max) const override { return tail_.read(mono, max); }

        // ControlSink
        void setControls(const ControlsFrame&) override;

        // AudioSource
        void prepare(double rate, int maxBlock) override;
        void process(float* l, float* r, int n) noexcept override;

    private:
        SimAdapter();
        void refresh() const;

        std::unique_ptr<SimHost> host_;
        std::unique_ptr<tee::Build> build_;
        std::vector<ParamInfo> infos_;
        std::vector<std::string> paths_;                    // each parameter's path in the Build ("spec....")
        std::vector<const FieldInfo*> tel_;                 // the tel block's scalar fields, by handle
        std::vector<std::string> telNames_;                 // their names ("engine.speed")
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
