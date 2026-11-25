// ui/app/AppFrame.h: the application's window (docs/plan/ui-design.md section 2; package U1, grown from J2's frame).
//
//   +------+---------------------------------------------------------------------------------------------+
//   | top bar: the wordmark, the build's name (* unsaved), the engine in a sentence; load, output, units, |
//   |          undo, redo                                                                                  |
//   +------+------------------------------------------------------------+--------------------------------+
//   | rail |  STAGE: the workspace's picture of its part of the machine  |  INSPECTOR: what is selected,  |
//   |GARAGE|  (a Workspace, Workspace.h), live; click a part to select it|  its fields (J2's generated    |
//   |ENGINE|                                                            |  pages, or the workspace's     |
//   | ...  |                                                            |  .gui page), tabs when several |
//   |      |                                                            +--------------------------------+
//   |      |                                                            |  DYNO RIBBON (folds away)      |
//   +------+------------------------------------------------------------+--------------------------------+
//   | cockpit: START, mode, the rpm bar (numerals, redline, shift lights), gear, speed, boost, lambda,     |
//   |          temperatures, the three pedals                                                            |
//   +---------------------------------------------------------------------------------------------------+
//
// The workspaces: GARAGE ENGINE VALVES AIR EXHAUST TUNE CAR DYNO DRIVE SCOPE. Each is the one its package registered
// (ui/views/<id>/register.h), or until then a placeholder (Placeholders.h) whose stage says what will be there and
// whose inspector is the generated pages of its Spec sections, so every field is reachable from the first day. A
// section no workspace names is added to ENGINE's inspector.
//
// The frame is a Board (its own items: the rail, the top bar, the inspector's header and tabs, the ribbon, the
// cockpit) over Panes (the workspace's stage and the inspector's FieldPage). Smallest window 1100 x 700; the rail
// narrows to short words under 1280 wide. needsDraw() is true only when something drawn changed: an input, an ease
// still moving, a live value, a field, the dyno. As every Panel: time only from tick(dt), draw() changes nothing.
// The keys of docs/controls.md are the shell's: the frame takes only Cmd/Ctrl-Z (undo, Shift: redo) and Cmd/Ctrl-Y,
// and lets every other key through unless a text field has the caret.
#pragma once

#include "app/Cockpit.h"
#include "app/LoadMeter.h"
#include "app/TryFirst.h"
#include "app/Panes.h"
#include "app/Workspace.h"
#include "kit/bind/Binding.h"
#include "kit/bind/Fields.h"
#include "kit/bind/Units.h"
#include "kit/widgets/Basic.h"
#include "kit/widgets/Board.h"

