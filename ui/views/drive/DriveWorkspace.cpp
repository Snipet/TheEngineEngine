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
#include "app/Look.h"
#include "app/Workspace.h"
#include "kit/widgets/Gauge.h"

#include <algorithm>
#include <array>
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
            tach_->setNumerals([](double v) { return std::to_string(static_cast<int>(std::lround(v / 1000.0))); });
            tach_->setUnit("X1000 RPM");
            tach_->setReadout([](double v) { return look::num(v, "%.0f"); });
            speedo_ = &make<Gauge>("Road speed", [this] {
                return ctx_.units.toUser(Quantity::speed, ctx_.live.get(telemetry::kCarSpeed));
            }, 0.0, 300.0);
            speedo_->setTicks(6, 2);
            speedo_->setReadout([](double v) { return look::num(v, "%.0f"); });
        }

        const char* id() const override { return "drive"; }
        const char* title() const override { return "DRIVE"; }
        std::vector<std::string> sections() const override { return { "env" }; }
        bool ribbon() const override { return false; }

        std::uint64_t drawKey() const override
        {
            DrawKey k;
            for (const char* n : { "boost.pressure", "engine.lambda", "engine.coolant", "engine.oilTemperature",
                                   "engine.oilPressure", "car.gear", "car.speed", "car.flags", "car.stripState", "car.pitch" })
                k.mix(static_cast<float>(ctx_.live.get(n)));
            for (int i = 0; i < 4; ++i)
                k.mix(static_cast<float>(ctx_.live.get(idx("car.wheelSlip", i))));
            for (const auto& row : kSlipRows)
                k.mix(static_cast<float>(ctx_.live.get(row[0])));
            const ControlsFrame cf = ctx_.app.controls ? ctx_.app.controls() : ControlsFrame{};
            k.mix(cf.throttle).mix(cf.brake).mix(cf.clutch);
            k.mix(roll_).mix(squat_).mix(ctx_.selection.revision()).mix(ctx_.units.revision());
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

    protected:
        void layout(const funkgui::Rect& s) override
        {
            const float pad = 16.0f;
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
            // The tachometer's span follows the redline.
            const double red = ctx_.live.get(telemetry::kRedline) * kToRpm;
            if (std::isfinite(red) && red > 1000.0)
            {
                const double top = std::ceil((red + 1000.0) / 1000.0) * 1000.0;
                tach_->setRange(0.0, top);
                tach_->setTicks(static_cast<int>(top / 1000.0), 2);
                tach_->setRedline(red);
            }
            // The speedometer in the user's unit, to a round top.
            const double vmax = ctx_.units.toUser(Quantity::speed, 300.0 / 3.6);
            const double every = vmax > 200.0 ? 40.0 : 20.0;
            speedo_->setRange(0.0, std::ceil(vmax / every) * every);
            speedo_->setTicks(static_cast<int>(std::ceil(vmax / every)), 2);
            speedo_->setUnit(upper(ctx_.units.unit(Quantity::speed).name));
        }

        void drawStage(funkgui::Canvas& c, const funkgui::Theme& th) const override
        {
            const look::Palette p = look::Palette::of(th);
            drawInstruments(c, p);
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
            const double lambda = ctx_.live.get("engine.lambda");
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
            const funkgui::Rect s = look::region(c, p, scene_, "THE CAR", ctx_.selection.is("env"));
            const funkgui::Canvas::ClipScope clip(c, scene_.reduced(1.0f));
            const Geometry gm = geometry();
            // The road: a line, and a mark every metre that passes at the road's speed.
            const float roadY = gm.roadY, scale = gm.scale;
            c.segment(s.x, roadY, s.right(), roadY, 2.0f, p.text);
            for (float m = -roll_ * scale; m < s.w; m += scale)
                if (m >= 0.0f)
                    c.segment(s.x + m, roadY + 8.0f, s.x + m + scale * 0.45f, roadY + 8.0f, 2.0f, p.dim);
            // The car: wheels 2.7 m apart, radius 0.33 m; the body squats at the rear when it pulls.
            const float r = gm.r, wb = gm.wb, cx = gm.cx, fx = gm.fx, rx = gm.rx, wy = gm.wy, x0 = gm.x0, x1 = gm.x1;
            const float sq = squat_ * 7.0f;                     // the rear down (px), the front up by half
            const float rideF = wy - r * 0.55f - sq * 0.5f, rideR = wy - r * 0.55f + sq;
            const funkgui::Col body = ctx_.selection.is("car") ? p.selected : p.value;
            // The body as a profile: sill, nose, bonnet, screen, roof, back, tail (a polyline, opaque).
            const auto yAt = [&](float x) { return rideR + (rideF - rideR) * (x - rx) / (fx - rx); };
            const float bodyH = r * 1.25f, roofH = r * 2.35f;
            const float xs[] = { x0, x0 - r * 0.1f, x0 + r * 0.3f, cx - wb * 0.28f, cx - wb * 0.05f, cx + wb * 0.22f,
                                 cx + wb * 0.42f, x1 - r * 0.2f, x1, x1 - r * 0.05f, x0 };
            const float ys[] = { yAt(x0), yAt(x0) - bodyH * 0.9f, yAt(x0 + r * 0.3f) - bodyH,
                                 yAt(cx - wb * 0.28f) - roofH, yAt(cx - wb * 0.05f) - roofH, yAt(cx + wb * 0.22f) - bodyH * 1.02f,
                                 yAt(cx + wb * 0.42f) - bodyH, yAt(x1 - r * 0.2f) - bodyH * 0.75f, yAt(x1) - bodyH * 0.25f,
                                 yAt(x1), yAt(x0) };
            c.polyline(xs, ys, 11, 2.0f, body);
            // The windows' line.
            c.segment(cx - wb * 0.25f, yAt(cx - wb * 0.25f) - roofH + 6.0f, cx + wb * 0.18f, yAt(cx + wb * 0.18f) - bodyH - 4.0f,
                      1.25f, p.label);
            // Wheels: a tyre ring, a rim, five spokes turned by the wheel's angle; a spinning wheel's ring in amber.
            const double slipR = std::max(std::fabs(ctx_.live.get(idx("car.wheelSlip", 2))), std::fabs(ctx_.live.get(idx("car.wheelSlip", 3))));
            const double slipF = std::max(std::fabs(ctx_.live.get(idx("car.wheelSlip", 0))), std::fabs(ctx_.live.get(idx("car.wheelSlip", 1))));
            for (const auto& [x, slip] : { std::pair{ rx, slipR }, std::pair{ fx, slipF } })
            {
                const bool spinning = std::isfinite(slip) && slip > 0.15;
                c.disc(x, wy, r, p.over(p.faint, 0.6f), 4.0f, spinning ? p.warning : p.text);
                c.disc(x, wy, r * 0.55f, p.panel, 1.25f, p.label);
                for (int k = 0; k < 5; ++k)
                {
                    const float a = spin_ + static_cast<float>(k) * 1.2566371f;
                    c.segment(x, wy, x + std::cos(a) * r * 0.5f, wy + std::sin(a) * r * 0.5f, 1.25f, p.label);
                }
                if (spinning)
                    c.text("SPIN", x - r - 8.0f, wy - 5.0f, look::kCaps, p.warning, funkgui::Align::right);
            }
            // The wheels' slip as bars along the bottom: amber past 15 %, with the word.
            const float bw = (s.w - 3.0f * 12.0f) / 4.0f, by = s.bottom() - 14.0f;
            for (int i = 0; i < 4; ++i)
            {
                const double sl = ctx_.live.get(idx("car.wheelSlip", i));
                const float x = s.x + static_cast<float>(i) * (bw + 12.0f);
                const bool over = std::isfinite(sl) && std::fabs(sl) > 0.15;
                c.text((std::string(kWheels[i]) + " SLIP " + (std::isfinite(sl) ? look::num(sl * 100.0, "%.0f") + "%" : "-")).c_str(),
                       x, by - 15.0f, look::kCaps, over ? p.warning : p.label);
                look::level(c, p, { x, by, bw, 6.0f }, std::isfinite(sl) ? std::fabs(sl) / 0.5 : std::nan(""), over ? p.warning : p.text,
                            0.3);
            }
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
        funkgui::Rect instruments_{}, scene_{}, strip_{};
        funkgui::Rect carHit_{}, tree_{};
        float spin_ = 0.0f, roll_ = 0.0f, squat_ = 0.0f;
        double lastSpeed_ = 0.0;
    };

    std::unique_ptr<Workspace> makeDriveWorkspace(WorkspaceContext& c) { return std::make_unique<DriveWorkspace>(c); }
}
