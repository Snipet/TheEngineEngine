// ui/app/Cockpit.cpp: see Cockpit.h.
#include "app/Cockpit.h"

#include <funkgui/core/Ease.h>

#include <algorithm>
#include <cmath>

namespace tee::ui
{
    namespace
    {
        using look::Palette;
        constexpr double kPi = 3.14159265358979323846;

        funkgui::A11yItem a11yOf(uint32_t id, funkgui::A11yRole role, const funkgui::Rect& r, std::string title,
                                 std::string value)
        {
            funkgui::A11yItem it;
            it.id = id;
            it.role = role;
            it.bounds = r;
            it.title = std::move(title);
            it.value = std::move(value);
            it.readOnly = true;
            return it;
        }

        double finiteOr(double v, double fallback) { return std::isfinite(v) ? v : fallback; }
    }

    // ---- RpmBar -----------------------------------------------------------------------------------------------------
    RpmBar::RpmBar(uint32_t id, std::function<double()> rpm, std::function<double()> redline, std::function<bool()> limiter)
        : Widget(id), rpm_(std::move(rpm)), redline_(std::move(redline)), limiter_(std::move(limiter))
    {
        title_ = "Engine speed";
    }

    double RpmBar::scaleTop() const noexcept { return std::ceil((red_ + 1000.0) / 1000.0) * 1000.0; }

    int RpmBar::lightsLit() const noexcept
    {
        if (!std::isfinite(target_))
            return 0;
        static constexpr double kBefore[5] = { 1500.0, 1100.0, 750.0, 450.0, 200.0 };
        int n = 0;
        for (double b : kBefore)
            n += target_ >= red_ - b ? 1 : 0;
        return n;
    }

    void RpmBar::step(float dt)
    {
        target_ = rpm_ ? rpm_() : std::nan("");
        red_ = finiteOr(redline_ ? redline_() : std::nan(""), 7000.0);
        limiting_ = limiter_ && limiter_();
        // A little inertia, as a needle's (Look: motion eased, never stepped).
        shown_ = funkgui::ease::toward(shown_, static_cast<float>(finiteOr(target_, 0.0)), dt, 0.05f, 0.5f);
    }

    bool RpmBar::still() const { return shown_ == static_cast<float>(finiteOr(target_, 0.0)); }

    uint64_t RpmBar::drawKey() const
    {
        return DrawKey{}.mix(static_cast<float>(finiteOr(rpm_ ? rpm_() : 0.0, -1.0))).mix(limiter_ && limiter_() ? 1 : 0).h;
    }

    void RpmBar::draw(funkgui::Canvas& c, const funkgui::Theme& th, bool) const
    {
        const Palette p = Palette::of(th);
        const funkgui::Rect b = bounds_;
        const float digitsW = 92.0f, lightsH = 14.0f;
        const funkgui::Rect bar{ b.x, b.y + lightsH + 6.0f, std::max(40.0f, b.w - digitsW - 10.0f), 14.0f };
        const double top = scaleTop();
        const auto xOf = [&](double rpm) { return bar.x + bar.w * static_cast<float>(std::clamp(rpm / top, 0.0, 1.0)); };
        // The track, the redline zone (red, hatched), the fill.
        c.rrect(bar.x, bar.y, bar.w, bar.h, 3.0f, p.over(p.faint, 0.9f));
        const float rx = xOf(red_);
        c.rrect(rx, bar.y, bar.right() - rx, bar.h, 0.0f, p.over(p.hotter, 0.35f));
        for (float hx = rx + 3.0f; hx < bar.right() - 2.0f; hx += 6.0f)
            c.segment(hx, bar.bottom() - 2.0f, std::min(hx + 4.0f, bar.right() - 1.0f), bar.y + 2.0f, 1.0f, p.over(p.hotter, 0.7f));
        const float fx = xOf(shown_);
        if (fx > bar.x + 0.5f)
            c.rrect(bar.x, bar.y + 3.0f, fx - bar.x, bar.h - 6.0f, 2.0f, shown_ >= red_ ? p.hotter : p.value);
        c.segment(fx, bar.y - 3.0f, fx, bar.bottom() + 3.0f, look::kNeedle, shown_ >= red_ ? p.hotter : p.selected);
        // Numerals every 1000 (the thousands), ticks every 500.
        for (int k = 0; k * 500.0 <= top + 1e-9; ++k)
        {
            const double rpm = k * 500.0;
            const float x = xOf(rpm);
            const bool major = k % 2 == 0;
            const funkgui::Col ink = rpm >= red_ ? p.hotter : (major ? p.text : p.dim);
            c.segment(x, bar.bottom() + 1.0f, x, bar.bottom() + (major ? 6.0f : 3.5f), 1.0f, ink);
            if (major)
            {
                const std::string n = std::to_string(static_cast<int>(rpm / 1000.0));
                c.text(n.c_str(), x, bar.bottom() + 7.0f, look::kUnit, ink, funkgui::Align::centre);
            }
        }
        c.text("X1000 RPM", bar.x, b.y, look::kCaps, p.label);
        // The shift lights, right-aligned over the bar's end: green, green, amber, amber, red.
        const int lit = lightsLit();
        const funkgui::Col inks[5] = { p.ok, p.ok, p.warning, p.warning, p.hotter };
        for (int i = 0; i < 5; ++i)
        {
            const float cx = bar.right() - 8.0f - static_cast<float>(4 - i) * 16.0f, cy = b.y + 6.0f;
            if (i < lit)
                c.disc(cx, cy, 5.0f, inks[i]);
            else
                c.disc(cx, cy, 5.0f, look::kClear, 1.25f, p.dim);
        }
        const float wordX = bar.right() - 8.0f - 4.0f * 16.0f - 14.0f;
        if (limiting_)
            c.text("LIMITER", wordX, b.y, look::kCaps, p.warning, funkgui::Align::right);
        else if (lit == 5)
            c.text("SHIFT", wordX, b.y, look::kCaps, p.hotter, funkgui::Align::right);
        // The digits.
        const std::string digits = look::num(target_, "%.0f");
        c.text(digits.c_str(), b.right(), b.y + 6.0f, look::kNumberL, std::isfinite(target_) ? p.value : p.dim, funkgui::Align::right);
        c.text("RPM", b.right(), b.y + 34.0f, look::kUnit, p.label, funkgui::Align::right);
    }

