// ui/ref/MainPanel.cpp: tee::ui::makeMainPanel, the one function through which the application gets its Panel
// (shell/include/tee/app/MainPanel.h declares it; card J defines it). Today that is the reference's own panel
// (RefPanel); card J2's frame of screens takes its place later, behind the same function.
#include "ref/RefPanel.h"

#include <tee/app/MainPanel.h>

#include <memory>

namespace tee::ui
{
    std::unique_ptr<funkgui::Panel> makeMainPanel(tee::Player& player, const tee::AppServices& services)
    {
        return std::make_unique<RefPanel>(refPanelFiles(), player, services, RefPanel::Options{});
    }
}
