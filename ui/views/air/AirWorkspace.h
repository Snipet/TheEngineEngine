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
        // The mismatch lines shown under the row, their words (for tests; U1 for U3, 26 Nov); empty for one not AIR's.
        std::vector<std::string> airWarnings(const Workspace& w);
        // The absolute pressure after each part of the row, Pa, in the row's order (for tests; U1 for U3, 26 Nov).
        std::vector<double> airAbsolutes(const Workspace& w);
    }

    namespace air
    {
        // The telemetry names read (SI; absent ones draw a dash), sim/spec/telemetry.fm's (card F's tel.boost, integrated).
        // One table: a rename is a one-line change.
        struct AirNames
        {
            static constexpr const char* boost = "boost.pressure";       // over the air outside, before the throttle
            static constexpr const char* manifold = "intake.manifold";   // absolute (card E's; the runners' mean on ITBs)
            static constexpr const char* engineManifold = "engine.manifold";   // the same before card E's
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
            static constexpr const char* box = "intake.box";             // over the air outside (card E's)
            static constexpr const char* ripple = "intake.ripple";       // the manifold's pulsing, peak to peak
            static constexpr const char* runner = "intake.runnerLength"; // m: the runner length in use
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
            // card N2's: a charge system far from the engine it is on (sim/boost/boost.fm's judgement, not physics).
            // mismatch's bits: 1 the cooler's rated flow, 2 the gate's spring, 4 the charge pipes' volume, 8 the
            // compressors' choke flow (standard air) against a matched one's; each as has and wants (kg/s, Pa, m^3,
            // kg/s), changing only at configure (the spring's wants follows the ECU's target)
            static constexpr const char* mismatch = "boost.mismatch";
            static constexpr const char* coolerHas = "boost.coolerHas";
            static constexpr const char* coolerWants = "boost.coolerWants";
            static constexpr const char* springHas = "boost.springHas";
            static constexpr const char* springWants = "boost.springWants";
            static constexpr const char* volumeHas = "boost.volumeHas";
            static constexpr const char* volumeWants = "boost.volumeWants";
            static constexpr const char* compressorHas = "boost.compressorHas";
            static constexpr const char* compressorWants = "boost.compressorWants";
            // (U1 for U3, 26 Nov) 16 the turbines' nozzle area against the build's design point's (m^2); 128 a turbo
            // the build's exhaust leaves with none of the engine's flow (card N2's flag; the cure is the next window's)
            static constexpr const char* turbineHas = "boost.turbineHas";
            static constexpr const char* turbineWants = "boost.turbineWants";
        };
    }
}
