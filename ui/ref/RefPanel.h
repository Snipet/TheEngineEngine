// ui/ref/RefPanel.h: the reference engine's own panel in our window (card J, item 4): a GuiPanel over
// sim/ref/panel.gui and its three shaders (header.sc, cluster.sc, layout.sc), bound to a tee::Player:
//
//   its knobs, menus and switches   the Player's parameters, by the names the .gui file uses (the reference's dials)
//   shared->showRpm and the like    the Player's telemetry of that name without "shared->" (showRpm): the
//                                   reference's own Shared fields, raw, as its shaders read them
//   out                             the output's peak
//   its keyboard                    not drawn (no MIDI). In its place, the strip (Strip.h): START, a throttle bar, the
//                                   gear between - and +. A panel file with no keyboard gets the strip under it
//                                   instead.
//
// The strip never drives the engine itself: it says what it asks for through AppServices::panelControls, and shows the
// pedal the application last sent (AppServices::controls). AppServices::status, when its text starts with "error:"
// (the program did not compile, or was stopped), is shown across the panel's top, its first line.
//
// refPanelFiles() is the reference's panel as the build embedded it (ui/ref/CMakeLists.txt); makeMainPanel
// (MainPanel.cpp) makes a RefPanel of them.
#pragma once

#include "kit/gui/GuiPanel.h"
#include "ref/Strip.h"

#include <tee/app/Sources.h>

#include <functional>
#include <string>

namespace tee::ui
{
    // sim/ref's panel.gui, header.sc, cluster.sc and layout.sc (and any .sh beside them); empty in a build made
    // before sim/ref/ was in the tree.
    Sources refPanelFiles();

    class RefPanel final : public GuiPanel
    {
    public:
        static constexpr float kStripH = 56.0f;              // the strip's height when the file has no keyboard
        static constexpr float kStripPad = 8.0f;

        struct Options
        {
            int width = 0, height = 0;                       // 0: the panel's natural size
            std::function<void(float* out, int n)> scope;    // GuiPanel::Options::scope
            bool shaders = true;                             // GuiPanel::Options::shaders
        };

        RefPanel(Sources files, Player& player, const AppServices& services, Options options);

        // The strip, for tests and probes.
        const StripModel& strip() const noexcept { return model_; }
        funkgui::Rect stripRect() const noexcept { return strip_; }
        funkgui::Rect startRect() const;
        funkgui::Rect throttleRect() const;                  // the bar itself
        funkgui::Rect shiftRect(bool up) const;
        std::string gearText() const;

        void tick(float dt) override;
        void draw(funkgui::Canvas& c, const funkgui::Theme& theme) override;

    protected:
        void placed() override;

    private:
        Player& player_;
        AppServices services_;                               // a copy of the application's functions
        StripModel model_;
        bool hasFiles_ = false, hasKeyboard_ = false;
        int gearHandle_ = -1, stateHandle_ = -1;
        std::string error_;                                  // the status' first line, while it is an error
        int start_ = -1, throttle_ = -1, down_ = -1, gear_ = -1, up_ = -1;   // items()
        funkgui::Rect strip_;
    };
}