#include <tee/app/Sources.h>

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tee::ui
{
    // A workspace on the rail: its id, its words (whole and short), the Spec sections a placeholder shows, what its
    // stage will show (a placeholder's text), and the package that makes it.
    struct WorkspaceSlot
    {
        std::string id, title, shortTitle;
        std::vector<std::string> sections;
        std::string promise;
        std::string package;
        bool ribbon = true;
    };
    // The ten of ui-design.md section 3, in the rail's order.
    std::vector<WorkspaceSlot> defaultSlots();

    class AppFrame final : public Board
    {
    public:
        static constexpr float kTopH = 48.0f, kRailW = 112.0f, kRailNarrowW = 64.0f, kCockpitH = 78.0f;
        static constexpr float kInspectorW = 340.0f, kRibbonH = 210.0f, kRibbonFoldedH = 30.0f, kHeadH = 34.0f;
        static constexpr int kMinW = 1100, kMinH = 700;

        // `dyno` and `scope` may be null (a source that has none: an empty curve, no arrays).
        AppFrame(FieldDoc& doc, Units& units, TelemetrySource& telemetry, FrameServices services,
                 DynoSource* dyno = nullptr, ScopeSource* scope = nullptr, std::vector<WorkspaceSlot> slots = defaultSlots(),
                 int width = 1280, int height = 800);
        ~AppFrame() override;

        // The workspaces.
        int workspaceCount() const noexcept { return static_cast<int>(workspaces_.size()); }
        Workspace& workspace(int i) const { return *workspaces_[static_cast<std::size_t>(i)]; }
        const WorkspaceSlot& slot(int i) const { return slots_[static_cast<std::size_t>(i)]; }
        int find(std::string_view id) const;                 // -1: none
        int shown() const noexcept { return shown_; }
        Workspace& current() const { return workspace(shown_); }
        void show(int i);
        bool placeholder(int i) const;                       // the stand-in of a package not here yet

        // J2's names, kept for the shell's --workspace (app/Offline.cpp): a "screen" is a workspace, named by its id;
        // its "page" an inspector page. show(s, page) shows the workspace, then the page (kept in range).
        struct Screen
        {
            std::string name;
        };
        int screenCount() const noexcept { return workspaceCount(); }
        Screen screenDef(int s) const { return { slot(s).id }; }
        int pageCount(int s) const { return s == shown_ ? inspectorPageCount() : 64; }
        void show(int s, int page)
        {
            show(s);
            showInspectorPage(std::clamp(page, 0, std::max(0, inspectorPageCount() - 1)));
        }

        // The selection and the inspector.
        Selection& selection() noexcept { return selection_; }
        void select(std::string path) { selection_.select(std::move(path)); }
        int inspectorPageCount() const noexcept { return static_cast<int>(pages_.size()); }
        int inspectorPageIndex() const noexcept { return page_; }
        const std::string& inspectorTitle(int p) const;
        FieldPage* inspector() const;                        // the page shown (nullptr: the units page)
        void showInspectorPage(int p);
        // The settings page (the top bar's SETTINGS; the inspector of a workspace with no Spec section): the audio
        // device, the theme, the units, the keys. unitsShown/showUnits are J2's names for it.
        bool unitsShown() const noexcept { return unitsPage_; }
        bool settingsShown() const noexcept { return unitsPage_; }
        void showUnits(bool on);
        void showSettings(bool on) { showUnits(on); }
        bool ribbonFolded() const noexcept { return folded_; }
        void foldRibbon(bool folded);

        // The builds on disk (GARAGE's; the shell's implementation): ticked once a frame. None: GARAGE uses its own.
        void setLibrary(garage::Library* library);
        // The stage the frame gives a workspace in a window of w x h (the same whatever the ribbon does): at the
        // smallest window, 1100 x 700, {64, 48, 696, 574}; at 1600 x 1000, {112, 48, 1128, 874}.
        static funkgui::Rect stageFor(int w, int h);

        // Where things are (window px), for tests and probes.
        float railWidth() const noexcept;
        funkgui::Rect railRect(int i) const;
        funkgui::Rect stageRect() const;
        funkgui::Rect inspectorRect() const;                 // the page's room (under the header and tabs)
        funkgui::Rect ribbonRect() const;                    // empty: no ribbon in this workspace
        funkgui::Rect cockpitRect() const;
        funkgui::Rect startRect() const;
        funkgui::Rect pedalRect(int pedal) const;            // 0 throttle, 1 brake, 2 clutch
        funkgui::Rect inspectorTabRect(int p) const;
        RpmBar& rpmBar() const { return *rpm_; }
        const std::vector<Stepper*>& unitSteppers() const noexcept { return unitSteppers_; }
        int railFocus() const;                               // the rail button with the keyboard's focus; -1: none
        PedalBar& pedal(int i) const { return *pedals_[static_cast<std::size_t>(i)]; }
        const PanelControls& panelControls() const noexcept { return controls_; }
        std::string engineSentence() const;

        // Every field an inspector page shows a control or a widget for, by the document's index, over every workspace
        // with nothing selected (it shows each in turn and comes back): every field that is not hidden must be in it.
        std::vector<int> fieldsShown();

        // How many frames were drawn (draw() calls), and whether one is wanted now.
        int drawCount() const noexcept { return draws_; }

        // funkgui::Panel
        void tick(float dt) override;
        void draw(funkgui::Canvas& c, const funkgui::Theme& th) override;
        bool needsDraw() const override;
        bool key(const funkgui::KeyEvent& e) override;
        void pointerDown(const funkgui::PointerEvent& e) override;
        void pointerDrag(const funkgui::PointerEvent& e) override;
        void pointerUp(const funkgui::PointerEvent& e) override;
        void pointerMove(const funkgui::PointerEvent& e) override;
        bool wheel(const funkgui::WheelEvent& e) override;
        bool wantsFullRate() const override;

    protected:
        void arrange() override;
        void attached() override;
        funkgui::Panel* inner() const override { return panes_.get(); }
        void drawGround(funkgui::Canvas& c, const funkgui::Theme& th) const override;
        void drawOver(funkgui::Canvas& c, const funkgui::Theme& th) const override;

    private:
        struct Page
        {
            std::string title, gui;
            std::unique_ptr<FieldPage> panel;
        };
        void makeWorkspaces();
        void makeCockpit();
        void makeInspector();                                // the pages for the current workspace and selection
        void placeInspector();
        void setPanes();
        int tabRows() const noexcept;
        std::vector<std::string> sectionsOf(int ws) const;
        std::uint64_t frameKey() const;
        void report();

        FieldDoc& doc_;
        Units& units_;
        TelemetrySource& telemetry_;
        FrameServices services_;
        std::unique_ptr<DynoSource> ownDyno_;
        std::unique_ptr<ScopeSource> ownScope_;
        DynoSource* dyno_;
        ScopeSource* scope_;
        Selection selection_;
        std::unique_ptr<WorkspaceContext> ctx_;
        std::vector<WorkspaceSlot> slots_;
        std::vector<std::unique_ptr<Workspace>> workspaces_;
        std::vector<bool> placeholders_;
        std::vector<std::string> rest_;                      // sections no workspace names (ENGINE's inspector)
        std::unique_ptr<Panes> panes_;
        int shown_ = 0;
        // the inspector
        std::vector<Page> pages_;
        int page_ = 0;
        bool unitsPage_ = false, folded_ = false;
        std::uint32_t selectionSeen_ = 0, unitsSeen_ = 0;
        std::string inspectorHead_, pagesFor_;
        std::vector<Button*> tabs_;
        Button *clear_ = nullptr, *fold_ = nullptr, *unitsButton_ = nullptr;
        // the settings page (unitsPage_): audio, theme, units, keys
        Label* audioLine_ = nullptr;
        Button *audioMenu_ = nullptr, *graphite_ = nullptr, *paper_ = nullptr, *full_ = nullptr, *economy_ = nullptr;
        Button *warmStart_ = nullptr, *coldStart_ = nullptr;   // ENGINE START (FrameServices::coldStart)
        float startNoteY_ = -1.0f;
        float qualityNoteY_ = 0.0f, settingsHeadX_ = 0.0f;
        std::vector<std::pair<std::string, float>> settingsHeads_;   // a heading and its y (arrange's)
        float keysY_ = 0.0f;
        std::vector<Stepper*> unitSteppers_;
        std::vector<std::unique_ptr<funkgui::ParamPort>> unitPorts_;
        DynoRibbon* ribbon_ = nullptr;
        // the top bar and the rail
        std::vector<Button*> rail_;
        Widget* railMarks_ = nullptr;
        Label *name_ = nullptr, *sentence_ = nullptr, *status_ = nullptr;
        Bar *level_ = nullptr, *limiter_ = nullptr;
        LoadMeter* load_ = nullptr;       // the simulator's peak share of a core, its verdict, the dropouts
        LoadAdvice* advice_ = nullptr;    // the sentence while the build does not fit at this quality
        Button* adviceButton_ = nullptr;  // QUALITY: the settings page
        TryFirstCard* tryFirst_ = nullptr;    // the first launch's card, over the stage's top right
        Button* tryFirstClose_ = nullptr;
        std::string tryFirstNotes_;           // what the card shows now (empty: none)
        bool tryFirstClosed_ = false, sawRunning_ = false;
        void closeTryFirst();
        std::string bakeWords() const;   // the bake's state as the status line says it (empty: nothing to say)
        static std::string bakeWordsOf(const std::string& state);   // the same, a state without its count
        bool bakeShown_ = false;
        Button *undo_ = nullptr, *redo_ = nullptr;
        // the cockpit
        Button* start_ = nullptr;
        std::vector<Button*> modes_;
        RpmBar* rpm_ = nullptr;
        std::vector<Readout*> readouts_;
        BrakeLamp* brake_ = nullptr;   // a truck's engine brake (card D2): shown while engineBrakeState(ctx).shown
        std::vector<PedalBar*> pedals_;
        PanelControls controls_{};
        int localMode_ = 0;
        // drawing only when something changed
        bool dirty_ = true;
        std::uint64_t drawnKey_ = 0;
        int draws_ = 0;
    };

    // The frame over a document: what the shell shows once it has a Build (docs/handoffs/u1-frame.md).
    std::unique_ptr<AppFrame> makeAppFrame(FieldDoc& doc, Units& units, TelemetrySource& telemetry, FrameServices services,
                                           DynoSource* dyno = nullptr, ScopeSource* scope = nullptr);
}
