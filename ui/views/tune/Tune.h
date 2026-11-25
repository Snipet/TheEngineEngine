// ui/views/tune/Tune.h: the TUNE workspace's factory and what its tests reach (TuneWorkspace.cpp).
#pragma once

#include "app/Workspace.h"
#include "kit/widgets/Tables.h"

namespace tee::ui
{
    std::unique_ptr<Workspace> makeTuneWorkspace(WorkspaceContext& c);
    // The table of the map shown (nullptr: the map is not in this build), and the map chosen by its row (0 spark ...).
    MapEdit* tuneMapEdit(Workspace& w);
    void tuneChoose(Workspace& w, int map);
    // Where the BAKE button is (empty: not shown).
    funkgui::Rect tuneBakeButton(Workspace& w);
    // The fuel band's Prev (dir < 0) and Next buttons (empty: not shown), its words ("name | octane | effect"), and
    // which map is shown.
    funkgui::Rect tuneFuelButton(Workspace& w, int dir);
    std::string tuneFuelText(Workspace& w);
    int tuneShownMap(Workspace& w);
}
