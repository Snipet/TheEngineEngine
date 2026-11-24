// ui/kit/widgets/Gauge.cpp: see Gauge.h.
#include "kit/widgets/Gauge.h"

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
        // Ticks and numerals.
        const int n = major_ * minor_;
        const funkgui::TextStyle numeral{ std::clamp(side * 0.075f, 10.0f, 16.0f), 0.0f, 0.0f, false };
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
                const std::string s = numerals_ ? numerals_(v) : plain(v);
                const float rn = ra - len - numeral.px * 0.9f;
                c.text(s.c_str(), o.x + ca * rn, c.capCentreTop(o.y + sa * rn, numeral), numeral, red ? l.accent : l.text,
                       funkgui::Align::centre);
            }
        }
        // The readout, then the needle over it, then the hub.
        const funkgui::TextStyle value{ std::clamp(side * 0.13f, 12.0f, 26.0f), 0.0f, 0.0f, true };
        c.text(readoutText().c_str(), o.x, c.capCentreTop(o.y + r * 0.42f, value), value, l.value, funkgui::Align::centre);
        if (!unit_.empty())
            c.text(upper(unit_).c_str(), o.x, c.capCentreTop(o.y + r * 0.42f + value.px * 0.75f + 4.0f, kWidgetLabel),
                   kWidgetLabel, l.label, funkgui::Align::centre);
        const float a = angleOf(needle_);
        c.segment(o.x - std::cos(a) * r * 0.12f, o.y - std::sin(a) * r * 0.12f, o.x + std::cos(a) * (ra - 3.0f),
                  o.y + std::sin(a) * (ra - 3.0f), 2.5f, l.accent);
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
