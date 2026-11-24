// app/FrameSources.cpp: FrameSources.h.
#include "FrameSources.h"

#if defined(TEE_HAS_UI_FRAME)

#include "files/BuildFiles.h"
#include "input/Controls.h"
#include "player/SimAdapter.h"

#include "app/host/HostSources.h"
#include "kit/bind/host/BuildDoc.h"
#include "tee/SimHost.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <thread>

namespace tee::app
{
    LateDyno::LateDyno(player::SimAdapter& player) : player_(player) {}
    LateDyno::~LateDyno() = default;

    void LateDyno::tick(float dt)
    {
        if (!inner_ && player_.ready() && player_.host().sim())
        {
            inner_ = std::make_unique<ui::ServiceDyno>(player_.host().sim(), player_.build());
            made_ = 1000;                         // a revision past the ones said before
        }
        if (inner_)
            inner_->tick(dt);
    }

    const ui::DynoTrace& LateDyno::predicted() const { return inner_ ? inner_->predicted() : none_; }
    const ui::DynoTrace& LateDyno::before() const { return inner_ ? inner_->before() : none_; }
    bool LateDyno::busy() const { return inner_ ? inner_->busy() : !player_.ready() && player_.compileError().empty(); }
    double LateDyno::progress() const { return inner_ ? inner_->progress() : 0.0; }
    std::string LateDyno::error() const
    {
        if (inner_)
            return inner_->error();
        return player_.compileError().empty() ? std::string("the engine is being compiled") : player_.compileError();
    }
    std::uint64_t LateDyno::revision() const { return inner_ ? made_ + inner_->revision() : 1; }
    int LateDyno::runCount() const { return inner_ ? inner_->runCount() : 0; }
    const ui::DynoTrace& LateDyno::run(int i) const { return inner_ ? inner_->run(i) : none_; }
    bool LateDyno::startRun(const ui::DynoRequest& r) { return inner_ && inner_->startRun(r); }
    bool LateDyno::running() const { return inner_ && inner_->running(); }
    void LateDyno::stopRun()
    {
        if (inner_)
            inner_->stopRun();
    }

    bool waitForCurve(ui::DynoSource& dyno, double seconds)
    {
        const auto start = std::chrono::steady_clock::now();
        float dt = 0.5f;            // the first tick lets the quiet time after the last edit pass at once
        while (dyno.predicted().empty())
        {
            dyno.tick(dt);
            dt = 0.02f;
            if (!dyno.error().empty() && !dyno.busy())   // (a dyno that cannot run says so at once)
                return false;
            if (std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count() > seconds)
                return false;
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        return true;
    }

    // ---- the services ---------------------------------------------------------------------------------------------
    namespace
    {
        // The frame's modes (0 bench, 1 dyno, 2 drive) and the simulator's (MODE_BENCH 0, MODE_DRIVE 1,
        // MODE_DYNO_ENGINE 2).
        int toSim(int frameMode) { return frameMode == 1 ? 2 : frameMode == 2 ? 1 : 0; }
        int toFrame(int simMode) { return simMode == 2 ? 1 : simMode == 1 ? 2 : 0; }
    }

    void addShellServices(ui::FrameServices& s, player::SimAdapter& player, input::Controls& controls,
                          const std::function<float()>& level)
    {
        s.simLoad = [&player] { return static_cast<float>(player.load()); };
        // START is disabled while the engine's machine code is being made (SimAdapter::ready)
        s.engineReady = [&player] { return player.ready(); };
        // TUNE's BAKE: ctl.bakeMaps pressed once, where the host's Controls carry it (card C's SimControls does not
        // yet: then none, and TUNE says so)
        if (player.hasControl("bakeMaps"))
            s.bakeMaps = [&player] { player.pressCounter("bakeMaps"); };
        s.mode = [&controls] { return toFrame(controls.driver().settings().mode); };
        s.setMode = [&controls](int m) { controls.driver().setMode(toSim(m)); };
        s.clutch = [&controls](float v) { controls.driver().setClutchStrip(v, v >= 0.0f); };
        if (level)
            s.outputLevel = level;
    }

    void addFileServices(ui::FrameServices& s, files::BuildFile& file, const files::BuildLibrary& library)
    {
        s.buildName = [&file] {
            const std::string t = file.title();
            return t.size() > 2 && t.compare(t.size() - 2, 2, " *") == 0 ? t.substr(0, t.size() - 2) : t;
        };
        s.unsaved = [&file] { return file.dirty(); };
        s.builds = [&library] {
            std::vector<std::string> names;
            for (const files::BuildSummary& b : library.list())
                names.push_back(b.name);
            return names;
        };
        s.load = [&file, &library](const std::string& name, std::string& error) {
            std::vector<std::string> warnings;
            return library.load(file, name, warnings, error);
        };
        s.save = [&file, &library](const std::string& name, std::string& error) { return library.save(file, name, error); };
    }
}

#endif
