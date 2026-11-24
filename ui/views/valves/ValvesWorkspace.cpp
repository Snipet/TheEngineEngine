// ui/views/valves/ValvesWorkspace.cpp: the VALVES workspace (docs/ui/valves.md).
//
// Its stage:
//   CAM EDITOR      both lifts against the crank's angle over the cycle, overlap shaded with its angle in degrees, TDC
//                   and BDC marked, the piston's room above it ghosted (how near a valve comes to it at overlap), the
//                   phasers' ranges as bands with their live place, a second stage ghosted behind. Handles: each
//                   cam's peak (drag across: its centreline; up and down: its lift) and its seat events (drag: its
//                   duration about the centreline). Each drag is one step of undo, written through the document.
//   TIMING          the classic diagram: each valve's open period as an arc round TDC and BDC, IVO, IVC, EVO, EVC.
//   FLOW BENCH      flow against lift for each valve's head (bench.fm), the lift now marked on each.
//   PHASERS AND STAGES   the ranges and where they are now; which stage is engaged; the continuous lift.
//   LIVE            valve float's margin; the cylinders resting.
// Time only from step(dt); drawing changes nothing.
#include "views/valves/ValvesWorkspace.h"

#include "views/engine/EngineModel.h"
#include "views/engine/PageText.h"
#include "views/engine/StageDraw.h"
#include "views/valves/ValvesModel.h"

#include "app/Look.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

namespace tee::ui
{
    namespace
    {
        using funkgui::Align;
        using funkgui::Canvas;
        using funkgui::Col;
        using funkgui::Point;
        using funkgui::Rect;
        using namespace valves;
        using stage::put;
        using stage::putCentred;
        namespace lk = look;
        namespace en = engine;

        constexpr float kGap = 12.0f;
        constexpr double kCfm = 2118.88;   // m^3/s in cubic feet a minute

        Rect content(const Rect& r) { return { r.x + lk::kPad, r.y + 26.0f, r.w - 2 * lk::kPad, r.h - 26.0f - lk::kPad }; }
        std::string deg(double v) { return lk::num(v, "%.0f"); }

        enum Handle : int { inPeak, inOpen, inClose, exPeak, exOpen, exClose, handleCount };
    }

    class ValvesWorkspace final : public Workspace
    {
    public:
        explicit ValvesWorkspace(WorkspaceContext& c) : Workspace(c) {}

        const char* id() const override { return "valves"; }
        const char* title() const override { return "VALVES"; }
        std::vector<std::string> sections() const override { return { "valve" }; }
        bool moving() const override { return drag_ >= 0 || Workspace::moving(); }

        std::uint64_t drawKey() const override
        {
            DrawKey k;
            k.mix(ctx_.doc.revision()).mix(ctx_.selection.revision()).mix(ctx_.units.revision());
            const double a = crankAngle();
            k.mix(std::isfinite(a) ? static_cast<std::int64_t>(std::lround(a * 2.0)) : -1);
            for (const char* n : { kTelInPhase, kTelExPhase, kTelStage, kTelLift, kTelFloat, kTelActive, en::kTelSpeed })
                k.mix(static_cast<float>(ctx_.live.get(n)));
            k.mix(drag_).mix(hot_).mix(static_cast<float>(yMax_));
            return k.h;
        }

        // The inspector for what is selected: a cam (both its stages and its phaser), the timing (both cams and the
        // model), the head (the bench's), the phasers and stages, the springs and deactivation. Nothing selected: the
        // generated pages of `valve`.
        std::string inspectorPage() const override
        {
            const std::string& s = ctx_.selection.path();
            // the sections, each field's label short (its section names the valve): the order a cam card reads in
            const auto sec = [&](const char* title, std::initializer_list<VF> l, std::vector<std::string> labels) {
                pages::Section s;
                s.title = title;
                std::size_t k = 0;
                for (VF x : l)
                {
                    if (std::string q = en::fieldPath(ctx_.doc, path(x)); !q.empty())
                    {
                        s.paths.push_back(std::move(q));
                        s.labels.push_back(k < labels.size() ? labels[k] : std::string());
                    }
                    ++k;
                }
                return s;
            };
            const std::vector<std::string> cam = { "ACTUATION", "DURATION, SEAT", "DURATION, 1 MM", "LIFT", "CENTRELINE", "FATNESS" };
            const std::vector<std::string> camB = { "DURATION, SEAT", "DURATION, 1 MM", "LIFT", "CENTRELINE" };
            const pages::Section inA = sec("INTAKE CAM", { VF::inKind, VF::inDurSeat, VF::inDur1mm, VF::inLift, VF::inCentre, VF::inFat }, cam);
            const pages::Section exA = sec("EXHAUST CAM", { VF::exKind, VF::exDurSeat, VF::exDur1mm, VF::exLift, VF::exCentre, VF::exFat }, cam);
            const pages::Section inB = sec("INTAKE, STAGE B", { VF::inDurSeatB, VF::inDur1mmB, VF::inLiftB, VF::inCentreB }, camB);
            const pages::Section exB = sec("EXHAUST, STAGE B", { VF::exDurSeatB, VF::exDur1mmB, VF::exLiftB, VF::exCentreB }, camB);
            const pages::Section model = sec("MODEL", { VF::model, VF::cam }, { "MODEL", "CAM DIAL" });
            const pages::Section timing = sec("VARIABLE TIMING", { VF::inPhaseRange, VF::exPhaseRange, VF::phaseRate },
                                              { "INTAKE RANGE", "EXHAUST RANGE", "SPEED" });
            const pages::Section stages = sec("TWO STAGES", { VF::stages, VF::stageSpeed, VF::stageHyst }, { "TWO STAGES", "CHANGE AT", "HYSTERESIS" });
            const pages::Section lift = sec("LIFT CONTROL", { VF::liftControl, VF::liftMin, VF::liftThrottle },
                                            { "CONTINUOUS", "LEAST LIFT", "LOAD BY LIFT" });
            const pages::Section deac = sec("DEACTIVATION", { VF::deacMask, VF::deacLow, VF::deacHigh, VF::deacLoad },
                                            { "MAY REST", "FROM", "TO", "BELOW LOAD" });
            const pages::Section springs = sec("SPRINGS", { VF::train, VF::springRate, VF::springPreload, VF::valveMass },
                                               { "TRAIN", "RATE", "PRELOAD", "VALVE MASS" });
            const pages::Section head = sec("HEAD", { VF::valves, VF::dIn, VF::dEx, VF::portIn, VF::portEx },
                                            { "VALVES", "INTAKE DIA", "EXHAUST DIA", "INTAKE PORT", "EXHAUST PORT" });
            const Train t = trainOf(ctx_.doc);
            const std::string labNote = t.labOff ? "THE LAB IS OFF: THESE TAKE EFFECT WITH THE LAB MODEL" : "";
            if (s == "valve.intake")
                return pages::page(ctx_.doc, stageB(t) ? std::vector{ inB, inA, timing } : std::vector{ inA, inB, timing }, labNote);
            if (s == "valve.exhaust")
                return pages::page(ctx_.doc, stageB(t) ? std::vector{ exB, exA, timing } : std::vector{ exA, exB, timing }, labNote);
            if (s == "valve.timing")
                return pages::page(ctx_.doc, { model, inA, exA }, labNote);
            if (s == "valve.head")
                return pages::page(ctx_.doc, { head }, "THE BENCH IS FROM THESE AND THE BORE");
            if (s == "valve.phasers")
                return pages::page(ctx_.doc, { timing, stages, lift }, labNote);
            if (s == "valve.springs")
                return pages::page(ctx_.doc, { springs, deac }, labNote);
            return {};
        }

