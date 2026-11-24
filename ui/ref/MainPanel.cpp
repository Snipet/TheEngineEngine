// ui/ref/MainPanel.cpp: tee::ui::makeMainPanel, the one function the shell asks card J for (docs/plan/w1-b-shell.md):
// the application's main Panel for a Player. Today that is the reference's own panel (RefPanel); card J2's frame of
// screens takes its place later, behind the same function.
//
// AppServices is the shell's (card B names it and has not yet said what is in it): it is taken by reference and not
// read here. When shell/include/tee/app/Sources.h declares makeMainPanel, this definition must match that
// declaration; until then the declaration below is the card's own line.
#include "ref/RefPanel.h"

#include <funkgui/panel/Panel.h>
#include <tee/app/Sources.h>

#include <memory>

namespace tee::ui
{
    struct AppServices;
    std::unique_ptr<funkgui::Panel> makeMainPanel(tee::Player&, const AppServices&);

    std::unique_ptr<funkgui::Panel> makeMainPanel(tee::Player& player, const AppServices&)
    {
        return std::make_unique<RefPanel>(refPanelFiles(), player, RefPanel::Options{});
    }
}
