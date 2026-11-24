// ui/app/host/HostSources.h: the frame's DynoSource and ScopeSource over card C's host (built only where the host
// library is: target tee_ui_build, the root's build). The shell makes them beside the frame (docs/handoffs/u1-frame.md):
//
//     tee::ui::ServiceDyno dyno(simAdapter.host().sim(), simAdapter.build());
//     tee::ui::SimScope scope(simAdapter.host());
//     auto frame = tee::ui::makeAppFrame(doc, units, player, services, &dyno, &scope);
//
// ServiceDyno: tee::DynoService (a worker thread of its own, the newest request only, a cache by the build's hash and
// the plan) asked for the predicted curve 0.4 s after the build last changed; the curve before it kept as the ghost;
// the runs a person starts on a second service. SimScope: SimHost::scope() read by name ("pressure", "volume", ...,
// the fields of sim/spec/scope.fm), the per-degree arrays cut to `degrees`, the pipe's to `pipePoints`. UI thread.
#pragma once

#include "app/Workspace.h"

#include "tee/Build.h"
#include "tee/Dyno.h"
#include "tee/SimHost.h"

#include <memory>
#include <string>
#include <vector>

namespace tee::ui
{
    class ServiceDyno final : public DynoSource
    {
    public:
        ServiceDyno(std::shared_ptr<tee::Sim> sim, const tee::Build& build, tee::DynoPlan plan = {});

        void tick(float dt) override;
        const DynoTrace& predicted() const override { return predicted_; }
        const DynoTrace& before() const override { return before_; }
        bool busy() const override { return waiting_ || quiet_ > 0.0; }
        double progress() const override { return waiting_ ? service_.progress() : 0.0; }
        std::string error() const override { return service_.error(); }
        std::uint64_t revision() const override { return revision_; }
        int runCount() const override { return static_cast<int>(runs_.size()); }
        const DynoTrace& run(int i) const override { return runs_[static_cast<std::size_t>(i)]; }
        bool startRun(const DynoRequest& r) override;
        bool running() const override { return running_; }
        void stopRun() override;

        static DynoTrace traceOf(const tee::DynoCurve& c, std::string name);
        static constexpr double kQuiet = 0.4;   // seconds after the last edit before a curve is asked for

    private:
        void take(const DynoTrace& t);

        const tee::Build& build_;
        tee::DynoPlan plan_;
        tee::DynoService service_;
        std::unique_ptr<tee::DynoService> runner_;
        std::shared_ptr<tee::Sim> sim_;
        DynoTrace predicted_, before_;
        std::vector<DynoTrace> runs_;
        std::string runName_;
        std::uint64_t seen_ = 0, revision_ = 1;
        double quiet_ = 0.0;
        bool waiting_ = false, running_ = false, first_ = true;
    };

    class SimScope final : public ScopeSource
    {
    public:
        explicit SimScope(tee::SimHost& host);
        int find(std::string_view name) const override;
        int read(int handle, float* out, int max) override;
        std::uint64_t generation() const override { return generation_; }

    private:
        double scalar(std::string_view name) const;
        tee::SimHost& host_;
        std::vector<const tee::FieldInfo*> fields_;
        tee::ScopeFrame frame_;
        bool have_ = false;
        std::uint64_t lastBlock_ = 0, generation_ = 0;
    };
}
