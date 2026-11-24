// ui/views/air/AirDemo.h: the AIR stage's demo data, kept here until U1's demo hook takes it: the Spec fields the
// demo lacks (card F's sim/boost/machinespec.fm, card E's sim/intake/lab-spec.fm, the real layouts and aspirations),
// builds over them, and the telemetry the simulator does not write yet, made from the demo engine's throttle, speed
// and boost and the build's own compressor map. Fixed arithmetic, no clock, no randomness.
#pragma once

#include "AirModel.h"

#include "app/Demo.h"

#include <string_view>
#include <utility>
#include <vector>

namespace tee::ui::air
{
    std::vector<FieldInfo> airDemoFields();
    // "na4-itb", "turbo4" (one turbo, variable vanes, external gate, anti-lag, nitrous), "tt-v8", "sc-v8" (Roots),
    // "seq6" (sequential), "cmp6" (compound), "cf-v8" (centrifugal blower), "var-v8" (two runner lengths); false for none.
    bool setAirDemoBuild(FieldDoc& doc, std::string_view name);
    // The live values a build would show at a speed (rpm) and a load (0..1): telemetry names and SI values (NaN: none).
    std::vector<std::pair<const char*, double>> airLiveValues(const AirBuild& b, double rpm, double load);
    // The live values for the build in `doc` at the demo engine's present moment, held (DemoEngine::set).
    void airDemoLive(DemoEngine& engine, const FieldDoc& doc);
}
