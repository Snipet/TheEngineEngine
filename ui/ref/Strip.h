// ui/ref/Strip.h: what stands where the reference's panel had its keyboard (there is no MIDI here): the driver's
// controls a mouse can reach. Ours, drawn on FunkGui's Canvas as the kit's items are (kit/gui/Items.h).
//
//   START     the key: held down it is the starter (ControlsFrame::ignition 2), let go the ignition stays on (1)
//   OFF       the ignition off (0)
//   THROTTLE  a bar to drag: the pedal goes where the pointer is and comes back up when it is let go; the wheel over
//             it (and the left and right keys) set a rest position it comes back to instead (a held throttle),
//             a double-click puts that back to 0
//   - GEAR +  a press of - or + is one shift down or up (the counters shiftDown and shiftUp); the gear shown is the
//             telemetry's, when it has a value named gear or showGear
//
// Every change goes out as a whole ControlsFrame through StripModel::send. Keys reach an item only when it has the
// focus and FunkGui sends a Panel no key's release, so Space or Return on START holds the starter for kKeyHold
// seconds; the application's own keyboard handling (the shell's) is the real way to drive.
#pragma once

#include "kit/gui/Items.h"

#include <tee/app/Sources.h>

#include <functional>
#include <string>

namespace tee::ui
{
    struct StripModel
    {
        ControlsFrame frame{};
        std::function<void(const ControlsFrame&)> send;      // the Player's setControls, or the application's own
        float rest = 0.0f;                                   // the throttle's rest position (above)
        int gear = 0;                                        // as the telemetry says: -1 R, 0 N, 1 up
        bool gearKnown = false;
        void push() const
        {
            if (send)
                send(frame);
        }
    };

    // A button of the strip: START (held), OFF, - and + (clicked).
    class StripButton final : public Item
    {
    public:
        enum class What : uint8_t { start, off, shiftDown, shiftUp };
        static constexpr float kKeyHold = 1.5f;              // seconds a key's press holds the starter

        StripButton(uint32_t id, What what, StripModel& model) : id_(id), what_(what), model_(model) {}
        void place(const funkgui::Rect& r, bool shown)
        {
            rect_ = r;
            shown_ = shown;
        }
        const funkgui::Rect& rect() const noexcept { return rect_; }

        void tick(float dt, bool hovered, bool focused, funkgui::Point) override;
        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        bool contains(funkgui::Point p) const override { return shown_ && rect_.contains(p); }
        funkgui::Cursor cursorAt(funkgui::Point) const override { return funkgui::Cursor::pointingHand; }
        void pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&) override;
        void pointerUp(const funkgui::PointerEvent&, funkgui::GestureController&) override;
        bool key(const funkgui::KeyEvent& e, funkgui::GestureController&) override;
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t id, funkgui::A11yAction a, double, funkgui::GestureController&) override;
        void specLine(char* out, std::size_t n) const override;
        bool settled() const override { return !down_ && keyHeld_ <= 0.0f; }
        uint64_t drawKey() const override { return DrawKey{}.mix(down_).mix(hovered_).mix(model_.frame.ignition).h; }
        funkgui::Point centre() const override { return { rect_.centreX(), rect_.centreY() }; }
        bool shown() const override { return shown_; }

    private:
        const char* label() const noexcept;
        void press();
        void release();

        uint32_t id_;
        What what_;
        StripModel& model_;
        funkgui::Rect rect_;
        bool shown_ = false, hovered_ = false, down_ = false;
        float keyHeld_ = 0.0f;
    };

    class StripThrottle final : public Item
    {
    public:
        static constexpr float kStep = 0.05f;                // a wheel notch's or a key's change of the rest position

        StripThrottle(uint32_t id, StripModel& model) : id_(id), model_(model) {}
        void place(const funkgui::Rect& r, bool shown)
        {
            rect_ = r;
            shown_ = shown;
        }
        const funkgui::Rect& rect() const noexcept { return rect_; }
        funkgui::Rect track() const noexcept;                // the bar itself, under its label

        void tick(float, bool hovered, bool, funkgui::Point) override { hovered_ = hovered; }
        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        bool contains(funkgui::Point p) const override { return shown_ && rect_.contains(p); }
        funkgui::Cursor cursorAt(funkgui::Point) const override { return funkgui::Cursor::leftRight; }
        void pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&) override;
        void pointerDrag(const funkgui::PointerEvent& e, funkgui::GestureController&) override;
        void pointerUp(const funkgui::PointerEvent&, funkgui::GestureController&) override;
        void doubleClick(const funkgui::PointerEvent&, funkgui::GestureController&) override;
        bool wheel(const funkgui::WheelEvent& e, funkgui::GestureController&, double) override;
        bool key(const funkgui::KeyEvent& e, funkgui::GestureController&) override;
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t id, funkgui::A11yAction a, double v, funkgui::GestureController&) override;
        void specLine(char* out, std::size_t n) const override;
        bool settled() const override { return !dragging_; }
        uint64_t drawKey() const override { return DrawKey{}.mix(model_.frame.throttle).mix(model_.rest).mix(hovered_).mix(dragging_).h; }
        funkgui::Point centre() const override { return { rect_.centreX(), rect_.centreY() }; }
        bool shown() const override { return shown_; }

    private:
        void set(float throttle);
        void setRest(float rest);
        float at(float x) const noexcept;                    // the pedal's position for a pointer's x

        uint32_t id_;
        StripModel& model_;
        funkgui::Rect rect_;
        bool shown_ = false, hovered_ = false, dragging_ = false;
    };

    // The gear, between the - and + buttons: a readout.
    class StripGear final : public Item
    {
    public:
        StripGear(uint32_t id, const StripModel& model) : id_(id), model_(model) {}
        void place(const funkgui::Rect& r, bool shown)
        {
            rect_ = r;
            shown_ = shown;
        }
        std::string text() const;                            // "N", "R", "3"; "-" while the telemetry has none

        void tick(float, bool, bool, funkgui::Point) override {}
        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool) const override;
        bool contains(funkgui::Point) const override { return false; }
        void pointerDown(const funkgui::PointerEvent&, funkgui::GestureController&, funkgui::HostServices&) override {}
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t, funkgui::A11yAction, double, funkgui::GestureController&) override { return false; }
        void specLine(char* out, std::size_t n) const override;
        uint64_t drawKey() const override { return DrawKey{}.mix(model_.gear).mix(model_.gearKnown).h; }
        funkgui::Point centre() const override { return { rect_.centreX(), rect_.centreY() }; }
        bool shown() const override { return shown_; }
        bool focusable() const override { return false; }

    private:
        uint32_t id_;
        const StripModel& model_;
        funkgui::Rect rect_;
        bool shown_ = false;
    };
}
