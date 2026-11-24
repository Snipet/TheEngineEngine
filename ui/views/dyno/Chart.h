// ui/views/dyno/Chart.h: the small drawing pieces the TUNE, CAR and DYNO stages share (package U4): round ticks, a
// polyline broken into dashes (a run is told from another by its dash as well as its colour), a value looked up on a
// curve, and a plot frame with its grid. Canvas primitives only: opaque strokes, segments, discs, text of 10 px or more.
#pragma once

#include "app/Look.h"
#include "kit/widgets/Widget.h"

#include <funkgui/canvas/Canvas.h>
#include <funkgui/core/Geometry.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>

namespace tee::ui::chart
{
    inline constexpr double kPi = 3.14159265358979323846;
    inline constexpr double kRpm = kPi / 30.0;   // rad/s in one rpm

    // A round step (1, 2, 2.5 or 5 times a power of ten) near `span / count`.
    inline double niceStep(double span, int count)
    {
        if (!(span > 0.0) || count < 1)
            return 1.0;
        const double raw = span / count, mag = std::pow(10.0, std::floor(std::log10(raw)));
        const double f = raw / mag;
        return (f <= 1.0 ? 1.0 : f <= 2.0 ? 2.0 : f <= 2.5 ? 2.5 : f <= 5.0 ? 5.0 : 10.0) * mag;
    }

    // A dash pattern: lengths on, off, on, off (px). Empty: a solid line.
    struct Dash
    {
        float a = 0.0f, b = 0.0f, c = 0.0f, d = 0.0f;
        bool solid() const noexcept { return a <= 0.0f; }
    };

    // A polyline through (xs, ys), n points, in `dash`. A point that is not finite breaks the line.
    inline void stroke(funkgui::Canvas& c, const std::vector<float>& xs, const std::vector<float>& ys, float width,
                       funkgui::Col ink, Dash dash = {})
    {
        const int n = static_cast<int>(std::min(xs.size(), ys.size()));
        if (dash.solid())
        {
            int start = 0;
            for (int i = 0; i <= n; ++i)
                if (i == n || !std::isfinite(xs[static_cast<std::size_t>(i)]) || !std::isfinite(ys[static_cast<std::size_t>(i)]))
                {
                    if (i - start >= 2)
                        c.polyline(xs.data() + start, ys.data() + start, i - start, width, ink);
                    start = i + 1;
                }
            return;
        }
        const float pat[4] = { dash.a, dash.b, dash.c > 0.0f ? dash.c : dash.a, dash.d > 0.0f ? dash.d : dash.b };
        int k = 0;
        float left = pat[0];
        for (int i = 0; i + 1 < n; ++i)
        {
            float x0 = xs[static_cast<std::size_t>(i)], y0 = ys[static_cast<std::size_t>(i)];
            const float x1 = xs[static_cast<std::size_t>(i + 1)], y1 = ys[static_cast<std::size_t>(i + 1)];
            if (!std::isfinite(x0) || !std::isfinite(y0) || !std::isfinite(x1) || !std::isfinite(y1))
                continue;
            float len = std::hypot(x1 - x0, y1 - y0);
            if (len <= 0.0f)
                continue;
            const float ux = (x1 - x0) / len, uy = (y1 - y0) / len;
            while (len > 0.0f)
            {
                const float run = std::min(left, len);
                if ((k & 1) == 0)
                    c.segment(x0, y0, x0 + ux * run, y0 + uy * run, width, ink);
                x0 += ux * run;
                y0 += uy * run;
                len -= run;
                left -= run;
                if (left <= 0.0f)
                {
                    k = (k + 1) & 3;
                    left = pat[k];
                }
            }
        }
    }

    // A short sample of a stroke (a legend's), from x0 to x1 at y.
    inline void sample(funkgui::Canvas& c, float x0, float x1, float y, float width, funkgui::Col ink, Dash dash = {})
    {
        stroke(c, { x0, x1 }, { y, y }, width, ink, dash);
    }

    // y at x on a curve given as rising xs (straight lines between; NaN outside).
    inline double at(const std::vector<double>& xs, const std::vector<double>& ys, double x)
    {
        const std::size_t n = std::min(xs.size(), ys.size());
        if (n == 0 || !std::isfinite(x) || x < xs.front() || x > xs[n - 1])
            return std::nan("");
        for (std::size_t i = 1; i < n; ++i)
            if (x <= xs[i])
            {
                const double t = xs[i] > xs[i - 1] ? (x - xs[i - 1]) / (xs[i] - xs[i - 1]) : 0.0;
                return ys[i - 1] * (1.0 - t) + ys[i] * t;
            }
        return ys[n - 1];
    }

    // A linear map from data to px.
    struct Scale
    {
        double d0 = 0.0, d1 = 1.0;
        float p0 = 0.0f, p1 = 1.0f;
        float px(double v) const noexcept
        {
            return d1 != d0 ? p0 + static_cast<float>((v - d0) / (d1 - d0)) * (p1 - p0) : p0;
        }
        double data(float p) const noexcept { return p1 != p0 ? d0 + (p - p0) / (p1 - p0) * (d1 - d0) : d0; }
    };

    inline std::string fixed(double v, int decimals)
    {
        return look::num(v, decimals <= 0 ? "%.0f" : decimals == 1 ? "%.1f" : "%.2f");
    }
    // The decimals a tick step needs.
    inline int decimalsFor(double step) { return step >= 1.0 ? 0 : step >= 0.1 ? 1 : 2; }

    // Drawing over the widgets made before it (a trail over a table, a note over a plot): an item that takes no press,
    // no focus and no key, drawn in its turn. `draw` must change nothing; `key` mixes what it reads.
    class Layer final : public Widget
    {
    public:
        Layer(uint32_t id, std::function<void(funkgui::Canvas&, const funkgui::Theme&)> draw,
              std::function<std::uint64_t()> key)
            : Widget(id), draw_(std::move(draw)), key_(std::move(key))
        {
        }
        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool) const override
        {
            if (draw_)
                draw_(c, th);
        }
        bool contains(funkgui::Point) const override { return false; }
        bool focusable() const override { return false; }
        void accessibility(std::vector<funkgui::A11yItem>&) const override {}
        bool a11yAction(uint32_t, funkgui::A11yAction, double, funkgui::GestureController&) override { return false; }
        uint64_t drawKey() const override { return key_ ? key_() : 0; }

    protected:
        const char* kind() const override { return "layer"; }

    private:
        std::function<void(funkgui::Canvas&, const funkgui::Theme&)> draw_;
        std::function<std::uint64_t()> key_;
    };
}
