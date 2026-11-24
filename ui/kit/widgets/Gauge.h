// ui/kit/widgets/Gauge.h: a round instrument (card J2, item 6): an arc of 270 degrees, major and minor ticks, numerals,
// a red band from the redline up, a needle (a segment from the hub) that eases to the value, and the value in digits
// under the hub. value() is read each tick (a telemetry value, in whatever unit the owner gives: the frame shows rpm).
#pragma once

#include "kit/widgets/Widget.h"

#include <algorithm>
#include <functional>
#include <string>
#include <vector>

namespace tee::ui
{
    class Gauge final : public Widget
    {
    public:
        Gauge(uint32_t id, std::string title, std::function<double()> value, double lo, double hi);

        // The red band from `from` to the top (none: NaN, the default).
        void setRedline(double from) { redline_ = from; }
        // `major` divisions with numerals, each cut in `minor` (1: no minor ticks).
        void setTicks(int major, int minor) { major_ = std::max(1, major); minor_ = std::max(1, minor); }
        // A numeral's text (the tachometer's 1 for 1000 rpm); none: the number itself.
        void setNumerals(std::function<std::string(double)> f) { numerals_ = std::move(f); }
        // The digits under the hub; none: the value with no decimals. And the unit under them ("RPM", "X1000").
        void setReadout(std::function<std::string(double)> f) { readout_ = std::move(f); }
        void setUnit(std::string u) { unit_ = std::move(u); }
        void setRange(double lo, double hi) { lo_ = lo; hi_ = hi; }
        // U1: numerals set clear of the ticks by their own size, over the needle on a backing of the face, and a short
        // needle tail (DRIVE's instruments). Off (the default): J2's look.
        void setClearNumerals(bool on) { clearNumerals_ = on; }

        double value() const noexcept { return value_; }
        float needle() const noexcept { return needle_; }   // where the needle is, 0..1 of the arc
        // The needle's angle for a fraction of the arc (radians, 0 = +x, clockwise on screen: the Canvas's).
        static float angleOf(float t) noexcept { return kA0 + (kA1 - kA0) * t; }
        static constexpr float kA0 = 2.35619449f, kA1 = 7.06858347f;   // 0.75 pi to 2.25 pi

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        bool focusable() const override { return false; }
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t, funkgui::A11yAction, double, funkgui::GestureController&) override { return false; }
        uint64_t drawKey() const override;

    protected:
        void step(float dt) override;
        bool still() const override { return needle_ == fractionOf(value_); }
        const char* kind() const override { return "gauge"; }

    private:
        float fractionOf(double v) const noexcept;
        std::string readoutText() const;

        std::function<double()> source_;
        double lo_, hi_, redline_;
        int major_ = 8, minor_ = 2;
        std::function<std::string(double)> numerals_, readout_;
        std::string unit_;
        double value_ = 0.0;
        bool clearNumerals_ = false;
        float needle_ = 0.0f;
    };
}