        void pointerDown(const funkgui::PointerEvent& e) override
        {
            const Point pt{ e.x, e.y };
            const int h = handleAt(pt);
            if (h >= 0)
            {
                drag_ = h;
                ctx_.doc.beginGesture();
                ctx_.selection.select(h < exPeak ? "valve.intake" : "valve.exhaust");
                return;
            }
            const std::string hit = regionAt(pt);
            if (!hit.empty())
                ctx_.selection.select(hit);
            else
                Board::pointerDown(e);
        }
        void pointerDrag(const funkgui::PointerEvent& e) override
        {
            if (drag_ >= 0)
                dragTo({ e.x, e.y });
            else
                Board::pointerDrag(e);
        }
        void pointerUp(const funkgui::PointerEvent& e) override
        {
            if (drag_ >= 0)
            {
                dragTo({ e.x, e.y });
                ctx_.doc.endGesture();
                drag_ = -1;
            }
            else
                Board::pointerUp(e);
        }
        void pointerMove(const funkgui::PointerEvent& e) override
        {
            hot_ = handleAt({ e.x, e.y });
            ctx_.selection.hover(hot_ >= 0 ? (hot_ < exPeak ? "valve.intake" : "valve.exhaust") : regionAt({ e.x, e.y }));
            Board::pointerMove(e);
        }
        void closeGestures() override
        {
            if (drag_ >= 0)
            {
                ctx_.doc.endGesture();
                drag_ = -1;
            }
            Board::closeGestures();
        }

    protected:
        void layout(const Rect& s) override
        {
            const float pad = 16.0f;
            const Rect in{ s.x + pad, s.y + pad, s.w - 2 * pad, s.h - 2 * pad };
            const float wR = std::clamp(std::floor(in.w * 0.3f), 280.0f, 460.0f);
            const float wL = in.w - wR - kGap;
            const float hE = std::floor(in.h - std::clamp(in.h * 0.3f, 190.0f, 260.0f) - kGap);
            editor_ = { in.x, in.y, wL, hE };
            const float wP = std::floor((wL - kGap) * 0.5f);
            phasers_ = { in.x, in.y + hE + kGap, wP, in.h - hE - kGap };
            live_ = { phasers_.right() + kGap, phasers_.y, wL - wP - kGap, phasers_.h };
            const float hR = std::floor(in.h * 0.5f);
            ring_ = { editor_.right() + kGap, in.y, wR, hR };
            bench_ = { ring_.x, in.y + hR + kGap, wR, in.h - hR - kGap };
            const Rect e = content(editor_);
            plot_ = { e.x + 44.0f, e.y + 46.0f, e.w - 52.0f, e.h - 46.0f - 34.0f };
        }

        void step(float) override
        {
            // the lift axis: the larger lobe and a quarter, held still while a handle is dragged
            if (drag_ < 0)
            {
                const Train t = trainOf(ctx_.doc);
                double most = 0.0;
                for (const Lobe* l : { &t.in, &t.ex, &t.inB, &t.exB })
                    if (l->known())
                        most = std::max(most, std::isfinite(l->lift) ? l->lift : 1.0);
                yMax_ = most > 0.0 ? most * 1.3 : 0.015;
            }
        }

