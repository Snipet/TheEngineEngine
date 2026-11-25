// ui/views/drive/DriveWorkspace.cpp: DRIVE (docs/plan/ui-design.md section 3; package U1): the instruments of a car
// being driven, and the car itself.
//
//   +--------------------------------------------+--------------------------+
//   |  TACHOMETER        SPEEDOMETER             |  BOOST  LAMBDA           |
//   |  (Gauge: arc, ticks, numerals, needle)     |  COOLANT  OIL  OIL PRESS.|
//   |                                            |  GEAR   PEDALS           |
//   +--------------------------------------------+--------------------------+
//   |  THE CAR on its road, from the side: it    |  THE TREE   TIME SLIP    |
//   |  squats as it pulls, its wheels turn       |                          |
//   |  WHEEL SLIP  FL FR RL RR                   |                          |
//   +--------------------------------------------+--------------------------+
//
// Every value is telemetry by name (sim/spec/telemetry.fm; tel.car's vehicle fields of card I: car.wheelSlip[i],
// car.wheelSpeed[i], car.pitch, car.flags, car.stripState, car.tReaction ... car.v1320). A name the program does not
// have yet reads NaN and draws a dash; the car then turns its wheels by the road speed. Clicking the car selects "car",
// the road "env", the tree "car" (its strip settings live there). Canvas primitives only: the body is a polyline and
// capsules (no rotation, no filled polygon), the wheels discs with spokes, the road a line with marks that pass.
//
// The dash above the car has styles (Dash.h; the owner's ask, 25 Nov): STANDARD (the instruments above), CLASSIC,
// DIGITAL and RACE. The driver picks one with the DASH buttons at the stage's top right, which show where the shell
// keeps the choice (FrameServices::driveStyle); STANDARD's layout is the first one, the same to the pixel.
#include "app/Cockpit.h"
#include "app/Look.h"
#include "app/Workspace.h"
#include "kit/widgets/Basic.h"
#include "kit/widgets/Gauge.h"
#include "views/drive/Dash.h"

#include <algorithm>
#include <array>
#include <initializer_list>
#include <cmath>
#include <string>

namespace tee::ui
{
    namespace
    {
        constexpr double kPi = 3.14159265358979323846;
        constexpr double kToRpm = 30.0 / kPi;
        const char* const kWheels[4] = { "FL", "FR", "RL", "RR" };
        const char* const kSlipRows[8][2] = {
            { "car.tReaction", "REACTION" }, { "car.t60", "60 FT" }, { "car.t330", "330 FT" }, { "car.t660", "1/8 MILE" },
            { "car.v660", "1/8 SPEED" }, { "car.t1000", "1000 FT" }, { "car.t1320", "1/4 MILE" }, { "car.v1320", "TRAP SPEED" },
        };
        std::string idx(const char* name, int i) { return std::string(name) + "[" + std::to_string(i) + "]"; }
    }

    class DriveWorkspace final : public Workspace
    {
    public:
        explicit DriveWorkspace(WorkspaceContext& c) : Workspace(c)
        {
            tach_ = &make<Gauge>("Engine", [this] { return ctx_.live.get(telemetry::kSpeed) * kToRpm; }, 0.0, 8000.0);
            tach_->setTicks(8, 2);
            tach_->setClearNumerals(true);
            tach_->setNumerals([](double v) { return std::to_string(static_cast<int>(std::lround(v / 1000.0))); });
            tach_->setUnit("X1000 RPM");
            tach_->setReadout([](double v) { return look::num(v, "%.0f"); });
            speedo_ = &make<Gauge>("Road speed", [this] {
                return ctx_.units.toUser(Quantity::speed, ctx_.live.get(telemetry::kCarSpeed));
            }, 0.0, 300.0);
            speedo_->setTicks(6, 2);
            speedo_->setClearNumerals(true);
            speedo_->setReadout([](double v) { return look::num(v, "%.0f"); });
            // The digits and the unit in the arc's open wedge, out of the needle's sweep at any value (the owner's rule).
            tach_->setReadoutClear(true);
            speedo_->setReadoutClear(true);
            tach_->setHelp("The engine's speed; the red band from its redline");
            speedo_->setHelp("The car's speed on the road");
            // CLASSIC's small dials (Dash.h's table): each reads its channel in the user's unit.
            for (int i = 0; i < dash::kDials; ++i)
            {
                const dash::Dial& d = dash::dial(i);
                Gauge& g = make<Gauge>(d.title, [this, i] {
                    const dash::Dial& dd = dash::dial(i);
                    return ctx_.units.toUser(dd.quantity, ctx_.live.get(dd.name));
                }, 0.0, 1.0);
                g.setTicks(4, 2);
                g.setClearNumerals(true);
                g.setReadoutClear(true);   // the digits out of the needle's sweep (the lead's ask, 25 Nov)
                g.setReadout([dec = d.decimals](double v) { return std::isfinite(v) ? formatNumber(v, dec) : std::string("-"); });
                g.setHelp(d.help);
                dials_[static_cast<std::size_t>(i)] = &g;
            }
            // The style's buttons (shown only where the shell keeps the choice).
            for (int s = 0; s < dash::kStyles; ++s)
            {
                Button& b = make<Button>(dash::styleName(s), [this, s] { setStyle(s); });
                b.setLit([this, s] { return style() == s; });
                b.setHelp(dash::styleHelp(s));
                styles_[static_cast<std::size_t>(s)] = &b;
            }
            a11yBase_ = nextId();   // the drawn dashes' readouts (accessibility()): past every item's ids
        }

