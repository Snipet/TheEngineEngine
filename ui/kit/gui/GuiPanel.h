// ui/kit/gui/GuiPanel.h: from TestingPlayground, Source/editor/PanelView.h at 47bac40327ed9e8e59928500de9f3beb63fe8636 (ui/kit/SEED.tsv); what was changed: docs/ui-kit.md.
// GuiPanel.h: a .gui file's interface as a funkgui::Panel, bound to a tee::ParamSource (what its controls turn) and a
// tee::TelemetrySource (what its displays, live= marks and shader inputs read). The view's code is GuiView's; this is
// the Panel around it. It grew from the plugin's GUI sandbox's panel (a picture with mock values and no input) and adds
// what the plugin's own Panel did for a GuiView: the pointer, the wheel, the keys, focus and Tab, menus, scrolling,
// resizing and accessibility.
//
//   parameters   the source's parameter i is slot i (kMaxControls at most; a source with more shows its first 64 and
//                says so in notes()). The .gui file names a parameter by its ParamInfo::id.
//   fields       a display (or live=, or a shader's input) names a telemetry value: `rpm`, or `shared->rpm` as the
//                reference's panel writes it (the part after -> is the name asked of TelemetrySource::findValue). A name
//                the source does not have is a note, as an unknown field was in the plugin.
//   out          the output's peak (TelemetrySource::outputPeak). There is no input (`in` is left out) and, until the
//                shell gives one, no access to the output's samples: a scope or a spectrum of `out` reads what
//                Options::scope gives (nothing: a flat line).
//   keyboard     laid out (its cell keeps the room) and never drawn: there is no MIDI. keyboardRects() says where, so
//                an owner can put something of its own there (ui/ref's strip).
//
// A subclass may add items of its own (addItem): they are ticked, drawn after the interface, hit before it and take
// their place in the Tab order after it; placed() is called after every layout so that it can put them somewhere.
//
// As every Panel: time only from tick(dt), nothing read from a clock or the environment, and draw() changes nothing.
// UI thread only.
#pragma once

#include "kit/gui/DisplayItem.h"
#include "kit/gui/GuiView.h"
#include "kit/gui/Items.h"
#include "kit/gui/Ports.h"

#include "kit/gui/Controls.h"
#include "kit/gui/Meters.h"
#include "kit/gui/Sources.h"
#include "kit/gui/PanelSpec.h"

#include <funkgui/panel/HostServices.h>
#include <funkgui/panel/Panel.h>
#include <funkgui/params/GestureController.h>
#include <funkgui/params/ParamPort.h>

