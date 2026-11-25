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
        funkgui::TextStyle numeralStyle(float side) { return { std::clamp(side * 0.075f, 10.0f, 16.0f), 0.0f, 0.0f, false }; }
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
        numeralsOf(g);   // the tick ring's radius
        return g;
    }

    std::vector<Gauge::Label> Gauge::numeralsOf(Geometry& g) const
    {
        std::vector<Label> out;
        g.ra = g.r - 7.0f;
        if (g.side < 90.0f)
            return out;
        const funkgui::TextStyle numeral = numeralStyle(g.side);
        for (int m = 0; m <= major_; ++m)
        {
            if (m % numeralStride_ != 0 && m != major_)
                continue;
            const float t = static_cast<float>(m) / static_cast<float>(major_);
            const float a = angleOf(t), ca = std::cos(a), sa = std::sin(a);
            const double v = lo_ + (hi_ - lo_) * static_cast<double>(t);
            Label l;
            l.text = numerals_ ? numerals_(v) : plain(v);
            l.red = std::isfinite(redline_) && v >= redline_ - 1e-9;
            const float w = widthOf(l.text, numeral);
            float rn = 0.0f;
            if (clearNumerals_)
            {
                l.hw = w * 0.5f + 3.0f;   // the backing's
                l.hh = numeral.px * 0.5f + 2.0f;
                rn = g.r - 3.0f - (l.hw * std::fabs(ca) + l.hh * std::fabs(sa));   // just inside the rim
            }
            else
            {
                l.hw = w * 0.5f;
                l.hh = numeral.px * 0.5f;
                rn = g.ra - 9.0f - numeral.px * 0.9f;
            }
            l.x = g.hub.x + ca * rn;
            l.y = g.hub.y + sa * rn;
            out.push_back(std::move(l));
        }
        if (clearNumerals_)
        {
            // The ticks (out to ra + 1) and the needle (to ra - 3) end inside the nearest numeral's box, a gap short.
            const auto ringInside = [&g](const std::vector<Label>& ls) {
                float nearest = g.r;
                for (const Label& l : ls)
                {
                    const float dx = std::max(0.0f, std::fabs(l.x - g.hub.x) - l.hw), dy = std::max(0.0f, std::fabs(l.y - g.hub.y) - l.hh);
                    nearest = std::min(nearest, std::sqrt(dx * dx + dy * dy));
                }
                return nearest - 3.0f;
            };
            g.ra = ringInside(out);
            if (g.ra < 0.55f * g.r && out.size() > 2)
            {
                // A small face: the ends' numbers (on the diagonals, the widest along the radius) go; the ones between
                // stay, near the top, where they cost little of the needle's length.
                out.erase(out.begin());
                out.pop_back();
                g.ra = ringInside(out);
            }
            if (g.ra < 0.55f * g.r)
            {
                // No room for a sweep worth the name inside them: ticks and digits only.
                out.clear();
                g.ra = g.r - 7.0f;
            }
        }
        return out;
    }

    std::vector<funkgui::Rect> Gauge::numeralBoxes() const
    {
        Geometry g = geometry();
        std::vector<funkgui::Rect> out;
        for (const Label& l : numeralsOf(g))
            out.push_back({ l.x - l.hw, l.y - l.hh, 2.0f * l.hw, 2.0f * l.hh });
        return out;
    }

    bool Gauge::labelsClearOfNeedle(double t, std::string& why) const
    {
        Geometry g = geometry();
        std::vector<std::pair<funkgui::Rect, std::string>> boxes;
        for (const Label& l : numeralsOf(g))
            boxes.push_back({ { l.x - l.hw, l.y - l.hh, 2.0f * l.hw, 2.0f * l.hh }, "the number " + l.text });
        const double v = lo_ + (hi_ - lo_) * t;
        const std::string text = readout_ ? readout_(v) : plain(v);
        boxes.push_back({ readoutBox(text), "the readout " + text + (unit_.empty() ? "" : " " + unit_) });
        const float a = angleOf(static_cast<float>(t)), tail = g.r * (clearNumerals_ ? 0.05f : 0.12f);
        for (const auto& [box, what] : boxes)
        {
            const funkgui::Rect b = box.expanded(1.25f);   // the needle's half-width
            for (float d = -tail; d <= g.ra - 3.0f; d += 0.5f)
                if (b.contains({ g.hub.x + std::cos(a) * d, g.hub.y + std::sin(a) * d }))
                {
                    why = "the needle meets " + what;
                    return false;
                }
        }
        return true;
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
        std::vector<funkgui::Rect> all = numeralBoxes(), out;
        if (!all.empty())
        {
            out.push_back(all.front());
            if (all.size() > 1)
                out.push_back(all.back());
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
        Geometry g = geometry();
        const std::vector<Label> labels = numeralsOf(g);
        const float ra = g.ra;
        c.arc(o, ra, kA0, kA1, 2.0f, l.dim);
        if (std::isfinite(redline_) && redline_ < hi_)
            c.arc(o, ra, angleOf(fractionOf(redline_)), kA1, 5.0f, l.over(l.accent, 0.85f));
        // Ticks, then the numerals (numeralsOf: clear numerals out of the needle's reach, on a backing of the face).
        const int n = major_ * minor_;
        const funkgui::TextStyle numeral = numeralStyle(side);
        const funkgui::Col face = l.over(l.edge, 0.35f);
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
        }
        const auto drawNumerals = [&] {
            for (const Label& m : labels)
            {
                if (clearNumerals_)
                    c.rrect(m.x - m.hw, m.y - m.hh, 2.0f * m.hw, 2.0f * m.hh, 3.0f, face);
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
