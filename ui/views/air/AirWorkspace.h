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
        // The telemetry names read (SI; absent ones draw a dash), sim/spec/telemetry.fm's (card F's tel.boost, integrated).
        // One table: a rename is a one-line change.
        struct AirNames
        {
            static constexpr const char* boost = "boost.pressure";       // over the air outside, before the throttle
            static constexpr const char* manifold = "engine.manifold";   // absolute
            static constexpr const char* throttle = "engine.throttle";   // 0..1
            static constexpr const char* turbos = "boost.turbos";        // shafts in use
            static constexpr const char* speed[2] = { "boost.shaftSpeed[0]", "boost.shaftSpeed[1]" };   // rad/s
            static constexpr const char* ratio[2] = { "boost.ratio[0]", "boost.ratio[1]" };
            static constexpr const char* flow[2] = { "boost.flow[0]", "boost.flow[1]" };   // corrected, kg/s
            static constexpr const char* eta = "boost.efficiency[0]";
            static constexpr const char* outT[2] = { "boost.compressorOut[0]", "boost.compressorOut[1]" };   // K
            static constexpr const char* gate = "boost.gate[0]";         // 0..1 open
            static constexpr const char* vanes = "boost.vanes[0]";       // 0..1 open (1: fixed)
            static constexpr const char* drive = "boost.drive[0]";       // before the turbine over the charge pipe
            static constexpr const char* chargePipe = "boost.chargePipe"; // after the compressor: over the air outside
            static constexpr const char* icOut = "boost.intercoolerOut";  // K
            static constexpr const char* throttleIn = "boost.throttleIn"; // K, before the throttle
            static constexpr const char* scPower = "boost.superchargerPower";   // W
            static constexpr const char* blowOff = "boost.blowOffValve"; // 0..1 open
            static constexpr const char* flags = "boost.flags";          // 1 surge, 2 choke, 4 over-speed, 8 over-boost,
                                                                          // 16 blow-off open, 32 nitrous flowing
            static constexpr const char* box = "intake.boxPressure";     // card E's, not integrated yet
            static constexpr const char* plenumT = "intake.plenumTemperature";   // card E's, not integrated yet
            // the Scope's map (sim/spec/scope.fm, card F's), while ctl.scopeOn
            static constexpr const char* mapRev = "boostMapRev";
            static constexpr const char* lineSpeed = "boostLineSpeed";        // [8]
            static constexpr const char* lineFlow = "boostLineFlow";          // [8][16]
            static constexpr const char* lineRatio = "boostLineRatio";        // [8][16]
            static constexpr const char* contourEta = "boostContourEta";      // [5]
            static constexpr const char* contourFlow = "boostContourFlow";    // [5][16]: left crossings up, right down
            static constexpr const char* contourRatio = "boostContourRatio";  // [5][16]
            static constexpr const char* trailAt = "boostTrailAt";
            static constexpr const char* trailFlow = "boostTrailFlow";        // [256], a ring, newest at trailAt - 1
            static constexpr const char* trailRatio = "boostTrailRatio";      // [256]
        };
    }
}
