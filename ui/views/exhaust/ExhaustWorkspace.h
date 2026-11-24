// ui/views/exhaust/ExhaustWorkspace.h: the EXHAUST workspace (docs/plan/ui-design.md section 3; docs/ui/exhaust.md).
//
// The stage is the system as built (ExhaustModel.h resolves it from the fields as the simulator's builder does;
// ExhaustStage.h lays it out): the heads and their ports, each primary to scale with the others, the collectors and
// merges, the turbine housings, then each side's chain (converter, main pipe, the crossover between the sides,
// resonator, muffler by its kind, the bypass valve drawn open or shut, the tails) to each radiating end with its
// position and level. The walls are tinted by temperature (with the numbers at the port, the collector and the tail);
// the pressure along cylinder 1's path rides that path as a trace, live from the Scope's pipe array, with a SLOW switch.
// A press on an element selects it; the selected one carries a ring and its name.
#pragma once

#include "app/Workspace.h"

#include <memory>

namespace tee::ui
{
    std::unique_ptr<Workspace> makeExhaustWorkspace(WorkspaceContext& context);

    namespace exhaust
    {
        // The telemetry names the stage reads (in SI; absent ones draw a dash or a neutral wall). One table, so a
        // rename in sim/spec/telemetry.fm is a one-line change.
        struct ExhaustNames
        {
            static constexpr const char* port = "exhaust.portTemperature";         // on the branch
            static constexpr const char* collector = "exhaust.collectorTemperature";
            static constexpr const char* tail = "exhaust.tailTemperature";
            // each heat section's wall (sim/exhaust/heat2.fm tWall), in the gas's order
            static constexpr const char* walls[7] = { "exhaust.wallPrimaries", "exhaust.wallCollectors",
                                                      "exhaust.wallMerges",    "exhaust.wallMain",
                                                      "exhaust.wallLink",      "exhaust.wallMufflers",
                                                      "exhaust.wallTails" };
            static constexpr const char* level[2] = { "exhaust.tailLevel[0]", "exhaust.tailLevel[1]" };   // 0..1 of full scale
            static constexpr const char* valve[2] = { "exhaust.valve[0]", "exhaust.valve[1]" };           // 0 shut .. 1 open
            static constexpr const char* pop = "exhaust.pop";          // 0..1, a bang at the tails (card H's account)
            static constexpr const char* flags = "engine.flags";       // on the branch: FLAG_FUEL_CUT = 2
            static constexpr const char* throttle = "engine.throttle"; // on the branch
            static constexpr const char* scopePipe = "pipePressure";   // sim/spec/scope.fm, over cylinder 1's path
            static constexpr const char* ambient = "env.pressure";     // a field: the air outside
        };
    }
}