        void drawStage(Canvas& c, const funkgui::Theme& th) const override
        {
            const lk::Palette p = lk::Palette::of(th);
            const Train t = trainOf(ctx_.doc);
            drawEditor(c, p, t);
            drawRing(c, p, t);
            drawBench(c, p, t);
            drawPhasers(c, p, t);
            drawLive(c, p, t);
        }

    private:
        // ---- the editor's axes ---------------------------------------------------------------------------------------
        static constexpr double kA0 = 0.0, kA1 = 720.0;
        float X(double a) const { return plot_.x + static_cast<float>((a - kA0) / (kA1 - kA0)) * plot_.w; }
        double angleAt(float x) const { return kA0 + (x - plot_.x) / plot_.w * (kA1 - kA0); }
        bool metres(const Lobe& l) const { return std::isfinite(l.lift); }
        float Y(double lift) const { return plot_.bottom() - static_cast<float>(lift / yMax_) * plot_.h; }
        double liftAt(float y) const { return (plot_.bottom() - y) / plot_.h * yMax_; }

        double crankAngle() const
        {
            const double c = ctx_.live.get(en::kTelCrank);
            return std::isfinite(c) ? en::wrap(c, 720.0) : en::kNaN;
        }
        // A lobe as it runs now: shifted by its phaser (the intake's advances, the exhaust's retards).
        static double phaseShift(bool intake, double phase) { return std::isfinite(phase) ? (intake ? -phase : phase) : 0.0; }

        // Stage B is what runs (and what the handles edit) while telemetry says it is engaged.
        bool stageB(const Train& t) const { return t.stages && t.inB.known() && t.exB.known() && ctx_.live.get(kTelStage) >= 0.5; }
        const Lobe& lobe(const Train& t, bool intake) const
        {
            return stageB(t) ? (intake ? t.inB : t.exB) : (intake ? t.in : t.ex);
        }
        const Lobe& ghost(const Train& t, bool intake) const
        {
            return stageB(t) ? (intake ? t.in : t.ex) : (intake ? t.inB : t.exB);
        }

        Point handlePoint(int h, const Train& t) const
        {
            const Lobe& l = lobe(t, h < exPeak);
            const int k = h % 3;
            if (k == 0)
                return { X(l.centre), Y(l.liftAt(l.centre)) };
            return { X(k == 1 ? l.centre - 0.5 * l.durSeat : l.centre + 0.5 * l.durSeat), plot_.bottom() };
        }
        int handleAt(Point pt) const
        {
            const Train t = trainOf(ctx_.doc);
            if (!t.editable)
                return -1;
            int best = -1;
            float bestD = 11.0f * 11.0f;
            for (int h = 0; h < handleCount; ++h)
            {
                const Point q = handlePoint(h, t);
                const float dx = pt.x - q.x, dy = pt.y - q.y, d = dx * dx + dy * dy;
                if (d < bestD)
                {
                    bestD = d;
                    best = h;
                }
            }
            return best;
        }

        void write(VF f, double v)
        {
            const int i = en::findField(ctx_.doc, path(f));
            if (i >= 0)
                ctx_.doc.set(i, v);
        }
        void dragTo(Point pt)
        {
            const Train t = trainOf(ctx_.doc);
            const bool intake = drag_ < exPeak, b = stageB(t);
            const Lobe& l = lobe(t, intake);
            const double a = std::clamp(angleAt(pt.x), kA0, kA1);
            if (drag_ % 3 == 0)
            {
                // the peak: across moves the centreline (a cam card's: after the overlap's top for the intake,
                // before it for the exhaust), up and down the lift
                write(intake ? (b ? VF::inCentreB : VF::inCentre) : (b ? VF::exCentreB : VF::exCentre), intake ? a - 360.0 : 360.0 - a);
                if (metres(l))
                    write(intake ? (b ? VF::inLiftB : VF::inLift) : (b ? VF::exLiftB : VF::exLift), liftAt(pt.y));
            }
            else
            {
                // a seat event: the duration about the centreline; the 1 mm duration keeps its distance (the ramps)
                const double dur = 2.0 * std::fabs(a - l.centre);
                const double ramp = std::isfinite(l.dur1mm) ? l.durSeat - l.dur1mm : en::kNaN;
                write(intake ? (b ? VF::inDurSeatB : VF::inDurSeat) : (b ? VF::exDurSeatB : VF::exDurSeat), dur);
                if (std::isfinite(ramp))
                    write(intake ? (b ? VF::inDur1mmB : VF::inDur1mm) : (b ? VF::exDur1mmB : VF::exDur1mm), dur - ramp);
            }
        }

        std::string regionAt(Point pt) const
        {
            if (plot_.contains(pt))
            {
                // the nearer lobe under the pointer
                const Train t = trainOf(ctx_.doc);
                const double a = angleAt(pt.x);
                return t.in.lift01(a) >= t.ex.lift01(a) ? (t.in.lift01(a) > 0.0 ? "valve.intake" : "") : "valve.exhaust";
            }
            if (ring_.contains(pt))
                return "valve.timing";
            if (bench_.contains(pt))
                return "valve.head";
            if (phasers_.contains(pt))
                return "valve.phasers";
            if (live_.contains(pt))
                return "valve.springs";
            return {};
        }