        // The dash's style: the shell's choice where it keeps one, else this workspace's own.
        int style() const
        {
            const FrameServices* f = ctx_.services;
            const int s = f != nullptr && f->driveStyle ? f->driveStyle() : style_;
            return std::clamp(s, 0, dash::kStyles - 1);
        }
        void setStyle(int s)
        {
            style_ = std::clamp(s, 0, dash::kStyles - 1);
            if (ctx_.services != nullptr && ctx_.services->setDriveStyle)
                ctx_.services->setDriveStyle(style_);
            place(stage_);
        }
        // The buttons show where the shell serves the choice (a test's rig may serve it too).
        bool chooser() const { return ctx_.services != nullptr && ctx_.services->driveStyle && ctx_.services->setDriveStyle; }

        void accessibility(std::vector<funkgui::A11yItem>& out) const override
        {
            Board::accessibility(out);
            const int st = style();
            if (st != dash::kDigital && st != dash::kRace)
                return;
            // The drawn dash's readouts, as a screen reader hears them.
            const dash::Values v = dash::Values::read(ctx_.live);
            const Units& u = ctx_.units;
            uint32_t id = a11yBase_;
            const auto add = [&](std::string title, std::string value) {
                funkgui::A11yItem it;
                it.id = id++;
                it.role = funkgui::A11yRole::staticText;
                it.bounds = dash_;
                it.title = std::move(title);
                it.value = std::move(value);
                it.readOnly = true;
                out.push_back(std::move(it));
            };
            add("Gear", dash::gearText(v.gear));
            add("Engine", look::num(v.rpm, "%.0f") + " rpm" + (v.limiter ? ", on the limiter" : ""));
            add("Road speed", std::isfinite(v.speed) ? u.format(Quantity::speed, v.speed, 0) : "-");
            add("Shift lights", std::to_string(v.lights) + " of 5" + (v.shift ? ", shift" : ""));
            for (const dash::Channel& ch : dash::channels(v, u))
                add(ch.label, ch.value + (ch.unit.empty() ? "" : " " + ch.unit) + (ch.alarm != nullptr ? std::string(", ") + ch.alarm : ""));
            add("Torque", std::isfinite(v.torque) ? u.format(Quantity::torque, v.torque, 0) : "-");
            add("Power", std::isfinite(v.power) ? u.format(Quantity::power, v.power, 0) : "-");
            add("Longitudinal g", look::num(std::isfinite(v.accel) ? v.accel / 9.80665 : v.accel, "%+.2f"));
        }

        const char* id() const override { return "drive"; }
        const char* title() const override { return "DRIVE"; }
        std::vector<std::string> sections() const override { return { "env" }; }
        bool ribbon() const override { return false; }

        std::uint64_t drawKey() const override
        {
            DrawKey k;
            k.mix(static_cast<float>(ctx_.live.lambda()));
            for (const char* n : { "boost.pressure", "engine.coolant", "engine.oilTemperature",
                                   "engine.oilPressure", "car.gear", "car.speed", "car.flags", "car.stripState", "car.pitch" })
                k.mix(static_cast<float>(ctx_.live.get(n)));
            for (int i = 0; i < 4; ++i)
                k.mix(static_cast<float>(ctx_.live.get(idx("car.wheelSlip", i))));
            for (const auto& row : kSlipRows)
                k.mix(static_cast<float>(ctx_.live.get(row[0])));
            const ControlsFrame cf = ctx_.app.controls ? ctx_.app.controls() : ControlsFrame{};
            k.mix(cf.throttle).mix(cf.brake).mix(cf.clutch);
            k.mix(qsFlash_ > 0.0f ? 1 : 0).mix(blipFlash_ > 0.0f ? 1 : 0).mix(static_cast<float>(ctx_.live.get("car.bikeFlags")));
            const EngineBrakeState eb = engineBrakeState(ctx_);
            k.mix(eb.shown ? 1 : 0).mix(eb.stage).mix(eb.armed).mix(eb.exhaustOn ? 1 : 0).mix(eb.exhaust ? 1 : 0);
            k.mix(roll_).mix(squat_).mix(ctx_.selection.revision()).mix(ctx_.units.revision());
            if (const int st = style(); st != dash::kStandard)
            {
                k.mix(st);
                for (const char* n : { "engine.speed", "engine.redline", "engine.flags", "engine.torque", "engine.power",
                                       "exhaust.portTemperature", "ecu.fuelFlow", "ecu.knockRetard", "car.accel" })
                    k.mix(static_cast<float>(ctx_.live.get(n)));
            }
            return k.h;
        }

