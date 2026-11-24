// ui/app/Cockpit.h: the instruments of the cockpit strip (docs/plan/ui-design.md section 2), each a Widget the frame
// places: the tachometer as a wide bar (RpmBar), a readout (Readout), a pedal a mouse can drag (PedalBar). All of them
// read through functions each tick, ease what moves, and draw with Canvas primitives and the look (Look.h).
#pragma once

#include "app/Look.h"
#include "app/Workspace.h"
#include "kit/bind/Units.h"
#include "kit/widgets/Widget.h"

#include <functional>
#include <string>

namespace tee::ui
{
    // The engine's speed as a wide bar: numerals every 1000 rpm, the redline zone (red, and its own hatching), five
    // shift lights over the right end (two green, two amber, one red, lit in turn as the redline nears; "SHIFT" at the
    // last), the limiter's word while it holds, the speed in digits at the right. Values in rpm; NaN draws a dash.
    class RpmBar final : public Widget
    {
    public:
        RpmBar(uint32_t id, std::function<double()> rpm, std::function<double()> redline, std::function<bool()> limiter);

        double scaleTop() const noexcept;          // the bar's right end, rpm
        int lightsLit() const noexcept;            // 0..5
        float shownRpm() const noexcept { return shown_; }   // where the fill is, rpm

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        bool focusable() const override { return false; }
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t, funkgui::A11yAction, double, funkgui::GestureController&) override { return false; }
        uint64_t drawKey() const override;

    protected:
        void step(float dt) override;
        bool still() const override;
        const char* kind() const override { return "rpm"; }

    private:
        std::function<double()> rpm_, redline_;
        std::function<bool()> limiter_;
        double target_ = 0.0, red_ = 7000.0;
        bool limiting_ = false;
        float shown_ = 0.0f;
    };

    // A number with its label over it and its unit beside it (look::readout), in an ink by meaning when ink() is given.
    class Readout final : public Widget
    {
    public:
        Readout(uint32_t id, std::string label, std::function<std::string()> number, std::function<std::string()> unit = {},
                funkgui::TextStyle style = look::kNumberM);
        // The number's colour (kClear: the ordinary ink), with a word that says the same (shown under the number).
        void setInk(std::function<funkgui::Col(const look::Palette&)> ink) { ink_ = std::move(ink); }
        const std::string& number() const noexcept { return number_; }

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        bool focusable() const override { return false; }
        bool contains(funkgui::Point) const override { return false; }
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t, funkgui::A11yAction, double, funkgui::GestureController&) override { return false; }
        uint64_t drawKey() const override;

    protected:
        void step(float dt) override;
        const char* kind() const override { return "readout"; }

    private:
        std::function<std::string()> numberOf_, unitOf_;
        std::function<funkgui::Col(const look::Palette&)> ink_;
        funkgui::TextStyle style_;
        std::string number_, unit_;
    };

    // A pedal as a vertical bar: it shows what the application last sent (sent(): a key's, a gamepad's, the mouse's),
    // and a press and drag on it holds it at the pointer's height (onHold(true, v) as it moves, onHold(false, 0) at the
    // release). Its short name under it ("THR"), the percentage over it.
    class PedalBar final : public Widget
    {
    public:
        PedalBar(uint32_t id, std::string name, std::function<float()> sent, std::function<void(bool held, float v)> onHold,
                 funkgui::Col (*ink)(const look::Palette&));
        bool held() const noexcept { return held_; }
        float shownValue() const noexcept { return shown_; }
        funkgui::Rect track() const noexcept;

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        bool focusable() const override { return false; }
        funkgui::Cursor cursorAt(funkgui::Point) const override { return funkgui::Cursor::pointingHand; }
        void pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&) override;
        void pointerDrag(const funkgui::PointerEvent& e, funkgui::GestureController&) override;
        void pointerUp(const funkgui::PointerEvent& e, funkgui::GestureController&) override;
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t, funkgui::A11yAction, double, funkgui::GestureController&) override { return false; }
        uint64_t drawKey() const override;

    protected:
        void step(float dt) override;
        bool still() const override { return shown_ == target_; }
        const char* kind() const override { return "pedal"; }

    private:
        float at(float y) const noexcept;
        std::function<float()> sent_;
        std::function<void(bool, float)> onHold_;
        funkgui::Col (*ink_)(const look::Palette&);
        bool held_ = false;
        float hold_ = 0.0f, target_ = 0.0f, shown_ = 0.0f;
    };

    // The dyno ribbon: the predicted torque and power of the build as it stands, the curve before the last edit ghosted
    // behind it, the peaks in numbers; torque against the left scale, power against the right. Folded, only its title
    // row. Speeds in rpm, torque and power in the user's units.
    class DynoRibbon final : public Widget
    {
    public:
        DynoRibbon(uint32_t id, const DynoSource& dyno, const Units& units);
        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        bool focusable() const override { return false; }
        bool contains(funkgui::Point) const override { return false; }
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t, funkgui::A11yAction, double, funkgui::GestureController&) override { return false; }
        uint64_t drawKey() const override;
        std::string peaksText() const;

    protected:
        const char* kind() const override { return "dyno"; }

    private:
        const DynoSource& dyno_;
        const Units& units_;
    };
}
