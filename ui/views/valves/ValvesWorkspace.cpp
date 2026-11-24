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
#include "views/engine/TwoStroke.h"
#include "views/valves/ValvesModel.h"

#include "app/Look.h"
#include "kit/widgets/Basic.h"

#include <algorithm>
#include <array>
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
        explicit ValvesWorkspace(WorkspaceContext& c) : Workspace(c)
        {
            // on a small stage the bench, the phasers and the springs share one panel, chosen by these
            const char* names[3] = { "Bench", "Phasers", "Springs" };
            for (int i = 0; i < 3; ++i)
            {
                tabs_[i] = &make<Button>(names[i], [this, i] { tab_ = i; });
                tabs_[i]->setLit([this, i] { return tab_ == i; });
            }
        }

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
            for (const char* n : { en::kTelPurity, en::kTelDelivery, en::kTelTrapping, en::kTelPowerValve, en::kTelCrankcase,
                                   kTelInPhase, kTelExPhase, kTelInTarget, kTelExTarget, kTelStage, kTelLift, kTelFloat, kTelFloating,
                                   kTelActive, en::kTelSpeed, kTelEvents[0], kTelEvents[1], kTelEvents[2], kTelEvents[3], kTelEvents[4] })
                k.mix(static_cast<float>(ctx_.live.get(n)));
            k.mix(drag_).mix(hot_).mix(static_cast<float>(yMax_)).mix(two_).mix(scopeVersion_).mix(tab_);
            // the oil, as the words say it: whole degrees, and whether it holds the phasers or stage B
            const double oilT = ctx_.live.get(kTelOilTemp);
            k.mix(std::isfinite(oilT) ? static_cast<std::int64_t>(std::lround(oilT)) : -1);
            k.mix(oilHold(ctx_.live, kPhaserOilTemp, kPhaserOilPressure).held).mix(oilHold(ctx_.live, kStageOilTemp, kStageOilPressure).held);
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
            if (engine::twoStrokeOf(ctx_.doc).on)
            {
                const pages::Section ports{ "PORTS", { "engine.tsExhaustDur", "engine.tsExhaustWidth", "engine.tsExhaustRadius",
                                                       "engine.tsTransferDur", "engine.tsTransferWidth", "engine.tsTransferRadius",
                                                       "engine.tsPortCd" },
                                            { "EXHAUST OPEN", "EXHAUST WIDTH", "EXHAUST CORNERS", "TRANSFERS OPEN", "TRANSFER WIDTH",
                                              "TRANSFER CORNERS", "DISCHARGE" } };
                const pages::Section valve{ "POWER VALVE", { "engine.tsValveDur", "engine.tsValveFrom", "engine.tsValveTo" },
                                            { "SHUT: OPEN FOR", "OPENS FROM", "FULLY OPEN AT" } };
                const pages::Section cc{ "CRANKCASE", { "engine.tsCrankcaseRatio", "engine.tsInletKind", "engine.tsInletArea",
                                                        "engine.tsReedFull", "engine.tsReedHz", "engine.tsReedDamping",
                                                        "engine.tsDiscOpen", "engine.tsDiscDur", "engine.tsPistonDur", "engine.tsCarbArea" },
                                         { "COMPRESSION", "INLET", "INLET AREA", "REED FULL AT", "REED HZ", "REED DAMPING",
                                           "DISC OPENS", "DISC OPEN FOR", "PORT OPEN FOR", "CARB AREA" } };
                const pages::Section scav{ "SCAVENGING", { "engine.tsMixing", "engine.tsShortCircuit" }, { "MIXING", "SHORT-CIRCUITED" } };
                if (s == "valve.ports")
                    return pages::page(ctx_.doc, { ports, valve });
                if (s == "valve.powervalve")
                    return pages::page(ctx_.doc, { valve, ports });
                if (s == "valve.crankcase")
                    return pages::page(ctx_.doc, { cc });
                if (s == "valve.scavenging")
                    return pages::page(ctx_.doc, { scav });
                return pages::page(ctx_.doc, { ports, valve, cc, scav }, "A TWO-STROKE: PORTS IN PLACE OF VALVES");
            }
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
                ctx_.selection.select(h >= kPortHandle ? "valve.ports" : h < exPeak ? "valve.intake" : "valve.exhaust");
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
            ctx_.selection.hover(hot_ >= kPortHandle ? "valve.ports"
                                 : hot_ >= 0             ? (hot_ < exPeak ? "valve.intake" : "valve.exhaust")
                                                         : regionAt({ e.x, e.y }));
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
            compact_ = in.w < 980.0f;
            two_ = engine::twoStrokeOf(ctx_.doc).on;
            const char* four[3] = { "Bench", "Phasers", "Springs" };
            const char* twoNames[3] = { "Power valve", "Crankcase", "Scavenge" };
            for (int i = 0; i < 3; ++i)
                tabs_[i]->setTitle(two_ ? twoNames[i] : four[i]);
            hasPhasers_ = hasPhasers();
            hasLive_ = hasLive();
            if (compact_)
            {
                // the editor across the stage; under it the timing ring, and a panel of tabs for the rest
                const float hE = std::floor(in.h * 0.56f), wRing = std::floor(std::min(in.w * 0.4f, 300.0f));
                editor_ = { in.x, in.y, in.w, hE };
                ring_ = { in.x, in.y + hE + kGap, wRing, in.h - hE - kGap };
                bench_ = phasers_ = live_ = { ring_.right() + kGap, ring_.y, in.w - wRing - kGap, ring_.h };
                for (int i = 0; i < 3; ++i)
                    tabs_[i]->setPlace({ bench_.x + lk::kPad + 84.0f * static_cast<float>(i), bench_.y + 6.0f, 80.0f, 20.0f }, true);
                const Rect e = content(editor_);
                plot_ = { e.x + 44.0f, e.y + 46.0f, e.w - 52.0f, e.h - 46.0f - 62.0f };
                return;
            }
            for (Button* b : tabs_)
                b->setPlace({}, false);
            // the editor takes what the lower panels leave: they fold to a line each when the build has nothing for them
            const float wR = compact_ ? std::floor(in.w * 0.38f) : std::clamp(std::floor(in.w * 0.3f), 280.0f, 460.0f);
            const float wL = in.w - wR - kGap;
            const float hP = hasPhasers_ ? 132.0f : 34.0f, hL = hasLive_ ? 150.0f : 34.0f;
            const float hB = std::max(hP, hL);
            editor_ = { in.x, in.y, wL, in.h - hB - kGap };
            const float wP = std::floor((wL - kGap) * 0.5f);
            phasers_ = { in.x, editor_.bottom() + kGap, wP, hB };
            live_ = { phasers_.right() + kGap, phasers_.y, wL - wP - kGap, hB };
            const float hR = std::floor(in.h * 0.5f);
            ring_ = { editor_.right() + kGap, in.y, wR, hR };
            bench_ = { ring_.x, in.y + hR + kGap, wR, in.h - hR - kGap };
            const Rect e = content(editor_);
            plot_ = { e.x + 44.0f, e.y + 46.0f, e.w - 52.0f, e.h - 46.0f - 62.0f };
        }

        bool hasPhasers() const
        {
            if (engine::twoStrokeOf(ctx_.doc).on)
                return true;
            for (VF f : { VF::inPhaseRange, VF::exPhaseRange, VF::stages, VF::liftControl })
                if (en::findField(ctx_.doc, path(f)) >= 0)
                    return true;
            return false;
        }
        bool hasLive() const
        {
            if (engine::twoStrokeOf(ctx_.doc).on)
                return true;
            return en::findField(ctx_.doc, path(VF::springRate)) >= 0 || en::findField(ctx_.doc, path(VF::deacMask)) >= 0
                || ctx_.live.has(kTelFloat) || ctx_.live.has(kTelActive);
        }

        void step(float) override
        {
            // a build that gains or loses the lower panels' fields lays out again
            if (hasPhasers() != hasPhasers_ || hasLive() != hasLive_ || engine::twoStrokeOf(ctx_.doc).on != two_)
                arrange();
            if (two_)
            {
                // the Scope's two-stroke arrays (the ports' shares, the crankcase's pressure), read while shown
                const std::uint64_t g = ctx_.scope.generation();
                if (g != scopeGen_ || !scopeRead_)
                {
                    scopeGen_ = g;
                    scopeRead_ = true;
                    readPortScope();
                }
            }
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
            if (two_)
            {
                const engine::TwoStroke ts = engine::twoStrokeOf(ctx_.doc);
                drawPorts(c, p, ts);
                drawPortRing(c, p, ts);
                if (!compact_ || tab_ == 0)
                    drawPowerValve(c, p, ts);
                if (!compact_ || tab_ == 1)
                    drawCrankcase(c, p, ts);
                if (!compact_ || tab_ == 2)
                    drawScavenging(c, p, ts);
                return;
            }
            const Train t = trainOf(ctx_.doc);
            drawEditor(c, p, t);
            drawRing(c, p, t);
            if (!compact_ || tab_ == 0)
                drawBench(c, p, t);
            if (!compact_ || tab_ == 1)
                drawPhasers(c, p, t);
            if (!compact_ || tab_ == 2)
                drawLive(c, p, t);
        }

    private:
        // ---- the editor's axes ---------------------------------------------------------------------------------------
        static constexpr double kA0 = 0.0;
        // the cycle across the plot: a four-stroke's 720 degrees, a two-stroke's 360
        double span() const { return two_ ? 360.0 : 720.0; }
        float X(double a) const { return plot_.x + static_cast<float>((a - kA0) / (span() - kA0)) * plot_.w; }
        double angleAt(float x) const { return kA0 + (x - plot_.x) / plot_.w * (span() - kA0); }
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
        // An oil temperature in the person's units, rounded: "15 C"
        std::string oilText(double kelvin) const
        {
            return ctx_.units.format(Quantity::temperature, kelvin, 0);
        }
        // What the oil holds back, in the words the PHASERS AND STAGES panel shows ("" : nothing)
        std::string phaserHeldText() const { return phaserParkedWords(ctx_.live, ctx_.units); }
        std::string stageHeldText() const { return stageWaitWords(ctx_.live, ctx_.units); }
    public:
        // For tests: the oil's words as drawn now (the phasers' and stage B's, where the build has them)
        std::string oilWords() const
        {
            const Train t = trainOf(ctx_.doc);
            std::string w;
            if (t.inPhaseRange > 0.0 || t.exPhaseRange > 0.0)
                w += phaserHeldText();
            if (t.stages)
                w += (w.empty() ? "" : " | ") + stageHeldText();
            return w;
        }
    private:
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
        // A two-stroke's handles: each port's opening and closing marks (the exhaust's, then the transfers').
        static constexpr int kPortHandle = 100;
        Point portHandle(int h, const engine::TwoStroke& t) const
        {
            const double dur = h < 2 ? t.exDur : t.trDur;
            return { X(h % 2 == 0 ? 180.0 - 0.5 * dur : 180.0 + 0.5 * dur), plot_.bottom() };
        }
        int handleAt(Point pt) const
        {
            if (two_)
            {
                const engine::TwoStroke t = engine::twoStrokeOf(ctx_.doc);
                for (int h = 0; h < 4; ++h)
                {
                    const Point q = portHandle(h, t);
                    if (std::isfinite(q.x) && std::fabs(pt.x - q.x) < 8.0f && std::fabs(pt.y - q.y) < 11.0f)
                        return kPortHandle + h;
                }
                return -1;
            }
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
            if (drag_ >= kPortHandle)
            {
                // a port's mark: its duration about bottom dead centre (the other mark moves with it)
                const double a = std::clamp(angleAt(pt.x), 0.0, 360.0);
                const int f = ctx_.doc.find(drag_ - kPortHandle < 2 ? "engine.tsExhaustDur" : "engine.tsTransferDur");
                if (f >= 0)
                    ctx_.doc.set(f, 2.0 * std::fabs(180.0 - a));
                return;
            }
            const Train t = trainOf(ctx_.doc);
            const bool intake = drag_ < exPeak, b = stageB(t);
            const Lobe& l = lobe(t, intake);
            const double a = std::clamp(angleAt(pt.x), kA0, span());
            if (l.free)
            {
                // a free valve is where it opens and closes: the peak moves both, a seat mark its own; the lift as a cam's
                const VF open = intake ? VF::freeInOpen : VF::freeExOpen, close = intake ? VF::freeInClose : VF::freeExClose;
                if (drag_ % 3 == 0)
                {
                    const double shift = a - l.centre;
                    write(open, en::wrap(l.seatOpen() + shift, 720.0));
                    write(close, en::wrap(l.seatClose() + shift, 720.0));
                    if (metres(l))
                        write(intake ? VF::inLift : VF::exLift, liftAt(pt.y));
                }
                else
                    write(drag_ % 3 == 1 ? open : close, en::wrap(a, 720.0));
                return;
            }
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
            if (two_)
            {
                if (editor_.contains(pt) || ring_.contains(pt))
                    return "valve.ports";
                if (compact_ && bench_.contains(pt))
                    return pt.y < bench_.y + 30.0f ? "" : tab_ == 0 ? "valve.powervalve" : tab_ == 1 ? "valve.crankcase" : "valve.scavenging";
                if (bench_.contains(pt))
                    return "valve.powervalve";
                if (phasers_.contains(pt))
                    return "valve.crankcase";
                if (live_.contains(pt))
                    return "valve.scavenging";
                return {};
            }
            if (plot_.contains(pt))
            {
                // the nearer lobe under the pointer
                const Train t = trainOf(ctx_.doc);
                const double a = angleAt(pt.x);
                return t.in.lift01(a) >= t.ex.lift01(a) ? (t.in.lift01(a) > 0.0 ? "valve.intake" : "") : "valve.exhaust";
            }
            if (ring_.contains(pt))
                return "valve.timing";
            if (compact_ && bench_.contains(pt))
                return pt.y < bench_.y + 30.0f ? "" : tab_ == 0 ? "valve.head" : tab_ == 1 ? "valve.phasers" : "valve.springs";
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
            for (double a = kA0; a <= span() + 1e-9; a += 2.0)
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
            for (double a = kA0; a <= span() + 1e-9; a += 2.0)
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
            lk::region(c, p, editor_, "CAM EDITOR", sel);
            const Rect e = content(editor_);
            const Units& u = ctx_.units;
            const Lobe& li = lobe(t, true);
            const Lobe& le = lobe(t, false);
            // the cams as they run now: each phaser's shift, and the overlap that makes (the simulator's when it says)
            const double inPh = ctx_.live.get(kTelInPhase), exPh = ctx_.live.get(kTelExPhase);
            const double inShift = phaseShift(true, inPh), exShift = phaseShift(false, exPh);
            Events ev = eventsOf(li, le);
            if (const double ov = ctx_.live.get(kTelEvents[4]); std::isfinite(ov))
                ev.overlap = ov;
            else if (std::isfinite(ev.overlap))
                ev.overlap = std::max(0.0, ev.overlap - inShift + exShift);
            // the piston's room: the least between each valve and the crown near the overlap's top, and where
            const en::Layout lay = en::layoutOf(ctx_.doc);
            const en::Cylinder cyl = en::cylinderOf(ctx_.doc, lay);
            const bool piston = cyl.known() && std::isfinite(cyl.compression) && cyl.compression > 1.05 && metres(t.in);
            const double clear = piston ? cyl.stroke / (cyl.compression - 1.0) : en::kNaN;
            const auto room = [&](double a) { return clear + en::pistonDrop(a - 360.0, cyl.radius(), cyl.rod); };
            double leastIn = en::kNaN, leastEx = en::kNaN, atIn = en::kNaN, atEx = en::kNaN;
            if (piston)
            {
                leastIn = leastEx = 1e9;
                for (double a = 300.0; a <= 420.0; a += 0.5)
                {
                    if (const double r = room(a) - li.liftAt(a - inShift); r < leastIn)
                    {
                        leastIn = r;
                        atIn = a;
                    }
                    if (const double r = room(a) - le.liftAt(a - exShift); r < leastEx)
                    {
                        leastEx = r;
                        atEx = a;
                    }
                }
            }
            const auto len = [&](double m) { return std::isfinite(m) ? u.number(Quantity::length, m) : std::string("-"); };
            const std::string lu = u.unit(Quantity::length).name;
            // the numbers that matter at a glance, over the plot, as many as fit
            struct Stat
            {
                const char* label;
                std::string value;
                std::string unit;
                Col ink;
            };
            const Stat stats[] = {
                { "OVERLAP", deg(ev.overlap), "deg", kClear },
                { "ROOM, INTAKE", len(leastIn), lu, std::isfinite(leastIn) && leastIn < 0.001 ? p.warning : kClear },
                { "ROOM, EXHAUST", len(leastEx), lu, std::isfinite(leastEx) && leastEx < 0.001 ? p.warning : kClear },
                { "LOBE SEPARATION", lk::num(ev.lsa, "%.1f"), "deg", kClear },
            };
            float x = e.x;
            for (const Stat& st : stats)
            {
                const float w = std::max(c.textWidth(st.label, lk::kCaps), c.textWidth(st.value.c_str(), lk::kNumberS) + 30.0f);
                if (x + w > e.right())
                    break;
                stage::readout(c, p, x, e.y - 4.0f, st.label, st.value, st.unit, lk::kNumberS, st.ink);
                x += w + 20.0f;
            }
            // what drives it, in words, on the title's line at the right
            const char* note = t.reference ? "THE REFERENCE'S CAM DIAL"
                             : t.labOff    ? "LAB OFF: THE ENGINE RUNS THE DIAL"
                             : t.editable  ? "DRAG A PEAK OR A SEAT MARK"
                                           : "";
            put(c, note, editor_.right() - lk::kPad, editor_.y + 9.0f, lk::kCaps, t.labOff ? p.warning : p.label, Align::right);

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
            const funkgui::Canvas::ClipScope clip(c, { plot_.x, plot_.y, plot_.w, plot_.h + 62.0f });   // the plot and its key
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
                }
            }
            // the other stage behind, ghosted and named
            if (t.stages)
            {
                const Lobe& gi = ghost(t, true);
                const Lobe& ge = ghost(t, false);
                curve(c, gi, 0.0, p.over(p.cold, 0.5f), kClear, 1.25f);
                curve(c, ge, 0.0, p.over(p.hot, 0.5f), kClear, 1.25f);

            }
            // the lobes parked, ghosted when a phaser has moved them; then as they run now
            if (inShift != 0.0)
                curve(c, li, 0.0, p.over(p.cold, 0.4f), kClear, 1.0f);
            if (exShift != 0.0)
                curve(c, le, 0.0, p.over(p.hot, 0.4f), kClear, 1.0f);
            curve(c, le, exShift, p.hot, kClear, lk::kTrace);
            curve(c, li, inShift, p.cold, kClear, lk::kTrace);
            // the second exhaust event (a compression brake's small lobe), when on
            if (t.aux.known())
                curve(c, t.aux, exShift, p.hot, p.over(p.hot, 0.2f), 1.5f);
            // the phasers: their ranges as bands along the top, where each is now
            const auto band = [&](const Lobe& l, double range, double now, bool intake, float y) {
                if (!(range > 0.0) || !l.known())
                    return;
                const double a0 = intake ? l.centre - range : l.centre, a1 = intake ? l.centre : l.centre + range;
                const Col ink = intake ? p.cold : p.hot;
                c.rrect(X(a0), y, X(a1) - X(a0), 8.0f, 4.0f, p.over(ink, 0.3f), 1.0f, p.over(ink, 0.7f));
                c.segment(X(l.centre), y - 2.0f, X(l.centre), y + 10.0f, 1.0f, p.label);
                // where it is asked to go (a ring), and where it is (a disc)
                const double target = ctx_.live.get(intake ? kTelInTarget : kTelExTarget);
                if (std::isfinite(target))
                    c.disc(X(l.centre + phaseShift(intake, target)), y + 4.0f, 6.5f, kClear, 1.5f, p.value);
                if (std::isfinite(now))
                {
                    const float nx = X(l.centre + phaseShift(intake, now));
                    c.disc(nx, y + 4.0f, 5.0f, ink, 1.5f, p.ground);
                }
                // held parked by cold or thin oil (the lab's rule): said, so a parked phaser does not look dead
                const OilHold oil = oilHold(ctx_.live, kPhaserOilTemp, kPhaserOilPressure);
                const std::string w = std::string(intake ? "INTAKE PHASER " : "EXHAUST PHASER ")
                                      + (oil.held ? std::string(oil.cold ? "PARKED: OIL COLD" : "PARKED: OIL PRESSURE LOW")
                                                  : deg(range) + " DEG");
                // (clear of the target's ring, which stands at the band's end when it asks the whole range)
                put(c, w, intake ? X(a0) - 12.0f : X(a1) + 12.0f, y - 1.0f, lk::kCaps, ink, intake ? Align::right : Align::left);
            };
            band(li, t.inPhaseRange, inPh, true, plot_.y + 8.0f);
            band(le, t.exPhaseRange, exPh, false, plot_.y + 8.0f);
            const char* st = t.stages ? (stageB(t) ? " B" : " A") : "";
            put(c, std::string("EXHAUST") + st, X(le.centre + exShift), Y(le.liftAt(le.centre)) - 24.0f, lk::kCaps, p.hot, Align::centre);
            put(c, std::string("INTAKE") + st, X(li.centre + inShift), Y(li.liftAt(li.centre)) - 24.0f, lk::kCaps, p.cold, Align::centre);
            if (std::isfinite(ev.overlap) && ev.overlap > 0.0)
                put(c, "OVERLAP", X(360.0), plot_.bottom() - 30.0f, lk::kCaps, p.value, Align::centre);
            // the key to what is faint, under the axis: as many to a line as fit
            {
                std::vector<std::string> keys;
                if (t.stages && ghost(t, true).known())
                    keys.push_back(stageB(t) ? "FAINT: STAGE A" : "FAINT: STAGE B");
                if (inShift != 0.0 || exShift != 0.0)
                    keys.push_back("FAINTER: PARKED");
                if (t.aux.known())
                    keys.push_back("SMALL LOBE: THE SECOND EXHAUST EVENT");
                if (piston)
                {
                    keys.push_back("GREY: THE PISTON'S CROWN, IN " + lu);
                    keys.push_back("BARS: WHERE A VALVE IS NEAREST IT");
                }
                float kx = plot_.x, ky = plot_.bottom() + 34.0f;
                for (const std::string& k : keys)
                {
                    const float w = c.textWidth(k.c_str(), lk::kCaps);
                    if (kx > plot_.x && kx + w > plot_.right())
                    {
                        kx = plot_.x;
                        ky += 14.0f;
                    }
                    put(c, k, kx, ky, lk::kCaps, p.label);
                    kx += w + 18.0f;
                }
            }
            // where each valve comes nearest the crown: a bar from the valve to the crown (the stats row has the numbers)
            const auto mark = [&](double a, const Lobe& l, double shift, Col ink) {
                if (!std::isfinite(a))
                    return;
                const float mx = X(a), y0 = Y(l.liftAt(a - shift)), y1 = Y(room(a));
                c.segment(mx, y0, mx, y1, 1.5f, ink);
                c.disc(mx, y0, 2.5f, ink);
                c.disc(mx, y1, 2.5f, ink);
            };
            if (piston)
            {
                mark(atEx, le, exShift, p.hot);
                mark(atIn, li, inShift, p.cold);
            }
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
            // the simulator's events now (cylinder 1's, phased: the cycle's degrees) when telemetry has them, else the
            // cams' at the seat
            Events ev = eventsOf(lobe(t, true), lobe(t, false));
            double now[5];
            bool live = true;
            for (int i = 0; i < 5; ++i)
                live = std::isfinite(now[i] = ctx_.live.get(kTelEvents[i])) && live;
            if (live)
            {
                ev.ivo = 360.0 - now[0];
                ev.ivc = now[1] - 540.0;
                ev.evo = 180.0 - now[2];
                ev.evc = now[3] - 360.0;
                ev.overlap = now[4];
            }
            const char* ringTitle = live ? "TIMING, LIVE" : "TIMING, AT THE SEAT";
            lk::region(c, p, ring_, ringTitle, ctx_.selection.is("valve.timing"));
            const float titleEnd = ring_.x + lk::kPad + c.textWidth(ringTitle, lk::kCaps);
            const Rect in = content(ring_);
            const float R = std::max(20.0f, std::min(in.w * 0.5f - 24.0f, (in.h - 84.0f) * 0.5f));
            const Point ctr{ in.centreX(), in.y + 18.0f + R };
            c.arc(ctr, R + 10.0f, 0.0f, static_cast<float>(2.0 * stage::kPi), 1.0f, p.grid);
            // TDC up, BDC down
            // TDC over its tick, on the title's line: moved right of a title long enough to reach it
            const float tdcW = c.textWidth("TDC", lk::kCaps);
            put(c, "TDC", std::max(ctr.x, titleEnd + 10.0f + tdcW * 0.5f), ctr.y - R - 30.0f, lk::kCaps, p.text, Align::centre);
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
            lk::region(c, p, bench_, compact_ ? "" : "FLOW BENCH, 28 IN OF WATER", ctx_.selection.is("valve.head"));
            const Rect in = content(bench_);
            const float down = compact_ ? 10.0f : 0.0f;   // under the tabs
            const Rect plot{ in.x + 40.0f, in.y + 14.0f + down, in.w - 44.0f, in.h - 14.0f - down - 30.0f };
            const Units& u = ctx_.units;
            const en::Layout lay = en::layoutOf(ctx_.doc);
            const en::Cylinder cyl = en::cylinderOf(ctx_.doc, lay);
            const Bench b = benchOf(ctx_.doc, cyl.bore, ctx_.live);
            c.rrect(plot.x, plot.y, plot.w, plot.h, 0.0f, kClear, 1.0f, p.grid);
            if (b.known)
                put(c, b.simulated ? "THE SIMULATOR'S" : "FROM THE HEAD'S FIELDS", plot.right(), in.y - 2.0f + down, lk::kCaps, p.dim,
                    Align::right);
            put(c, "CFM", plot.x - 40.0f, in.y - 2.0f + down, lk::kCaps, p.label);
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
                // on the curve drawn: between its points (the first from 0)
                const auto mark = [&](double l, const std::array<double, 16>& q, Col ink) {
                    if (!(l > 0.0))
                        return;
                    const double lc = std::min(l, lmax);
                    double l0 = 0.0, q0 = 0.0, f = q[15];
                    for (int i = 0; i < 16; ++i)
                    {
                        const double l1 = b.lift[static_cast<std::size_t>(i)], q1 = q[static_cast<std::size_t>(i)];
                        if (lc <= l1)
                        {
                            f = q0 + (q1 - q0) * (lc - l0) / std::max(1e-9, l1 - l0);
                            break;
                        }
                        l0 = l1;
                        q0 = q1;
                    }
                    c.disc(X(lc), Y(f), 4.5f, ink, 1.5f, p.ground);
                };
                mark(li, b.flowIn, p.cold);
                mark(le, b.flowEx, p.hot);
            }
        }

        // ---- phasers and stages ----------------------------------------------------------------------------------------
        void drawPhasers(Canvas& c, const lk::Palette& p, const Train& t) const
        {
            if (!hasPhasers_)
            {
                lk::region(c, p, phasers_, compact_ ? "" : "PHASERS AND STAGES: NONE HERE YET", ctx_.selection.is("valve.phasers"));
                if (compact_)
                    putCentred(c, "NO PHASERS OR STAGES IN THIS BUILD YET", phasers_.centreX(), phasers_.centreY(), lk::kCaps, p.dim);
                return;
            }
            lk::region(c, p, phasers_, compact_ ? "" : "PHASERS AND STAGES", ctx_.selection.is("valve.phasers"));
            const Rect in = content(phasers_);
            // four cells, two by two: each a label, its value, and a bar or a lamp under it
            const float cw = std::floor(in.w * 0.5f), ch = std::floor(in.h * 0.5f);
            const auto cell = [&](int i) { return Point{ in.x + cw * static_cast<float>(i % 2), in.y + ch * static_cast<float>(i / 2) }; };
            const auto phaser = [&](int i, const char* name, double range, double now, double target, Col ink) {
                const Point at = cell(i);
                put(c, name, at.x, at.y, lk::kCaps, p.label);
                if (!(range > 0.0))
                {
                    put(c, std::isfinite(range) ? "none" : "-", at.x, at.y + 14.0f, lk::kBody, p.dim);
                    return;
                }
                std::string txt = (std::isfinite(now) ? deg(now) : std::string("-")) + " of " + deg(range) + " deg";
                if (std::isfinite(target) && std::isfinite(now) && std::fabs(target - now) >= 0.5)
                    txt += ", to " + deg(target);
                put(c, txt, at.x, at.y + 14.0f, lk::kBody, p.value);
                lk::level(c, p, { at.x, at.y + 31.0f, cw - 14.0f, 4.0f }, std::isfinite(now) ? now / range : en::kNaN, ink);
                // parked by the oil (the lab's rule: 313 K and 70 kPa), in words, with what it waits for
                if (const std::string held = phaserHeldText(); !held.empty())
                    put(c, held, at.x, at.y + 40.0f, lk::kCaps, p.warning);
            };
            phaser(0, "INTAKE PHASER", t.inPhaseRange, ctx_.live.get(kTelInPhase), ctx_.live.get(kTelInTarget), p.cold);
            phaser(1, "EXHAUST PHASER", t.exPhaseRange, ctx_.live.get(kTelExPhase), ctx_.live.get(kTelExTarget), p.hot);
            // the stages: which is engaged now, in words
            {
                const Point at = cell(2);
                put(c, "STAGES", at.x, at.y, lk::kCaps, p.label);
                if (!t.stages)
                    put(c, std::isfinite(fieldValue(ctx_.doc, VF::stages)) ? "one" : "-", at.x, at.y + 14.0f, lk::kBody, p.dim);
                else
                {
                    const double st = ctx_.live.get(kTelStage);
                    const std::string sp = std::isfinite(t.stageSpeed) ? ctx_.units.format(Quantity::rotation, t.stageSpeed, 0) : "-";
                    put(c, "B from " + sp, at.x, at.y + 14.0f, lk::kBody, p.value);
                    const std::string now = !std::isfinite(st) ? "ENGAGED -" : st >= 0.5 ? "B ENGAGED" : "A ENGAGED";
                    lk::lamp(c, p, at.x, at.y + 36.0f, now, std::isfinite(st) && st >= 0.5, p.selected);
                    // B waits for warm oil with pressure (the lab's rule: 333 K and 100 kPa), in words
                    if (const std::string held = stageHeldText(); !held.empty())
                        put(c, held, at.x, at.y + 52.0f, lk::kCaps, p.warning);
                }
            }
            // continuous lift
            {
                const Point at = cell(3);
                put(c, "CONTINUOUS LIFT", at.x, at.y, lk::kCaps, p.label);
                const double lc = fieldValue(ctx_.doc, VF::liftControl), lnow = ctx_.live.get(kTelLift);
                if (!(lc >= 0.5))
                    put(c, std::isfinite(lc) ? "off" : "-", at.x, at.y + 14.0f, lk::kBody, p.dim);
                else
                    put(c, (std::isfinite(lnow) ? lk::num(lnow * 100.0) + " %" : std::string("-")) + " of the lift", at.x,
                        at.y + 14.0f, lk::kBody, p.value);
            }
        }

        // ---- live: float and deactivation --------------------------------------------------------------------------------
        void drawLive(Canvas& c, const lk::Palette& p, const Train&) const
        {
            if (!hasLive_)
            {
                lk::region(c, p, live_, compact_ ? "" : "SPRINGS AND CYLINDERS: NONE HERE YET", ctx_.selection.is("valve.springs"));
                if (compact_)
                    putCentred(c, "NO SPRINGS OR CYLINDERS TO SHOW YET", live_.centreX(), live_.centreY(), lk::kCaps, p.dim);
                return;
            }
            lk::region(c, p, live_, compact_ ? "" : "SPRINGS AND CYLINDERS, LIVE", ctx_.selection.is("valve.springs"));
            const Rect in = content(live_);
            const Units& u = ctx_.units;
            // valve float: the speed it would float at, and how far below it the engine is
            const double fl = ctx_.live.get(kTelFloat), w = ctx_.live.get(en::kTelSpeed);
            const double margin = std::isfinite(fl) && std::isfinite(w) ? fl - w : en::kNaN;
            const bool floating = ctx_.live.get(kTelFloating) >= 0.5;
            const bool near = floating || (std::isfinite(margin) && margin < 500.0 * stage::kPi / 30.0);
            const float wa = stage::readout(c, p, in.x, in.y, "FLOATS AT", std::isfinite(fl) ? u.number(Quantity::rotation, fl) : "-",
                                         u.unit(Quantity::rotation).name, lk::kNumberS);
            stage::readout(c, p, in.x + std::max(96.0f, wa + 24.0f), in.y, floating ? "FLOATING NOW" : near ? "NEAR FLOAT" : "MARGIN",
                        std::isfinite(margin) ? u.number(Quantity::rotation, margin) : "-", u.unit(Quantity::rotation).name,
                        lk::kNumberS, near ? p.warning : kClear);
            const double rate = fieldValue(ctx_.doc, VF::springRate);
            if (std::isfinite(rate) && rate <= 0.0)
                put(c, "SPRING RATE 0: NO FLOAT", in.x, in.y + 36.0f, lk::kCaps, p.dim);
            // deactivation: each cylinder, those that can rest outlined, those resting now hatched and named
            const en::Layout lay = en::layoutOf(ctx_.doc);
            const double mask = fieldValue(ctx_.doc, VF::deacMask), act = ctx_.live.get(kTelActive);
            const float y = in.y + 44.0f;
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
            std::string s = std::isfinite(act) ? std::to_string(resting) + " RESTING" : "RESTING: -";
            put(c, s, in.x, y + 48.0f, lk::kCaps, resting > 0 ? p.warning : p.label);
            put(c, "OUTLINED: MAY REST", in.right(), y + 48.0f, lk::kCaps, p.dim, Align::right);
        }

        // ==== a two-stroke: PORT TIMING ====================================================================================
        void readPortScope()
        {
            auto get = [&](const char* name, std::vector<float>& out) {
                out.assign(1080, 0.0f);
                const int h = ctx_.scope.find(name);
                const int n = h < 0 ? 0 : ctx_.scope.read(h, out.data(), 1080);
                out.resize(static_cast<std::size_t>(std::max(0, n)));
                return n;
            };
            const int e = get("liftExhaust", shareEx_);
            get("liftIntake", shareTr_);
            get("portIntake", crankcase_);
            get("inletOpen", inlet_);   // the inlet's measured opening by degree (card T; 0 to 1)
            portDeg_ = e;
            ++scopeVersion_;
        }
        // The Scope's arrays are a two-stroke's (360 degrees) and so can be drawn here.
        bool portScope() const { return portDeg_ == 360; }
        // The Scope has the inlet's measured opening over this turn (a two-stroke's, not all shut).
        bool inletMeasured() const
        {
            return portScope() && inlet_.size() >= 360 && *std::max_element(inlet_.begin(), inlet_.end()) > 0.0f;
        }

        // The ports' open shares against the crank's angle over one turn: the exhaust's (with the power valve shut,
        // ghosted), the transfers', a timed inlet's window; the crankcase's pressure on its own scale at the right.
        void drawPorts(Canvas& c, const lk::Palette& p, const engine::TwoStroke& t) const
        {
            lk::region(c, p, editor_, "PORTS: OPEN AGAINST CRANK ANGLE", ctx_.selection.is("valve.ports"));
            const Rect e = content(editor_);
            const en::Layout lay = en::layoutOf(ctx_.doc);
            const en::Cylinder cyl = en::cylinderOf(ctx_.doc, lay);
            const bool geo = cyl.known() && std::isfinite(t.exDur);
            const double r = cyl.radius(), L = std::isfinite(cyl.rod) ? cyl.rod : 3.3 * r;
            const Units& u = ctx_.units;
            const double exNow = en::exhaustDurationNow(t, ctx_.live);
            // the numbers a tuner quotes
            const auto len = [&](double m) { return std::isfinite(m) ? u.number(Quantity::length, m) : std::string("-"); };
            const std::string lu = u.unit(Quantity::length).name;
            struct Stat
            {
                const char* label;
                std::string value, unit;
            };
            const Stat stats[] = {
                { "EXHAUST OPEN", deg(exNow), "deg" },
                { "TRANSFERS OPEN", deg(t.trDur), "deg" },
                { "BLOWDOWN", deg(0.5 * (exNow - t.trDur)), "deg" },
                { "EXHAUST HEIGHT", len(geo ? en::portHeight(exNow, r, L) : en::kNaN), lu },
                { "TRANSFER HEIGHT", len(geo ? en::portHeight(t.trDur, r, L) : en::kNaN), lu },
            };
            float x = e.x;
            for (const Stat& st : stats)
            {
                const float w = std::max(c.textWidth(st.label, lk::kCaps), c.textWidth(st.value.c_str(), lk::kNumberS) + 30.0f);
                if (x + w > e.right())
                    break;
                stage::readout(c, p, x, e.y - 4.0f, st.label, st.value, st.unit, lk::kNumberS);
                x += w + 20.0f;
            }
            put(c, "DRAG A PORT'S MARK", editor_.right() - lk::kPad, editor_.y + 9.0f, lk::kCaps, p.label, Align::right);
            // the axes: top, bottom and top again; the share open up the side
            c.rrect(plot_.x, plot_.y, plot_.w, plot_.h, 0.0f, kClear, 1.0f, p.grid);
            const char* marks[3] = { "TDC", "BDC", "TDC" };
            for (int i = 0; i < 3; ++i)
            {
                const float mx = X(180.0 * i);
                c.hairlineV(mx, plot_.y, plot_.h, p.dim);
                put(c, marks[i], mx, plot_.bottom() + 6.0f, lk::kCaps, p.text, i == 0 ? Align::left : i == 2 ? Align::right : Align::centre);
            }
            for (int d = 90; d < 360; d += 180)
                put(c, std::to_string(d), X(d), plot_.bottom() + 6.0f, lk::kUnit, p.dim, Align::centre);
            const auto Ys = [&](double share) { return plot_.bottom() - static_cast<float>(std::clamp(share, 0.0, 1.0) * 0.82) * plot_.h; };
            for (int i = 1; i <= 4; ++i)
            {
                const float y = Ys(i * 0.25);
                c.hairlineH(plot_.x, y, plot_.w, p.over(p.grid, 0.6f));
                put(c, std::to_string(i * 25), plot_.x - 6.0f, c.capCentreTop(y, lk::kUnit), lk::kUnit, p.label, Align::right);
            }
            put(c, "OPEN, %", plot_.x - 44.0f, plot_.y - 16.0f, lk::kCaps, p.label);
            if (!geo)
            {
                putCentred(c, "NO BORE AND STROKE IN THIS BUILD YET", plot_.centreX(), plot_.centreY(), lk::kCaps, p.dim);
                return;
            }
            const funkgui::Canvas::ClipScope clip(c, { plot_.x, plot_.y, plot_.w, plot_.h + 62.0f });
            // a timed inlet's window, shaded behind
            double from = 0.0, dur = 0.0;
            const bool timed = t.inletWindow(from, dur);
            const bool measured = inletMeasured();
            if (timed && !measured)
            {
                const auto band = [&](double a0, double a1) {
                    if (a1 > a0)
                        c.rrect(X(a0), plot_.y + 1.0f, X(a1) - X(a0), plot_.h - 2.0f, 0.0f, p.over(p.cold, 0.1f));
                };
                const double a0 = en::wrap(from, 360.0), a1 = a0 + dur;
                band(a0, std::min(a1, 360.0));
                if (a1 > 360.0)
                    band(0.0, a1 - 360.0);
            }
            // a share's curve over the turn: the Scope's when it is a two-stroke's, else from the geometry
            const auto shareCurve = [&](double d, const std::vector<float>* fromScope, Col ink, float width, bool fillUnder) {
                std::vector<float> xs, ys;
                for (double a = 0.0; a <= 360.0 + 1e-9; a += 1.0)
                {
                    const double v = fromScope != nullptr && portScope()
                        ? (*fromScope)[static_cast<std::size_t>(std::min(359, static_cast<int>(a)))]
                        : en::portShare(a, d, r, L);
                    xs.push_back(X(a));
                    ys.push_back(Ys(v));
                }
                if (fillUnder)
                    c.areaStrip(xs.data(), static_cast<int>(xs.size()), ys.data(), nullptr, plot_.bottom(), p.over(ink, 0.14f));
                c.polyline(xs.data(), ys.data(), static_cast<int>(xs.size()), width, ink);
            };
            if (t.hasValve() && std::fabs(exNow - t.valveDur) > 0.5)
                shareCurve(t.valveDur, nullptr, p.over(p.hot, 0.45f), 1.25f, false);    // the valve shut
            if (t.hasValve() && std::fabs(exNow - t.exDur) > 0.5)
                shareCurve(t.exDur, nullptr, p.over(p.hot, 0.45f), 1.25f, false);       // the valve open
            shareCurve(exNow, &shareEx_, p.hot, lk::kTrace, true);
            shareCurve(t.trDur, &shareTr_, p.cold, lk::kTrace, true);
            // the inlet's opening as the simulator measured it (a reed's by the crankcase's pull, a disc's or port's by
            // its window), in place of the window drawn from the fields
            if (measured)
            {
                std::vector<float> xs, ys;
                for (int i = 0; i <= 360; ++i)
                {
                    xs.push_back(X(i));
                    ys.push_back(Ys(inlet_[static_cast<std::size_t>(i % 360)]));
                }
                c.polyline(xs.data(), ys.data(), static_cast<int>(xs.size()), 1.5f, lk::Palette::mix(p.cold, p.value, 0.45f));
            }
            // the crankcase's pressure, when the Scope has it, on its own scale (kPa at the right)
            if (portScope() && crankcase_.size() >= 360)
            {
                double lo = 1e9, hi = 0.0;
                for (float v : crankcase_)
                {
                    lo = std::min(lo, static_cast<double>(v));
                    hi = std::max(hi, static_cast<double>(v));
                }
                lo = std::min(lo, 101325.0) - 5e3;
                hi = std::max(hi, 101325.0) + 5e3;
                const auto Yp = [&](double pa) { return plot_.bottom() - static_cast<float>((pa - lo) / (hi - lo) * 0.82) * plot_.h; };
                std::vector<float> xs, ys;
                for (int i = 0; i <= 360; ++i)
                {
                    xs.push_back(X(i));
                    ys.push_back(Yp(crankcase_[static_cast<std::size_t>(i % 360)]));
                }
                c.polyline(xs.data(), ys.data(), static_cast<int>(xs.size()), 1.25f, p.label);
                c.segment(plot_.right() - 6.0f, Yp(101325.0), plot_.right(), Yp(101325.0), 1.0f, p.label);
            }
            // labels at each curve, the crank now, the marks to drag
            put(c, "EXHAUST", X(180.0), Ys(1.0) - 14.0f, lk::kCaps, p.hot, Align::centre);
            put(c, "TRANSFERS", X(180.0), Ys(0.35), lk::kCaps, p.cold, Align::centre);
            const double a = crankAngle360();
            if (std::isfinite(a))
            {
                c.segment(X(a), plot_.y, X(a), plot_.bottom(), 1.5f, p.value);
                c.disc(X(a), plot_.y, 3.5f, p.value);
            }
            for (int h = 0; h < 4; ++h)
            {
                const Point q = portHandle(h, t);
                const bool on = kPortHandle + h == drag_ || kPortHandle + h == hot_;
                c.rrect(q.x - 4.0f, q.y - 9.0f, 8.0f, 12.0f, 2.0f, on ? p.selected : (h < 2 ? p.hot : p.cold), 1.0f, p.ground);
            }
            // the key, under the axis
            std::vector<std::string> keys;
            if (t.hasValve())
                keys.push_back("FAINT: THE EXHAUST WITH THE POWER VALVE SHUT AND OPEN");
            if (measured)
                keys.push_back(std::string("PALE: THE ") + t.inletName() + "'S OPENING, MEASURED");
            else if (timed)
                keys.push_back(std::string("SHADED: THE ") + t.inletName() + " OPEN");
            else
                keys.push_back("THE REED OPENS BY THE CRANKCASE'S PULL: NO FIXED TIMING");
            if (portScope() && crankcase_.size() >= 360)
                keys.push_back("GREY: THE CRANKCASE'S PRESSURE, THE TICK AT THE AIR OUTSIDE");
            float kx = plot_.x, ky = plot_.bottom() + 22.0f;
            for (const std::string& k : keys)
            {
                const float w = c.textWidth(k.c_str(), lk::kCaps);
                if (kx > plot_.x && kx + w > plot_.right())
                {
                    kx = plot_.x;
                    ky += 14.0f;
                }
                put(c, k, kx, ky, lk::kCaps, p.label);
                kx += w + 18.0f;
            }
        }

        double crankAngle360() const
        {
            const double c = ctx_.live.get(en::kTelCrank);
            return std::isfinite(c) ? en::wrap(c, 360.0) : en::kNaN;
        }

        // The classic port-timing diagram: one turn, top up, bottom down; the exhaust's open period the outer arc, the
        // transfers' inside it, a timed inlet's innermost; each about bottom dead centre.
        void drawPortRing(Canvas& c, const lk::Palette& p, const engine::TwoStroke& t) const
        {
            lk::region(c, p, ring_, "PORT TIMING", ctx_.selection.is("valve.ports"));
            const Rect in = content(ring_);
            const double exNow = en::exhaustDurationNow(t, ctx_.live);
            const float R = std::max(20.0f, std::min(in.w * 0.5f - 24.0f, (in.h - 84.0f) * 0.5f));
            const Point ctr{ in.centreX(), in.y + 18.0f + R };
            c.arc(ctr, R + 10.0f, 0.0f, static_cast<float>(2.0 * stage::kPi), 1.0f, p.grid);
            put(c, "TDC", ctr.x, ctr.y - R - 30.0f, lk::kCaps, p.text, Align::centre);
            put(c, "BDC", ctr.x, ctr.y + R + 18.0f, lk::kCaps, p.text, Align::centre);
            c.segment(ctr.x, ctr.y - R - 16.0f, ctr.x, ctr.y + R + 16.0f, 1.0f, p.dim);
            if (std::isfinite(exNow))
                c.arc(ctr, R, stage::arcAngle(180.0 - 0.5 * exNow), stage::arcAngle(180.0 + 0.5 * exNow), 7.0f, p.hot);
            if (std::isfinite(t.trDur))
                c.arc(ctr, R - 13.0f, stage::arcAngle(180.0 - 0.5 * t.trDur), stage::arcAngle(180.0 + 0.5 * t.trDur), 7.0f, p.cold);
            double from = 0.0, dur = 0.0;
            bool inletKnown = t.inletWindow(from, dur);
            if (inletMeasured())
            {
                // the measured opening's span: from where it rises above a twentieth to where it falls back
                int start = -1, len = 0, bestStart = -1, bestLen = 0;
                for (int i = 0; i < 720; ++i)
                {
                    const bool open = inlet_[static_cast<std::size_t>(i % 360)] > 0.05f;
                    if (open)
                    {
                        if (start < 0)
                            start = i;
                        ++len;
                        if (len > bestLen && len <= 360)
                        {
                            bestLen = len;
                            bestStart = start;
                        }
                    }
                    else
                    {
                        start = -1;
                        len = 0;
                    }
                }
                inletKnown = bestLen > 0;
                from = bestStart % 360;
                dur = bestLen;
            }
            if (inletKnown)
                c.arc(ctr, R - 26.0f, stage::arcAngle(from), stage::arcAngle(from + dur), 5.0f, p.over(p.cold, 0.6f));
            const auto corner = [&](float x, float y, Align al, const char* name, const std::string& v, Col ink) {
                put(c, name, x, y, lk::kCaps, ink, al);
                put(c, v, x, y + 13.0f, lk::kBody, p.value, al);
            };
            corner(in.x, in.y, Align::left, "EXHAUST", deg(exNow) + " deg", p.hot);
            corner(in.right(), in.y, Align::right, "TRANSFERS", deg(t.trDur) + " deg", p.cold);
            corner(in.x, in.bottom() - 26.0f, Align::left, "BLOWDOWN", deg(0.5 * (exNow - t.trDur)) + " deg", p.label);
            corner(in.right(), in.bottom() - 26.0f, Align::right, "INLET",
                   inletKnown ? deg(dur) + " deg" : std::string("reed"), p.label);
            putCentred(c, "EXHAUST OPENS", ctr.x, ctr.y - 8.0f, lk::kCaps, p.label);
            putCentred(c, deg(180.0 - 0.5 * exNow) + " ATDC", ctr.x, ctr.y + 8.0f, lk::kNumberS, p.value);
        }

        // The power valve: the exhaust's duration (and its port's height) against the engine's speed, the speed now.
        void drawPowerValve(Canvas& c, const lk::Palette& p, const engine::TwoStroke& t) const
        {
            lk::region(c, p, bench_, compact_ ? "" : "POWER VALVE", ctx_.selection.is("valve.powervalve"));
            const Rect in = content(bench_);
            const float down = compact_ ? 10.0f : 0.0f;
            if (!t.hasValve())
            {
                putCentred(c, "NO POWER VALVE: THE EXHAUST OPENS " + deg(t.exDur) + " DEG AT EVERY SPEED", bench_.centreX(),
                           bench_.centreY(), lk::kCaps, p.dim);
                return;
            }
            const Rect plot{ in.x + 40.0f, in.y + 14.0f + down, in.w - 44.0f, in.h - 14.0f - down - 30.0f };
            const Units& u = ctx_.units;
            const double w1 = std::max(t.valveTo, t.valveFrom) * 1.4;
            const double d0 = std::min(t.valveDur, t.exDur) - 6.0, d1 = std::max(t.valveDur, t.exDur) + 6.0;
            const auto Xs = [&](double w) { return plot.x + static_cast<float>(w / w1) * plot.w; };
            const auto Yd = [&](double d) { return plot.bottom() - static_cast<float>((d - d0) / (d1 - d0)) * plot.h; };
            c.rrect(plot.x, plot.y, plot.w, plot.h, 0.0f, kClear, 1.0f, p.grid);
            put(c, "EXHAUST OPEN, DEG", plot.x - 40.0f, in.y - 2.0f + down, lk::kCaps, p.label);
            for (const double d : { t.valveDur, t.exDur })
                put(c, deg(d), plot.x - 5.0f, c.capCentreTop(Yd(d), lk::kUnit), lk::kUnit, p.label, Align::right);
            // the speed axis ticked where the valve starts and ends its travel
            for (const double w : { t.valveFrom, t.valveTo })
            {
                c.hairlineV(Xs(w), plot.y, plot.h, p.over(p.grid, 0.6f));
                put(c, u.number(Quantity::rotation, w, 0), Xs(w), plot.bottom() + 4.0f, lk::kUnit, p.label,
                    w == t.valveFrom ? Align::right : Align::left);
            }
            put(c, "0", plot.x, plot.bottom() + 4.0f, lk::kUnit, p.label);
            put(c, std::string("SPEED, ") + u.unit(Quantity::rotation).name, plot.right(), plot.bottom() + 16.0f, lk::kCaps, p.label,
                Align::right);
            const float xs[4] = { plot.x, Xs(t.valveFrom), Xs(t.valveTo), plot.right() };
            const float ys[4] = { Yd(t.valveDur), Yd(t.valveDur), Yd(t.exDur), Yd(t.exDur) };
            c.polyline(xs, ys, 4, lk::kTrace, p.hot);
            put(c, "SHUT", plot.x + 6.0f, Yd(t.valveDur) - 16.0f, lk::kCaps, p.hot);
            put(c, "OPEN", plot.right() - 6.0f, Yd(t.exDur) - 16.0f, lk::kCaps, p.hot, Align::right);
            const double speed = ctx_.live.get(en::kTelSpeed);
            if (std::isfinite(speed))
            {
                const float sx = Xs(std::clamp(speed, 0.0, w1));
                c.segment(sx, plot.y, sx, plot.bottom(), 1.5f, p.value);
                c.disc(sx, Yd(en::exhaustDurationNow(t, ctx_.live)), 4.5f, p.hot, 1.5f, p.ground);
            }
        }

        // The crankcase: its compression, the inlet, the areas, its pressure over the turn.
        void drawCrankcase(Canvas& c, const lk::Palette& p, const engine::TwoStroke& t) const
        {
            lk::region(c, p, phasers_, compact_ ? "" : "CRANKCASE", ctx_.selection.is("valve.crankcase"));
            const Rect in = content(phasers_);
            const Units& u = ctx_.units;
            const float cw = std::floor(in.w * 0.5f), ch = std::floor(in.h * 0.5f);
            const float top = in.y + (compact_ ? 10.0f : 0.0f);
            const auto cell = [&](int i) { return Point{ in.x + cw * static_cast<float>(i % 2), top + ch * static_cast<float>(i / 2) }; };
            const auto item = [&](int i, const char* label, const std::string& value, Col ink) {
                const Point at = cell(i);
                put(c, label, at.x, at.y, lk::kCaps, p.label);
                put(c, value, at.x, at.y + 14.0f, lk::kBody, ink);
            };
            item(0, "COMPRESSION", lk::num(t.crankcaseRatio, "%.2f") + " : 1", p.value);
            item(1, "INLET", t.inletName(), p.value);
            item(2, "INLET AREA", std::isfinite(t.inletArea) ? u.format(Quantity::area, t.inletArea) : "-", p.value);
            double lo = en::kNaN, hi = en::kNaN;
            if (portScope() && !crankcase_.empty())
            {
                lo = *std::min_element(crankcase_.begin(), crankcase_.end());
                hi = *std::max_element(crankcase_.begin(), crankcase_.end());
            }
            if (const double now = ctx_.live.get(en::kTelCrankcase); std::isfinite(now) && now > 0.0)
                put(c, "NOW " + u.format(Quantity::pressure, now), cell(3).x, cell(3).y + 30.0f, lk::kCaps, p.label);
            item(3, "PRESSURE, A TURN",
                 std::isfinite(lo) ? u.number(Quantity::pressure, lo) + " to " + u.format(Quantity::pressure, hi) : std::string("-"),
                 std::isfinite(lo) ? p.value : p.dim);
        }

        // Scavenging: how the fresh charge meets the burnt (the fields; the simulator's measures when telemetry has them).
        void drawScavenging(Canvas& c, const lk::Palette& p, const engine::TwoStroke&) const
        {
            lk::region(c, p, live_, compact_ ? "" : "SCAVENGING", ctx_.selection.is("valve.scavenging"));
            const Rect in = content(live_);
            const float cw = std::floor(in.w * 0.5f);
            const float top = in.y + (compact_ ? 10.0f : 0.0f);
            const auto field = [&](const char* path) {
                const int f = ctx_.doc.find(path);
                return f < 0 ? en::kNaN : ctx_.doc.get(f);
            };
            // live, the simulator's: how much fresh charge stays, how much was sent, how pure what stays is
            const float c3 = std::floor(in.w / 3.0f);
            const auto pct = [&](const char* name) {
                const double v = ctx_.live.get(name);
                return std::isfinite(v) ? lk::num(v * 100.0, "%.0f") : std::string("-");
            };
            stage::readout(c, p, in.x, top, "PURITY", pct(en::kTelPurity), "%", lk::kNumberS);
            stage::readout(c, p, in.x + c3, top, "DELIVERY", pct(en::kTelDelivery), "%", lk::kNumberS);
            stage::readout(c, p, in.x + 2 * c3, top, "TRAPPING", pct(en::kTelTrapping), "%", lk::kNumberS);
            // the build's: how the fresh charge meets the burnt
            const double mix = field("engine.tsMixing"), shortc = field("engine.tsShortCircuit");
            stage::readout(c, p, in.x, top + 42.0f, "MIXING", lk::num(mix * 100.0, "%.0f"), "%", lk::kNumberS);
            stage::readout(c, p, in.x + cw, top + 42.0f, "SHORT-CIRCUITED", lk::num(shortc * 100.0, "%.0f"), "%", lk::kNumberS);
            if (in.h > 110.0f)
                put(c, "MIXING 0 %: THE FRESH CHARGE PUSHES THE BURNT OUT", in.x, top + 84.0f, lk::kUnit, p.dim);
        }

        int drag_ = -1, hot_ = -1;
        double yMax_ = 0.015;
        Rect editor_{}, plot_{}, ring_{}, bench_{}, phasers_{}, live_{};
        bool compact_ = false, hasPhasers_ = false, hasLive_ = false, two_ = false;
        // a two-stroke's Scope: the cycle's length, the exhaust's and transfers' shares, the crankcase's pressure
        int portDeg_ = 0;
        std::vector<float> shareEx_, shareTr_, crankcase_, inlet_;
        std::uint64_t scopeGen_ = 0;
        bool scopeRead_ = false;
        std::uint32_t scopeVersion_ = 0;
        int tab_ = 0;
        Button* tabs_[3] = {};

    public:
        std::vector<Rect> panels() const { return { editor_, ring_, bench_, phasers_, live_ }; }
        // Where handle h is now (0..2 the intake's peak, opening, closing; 3..5 the exhaust's); {-1, -1}: no handles.
        Point handle(int h) const
        {
            if (two_)
                return h >= kPortHandle && h < kPortHandle + 4 ? portHandle(h - kPortHandle, engine::twoStrokeOf(ctx_.doc))
                                                              : Point{ -1.0f, -1.0f };
            const Train t = trainOf(ctx_.doc);
            return t.editable && h >= 0 && h < handleCount ? handlePoint(h, t) : Point{ -1.0f, -1.0f };
        }
        bool compact() const noexcept { return compact_; }
    };

    std::unique_ptr<Workspace> makeValvesWorkspace(WorkspaceContext& c) { return std::make_unique<ValvesWorkspace>(c); }

    std::string valvesOilWords(const Workspace& w)
    {
        const auto* v = dynamic_cast<const ValvesWorkspace*>(&w);
        return v != nullptr ? v->oilWords() : std::string();
    }

    funkgui::Point valvesHandle(const Workspace& w, int h)
    {
        const auto* v = dynamic_cast<const ValvesWorkspace*>(&w);
        return v != nullptr ? v->handle(h) : funkgui::Point{ -1.0f, -1.0f };
    }

    std::vector<funkgui::Rect> valvesPanels(const Workspace& w)
    {
        const auto* v = dynamic_cast<const ValvesWorkspace*>(&w);
        return v != nullptr ? v->panels() : std::vector<funkgui::Rect>{};
    }
}
