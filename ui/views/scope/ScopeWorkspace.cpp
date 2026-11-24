// ui/views/scope/ScopeWorkspace.cpp: see ScopeWorkspace.h.
#include "views/scope/ScopeWorkspace.h"

#include "app/Look.h"
#include "views/scope/Layout.h"

#include <algorithm>
#include <bit>
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
        using Ink = ScopePlot::Ink;
        using Style = ScopePlot::Style;

        constexpr double kPi = scope::kPi;
        constexpr double kRpmPerRad = 30.0 / kPi;          // rpm in one rad/s
        constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
        constexpr double kAtm = 101325.0;
        constexpr int kMaxDegrees = 1080;                   // sim/spec/scope.fm SCOPE_DEGREES
        constexpr int kTail = 8192;                         // the shell keeps 8192 (shell/player/Boxes.h)
        constexpr double kDefaultRate = 48000.0;            // until telemetry says the device's rate ("sound.rate")
        constexpr float kBarH = 58.0f, kGap = 10.0f;

        // The round number at or above v (1, 2, 2.5, 5 times a power of ten), and at or below.
        double niceUp(double v)
        {
            if (!(v > 0.0) || !std::isfinite(v))
                return 1.0;
            const double m = std::pow(10.0, std::floor(std::log10(v)));
            for (const double k : { 1.0, 2.0, 2.5, 5.0, 10.0 })
                if (m * k >= v * (1.0 - 1e-9))
                    return m * k;
            return 10.0 * m;
        }
        double niceDown(double v)
        {
            if (!(v > 0.0) || !std::isfinite(v))
                return 1.0;
            const double m = std::pow(10.0, std::floor(std::log10(v)));
            for (const double k : { 5.0, 2.5, 2.0, 1.0 })
                if (m * k <= v * (1.0 + 1e-9))
                    return m * k;
            return m;
        }
        std::string fmt(const char* f, double v)
        {
            if (!std::isfinite(v))
                return "-";
            char b[48];
            std::snprintf(b, sizeof b, f, v);
            return b;
        }
        // A degree after firing top centre, -n/2 .. n/2.
        double wrapDeg(double d, int n)
        {
            const double h = n * 0.5;
            d = std::fmod(d + h, static_cast<double>(n));
            if (d < 0.0)
                d += n;
            return d - h;
        }
        // Where a valve opens and closes (the indices where its lift rises from and falls to nothing); -1 when it
        // never does.
        void events(const std::vector<float>& lift, int n, int& open, int& close)
        {
            open = close = -1;
            if (n <= 0 || static_cast<int>(lift.size()) < n)
                return;
            float top = 0.0f;
            for (int i = 0; i < n; ++i)
                top = std::max(top, lift[static_cast<std::size_t>(i)]);
            if (!(top > 0.0f))
                return;
            const float thr = top * 0.02f;
            for (int i = 0; i < n; ++i)
            {
                const bool now = lift[static_cast<std::size_t>(i)] > thr;
                const bool before = lift[static_cast<std::size_t>((i + n - 1) % n)] > thr;
                if (now && !before)
                    open = i;
                if (!now && before)
                    close = i;
            }
        }
    }

    ScopeWorkspace::ScopeWorkspace(WorkspaceContext& c) : Workspace(c)
    {
        freeze_ = &make<Button>("FREEZE", [this] { setFrozen(!frozen_); });
        freeze_->setLit([this] { return frozen_; });
        log_ = &make<Button>("LOG P-V", [this] { setLogPv(!logPv_); });
        log_->setLit([this] { return logPv_; });
        clear_ = &make<Button>("CLEAR MAP", [this] { clearMap(); });
        // What is plotted on a small stage (a large one shows all six).
        const char* words[6] = { "PRESSURE", "P-V", "PORTS", "WAVE", "SPECTRUM", "MAP" };
        for (int i = 0; i < 6; ++i)
        {
            selectors_[static_cast<std::size_t>(i)] = &make<Button>(words[i], [this, i] { choosePlot(i); });
            selectors_[static_cast<std::size_t>(i)]->setLit(
                [this, i] { return std::find(chosen_.begin(), chosen_.end(), i) != chosen_.end(); });
        }

        pressure_ = &make<ScopePlot>("Cylinder pressure");
        pv_ = &make<ScopePlot>("P-V loop");
        ports_ = &make<ScopePlot>("Ports");
        wave_ = &make<ScopePlot>("Output wave");
        spec_ = &make<ScopePlot>("Spectrum");
        map_ = &make<ScopePlot>("Speed map");

        pressure_->setTraces({ { "P", Style{ Ink::value, false, true, look::kTrace } } });
        pv_->setTraces({ { "P", Style{ Ink::value, false, false, look::kTrace } } });
        ports_->setTraces({ { "IN", Style{ Ink::cold, false, false, look::kTrace } },
                            { "EX", Style{ Ink::hot, true, false, 2.0f } } });
        wave_->setTraces({ { "", Style{ Ink::value, false, false, 1.25f } } });
        spec_->setTraces({ { "", Style{ Ink::value, false, true, 1.25f } } });

        const auto degAxis = [](ScopePlot* p) {
            p->xAxis().label = "deg ATDC";
            p->xAxis().format = [](double v) { return fmt("%.0f", v); };
        };
        degAxis(pressure_);
        degAxis(ports_);
        wave_->xAxis().label = "ms";
        wave_->yAxis().label = "full scale";
        spec_->xAxis().label = "Hz";
        spec_->yAxis().label = "dB";
        map_->xAxis().label = "rpm";
        map_->yAxis().label = "Hz";
        map_->xAxis().format = [](double v) { return fmt("%.0f", v); };
        spec_->setAxes(20.0, 20000.0, true, -100.0, 0.0, false);
        wave_->setAxes(-50.0, 0.0, false, -1.0, 1.0, false);
        map_->setAxes(campbell_.rpm0(), campbell_.rpm1(), false, campbell_.f0(), campbell_.f1(), true);

        pressure_->setOverlay([this](Canvas& cv, const look::Palette& p, const ScopePlot& pl) {
            drawValveBands(cv, p, pl, true);
            if (pl.hasData())
            {
                const Rect a = pl.plotArea();
                // Top centres: firing at 0, overlap at half the cycle either side.
                for (const double d : { 0.0, -degrees_ * 0.5, degrees_ * 0.5 })
                {
                    const float x = cv.snapX(pl.pxX(d));
                    cv.hairlineV(x, pl.plotArea().y, pl.plotArea().h, p.dim);
                    cv.text(d == 0.0 ? "TDC FIRE" : "TDC", x + 4.0f, pl.plotArea().y + 4.0f, look::kCaps, p.label);
                }
                if (std::isfinite(f_.sparkDeg))
                {
                    const float x = cv.snapX(pl.pxX(-f_.sparkDeg));
                    const float xs[2] = { x, x }, ys[2] = { a.y + 18.0f, a.bottom() - 26.0f };
                    dashedPolyline(cv, xs, ys, 2, 1.25f, p.value, 4.0f, 3.0f);
                    cv.text("SPARK", x - 4.0f, a.y + 18.0f, look::kCaps, p.text, funkgui::Align::right);
                }
                // The valves' events in numbers, at the top right (where the expansion has fallen away).
                cv.text(("EVO " + fmt("%.0f", f_.evo) + "  EVC " + fmt("%.0f", f_.evc)).c_str(), a.right() - 6.0f, a.y + 22.0f,
                        look::kUnit, p.hot, funkgui::Align::right);
                cv.text(("IVO " + fmt("%.0f", f_.ivo) + "  IVC " + fmt("%.0f", f_.ivc)).c_str(), a.right() - 6.0f, a.y + 36.0f,
                        look::kUnit, p.cold, funkgui::Align::right);
                if (std::isfinite(f_.peakDeg))
                {
                    const float x = pl.pxX(f_.peakDeg), y = pl.pxY(ctx_.units.toUser(Quantity::pressure, f_.peakPa));
                    cv.disc(x, y, 4.0f, p.value, 1.5f, p.ground);
                    cv.text("PEAK", x + 8.0f, y - 5.0f, look::kCaps, p.value);
                }
            }
        });
        ports_->setOverlay(
            [this](Canvas& cv, const look::Palette& p, const ScopePlot& pl) {
                drawValveBands(cv, p, pl, false);
                if (pl.hasData())
                {
                    const float y = cv.snapY(pl.pxY(ctx_.units.toUser(Quantity::pressure, kAtm)));
                    cv.hairlineH(pl.plotArea().x, y, pl.plotArea().w, p.label);
                    cv.text("AIR OUTSIDE", pl.plotArea().x + 4.0f, y - 13.0f, look::kCaps, p.label);
                }
            },
            [](Canvas& cv, const look::Palette& p, const ScopePlot& pl) {
                if (pl.cursorOn())
                    return;
                const Rect a = pl.plotArea();
                float x = a.right() - 200.0f;
                if (x < pl.titleRight(cv) + 12.0f)   // no room beside the title: inside the area's top right
                {
                    cv.rrect(a.right() - 206.0f, a.y + 2.0f, 204.0f, 16.0f, 3.0f, p.panel);
                    x += 2.0f;
                    x += legendEntry(cv, p, x, a.y + 5.0f, "INTAKE", p.cold, look::kTrace, false);
                    legendEntry(cv, p, x, a.y + 5.0f, "EXHAUST", p.hot, 2.0f, true);
                    return;
                }
                x += legendEntry(cv, p, x, a.y - 15.0f, "INTAKE", p.cold, look::kTrace, false);
                legendEntry(cv, p, x, a.y - 15.0f, "EXHAUST", p.hot, 2.0f, true);
            });
        pv_->setOverlay([this](Canvas& cv, const look::Palette& p, const ScopePlot& pl) {
            if (!pl.hasData() || degrees_ <= 0)
                return;
            // The events as points on the loop.
            const auto mark = [&](double atdc, const char* name, Col ink, bool below) {
                if (!std::isfinite(atdc))
                    return;
                const int i = (tdc_ + static_cast<int>(std::lround(atdc)) % degrees_ + degrees_) % degrees_;
                const double v = ctx_.units.toUser(Quantity::volume, v_[static_cast<std::size_t>(i)]);
                const double pr = ctx_.units.toUser(Quantity::pressure, p_[static_cast<std::size_t>(i)]);
                const float x = pl.pxX(v), y = pl.pxY(pr);
                if (!std::isfinite(x) || !std::isfinite(y))
                    return;
                cv.disc(x, y, 3.5f, ink, 1.25f, p.ground);
                if (below)
                    cv.text(name, x - 7.0f, y - 14.0f, look::kCaps, ink, funkgui::Align::right);
                else
                    cv.text(name, x + 7.0f, y - 12.0f, look::kCaps, ink);
            };
            mark(f_.evo, "EVO", p.hot, false);
            mark(f_.ivc, "IVC", p.cold, true);
            if (std::isfinite(f_.sparkDeg))
                mark(-f_.sparkDeg, "SPARK", p.text, false);
        });
        wave_->setOverlay([this](Canvas& cv, const look::Palette& p, const ScopePlot& pl) {
            if (!pl.hasData())
                return;
            const Rect a = pl.plotArea();
            // Each cycle's start (cylinder 1's firing top centre), the newest at 0 ms.
            for (const double ms : cycleMarksMs_)
                cv.hairlineV(cv.snapX(pl.pxX(ms)), a.y, a.h, p.dim);
            const float y = cv.snapY(pl.pxY(0.0));
            cv.hairlineH(a.x, y, a.w, p.dim);
        });
        spec_->setOverlay([this](Canvas& cv, const look::Palette& p, const ScopePlot& pl) { drawOrders(cv, p, pl); });
        map_->setOverlay([this](Canvas& cv, const look::Palette& p, const ScopePlot& pl) { drawCampbell(cv, p, pl); },
                         [](Canvas& cv, const look::Palette& p, const ScopePlot& pl) {
                             if (pl.cursorOn())
                                 return;
                             // The scale: levels as the cells show them; beside the title, or in the area's top left.
                             const Rect a = pl.plotArea();
                             const float w = 10.0f, h = 9.0f;
                             float x = a.right() - 6 * w - 54.0f, y = a.y - 15.0f;
                             if (x < pl.titleRight(cv) + 10.0f)
                             {
                                 x = a.x + 4.0f;
                                 y = a.y + 5.0f;
                                 cv.rrect(x - 2.0f, y - 3.0f, 6 * w + 58.0f, 16.0f, 3.0f, p.panel);
                             }
                             cv.text("-90", x, y - 1.0f, look::kUnit, p.label);
                             x += 22.0f;
                             for (int i = 0; i < 6; ++i)
                                 cv.rrect(x + i * w, y, w, h, 0.0f, p.over(p.value, (i + 0.5f) / 6.0f));
                             cv.text("0 dB", x + 6 * w + 4.0f, y - 1.0f, look::kUnit, p.label);
                         });

        p_.assign(kMaxDegrees, 0.0f);
        v_ = li_ = le_ = pi_ = pe_ = p_;
        tail_.assign(kTail, 0.0f);
        refreshPlots();
    }

    bool ScopeWorkspace::moving() const { return (live_ && !frozen_) || wantsFullRate(); }

    std::uint64_t ScopeWorkspace::drawKey() const
    {
        return DrawKey{}.mix(revision_).mix(frozen_).mix(logPv_).mix(ctx_.units.revision()).h;
    }

    void ScopeWorkspace::setFrozen(bool f)
    {
        if (f != frozen_)
        {
            frozen_ = f;
            ++revision_;
        }
    }

    void ScopeWorkspace::setLogPv(bool on)
    {
        if (on != logPv_)
        {
            logPv_ = on;
            refreshPlots();
            ++revision_;
        }
    }

    void ScopeWorkspace::clearMap()
    {
        campbell_.clear();
        refreshPlots();
        ++revision_;
    }

    double ScopeWorkspace::rpmNow() const
    {
        const double w = ctx_.live.get("engine.speed");
        return std::isfinite(w) ? w * kRpmPerRad : kNaN;
    }

    double ScopeWorkspace::toAtdc(int index) const noexcept
    {
        return index < 0 || degrees_ <= 0 ? kNaN : wrapDeg(static_cast<double>(index - tdc_), degrees_);
    }

    // ---- reading --------------------------------------------------------------------------------------------------
    void ScopeWorkspace::step(float dt)
    {
        if (frozen_)
            return;
        readScope();
        readOutput(dt);
    }

    void ScopeWorkspace::readScope()
    {
        const std::uint64_t g = ctx_.scope.generation();
        if (g == seenGeneration_)
            return;
        seenGeneration_ = g;
        ++reads_;
        const auto read = [&](const char* name, std::vector<float>& out) {
            const int h = ctx_.scope.find(name);
            return h < 0 ? 0 : ctx_.scope.read(h, out.data(), static_cast<int>(out.size()));
        };
        float deg = 0.0f;
        const int hd = ctx_.scope.find("degrees");
        const int nd = hd < 0 ? 0 : ctx_.scope.read(hd, &deg, 1);
        const int np = read("pressure", p_);
        const int nv = read("volume", v_);
        const int ni = read("liftIntake", li_);
        const int ne = read("liftExhaust", le_);
        const int npi = read("portIntake", pi_);
        const int npe = read("portExhaust", pe_);
        degrees_ = nd > 0 && deg > 0.0f ? std::min(kMaxDegrees, static_cast<int>(deg)) : np;
        if (np < degrees_)
            degrees_ = np;
        // What is missing reads as nothing.
        const auto keep = [&](std::vector<float>& a, int n) {
            if (n < degrees_)
                std::fill(a.begin(), a.end(), std::numeric_limits<float>::quiet_NaN());
        };
        keep(v_, nv);
        keep(li_, ni);
        keep(le_, ne);
        keep(pi_, npi);
        keep(pe_, npe);
        cylinderFacts();
        refreshPlots();
        ++revision_;
    }

    void ScopeWorkspace::cylinderFacts()
    {
        const int n = degrees_;
        f_ = Facts{};
        f_.degrees = n;
        f_.rpm = rpmNow();
        f_.cylinders = layout::cylinders(ctx_.doc);
        f_.firingOrder = layout::firingPerRev(f_.cylinders, n > 0 ? n : 720, layout::isRotary(layout::layoutName(ctx_.doc)));
        f_.firingHz = f_.firingOrder > 0.0 && std::isfinite(f_.rpm) ? f_.rpm / 60.0 * f_.firingOrder : kNaN;
        const double adv = ctx_.live.get("ecu.advance");
        f_.sparkDeg = adv;
        f_.peakPa = f_.peakDeg = f_.ivo = f_.ivc = f_.evo = f_.evc = kNaN;
        if (n <= 0)
            return;
        // Firing top centre: of the cycle's two smallest volumes, the one with the higher pressure.
        tdc_ = 0;
        if (std::isfinite(v_[0]))
        {
            int a = 0;
            for (int i = 1; i < n; ++i)
                if (v_[static_cast<std::size_t>(i)] < v_[static_cast<std::size_t>(a)])
                    a = i;
            int b = (a + n / 2) % n;   // the other: the smallest within a quarter cycle of the opposite point
            for (int k = -n / 8; k <= n / 8; ++k)
            {
                const int i = ((a + n / 2 + k) % n + n) % n;
                if (v_[static_cast<std::size_t>(i)] < v_[static_cast<std::size_t>(b)])
                    b = i;
            }
            tdc_ = p_[static_cast<std::size_t>(a)] >= p_[static_cast<std::size_t>(b)] ? a : b;
        }
        int peak = 0;
        for (int i = 1; i < n; ++i)
            if (p_[static_cast<std::size_t>(i)] > p_[static_cast<std::size_t>(peak)])
                peak = i;
        f_.peakPa = p_[static_cast<std::size_t>(peak)];
        f_.peakDeg = toAtdc(peak);
        int o = -1, cl = -1;
        events(li_, n, o, cl);
        f_.ivo = toAtdc(o);
        f_.ivc = toAtdc(cl);
        events(le_, n, o, cl);
        f_.evo = toAtdc(o);
        f_.evc = toAtdc(cl);
    }

    void ScopeWorkspace::readOutput(float dt)
    {
        const int n = ctx_.telemetry.outputTail(tail_.data(), kTail);
        const double rpm = rpmNow();
        if (std::isfinite(rpm) != std::isfinite(f_.rpm) || std::fabs(rpm - f_.rpm) > 0.5)
        {
            f_.rpm = rpm;
            f_.firingHz = f_.firingOrder > 0.0 && std::isfinite(rpm) ? rpm / 60.0 * f_.firingOrder : kNaN;
            ++revision_;
        }
        // The same samples as last time (an engine stopped, a test's still frame): nothing new to show.
        std::uint64_t sum = 1469598103934665603ull;
        for (int i = std::max(0, n - 512); i < n; ++i)
        {
            sum ^= std::bit_cast<std::uint32_t>(tail_[static_cast<std::size_t>(i)]);
            sum *= 1099511628211ull;
        }
        sum ^= static_cast<std::uint64_t>(n);
        const bool same = n > 0 && sum == tailSum_;
        sinceSpectrum_ += dt;
        if (n <= 0)
        {
            const bool was = tailN_ > 0;
            tailN_ = 0;
            live_ = false;
            if (was)
            {
                refreshPlots();
                ++revision_;
            }
            return;
        }
        live_ = !same;
        if (same)
            return;
        tailSum_ = sum;
        tailN_ = n;
        const double hz = ctx_.live.get("sound.rate");
        const double rate = std::isfinite(hz) && hz > 1000.0 ? hz : kDefaultRate;
        if (spectrum_.empty() || sinceSpectrum_ >= 1.0f / 30.0f)
        {
            spectrum_.analyse(tail_.data(), n, rate, spectrum_.empty() ? 0.0f : sinceSpectrum_);
            sinceSpectrum_ = 0.0f;
            if (std::isfinite(rpm) && rpm > 100.0)
                campbell_.take(rpm, spectrum_);
        }
        refreshPlots();
        ++revision_;
    }

    // ---- the plots' data ------------------------------------------------------------------------------------------
    void ScopeWorkspace::refreshPlots()
    {
        const Units& u = ctx_.units;
        const std::string pu = u.unit(Quantity::pressure).name, vu = u.unit(Quantity::volume).name;
        const int n = degrees_;
        const double half = n > 0 ? n * 0.5 : 360.0;

        // Cylinder pressure and the P-V loop, in cycle order from half a cycle before firing top centre.
        PlotSeries& ps = pressure_->series()[0];
        PlotSeries& lv = pv_->series()[0];
        ps.x.clear();
        ps.y.clear();
        lv.x.clear();
        lv.y.clear();
        double pmax = 0.0, pmin = 1e30, vmax = 0.0, vmin = 1e30;
        for (int j = 0; j < n; ++j)
        {
            const int i = (tdc_ + j - n / 2 + 2 * n) % n;
            const double pr = p_[static_cast<std::size_t>(i)], vol = v_[static_cast<std::size_t>(i)];
            ps.x.push_back(j - n / 2);
            ps.y.push_back(u.toUser(Quantity::pressure, pr));
            pmax = std::max(pmax, pr);
            pmin = std::min(pmin, pr);
            if (std::isfinite(vol))
            {
                lv.x.push_back(u.toUser(Quantity::volume, vol));
                lv.y.push_back(u.toUser(Quantity::pressure, pr));
                vmax = std::max(vmax, vol);
                vmin = std::min(vmin, vol);
            }
        }
        // Crank degrees ticked at the top and bottom centres, named (the valve events read against them).
        {
            std::vector<std::pair<double, std::string>> t;
            const int quarter = (n > 0 ? n : 720) / 4;
            for (int k = -2; k <= 2; ++k)
            {
                const int d = k * quarter;
                const bool tdc = k % 2 == 0;
                t.emplace_back(d, (d == 0 ? std::string("0") : fmt("%.0f", d)) + (tdc ? " TDC" : " BDC"));
            }
            pressure_->setXTicks(t);
            ports_->setXTicks(t);
        }
        pressure_->yAxis().label = pu;
        pressure_->setAxes(-half, half, false, 0.0, niceUp(u.toUser(Quantity::pressure, std::max(pmax, 2.0 * kAtm)) * 1.05),
                           false);
        pressure_->setNote(n > 0 ? "" : "no cylinder pressure from this program yet");
        pressure_->touched();

        pv_->xAxis().label = vu;
        pv_->yAxis().label = pu;
        if (lv.x.empty())
            pv_->setAxes(0.0, 1.0, false, 0.0, 1.0, false);
        else if (logPv_)
            pv_->setAxes(niceDown(u.toUser(Quantity::volume, vmin) * 0.9), niceUp(u.toUser(Quantity::volume, vmax) * 1.1),
                         true, niceDown(u.toUser(Quantity::pressure, std::max(1000.0, pmin) * 0.8)),
                         niceUp(u.toUser(Quantity::pressure, pmax) * 1.2), true);
        else
            pv_->setAxes(0.0, niceUp(u.toUser(Quantity::volume, vmax) * 1.05), false, 0.0,
                         niceUp(u.toUser(Quantity::pressure, std::max(pmax, 2.0 * kAtm)) * 1.05), false);
        pv_->setNote(n <= 0 ? "no cylinder pressure from this program yet"
                            : lv.x.empty() ? "no cylinder volume from this program yet" : "");
        pv_->setCaption(lv.x.empty() ? std::string() : logPv_ ? "LOG-LOG" : "LINEAR");
        pv_->touched();

        // The ports.
        double qmax = 0.0, qmin = 1e30;
        for (int k = 0; k < 2; ++k)
        {
            const std::vector<float>& a = k == 0 ? pi_ : pe_;
            PlotSeries& s = ports_->series()[static_cast<std::size_t>(k)];
            s.x.clear();
            s.y.clear();
            if (n <= 0 || !std::isfinite(a[0]))
                continue;
            for (int j = 0; j < n; ++j)
            {
                const int i = (tdc_ + j - n / 2 + 2 * n) % n;
                s.x.push_back(j - n / 2);
                s.y.push_back(u.toUser(Quantity::pressure, a[static_cast<std::size_t>(i)]));
                qmax = std::max(qmax, static_cast<double>(a[static_cast<std::size_t>(i)]));
                qmin = std::min(qmin, static_cast<double>(a[static_cast<std::size_t>(i)]));
            }
        }
        ports_->yAxis().label = pu;
        if (qmax > 0.0)
        {
            const double lo = u.toUser(Quantity::pressure, std::min(qmin, kAtm)), hi = u.toUser(Quantity::pressure, qmax);
            const double span = niceUp((hi - lo) * 0.25);
            ports_->setAxes(-half, half, false, std::floor(lo / span) * span, std::ceil(hi / span) * span, false);
        }
        else
            ports_->setAxes(-half, half, false, 0.0, niceUp(u.toUser(Quantity::pressure, 2.0 * kAtm)), false);
        ports_->setNote(n <= 0 ? "no Scope from this program yet"
                               : qmax > 0.0 ? "" : "no port pressures from this program yet");
        ports_->touched();

        // The output's wave, triggered on the crank's cycle.
        PlotSeries& w = wave_->series()[0];
        w.x.clear();
        w.y.clear();
        cycleMarksMs_.clear();
        const double hz = ctx_.live.get("sound.rate");
        const double rate = std::isfinite(hz) && hz > 1000.0 ? hz : kDefaultRate;
        const int nt = tailN_;
        if (nt > 0)
        {
            const double speed = ctx_.live.get("engine.speed");            // rad/s
            const double crank = ctx_.live.get("engine.crank");            // degrees, 0..2160
            const double cycDeg = n == 1080 ? 1080.0 : n == 360 ? 360.0 : 720.0;
            int end = nt;
            double cycleSamples = 0.0;
            if (std::isfinite(speed) && speed > 1.0)
            {
                cycleSamples = rate * (cycDeg / 360.0) * (2.0 * kPi / speed);
                if (std::isfinite(crank))
                    end = nt - static_cast<int>(std::lround(std::fmod(std::fmod(crank, cycDeg) + cycDeg, cycDeg) / cycDeg
                                                            * cycleSamples));
            }
            if (end == nt || !std::isfinite(crank))
            {
                // No crank: the newest rising zero crossing.
                for (int i = nt - 1; i > nt / 2; --i)
                    if (tail_[static_cast<std::size_t>(i - 1)] < 0.0f && tail_[static_cast<std::size_t>(i)] >= 0.0f)
                    {
                        end = i;
                        break;
                    }
            }
            end = std::clamp(end, 1, nt);
            int span = static_cast<int>(rate * 0.05);    // 50 ms, or whole cycles that fit in it
            if (cycleSamples > 1.0)
            {
                const int cycles = std::clamp(static_cast<int>(0.05 * rate / cycleSamples), 1, 8);
                span = static_cast<int>(cycles * cycleSamples);
                for (int k = 0; k <= cycles; ++k)
                    cycleMarksMs_.push_back(-k * cycleSamples / rate * 1000.0);
            }
            span = std::min(span, end);
            const int start = end - span;
            // Min and max of each bucket, in the order they come, about 600 buckets across.
            const int bucket = std::max(1, span / 600);
            double peak = 0.0;
            for (int b = start; b < end; b += bucket)
            {
                int lo = b, hi = b;
                for (int i = b; i < std::min(end, b + bucket); ++i)
                {
                    if (tail_[static_cast<std::size_t>(i)] < tail_[static_cast<std::size_t>(lo)])
                        lo = i;
                    if (tail_[static_cast<std::size_t>(i)] > tail_[static_cast<std::size_t>(hi)])
                        hi = i;
                }
                for (const int i : { std::min(lo, hi), std::max(lo, hi) })
                {
                    w.x.push_back((i - end) / rate * 1000.0);
                    w.y.push_back(tail_[static_cast<std::size_t>(i)]);
                    peak = std::max(peak, std::fabs(static_cast<double>(tail_[static_cast<std::size_t>(i)])));
                    if (lo == hi)
                        break;
                }
            }
            const double ms = span / rate * 1000.0;
            const double amp = std::min(1.0, niceUp(std::max(peak * 1.1, 0.05)));
            wave_->setAxes(-ms, 0.0, false, -amp, amp, false);
            wave_->setCaption(cycleSamples > 1.0 ? fmt("%.0f", (cycleMarksMs_.size() - 1.0)) + " CYCLE"
                                                       + (cycleMarksMs_.size() > 2 ? "S" : "") + "   "
                                                       + (std::isfinite(crank) ? "ON THE CRANK" : "ON ITS EDGE")
                                                 : std::string("ON ITS EDGE"));
        }
        wave_->setNote(nt > 0 ? "" : "no output yet");
        wave_->touched();

        // The spectrum: a point a band, about 360 bands log-spaced.
        PlotSeries& sp = spec_->series()[0];
        sp.x.clear();
        sp.y.clear();
        const double top = std::min(20000.0, spectrum_.rate() * 0.5);
        if (!spectrum_.empty() && nt > 0)
        {
            const int bands = 360;
            for (int b = 0; b < bands; ++b)
            {
                const double f0 = 20.0 * std::pow(top / 20.0, static_cast<double>(b) / bands);
                const double f1 = 20.0 * std::pow(top / 20.0, static_cast<double>(b + 1) / bands);
                sp.x.push_back(std::sqrt(f0 * f1));
                sp.y.push_back(std::max(-120.0, static_cast<double>(spectrum_.bandDb(f0, f1))));
            }
        }
        spec_->setAxes(20.0, top, true, -100.0, 0.0, false);
        spec_->setNote(nt > 0 ? "" : "no output yet");
        spec_->setCaption(std::isfinite(f_.firingHz) ? "FIRING " + fmt("%.0f", f_.firingHz) + " HZ" : std::string());
        spec_->touched();

        // The speed map: its cells are drawn by the overlay; its range follows the redline.
        const double red = ctx_.live.get("engine.redline");
        const double rpm1 = std::isfinite(red) && red > 0.0 ? std::ceil(red * kRpmPerRad * 1.1 / 1000.0) * 1000.0 : 8000.0;
        if (rpm1 != campbell_.rpm1())
            campbell_.setSpeedRange(0.0, rpm1);
        map_->setAxes(campbell_.rpm0(), campbell_.rpm1(), false, campbell_.f0(), campbell_.f1(), true);
        map_->setNote(campbell_.filled() > 0 ? "" : "fills in as the engine sweeps its speed");
        map_->touched();
    }

    // ---- drawing ----------------------------------------------------------------------------------------------------
    void ScopeWorkspace::drawValveBands(Canvas& c, const look::Palette& p, const ScopePlot& pl, bool labels) const
    {
        const Rect a = pl.plotArea();
        const float h = 9.0f, y = a.bottom() - h - 3.0f;
        const auto band = [&](double open, double close, Col ink, const char* word, float row) {
            if (!std::isfinite(open) || !std::isfinite(close) || degrees_ <= 0)
                return;
            const double half = degrees_ * 0.5;
            const auto piece = [&](double d0, double d1) {
                const float x0 = pl.pxX(d0), x1 = pl.pxX(d1);
                if (x1 - x0 > 0.5f)
                    c.rrect(x0, y - row, x1 - x0, h, 2.0f, p.over(ink, 0.55f), 1.0f, ink);
            };
            if (close >= open)
                piece(open, close);
            else
            {
                piece(open, half);
                piece(-half, close);
            }
            if (labels)
            {
                const double mid = close >= open ? 0.5 * (open + close) : wrapDeg(0.5 * (open + close + degrees_), degrees_);
                c.text(word, pl.pxX(mid), y - row - 14.0f, look::kCaps, ink, funkgui::Align::centre);
            }
        };
        band(f_.evo, f_.evc, p.hot, "EXHAUST", 0.0f);
        band(f_.ivo, f_.ivc, p.cold, "INTAKE", h + 3.0f);
    }

    void ScopeWorkspace::drawOrders(Canvas& c, const look::Palette& p, const ScopePlot& pl) const
    {
        if (!(f_.firingOrder > 0.0) || !std::isfinite(f_.rpm) || f_.rpm < 60.0 || !pl.hasData())
            return;
        const Rect a = pl.plotArea();
        const double rev = f_.rpm / 60.0;   // crank revolutions a second
        float lastLabel = -1e9f;
        for (int k = 1; k <= 12; ++k)
        {
            const double order = f_.firingOrder * k;
            const double hz = order * rev;
            const float x = c.snapX(pl.pxX(hz));
            if (!(x > a.x + 2.0f) || x > a.right() - 2.0f)
                continue;
            const bool firing = k == 1;
            c.segment(x, a.y + 16.0f, x, a.bottom(), firing ? 1.5f : 1.0f, firing ? p.value : p.dim);
            // Numbered where there is room beside the last number.
            const std::string t = fmt(order == std::floor(order) ? "%.0f" : "%.1f", order);
            const float tw = c.textWidth(t.c_str(), look::kUnit);
            if (x - tw * 0.5f > lastLabel + 6.0f)
            {
                c.text(t.c_str(), x, a.y + 3.0f, look::kUnit, firing ? p.value : p.label, funkgui::Align::centre);
                lastLabel = x + tw * 0.5f;
            }
        }
        c.text("ORDER", a.x + 4.0f, a.y + 3.0f, look::kCaps, p.label);
    }

    void ScopeWorkspace::drawCampbell(Canvas& c, const look::Palette& p, const ScopePlot& pl) const
    {
        const Rect a = pl.plotArea();
        const scope::Campbell& m = campbell_;
        // The cells: brighter is louder (the ink's ladder, so either theme reads); the scale in the title line.
        constexpr float lo = -90.0f, hi = 0.0f;
        for (int col = 0; col < m.cols(); ++col)
        {
            const double r0 = m.rpm0() + (m.rpm1() - m.rpm0()) * col / m.cols();
            const double r1 = m.rpm0() + (m.rpm1() - m.rpm0()) * (col + 1) / m.cols();
            const float x0 = c.snapX(pl.pxX(r0)), x1 = c.snapX(pl.pxX(r1));
            for (int row = 0; row < m.rows(); ++row)
            {
                const float v = m.cell(col, row);
                if (!std::isfinite(v))
                    continue;
                const float t = std::clamp((v - lo) / (hi - lo), 0.0f, 1.0f);
                const float y1 = c.snapY(pl.pxY(m.rowLo(row))), y0 = c.snapY(pl.pxY(m.rowHi(row)));
                c.rrect(x0, y0, x1 - x0, y1 - y0, 0.0f, p.over(p.value, 0.06f + 0.94f * t));
            }
        }
        // The orders as slanted lines: f = order * rpm / 60, numbered where they leave the top or the right.
        if (f_.firingOrder > 0.0 && m.filled() > 0)
        {
            float lastY = 1e9f, lastX = -1e9f;
            for (int k = 1; k <= 4; ++k)
            {
                const double order = f_.firingOrder * k;
                const double rA = std::max(m.rpm0(), m.f0() * 60.0 / order), rB = std::min(m.rpm1(), m.f1() * 60.0 / order);
                if (!(rB > rA))
                    continue;
                const float xs[2] = { pl.pxX(rA), pl.pxX(rB) };
                const float ys[2] = { pl.pxY(order * rA / 60.0), pl.pxY(order * rB / 60.0) };
                dashedPolyline(c, xs, ys, 2, k == 1 ? 1.75f : 1.0f, k == 1 ? p.value : p.label, 5.0f, 4.0f);
                // Numbered where it leaves the plot, while the numbers keep apart.
                const bool right = rB >= m.rpm1();
                const std::string t = fmt(order == std::floor(order) ? "%.0f" : "%.1f", order);
                if (right ? lastY - ys[1] >= 12.0f : xs[1] - lastX >= 18.0f)
                {
                    c.text(t.c_str(), xs[1] - 5.0f, right ? ys[1] - 6.0f : ys[1] + 3.0f, look::kUnit,
                           k == 1 ? p.value : p.label, funkgui::Align::right);
                    if (right)
                        lastY = ys[1];
                    else
                        lastX = xs[1];
                }
            }
        }
        // Where the engine is now.
        if (std::isfinite(f_.rpm) && f_.rpm >= m.rpm0() && f_.rpm < m.rpm1())
        {
            const float x = c.snapX(pl.pxX(f_.rpm));
            c.segment(x, a.y, x, a.bottom(), 1.0f, p.selected);
        }
    }

    void ScopeWorkspace::layout(const Rect& s)
    {
        // A large stage shows all six plots; a small one (the frame at its smallest: about 660 x 570) shows the plots
        // chosen with the selectors, at most three, and keeps the buttons in a row of their own.
        compact_ = s.w < 1000.0f || s.h < 640.0f;
        bar_ = { s.x + kGap, s.y + kGap, s.w - 2 * kGap, kBarH };
        const float bh = 26.0f;
        const auto widthOf = [](Button* b) { return 18.0f + 8.4f * static_cast<float>(b->title().size()); };
        float top = bar_.bottom() + kGap;
        if (compact_)
        {
            tools_ = { bar_.x, top, bar_.w, bh };
            float x = tools_.x;
            for (Button* b : selectors_)
            {
                const float w = widthOf(b);
                b->setPlace({ x, tools_.y, w, bh }, true);
                x += w + 4.0f;
            }
            float bx = tools_.right();
            for (Button* b : { clear_, log_, freeze_ })
            {
                const float w = widthOf(b);
                bx -= w;
                b->setPlace({ bx, tools_.y, w, bh }, true);
                bx -= 4.0f;
            }
            top = tools_.bottom() + kGap;
        }
        else
        {
            tools_ = {};
            for (Button* b : selectors_)
                b->setPlace({}, false);
            const float bw = 96.0f, by = bar_.y + (kBarH - 28.0f) * 0.5f;
            float bx = bar_.right() - look::kPad - bw;
            for (Button* b : { clear_, log_, freeze_ })
            {
                b->setPlace({ bx, by, bw, 28.0f }, true);
                bx -= bw + 8.0f;
            }
        }
        ScopePlot* plots[6] = { pressure_, pv_, ports_, wave_, spec_, map_ };
        for (Rect& r : cells_)
            r = {};
        const float W = s.w - 2 * kGap, H = s.bottom() - kGap - top;
        if (!compact_)
        {
            const float h = (H - kGap) * 0.5f, w = (W - 2 * kGap) / 3.0f;
            for (int i = 0; i < 6; ++i)
                cells_[static_cast<std::size_t>(i)] = { s.x + kGap + (i % 3) * (w + kGap), top + (i / 3) * (h + kGap), w, h };
        }
        else
        {
            // One: the whole room; two: side by side; three: the first across the top, two under it.
            const int n = static_cast<int>(chosen_.size());
            const float x0 = s.x + kGap, half = (W - kGap) * 0.5f, hh = (H - kGap) * 0.5f;
            for (int k = 0; k < n; ++k)
            {
                Rect r;
                if (n == 1)
                    r = { x0, top, W, H };
                else if (n == 2)
                    r = { x0 + k * (half + kGap), top, half, H };
                else
                    r = k == 0 ? Rect{ x0, top, W, hh } : Rect{ x0 + (k - 1) * (half + kGap), top + hh + kGap, half, hh };
                cells_[static_cast<std::size_t>(chosen_[static_cast<std::size_t>(k)])] = r;
            }
        }
        for (int i = 0; i < 6; ++i)
        {
            const Rect& r = cells_[static_cast<std::size_t>(i)];
            plots[i]->setPlace(r.isEmpty() ? Rect{} : Rect{ r.x + 8.0f, r.y + 8.0f, r.w - 14.0f, r.h - 12.0f }, !r.isEmpty());
        }
    }

    void ScopeWorkspace::choosePlot(int i)
    {
        if (i < 0 || i >= 6)
            return;
        auto it = std::find(chosen_.begin(), chosen_.end(), i);
        if (it != chosen_.end())
        {
            if (chosen_.size() > 1)   // one stays shown
                chosen_.erase(it);
        }
        else
        {
            chosen_.push_back(i);
            if (chosen_.size() > 3)
                chosen_.erase(chosen_.begin());   // the one chosen longest ago goes
        }
        if (!stage_.isEmpty())
            layout(stage_);
        bump();
        ++revision_;
    }

    void ScopeWorkspace::drawStage(Canvas& c, const funkgui::Theme& th) const
    {
        const look::Palette p = look::Palette::of(th);
        const Units& u = ctx_.units;
        c.rrect(bar_.x, bar_.y, bar_.w, bar_.h, look::kRadius, p.panel, 1.0f, p.edge);
        float x = bar_.x + look::kPad;
        const float y = bar_.y + 10.0f;
        const auto block = [&](const char* label, const std::string& number, const std::string& unit, Col ink = kClear) {
            x += look::readout(c, p, x, y, label, number, unit, look::kNumberM, ink) + 26.0f;
        };
        block("SPEED", fmt("%.0f", f_.rpm), "rpm");
        block("FIRING", fmt("%.0f", f_.firingHz), "Hz");
        const bool havePeak = std::isfinite(f_.peakPa);
        const double peakUser = u.toUser(Quantity::pressure, f_.peakPa);
        block("PEAK PRESSURE", havePeak ? u.number(Quantity::pressure, f_.peakPa, std::fabs(peakUser) >= 1000.0 ? 0 : -1) : "-",
              u.unit(Quantity::pressure).name);
        block("AT", fmt("%.0f", f_.peakDeg), "deg ATDC");
        block("SPARK", fmt("%.0f", f_.sparkDeg), "deg BTDC");
        // Whether it moves: a lamp and its word.
        const float lx = compact_ ? bar_.right() - look::kPad - 80.0f : freeze_->bounds().x - 100.0f;
        if (lx > x)
        {
            if (frozen_)
                look::lamp(c, p, lx, bar_.y + bar_.h * 0.5f, "FROZEN", true, p.warning);
            else
                look::lamp(c, p, lx, bar_.y + bar_.h * 0.5f, live_ ? "LIVE" : "STILL", live_, p.ok);
        }
        for (const Rect& r : cells_)
            if (!r.isEmpty())
                c.rrect(r.x, r.y, r.w, r.h, look::kRadius, p.panel, 1.0f, p.edge);
    }

    std::unique_ptr<Workspace> makeScopeWorkspace(WorkspaceContext& c) { return std::make_unique<ScopeWorkspace>(c); }
}