        // ---- the cam editor ------------------------------------------------------------------------------------------
        void fill(Canvas& c, const Lobe& l, double shift, Col fillInk) const
        {
            if (!l.known())
                return;
            std::vector<float> xs, ys;
            for (double a = kA0; a <= kA1 + 1e-9; a += 2.0)
            {
                xs.push_back(X(a));
                ys.push_back(Y(l.liftAt(a - shift)));
            }
            c.areaStrip(xs.data(), static_cast<int>(xs.size()), ys.data(), nullptr, plot_.bottom(), fillInk);
        }
        void curve(Canvas& c, const Lobe& l, double shift, Col ink, Col fill, float width) const
        {
            if (!l.known())
                return;
            std::vector<float> xs, ys;
            for (double a = kA0; a <= kA1 + 1e-9; a += 2.0)
            {
                xs.push_back(X(a));
                ys.push_back(Y(l.liftAt(a - shift)));
            }
            if (fill.a != 0)
                c.areaStrip(xs.data(), static_cast<int>(xs.size()), ys.data(), nullptr, plot_.bottom(), fill);
            c.polyline(xs.data(), ys.data(), static_cast<int>(xs.size()), width, ink);
        }

        void drawEditor(Canvas& c, const lk::Palette& p, const Train& t) const
        {
            const bool sel = ctx_.selection.is("valve.intake") || ctx_.selection.is("valve.exhaust");
            lk::region(c, p, editor_, "CAM EDITOR: LIFT AGAINST CRANK ANGLE", sel);
            const Rect e = content(editor_);
            const Units& u = ctx_.units;
            const Lobe& li = lobe(t, true);
            const Lobe& le = lobe(t, false);
            const Events ev = eventsOf(li, le);
            // the numbers that matter at a glance, over the plot
            float x = e.x;
            x += stage::readout(c, p, x, e.y - 4.0f, "OVERLAP", deg(ev.overlap), "deg", lk::kNumberS) + 22.0f;
            x += stage::readout(c, p, x, e.y - 4.0f, "LOBE SEPARATION", lk::num(ev.lsa, "%.1f"), "deg", lk::kNumberS) + 22.0f;
            // the piston's room: the least between each valve and the crown, near the overlap's top
            const en::Layout lay = en::layoutOf(ctx_.doc);
            const en::Cylinder cyl = en::cylinderOf(ctx_.doc, lay);
            const bool piston = cyl.known() && std::isfinite(cyl.compression) && cyl.compression > 1.05 && metres(t.in);
            const double clear = piston ? cyl.stroke / (cyl.compression - 1.0) : en::kNaN;
            const auto room = [&](double a) { return clear + en::pistonDrop(a - 360.0, cyl.radius(), cyl.rod); };
            double leastIn = en::kNaN, leastEx = en::kNaN;
            if (piston)
            {
                leastIn = leastEx = 1e9;
                for (double a = 300.0; a <= 420.0; a += 0.5)
                {
                    leastIn = std::min(leastIn, room(a) - li.liftAt(a));
                    leastEx = std::min(leastEx, room(a) - le.liftAt(a));
                }
            }
            const auto len = [&](double m) { return std::isfinite(m) ? u.number(Quantity::length, m) : std::string("-"); };
            const std::string lu = u.unit(Quantity::length).name;
            x += stage::readout(c, p, x, e.y - 4.0f, "ROOM, INTAKE", len(leastIn), lu, lk::kNumberS,
                             std::isfinite(leastIn) && leastIn < 0.001 ? p.warning : kClear) + 22.0f;
            stage::readout(c, p, x, e.y - 4.0f, "ROOM, EXHAUST", len(leastEx), lu, lk::kNumberS,
                        std::isfinite(leastEx) && leastEx < 0.001 ? p.warning : kClear);
            // what drives it, in words
            const char* note = t.reference ? "THE REFERENCE'S CAM DIAL: SET IN THE INSPECTOR"
                             : t.labOff    ? "THE LAB IS OFF: THE ENGINE RUNS THE CAM DIAL"
                             : t.editable  ? "DRAG A PEAK OR A SEAT MARK"
                                           : "";
            put(c, note, e.right(), e.y - 4.0f, lk::kCaps, t.labOff ? p.warning : p.label, Align::right);

            // the axes: TDC and BDC through the cycle, lift up the side
            c.rrect(plot_.x, plot_.y, plot_.w, plot_.h, 0.0f, kClear, 1.0f, p.grid);
            const char* marks[5] = { "TDC", "BDC", "TDC", "BDC", "TDC" };
            const char* under[5] = { "FIRING", "", "OVERLAP", "", "FIRING" };
            for (int i = 0; i < 5; ++i)
            {
                const float mx = X(180.0 * i);
                c.hairlineV(mx, plot_.y, plot_.h, i % 2 == 0 ? p.dim : p.grid);
                put(c, marks[i], mx, plot_.bottom() + 6.0f, lk::kCaps, p.text, i == 0 ? Align::left : i == 4 ? Align::right : Align::centre);
                if (*under[i])
                    put(c, under[i], mx, plot_.bottom() + 19.0f, lk::kCaps, p.dim, i == 0 ? Align::left : i == 4 ? Align::right : Align::centre);
            }
            for (int d = 90; d < 720; d += 180)
                put(c, std::to_string(d), X(d), plot_.bottom() + 6.0f, lk::kUnit, p.dim, Align::centre);
            const bool inM = metres(t.in);
            // lift ticks at 1, 2 or 5 of the user's unit (a share's 10, 20 or 50 %)
            const double top = inM ? u.toUser(Quantity::length, yMax_) : yMax_ * 100.0;
            const double raw = top / 5.0, mag = std::pow(10.0, std::floor(std::log10(raw)));
            const double tick = raw / mag < 1.5 ? mag : raw / mag < 3.5 ? 2.0 * mag : raw / mag < 7.5 ? 5.0 * mag : 10.0 * mag;
            for (double tv = tick; tv < top - 1e-9; tv += tick)
            {
                const double v = inM ? u.toSi(Quantity::length, tv) : tv / 100.0;
                const float y = Y(v);
                c.hairlineH(plot_.x, y, plot_.w, p.over(p.grid, 0.6f));
                put(c, inM ? u.number(Quantity::length, v) : lk::num(v * 100.0, "%.0f"), plot_.x - 6.0f,
                    c.capCentreTop(y, lk::kUnit), lk::kUnit, p.label, Align::right);
            }
            put(c, inM ? std::string("LIFT, ") + u.unit(Quantity::length).name : "LIFT, %", plot_.x - 44.0f, plot_.y - 16.0f,
                lk::kCaps, p.label);
            if (!t.in.known() && !t.ex.known())
            {
                putCentred(c, "NO CAMS IN THIS BUILD YET", plot_.centreX(), plot_.centreY(), lk::kCaps, p.dim);
                return;
            }
            const funkgui::Canvas::ClipScope clip(c, plot_);
            const double inPh = ctx_.live.get(kTelInPhase), exPh = ctx_.live.get(kTelExPhase);
            const double inShift = phaseShift(true, inPh), exShift = phaseShift(false, exPh);
            // under everything: each lobe's area as it runs now, and the overlap (the area under both)
            fill(c, le, exShift, p.over(p.hot, 0.12f));
            fill(c, li, inShift, p.over(p.cold, 0.12f));
            {
                std::vector<float> xs, ys;
                for (double a = 240.0; a <= 480.0; a += 1.0)
                {
                    xs.push_back(X(a));
                    ys.push_back(Y(std::min(li.liftAt(a - inShift), le.liftAt(a - exShift))));
                }
                c.areaStrip(xs.data(), static_cast<int>(xs.size()), ys.data(), nullptr, plot_.bottom(), p.over(p.value, 0.32f));
            }
            // the piston's room above its crown, ghosted: where a valve comes near it
            if (piston)
            {
                std::vector<float> xs, ys;
                for (double a = 240.0; a <= 480.0; a += 2.0)
                {
                    const double r = room(a);
                    if (r > yMax_ * 1.05)
                        continue;
                    xs.push_back(X(a));
                    ys.push_back(Y(r));
                }
                if (xs.size() > 1)
                {
                    c.polyline(xs.data(), ys.data(), static_cast<int>(xs.size()), 1.25f, p.label);
                    put(c, "PISTON", X(360.0) + 8.0f, Y(room(360.0)) - 4.0f, lk::kCaps, p.label);
                }
            }
            // the other stage behind, ghosted and named
            if (t.stages)
            {
                const Lobe& gi = ghost(t, true);
                const Lobe& ge = ghost(t, false);
                curve(c, gi, 0.0, p.over(p.cold, 0.5f), kClear, 1.25f);
                curve(c, ge, 0.0, p.over(p.hot, 0.5f), kClear, 1.25f);
                const char* w = stageB(t) ? "STAGE A" : "STAGE B";
                if (gi.known())
                    put(c, w, X(gi.centre) + 14.0f, Y(gi.liftAt(gi.centre)) - 4.0f, lk::kCaps, p.over(p.cold, 0.75f));
                if (ge.known())
                    put(c, w, X(ge.centre) - 14.0f, Y(ge.liftAt(ge.centre)) - 4.0f, lk::kCaps, p.over(p.hot, 0.75f), Align::right);
            }
            // the lobes parked, ghosted when a phaser has moved them; then as they run now
            if (inShift != 0.0)
                curve(c, li, 0.0, p.over(p.cold, 0.4f), kClear, 1.0f);
            if (exShift != 0.0)
                curve(c, le, 0.0, p.over(p.hot, 0.4f), kClear, 1.0f);
            curve(c, le, exShift, p.hot, kClear, lk::kTrace);
            curve(c, li, inShift, p.cold, kClear, lk::kTrace);
            // the phasers: their ranges as bands along the top, where each is now
            const auto band = [&](const Lobe& l, double range, double now, bool intake, float y) {
                if (!(range > 0.0) || !l.known())
                    return;
                const double a0 = intake ? l.centre - range : l.centre, a1 = intake ? l.centre : l.centre + range;
                const Col ink = intake ? p.cold : p.hot;
                c.rrect(X(a0), y, X(a1) - X(a0), 8.0f, 4.0f, p.over(ink, 0.3f), 1.0f, p.over(ink, 0.7f));
                c.segment(X(l.centre), y - 2.0f, X(l.centre), y + 10.0f, 1.0f, p.label);
                if (std::isfinite(now))
                {
                    const float nx = X(l.centre + phaseShift(intake, now));
                    c.disc(nx, y + 4.0f, 5.0f, ink, 1.5f, p.ground);
                }
                const std::string w = std::string(intake ? "INTAKE PHASER " : "EXHAUST PHASER ") + deg(range) + " DEG";
                put(c, w, intake ? X(a0) - 8.0f : X(a1) + 8.0f, y - 1.0f, lk::kCaps, ink, intake ? Align::right : Align::left);
            };
            band(li, t.inPhaseRange, inPh, true, plot_.y + 8.0f);
            band(le, t.exPhaseRange, exPh, false, plot_.y + 8.0f);
            const char* st = t.stages ? (stageB(t) ? " B" : " A") : "";
            put(c, std::string("EXHAUST") + st, X(le.centre + exShift), Y(le.liftAt(le.centre)) - 24.0f, lk::kCaps, p.hot, Align::centre);
            put(c, std::string("INTAKE") + st, X(li.centre + inShift), Y(li.liftAt(li.centre)) - 24.0f, lk::kCaps, p.cold, Align::centre);
            if (std::isfinite(ev.overlap) && ev.overlap > 0.0)
                put(c, "OVERLAP", X(360.0), plot_.bottom() - 30.0f, lk::kCaps, p.value, Align::centre);
            // the crank now
            const double a = crankAngle();
            if (std::isfinite(a))
            {
                c.segment(X(a), plot_.y, X(a), plot_.bottom(), 1.5f, p.value);
                c.disc(X(a), plot_.y, 3.5f, p.value);
            }
            // the handles: a ring at each peak, a tab at each seat event
            if (t.editable)
                for (int h = 0; h < handleCount; ++h)
                {
                    const Point q = handlePoint(h, t);
                    const Col ink = h < exPeak ? p.cold : p.hot;
                    const bool on = h == drag_ || h == hot_;
                    if (h % 3 == 0)
                        c.disc(q.x, q.y, on ? 7.0f : 5.5f, p.ground, 2.0f, on ? p.selected : ink);
                    else
                        c.rrect(q.x - 4.0f, q.y - 9.0f, 8.0f, 12.0f, 2.0f, on ? p.selected : ink, 1.0f, p.ground);
                }
        }