        void pointerDown(const funkgui::PointerEvent& e) override
        {
            const funkgui::Point at{ e.x, e.y };
            if (carHit_.contains(at) || tree_.contains(at))
                ctx_.selection.select("car");
            else if (scene_.contains(at))
                ctx_.selection.select("env");
            else
                Board::pointerDown(e);
        }

        // For tests: where things are.
        const funkgui::Rect& sceneRect() const noexcept { return scene_; }
        const funkgui::Rect& carRect() const noexcept { return carHit_; }
        const funkgui::Rect& treeRect() const noexcept { return tree_; }
        const Gauge& tach() const { return *tach_; }
        const Gauge& speedo() const { return *speedo_; }
        const Gauge& smallDial(int i) const { return *dials_[static_cast<std::size_t>(i)]; }
        const funkgui::Rect& dashRect() const noexcept { return dash_; }
        const Button& styleButton(int s) const { return *styles_[static_cast<std::size_t>(s)]; }

    protected:
        void layout(const funkgui::Rect& stage) override
        {
            const float pad = 16.0f;
            // The style's buttons at the top right, the rest under them.
            const bool chooser = this->chooser();
            float bx = stage.right() - pad;
            for (int i = dash::kStyles - 1; i >= 0; --i)
            {
                const float w = 74.0f;
                styles_[static_cast<std::size_t>(i)]->setPlace({ bx - w, stage.y + 8.0f, w, 24.0f }, chooser);
                bx -= w + 6.0f;
            }
            chooserX_ = bx;
            funkgui::Rect s = stage;
            if (chooser)
            {
                s.y += 34.0f;
                s.h -= 34.0f;
            }
            const int st = style();
            laidStyle_ = st;
            for (Gauge* d : dials_)
                d->setPlace({}, false);
            if (st != dash::kStandard)
            {
                layoutDash(s, st);
                return;
            }
            dash_ = {};
            const float topH = std::floor(s.h * 0.52f);
            const float side = std::clamp(s.w * 0.30f, 210.0f, 300.0f);
            const float g = std::floor(std::min(topH - 2.0f * pad, (s.w - side - 3.0f * pad) / 2.0f - 8.0f));
            const float gy = s.y + pad + (topH - 2.0f * pad - g) * 0.5f;
            const float gx = s.x + pad + std::max(0.0f, (s.w - side - 2.0f * pad - 2.0f * g - 16.0f) * 0.5f);
            tach_->setPlace({ gx, gy, g, g }, true);
            speedo_->setPlace({ gx + g + 16.0f, gy, g, g }, true);
            instruments_ = { s.right() - side - pad, s.y + pad, side, topH - 2.0f * pad };
            const float by = s.y + topH;
            const float stripW = std::clamp(s.w * 0.34f, 220.0f, 320.0f);
            scene_ = { s.x + pad, by, s.w - stripW - 3.0f * pad, s.bottom() - by - pad };
            strip_ = { scene_.right() + pad, by, stripW, scene_.h };
            // What a press hits: the car (as drawn at rest) and the tree.
            const Geometry g0 = geometry();
            carHit_ = { g0.x0, g0.roadY - g0.r * 3.6f, g0.x1 - g0.x0, g0.r * 3.6f };
            tree_ = { strip_.x, strip_.y, 110.0f, 26.0f + 6.0f * 19.0f + 12.0f };
        }

        // A CLASSIC small dial's least size (px, its square with the title): under it four dials, not eight. At this
        // size a face of 79 px leaves the digits and the unit's line room below the hub, clear of the needle.
        static constexpr float kSmallDial = 96.0f;

