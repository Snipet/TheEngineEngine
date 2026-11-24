// shell/panel/PlaceholderMain.cpp: tee::ui::makeMainPanel for a build with no interface package in it (no target
// tee_ui): the shell's plain Panel. app/CMakeLists.txt compiles this file only then.
#include "panel/PlaceholderPanel.h"
#include "tee/app/MainPanel.h"

namespace tee::ui
{
    std::unique_ptr<funkgui::Panel> makeMainPanel(tee::Player& player, const tee::AppServices& services)
    {
        return makePlaceholderPanel(player, services);
    }
}
