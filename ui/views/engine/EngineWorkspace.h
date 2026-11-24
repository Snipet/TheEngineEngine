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

namespace tee::ui
{
    std::unique_ptr<Workspace> makeEngineWorkspace(WorkspaceContext& c);
}