        // CLASSIC, DIGITAL, RACE: the dash across the stage's top (a little taller than STANDARD's), the car and the
        // strip under it as before.
        void layoutDash(const funkgui::Rect& s, int st)
        {
            const float pad = 16.0f;
            float topH = std::floor(s.h * 0.56f);
            dash_ = { s.x + pad, s.y + pad, s.w - 2.0f * pad, topH - pad - 8.0f };
            instruments_ = {};
            if (st == dash::kClassic)
            {
                // Eight small dials, two by two at either end, the tachometer and the speedometer between, when the
                // small ones have room for their digits clear of the needles (a face of kSmallDial); else four larger
                // ones, one above the other at either end: boost and oil pressure, water and lambda (the band's lamps
                // still say oil temperature, exhaust and knock). The band (gear, torque, power, g, lamps) under them.
                const float gap = 12.0f, bandH = 54.0f;
                const funkgui::Rect a{ dash_.x + 10.0f, dash_.y + 10.0f, dash_.w - 20.0f, dash_.h - 20.0f - bandH - 6.0f };
                float b = std::floor(std::min(a.h, (a.w - 3.0f * gap) / 4.0f)), m = std::floor((b - 6.0f) * 0.5f);
                const bool eight = m >= kSmallDial;
                if (!eight)
                {
                    b = std::floor(std::min(a.h, (a.w - 3.0f * gap + 6.0f) / 3.0f));
                    m = std::floor((b - 6.0f) * 0.5f);
                }
                const float total = eight ? 4.0f * b + 3.0f * gap : 2.0f * b + 2.0f * m + 3.0f * gap;
                const float x0 = a.x + std::floor((a.w - total) * 0.5f), y0 = a.y;
                const float bx0 = eight ? x0 + b + gap : x0 + m + gap;   // the tachometer's left
                const float right = bx0 + 2.0f * (b + gap);               // the right block's left
                for (int i = 0; i < dash::kDials; ++i)
                {
                    Gauge& d = *dials_[static_cast<std::size_t>(i)];
                    if (eight)
                    {
                        const int k = i % 4;
                        const float bxo = i < 4 ? x0 : right;
                        d.setPlace({ bxo + static_cast<float>(k % 2) * (m + 6.0f), y0 + static_cast<float>(k / 2) * (m + 6.0f), m, m }, true);
                        continue;
                    }
                    static constexpr int kFour[4] = { 0, 1, 3, 5 };   // boost, oil pressure | water, lambda
                    const int at = static_cast<int>(std::find(kFour, kFour + 4, i) - kFour);
                    if (at < 4)
                        d.setPlace({ at < 2 ? x0 : right, y0 + static_cast<float>(at % 2) * (m + 6.0f), m, m }, true);
                }
                const float x0b = x0;
                tach_->setPlace({ bx0, y0, b, b }, true);
                speedo_->setPlace({ bx0 + b + gap, y0, b, b }, true);
                band_ = { x0b, y0 + b + 10.0f, total, bandH };
                // The dash as tall as the cluster (the car's scene takes the rest).
                dash_.h = std::min(dash_.h, band_.bottom() + 10.0f - dash_.y);
                topH = dash_.bottom() + 8.0f + pad - s.y;
            }
            else
            {
                tach_->setPlace({}, false);
                speedo_->setPlace({}, false);
                band_ = {};
            }
            const float by = s.y + topH;
            const float stripW = std::clamp(s.w * 0.34f, 220.0f, 320.0f);
            scene_ = { s.x + pad, by, s.w - stripW - 3.0f * pad, s.bottom() - by - pad };
            strip_ = { scene_.right() + pad, by, stripW, scene_.h };
            const Geometry g0 = geometry();
            carHit_ = { g0.x0, g0.roadY - g0.r * 3.6f, g0.x1 - g0.x0, g0.r * 3.6f };
            tree_ = { strip_.x, strip_.y, 110.0f, 26.0f + 6.0f * 19.0f + 12.0f };
        }

        // The scene's measures: the road's height, a metre in px, the wheels' radius and places, the body's ends.
        struct Geometry
        {
            funkgui::Rect inner;
            float roadY, scale, r, wb, cx, fx, rx, wy, x0, x1;
        };
        Geometry geometry() const
        {
            Geometry g;
            g.inner = { scene_.x + look::kPad, scene_.y + 26.0f, std::max(0.0f, scene_.w - 2 * look::kPad),
                        std::max(0.0f, scene_.h - 26.0f - look::kPad) };
            g.roadY = g.inner.bottom() - 44.0f;
            g.scale = std::clamp(std::min(g.inner.w / 6.0f, (g.inner.h - 60.0f) / 2.4f), 24.0f, 90.0f);   // px a metre
            g.r = 0.33f * g.scale;
            g.wb = 2.7f * g.scale;
            g.cx = g.inner.centreX();
            g.fx = g.cx + g.wb * 0.5f;
            g.rx = g.cx - g.wb * 0.5f;
            g.wy = g.roadY - g.r;
            const float len = g.wb * 1.62f;
            g.x0 = g.cx - len * 0.52f;
            g.x1 = g.cx + len * 0.48f;
            return g;
        }