        // ---- the timing ring -----------------------------------------------------------------------------------------
        void drawRing(Canvas& c, const lk::Palette& p, const Train& t) const
        {
            lk::region(c, p, ring_, "TIMING", ctx_.selection.is("valve.timing"));
            const Rect in = content(ring_);
            const Events ev = eventsOf(lobe(t, true), lobe(t, false));
            const float R = std::max(20.0f, std::min(in.w * 0.5f - 24.0f, (in.h - 84.0f) * 0.5f));
            const Point ctr{ in.centreX(), in.y + 18.0f + R };
            c.arc(ctr, R + 10.0f, 0.0f, static_cast<float>(2.0 * stage::kPi), 1.0f, p.grid);
            // TDC up, BDC down
            put(c, "TDC", ctr.x, ctr.y - R - 30.0f, lk::kCaps, p.text, Align::centre);
            put(c, "BDC", ctr.x, ctr.y + R + 18.0f, lk::kCaps, p.text, Align::centre);
            c.segment(ctr.x, ctr.y - R - 16.0f, ctr.x, ctr.y + R + 16.0f, 1.0f, p.dim);
            if (t.ex.known())
            {
                // the exhaust: from EVO before BDC round through TDC to EVC (clockwise on the outer arc)
                const double a0 = 180.0 - ev.evo, a1 = 360.0 + ev.evc;
                c.arc(ctr, R, stage::arcAngle(a0), stage::arcAngle(a1), 7.0f, p.hot);
            }
            if (t.in.known())
            {
                // the intake: from IVO before TDC through BDC to IVC (the inner arc)
                const double a0 = -ev.ivo, a1 = 180.0 + ev.ivc;
                c.arc(ctr, R - 13.0f, stage::arcAngle(a0), stage::arcAngle(a1), 7.0f, p.cold);
            }
            // the four events in the four corners, each by its quadrant
            const auto corner = [&](float x, float y, Align al, const char* name, double v, const char* ref, Col ink) {
                put(c, name, x, y, lk::kCaps, ink, al);
                put(c, std::isfinite(v) ? deg(v) + " " + ref : "-", x, y + 13.0f, lk::kBody, p.value, al);
            };
            corner(in.x, in.y, Align::left, "IVO", ev.ivo, "BTDC", p.cold);
            corner(in.right(), in.y, Align::right, "EVC", ev.evc, "ATDC", p.hot);
            corner(in.x, in.bottom() - 26.0f, Align::left, "IVC", ev.ivc, "ABDC", p.cold);
            corner(in.right(), in.bottom() - 26.0f, Align::right, "EVO", ev.evo, "BBDC", p.hot);
            putCentred(c, "OVERLAP", ctr.x, ctr.y - 8.0f, lk::kCaps, p.label);
            putCentred(c, deg(ev.overlap) + " deg", ctr.x, ctr.y + 8.0f, lk::kNumberS, p.value);
            put(c, "OUTSIDE: EXHAUST", ctr.x, in.bottom() - 24.0f, lk::kCaps, p.hot, Align::centre);
            put(c, "INSIDE: INTAKE", ctr.x, in.bottom() - 10.0f, lk::kCaps, p.cold, Align::centre);
        }

