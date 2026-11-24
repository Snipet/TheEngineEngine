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
            // card G's tel.exhaust: by heat section (sim/exhaust/heat2.fm), in the gas's order: 0 primaries, 1 collectors'
            // pipes, 2 second merges' pipes, 3 main pipes, 4 pipes after a crossover or resonator, 5 mufflers, 6 tails
            static constexpr const char* collector = "exhaust.gasTemperature[1]";
            static constexpr const char* tail = "exhaust.gasTemperature[6]";
            static constexpr const char* walls[7] = { "exhaust.wallTemperature[0]", "exhaust.wallTemperature[1]",
                                                      "exhaust.wallTemperature[2]", "exhaust.wallTemperature[3]",
                                                      "exhaust.wallTemperature[4]", "exhaust.wallTemperature[5]",
                                                      "exhaust.wallTemperature[6]" };
            // the radiating ends: how many, each one's level (dB) and the pipe slot it ends (slots.fm's numbering)
            static constexpr const char* ends = "exhaust.ends";
            static constexpr const char* endLevel = "exhaust.endLevel";   // [24], "<name>[i]"
            static constexpr const char* endSlot = "exhaust.endSlot";     // [24]
            static constexpr const char* valve[2] = { "exhaust.valve[0]", "exhaust.valve[1]" };           // 0 shut .. 1 open
            static constexpr const char* pop = "exhaust.flame";        // 1 while a bang burns near a hot tail
            static constexpr const char* flags = "engine.flags";       // on the branch: FLAG_FUEL_CUT = 2
            static constexpr const char* throttle = "engine.throttle"; // on the branch
            static constexpr const char* scopePipe = "pipePressure";   // K3's, the reference's (absolute or gauge)
            // card G's Scope (exhaust.model 1): cylinder 1's path (gauge Pa, and each point's distance from the port),
            // and every pipe slot's pressure at its two ends
            static constexpr const char* pathPressure = "exhaustPressure";   // [64]
            static constexpr const char* pathX = "exhaustX";                 // [64], m
            static constexpr const char* ductUsed = "ductUsed";              // [84]
            static constexpr const char* ductNear = "ductNear";              // [84], gauge Pa at the upstream end
            static constexpr const char* ductFar = "ductFar";                // [84]
            static constexpr const char* ductMid = "ductMid";                // [84], at the pipe's middle (else drawn straight)
            static constexpr const char* ambient = "env.pressure";     // a field: the air outside
            // card G's network as built (NetworkCost, ExhaustModel.h, prices them)
            static constexpr const char* cost[4] = { "exhaust.pipes", "exhaust.nodes", "exhaust.pairs", "exhaust.openEnds" };
            // card T's two-stroke (engine.cycle 1)
            static constexpr const char* tsPipeTemp = "engine.tsPipeTemperature";   // K, cylinder 1's pipe's gas
            // cylinder 1's exhaust port pressure each degree (absolute), read when the Scope's cycle is a turn (360);
            // and pipePressure: for a two-stroke, six points along cylinder 1's chamber, then its crankcase
            static constexpr const char* portScope = "portExhaust";
            static constexpr const char* degrees = "degrees";           // the Scope's cycle: 360 for a two-stroke
        };
    }
}