        void step(float dt) override
        {
            // The car's own motion: the wheels roll by their speed (or the road's), the marks pass, the body squats with
            // the load moved to the rear (car.pitch, else the acceleration seen), all eased.
            const double v = ctx_.live.get(telemetry::kCarSpeed);
            const double speed = std::isfinite(v) ? v : 0.0;
            const double r = 0.33;
            double wheel = ctx_.live.get(idx("car.wheelSpeed", 2));
            if (!std::isfinite(wheel))
                wheel = speed / r;
            spin_ = static_cast<float>(std::fmod(spin_ + wheel * dt, 2.0 * kPi));
            roll_ = static_cast<float>(std::fmod(roll_ + speed * dt, 4.0));
            const double pitch = ctx_.live.get("car.pitch");
            double accel = dt > 0.0f ? (speed - lastSpeed_) / dt : 0.0;
            lastSpeed_ = speed;
            accel = std::clamp(accel, -12.0, 12.0);
            const double target = std::isfinite(pitch) ? std::clamp(pitch / 3000.0, -1.0, 1.0) : std::clamp(accel / 8.0, -1.0, 1.0);
            squat_ = funkgui::ease::toward(squat_, static_cast<float>(target), dt, 0.25f, 1e-3f);
            // The bike's counters: a flash on each increase.
            const double qs = ctx_.live.get("car.quickshifts"), bl = ctx_.live.get("car.blips");
            qsFlash_ = std::isfinite(qs) && qs != lastQs_ && std::isfinite(lastQs_) ? 0.3f : std::max(0.0f, qsFlash_ - dt);
            blipFlash_ = std::isfinite(bl) && bl != lastBlips_ && std::isfinite(lastBlips_) ? 0.3f : std::max(0.0f, blipFlash_ - dt);
            lastQs_ = qs;
            lastBlips_ = bl;
            // The tachometer's span follows the redline.
            const double red = ctx_.live.get(telemetry::kRedline) * kToRpm;
            if (std::isfinite(red) && red > 1000.0)
            {
                const look::RpmScale sc = look::rpmScale(red);   // the cockpit's rev bar's scale
                tach_->setRange(0.0, sc.top);
                tach_->setTicks(static_cast<int>(std::lround(sc.top / sc.step)), 2);
                tach_->setRedline(red);
                if (sc.divisor != tachDivisor_)
                {
                    tachDivisor_ = sc.divisor;
                    const double d = sc.divisor;
                    tach_->setNumerals([d](double v) { return std::to_string(static_cast<int>(std::lround(v / d))); });
                    tach_->setUnit(sc.unit);
                }
            }
            // The speedometer in the user's unit, to a round top.
            const double vmax = ctx_.units.toUser(Quantity::speed, 300.0 / 3.6);
            const double every = vmax > 200.0 ? 40.0 : 20.0;
            speedo_->setRange(0.0, std::ceil(vmax / every) * every);
            speedo_->setTicks(static_cast<int>(std::ceil(vmax / every)), 2);
            speedo_->setUnit(upper(ctx_.units.unit(Quantity::speed).name));
            // The small dials' spans in the user's units, re-chosen there to round steps (the owner's ask, 25 Nov: 0 25 50
            // 75 100 psi, not 0 25 51 76 102), two or four steps, a number at the ends and the middle only (a face too
            // small for the ends keeps the middle's: Gauge::setClearNumerals).
            for (int i = 0; i < dash::kDials; ++i)
            {
                const dash::Dial& d = dash::dial(i);
                Gauge& g = *dials_[static_cast<std::size_t>(i)];
                const dash::Scale sc = dash::roundScale(ctx_.units.toUser(d.quantity, d.lo), ctx_.units.toUser(d.quantity, d.hi), 4, true);
                g.setRange(sc.lo, sc.hi);
                g.setTicks(sc.steps, 2);
                g.setNumeralStride(sc.steps == 4 ? 2 : 1);   // the ends and the middle
                g.setRedline(std::isfinite(d.red) ? ctx_.units.toUser(d.quantity, d.red) : d.red);
                g.setUnit(ctx_.units.unit(d.quantity).name);
                g.setNumerals([](double v) { return look::num(std::fabs(v) < 1e-9 ? 0.0 : v, "%g"); });
            }
            // The shell's choice moved (another window, a reload): lay out again.
            if (style() != laidStyle_ && !stage_.isEmpty())
                place(stage_);
        }

        void drawStage(funkgui::Canvas& c, const funkgui::Theme& th) const override
        {
            const look::Palette p = look::Palette::of(th);
            if (chooser())
                c.text("DASH", chooserX_, stage_.y + 15.0f, look::kCaps, p.label, funkgui::Align::right);
            const int st = style();
            if (st == dash::kStandard)
                drawInstruments(c, p);
            else
            {
                const dash::Values v = dash::Values::read(ctx_.live);
                if (st == dash::kClassic)
                {
                    c.rrect(dash_.x, dash_.y, dash_.w, dash_.h, look::kRadius, p.panel, 1.0f, p.edge);
                    dash::drawClassicBand(c, p, band_, v, ctx_.units);
                }
                else if (st == dash::kDigital)
                    dash::drawDigital(c, p, dash_, v, ctx_.units);
                else
                    dash::drawRace(c, p, dash_, v, ctx_.units);
            }
            drawScene(c, p);
            drawStrip(c, p);
        }