        // ---- the flow bench ------------------------------------------------------------------------------------------
        void drawBench(Canvas& c, const lk::Palette& p, const Train& t) const
        {
            lk::region(c, p, bench_, "FLOW BENCH, 28 IN OF WATER", ctx_.selection.is("valve.head"));
            const Rect in = content(bench_);
            const Rect plot{ in.x + 40.0f, in.y + 14.0f, in.w - 44.0f, in.h - 14.0f - 30.0f };
            const Units& u = ctx_.units;
            const en::Layout lay = en::layoutOf(ctx_.doc);
            const en::Cylinder cyl = en::cylinderOf(ctx_.doc, lay);
            const Bench b = benchOf(ctx_.doc, cyl.bore);
            c.rrect(plot.x, plot.y, plot.w, plot.h, 0.0f, kClear, 1.0f, p.grid);
            put(c, "CFM", plot.x - 40.0f, in.y - 2.0f, lk::kCaps, p.label);
            put(c, std::string("LIFT, ") + u.unit(Quantity::length).name, plot.right(), plot.bottom() + 16.0f, lk::kCaps,
                p.label, Align::right);
            if (!b.known)
            {
                putCentred(c, "NO HEAD IN THIS BUILD YET", plot.centreX(), plot.centreY(), lk::kCaps, p.dim);
                return;
            }
            const double lmax = b.lift[15], fmax = std::max(b.flowIn[15], b.flowEx[15]) * kCfm * 1.12;
            const auto X = [&](double l) { return plot.x + static_cast<float>(l / lmax) * plot.w; };
            const auto Y = [&](double q) { return plot.bottom() - static_cast<float>(q * kCfm / fmax) * plot.h; };
            const double step = fmax > 400.0 ? 100.0 : fmax > 160.0 ? 50.0 : 20.0;
            for (double f = step; f < fmax; f += step)
            {
                c.hairlineH(plot.x, Y(f / kCfm), plot.w, p.over(p.grid, 0.6f));
                put(c, lk::num(f), plot.x - 5.0f, c.capCentreTop(Y(f / kCfm), lk::kUnit), lk::kUnit, p.label, Align::right);
            }
            for (int i = 0; i <= 4; ++i)
            {
                const double l = lmax * i / 4.0;
                put(c, u.number(Quantity::length, l), X(l), plot.bottom() + 4.0f, lk::kUnit, p.label, Align::centre);
            }
            const auto trace = [&](const std::array<double, 16>& q, double d, Col ink, const char* word, float ky) {
                float xs[17], ys[17];
                xs[0] = plot.x;
                ys[0] = plot.bottom();
                for (int i = 0; i < 16; ++i)
                {
                    xs[i + 1] = X(b.lift[static_cast<std::size_t>(i)]);
                    ys[i + 1] = Y(q[static_cast<std::size_t>(i)]);
                }
                c.polyline(xs, ys, 17, lk::kTrace, ink);
                // a quarter of the diameter: past it, more lift buys little
                if (d * 0.25 < lmax)
                {
                    c.segment(X(d * 0.25), plot.bottom() - 6.0f, X(d * 0.25), plot.bottom(), 1.5f, ink);
                    put(c, "D/4", X(d * 0.25), plot.bottom() - 18.0f, lk::kUnit, ink, Align::centre);
                }
                put(c, word, plot.x + 8.0f, ky, lk::kCaps, ink);
            };
            trace(b.flowIn, b.dIn, p.cold, b.nIn > 1 ? "INTAKE VALVES" : "INTAKE VALVE", plot.y + 6.0f);
            trace(b.flowEx, b.dEx, p.hot, b.nEx > 1 ? "EXHAUST VALVES" : "EXHAUST VALVE", plot.y + 20.0f);
            // the lift now on each, from the cam and the crank
            const double a = crankAngle();
            if (std::isfinite(a) && metres(t.in))
            {
                const double li = lobe(t, true).liftAt(a - phaseShift(true, ctx_.live.get(kTelInPhase)));
                const double le = lobe(t, false).liftAt(a - phaseShift(false, ctx_.live.get(kTelExPhase)));
                const auto mark = [&](double l, int n, double d, double q, Col ink) {
                    if (!(l > 0.0))
                        return;
                    const double f = benchFlow(n, d, q, l);
                    c.disc(X(std::min(l, lmax)), Y(f), 4.5f, ink, 1.5f, p.ground);
                };
                const double qi = fieldValue(ctx_.doc, VF::portIn), qe = fieldValue(ctx_.doc, VF::portEx);
                mark(li, b.nIn, b.dIn, std::isfinite(qi) && qi > 0.0 ? qi : 1.0, p.cold);
                mark(le, b.nEx, b.dEx, std::isfinite(qe) && qe > 0.0 ? qe : 1.0, p.hot);
            }
        }

