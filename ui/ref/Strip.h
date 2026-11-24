// ui/ref/Strip.h: what stands where the reference's panel had its keyboard (there is no MIDI here): the driver's
// controls a mouse can reach. Ours, drawn on FunkGui's Canvas as the kit's items are (kit/gui/Items.h).
//
//   START     the key: while it is held, PanelControls::startHeld (the shell makes of that what a car's key does:
//             held is the starter; once the engine runs, a press of a second switches it off)
//   THROTTLE  a bar to drag: the pedal goes where the pointer is and comes back up when it is let go; the wheel over
//             it (and the left and right keys) set a rest position it comes back to instead (a held throttle), a
//             double-click puts that back to 0. The bar shows the pedal as the application last sent it
//             (AppServices::controls: the keyboard's and a gamepad's too), when it says
//   - GEAR +  a press of - or + is one shift down or up (the counters shiftDown and shiftUp); the gear shown is the
//             telemetry's (car.gear), when it has one
//
// The strip never drives the engine itself: every change goes out as a tee::PanelControls through StripModel::send
// (AppServices::panelControls), which the shell merges with the keyboard's and the devices'. Keys reach an item only
// when it has the focus and FunkGui sends a Panel no key's release, so Space or Return on START holds it for
// kKeyHold seconds; the shell's own keyboard handling is the real way to drive.
#pragma once

#include "kit/gui/Items.h"

#include <tee/app/Sources.h>

#include <functional>
#include <string>

namespace tee::ui
{
    struct StripModel
    {
        PanelControls out{};                                 // what the strip asks for now
        std::function<void(const PanelControls&)> send;      // AppServices::panelControls (none: nobody hears)
        float rest = 0.0f;                                   // the throttle's rest position (above)
        bool dragging = false;                               // the bar is held
        float drag = 0.0f;                                   // ... at this position
        float pedal = 0.0f;                                  // the pedal as the bar shows it
        int gear = 0;                                        // as the telemetry says: -1 R, 0 N, 1 up
        bool gearKnown = false;
        bool running = false;                                // the engine's state is cranking or running (START lit)
        // The throttle the strip asks for: the drag's, else the rest position, else none (-1: the pedal is not ours).
        void push()
        {
            out.throttle = dragging ? drag : (rest > 0.0f ? rest : -1.0f);
            if (send)
                send(out);
        }
    };

    // A button of the strip: START (held), - and + (clicked).
    class StripButton final : public Item
    {
    public:
        enum class What : uint8_t { start, shiftDown, shiftUp };
        static constexpr float kKeyHold = 1.5f;              // seconds a key's press holds START

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
        uint64_t drawKey() const override { return DrawKey{}.mix(down_).mix(hovered_).mix(model_.running).h; }
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
        bool settled() const override { return !model_.dragging; }
        uint64_t drawKey() const override { return DrawKey{}.mix(model_.pedal).mix(model_.rest).mix(hovered_).mix(model_.dragging).h; }
        funkgui::Point centre() const override { return { rect_.centreX(), rect_.centreY() }; }
        bool shown() const override { return shown_; }

    private:
        void setRest(float rest);
        float at(float x) const noexcept;                    // the pedal's position for a pointer's x

        uint32_t id_;
        StripModel& model_;
        funkgui::Rect rect_;
        bool shown_ = false, hovered_ = false;
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
