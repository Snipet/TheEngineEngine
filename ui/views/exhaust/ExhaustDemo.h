// ui/views/exhaust/ExhaustDemo.h: the EXHAUST stage's demo data, kept here until U1's demo hook takes it
// (docs/handoffs/u1-interface.md): the Spec fields the demo's made-up Spec lacks (sim/exhaust/buildspec.fm's XSpec,
// the real layouts and aspirations, card F's arrangement), a few builds over them, and telemetry the simulator does not
// write yet (the walls' temperatures, the ends' levels, the valve), made from the demo engine's own throttle and speed.
// Fixed arithmetic, no clock and no randomness.
#pragma once

#include "ExhaustModel.h"

#include "app/Demo.h"

#include <string_view>
#include <utility>
#include <vector>

namespace tee::ui::exhaust
{
    // demoFields() with the exhaust builder's fields added and engine.layout and boost.kind as the real Spec has them.
    std::vector<FieldInfo> exhaustDemoFields();

    // Builds over those fields: "na4-421", "tt-v8-x", "sc-v8-180", "single-stack", "v12-log", "flat4"; false for none.
    bool setExhaustDemoBuild(FieldDoc& doc, std::string_view name);

    // The telemetry the stage reads that the simulator does not yet write, at a speed (rpm) and a load (0..1).
    std::vector<std::pair<const char*, double>> exhaustLiveValues(double rpm, double load);
    // Every pipe slot's pressure at its two ends and its middle (card G's ductUsed, ductNear, ductFar, ductMid; 84 each)
    // for a build at a point of its cycle (0..1), a speed (rpm) and a load: each primary's blowdown in its firing order,
    // travelling its length. midP may be null.
    void exhaustDucts(const ExhaustBuild& b, double phase, double rpm, double load, float* used, float* nearP, float* farP,
                      float* midP = nullptr);
    // A two-stroke's exhaust port pressure over a turn (Pa absolute, out[0..360), each degree after top dead centre):
    // card T's 125 single on song (some 10 500 rpm, the pipe at 800 K), its port opening at 84 and closing at 276,
    // stretched to this build's port timing (the blowdown, the diffuser's suction, the baffle's plug just before the
    // port closes). Returns how many were written.
    int exhaustTwoStrokePort(const ExhaustBuild& b, float* out, int max);
    // The telemetry from the demo engine's throttle and speed, held (DemoEngine::set).
    void exhaustDemoLive(DemoEngine& engine);
}