#include <tee/app/Sources.h>

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tee::ui
{
    // The fields a .gui file's displays may name among `telemetry`'s: every word of `gui` that the source has a value
    // for, by the name the file uses (`rpm`, `shared->rpm`), each a float.
    std::vector<PanelField> telemetryFields(const std::string& gui, const TelemetrySource& telemetry);

    class GuiPanel : public funkgui::Panel
    {
    public:
        struct Options
        {
            int width = 0, height = 0;           // the size it opens at; 0: the interface's natural size
            int minWidth = 320, minHeight = 200; // the least a window may make it (it scrolls when smaller than natural)
            std::string name = "panel";          // the name its shaders were built under (cmake/TeeShaders.cmake's)
            // The output's last n samples, oldest first, for a scope or a spectrum of `out`; none: silence.
            std::function<void(float* out, int n)> scope;

            // ---- J2 (card J, items 8 and 9) ----
            // A widget element's item (plot, gauge, map, curve, bar, lamp: GuiNode::widget, what it shows in
            // GuiNode::file), made once with its accessibility id; none (or a null item) shows nothing there. The item
            // is placed in the element's cell after every layout and scroll (Item::setPlace), and drawn clipped to the
            // interface's area.
            std::function<std::unique_ptr<Item>(const GuiNode& node, uint32_t id)> widget;
            // `auto "boost"`: the .gui text that stands for a generated page, on one line (`;` between elements), so
            // the file's line numbers hold. None: an `auto` line is a note.
            std::function<std::string(std::string_view section)> autoPage;
            // Room kept around the interface for an owner's own things (the application's frame: its rail, top bar and
            // bottom strip): the interface is laid out inside it, and the natural size grows by it.
            int insetLeft = 0, insetTop = 0, insetRight = 0, insetBottom = 0;
        };

        // `files`: the panel's .gui file (the first whose name ends in .gui) and the shaders and .sh files it names.
        GuiPanel(Sources files, ParamSource& params, TelemetrySource& telemetry, Options options);
        ~GuiPanel() override;

        // What the .gui reader said ("panel.gui:12:5: note: ..."), a line each; "" when it took everything.
        const std::string& notes() const noexcept { return notes_; }
        const PanelSpec& spec() const noexcept { return spec_; }
        const Controls& controls() const noexcept { return controls_; }
        GuiView& view() noexcept { return view_; }
        const GuiView& view() const noexcept { return view_; }
        // The interface's own size (its root's, at no scroll).
        int naturalWidth() const noexcept { return naturalW_; }
        int naturalHeight() const noexcept { return naturalH_; }
        // Where the file's keyboards would be (see above), in the panel's px; those on the pages shown.
        std::vector<funkgui::Rect> keyboardRects() const;
        // J2: the widget elements' items (Options::widget), with their nodes, in the file's order.
        struct WidgetAt
        {
            int item = -1;
            int node = -1;
        };
        const std::vector<WidgetAt>& widgets() const noexcept { return widgets_; }

        // For tests and probes: a control's place by its parameter's id (an empty rect: not shown), a tabs element's
        // tab by its page's title, the item with the focus (-1: none), an item by index.
        funkgui::Rect controlRect(std::string_view paramId) const;
        funkgui::Rect tabRect(std::string_view pageTitle) const;
        int focus() const noexcept { return focus_; }
        const std::vector<std::unique_ptr<Item>>& items() const noexcept { return items_; }
        int slotOf(std::string_view paramId) const;
        SourcePort* port(int slot) noexcept;

        // funkgui::Panel
        void attach(funkgui::HostServices& host) override;
        int width() const override { return width_; }
        int height() const override { return height_; }
        void tick(float dt) override;
        void draw(funkgui::Canvas& c, const funkgui::Theme& theme) override;
        bool wantsFullRate() const override;
        void pointerMove(const funkgui::PointerEvent& e) override;
        void pointerExit() override;
        void pointerDown(const funkgui::PointerEvent& e) override;
        void pointerDrag(const funkgui::PointerEvent& e) override;
        void pointerUp(const funkgui::PointerEvent& e) override;
        void doubleClick(const funkgui::PointerEvent& e) override;
        bool wheel(const funkgui::WheelEvent& e) override;
        bool key(const funkgui::KeyEvent& e) override;
        funkgui::Cursor cursor() const override;
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        uint32_t a11yRevision() const override { return revision_; }
        void a11yAction(uint32_t id, funkgui::A11yAction a, double value) override;
        void closeGestures() override;
        bool resizable() const override { return true; }
        int minWidth() const override { return options_.minWidth; }
        int minHeight() const override { return options_.minHeight; }
        void resize(int w, int h) override;

    protected:
        // An item of the subclass's own (above); its index in items().
        int addItem(std::unique_ptr<Item> item);
        uint32_t nextId() const noexcept;                // an accessibility id for the item about to be added (its
                                                         // own first, then its parts)
        // After every layout (a resize, a tab chosen): the subclass places its own items.
        virtual void placed() {}
        // Room kept under the interface for the subclass's own items (px; the natural height grows by it), and the
        // layout made again (a subclass's constructor, once it has added its items).
        void setBottomReserve(int px);
        void relayout() { layout(); }
        int bottomReserve() const noexcept { return reserve_; }
        funkgui::HostServices* host() const noexcept { return host_; }
        const funkgui::Theme& lastTheme() const noexcept { return theme_; }
        void changed() noexcept { dirty_ = true; }       // something the subclass draws changed: a frame is wanted

    private:
        void layout();
        void fillMeters();
        int itemAt(funkgui::Point p) const;
        void setFocus(int item);
        bool moveFocus(int step);
        void scrollBy(float dx, float dy);
        void placeWidgets();
        bool isWidget(std::size_t item) const noexcept;

        static constexpr uint32_t kA11yStride = 64;      // item i's ids are kA11yStride * (i + 1) + 0..63
        mutable std::size_t idItems_ = static_cast<std::size_t>(-1);
        mutable uint32_t idNext_ = 0;

        Options options_;
        Sources files_;
        ParamSource& params_;
        TelemetrySource& telemetry_;
        std::string notes_;
        Controls controls_{};
        PanelSpec spec_;
        std::vector<std::unique_ptr<SourcePort>> ports_;
        std::vector<funkgui::ParamPort*> portList_;
        GuiView::Items items_;
        std::vector<int> displays_;
        GuiView view_;
        std::size_t firstOwn_ = 0;                       // items_ from here on are the subclass's (J2's widgets first)
        std::vector<WidgetAt> widgets_;
        std::vector<int> tapHandle_;                     // by tap: the telemetry's handle (-1: none)
        Meters meters_{};
        DisplayFeed feed_;
        int width_ = 0, height_ = 0, naturalW_ = 0, naturalH_ = 0;
        int reserve_ = 0;
        funkgui::HostServices* host_ = nullptr;
        std::unique_ptr<funkgui::GestureController> gestures_;
        funkgui::Theme theme_ = funkgui::Theme::graphite();
        funkgui::Point pointer_{};
        bool hasPointer_ = false;
        int pressed_ = -1, focus_ = -1;
        bool ring_ = false;                              // the focus shows its ring (it was reached by Tab)
        bool barPress_ = false;                          // a scroll bar's thumb is held
        bool dirty_ = true;
        uint32_t revision_ = 1;
    };
}
