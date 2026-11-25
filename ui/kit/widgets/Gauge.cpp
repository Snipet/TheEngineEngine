// ui/kit/widgets/Gauge.cpp: see Gauge.h.
#include "kit/widgets/Gauge.h"

#include <funkgui/text/FontService.h>
#include <funkgui/text/TextFit.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>

namespace tee::ui
{
    namespace
    {
        std::string plain(double v)
        {
            char buf[32];
            std::snprintf(buf, sizeof buf, "%.0f", v);
            return std::string(buf) == "-0" ? "0" : buf;
        }
    }

    Gauge::Gauge(uint32_t id, std::string title, std::function<double()> value, double lo, double hi)
        : Widget(id), source_(std::move(value)), lo_(lo), hi_(hi), redline_(std::numeric_limits<double>::quiet_NaN())
    {
        title_ = std::move(title);
    }

    float Gauge::fractionOf(double v) const noexcept
    {
        if (!(hi_ > lo_) || !std::isfinite(v))
            return 0.0f;
        return static_cast<float>(std::clamp((v - lo_) / (hi_ - lo_), 0.0, 1.0));
    }

    void Gauge::step(float dt)
    {
        value_ = source_ ? source_() : 0.0;
        needle_ = funkgui::ease::toward(needle_, fractionOf(value_), dt, 0.06f, 1e-4f);
    }

    std::string Gauge::readoutText() const { return readout_ ? readout_(value_) : plain(value_); }

    namespace
    {
        funkgui::TextStyle valueStyle(float side) { return { std::clamp(side * 0.13f, 12.0f, 26.0f), 0.0f, 0.0f, true }; }
        float widthOf(const std::string& t, const funkgui::TextStyle& s)
        {
            return funkgui::text::width(funkgui::FontService::get().atlas(), t.c_str(), s);
        }
    }

    Gauge::Geometry Gauge::geometry() const
    {
        Geometry g;
        const float titleH = title_.empty() ? 0.0f : 16.0f;
        g.side = std::min(bounds_.w, bounds_.h - titleH);
        g.r = g.side * 0.5f - 3.0f;
        g.ra = g.r - 7.0f;
        g.hub = { bounds_.centreX(), bounds_.y + g.side * 0.5f + 2.0f };
        return g;
    }

    funkgui::Rect Gauge::readoutBox(const std::string& text) const
    {
        const Geometry g = geometry();
        const funkgui::TextStyle value = valueStyle(g.side);
        const std::string unit = upper(unit_);
        const float wd = widthOf(text, value), wu = unit.empty() ? 0.0f : widthOf(unit, kWidgetLabel);
        if (readoutClear_)
        {
            // Below the hub by half the widest line and a margin (the needle's half-width, a gap): inside the wedge;
            // and below the end numerals.
            const float w = std::max(wd, wu);
            float top = std::max(g.r * 0.30f, w * 0.5f + 4.0f);
            for (const funkgui::Rect& n : endNumerals())
                if (n.x < g.hub.x + w * 0.5f + 2.0f && g.hub.x - w * 0.5f - 2.0f < n.right())
                    top = std::max(top, n.bottom() + 2.0f - g.hub.y);
            const float h = value.px + (unit.empty() ? 0.0f : 1.0f + kWidgetLabel.px);
            return { g.hub.x - w * 0.5f, g.hub.y + top, w, h };
        }
        const float top = g.hub.y + g.r * 0.42f - value.px * 0.5f;
        const float h = value.px * 1.25f + 4.0f + (unit.empty() ? 0.0f : kWidgetLabel.px);
        const float w = std::max(wd, wu);
        return { g.hub.x - w * 0.5f, top, w, h };
    }

    std::vector<funkgui::Rect> Gauge::endNumerals() const
    {
        // As draw() places them (its numeral's size and radius).
        std::vector<funkgui::Rect> out;
        const Geometry g = geometry();
        if (g.side < 90.0f)
            return out;
        const funkgui::TextStyle numeral{ std::clamp(g.side * 0.075f, 10.0f, 16.0f), 0.0f, 0.0f, false };
        for (const double v : { lo_, hi_ })
        {
            const float a = v == lo_ ? kA0 : kA1, ca = std::cos(a), sa = std::sin(a);
            const std::string t = numerals_ ? numerals_(v) : plain(v);
            const float w = widthOf(t, numeral), hw = w * 0.5f, hh = numeral.px * 0.5f;
            const float rn = clearNumerals_ ? g.ra - 9.0f - 6.0f - (hw * std::fabs(ca) + hh * std::fabs(sa))
                                            : g.ra - 9.0f - numeral.px * 0.9f;
            const float x = g.hub.x + ca * rn, y = g.hub.y + sa * rn;
            out.push_back({ x - hw - 3.0f, y - hh - 2.0f, w + 6.0f, numeral.px + 4.0f });
        }
        return out;
    }

    bool Gauge::readoutClearOfSweep() const
    {
        const Geometry g = geometry();
        const std::vector<funkgui::Rect> ends = endNumerals();
        for (const double v : { lo_, hi_, value_ })
        {
            const funkgui::Rect box = readoutBox(readout_ ? readout_(v) : plain(v)).expanded(1.25f);   // the needle's half-width
            for (const funkgui::Rect& n : ends)
                if (n.x < box.right() && box.x < n.right() && n.y < box.bottom() && box.y < n.bottom())
                    return false;
            for (const float a : { kA0, kA1 })
                for (float d = 0.0f; d <= g.ra + 1.0f; d += 0.5f)
                    if (box.contains({ g.hub.x + std::cos(a) * d, g.hub.y + std::sin(a) * d }))
                        return false;
        }
        return true;
    }

