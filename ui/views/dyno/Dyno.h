// ui/views/dyno/Dyno.h: the DYNO workspace's factory and what its tests reach (DynoWorkspace.cpp).
#pragma once

#include "app/Workspace.h"

#include <funkgui/core/Geometry.h>

#include <string>

namespace tee::ui
{
    std::unique_ptr<Workspace> makeDynoWorkspace(WorkspaceContext& c);
    // Where a control of the stage is: "start", "stop", "name", "copy", "engine", "chassis", "sweep", "steps", "settle",
    // "none", "boost",
    // "lambda", "advance" (an empty rect: no such control).
    funkgui::Rect dynoControl(Workspace& w, const char* which);
    // A row's name as the runs list says it (-2 predicted, -1 before the last edit, 0.. the runs).
    std::string dynoRowName(Workspace& w, int row);
    // (U1 for U4, 26 Nov) The words the cursor says of a row's curve at a speed (SI): its knock's, the program's doubt
    // (DynoTrace::distrustText) and its boost's ripple (rippleText), a line each; "" for an ordinary point. And the
    // dashed rings (points the program distrusts) the last draw made on the plot.
    std::string dynoWordsAt(Workspace& w, int row, double si);
    int dynoUnsteadyDrawn(Workspace& w);
    float dynoPlotX(Workspace& w, double si);   // where a speed (SI) is across the plot, px (NaN: not DYNO)
}
