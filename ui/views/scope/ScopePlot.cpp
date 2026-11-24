// ui/views/scope/ScopePlot.cpp: see ScopePlot.h.
#include "views/scope/ScopePlot.h"

namespace tee::ui
{
    using funkgui::Canvas;
    using funkgui::Col;
    using funkgui::Rect;

    void dashedPolyline(Canvas& c, const float* xs, const float* ys, int n, float width, Col col, float on, float off)
    {
        float phase = 0.0f;   // distance into the current on + off period
        const float period = on + off;
        for (int i = 1; i < n; ++i)
        {
            float x0 = xs[i - 1], y0 = ys[i - 1];
            const float x1 = xs[i], y1 = ys[i];
            if (!std::isfinite(x0) || !std::isfinite(y0) || !std::isfinite(x1) || !std::isfinite(y1))
            {
                phase = 0.0f;
                continue;
            }
            float len = std::hypot(x1 - x0, y1 - y0);
            if (!(len > 0.0f))
                continue;
            const float ux = (x1 - x0) / len, uy = (y1 - y0) / len;
            while (len > 1e-4f)
            {
                const bool drawing = phase < on;
                const float left = drawing ? on - phase : period - phase;
                const float step = std::min(left, len);
                if (drawing)
                    c.segment(x0, y0, x0 + ux * step, y0 + uy * step, width, col);
                x0 += ux * step;
                y0 += uy * step;
                len -= step;
                phase += step;
                if (phase >= period - 1e-4f)
                    phase = 0.0f;
            }
        }
    }

    float legendEntry(Canvas& c, const look::Palette& p, float x, float y, std::string_view name, Col line, float width,
                      bool dashed)
    {
        const float cy = y + 5.0f;
        if (dashed)
        {
            const float xs[2] = { x, x + 24.0f }, ys[2] = { cy, cy };
            dashedPolyline(c, xs, ys, 2, width, line, 6.0f, 3.0f);
        }
        else
            c.segment(x, cy, x + 24.0f, cy, width, line);
        const std::string n(name);
        c.text(n.c_str(), x + 30.0f, y, look::kCaps, p.text);
        return 30.0f + c.textWidth(n.c_str(), look::kCaps) + 16.0f;
    }

    Col ScopePlot::inkOf(const look::Palette& p, Ink ink) noexcept
    {
        switch (ink)
        {
            case Ink::value: return p.value;
            case Ink::text: return p.text;
            case Ink::label: return p.label;
            case Ink::dim: return p.dim;
            case Ink::cold: return p.cold;
            case Ink::hot: return p.hot;
            case Ink::hotter: return p.hotter;
            case Ink::ok: return p.ok;
            case Ink::warning: return p.warning;
            case Ink::selected: return p.selected;
        }
        return p.value;
    }

    void ScopePlot::setTraces(std::vector<std::pair<std::string, Style>> traces)
    {
        series_.clear();
        styles_.clear();
        for (auto& [name, style] : traces)
        {
            PlotSeries s;
            s.name = std::move(name);
            series_.push_back(std::move(s));
            styles_.push_back(style);
        }
        touched();
    }

    void ScopePlot::setNote(std::string note)
    {
        if (note != note_)
        {
            note_ = std::move(note);
            touched();
        }
    }

    void ScopePlot::setCaption(std::string s)
    {
        if (s != caption_)
        {
            caption_ = std::move(s);
            touched();
        }
    }

    bool ScopePlot::hasData() const noexcept
    {
        for (const PlotSeries& s : series_)
            if (!s.x.empty())
                return true;
        return false;
    }

    void ScopePlot::setAxes(double xlo, double xhi, bool xlog, double ylo, double yhi, bool ylog)
    {
        if (x_.lo == xlo && x_.hi == xhi && x_.log == xlog && y_.lo == ylo && y_.hi == yhi && y_.log == ylog)
            return;
        x_.lo = xlo;
        x_.hi = xhi;
        x_.log = xlog;
        y_.lo = ylo;
        y_.hi = yhi;
        y_.log = ylog;
        resetView();
    }

