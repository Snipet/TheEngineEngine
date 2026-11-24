// ui/views/air/AirWorkspace.h: the AIR workspace (docs/plan/ui-design.md section 3; docs/ui/air.md).
//
// The stage: the charge path as a row of parts in the order the build has them (air box, compressors or the
// supercharger, intercooler, throttle or a throttle a runner, plenum, runners), each with its live pressure and
// temperature beneath and the pipes between tinted cold to hot with their numbers; the compressor's map made from its
// sizes as the simulator makes it, with the operating point and its fading trail; the shaft; the gate and the blow-off
// valve as valves that open, boost against its target; anti-lag and nitrous where the build has them.
#pragma once

#include "app/Workspace.h"

#include <memory>
#include <string>
#include <vector>

namespace tee::ui
{
    std::unique_ptr<Workspace> makeAirWorkspace(WorkspaceContext& context);

    namespace air
    {
        // What a press selects on an AIR stage, and where (for tests: everything inside the stage, nothing overlapping).
        struct AirHit
        {
            funkgui::Rect r;
            std::string id;
        };
        std::vector<AirHit> airHits(const Workspace& w);   // empty for a workspace that is not AIR's
    }

    namespace air
    {
        // The telemetry names read (SI; absent ones draw a dash). One table: a rename is a one-line change.
        struct AirNames
        {
            static constexpr const char* boost = "boost.pressure";       // on the branch: over the air outside
            static constexpr const char* manifold = "engine.manifold";   // on the branch: absolute
            static constexpr const char* throttle = "engine.throttle";   // on the branch: 0..1
            static constexpr const char* target = "boost.target";        // over the air outside
            static constexpr const char* speed[2] = { "boost.shaftSpeed[0]", "boost.shaftSpeed[1]" };   // rad/s
            static constexpr const char* ratio[2] = { "boost.ratio[0]", "boost.ratio[1]" };
            static constexpr const char* flow[2] = { "boost.flow[0]", "boost.flow[1]" };   // corrected, kg/s
            static constexpr const char* eta = "boost.efficiency[0]";
            static constexpr const char* outT[2] = { "boost.compressorTemperature[0]", "boost.compressorTemperature[1]" };
            static constexpr const char* gate = "boost.gate[0]";         // 0..1 open
            static constexpr const char* vanes = "boost.vanes[0]";       // 0..1 open
            static constexpr const char* icOut = "boost.intercoolerTemperature";
            static constexpr const char* scPower = "boost.superchargerPower";   // W
            static constexpr const char* scBypass = "boost.bypass";      // 0..1 open
            static constexpr const char* blowOff = "boost.blowOff";      // 0..1 lift
            static constexpr const char* antiLag = "boost.antiLag";      // 0, 1 active
            static constexpr const char* nitrous = "boost.nitrous";      // 0, 1 active
            static constexpr const char* bottle = "boost.bottle";        // kg left
            static constexpr const char* box = "intake.boxPressure";     // absolute
            static constexpr const char* plenumT = "intake.plenumTemperature";
        };
    }
}
