// ui/views/scope/ScopeDemo.h: what SCOPE reads that U1's demo (ui/app/Demo.h) does not have yet, in one place, so it
// moves into the demo's hook when there is one (docs/handoffs/u1-interface.md).
//
//   ScopeDemo     a ScopeSource over another (the demo's) that adds cylinder 1's port pressures over the cycle,
//                 "portIntake" and "portExhaust" (Pa absolute, one per degree, as "pressure" is): names no package has
//                 put in sim/spec/scope.fm yet. Made up from the other's lifts and pressure: the exhaust's blowdown
//                 pulse at its opening and its reflections, the intake's suction and its rebound.
//   scopeDemoTelemetry(engine)  holds "ecu.advance" (the spark's advance now, crank degrees before top centre: a name
//                 telemetry.fm does not have yet) at a plausible value.
#pragma once

#include "app/Demo.h"
#include "app/Workspace.h"

#include <string_view>
#include <vector>

namespace tee::ui::scope
{
    class ScopeDemo final : public ScopeSource
    {
    public:
        explicit ScopeDemo(ScopeSource& inner) : inner_(&inner) {}

        int find(std::string_view name) const override;
        int read(int handle, float* out, int max) override;
        std::uint64_t generation() const override { return inner_->generation(); }

        static constexpr int kIntake = 1000, kExhaust = 1001;   // the handles of the two added arrays

    private:
        ScopeSource* inner_;
        std::vector<float> a_, b_;
    };

    void scopeDemoTelemetry(DemoEngine& engine);
}