    void ScopePlot::drawTraces(Canvas& c, const look::Palette& p) const
    {
        const Rect a = plotArea();
        const Canvas::ClipScope clip(c, { c.snapX(a.x), c.snapY(a.y), a.w, a.h });
        for (std::size_t si = 0; si < series_.size(); ++si)
        {
            const PlotSeries& s = series_[si];
            const Style st = si < styles_.size() ? styles_[si] : Style{};
            const Col col = inkOf(p, st.ink);
            const std::size_t n = std::min(s.x.size(), s.y.size());
            std::size_t i = 0;
            while (i < n)   // each unbroken run of points
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
                if (st.area && m > 1)
                    c.areaStrip(xs_.data(), m, ys_.data(), nullptr, a.bottom(), p.over(col, 0.16f));
                if (m > 1)
                {
                    if (st.dashed)
                        dashedPolyline(c, xs_.data(), ys_.data(), m, st.width, col);
                    else
                        c.polyline(xs_.data(), ys_.data(), m, st.width, col);
                }
                else
                    c.disc(xs_[0], ys_[0], 2.5f, col);
            }
        }
    }

    float ScopePlot::titleRight(Canvas& c) const
    {
        std::string head = upper(title_);
        if (!y_.label.empty())
            head += "  (" + upper(y_.label) + ")";
        return bounds_.x + c.textWidth(head.c_str(), kWidgetLabel);
    }

    void ScopePlot::drawReadout(Canvas& c, const look::Palette& p) const
    {
        if (!cursorShown())
        {
            // Only where it fits beside the title.
            std::string head = upper(title_);
            if (!y_.label.empty())
                head += "  (" + upper(y_.label) + ")";
            if (!caption_.empty()
                && c.textWidth(head.c_str(), kWidgetLabel) + c.textWidth(caption_.c_str(), kWidgetSmall) + 24.0f < bounds_.w)
                c.text(caption_.c_str(), bounds_.right(), bounds_.y + 3.0f, kWidgetSmall, p.text, funkgui::Align::right);
            return;
        }
        const Rect a = plotArea();
        const double x = dataX(pointer_.x);
        c.segment(pointer_.x, a.y, pointer_.x, a.bottom(), 1.0f, p.label);
        std::string line = numberOf(x_, x, (vx1_ - vx0_) / 100.0) + (x_.label.empty() ? "" : " " + x_.label);
        for (std::size_t si = 0; si < series_.size(); ++si)
        {
            const double y = yAt(series_[si], x);
            if (!std::isfinite(y))
                continue;
            const Style st = si < styles_.size() ? styles_[si] : Style{};
            const float py = pxY(y);
            if (py >= a.y && py <= a.bottom())
                c.disc(pointer_.x, py, 3.5f, inkOf(p, st.ink), 1.25f, p.ground);
            line += "   " + (series_[si].name.empty() ? std::string() : series_[si].name + " ")
                  + numberOf(y_, y, (vy1_ - vy0_) / 100.0);
        }
        c.text(line.c_str(), bounds_.right(), bounds_.y + 3.0f, kWidgetSmall, p.value, funkgui::Align::right);
    }

    void ScopePlot::draw(Canvas& c, const funkgui::Theme& th, bool focusRing) const
    {
        const Look l = Look::of(th);
        const look::Palette p = look::Palette::of(th);
        if (xTicks_.empty())
            drawFrame(c, l);
        else
            drawFrameTicked(c, l);
        drawTraces(c, p);
        const Rect a = plotArea();
        if (inside_)
        {
            const Canvas::ClipScope clip(c, { c.snapX(a.x), c.snapY(a.y), a.w, a.h });
            inside_(c, p, *this);
        }
        if (outside_)
            outside_(c, p, *this);
        if (!hasData() && !note_.empty())
        {
            // Wrapped to the area at spaces, centred.
            std::vector<std::string> lines;
            std::string line;
            std::size_t at = 0;
            while (at < note_.size())
            {
                std::size_t sp = note_.find(' ', at);
                if (sp == std::string::npos)
                    sp = note_.size();
                const std::string word = note_.substr(at, sp - at);
                const std::string tried = line.empty() ? word : line + " " + word;
                if (!line.empty() && c.textWidth(tried.c_str(), look::kBody) > a.w - 16.0f)
                {
                    lines.push_back(line);
                    line = word;
                }
                else
                    line = tried;
                at = sp + 1;
            }
            if (!line.empty())
                lines.push_back(line);
            float y = a.centreY() - 8.0f * static_cast<float>(lines.size());
            for (const std::string& ln : lines)
            {
                c.text(ln.c_str(), a.centreX(), y, look::kBody, p.label, funkgui::Align::centre);
                y += 16.0f;
            }
        }
        drawReadout(c, p);
        if (focusRing)
            ring(c, l, a, 0.0f);
    }

    // Plot::drawFrame with the x ticks given: the same area, y grid and numbers, title; x lines and words at the ticks.
    void ScopePlot::drawFrameTicked(Canvas& c, const Look& l) const
    {
        const Rect a = plotArea();
        c.rrect(a.x, a.y, a.w, a.h, 0.0f, l.over(l.edge, 0.3f));
        // Whole words while they fit between the lines; else the number alone (the words before the first space).
        float need = 0.0f;
        for (const auto& t : xTicks_)
            need += c.textWidth(t.second.c_str(), kWidgetSmall) + 28.0f;
        const bool whole = need <= a.w;
        for (const auto& [v, all] : xTicks_)
        {
            if (v < vx0_ || v > vx1_)
                continue;
            const std::string words = whole ? all : all.substr(0, all.find(' '));
            const float x = c.snapX(pxX(v));
            c.hairlineV(x, a.y, a.h, l.over(l.edge, 0.9f));
            const float tw = c.textWidth(words.c_str(), kWidgetSmall);
            const float tx = std::clamp(x, a.x + tw * 0.5f - 6.0f, a.right() - tw * 0.5f + 6.0f);
            c.text(words.c_str(), tx, a.bottom() + 4.0f, kWidgetSmall, l.label, funkgui::Align::centre);
        }
        if (!x_.label.empty())
            c.text(upper(x_.label).c_str(), a.right(), a.bottom() + 16.0f, kWidgetLabel, l.dim, funkgui::Align::right);
        std::vector<double> tv;
        std::vector<bool> major;
        ticks(y_, vy0_, vy1_, a.h, tv, major);
        const double step = tv.size() > 1 ? tv[1] - tv[0] : 0.0;
        for (std::size_t i = 0; i < tv.size(); ++i)
        {
            const float y = c.snapY(pxY(tv[i]));
            c.hairlineH(a.x, y, a.w, major[i] ? l.over(l.edge, 0.9f) : l.over(l.edge, 0.5f));
            if (major[i])
                c.text(numberOf(y_, tv[i], y_.log ? tv[i] : step).c_str(), a.x - 5.0f, c.capCentreTop(y, kWidgetSmall),
                       kWidgetSmall, l.label, funkgui::Align::right);
        }
        c.rrect(a.x, a.y, a.w, a.h, 0.0f, kClear, 1.0f, l.edge);
        std::string head = upper(title_);
        if (!y_.label.empty())
            head += head.empty() ? upper(y_.label) : "  (" + upper(y_.label) + ")";
        c.text(head.c_str(), bounds_.x, bounds_.y + 3.0f, kWidgetLabel, l.label);
    }

    uint64_t ScopePlot::drawKey() const
    {
        DrawKey k;
        k.word(Plot::drawKey()).mix(static_cast<float>(vy0_)).mix(static_cast<float>(vy1_)).mix(note_).mix(caption_);
        return k.h;
    }
}