        // ---- phasers and stages ----------------------------------------------------------------------------------------
        void drawPhasers(Canvas& c, const lk::Palette& p, const Train& t) const
        {
            lk::region(c, p, phasers_, "PHASERS AND STAGES", ctx_.selection.is("valve.phasers"));
            const Rect in = content(phasers_);
            const float rowH = std::min(34.0f, in.h / 4.0f);
            float y = in.y;
            const auto phaser = [&](const char* name, double range, double now, Col ink) {
                put(c, name, in.x, y, lk::kCaps, p.label);
                if (!(range > 0.0))
                {
                    put(c, std::isfinite(range) ? "NONE" : "-", in.x + 130.0f, y - 1.0f, lk::kBody, p.dim);
                }
                else
                {
                    const std::string s = (std::isfinite(now) ? deg(now) : std::string("-")) + " of " + deg(range) + " deg";
                    put(c, s, in.x + 130.0f, y - 1.0f, lk::kBody, p.value);
                    lk::level(c, p, { in.x + 130.0f, y + 15.0f, std::max(20.0f, in.w - 130.0f), 4.0f },
                              std::isfinite(now) ? now / range : en::kNaN, ink);
                }
                y += rowH;
            };
            phaser("INTAKE PHASER", t.inPhaseRange, ctx_.live.get(kTelInPhase), p.cold);
            phaser("EXHAUST PHASER", t.exPhaseRange, ctx_.live.get(kTelExPhase), p.hot);
            // the stages: which is engaged now, in words
            put(c, "STAGES", in.x, y, lk::kCaps, p.label);
            if (!t.stages)
                put(c, std::isfinite(fieldValue(ctx_.doc, VF::stages)) ? "ONE" : "-", in.x + 130.0f, y - 1.0f, lk::kBody, p.dim);
            else
            {
                const double st = ctx_.live.get(kTelStage);
                const std::string at = std::isfinite(t.stageSpeed) ? ctx_.units.format(Quantity::rotation, t.stageSpeed, 0) : "-";
                put(c, "B from " + at, in.x + 130.0f, y - 1.0f, lk::kBody, p.value);
                const std::string now = !std::isfinite(st) ? "ENGAGED -" : st >= 0.5 ? "B ENGAGED" : "A ENGAGED";
                lk::lamp(c, p, in.x + 130.0f, y + 22.0f, now, std::isfinite(st) && st >= 0.5, p.selected);
            }
            y += rowH + 8.0f;
            // continuous lift
            put(c, "CONTINUOUS LIFT", in.x, y, lk::kCaps, p.label);
            const double lc = fieldValue(ctx_.doc, VF::liftControl), lnow = ctx_.live.get(kTelLift);
            if (!(lc >= 0.5))
                put(c, std::isfinite(lc) ? "OFF" : "-", in.x + 130.0f, y - 1.0f, lk::kBody, p.dim);
            else
                put(c, (std::isfinite(lnow) ? lk::num(lnow * 100.0) + " %" : std::string("-")) + " of the lift", in.x + 130.0f,
                    y - 1.0f, lk::kBody, p.value);
        }

