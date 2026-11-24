// shell/panel/PlaceholderPanel.h: the shell's own plain Panel: the engine's speed, gear and state as numbers, a start
// button, a throttle strip, FunkGui's Dial for every parameter in a grid, and the application's status (a compile
// error in the compiler's words). It needs nothing but FunkGui's core, so it draws on any machine the window opens on.
//
// The application shows the interface package's Panel (tee::ui::makeMainPanel, card J) when that is in the build, and
// this one with `--panel plain`, or always when it is not (PlaceholderMain.cpp then defines makeMainPanel as this).
#pragma once

#include "tee/app/Sources.h"

#include <funkgui/panel/Panel.h>

#include <memory>

namespace tee::ui
{
    std::unique_ptr<funkgui::Panel> makePlaceholderPanel(tee::Player&, const tee::AppServices&);
}
