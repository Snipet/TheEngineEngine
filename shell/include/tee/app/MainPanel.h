// shell/include/tee/app/MainPanel.h: the one function through which the application gets its Panel. The shell
// declares it; the interface package (card J) defines it, in its target tee_ui. A build with no such target gets the
// shell's own plain Panel under this name (shell/panel/PlaceholderMain.cpp).
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