    private:
        void drawInstruments(funkgui::Canvas& c, const look::Palette& p) const
        {
            const funkgui::Rect in = look::region(c, p, instruments_, "INSTRUMENTS");
            const Units& u = ctx_.units;
            const auto val = [&](const char* name, Quantity q, int dec = -1) {
                const double v = ctx_.live.get(name);
                return std::isfinite(v) ? u.number(q, v, dec) : std::string("-");
            };
            const float colW = in.w * 0.5f, rowH = 46.0f;
            // Boost, or vacuum under a closed throttle (the word says which).
            const double boost = ctx_.live.get(telemetry::kBoost);
            look::readout(c, p, in.x, in.y, std::isfinite(boost) && boost < 0.0 ? "VACUUM" : "BOOST",
                          std::isfinite(boost) ? u.number(Quantity::pressure, std::fabs(boost)) : "-", u.unit(Quantity::pressure).name);
            const double lambda = ctx_.live.lambda();
            const bool lean = std::isfinite(lambda) && lambda > 1.08;
            look::readout(c, p, in.x + colW, in.y, lean ? "LAMBDA  LEAN" : "LAMBDA", look::num(lambda, "%.2f"), {}, look::kNumberM,
                          lean ? p.warning : look::kClear);
            const double coolant = ctx_.live.get(telemetry::kCoolant);
            const bool hot = std::isfinite(coolant) && coolant > 383.15;
            look::readout(c, p, in.x, in.y + rowH, hot ? "COOLANT  HOT" : "COOLANT", val("engine.coolant", Quantity::temperature, 0),
                          u.unit(Quantity::temperature).name, look::kNumberM, hot ? p.warning : look::kClear);
            look::readout(c, p, in.x + colW, in.y + rowH, "OIL", val("engine.oilTemperature", Quantity::temperature, 0),
                          u.unit(Quantity::temperature).name);
            look::readout(c, p, in.x, in.y + 2.0f * rowH, "OIL PRESSURE", val("engine.oilPressure", Quantity::pressure),
                          u.unit(Quantity::pressure).name);
            // The gear, large.
            const double g = ctx_.live.get(telemetry::kGear);
            const std::string gear = !std::isfinite(g) ? "-" : g < -0.5 ? "R" : g < 0.5 ? "N" : std::to_string(std::lround(g));
            look::readout(c, p, in.x + colW, in.y + 2.0f * rowH, "GEAR", gear, {}, look::kNumberL);
            // The pedals as the car has them: three levels with their words.
            const ControlsFrame cf = ctx_.app.controls ? ctx_.app.controls() : ControlsFrame{};
            const float py = in.y + 3.0f * rowH + 10.0f;
            if (py + 40.0f <= in.bottom() + 8.0f)
            {
                const float lw = (in.w - 2.0f * 10.0f) / 3.0f;
                const std::array<std::pair<const char*, float>, 3> pedals{ { { "THROTTLE", cf.throttle }, { "BRAKE", cf.brake },
                                                                             { "CLUTCH", cf.clutch } } };
                for (std::size_t k = 0; k < 3; ++k)
                {
                    const float x = in.x + static_cast<float>(k) * (lw + 10.0f);
                    c.text(pedals[k].first, x, py, look::kCaps, p.label);
                    look::level(c, p, { x, py + 16.0f, lw, 6.0f }, pedals[k].second, k == 1 ? p.hotter : p.value);
                }
            }
        }

        void drawScene(funkgui::Canvas& c, const look::Palette& p) const
        {
            const funkgui::Rect s = look::region(c, p, scene_, bike() ? "THE BIKE" : "THE CAR", ctx_.selection.is("env"));
            const funkgui::Canvas::ClipScope clip(c, scene_.reduced(1.0f));
            const Geometry gm = geometry();
            // The road: a line, and a mark every metre that passes at the road's speed.
            const float roadY = gm.roadY, scale = gm.scale;
            c.segment(s.x, roadY, s.right(), roadY, 2.0f, p.text);
            for (float m = -roll_ * scale; m < s.w; m += scale)
                if (m >= 0.0f)
                    c.segment(s.x + m, roadY + 8.0f, s.x + m + scale * 0.45f, roadY + 8.0f, 2.0f, p.dim);
            if (bike())
                drawBike(c, p, s, gm);
            else
                drawCar(c, p, s, gm);
        }

        // A motorcycle (spec.car.kind "Motorcycle", card B2): spec.car.kind's choice names it.
        bool bike() const
        {
            const int f = ctx_.doc.find("car.kind");
            if (f < 0 || ctx_.doc.info(f).kind != FieldKind::choice)
                return false;
            const auto k = static_cast<std::size_t>(std::lround(ctx_.doc.get(f)));
            return k < ctx_.doc.info(f).choices.size() && ctx_.doc.info(f).choices[k] == "Motorcycle";
        }

        // A wheel: a tyre ring (amber while it spins), a rim, five spokes turned by the wheel's angle.
        void drawWheel(funkgui::Canvas& c, const look::Palette& p, float x, float y, float r, double slip) const
        {
            const bool spinning = std::isfinite(slip) && std::fabs(slip) > 0.15;
            c.disc(x, y, r, p.over(p.faint, 0.6f), 4.0f, spinning ? p.warning : p.text);
            c.disc(x, y, r * 0.55f, p.panel, 1.25f, p.label);
            for (int k = 0; k < 5; ++k)
            {
                const float a = spin_ + static_cast<float>(k) * 1.2566371f;
                c.segment(x, y, x + std::cos(a) * r * 0.5f, y + std::sin(a) * r * 0.5f, 1.25f, p.label);
            }
            if (spinning)
                c.text("SPIN", x - r - 8.0f, y - 5.0f, look::kCaps, p.warning, funkgui::Align::right);
        }

        // The wheels' slip as bars along the bottom (amber past 15 %, with the word): `which` of car.wheelSlip[i].
        void drawSlips(funkgui::Canvas& c, const look::Palette& p, const funkgui::Rect& s, std::initializer_list<int> which,
                       const char* const* names) const
        {
            const auto n = static_cast<float>(which.size());
            const float bw = (s.w - (n - 1.0f) * 12.0f) / n, by = s.bottom() - 14.0f;
            int k = 0;
            for (const int i : which)
            {
                const double sl = ctx_.live.get(idx("car.wheelSlip", i));
                const float x = s.x + static_cast<float>(k) * (bw + 12.0f);
                const bool over = std::isfinite(sl) && std::fabs(sl) > 0.15;
                c.text((std::string(names[k]) + " SLIP " + (std::isfinite(sl) ? look::num(sl * 100.0, "%.0f") + "%" : "-")).c_str(),
                       x, by - 15.0f, look::kCaps, over ? p.warning : p.label);
                look::level(c, p, { x, by, bw, 6.0f }, std::isfinite(sl) ? std::fabs(sl) / 0.5 : std::nan(""), over ? p.warning : p.text,
                            0.3);
                ++k;
            }
        }