    void Gauge::draw(funkgui::Canvas& c, const funkgui::Theme& th, bool) const
    {
        const Look l = Look::of(th);
        const float titleH = title_.empty() ? 0.0f : 16.0f;
        const float side = std::min(bounds_.w, bounds_.h - titleH);
        if (side < 40.0f)
            return;
        const float r = side * 0.5f - 3.0f;
        const funkgui::Point o{ bounds_.centreX(), bounds_.y + side * 0.5f + 2.0f };
        // The face, the track, the red band.
        c.disc(o.x, o.y, r, l.over(l.edge, 0.35f), 1.0f, l.edge);
        const float ra = r - 7.0f;
        c.arc(o, ra, kA0, kA1, 2.0f, l.dim);
        if (std::isfinite(redline_) && redline_ < hi_)
            c.arc(o, ra, angleOf(fractionOf(redline_)), kA1, 5.0f, l.over(l.accent, 0.85f));
        // Ticks and numerals. Clear numerals (U1, setClearNumerals): set further in, by their own size, and drawn after
        // the needle on a backing of the face, so the needle passes under them; the needle's tail short.
        const int n = major_ * minor_;
        const funkgui::TextStyle numeral{ std::clamp(side * 0.075f, 10.0f, 16.0f), 0.0f, 0.0f, false };
        const funkgui::Col face = l.over(l.edge, 0.35f);
        struct Numeral
        {
            std::string text;
            float x, y;
            bool red;
        };
        std::vector<Numeral> numerals;
        for (int k = 0; k <= n; ++k)
        {
            const float t = static_cast<float>(k) / static_cast<float>(n);
            const float a = angleOf(t), ca = std::cos(a), sa = std::sin(a);
            const bool major = k % minor_ == 0;
            const float len = major ? 9.0f : 5.0f;
            const double v = lo_ + (hi_ - lo_) * static_cast<double>(t);
            const bool red = std::isfinite(redline_) && v >= redline_ - 1e-9;
            c.segment(o.x + ca * (ra - len), o.y + sa * (ra - len), o.x + ca * (ra + 1.0f), o.y + sa * (ra + 1.0f),
                      major ? 2.0f : 1.0f, red ? l.accent : (major ? l.text : l.label));
            if (major && side >= 90.0f)
            {
                std::string s = numerals_ ? numerals_(v) : plain(v);
                float rn = ra - len - numeral.px * 0.9f;
                if (clearNumerals_)
                {
                    // Inside the ticks by the numeral's own half-extent along the radius, and a gap.
                    const float hw = c.textWidth(s.c_str(), numeral) * 0.5f, hh = numeral.px * 0.5f;
                    rn = ra - len - 6.0f - (hw * std::fabs(ca) + hh * std::fabs(sa));
                }
                numerals.push_back({ std::move(s), o.x + ca * rn, o.y + sa * rn, red });
            }
        }
        const auto drawNumerals = [&] {
            for (const Numeral& m : numerals)
            {
                if (clearNumerals_)
                {
                    const float w = c.textWidth(m.text.c_str(), numeral);
                    c.rrect(m.x - w * 0.5f - 3.0f, m.y - numeral.px * 0.5f - 2.0f, w + 6.0f, numeral.px + 4.0f, 3.0f, face);
                }
                c.text(m.text.c_str(), m.x, c.capCentreTop(m.y, numeral), numeral, m.red ? l.accent : l.text, funkgui::Align::centre);
            }
        };
        if (!clearNumerals_)
            drawNumerals();
        // The readout, then the needle over it, then the hub.
        const funkgui::TextStyle value = valueStyle(side);
        if (readoutClear_)
        {
            const std::string text = readoutText();
            const funkgui::Rect box = readoutBox(text);
            c.text(text.c_str(), o.x, box.y, value, l.value, funkgui::Align::centre);
            if (!unit_.empty())
                c.text(upper(unit_).c_str(), o.x, box.y + value.px + 1.0f, kWidgetLabel, l.label, funkgui::Align::centre);
        }
        else
            c.text(readoutText().c_str(), o.x, c.capCentreTop(o.y + r * 0.42f, value), value, l.value, funkgui::Align::centre);
        if (!unit_.empty() && !readoutClear_)
            c.text(upper(unit_).c_str(), o.x, c.capCentreTop(o.y + r * 0.42f + value.px * 0.75f + 4.0f, kWidgetLabel),
                   kWidgetLabel, l.label, funkgui::Align::centre);
        const float a = angleOf(needle_), tail = clearNumerals_ ? 0.05f : 0.12f;
        c.segment(o.x - std::cos(a) * r * tail, o.y - std::sin(a) * r * tail, o.x + std::cos(a) * (ra - 3.0f),
                  o.y + std::sin(a) * (ra - 3.0f), 2.5f, l.accent);
        if (clearNumerals_)
            drawNumerals();
        c.disc(o.x, o.y, std::max(4.0f, r * 0.07f), l.faceHi, 1.0f, l.text);
        if (!title_.empty())
            c.text(upper(title_).c_str(), bounds_.centreX(), bounds_.bottom() - 13.0f, kWidgetLabel, l.label,
                   funkgui::Align::centre);
    }

    void Gauge::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem it;
        it.id = id_;
        it.role = funkgui::A11yRole::progressBar;
        it.bounds = bounds_;
        it.title = title_;
        it.value = readoutText() + (unit_.empty() ? "" : " " + unit_);
        it.v = value_;
        it.lo = lo_;
        it.hi = hi_;
        it.readOnly = true;
        out.push_back(std::move(it));
    }

    uint64_t Gauge::drawKey() const { return DrawKey{}.mix(static_cast<float>(source_ ? source_() : 0.0)).h; }
}
