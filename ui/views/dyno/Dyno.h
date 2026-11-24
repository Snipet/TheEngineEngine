// ui/views/dyno/Dyno.h: the DYNO workspace's factory and what its tests reach (DynoWorkspace.cpp).
#pragma once

#include "app/Workspace.h"

#include <funkgui/core/Geometry.h>

namespace tee::ui
{
    std::unique_ptr<Workspace> makeDynoWorkspace(WorkspaceContext& c);
    // Where a control of the stage is: "start", "stop", "name", "copy", "engine", "chassis", "none", "boost",
    // "lambda", "advance" (an empty rect: no such control).
    funkgui::Rect dynoControl(Workspace& w, const char* which);
}
