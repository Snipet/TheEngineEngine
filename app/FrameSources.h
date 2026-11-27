// app/FrameSources.h: the shell's share of what the interface's frame (ui/app/AppFrame.h) is given over the simulator.
// The Scope and the dyno are U1's (ui/app/host/HostSources.h: SimScope, ServiceDyno); the build's own services are
// J2's (ui/kit/bind/host/BuildDoc.h: buildServices). Here:
//
//   addShellServices  the simulator's load, the cockpit's mode and clutch, the output's level
//   addFileServices   the build's name, its unsaved flag and the garage's list, load and save over the document on
//                     disk and the library (shell/files/BuildFiles.h)
//   waitForCurve      tee-headless's --dyno-wait: the dyno ticked until its first curve is in (or an error, or time)
//
// UI thread only. Compiled into the executables that have the frame (TEE_HAS_UI_FRAME).
#pragma once

#if defined(TEE_HAS_UI_FRAME)

#include "app/Workspace.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

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

namespace tee
{
    class Sim;
}

namespace tee::ui
{
    class ServiceBake;
    class ServiceDyno;
}

namespace tee::app
{
    // `level` is the output's level (0..1) for the top bar; empty: the telemetry's peak.
    void addShellServices(ui::FrameServices& services, player::SimAdapter& player, input::Controls& controls,
                          const std::function<float()>& level = {});

    // In place of the plain ones of ui::buildServices: a build loaded or saved from the frame is the document's file,
    // and its star is the dirty flag's.
    void addFileServices(ui::FrameServices& services, files::BuildFile& file, const files::BuildLibrary& library);

    // The frame's dyno while the engine's machine code is still being made (SimAdapter::ready): U1's ServiceDyno,
    // made once there is a program to run (it needs one when it is made); until then no curve, and a sentence saying
    // why. UI thread.
    class LateDyno final : public ui::DynoSource
    {
    public:
        explicit LateDyno(player::SimAdapter& player);
        ~LateDyno() override;
        void tick(float dt) override;
        const ui::DynoTrace& predicted() const override;
        const ui::DynoTrace& before() const override;
        bool busy() const override;
        double progress() const override;
        std::string error() const override;
        std::uint64_t revision() const override;
        int runCount() const override;
        const ui::DynoTrace& run(int i) const override;
        bool startRun(const ui::DynoRequest& r) override;
        bool running() const override;
        void stopRun() override;
        // U1's ServiceDyno once made (the bake yields to its predictor); null until the engine's compile is in.
        const ui::ServiceDyno* service() const noexcept { return service_; }

    private:
        player::SimAdapter& player_;
        std::unique_ptr<ui::DynoSource> inner_;
        const ui::ServiceDyno* service_ = nullptr;
        ui::DynoTrace none_;
        std::uint64_t made_ = 0;
    };

    // TUNE's spark map (card C's knock-limit bake through U1's ServiceBake): made once there is a program (the engine's
    // compile in), made again when the host plays another (a QUALITY change); ticked once a frame on the UI thread.
    // Until then its state is "waiting to bake". The document (SimAdapter::build) is one object for the whole run.
    class LateBake
    {
    public:
        // `cache`: the bakes' folder (CalibrationBakeOptions::cacheDirectory); empty: none.
        // `dyno`: the ribbon's, whose predictor the bake yields to (between its points while a curve is wanted).
        // `scenarios`: content/validation/scenarios (CalibrationBakeOptions::idleScenarios: the warm idle a reused cam
        // map is checked with); empty: no cam map is reused.
        LateBake(player::SimAdapter& player, const LateDyno& dyno, std::filesystem::path cache,
                 std::filesystem::path scenarios);
        ~LateBake();
        void tick();
        std::string state() const;
        ui::BakeStatus status() const;   // the same as fields (U1, 27 Nov: the bake across edits' words); before a program, resting
        std::vector<std::vector<bool>> stillKnocking() const;

    private:
        player::SimAdapter& player_;
        const LateDyno& dyno_;
        std::filesystem::path cache_, scenarios_;
        std::shared_ptr<Sim> made_;
        const ui::ServiceDyno* yieldedTo_ = nullptr;
        std::unique_ptr<ui::ServiceBake> inner_;
    };

    // Ticks `dyno` (this machine's time) until a predicted curve is in, it reports an error, or `seconds` pass. True:
    // a curve is in.
    bool waitForCurve(ui::DynoSource& dyno, double seconds);
}

#endif
