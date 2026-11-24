// ui/app/host/HostSources.h: the frame's DynoSource and ScopeSource over card C's host (built only where the host
// library is: target tee_ui_build, the root's build). The shell makes them beside the frame (docs/handoffs/u1-frame.md):
//
//     tee::ui::ServiceDyno dyno(simAdapter.host().sim(), simAdapter.build());
//     tee::ui::SimScope scope(simAdapter.host());
//     auto frame = tee::ui::makeAppFrame(doc, units, player, services, &dyno, &scope);
//
// ServiceDyno: the predicted curve from card C's tee::PredictService (steady points, warm engine, several workers;
// coarse first, then filled in, then refined; a build seen before at once from its cache), asked kQuiet after the build
// last changed; predicted() is the newest curve for the build as it stands (coarse, partial or refined), before() the
// last refined curve of the build before the edit (the ghost). The runs a person starts on the DYNO workspace go to
// tee::DynoService (a real sweep, watched as it draws). SimScope: SimHost::scope() read by name ("pressure", "volume",
// ..., the fields of sim/spec/scope.fm), the per-degree arrays cut to `degrees`, the pipe's to `pipePoints`;
// generation() itself asks the host for a newer copy, so a reader that waits for it to move gets one. UI thread.
#pragma once

#include "app/Workspace.h"

#include "tee/Build.h"
#include "tee/Dyno.h"
#include "tee/Predict.h"
#include "tee/SimHost.h"

#include <memory>
#include <string>
#include <vector>

namespace tee::ui
{
    class ServiceDyno final : public DynoSource
    {
    public:
        // `plan`: the runs' (DynoRequest fills from, to, rate, pedal, steps); `predict`: the predicted curve's.
        ServiceDyno(std::shared_ptr<tee::Sim> sim, const tee::Build& build, tee::DynoPlan plan = {},
                    tee::PredictOptions predict = {});

        void tick(float dt) override;
        const DynoTrace& predicted() const override { return predicted_; }
        const DynoTrace& before() const override { return before_; }
        bool busy() const override { return quiet_ > 0.0 || predict_.busy(); }
        double progress() const override { return predict_.busy() ? predict_.progress() : 0.0; }
        std::string error() const override { return predict_.error(); }
        std::uint64_t revision() const override { return revision_; }
        int runCount() const override { return static_cast<int>(runs_.size()); }
        const DynoTrace& run(int i) const override { return runs_[static_cast<std::size_t>(i)]; }
        bool startRun(const DynoRequest& r) override;
        bool running() const override { return running_; }
        void stopRun() override;
        bool refined() const noexcept { return refined_; }   // predicted() is the refined curve
        std::uint64_t requests() const override { return requests_; }

        static DynoTrace traceOf(const tee::DynoCurve& c, std::string name);
        static DynoTrace traceOf(const tee::PredictCurve& c, std::string name);
        static constexpr double kQuiet = 0.25;   // seconds after the last edit before a curve is asked for

    private:
        const tee::Build& build_;
        tee::DynoPlan plan_;
        std::shared_ptr<tee::Sim> sim_;
        tee::PredictService predict_;
        std::unique_ptr<tee::DynoService> runner_;
        DynoTrace predicted_, before_, lastRefined_;
        std::vector<DynoTrace> runs_;
        std::string runName_;
        bool runStepped_ = false;   // the run under way is stepped (its walls said), else a sweep (-1)
        std::uint64_t seen_ = 0, revision_ = 1, request_ = 0, predictRevision_ = 0, refinedFor_ = 0;
        double quiet_ = 0.0;
        std::uint64_t requests_ = 0;
        bool running_ = false, first_ = true, refined_ = false;
    };

    class SimScope final : public ScopeSource
    {
    public:
        explicit SimScope(tee::SimHost& host);
        int find(std::string_view name) const override;
        int read(int handle, float* out, int max) override;
        // Asks the host for its newest copy (keeping it copying while someone looks) and moves when one is newer.
        std::uint64_t generation() const override;

    private:
        void poll() const;
        double scalar(std::string_view name) const;
        tee::SimHost& host_;
        std::vector<const tee::FieldInfo*> fields_;
        mutable tee::ScopeFrame frame_;
        mutable bool have_ = false;
        mutable std::uint64_t lastBlock_ = 0, generation_ = 0;
    };
}