        // ---- live: float and deactivation --------------------------------------------------------------------------------
        void drawLive(Canvas& c, const lk::Palette& p, const Train&) const
        {
            lk::region(c, p, live_, "SPRINGS AND CYLINDERS, LIVE", ctx_.selection.is("valve.springs"));
            const Rect in = content(live_);
            const Units& u = ctx_.units;
            // valve float: the speed it would float at, and how far below it the engine is
            const double fl = ctx_.live.get(kTelFloat), w = ctx_.live.get(en::kTelSpeed);
            const double margin = std::isfinite(fl) && std::isfinite(w) ? fl - w : en::kNaN;
            const bool near = std::isfinite(margin) && margin < 500.0 * stage::kPi / 30.0;
            const float wa = stage::readout(c, p, in.x, in.y, "FLOATS AT", std::isfinite(fl) ? u.number(Quantity::rotation, fl) : "-",
                                         u.unit(Quantity::rotation).name, lk::kNumberS);
            stage::readout(c, p, in.x + std::max(110.0f, wa + 24.0f), in.y, near ? "MARGIN: NEAR FLOAT" : "MARGIN",
                        std::isfinite(margin) ? u.number(Quantity::rotation, margin) : "-", u.unit(Quantity::rotation).name,
                        lk::kNumberS, near ? p.warning : kClear);
            const double rate = fieldValue(ctx_.doc, VF::springRate);
            if (std::isfinite(rate) && rate <= 0.0)
                put(c, "SPRING RATE 0: THE VALVES NEVER FLOAT", in.x, in.y + 36.0f, lk::kCaps, p.dim);
            // deactivation: each cylinder, those that can rest outlined, those resting now hatched and named
            const en::Layout lay = en::layoutOf(ctx_.doc);
            const double mask = fieldValue(ctx_.doc, VF::deacMask), act = ctx_.live.get(kTelActive);
            const float y = in.y + 58.0f;
            put(c, "CYLINDERS", in.x, y, lk::kCaps, p.label);
            if (lay.empty() || !std::isfinite(mask))
            {
                put(c, "-", in.x + 90.0f, y - 1.0f, lk::kBody, p.dim);
                return;
            }
            const auto can = static_cast<unsigned>(std::lround(mask));
            const auto on = std::isfinite(act) ? static_cast<unsigned>(std::lround(act)) : ~0u;
            const float bw = std::min(30.0f, (in.w - 8.0f) / static_cast<float>(lay.n) - 4.0f);
            int resting = 0;
            for (int i = 0; i < lay.n; ++i)
            {
                const float bx = in.x + static_cast<float>(i) * (bw + 4.0f), by = y + 16.0f;
                const bool may = (can >> i) & 1u, rest = std::isfinite(act) && !((on >> i) & 1u);
                resting += rest ? 1 : 0;
                c.rrect(bx, by, bw, 26.0f, 4.0f, rest ? p.panel : p.raised, may ? 1.5f : 1.0f, may ? p.warning : p.edge);
                if (rest)
                    for (float hx = bx + 4.0f; hx < bx + bw - 2.0f; hx += 6.0f)
                        c.segment(hx, by + 22.0f, std::min(hx + 5.0f, bx + bw - 2.0f), by + 4.0f, 1.0f, p.dim);
                putCentred(c, std::to_string(i + 1), bx + bw * 0.5f, by + 13.0f, lk::kCaps, rest ? p.dim : p.value);
            }
            std::string s = std::isfinite(act) ? std::to_string(resting) + " RESTING NOW" : "RESTING NOW: -";
            put(c, s, in.x, y + 50.0f, lk::kCaps, resting > 0 ? p.warning : p.label);
            put(c, "OUTLINED: MAY REST", in.right(), y + 50.0f, lk::kCaps, p.dim, Align::right);
        }

        int drag_ = -1, hot_ = -1;
        double yMax_ = 0.015;
        Rect editor_{}, plot_{}, ring_{}, bench_{}, phasers_{}, live_{};
    };

    std::unique_ptr<Workspace> makeValvesWorkspace(WorkspaceContext& c) { return std::make_unique<ValvesWorkspace>(c); }
}
