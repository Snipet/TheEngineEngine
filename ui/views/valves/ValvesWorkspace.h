// ui/views/valves/ValvesWorkspace.h: the VALVES workspace (docs/ui/valves.md): the cam editor with its handles, the
// timing ring, the flow bench, the phasers and stages, valve float and deactivation, live.
#pragma once

#include "app/Workspace.h"

#include <memory>

namespace tee::ui
{
    std::unique_ptr<Workspace> makeValvesWorkspace(WorkspaceContext& c);
}
