// ui/app/Panes.h: Panels side by side in one window, as one Panel (the frame's inner Panel: the workspace's stage and
// the inspector's page). Each pane is a whole-window Panel whose content keeps to its rectangle (a Workspace keeps to
// its stage, a FieldPage to its insets); Panes draws each clipped to its rectangle and gives it the input that falls
// there: a press and its drag to the pane pressed, the wheel to the pane under the pointer, keys to the pane last
// pressed. Accessibility ids are each pane's own with the pane's number in the top four bits (ids below 2^28).
// UI thread only.
#pragma once

#include <funkgui/canvas/Canvas.h>
#include <funkgui/panel/HostServices.h>
#include <funkgui/panel/Panel.h>

#include <vector>

namespace tee::ui
{
    class Panes final : public funkgui::Panel
    {
    public:
        struct Pane
        {
            funkgui::Panel* panel = nullptr;   // attached by its owner
            funkgui::Rect rect{};
        };

        Panes(int width, int height) : width_(width), height_(height) {}
        // The panes now (the owner's, at every layout and switch); a press in progress on a pane that went is ended.
        void set(std::vector<Pane> panes);
        const std::vector<Pane>& panes() const noexcept { return panes_; }

        // funkgui::Panel
        void attach(funkgui::HostServices& host) override { host_ = &host; }
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
        bool magnify(const funkgui::MagnifyEvent& e) override;
        bool key(const funkgui::KeyEvent& e) override;
        funkgui::Cursor cursor() const override;
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        uint32_t a11yRevision() const override;
        void a11yAction(uint32_t id, funkgui::A11yAction a, double value) override;
        void closeGestures() override;
        bool resizable() const override { return true; }
        void resize(int w, int h) override { width_ = w; height_ = h; }

        static constexpr uint32_t kShift = 28;

    private:
        int at(float x, float y) const;
        funkgui::Panel* pane(int i) const { return i >= 0 && i < static_cast<int>(panes_.size()) ? panes_[static_cast<std::size_t>(i)].panel : nullptr; }

        std::vector<Pane> panes_;
        int width_, height_;
        funkgui::HostServices* host_ = nullptr;
        int pressed_ = -1, hover_ = -1, keys_ = -1;
        uint32_t revision_ = 1;
    };
}
