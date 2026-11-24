// ui/views/car/Car.h: the CAR workspace's factory and what its tests reach (CarWorkspace.cpp).
#pragma once

#include "app/Workspace.h"

namespace tee::ui
{
    std::unique_ptr<Workspace> makeCarWorkspace(WorkspaceContext& c);
    // The top speed the gearing chart marks (m/s; NaN: no predicted curve), the gear it is in (from 0), and whether
    // the limiter holds it rather than drag.
    double carTopSpeed(Workspace& w, int* gear = nullptr, bool* limiterBound = nullptr);
}
