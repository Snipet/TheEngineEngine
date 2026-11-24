// shell/include/tee/app/MainPanel.h: the one function through which the application gets its Panel. The shell
// declares it; the interface package (card J) defines it. Until J's lands, shell/panel/PlaceholderPanel.cpp defines a
// plain one (TEE_PLACEHOLDER_PANEL, on by default while no target tee_ui exists).
//
// The Panel keeps the Player's reference and a copy of the AppServices' functions: the Player and whatever those
// functions reach outlive it.
#pragma once

#include "tee/app/Sources.h"

#include <funkgui/panel/Panel.h>

#include <memory>

namespace tee::ui
{
    std::unique_ptr<funkgui::Panel> makeMainPanel(tee::Player&, const tee::AppServices&);
}