        void drawCar(funkgui::Canvas& c, const look::Palette& p, const funkgui::Rect& s, const Geometry& gm) const
        {
            // The car: wheels 2.7 m apart, radius 0.33 m; the body squats at the rear when it pulls.
            const float r = gm.r, fx = gm.fx, rx = gm.rx, wy = gm.wy;
            const float sq = squat_ * 7.0f;                     // the rear down (px), the front up by half
            const funkgui::Col body = ctx_.selection.is("car") ? p.selected : p.value;
            // The body: Look's car profile (CAR draws the same one), front at the right.
            float xs[look::kCarPoints], ys[look::kCarPoints];
            look::carProfile(fx, rx, wy, r, sq, xs, ys);
            c.polyline(xs, ys, look::kCarPoints, 2.0f, body);
            float wx0, wy0, wx1, wy1;
            look::carWindow(fx, rx, wy, r, sq, wx0, wy0, wx1, wy1);
            c.segment(wx0, wy0, wx1, wy1, 1.25f, p.label);
            const double slipR = std::max(std::fabs(ctx_.live.get(idx("car.wheelSlip", 2))), std::fabs(ctx_.live.get(idx("car.wheelSlip", 3))));
            const double slipF = std::max(std::fabs(ctx_.live.get(idx("car.wheelSlip", 0))), std::fabs(ctx_.live.get(idx("car.wheelSlip", 1))));
            drawWheel(c, p, rx, wy, r, slipR);
            drawWheel(c, p, fx, wy, r, slipF);
            drawSlips(c, p, s, { 0, 1, 2, 3 }, kWheels);
            // A truck's engine brake (card D2), top left: its lamp, the stage as segments and in a word, the exhaust
            // brake's lamp.
            if (const EngineBrakeState eb = engineBrakeState(ctx_); eb.shown)
                drawEngineBrake(c, p, s.x, s.y, eb, true);
        }

        void drawBike(funkgui::Canvas& c, const look::Palette& p, const funkgui::Rect& s, const Geometry& gm) const
        {
            // The bike from the side, front at the right: its wheelbase (car.wheelbase, else 1.45 m) and tyres (0.31 m);
            // the whole bike turned about the rear tyre's contact by car.liftAngle (degrees, the front's lift: a
            // wheelie), the points turned by hand (no rotation on the Canvas).
            const int wbField = ctx_.doc.find("car.wheelbase");
            const float wbM = wbField >= 0 ? static_cast<float>(ctx_.doc.get(wbField)) : 1.45f;
            const float scale = std::min(gm.scale * 1.35f, (gm.roadY - gm.inner.y - 46.0f) / 1.75f);   // the rider's head in room
            const double lift = ctx_.live.get("car.liftAngle");
            const float th = std::isfinite(lift) ? static_cast<float>(std::clamp(lift, -15.0, 80.0) * kPi / 180.0) : 0.0f;
            const float px = gm.cx - wbM * scale * 0.5f, py = gm.roadY;   // the rear tyre's contact
            const int tuckField = ctx_.doc.find("car.tuck"), radiusField = ctx_.doc.find("car.tyreRadius");
            const double tuck = tuckField >= 0 ? ctx_.doc.get(tuckField) : 1.0;
            const double rM = radiusField >= 0 ? ctx_.doc.get(radiusField) : 0.31;
            const float k = scale;   // px a metre
            const auto lines = look::bikeLines(px, py, k, wbM, rM, th * 180.0 / kPi, tuck, 1.0f);
            const funkgui::Col frame = ctx_.selection.is("car") ? p.selected : p.value;
            for (std::size_t i = 0; i + 1 < lines.size(); ++i)
            {
                std::vector<float> xs, ys;
                for (const funkgui::Point& q : lines[i])
                {
                    xs.push_back(q.x);
                    ys.push_back(q.y);
                }
                c.polyline(xs.data(), ys.data(), static_cast<int>(xs.size()), i == 0 || i == 4 ? 2.5f : 2.0f,
                           i >= 5 ? p.text : frame);   // the rider in the text's ink, the bike in the value's
            }
            const funkgui::Point rear = lines[0][0], front = lines.back()[0], head = lines.back()[1];
            c.disc(head.x, head.y, static_cast<float>(0.13 * k), look::kClear, 2.0f, p.text);
            const float r = static_cast<float>(rM) * k;
            drawWheel(c, p, rear.x, rear.y, r, ctx_.live.get(idx("car.wheelSlip", 2)));
            drawWheel(c, p, front.x, front.y, r, ctx_.live.get(idx("car.wheelSlip", 0)));
            // A wheelie (or the control holding one) and a stoppie say so in words: car.bikeFlags 4 and 64, car.flags 2.
            const double bf = ctx_.live.get("car.bikeFlags"), cf = ctx_.live.get("car.flags");
            const long bits = std::isfinite(bf) ? std::lround(bf) : 0, cbits = std::isfinite(cf) ? std::lround(cf) : 0;
            const char* word = (bits & 64) != 0 ? "STOPPIE" : (bits & 4) != 0 ? "WHEELIE CONTROL" : ((cbits & 2) != 0 || th > 0.035f) ? "WHEELIE" : nullptr;
            if (word != nullptr)
                c.text((std::string(word) + "  " + look::num(lift, "%.0f") + " DEG").c_str(), s.x, s.y, look::kCaps, p.warning);
            // The rest of car.bikeFlags as words in a row: a quickshift's cut and an auto-blip held 0.3 s past their
            // counters' increase (car.quickshifts, car.blips; a 50 ms bit is easily missed between frames), the slipper
            // clutch slipping, the rear hopping.
            float wx = s.x;
            const auto flag = [&](const char* text, bool on, funkgui::Col ink) {
                wx += look::lamp(c, p, wx, s.y + 22.0f, text, on, ink) + 14.0f;
            };
            flag("QUICKSHIFT", (bits & 16) != 0 || qsFlash_ > 0.0f, p.ok);
            flag("BLIP", (bits & 32) != 0 || blipFlash_ > 0.0f, p.ok);
            flag("SLIPPER", (bits & 8) != 0, p.warning);
            flag("REAR HOP", (bits & 1) != 0, p.warning);
            static const char* const kBikeWheels[2] = { "FRONT", "REAR" };
            drawSlips(c, p, s, { 0, 2 }, kBikeWheels);
        }

