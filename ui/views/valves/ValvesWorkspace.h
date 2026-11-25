// ui/views/valves/ValvesWorkspace.h: the VALVES workspace (docs/ui/valves.md): the cam editor with its handles, the
// timing ring, the flow bench, the phasers and stages, valve float and deactivation, live.
#pragma once

#include "app/Workspace.h"

#include <memory>
#include <string>
#include <vector>

namespace tee::ui
{
    std::unique_ptr<Workspace> makeValvesWorkspace(WorkspaceContext& c);
    // For tests: the panels drawn now (no text may cross one's edge).
    std::vector<funkgui::Rect> valvesPanels(const Workspace& w);
    // For tests: where a cam's handle is now (0..2 the intake's peak, opening, closing; 3..5 the exhaust's), in window
    // coordinates; {-1, -1} when the build has no cams to edit.
    funkgui::Point valvesHandle(const Workspace& w, int h);
    // For tests: what the oil holds back, in the words the stage shows ("" : nothing): the phasers parked below 313 K
    // or 70 kPa, stage B waiting for 333 K and 100 kPa.
    std::string valvesOilWords(const Workspace& w);
    // For tests: the overlap's pressure as the ring says it ("-63 kPa BACK-FILLS", "+12 kPa SCAVENGES"; "" none).
    std::string valvesOverlapWords(const Workspace& w);
    // For tests: words the page draws from the build: 0 the intake phaser's band ("INTAKE PHASER 60 DEG"), 1 the
    // exhaust's, 2 the stage's speed ("B from 3100 rpm"); "" where the build has none.
    std::string valvesPageWords(const Workspace& w, int which);
}
