// ui/views/engine/EngineWorkspace.h: the ENGINE workspace (docs/ui/engine.md).
//
// Its stage, from the build and moved by the crank:
//   BLOCK        the block from above: each bank's cylinders as bores, numbered, lit as each fires; the crank's pins
//                beneath it, seen from the front, turning. Click a cylinder to select it, the block for the layout.
//   FIRING       720 degrees as a ring (a rotary's 1080): a tick and a number at each firing, a ring for each bank
//                inside it, the intervals in words, a marker going round with the crank.
//   SECTION      one cylinder cut through at its true proportions: the piston on its rod and crank, both valves
//                lifting by the Scope's lift curves, the gas tinted by its pressure with the number beside it, the
//                spark. LIVE follows the crank; SLOW plays the Scope's last cycle at a chosen pace.
//   DERIVED      displacement, bore over stroke, rod over stroke, mean piston speed at the limiter, compression.
//   PV LOOP      the Scope's pressure against volume, linear or log-log, the point now marked.
#pragma once

#include "app/Workspace.h"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace tee::ui
{
    std::unique_ptr<Workspace> makeEngineWorkspace(WorkspaceContext& c);
    // For tests: the panels drawn now (no text may cross one's edge), and whether the stage is the small one (the block
    // over a panel of tabs: FIRING, P-V LOOP, DERIVED).
    std::vector<funkgui::Rect> enginePanels(const Workspace& w);
    bool engineCompact(const Workspace& w);
    // For tests: the name of the layout the block draws (the dial's while the simulator refuses a custom one).
    std::string engineRunningLayout(const Workspace& w);
    // For tests: the full lifts (m, intake and exhaust) the section scales the Scope's lift shares to: the taller of a
    // two-stage lab's cams, as the simulator's Scope divides by.
    std::pair<double, double> engineFullLifts(const Workspace& w);
    // For tests: the cam stage the section names ("STAGE A", "STAGE B"; "" without two stages) and why B is held
    // while the speed asks for it (VALVES' words; "" none).
    std::pair<std::string, std::string> engineStageWords(const Workspace& w);
}
