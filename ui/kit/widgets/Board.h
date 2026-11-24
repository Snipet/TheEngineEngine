// ui/kit/widgets/Board.h: a funkgui::Panel made of Items (kit/gui/Items.h) that its owner places: the widgets' gallery,
// a test's rig, and the application's frame (ui/app/), which puts its rail, top bar and strip on a Board around a
// screen. It does for its items what J1's GuiPanel does for a .gui file's: hover, a press and its drag to the item
// pressed, the focus by click and Tab (with a ring), keys to the item with the focus (a text field takes them all), the
// wheel to the item under the pointer, accessibility, gestures through one funkgui::GestureController.
//
// An inner Panel (inner()) lies under the board's items: what no item of the board's takes goes to it (a press and
// its drag, the wheel, keys while no item of the board's has the focus, accessibility), it is ticked and drawn first,
// and the board's items are drawn over it. The application's frame's screen is one.
//
// Accessibility ids: item i's are kIdBase + kStride * (i + 1) + 0..63 (nextId()), apart from an inner Panel's, which
// stay below kIdBase. As every Panel: time only from tick(dt), draw() changes nothing. UI thread only.
#pragma once

#include "kit/gui/Items.h"

#include <funkgui/panel/HostServices.h>
#include <funkgui/panel/Panel.h>
#include <funkgui/params/GestureController.h>

#include <memory>
#include <utility>
#include <vector>

namespace tee::ui
{
    class Board : public funkgui::Panel
    {
    public:
        static constexpr uint32_t kIdBase = 0x01000000u;
        static constexpr uint32_t kStride = 64;

        Board(int width, int height, int minWidth = 320, int minHeight = 200);
        ~Board() override;

        // An item, its index returned; give it nextId() as its accessibility id when it is made.
        int add(std::unique_ptr<Item> item);
        template <typename T, typename... Args>
        T& make(Args&&... args)
        {
            auto p = std::make_unique<T>(nextId(), std::forward<Args>(args)...);
            T& r = *p;
            add(std::move(p));
            return r;
        }
        uint32_t nextId() const noexcept { return kIdBase + kStride * static_cast<uint32_t>(items_.size() + 1); }
        Item& item(int i) { return *items_[static_cast<std::size_t>(i)]; }
        int count() const noexcept { return static_cast<int>(items_.size()); }
        int focus() const noexcept { return focus_; }
        funkgui::HostServices* host() const noexcept { return host_; }
        funkgui::GestureController* gestures() const noexcept { return gestures_.get(); }

        // funkgui::Panel
        void attach(funkgui::HostServices& host) override;
        int width() const override { return width_; }
        int height() const override { return height_; }
        void tick(float dt) override;
        void draw(funkgui::Canvas& c, const funkgui::Theme& th) override;
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
        uint32_t a11yRevision() const override;
        void a11yAction(uint32_t id, funkgui::A11yAction a, double value) override;
        void closeGestures() override;
        bool resizable() const override { return true; }
        int minWidth() const override { return minW_; }
        int minHeight() const override { return minH_; }
        void resize(int w, int h) override;

    protected:
        // The owner's: place the items (and the inner Panel) for width() x height(); called at attach and every resize.
        virtual void arrange() {}
        // The ground under everything (the theme's).
        virtual void drawGround(funkgui::Canvas& c, const funkgui::Theme& th) const;
        // The Panel under the items; nullptr: none.
        virtual funkgui::Panel* inner() const { return nullptr; }
        // Called once the host is known (an inner Panel is attached here).
        virtual void attached() {}
        void bump() noexcept { ++revision_; }            // items appeared, moved or went (accessibility)
        void setFocus(int item);

    private:
        int itemAt(funkgui::Point p) const;
        bool moveFocus(int step);

        std::vector<std::unique_ptr<Item>> items_;
        int width_, height_, minW_, minH_;
        funkgui::HostServices* host_ = nullptr;
        std::unique_ptr<funkgui::GestureController> gestures_;
        funkgui::Point pointer_{};
        bool hasPointer_ = false, innerPress_ = false, ring_ = false;
        int pressed_ = -1, focus_ = -1;
        uint32_t revision_ = 1;
    };
}
