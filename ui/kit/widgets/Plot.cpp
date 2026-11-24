// ui/kit/widgets/Plot.cpp: see Plot.h.
#include "kit/widgets/Plot.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace tee::ui
{
    namespace
    {
        using funkgui::Canvas;
        using funkgui::Col;
        using funkgui::Rect;

        constexpr float kLeft = 46.0f, kRight = 12.0f, kTop = 20.0f, kBottom = 30.0f;
        constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();

        // An axis's value as a place 0..1 across its view [v0, v1], and back (log: in decades).
        double toT(bool log, double v0, double v1, double v)
        {
            if (log)
            {
                if (!(v > 0.0) || !(v0 > 0.0) || !(v1 > 0.0))
                    return kNaN;
                return (std::log10(v) - std::log10(v0)) / (std::log10(v1) - std::log10(v0));
            }
            return (v - v0) / (v1 - v0);
        }

        double fromT(bool log, double v0, double v1, double t)
        {
            if (log)
                return std::pow(10.0, std::log10(v0) + t * (std::log10(v1) - std::log10(v0)));
            return v0 + t * (v1 - v0);
        }

        // The view [v0, v1] kept inside the axis's range and no wider than it.
        void clampView(const PlotAxis& a, double& v0, double& v1)
        {
            const double lo = a.log ? std::log10(std::max(a.lo, 1e-30)) : a.lo;
            const double hi = a.log ? std::log10(std::max(a.hi, 1e-30)) : a.hi;
            double u0 = a.log ? std::log10(std::max(v0, 1e-30)) : v0, u1 = a.log ? std::log10(std::max(v1, 1e-30)) : v1;
            const double span = std::min(u1 - u0, hi - lo);
            const double least = (hi - lo) * 1e-4;
            const double w = std::max(span, least);
            u0 = std::clamp(u0, lo, hi - w);
            u1 = u0 + w;
            v0 = a.log ? std::pow(10.0, u0) : u0;
            v1 = a.log ? std::pow(10.0, u1) : u1;
        }
    }

    Plot::Plot(uint32_t id, std::string title) : Widget(id)
    {
        title_ = std::move(title);
    }

    void Plot::setView(double xlo, double xhi, double ylo, double yhi)
    {
        vx0_ = xlo;
        vx1_ = xhi;
        vy0_ = ylo;
        vy1_ = yhi;
        clampView(x_, vx0_, vx1_);
        clampView(y_, vy0_, vy1_);
        ++revision_;
    }

    void Plot::resetView() { setView(x_.lo, x_.hi, y_.lo, y_.hi); }

    void Plot::zoomX(double factor, double about)
    {
        const double t = toT(x_.log, vx0_, vx1_, about);
        if (!std::isfinite(t) || !(factor > 0.0))
            return;
        const double a = fromT(x_.log, vx0_, vx1_, t - t / factor), b = fromT(x_.log, vx0_, vx1_, t + (1.0 - t) / factor);
        setView(a, b, vy0_, vy1_);
    }

    void Plot::zoomY(double factor, double about)
    {
        const double t = toT(y_.log, vy0_, vy1_, about);
        if (!std::isfinite(t) || !(factor > 0.0))
            return;
        const double a = fromT(y_.log, vy0_, vy1_, t - t / factor), b = fromT(y_.log, vy0_, vy1_, t + (1.0 - t) / factor);
        setView(vx0_, vx1_, a, b);
    }

    Rect Plot::plotArea() const noexcept
    {
        return { bounds_.x + kLeft, bounds_.y + kTop, std::max(1.0f, bounds_.w - kLeft - kRight),
                 std::max(1.0f, bounds_.h - kTop - kBottom) };
    }

    float Plot::pxX(double x) const noexcept
    {
        const Rect a = plotArea();
        return a.x + static_cast<float>(toT(x_.log, vx0_, vx1_, x)) * a.w;
    }

    float Plot::pxY(double y) const noexcept
    {
        const Rect a = plotArea();
        return a.bottom() - static_cast<float>(toT(y_.log, vy0_, vy1_, y)) * a.h;
    }

    double Plot::dataX(float px) const noexcept
    {
        const Rect a = plotArea();
        return fromT(x_.log, vx0_, vx1_, (px - a.x) / a.w);
    }

    double Plot::dataY(float py) const noexcept
    {
        const Rect a = plotArea();
        return fromT(y_.log, vy0_, vy1_, (a.bottom() - py) / a.h);
    }

    double Plot::yAt(const PlotSeries& s, double x) noexcept
    {
        const std::size_t n = std::min(s.x.size(), s.y.size());
        for (std::size_t i = 1; i < n; ++i)
        {
            const double x0 = s.x[i - 1], x1 = s.x[i];
            if ((x >= x0 && x <= x1) || (x <= x0 && x >= x1))
            {
                const double t = x1 != x0 ? (x - x0) / (x1 - x0) : 0.0;
                return s.y[i - 1] + (s.y[i] - s.y[i - 1]) * t;
            }
        }
        return n == 1 && s.x[0] == x ? s.y[0] : kNaN;
    }

    void Plot::ticks(const PlotAxis& a, double v0, double v1, float px, std::vector<double>& out, std::vector<bool>& major)
    {
        out.clear();
        major.clear();
        if (!(v1 > v0) || !(px > 0.0f))
            return;
        if (a.log && v0 > 0.0)
        {
            const double decades = std::log10(v1) - std::log10(v0);
            const double perDecade = px / std::max(decades, 1e-9);
            for (int d = static_cast<int>(std::floor(std::log10(v0))); d <= static_cast<int>(std::ceil(std::log10(v1))); ++d)
                for (int m = 1; m <= 9; ++m)
                {
                    const double v = static_cast<double>(m) * std::pow(10.0, d);
                    if (v < v0 * (1 - 1e-9) || v > v1 * (1 + 1e-9))
                        continue;
                    const bool numbered = m == 1 || (perDecade > 160.0 && (m == 2 || m == 5));
                    if (m != 1 && perDecade < 50.0)
                        continue;
                    out.push_back(v);
                    major.push_back(numbered);
                }
            return;
        }
        const double count = std::max(2.0, static_cast<double>(px) / 70.0);
        const double raw = (v1 - v0) / count;
        const double mag = std::pow(10.0, std::floor(std::log10(raw)));
        double step = mag;
        for (const double k : { 1.0, 2.0, 2.5, 5.0, 10.0 })
            if (mag * k >= raw)
            {
                step = mag * k;
                break;
            }
        for (double v = std::ceil(v0 / step - 1e-9) * step; v <= v1 + step * 1e-9; v += step)
        {
            out.push_back(std::fabs(v) < step * 1e-9 ? 0.0 : v);
            major.push_back(true);
        }
    }

    std::string Plot::numberOf(const PlotAxis& a, double v, double step) const
    {
        if (a.format)
            return a.format(v);
        char buf[48];
        if (a.log && std::fabs(v) >= 1000.0)
        {
            std::snprintf(buf, sizeof buf, "%gk", v / 1000.0);
            return buf;
        }
        const int d = step > 0.0 ? std::clamp(static_cast<int>(-std::floor(std::log10(step) + 1e-9)), 0, 4) : 2;
        std::snprintf(buf, sizeof buf, "%.*f", d, v);
        std::string s = buf;
        if (s.find_first_not_of("-0.") == std::string::npos && s.front() == '-')
            s.erase(0, 1);
        return s;
    }

    Col Plot::colorOf(const Look& l, std::size_t i) const noexcept
    {
        if (i < series_.size() && series_[i].color)
            return *series_[i].color;
        const Col inks[] = { l.accent, l.value, l.signal, l.label };
        return inks[i % 4];
    }

    bool Plot::cursorShown() const noexcept { return hover_ > 0.5f && plotArea().contains(pointer_) && !panning_; }

    void Plot::drawFrame(Canvas& c, const Look& l) const
    {
        const Rect a = plotArea();
        c.rrect(a.x, a.y, a.w, a.h, 0.0f, l.over(l.edge, 0.3f));
        std::vector<double> tv;
        std::vector<bool> major;
        // x: grid lines, ticks, numbers under the area.
        ticks(x_, vx0_, vx1_, a.w, tv, major);
        double step = tv.size() > 1 ? tv[1] - tv[0] : 0.0;
        for (std::size_t i = 0; i < tv.size(); ++i)
        {
            const float x = c.snapX(pxX(tv[i]));
            c.hairlineV(x, a.y, a.h, major[i] ? l.over(l.edge, 0.9f) : l.over(l.edge, 0.5f));
            if (major[i])
                c.text(numberOf(x_, tv[i], x_.log ? tv[i] : step).c_str(), x, a.bottom() + 4.0f, kWidgetSmall, l.label,
                       funkgui::Align::centre);
        }
        if (!x_.label.empty())
            c.text(upper(x_.label).c_str(), a.right(), a.bottom() + 16.0f, kWidgetLabel, l.dim, funkgui::Align::right);
        // y: grid lines, numbers at the left.
        ticks(y_, vy0_, vy1_, a.h, tv, major);
        step = tv.size() > 1 ? tv[1] - tv[0] : 0.0;
        for (std::size_t i = 0; i < tv.size(); ++i)
        {
            const float y = c.snapY(pxY(tv[i]));
            c.hairlineH(a.x, y, a.w, major[i] ? l.over(l.edge, 0.9f) : l.over(l.edge, 0.5f));
            if (major[i])
                c.text(numberOf(y_, tv[i], y_.log ? tv[i] : step).c_str(), a.x - 5.0f, c.capCentreTop(y, kWidgetSmall),
                       kWidgetSmall, l.label, funkgui::Align::right);
        }
        c.rrect(a.x, a.y, a.w, a.h, 0.0f, kClear, 1.0f, l.edge);
        // The title and the y axis's label over the area.
        std::string head = upper(title_);
        if (!y_.label.empty())
            head += head.empty() ? upper(y_.label) : "  (" + upper(y_.label) + ")";
        c.text(head.c_str(), bounds_.x, bounds_.y + 3.0f, kWidgetLabel, l.label);
    }

    void Plot::drawSeries(Canvas& c, const Look& l) const
    {
        const Rect a = plotArea();
        const Canvas::ClipScope clip(c, { c.snapX(a.x), c.snapY(a.y), a.w, a.h });
        for (std::size_t si = 0; si < series_.size(); ++si)
        {
            const PlotSeries& s = series_[si];
            const Col col = colorOf(l, si);
            const std::size_t n = std::min(s.x.size(), s.y.size());
            std::size_t i = 0;
            while (i < n)                                // each unbroken run of points
            {
                xs_.clear();
                ys_.clear();
                for (; i < n; ++i)
                {
                    const float px = pxX(s.x[i]), py = pxY(s.y[i]);
                    if (!std::isfinite(px) || !std::isfinite(py))
                    {
                        ++i;
                        break;
                    }
                    xs_.push_back(px);
                    ys_.push_back(py);
                }
                const int m = static_cast<int>(xs_.size());
                if (m == 0)
                    continue;
                if (s.area && m > 1)
                    c.areaStrip(xs_.data(), m, ys_.data(), nullptr, a.bottom(), l.over(col, 0.22f));
                if (m > 1)
                    c.polyline(xs_.data(), ys_.data(), m, s.width, col);
                if (s.points || m == 1)
                    for (int k = 0; k < m; ++k)
                        c.disc(xs_[static_cast<std::size_t>(k)], ys_[static_cast<std::size_t>(k)], 2.5f, col);
            }
        }
    }

    void Plot::drawCursor(Canvas& c, const Look& l) const
    {
        if (!cursorShown())
            return;
        const Rect a = plotArea();
        const double x = dataX(pointer_.x);
        c.segment(pointer_.x, a.y, pointer_.x, a.bottom(), 1.0f, l.dim);
        std::string line = numberOf(x_, x, (vx1_ - vx0_) / 100.0) + (x_.label.empty() ? "" : " " + x_.label);
        for (std::size_t si = 0; si < series_.size(); ++si)
        {
            const double y = yAt(series_[si], x);
            if (!std::isfinite(y))
                continue;
            const float py = pxY(y);
            if (py >= a.y && py <= a.bottom())
                c.disc(pointer_.x, py, 3.0f, colorOf(l, si), 1.0f, l.ground);
            line += "   " + (series_[si].name.empty() ? std::string() : series_[si].name + " ")
                  + numberOf(y_, y, (vy1_ - vy0_) / 100.0);
        }
        c.text(line.c_str(), a.right(), bounds_.y + 3.0f, kWidgetSmall, l.value, funkgui::Align::right);
    }

    void Plot::draw(Canvas& c, const funkgui::Theme& th, bool focusRing) const
    {
        const Look l = Look::of(th);
        drawFrame(c, l);
        drawSeries(c, l);
        drawCursor(c, l);
        if (focusRing)
            ring(c, l, plotArea(), 0.0f);
    }

    funkgui::Cursor Plot::cursorAt(funkgui::Point p) const
    {
        if (panning_)
            return funkgui::Cursor::grabbing;
        return plotArea().contains(p) ? funkgui::Cursor::crosshair : funkgui::Cursor::normal;
    }

    void Plot::pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&)
    {
        if (e.popup || !plotArea().contains({ e.x, e.y }))
            return;
        panning_ = true;
        panFrom_ = { e.x, e.y };
        panX0_ = vx0_;
        panX1_ = vx1_;
        panY0_ = vy0_;
        panY1_ = vy1_;
    }

    void Plot::pointerDrag(const funkgui::PointerEvent& e, funkgui::GestureController&)
    {
        if (!panning_)
            return;
        const Rect a = plotArea();
        const double dtx = static_cast<double>((e.x - panFrom_.x) / a.w), dty = static_cast<double>((e.y - panFrom_.y) / a.h);
        setView(fromT(x_.log, panX0_, panX1_, -dtx), fromT(x_.log, panX0_, panX1_, 1.0 - dtx),
                fromT(y_.log, panY0_, panY1_, dty), fromT(y_.log, panY0_, panY1_, 1.0 + dty));
    }

    void Plot::pointerUp(const funkgui::PointerEvent&, funkgui::GestureController&) { panning_ = false; }

    void Plot::doubleClick(const funkgui::PointerEvent&, funkgui::GestureController&) { resetView(); }

    bool Plot::wheel(const funkgui::WheelEvent& e, funkgui::GestureController&, double)
    {
        if (e.dy == 0.0f || !plotArea().contains({ e.x, e.y }))
            return false;
        const double f = std::pow(1.25, static_cast<double>(e.dy) * (e.smooth ? 0.2 : 1.0));
        if (e.mods.shift)
            zoomY(f, dataY(e.y));
        else
            zoomX(f, dataX(e.x));
        return true;
    }

    bool Plot::magnify(const funkgui::MagnifyEvent& e)
    {
        if (!plotArea().contains({ e.x, e.y }))
            return false;
        zoomX(static_cast<double>(e.scale), dataX(e.x));
        return true;
    }

    bool Plot::key(const funkgui::KeyEvent& e, funkgui::GestureController&)
    {
        const double cx = fromT(x_.log, vx0_, vx1_, 0.5);
        if (e.key == funkgui::Key::character && (e.ch == '+' || e.ch == '='))
            zoomX(1.25, cx);
        else if (e.key == funkgui::Key::character && e.ch == '-')
            zoomX(0.8, cx);
        else if (e.key == funkgui::Key::left || e.key == funkgui::Key::right)
        {
            const double d = e.key == funkgui::Key::left ? -0.1 : 0.1;
            setView(fromT(x_.log, vx0_, vx1_, d), fromT(x_.log, vx0_, vx1_, 1.0 + d), vy0_, vy1_);
        }
        else if (e.key == funkgui::Key::home)
            resetView();
        else
            return false;
        return true;
    }

    void Plot::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem it;
        it.id = id_;
        it.role = funkgui::A11yRole::image;
        it.bounds = bounds_;
        it.title = title_;
        it.description = std::to_string(series_.size()) + " series; x " + numberOf(x_, vx0_, (vx1_ - vx0_) / 100.0) + " to "
                       + numberOf(x_, vx1_, (vx1_ - vx0_) / 100.0) + (x_.label.empty() ? "" : " " + x_.label);
        it.readOnly = true;
        out.push_back(std::move(it));
    }

    bool Plot::a11yAction(uint32_t, funkgui::A11yAction, double, funkgui::GestureController&) { return false; }

    uint64_t Plot::drawKey() const
    {
        return DrawKey{}.mix(revision_).mix(static_cast<float>(vx0_)).mix(static_cast<float>(vx1_)).h;
    }
}
