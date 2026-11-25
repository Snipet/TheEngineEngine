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
        // U1: the numerals out of the needle's reach (DRIVE's instruments; the owner's rule, 25 Nov: "a needle never
        // crosses a number"): each just inside the rim, the tick ring and the needle ending inside the nearest one's box;
        // a face too small for that keeps its ticks and its digits and shows no numerals. A short needle tail. Off (the
        // default): J2's look, the numerals inside the ticks.
        void setClearNumerals(bool on) { clearNumerals_ = on; }
        // U1: a numeral at every `n`th major tick only (the two ends always): a small dial's few numbers.
        void setNumeralStride(int n) { numeralStride_ = std::max(1, n); }
        // U1: the digits (and the unit's line under them) kept out of the needle's sweep: set low in the arc's open
        // wedge, as far below the hub as half their width (the wedge's sides are the needle at the range's ends), so the
        // needle and the end ticks pass clear of them at any value. A small dial's (DRIVE's CLASSIC). Off (the default):
        // under the hub at a fixed place.
        void setReadoutClear(bool on) { readoutClear_ = on; }

        // Where it draws, as placed now: the hub, the face's radius, the ticks' radius (the sweep reaches ra + 1). A
        // face too small to draw: side < 40.
        struct Geometry
        {
            funkgui::Point hub;
            float side = 0.0f, r = 0.0f, ra = 0.0f;
        };
        Geometry geometry() const;
        // The box the readout `text` (and the unit's line) is drawn in.
        funkgui::Rect readoutBox(const std::string& text) const;
        // Whether the needle at either end of the range (hub to the ticks' outer end, its width included) and the end
        // numerals miss the readout's box with the value at the range's ends and now (for tests).
        bool readoutClearOfSweep() const;
        // The numerals' boxes at the range's ends (none on a face too small for numerals).
        std::vector<funkgui::Rect> endNumerals() const;
        // Every numeral's box as drawn (with its backing).
        std::vector<funkgui::Rect> numeralBoxes() const;
        // Whether the needle at `t` (0..1 of the range: its box, hub to tip, its width included) misses every numeral's
        // box and the readout's (the digits it would show there, and the unit); `why` names what it meets (tests).
        bool labelsClearOfNeedle(double t, std::string& why) const;

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
        struct Label
        {
            std::string text;
            float x = 0.0f, y = 0.0f, hw = 0.0f, hh = 0.0f;   // its centre, its box's half sizes (the backing's)
            float t = 0.0f;                                     // where on the scale, 0..1
            bool red = false;
        };
        // The numerals for `g`'s face; sets g.ra, the tick ring's radius (the needle ends 3 px inside it).
        std::vector<Label> numeralsOf(Geometry& g) const;
        float fractionOf(double v) const noexcept;
        std::string readoutText() const;

        std::function<double()> source_;
        double lo_, hi_, redline_;
        int major_ = 8, minor_ = 2;
        std::function<std::string(double)> numerals_, readout_;
        std::string unit_;
        double value_ = 0.0;
        bool clearNumerals_ = false, readoutClear_ = false;
        int numeralStride_ = 1;
        float needle_ = 0.0f;
    };
}