        void drawStrip(funkgui::Canvas& c, const look::Palette& p) const
        {
            const funkgui::Rect s = look::region(c, p, strip_, "THE STRIP", false);
            // The tree: two staged lamps, three ambers, the green, the red; lit by car.stripState (0 off, 1 staged,
            // 2 ambers, 3 green, 4 running, 5 finished, 6 red light). Each lamp has its word: colour is not the only sign.
            const double st = ctx_.live.get("car.stripState");
            const int state = std::isfinite(st) ? static_cast<int>(std::lround(st)) : -1;
            const float tx = s.x + 6.0f, gap = 19.0f;
            struct Lamp { const char* word; funkgui::Col ink; bool on; };
            const Lamp lamps[] = {
                { "STAGED", p.text, state >= 1 && state != 6 }, { "AMBER", p.warning, state == 2 },
                { "AMBER", p.warning, state == 2 },             { "AMBER", p.warning, state == 2 },
                { "GREEN", p.ok, state == 3 || state == 4 },    { "RED", p.hotter, state == 6 },
            };
            float y = s.y + 6.0f;
            for (const Lamp& l : lamps)
            {
                look::lamp(c, p, tx, y, l.word, l.on, l.ink);
                y += gap;
            }
            if (state < 0)
                c.text("NO STRIP YET", tx, y + 2.0f, look::kCaps, p.dim);
            // The time slip.
            const float sx = s.x + 104.0f, sw = s.right() - sx;
            c.text("TIME SLIP", sx, s.y, look::kCaps, p.label);
            float ry = s.y + 18.0f;
            for (const auto& row : kSlipRows)
            {
                if (ry + 14.0f > s.bottom())
                    break;
                const double v = ctx_.live.get(row[0]);
                const bool speed = row[0][6] == 'v';
                const std::string text = !std::isfinite(v) ? "-" : speed ? ctx_.units.format(Quantity::speed, v, 1) : look::num(v, "%.3f") + " s";
                c.text(row[1], sx, ry + 2.0f, look::kUnit, p.label);
                c.text(text.c_str(), sx + sw, ry, look::kNumberS, std::isfinite(v) ? p.value : p.dim, funkgui::Align::right);
                c.hairlineH(sx, c.snapY(ry + 17.0f), sw, p.grid);
                ry += 20.0f;
            }
        }

        Gauge *tach_ = nullptr, *speedo_ = nullptr;
        std::array<Gauge*, dash::kDials> dials_{};
        std::array<Button*, dash::kStyles> styles_{};
        int style_ = dash::kStandard, laidStyle_ = -1;
        uint32_t a11yBase_ = 0;
        float chooserX_ = 0.0f;
        funkgui::Rect dash_{}, band_{};
        funkgui::Rect instruments_{}, scene_{}, strip_{};
        funkgui::Rect carHit_{}, tree_{};
        float spin_ = 0.0f, roll_ = 0.0f, squat_ = 0.0f, qsFlash_ = 0.0f, blipFlash_ = 0.0f;
        double lastQs_ = std::nan(""), lastBlips_ = std::nan("");
        double lastSpeed_ = 0.0;
        double tachDivisor_ = 1000.0;   // the tachometer's numerals: thousands, or hundreds under a 4000 rpm redline
    };

    std::unique_ptr<Workspace> makeDriveWorkspace(WorkspaceContext& c) { return std::make_unique<DriveWorkspace>(c); }
}
