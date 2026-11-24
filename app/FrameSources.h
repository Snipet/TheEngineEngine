// app/FrameSources.h: what the interface's frame (ui/app/AppFrame.h) is given by the application, over the simulator
// (sim/engine.fm played by card C's SimHost, its document a tee::Build):
//
//   SimScope     the frame's ScopeSource: the scope block's arrays by name (sim/spec/scope.fm), copied from the host
//                only while a workspace reads them (SimHost::scope asks for about half a second each time).
//   SimDyno      the frame's DynoSource: the predicted curve of the build as it stands, made by card C's DynoService on
//                a worker thread of its own (never the audio thread) a moment after each edit, with the one before it
//                kept as the ghost; and the DYNO workspace's runs, on a second service so that a run and a prediction
//                do not cancel each other.
//   addShellServices  the build's services (ui/kit/bind/host/BuildDoc.h) with the shell's added: the simulator's load,
//                the cockpit's mode and clutch, the output's level.
//
// UI thread only, except where a comment says otherwise. Compiled into the executables that have the frame
// (TEE_HAS_UI_FRAME); each object here is owned by the application's Shown (app/Offline.h), destroyed before main
// returns (never a static: docs/plan/01-rules.md, "Found on macOS").
#pragma once

#if defined(TEE_HAS_UI_FRAME)

#include "app/Workspace.h"

#include "tee/Dyno.h"
#include "tee/Telemetry.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tee
{
    class SimHost;
}

namespace tee::input
{
    class Controls;
}

namespace tee::player
{
    class SimAdapter;
}

namespace tee::files
{
    class BuildFile;
    class BuildLibrary;
}

namespace tee::app
{
    class SimScope final : public ui::ScopeSource
    {
    public:
        explicit SimScope(SimHost& host);
        ~SimScope() override;

        // Once a frame, before the Panel ticks: the next read fetches the newest copy (at most one fetch a frame, so
        // what a frame draws is of one copy).
        void frame() noexcept { stale_ = true; }

        int find(std::string_view name) const override;
        int read(int handle, float* out, int max) override;
        std::uint64_t generation() const override { return generation_; }

        // How many times the host's copy was fetched (tests: nothing is fetched while nobody reads).
        int fetches() const noexcept { return fetches_; }

    private:
        void fetch();

        SimHost& host_;
        mutable std::vector<std::string> names_;           // a handle's name ("pressure")
        std::unique_ptr<ScopeFrame> copy_;
        bool stale_ = true, have_ = false;
        std::uint64_t generation_ = 0, lastBlock_ = 0;
        int fetches_ = 0;
    };

    class SimDyno final : public ui::DynoSource
    {
    public:
        struct Settings
        {
            double quiet = 0.4;         // seconds with no edit before a curve is asked for
            double from = 0.0;          // rad/s; 0: the idle's (telemetry engine.idle), else 105 (1000 rpm)
            double to = 0.0;            // rad/s; 0: the redline's (engine.redline), else 680 (6500 rpm)
            double rate = 120.0;        // the predicted sweep's rate, rad/s^2
            double spacing = 15.0;      // rad/s between its points
        };

        // `player` gives the build, the program and the telemetry (the redline). The services start no thread until
        // the first curve is asked for.
        SimDyno(player::SimAdapter& player, Settings settings);
        explicit SimDyno(player::SimAdapter& player) : SimDyno(player, Settings{}) {}
        ~SimDyno() override;

        void tick(float dt) override;
        const ui::DynoTrace& predicted() const override { return predicted_; }
        const ui::DynoTrace& before() const override { return before_; }
        bool busy() const override;
        double progress() const override;
        std::string error() const override { return error_; }
        std::uint64_t revision() const override { return revision_; }

        int runCount() const override { return static_cast<int>(runs_.size()); }
        const ui::DynoTrace& run(int i) const override;
        bool startRun(const ui::DynoRequest&) override;
        bool running() const override { return runRunning_; }
        void stopRun() override;

        // Waits (on the UI thread, a test's or tee-headless's) until the curve asked for is in, or `seconds` have
        // passed: true when one is. tee-headless uses it so a picture of a workspace with the ribbon has its curve.
        bool waitForCurve(double seconds);
        // The last curve's speed against real time, and how long it took (seconds of the machine's time).
        double lastRealTime() const noexcept { return lastRealTime_; }
        double lastWallSeconds() const noexcept { return lastWall_; }

    private:
        DynoPlan predictedPlan() const;
        void ask();
        static ui::DynoTrace trace(const DynoCurve&, std::string name);

        player::SimAdapter& player_;
        Settings settings_;
        std::unique_ptr<DynoService> service_, runner_;
        ui::DynoTrace predicted_, before_;
        std::vector<ui::DynoTrace> runs_;
        std::string error_;
        std::uint64_t revision_ = 1;
        std::uint64_t seenHash_ = 0, askedHash_ = 0;
        bool waiting_ = false, sawBusy_ = false;
        double sinceAsk_ = 0.0;
        double sinceEdit_ = 0.0, sinceHash_ = 1.0;
        bool lastBusy_ = false;
        int lastProgress_ = -1;
        bool runRunning_ = false;
        std::string runName_;
        int runsMade_ = 0;
        double lastRealTime_ = 0.0, lastWall_ = 0.0;
    };

    // The shell's share of the frame's services, added to the build's (ui::buildServices: load, save, undo, the
    // garage's list): the simulator's load, the cockpit's mode and clutch. `level` is the output's level (0..1) for
    // the top bar; empty: the telemetry's peak.
    void addShellServices(ui::FrameServices& services, player::SimAdapter& player, input::Controls& controls,
                          const std::function<float()>& level = {});

    // The build's name, its unsaved flag and the garage's list, load and save, over the document on disk and the
    // library (shell/files/BuildFiles.h), in place of the plain ones of ui::buildServices: so that a build loaded or
    // saved from the frame is the document's file, and its star is the dirty flag's.
    void addFileServices(ui::FrameServices& services, files::BuildFile& file, const files::BuildLibrary& library);
}

#endif
