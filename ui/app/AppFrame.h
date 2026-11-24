// ui/app/AppFrame.h: the application's frame of screens (card J2, item 9; docs/plan/00-abstract-plan.md section 6).
//
//   +------+--------------------------------------------------------------------+
//   | rail | top bar: the build's name, what plays, the engine's state, the      |
//   |      | output's level, undo, redo, START (held)                            |
//   | BUILD|--------------------------------------------------------------------+
//   | VALVE| (a strip of the screen's pages, when it has more than one)          |
//   | BOOST|                                                                    |
//   | ...  | the screen: a FieldPage (a .gui page of generated sections, until   |
//   |      | wave 2 gives the screen its own view)                              |
//   |      +--------------------------------------------------------------------+
//   |      | bottom strip: the pedals as bars, the gear, the speed, the engine   |
//   +------+--------------------------------------------------------------------+
//
// The screens: Build, Valvetrain, Boost, Exhaust, Tune, Dyno, Drive, Scope, Garage. Each but the Garage is either a
// list of Spec sections (a generated page each, cut into pages of at most 60 controls) or a .gui text (with `auto`
// sections, gauges, plots). A section no screen names goes to the Build screen, so every field that is not hidden can
// be seen and set from the frame with no hand-written C++ for it. The Garage lists the builds, loads and saves them,
// renames this one, and chooses the units (Units.h); a change of units makes the pages again.
//
// The frame is a Board (ui/kit/widgets/Board.h): its own items are the rail, the bars and the garage; the screen's
// page is the inner Panel. Dark, an instrument's look, one accent (funkgui::Theme::graphite). As every Panel: time only
// from tick(dt), draw() changes nothing. UI thread only.
#pragma once

#include "kit/bind/Binding.h"
#include "kit/bind/Fields.h"
#include "kit/bind/Units.h"
#include "kit/widgets/Basic.h"
#include "kit/widgets/Board.h"

#include <tee/app/Sources.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tee::ui
{
    struct Screen
    {
        std::string name;                    // on the rail: "Build"
        std::vector<std::string> sections;   // a generated page each (in order); or
        std::string gui;                     // a .gui text, one page
        bool garage = false;                 // the garage (its own items)
    };

    // The nine of 00-abstract-plan.md section 6, as wave 1 has them.
    std::vector<Screen> defaultScreens();

    // What the frame asks of the application, besides the document, the units and the telemetry. Any may be empty.
    struct FrameServices
    {
        AppServices app;                                          // the start button, the pedals, the status line
        std::function<std::string()> buildName;
        std::function<bool(const std::string&)> rename;
        std::function<void()> undo, redo;
        std::function<std::vector<std::string>()> builds;         // the garage's list
        std::function<bool(const std::string& name, std::string& error)> load;
        std::function<bool(const std::string& name, std::string& error)> save;   // as that name
        std::function<float()> outputLevel;                       // 0..1; none: the telemetry's output peak
    };

    class AppFrame final : public Board
    {
    public:
        static constexpr float kRailW = 124.0f, kTopH = 52.0f, kPagesH = 32.0f, kBottomH = 62.0f;

        AppFrame(FieldDoc& doc, Units& units, TelemetrySource& telemetry, FrameServices services,
                 std::vector<Screen> screens = defaultScreens(), int width = 1280, int height = 800);
        ~AppFrame() override;

        int screenCount() const noexcept { return static_cast<int>(screens_.size()); }
        const Screen& screenDef(int s) const { return screens_[static_cast<std::size_t>(s)]; }
        int screen() const noexcept { return screen_; }
        int page() const noexcept { return page_; }
        int pageCount(int s) const;
        // A screen's page (nullptr for the garage's, or none).
        FieldPage* pageOf(int s, int p) const;
        // The page shown (nullptr on the garage).
        FieldPage* shownPage() const { return pageOf(screen_, page_); }
        void show(int screen, int page = 0);
        // The title of a screen's page ("BOOST", "BOOST 2").
        const std::string& pageTitle(int s, int p) const;

        // Where things are, for tests and probes.
        funkgui::Rect railRect(int s) const;
        funkgui::Rect pageTabRect(int p) const;
        funkgui::Rect startRect() const;
        funkgui::Rect contentRect() const;
        // The garage's items.
        List& garageList() const { return *garageList_; }
        Button& garageLoad() const { return *garageLoad_; }
        Button& garageSave() const { return *garageSave_; }
        TextField& garageName() const { return *garageName_; }
        const std::string& garageMessage() const noexcept { return message_; }
        // Every field the frame's pages show a control or a widget for, by the document's index (a test's: every field
        // not hidden must be in it).
        std::vector<int> fieldsShown() const;

        // funkgui::Panel
        void tick(float dt) override;
        bool key(const funkgui::KeyEvent& e) override;

    protected:
        void arrange() override;
        void attached() override;
        funkgui::Panel* inner() const override;
        void drawOver(funkgui::Canvas& c, const funkgui::Theme& th) const override;

    private:
        struct Page
        {
            std::string title;
            std::string gui;
            std::unique_ptr<FieldPage> panel;
        };
        struct ScreenState
        {
            std::vector<Page> pages;
        };

        void makePages();
        void makeGarage();
        void placeGarage(bool shown);
        std::string stateText() const;

        FieldDoc& doc_;
        Units& units_;
        TelemetrySource& telemetry_;
        FrameServices services_;
        std::vector<Screen> screens_;
        std::vector<ScreenState> states_;
        std::vector<Button*> rail_, pageTabs_;
        int screen_ = 0, page_ = 0;
        std::uint32_t unitsSeen_ = 0;
        int stateHandle_ = -1, gearHandle_ = -1, speedHandle_ = -1, rpmHandle_ = -1;
        // the top bar and the strip
        Label *name_ = nullptr, *status_ = nullptr, *state_ = nullptr, *gear_ = nullptr, *speed_ = nullptr, *rpm_ = nullptr;
        Bar *level_ = nullptr, *throttle_ = nullptr, *brake_ = nullptr, *clutch_ = nullptr;
        Button *start_ = nullptr, *undo_ = nullptr, *redo_ = nullptr;
        // the garage
        List* garageList_ = nullptr;
        Button *garageLoad_ = nullptr, *garageSave_ = nullptr;
        TextField* garageName_ = nullptr;
        Label* garageNote_ = nullptr;
        std::vector<Stepper*> unitSteppers_;
        std::vector<std::unique_ptr<funkgui::ParamPort>> unitPorts_;
        std::string message_;
        std::vector<std::string> builds_;
        PanelControls controls_{};
    };

    // The frame over a document: what the shell shows once it has a Build (docs/ui-kit.md, "The application's frame").
    std::unique_ptr<AppFrame> makeAppFrame(FieldDoc& doc, Units& units, TelemetrySource& telemetry, FrameServices services);
}
