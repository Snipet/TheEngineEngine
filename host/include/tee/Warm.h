// host/include/tee/Warm.h: state of the simulator a host may hold where it wants it, for offline runs and the dyno
// (never the application's engine): a warm engine, as a dyno cell's conditioning keeps one.
//
// The simulator's coolant, oil and combustion chamber's walls (core/heat.fm) warm over minutes of running and, left at
// full load for minutes, climb past the thermostat. A curve or a "warm idle" wants an engine at its working
// temperatures now and kept there: these are written into the instance before each block (LoadOptions::hold, and the
// PredictService's own instances). A path the program lacks is skipped.
#pragma once

#include <string>
#include <vector>

namespace tee
{
    struct HeldValue
    {
        std::string path;   // in the main module: "heat.tCool"
        double value = 0.0;
    };

    // The coolant at its thermostat (368 K), the oil 10 K and the chamber's walls 87 K over it, as Heat has a running
    // engine's, and its two "how warm" shares at 1.
    inline std::vector<HeldValue> warmEngine()
    {
        const double coolant = 368.0;
        return {{"heat.tCool", coolant}, {"heat.tOil", coolant + 10.0}, {"heat.tSurf", coolant + 87.0},
                {"heat.warm", 1.0},      {"heat.warmS", 1.0}};
    }
} // namespace tee