    void RpmBar::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem it = a11yOf(id_, funkgui::A11yRole::progressBar, bounds_, title_, look::num(target_) + " rpm");
        it.v = finiteOr(target_, 0.0);
        it.hi = scaleTop();
        out.push_back(std::move(it));
    }

    // ---- Readout ----------------------------------------------------------------------------------------------------
    Readout::Readout(uint32_t id, std::string label, std::function<std::string()> number, std::function<std::string()> unit,
                     funkgui::TextStyle style)
        : Widget(id), numberOf_(std::move(number)), unitOf_(std::move(unit)), style_(style)
    {
        title_ = std::move(label);
    }

    void Readout::step(float)
    {
        number_ = numberOf_ ? numberOf_() : std::string("-");
        unit_ = unitOf_ ? unitOf_() : std::string();
    }

    uint64_t Readout::drawKey() const
    {
        return DrawKey{}.mix(numberOf_ ? numberOf_() : std::string()).mix(unitOf_ ? unitOf_() : std::string()).h;
    }

    void Readout::draw(funkgui::Canvas& c, const funkgui::Theme& th, bool) const
    {
        const Palette p = Palette::of(th);
        const funkgui::Col ink = ink_ ? ink_(p) : look::kClear;
        look::readout(c, p, bounds_.x, bounds_.y, upper(title_), number_, unit_, style_, ink);
    }

    void Readout::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        out.push_back(a11yOf(id_, funkgui::A11yRole::staticText, bounds_, title_, number_ + (unit_.empty() ? "" : " " + unit_)));
    }

    // ---- PedalBar ---------------------------------------------------------------------------------------------------
    PedalBar::PedalBar(uint32_t id, std::string name, std::function<float()> sent, std::function<void(bool, float)> onHold,
                       funkgui::Col (*ink)(const look::Palette&))
        : Widget(id), sent_(std::move(sent)), onHold_(std::move(onHold)), ink_(ink)
    {
        title_ = std::move(name);
    }

    funkgui::Rect PedalBar::track() const noexcept
    {
        return { bounds_.centreX() - 7.0f, bounds_.y + 14.0f, 14.0f, std::max(8.0f, bounds_.h - 30.0f) };
    }

    float PedalBar::at(float y) const noexcept
    {
        const funkgui::Rect t = track();
        return std::clamp(1.0f - (y - t.y) / t.h, 0.0f, 1.0f);
    }

    void PedalBar::step(float dt)
    {
        target_ = held_ ? hold_ : std::clamp(sent_ ? sent_() : 0.0f, 0.0f, 1.0f);
        shown_ = funkgui::ease::toward(shown_, target_, dt, 0.04f, 1e-3f);
    }

    void PedalBar::pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&)
    {
        held_ = true;
        hold_ = at(e.y);
        if (onHold_)
            onHold_(true, hold_);
    }

    void PedalBar::pointerDrag(const funkgui::PointerEvent& e, funkgui::GestureController&)
    {
        if (!held_)
            return;
        hold_ = at(e.y);
        if (onHold_)
            onHold_(true, hold_);
    }

    void PedalBar::pointerUp(const funkgui::PointerEvent&, funkgui::GestureController&)
    {
        if (!held_)
            return;
        held_ = false;
        if (onHold_)
            onHold_(false, 0.0f);
    }

    uint64_t PedalBar::drawKey() const { return DrawKey{}.mix(sent_ ? sent_() : 0.0f).mix(held_ ? hold_ : -1.0f).h; }

    void PedalBar::draw(funkgui::Canvas& c, const funkgui::Theme& th, bool) const
    {
        const Palette p = Palette::of(th);
        const funkgui::Rect t = track();
        const std::string pct = std::to_string(static_cast<int>(std::lround(shown_ * 100.0f)));
        c.text(pct.c_str(), t.centreX(), bounds_.y, look::kUnit, shown_ > 0.005f ? p.value : p.dim, funkgui::Align::centre);
        c.rrect(t.x, t.y, t.w, t.h, 3.0f, p.over(p.faint, 0.9f), held_ || hover_ > 0.5f ? 1.25f : 0.0f, p.selected);
        const float h = t.h * shown_;
        if (h > 0.5f)
            c.rrect(t.x + 2.0f, t.bottom() - h, t.w - 4.0f, h, 2.0f, ink_ != nullptr ? ink_(p) : p.value);
        c.text(upper(title_).c_str(), t.centreX(), t.bottom() + 4.0f, look::kCaps, held_ ? p.selected : p.label,
               funkgui::Align::centre);
    }

    void PedalBar::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem it = a11yOf(id_, funkgui::A11yRole::progressBar, bounds_, title_,
                                      std::to_string(static_cast<int>(std::lround(target_ * 100.0f))) + " %");
        it.v = target_;
        out.push_back(std::move(it));
    }

    // ---- DynoRibbon -------------------------------------------------------------------------------------------------
    DynoRibbon::DynoRibbon(uint32_t id, const DynoSource& dyno, const Units& units) : Widget(id), dyno_(dyno), units_(units)
    {
        title_ = "Dyno";
    }

    uint64_t DynoRibbon::drawKey() const
    {
        return DrawKey{}.mix(dyno_.revision()).mix(static_cast<float>(dyno_.progress())).mix(units_.revision()).h;
    }

    std::string DynoRibbon::peaksText() const
    {
        const DynoTrace& d = dyno_.predicted();
        if (d.empty())
            return dyno_.error().empty() ? "no curve yet" : dyno_.error();
        const DynoTrace::Peak t = d.peakTorque(), w = d.peakPower();
        const auto rpm = [](double s) { return look::num(s * 30.0 / kPi, "%.0f"); };
        return units_.format(Quantity::torque, t.value, 0) + " @ " + rpm(t.speed) + "   "
             + units_.format(Quantity::power, w.value, 0) + " @ " + rpm(w.speed);
    }

    void DynoRibbon::draw(funkgui::Canvas& c, const funkgui::Theme& th, bool) const
    {
        const Palette p = Palette::of(th);
        const funkgui::Rect b = bounds_;
        const DynoTrace& now = dyno_.predicted();
        const DynoTrace& was = dyno_.before();
        // The peaks, over the plot: torque in the value's ink, power in the text's, each with its word.
        const DynoTrace::Peak tq = now.peakTorque(), pw = now.peakPower();
        const auto rpm = [](double s) { return look::num(s * 30.0 / kPi, "%.0f"); };
        look::readout(c, p, b.x, b.y, "TORQUE", units_.number(Quantity::torque, tq.value, 0), units_.unit(Quantity::torque).name,
                      look::kNumberM);
        c.text(("@ " + rpm(tq.speed)).c_str(), b.x, b.y + 37.0f, look::kUnit, p.label);
        look::readout(c, p, b.x + b.w * 0.5f, b.y, "POWER", units_.number(Quantity::power, pw.value, 0), units_.unit(Quantity::power).name,
                      look::kNumberM, p.text);
        c.text(("@ " + rpm(pw.speed)).c_str(), b.x + b.w * 0.5f, b.y + 37.0f, look::kUnit, p.label);
        if (dyno_.busy())
        {
            const funkgui::Rect pr{ b.right() - 60.0f, b.y + 2.0f, 60.0f, 4.0f };
            look::level(c, p, pr, dyno_.progress(), p.selected);
            c.text("RUNNING", b.right(), b.y + 9.0f, look::kCaps, p.label, funkgui::Align::right);
        }
        // The plot: rpm across, torque on the left scale, power on the right.
        const funkgui::Rect a{ b.x + 30.0f, b.y + 56.0f, std::max(20.0f, b.w - 60.0f), std::max(20.0f, b.h - 74.0f) };
        if (now.empty() && was.empty())
        {
            c.text(fitText(c, peaksText(), look::kBody, a.w + 40.0f).c_str(), a.centreX(), a.centreY() - 6.0f, look::kBody, p.dim,
                   funkgui::Align::centre);
            return;
        }
        double x0 = 1e9, x1 = -1e9, t1 = 1.0, w1 = 1.0;
        for (const DynoTrace* d : { &now, &was })
            for (std::size_t i = 0; i < d->speed.size(); ++i)
            {
                x0 = std::min(x0, d->speed[i]);
                x1 = std::max(x1, d->speed[i]);
                t1 = std::max(t1, d->torque[i]);
                w1 = std::max(w1, d->power[i]);
            }
        t1 *= 1.1;
        w1 *= 1.1;
        if (!(x1 > x0))
            x1 = x0 + 1.0;
        c.rrect(a.x, a.y, a.w, a.h, 0.0f, look::kClear, 1.0f, p.grid);
        // Grid every 1000 rpm, with the thousands under it.
        for (double r = std::ceil(x0 * 30.0 / kPi / 1000.0) * 1000.0; r <= x1 * 30.0 / kPi + 1e-6; r += 1000.0)
        {
            const float x = a.x + a.w * static_cast<float>((r * kPi / 30.0 - x0) / (x1 - x0));
            c.hairlineV(c.snapX(x), a.y, a.h, p.grid);
            c.text(std::to_string(static_cast<int>(r / 1000.0)).c_str(), x, a.bottom() + 3.0f, look::kUnit, p.label, funkgui::Align::centre);
        }
        c.text("NM", a.x - 4.0f, a.y - 1.0f, look::kUnit, p.label, funkgui::Align::right);
        c.text("KW", a.right() + 4.0f, a.y - 1.0f, look::kUnit, p.label);
        c.text(units_.number(Quantity::torque, t1 / 1.1, 0).c_str(), a.x - 4.0f, a.y + 10.0f, look::kUnit, p.label, funkgui::Align::right);
        c.text(units_.number(Quantity::power, w1 / 1.1, 0).c_str(), a.right() + 4.0f, a.y + 10.0f, look::kUnit, p.label);
        std::vector<float> xs, ys;
        // Power also has a dot every fourth point: its shape, besides its ink and its scale's word.
        const auto trace = [&](const DynoTrace& d, const std::vector<double>& v, double top, funkgui::Col ink, float width,
                               bool dots) {
            xs.clear();
            ys.clear();
            for (std::size_t i = 0; i < d.speed.size() && i < v.size(); ++i)
            {
                xs.push_back(a.x + a.w * static_cast<float>((d.speed[i] - x0) / (x1 - x0)));
                ys.push_back(a.bottom() - a.h * static_cast<float>(std::clamp(v[i] / top, 0.0, 1.0)));
            }
            if (xs.size() >= 2)
                c.polyline(xs.data(), ys.data(), static_cast<int>(xs.size()), width, ink);
            for (std::size_t i = 0; dots && i < xs.size(); i += 4)
                c.disc(xs[i], ys[i], 2.5f, ink);
        };
        const funkgui::Canvas::ClipScope clip(c, a.expanded(2.0f));
        if (!was.empty())
        {
            trace(was, was.torque, t1, p.over(p.value, look::kPlot.ghostAlpha), look::kPlot.ghost, false);
            trace(was, was.power, w1, p.over(p.text, look::kPlot.ghostAlpha), look::kPlot.ghost, true);
        }
        if (!now.empty())
        {
            trace(now, now.torque, t1, p.value, look::kPlot.trace, false);
            trace(now, now.power, w1, p.text, look::kPlot.trace, true);
        }
    }

    void DynoRibbon::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        out.push_back(a11yOf(id_, funkgui::A11yRole::image, bounds_, "Predicted torque and power", peaksText()));
    }
}
