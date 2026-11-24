// ui/views/exhaust/ExhaustWorkspace.cpp: see ExhaustWorkspace.h. Everything on the stage is Canvas primitives: capsule
// segments for pipes, cubics for bends, area strips for the collectors' cones, rounded rects for the cans, stroked arcs
// for the housings and the radiating ends, discs, text. Colour is never the only sign: every tint has a number or a
// word beside it, the selection a ring and a name, the valve a drawn position and a word.
#include "ExhaustWorkspace.h"

#include "ExhaustModel.h"
#include "ExhaustStage.h"

#include "app/Look.h"
#include "kit/bind/Binding.h"
#include "kit/widgets/Basic.h"
#include "views/fx/Fx.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

namespace tee::ui
{
    namespace
    {
        using namespace exhaust;
        using funkgui::Canvas;
        using funkgui::Col;
        using funkgui::Point;
        using funkgui::Rect;
        using look::Palette;
        using N = ExhaustNames;

        constexpr double kAmbient = 101325.0;
        // The heat scale's ends (K): cold below 300, the ink at 400, hottest at 1373 (1100 C), which the gas at the
        // ports reaches at full load (the integrator measured 1100 to 1330 K).
        constexpr double kHeatTop = 1373.0;
        Col heat(const Palette& p, double kelvin) { return p.heat(kelvin, 300.0, 400.0, kHeatTop); }
        constexpr float kPulseScale = 40e3f;       // Pa over the air outside at the trace's full swing
        constexpr float kPulseSwing = 15.0f;       // px
        constexpr int kHistory = 48;               // Scope copies kept for the slow replay
        constexpr float kSlow = 0.125f;            // the slow replay's rate

        // A pipe of a polyline (each joint a capsule), an edge under its fill.
        void pipeLine(Canvas& c, const std::vector<Point>& pts, float width, Col fill, Col edge)
        {
            if (pts.size() < 2)
                return;
            std::vector<float> xs(pts.size()), ys(pts.size());
            for (std::size_t i = 0; i < pts.size(); ++i)
            {
                xs[i] = pts[i].x;
                ys[i] = pts[i].y;
            }
            c.polyline(xs.data(), ys.data(), static_cast<int>(pts.size()), width + 2.0f, edge);
            c.polyline(xs.data(), ys.data(), static_cast<int>(pts.size()), width, fill);
        }

        std::vector<Point> sBendPts(Point a, Point b, int n = 16)
        {
            std::vector<Point> out;
            const float mx = (a.x + b.x) * 0.5f;
            const Point p[4] = { a, { mx, a.y }, { mx, b.y }, b };
            for (int i = 0; i <= n; ++i)
                out.push_back(bezier(p, static_cast<float>(i) / static_cast<float>(n)));
            return out;
        }

        // Text centred on x.
        void centred(Canvas& c, const std::string& s, float x, float y, const funkgui::TextStyle& st, Col ink)
        {
            c.text(s.c_str(), x - c.textWidth(s.c_str(), st) * 0.5f, y, st, ink);
        }
    }

    class ExhaustWorkspace final : public Workspace
    {
    public:
        explicit ExhaustWorkspace(WorkspaceContext& c) : Workspace(c)
        {
            slowButton_ = &make<Button>("SLOW", [this] { slow_ = !slow_; play_ = clock_; });
            slowButton_->setLit([this] { return slow_; });
        }

        const char* id() const override { return "exhaust"; }
        const char* title() const override { return "EXHAUST"; }
        std::vector<std::string> sections() const override { return { "exhaust" }; }

        // The selected element's fields, on a page of their own (those the build has; none: the frame's pages).
        std::string inspectorPage() const override
        {
            std::string title;
            const std::vector<std::string> paths = elementFields(ctx_.selection.path(), title);
            std::vector<int> fields;
            for (const std::string& path : paths)
                if (const int f = ctx_.doc.find(path); f >= 0 && !ctx_.doc.info(f).hidden)
                    fields.push_back(f);
            return fields.empty() ? std::string() : pageGui(ctx_.doc, fields, title);
        }

        bool moving() const override
        {
            return wantsFullRate() || effectsLive() || (slow_ && history_.size() >= 2 && play_ < history_.back().t - 1e-6);
        }

        std::uint64_t drawKey() const override
        {
            DrawKey k;
            k.mix(ctx_.doc.revision()).mix(ctx_.selection.revision()).mix(ctx_.units.revision()).mix(slow_ ? 1 : 0);
            for (float v : shown_)
                k.mix(v);
            k.mix(static_cast<int>(shown_.size())).mix(ducts_ ? 1 : 0);
            if (ducts_)
                for (int i = 0; i < kDucts; ++i)
                    k.mix(ductNear_[static_cast<std::size_t>(i)]).mix(ductFar_[static_cast<std::size_t>(i)])
                        .mix(ductMidOn_ ? ductMid_[static_cast<std::size_t>(i)] : 0.0f);
            for (const double v : liveValues())
                k.mix(static_cast<float>(v));
            return k.h;
        }

        void pointerMove(const funkgui::PointerEvent& e) override
        {
            const Hit* h = hitAt({ e.x, e.y });
            ctx_.selection.hover(h != nullptr ? h->id : std::string());
            Board::pointerMove(e);
        }
        void pointerExit() override
        {
            ctx_.selection.hover({});
            Board::pointerExit();
        }
        void pointerDown(const funkgui::PointerEvent& e) override
        {
            if (const Hit* h = hitAt({ e.x, e.y }); h != nullptr && !slowButtonRect_.contains({ e.x, e.y }))
                ctx_.selection.select(h->id);
            else
                Board::pointerDown(e);
        }

        // For tests: the geometry as laid out, and the build as resolved.
        const Geo& geo() const noexcept { return geo_; }
        const ExhaustBuild& build() const noexcept { return build_; }

    protected:
        void layout(const Rect& s) override
        {
            slowButtonRect_ = { s.right() - 16.0f - 64.0f, s.y + 12.0f, 64.0f, 26.0f };
            slowButton_->setPlace(slowButtonRect_, true);
            rebuild();
        }

        void step(float dt) override
        {
            if (ctx_.doc.revision() != seenDoc_)
                rebuild();
            clock_ += static_cast<double>(dt);
            if (effectsLive())   // the effects' flicker: their own time, only while one would draw
                phase_ = std::fmod(phase_ + dt, 1000.0f);
            readScope();
            // what is shown: the newest copy, or (slow) the copies replayed at an eighth of their pace
            if (history_.empty())
            {
                shown_.clear();
                return;
            }
            if (!slow_)
            {
                shown_ = history_.back().p;
                play_ = history_.back().t;
                return;
            }
            play_ = std::clamp(play_ + static_cast<double>(dt) * kSlow, history_.front().t, history_.back().t);
            std::size_t i = 0;
            while (i + 1 < history_.size() && history_[i + 1].t < play_)
                ++i;
            const Snap& a = history_[i];
            const Snap& b = history_[std::min(i + 1, history_.size() - 1)];
            const double span = b.t - a.t;
            const float w = span > 1e-9 ? static_cast<float>((play_ - a.t) / span) : 0.0f;
            shown_.resize(std::min(a.p.size(), b.p.size()));
            for (std::size_t k = 0; k < shown_.size(); ++k)
                shown_[k] = a.p[k] + (b.p[k] - a.p[k]) * std::clamp(w, 0.0f, 1.0f);
        }

        void drawStage(Canvas& c, const funkgui::Theme& th) const override
        {
            const Palette p = Palette::of(th);
            const Rect& s = stage_;
            c.rrect(s.x, s.y, s.w, s.h, 0.0f, p.ground);
            if (build_.ts.on)
            {
                drawHeader(c, p);
                drawTwoStroke(c, p);
                drawMarks(c, p);
                return;
            }
            drawEffects(c, p);
            drawHeader(c, p);
            drawPipes(c, p);
            drawParts(c, p);
            drawDucts(c, p);
            drawPulse(c, p);
            drawReadouts(c, p);
            drawMarks(c, p);
        }

    private:
        // An element's selection path to its Spec fields: "exhaust.muffler[1]" -> the muffler's kind, size and chamber
        // on side 1, and its bypass. The section's own dials and shared fields come after the element's.
        static std::vector<std::string> elementFields(const std::string& sel, std::string& title)
        {
            const auto index = [&](std::string_view prefix) -> int {
                if (sel.size() <= prefix.size() + 2 || sel.compare(0, prefix.size(), prefix) != 0 || sel[prefix.size()] != '[')
                    return -1;
                return std::atoi(sel.c_str() + prefix.size() + 1);
            };
            const auto at = [](const char* base, int i) { return std::string(base) + "[" + std::to_string(i) + "]"; };
            int i = -1;
            if ((i = index("exhaust.primary")) >= 0)
            {
                title = "Primary " + std::to_string(i + 1);
                return { at("exhaust.primaryTo", i), at("exhaust.primLen", i), at("exhaust.primD", i), "exhaust.primSpread",
                         "exhaust.primStepAt", "exhaust.primStepD", "exhaust.headers" };
            }
            if ((i = index("exhaust.collector")) >= 0)
            {
                title = "Collector " + std::to_string(i + 1);
                return { at("exhaust.collectorTo", i), at("exhaust.colLen", i), at("exhaust.colD", i), at("exhaust.colVol", i) };
            }
            const auto side = [](int s) { return std::string(s == 0 ? " (left)" : " (right)"); };
            if ((i = index("exhaust.converter")) >= 0)
            {
                title = "Converter" + side(i);
                return { at("exhaust.converter", i), "exhaust.cat" };
            }
            if ((i = index("exhaust.main")) >= 0)
            {
                title = "Main pipe" + side(i);
                return { "exhaust.sides", at("exhaust.mainLen", i), at("exhaust.mainD", i), "exhaust.pipe" };
            }
            if ((i = index("exhaust.resonator")) >= 0)
            {
                title = "Resonator" + side(i);
                return { at("exhaust.resonator", i), at("exhaust.resHz", i), at("exhaust.resD", i), at("exhaust.resVol", i),
                         at("exhaust.resLen", i), "exhaust.linkLen" };
            }
            if ((i = index("exhaust.muffler")) >= 0)
            {
                title = "Muffler" + side(i);
                return { at("exhaust.mufKind", i), at("exhaust.mufSize", i), at("exhaust.mufLen", i), at("exhaust.bypass", i),
                         "exhaust.muffler" };
            }
            if ((i = index("exhaust.bypass")) >= 0)
            {
                title = "Bypass valve" + side(i);
                return { at("exhaust.bypass", i), "exhaust.bypassD", "exhaust.bypassRpm", "exhaust.bypassLoad", "exhaust.bypassTime" };
            }
            if ((i = index("exhaust.tail")) >= 0)
            {
                title = "Tail" + side(i);
                return { at("exhaust.tailLen", i), at("exhaust.tailD", i), at("exhaust.tips", i), at("exhaust.exitAt", i) };
            }
            if (sel == "exhaust.crossover")
            {
                title = "Crossover";
                return { "exhaust.crossKind", "exhaust.crossAt", "exhaust.crossD", "exhaust.crossLen", "exhaust.postLen",
                         "exhaust.crossover" };
            }
            if (sel == "boost")
            {
                title = "Turbines";
                return { "boost.kind", "boost.layout", "boost.turbWheel", "boost.turbAR", "boost.turbEta", "boost.scrolls",
                         "boost.housingVolume", "boost.vanes", "boost.vaneMin", "boost.gate", "boost.gateDiameter",
                         "exhaust.gateLen", "exhaust.gateD" };
            }
            // a two-stroke's pipe (card T's fields)
            if (sel == "engine.tsHeader")
            {
                title = "Header";
                return { "engine.tsChamber", "engine.tsHeaderLen", "engine.tsHeaderDiam" };
            }
            if (sel == "engine.tsDiffuser")
            {
                title = "Diffuser cone";
                return { "engine.tsDiffuserLen", "engine.tsHeaderDiam", "engine.tsBellyDiam", "engine.tsConeSteps" };
            }
            if (sel == "engine.tsBelly")
            {
                title = "Belly";
                return { "engine.tsBellyLen", "engine.tsBellyDiam" };
            }
            if (sel == "engine.tsBaffle")
            {
                title = "Baffle cone";
                return { "engine.tsBaffleLen", "engine.tsBellyDiam", "engine.tsStingerDiam", "engine.tsConeSteps" };
            }
            if (sel == "engine.tsStinger")
            {
                title = "Stinger";
                return { "engine.tsStingerLen", "engine.tsStingerDiam" };
            }
            if (sel == "engine.tsSilencer")
            {
                title = "Silencer";
                return { "engine.tsSilencer", "engine.tsSilencerLen", "engine.tsSilencerDiam", "engine.tsSilencerPack" };
            }
            if (sel == "engine.tsPort")
            {
                title = "Exhaust port";
                return { "engine.tsExhaustDur", "engine.tsExhaustWidth", "engine.tsExhaustRadius", "engine.tsValveDur",
                         "engine.tsValveFrom", "engine.tsValveTo" };
            }
            if (sel == "engine")
            {
                title = "Layout";
                return { "engine.layout", "exhaust.model" };
            }
            return {};
        }

        // ---- atmosphere (ui/views/fx): behind the primitives, carrying no fact; nothing on the software rasteriser ----
        // The headers' heat 0..1 (their wall, else the gas at the ports), a tail's pop 0..1 (the overrun's fuel cut).
        double headerHeat() const
        {
            double t = wall(kPrim);
            if (!std::isfinite(t))
                t = ctx_.live.get(N::port);
            return std::isfinite(t) ? std::clamp((t - 600.0) / 500.0, 0.0, 1.0) : 0.0;
        }
        double turbineHeat() const
        {
            double t = wall(kColl);
            if (!std::isfinite(t))
                t = ctx_.live.get(N::port);
            return std::isfinite(t) ? std::clamp((t - 650.0) / 450.0, 0.0, 1.0) : 0.0;
        }
        double pop() const
        {
            const double v = ctx_.live.get(N::pop);   // card G's flame: 1 while a bang burns near a hot tail
            if (std::isfinite(v))
                return v > 0.5 ? 0.8 : 0.0;
            const double flags = ctx_.live.get(N::flags), th = ctx_.live.get(N::throttle);
            const bool cut = std::isfinite(flags) && (static_cast<int>(flags) & 2) != 0;   // FLAG_FUEL_CUT
            return cut && std::isfinite(th) && th < 0.05 ? 0.5 : 0.0;
        }
        bool effectsLive() const
        {
            return (fx::available(fx::Effect::glow) && (headerHeat() > 0.0 || turbineHeat() > 0.0))
                || (fx::available(fx::Effect::flame) && pop() > 0.0);
        }
        void drawEffects(Canvas& c, const Palette& p) const
        {
            const Geo& g = geo_;
            if (const double h = headerHeat(); h > 0.0 && build_.ports > 0)
            {
                float top = 1e9f, bottom = -1e9f, right = g.headX;
                for (int i = 0; i < build_.ports; ++i)
                {
                    top = std::min(top, g.portY[static_cast<std::size_t>(i)]);
                    bottom = std::max(bottom, g.portY[static_cast<std::size_t>(i)]);
                }
                for (int ci = 0; ci < kCollectors; ++ci)
                    if (build_.colOn[static_cast<std::size_t>(ci)])
                        right = std::max(right, g.colExit[static_cast<std::size_t>(ci)]);
                fx::glow(c, { g.headX - 10.0f, top - 30.0f, right - g.headX + 30.0f, bottom - top + 60.0f },
                         static_cast<float>(h), p.hot, phase_, 0.5f);
            }
            if (const double h = turbineHeat(); h > 0.0)
                for (const Housing& hs : g.housings)
                    fx::glow(c, { hs.x - hs.r * 2.5f, hs.y - hs.r * 2.5f, hs.r * 5.0f, hs.r * 5.0f }, static_cast<float>(h), p.hot,
                             phase_, 0.4f);
            if (const double pp = pop(); pp > 0.0 && g.endX > 0)
                for (int si = 0; si < build_.sides; ++si)
                    if (build_.sideOn[static_cast<std::size_t>(si)])
                    {
                        const float y = g.laneY[static_cast<std::size_t>(si)];
                        fx::flame(c, { g.endX + 2.0f, y - 14.0f, 48.0f, 28.0f }, static_cast<float>(pp), phase_, false);
                    }
        }

        struct Snap
        {
            double t = 0;
            std::vector<float> p;
        };

        void rebuild()
        {
            seenDoc_ = ctx_.doc.revision();
            build_ = resolveExhaust(ctx_.doc);
            const Rect& s = stage_;
            const Rect d{ s.x + 20.0f, s.y + 84.0f, std::max(200.0f, s.w - 40.0f), std::max(160.0f, s.h - 84.0f - 74.0f) };
            geo_ = build_.ts.on ? Geo{} : layoutExhaust(build_, d);
            if (build_.ts.on)
                layoutTwoStroke(d);
        }

        // ---- a two-stroke's pipes (engine.cycle 1) -------------------------------------------------------------------
        // Each cylinder's pipe to scale, lengths and bores alike (one scale for both, so a cone looks as steep as it is),
        // a lane each; under them, the exhaust port's pressure over a turn with the port's opening and closing marked.
        void layoutTwoStroke(const Rect& d)
        {
            const TwoStrokePipe& t = build_.ts;
            geo_.diagram = d;
            const float plotH = std::clamp(d.h * 0.34f, 110.0f, 200.0f);
            tsPlot_ = { d.x, d.bottom() - plotH, d.w, plotH };
            const float top = d.y, h = d.h - plotH - 16.0f;
            const int n = t.cylinders;
            const float laneH = h / static_cast<float>(n);
            const float total = t.length() + (t.silencer ? t.silLen : 0.0f);
            const float x0 = d.x + 46.0f, room = d.right() - 120.0f - x0;
            tsScale_ = std::min(room / std::max(0.2f, total), laneH * 0.78f / std::max(0.02f, t.chamber ? t.dBelly : t.dHeader));
            tsX_[0] = x0;
            const float lens[5] = { t.header, t.diffuser, t.belly, t.baffle, t.stinger };
            for (int k = 0; k < 5; ++k)
                tsX_[static_cast<std::size_t>(k + 1)] = tsX_[static_cast<std::size_t>(k)] + lens[k] * tsScale_;
            tsEnd_ = tsX_[5] + (t.silencer ? t.silLen * tsScale_ : 0.0f);
            tsLane_.clear();
            for (int c = 0; c < n; ++c)
                tsLane_.push_back(top + laneH * (static_cast<float>(c) + 0.5f));
            static const char* const kIds[5] = { "engine.tsHeader", "engine.tsDiffuser", "engine.tsBelly", "engine.tsBaffle",
                                                 "engine.tsStinger" };
            static const char* const kNames[5] = { "HEADER", "DIFFUSER", "BELLY", "BAFFLE CONE", "STINGER" };
            for (int c = 0; c < n; ++c)
            {
                const float y = tsLane_[static_cast<std::size_t>(c)], half = 0.5f * t.dBelly * tsScale_ + 6.0f;
                for (int k = 0; k < 5; ++k)
                    if (tsX_[static_cast<std::size_t>(k + 1)] - tsX_[static_cast<std::size_t>(k)] > 1.0f && (t.chamber || k == 0))
                        geo_.hits.push_back({ { tsX_[static_cast<std::size_t>(k)], y - std::max(10.0f, half), t.chamber
                                                  ? tsX_[static_cast<std::size_t>(k + 1)] - tsX_[static_cast<std::size_t>(k)]
                                                  : tsX_[5] - tsX_[0], 2.0f * std::max(10.0f, half) },
                                              kIds[k], t.chamber ? kNames[k] : "STRAIGHT PIPE" });
                if (t.silencer)
                    geo_.hits.push_back({ { tsX_[5], y - 12.0f, tsEnd_ - tsX_[5], 24.0f }, "engine.tsSilencer", "SILENCER" });
            }
            geo_.hits.push_back({ tsPlot_, "engine.tsPort", "EXHAUST PORT" });
        }

        void drawTwoStroke(Canvas& c, const Palette& p) const
        {
            const TwoStrokePipe& t = build_.ts;
            const double gasK = ctx_.live.get(N::tsPipeTemp);
            const Col ink = std::isfinite(gasK) ? heat(p, gasK) : p.text;
            const Col fill = Palette::mix(p.ground, ink, 0.16f);
            const float s = tsScale_;
            for (std::size_t ci = 0; ci < tsLane_.size(); ++ci)
            {
                const float y = tsLane_[ci];
                // the cylinder and its port
                c.rrect(tsX_[0] - 40.0f, y - 22.0f, 34.0f, 44.0f, 5.0f, p.raised, 1.0f, p.edge);
                c.text(std::to_string(ci + 1).c_str(), tsX_[0] - 28.0f, y - 7.0f, look::kNumberS, p.text);
                c.segment(tsX_[0] - 6.0f, y, tsX_[0], y, 0.5f * t.dHeader * s * 2.0f, Palette::mix(p.ground, ink, 0.5f));
                // the walls: the radius at each section's ends (a cone is two straight lines, diverging or converging)
                float xs[6], rs[6];
                for (int k = 0; k < 6; ++k)
                    xs[k] = tsX_[static_cast<std::size_t>(k)];
                if (t.chamber)
                {
                    const float rh = 0.5f * t.dHeader * s, rb = 0.5f * t.dBelly * s, rt = 0.5f * t.dStinger * s;
                    const float r[6] = { rh, rh, rb, rb, rt, rt };
                    for (int k = 0; k < 6; ++k)
                        rs[k] = r[k];
                }
                else
                    for (int k = 0; k < 6; ++k)
                        rs[k] = 0.5f * t.dHeader * s;
                float top[6], bot[6];
                for (int k = 0; k < 6; ++k)
                {
                    top[k] = y - rs[k];
                    bot[k] = y + rs[k];
                }
                c.areaStrip(xs, 6, top, bot, 0.0f, fill);
                c.polyline(xs, top, 6, 1.75f, ink);
                c.polyline(xs, bot, 6, 1.75f, ink);
                for (int k = 1; k < 5; ++k)   // the joints, faint
                    if (t.chamber)
                        c.segment(xs[k], top[k], xs[k], bot[k], 1.0f, Palette::mix(p.ground, ink, 0.35f));
                // the silencer: a packed can (its packing as dots)
                float endX = tsX_[5];
                if (t.silencer)
                {
                    const float rr = std::max(5.0f, 0.5f * t.silDiam * s * 2.2f);
                    c.rrect(tsX_[5], y - rr, tsEnd_ - tsX_[5], 2.0f * rr, rr, Palette::mix(p.panel, ink, 0.25f), 1.5f, ink);
                    c.dotted(tsX_[5] + 6.0f, y - rr * 0.5f, tsEnd_ - tsX_[5] - 12.0f, 4.0f, Palette::mix(p.panel, ink, 0.8f));
                    endX = tsEnd_;
                }
                radiating(c, p, endX + 6.0f, y);
                // the section's names, under the lane where there is room
                if (ci + 1 == tsLane_.size() || tsLane_.size() == 1)
                {
                    static const char* const kNames[5] = { "HEADER", "DIFFUSER", "BELLY", "BAFFLE", "STINGER" };
                    float lastRight = -1e9f;
                    const float ly = y + 0.5f * (t.chamber ? t.dBelly : t.dHeader) * s + 8.0f;
                    for (int k = 0; k < 5 && t.chamber; ++k)
                    {
                        const float w = c.textWidth(kNames[k], look::kCaps), cx = 0.5f * (xs[k] + xs[k + 1]);
                        const float x = std::max(cx - 0.5f * w, lastRight + 6.0f);
                        if (x + w <= xs[k + 1] + 30.0f)
                        {
                            c.text(kNames[k], x, ly, look::kCaps, p.label);
                            lastRight = x + w;
                        }
                    }
                }
            }
            drawTwoStrokePulse(c, p);
            drawPortPlot(c, p);
            // the derived speed the pipe is tuned for, at the pipe's gas temperature (or a warm pipe's 680 K)
            const Rect& st = stage_;
            const float y = st.bottom() - 62.0f;
            c.hairlineH(st.x + 20.0f, y - 8.0f, st.w - 40.0f, p.grid);
            const double k = std::isfinite(gasK) && gasK > 0.0 ? gasK : 680.0;
            const double rpm = t.tunedRpm(k);
            if (rpm > 0.0)
            {
                const std::string v = look::num(std::round(rpm / 50.0) * 50.0, "%.0f");
                look::readout(c, p, st.x + 20.0f, y, "ON THE PIPE NEAR", v, "rpm, DERIVED", look::kNumberM);
            }
            else
                look::readout(c, p, st.x + 20.0f, y, "A STRAIGHT PIPE", "-", "no tuned speed", look::kNumberM);
            const std::string gas = std::isfinite(gasK) && gasK > 0.0
                                        ? ctx_.units.format(Quantity::temperature, gasK, 0)
                                        : "680 K assumed (a warm pipe)";
            look::readout(c, p, st.x + 300.0f, y, "PIPE GAS", gas, {}, look::kNumberS);
        }

        // The pulse and its returning wave along cylinder 1's pipe: the Scope's pressure along the path, spread over
        // the pipe's length (its points evenly spaced by length).
        void drawTwoStrokePulse(Canvas& c, const Palette& p) const
        {
            if (shown_.size() < 2 || tsLane_.empty())
                return;
            const int n = static_cast<int>(shown_.size());
            const float y = tsLane_[0], x0 = tsX_[0], x1 = build_.ts.silencer ? tsEnd_ : tsX_[5];
            std::vector<float> xs, ys;
            for (int i = 0; i < n; ++i)
            {
                const float f = (static_cast<float>(i) + 0.5f) / static_cast<float>(n);
                xs.push_back(x0 + (x1 - x0) * f);
                ys.push_back(y - std::clamp(shown_[static_cast<std::size_t>(i)] / kPulseScale, -1.0f, 1.0f) * kPulseSwing * 1.6f);
            }
            c.polyline(xs.data(), ys.data(), n, 1.75f, p.value);
        }

        // The exhaust port's pressure over a turn (the Scope's portExhaust, cylinder 1), with the port's opening and
        // closing marked: the plugging pulse should arrive just before the closing.
        void drawPortPlot(Canvas& c, const Palette& p) const
        {
            const Rect in = look::region(c, p, tsPlot_, "AT THE EXHAUST PORT, OVER A TURN");
            // the plot under a row of its own for the port's two edges in words (so no trace runs through them)
            const Rect pl{ in.x + 34.0f, in.y + 16.0f, in.w - 44.0f, in.h - 32.0f };
            const float epo = 180.0f - 0.5f * build_.ts.exhaustDur, epc = 180.0f + 0.5f * build_.ts.exhaustDur;
            const auto X = [&](float deg) { return pl.x + pl.w * deg / 360.0f; };
            c.hairlineH(pl.x, pl.centreY(), pl.w, p.grid);
            for (int dgr = 0; dgr <= 360; dgr += 90)
            {
                c.hairlineV(X(static_cast<float>(dgr)), pl.y, pl.h, p.grid);
                c.text(std::to_string(dgr).c_str(), X(static_cast<float>(dgr)) - 6.0f, pl.bottom() + 3.0f, look::kUnit, p.label);
            }
            // the port's open span, and its two edges in words
            c.rrect(X(epo), pl.y, X(epc) - X(epo), pl.h, 0.0f, Palette::mix(p.panel, p.hot, 0.10f));
            c.segment(X(epo), pl.y, X(epo), pl.bottom(), 1.25f, p.hot);
            c.segment(X(epc), pl.y, X(epc), pl.bottom(), 2.0f, p.selected);
            c.text("OPENS", X(epo) - 0.5f * c.textWidth("OPENS", look::kCaps), pl.y - 15.0f, look::kCaps, p.hot);
            c.text("CLOSES", X(epc) - 0.5f * c.textWidth("CLOSES", look::kCaps), pl.y - 15.0f, look::kCaps, p.selected);
            if (portTrace_.size() < 2)
            {
                c.text("no port pressure from the Scope yet", pl.x + 8.0f, pl.centreY() + 6.0f, look::kBody, p.dim);
                return;
            }
            const int n = static_cast<int>(portTrace_.size());
            float lo = 1e9f, hi = -1e9f;
            for (float v : portTrace_)
            {
                lo = std::min(lo, v);
                hi = std::max(hi, v);
            }
            const float mid = 0.5f * (lo + hi), span = std::max(1000.0f, hi - lo);
            std::vector<float> xs, ys;
            for (int i = 0; i < n; ++i)
            {
                xs.push_back(X(360.0f * static_cast<float>(i) / static_cast<float>(n)));
                ys.push_back(pl.centreY() - (portTrace_[static_cast<std::size_t>(i)] - mid) / span * pl.h * 0.9f);
            }
            c.polyline(xs.data(), ys.data(), n, 1.5f, p.value);
            c.text(ctx_.units.format(Quantity::pressure, hi - lo).c_str(), pl.x - 30.0f, pl.y - 14.0f, look::kUnit, p.label);
        }

        void readScope()
        {
            ScopeSource& sc = ctx_.scope;
            if (scopeGen_ == sc.generation() && !history_.empty())
                return;
            scopeGen_ = sc.generation();
            float buf[256];
            int n = 0;
            double ambient = 0.0;
            // card G's path (gauge Pa, with each point's distance), else K3's pipePressure (absolute in the demo)
            pathX_.clear();
            if (const int h = sc.find(N::pathPressure); h >= 0 && (n = sc.read(h, buf, 256)) > 1)
            {
                float xs[256];
                const int hx = sc.find(N::pathX);
                if (hx >= 0 && sc.read(hx, xs, 256) == n && xs[n - 1] > xs[0])
                    pathX_.assign(xs, xs + n);
            }
            else if (const int h2 = sc.find(N::scopePipe); h2 >= 0 && (n = sc.read(h2, buf, 256)) > 1)
            {
                ambient = fieldOr(ctx_.doc, N::ambient, -1, kAmbient);
                double mean = 0;
                for (int i = 0; i < n; ++i)
                    mean += buf[i];
                if (mean / n < ambient * 0.5)
                    ambient = 0.0;
            }
            else
                n = 0;
            // a two-stroke's port pressure over the turn (the Scope's cycle: `degrees` entries)
            portTrace_.clear();
            if (build_.ts.on)
                if (const int hp = sc.find(N::tsPortScope) >= 0 ? sc.find(N::tsPortScope) : sc.find(N::portScope); hp >= 0)
                {
                    float deg[1] = { 0.0f };
                    const int hd = sc.find(N::degrees);
                    const int want = hd >= 0 && sc.read(hd, deg, 1) == 1 && deg[0] > 0.0f ? static_cast<int>(deg[0]) : 360;
                    std::vector<float> v(1080);
                    const int got = sc.read(hp, v.data(), 1080);
                    // a two-stroke's turn is 360 degrees: a longer copy holds two turns; the last turn is the one shown
                    if (got >= 2)
                        portTrace_.assign(v.begin() + std::max(0, got - std::min(want, 360)), v.begin() + got);
                }
            // every pipe slot's pressure at its two ends (card G's)
            ducts_ = false;
            {
                const int hu = sc.find(N::ductUsed), hn = sc.find(N::ductNear), hf = sc.find(N::ductFar);
                if (hu >= 0 && hn >= 0 && hf >= 0 && sc.read(hu, ductUsed_.data(), kDucts) == kDucts
                    && sc.read(hn, ductNear_.data(), kDucts) == kDucts && sc.read(hf, ductFar_.data(), kDucts) == kDucts)
                    ducts_ = true;
                const int hm = sc.find(N::ductMid);
                ductMidOn_ = ducts_ && hm >= 0 && sc.read(hm, ductMid_.data(), kDucts) == kDucts;
            }
            if (n <= 1)
                return;
            Snap snap;
            snap.t = clock_;
            snap.p.resize(static_cast<std::size_t>(n));
            for (int i = 0; i < n; ++i)
                snap.p[static_cast<std::size_t>(i)] = static_cast<float>(buf[i] - ambient);
            if (!history_.empty() && history_.back().p == snap.p)
                return;
            history_.push_back(std::move(snap));
            if (history_.size() > kHistory)
                history_.erase(history_.begin());
        }

        // A side's radiating ends' level (dB): the loudest of the ends whose pipe slot is that side's tail or tips.
        double endDb(int side) const
        {
            const double ends = ctx_.live.get(N::ends);
            if (!std::isfinite(ends))
                return std::nan("");
            const int base = 48 + side * 15;
            double best = std::nan("");
            for (int i = 0; i < std::min(24, static_cast<int>(ends)); ++i)
            {
                const std::string idx = "[" + std::to_string(i) + "]";
                const double slot = ctx_.live.get(std::string(N::endSlot) + idx);
                if (!std::isfinite(slot))
                    continue;
                const int sl = static_cast<int>(slot);
                if (sl == base + 11 || sl == base + 12 || sl == base + 13)
                {
                    const double db = ctx_.live.get(std::string(N::endLevel) + idx);
                    if (std::isfinite(db) && !(db <= best))
                        best = db;
                }
            }
            return best;
        }

        std::array<double, 15> liveValues() const
        {
            std::array<double, 15> v{};
            v[0] = ctx_.live.get(N::port);
            v[1] = ctx_.live.get(N::collector);
            v[2] = ctx_.live.get(N::tail);
            for (int i = 0; i < 7; ++i)
                v[static_cast<std::size_t>(3 + i)] = ctx_.live.get(N::walls[i]);
            v[10] = endDb(0);
            v[11] = endDb(1);
            v[12] = ctx_.live.get(N::valve[0]);
            v[13] = ctx_.live.get(N::valve[1]);
            v[14] = ctx_.live.get(N::pop);
            return v;
        }

        const Hit* hitAt(Point pt) const
        {
            for (auto it = geo_.hits.rbegin(); it != geo_.hits.rend(); ++it)
                if (it->r.contains(pt))
                    return &*it;
            return nullptr;
        }

        // ---- colours ------------------------------------------------------------------------------------------------
        double wall(int section) const { return ctx_.live.get(N::walls[section]); }
        // With its wall's temperature a pipe is filled with its tint; without, it is drawn as an outline (the ground
        // inside, a dim edge), so the absent state is plainly not a colour.
        bool walls() const { return std::isfinite(wall(kPrim)) || std::isfinite(wall(kMain)) || std::isfinite(wall(kTail)); }
        Col pipeFill(const Palette& p, int section) const
        {
            const double t = wall(section);
            return std::isfinite(t) ? heat(p, t) : walls() ? Palette::mix(p.panel, p.text, 0.32f) : p.ground;
        }
        // A part's own stroke (a can's outline, a scroll): its tint, or the dim ink.
        Col partInk(const Palette& p, int section) const
        {
            const double t = wall(section);
            return std::isfinite(t) ? heat(p, t) : p.dim;
        }
        Col pipeEdge(const Palette& p) const
        {
            if (!walls())
                return p.dim;
            return p.dark ? Palette::mix(p.ground, p.edge, 0.6f) : p.edge;
        }
        std::string temp(double k) const
        {
            return std::isfinite(k) ? ctx_.units.format(Quantity::temperature, k, 0) : "-";
        }

        // ---- the header: the build in a sentence, the heat's scale --------------------------------------------------
        void drawHeader(Canvas& c, const Palette& p) const
        {
            const Rect& s = stage_;
            c.text("EXHAUST", s.x + 20.0f, s.y + 14.0f, look::kTitle, p.label);
            c.text(build_.sentence().c_str(), s.x + 20.0f, s.y + 42.0f, look::kBody, p.text);
            // the walls' scale: a strip of the tint with its ends in the user's unit; without the walls' temperatures,
            // the words instead (the pipes are outlines then)
            const float x1 = slowButtonRect_.x - 20.0f, w = std::min(150.0f, std::max(80.0f, x1 - s.x - 140.0f)), x0 = x1 - w;
            const float y = s.y + 14.0f;
            if (!walls())
            {
                const char* t = "NO WALL TEMPERATURES YET";
                c.text(t, x1 - c.textWidth(t, look::kCaps), y, look::kCaps, p.dim);
                return;
            }
            c.text("WALL", x0 - c.textWidth("WALL", look::kCaps) - 8.0f, y - 2.0f, look::kCaps, p.label);
            const int n = 24;
            for (int i = 0; i < n; ++i)
            {
                const double k = 350.0 + (kHeatTop - 350.0) * (i + 0.5) / n;
                c.rrect(x0 + w * static_cast<float>(i) / n, y, w / n + 0.5f, 6.0f, 0.0f, heat(p, k));
            }
            c.text(temp(350.0).c_str(), x0, y + 9.0f, look::kUnit, p.label);
            const std::string hot = temp(kHeatTop);
            c.text(hot.c_str(), x1 - c.textWidth(hot.c_str(), look::kUnit), y + 9.0f, look::kUnit, p.label);
        }

        // ---- the pipes ---------------------------------------------------------------------------------------------
        void drawPipes(Canvas& c, const Palette& p) const
        {
            const ExhaustBuild& b = build_;
            const Geo& g = geo_;
            const Col edge = pipeEdge(p);
            // collector pipes first (under the primaries' ends), then the sides, then the primaries
            for (int ci = 0; ci < kCollectors; ++ci)
            {
                const auto k = static_cast<std::size_t>(ci);
                if (!b.colOn[k])
                    continue;
                const Point a{ g.colExit[k], g.colY[k] };
                Point to{};
                const int dest = b.colTo[k];
                if (dest >= 2)
                    to = { g.colX[static_cast<std::size_t>(dest - 2)], g.colY[static_cast<std::size_t>(dest - 2)] };
                else
                {
                    const int side = std::min(dest, b.sides - 1);
                    to = { g.laneX, g.laneY[static_cast<std::size_t>(side)] };
                    // into the nearest housing of its side
                    float best = 1e9f;
                    for (const Housing& h : g.housings)
                        if (h.side == side || b.turbos == Turbos::one || b.turbos == Turbos::compound)
                        {
                            const float dy = std::fabs(h.y - a.y);
                            if (dy < best && h.x < g.laneX + 60.0f)
                            {
                                best = dy;
                                to = { h.x - h.r, h.y };
                            }
                        }
                    if (!b.sideOn[static_cast<std::size_t>(side)])
                        continue;
                }
                pipeLine(c, sBendPts(a, to), 8.0f, pipeFill(p, b.colFed[k] ? kMerge : kColl), edge);
            }
            for (int si = 0; si < b.sides; ++si)
            {
                const auto k = static_cast<std::size_t>(si);
                if (!b.sideOn[k] || g.endX <= 0)
                    continue;
                drawLane(c, p, si);
            }
            for (int i = 0; i < b.ports; ++i)
            {
                const auto k = static_cast<std::size_t>(i);
                const float y = g.portY[k];
                const std::vector<Point> pts = primaryPts(i);
                const int to = b.portTo[k];
                pipeLine(c, pts, 6.0f, pipeFill(p, kPrim), edge);
                if (to < 0)
                    drawStack(c, p, g.primEnd[k], y);
            }
            // open stacks: their position (each is a radiating end in front) once, by the topmost
            if (b.ports > 0 && b.portTo[static_cast<std::size_t>(g.order[0])] < 0)
            {
                const auto k = static_cast<std::size_t>(g.order[0]);
                const float tx = g.primEnd[k] + 34.0f, ty = g.portY[k] - 16.0f;
                c.text(b.openStacks() && b.ports > 1 ? "OPEN STACKS, FRONT" : "OPEN STACK, FRONT", tx, ty, look::kCaps, p.text);
            }
        }

        // A primary's centreline: its straight run, then its bend into its own place in its collector's mouth.
        std::vector<Point> primaryPts(int i) const
        {
            const ExhaustBuild& b = build_;
            const Geo& g = geo_;
            const auto k = static_cast<std::size_t>(i);
            const float y = g.portY[k];
            std::vector<Point> pts{ { g.headX, y }, { g.primEnd[k], y } };
            const int to = b.portTo[k];
            if (to >= 0)
            {
                const auto ck = static_cast<std::size_t>(to);
                // each primary into its own place in the cone's mouth, in the order they come
                int n = 0, m = 0;
                for (int j = 0; j < b.ports; ++j)
                    if (b.portTo[static_cast<std::size_t>(j)] == to)
                    {
                        if (g.portY[static_cast<std::size_t>(j)] < y)
                            m += 1;
                        n += 1;
                    }
                const bool log = b.colVol[ck] > 0.0f;
                const float mouth = log ? g.colH[ck] - 18.0f : g.colH[ck] - 8.0f;
                const float off = n > 1 ? (static_cast<float>(m) / static_cast<float>(n - 1) - 0.5f) * mouth : 0.0f;
                const Point end{ g.colX[ck], log ? y : g.colY[ck] + off };
                if (std::fabs(end.y - y) < 0.5f)
                    pts.push_back(end);
                else
                {
                    const std::vector<Point> bend = sBendPts({ g.primEnd[k], y }, end);
                    pts.insert(pts.end(), bend.begin() + 1, bend.end());
                }
            }
            return pts;
        }

        // One side's lane: the pipe through every column, the crossover's bends, the tips.
        void drawLane(Canvas& c, const Palette& p, int s) const
        {
            const ExhaustBuild& b = build_;
            const Geo& g = geo_;
            const auto k = static_cast<std::size_t>(s);
            const float y = g.laneY[k];
            const Col edge = pipeEdge(p);
            const float mid = (g.laneY[0] + g.laneY[1]) * 0.5f;
            // from the head (or the housings' outlets: a downpipe from each that serves this side) to the main pipe's end
            float start = g.laneX;
            if (!g.housings.empty())
            {
                start = g.chainX - 12.0f;
                for (std::size_t i = 0; i < g.housings.size(); ++i)
                {
                    const Housing& h = g.housings[i];
                    if (b.turbos == Turbos::compound)
                    {
                        if (i + 1 < g.housings.size())   // the first's outlet into the second
                        {
                            const Housing& n = g.housings[i + 1];
                            pipeLine(c, { { h.x + h.r, h.y }, { n.x - n.r, n.y } }, 8.0f, pipeFill(p, kColl), edge);
                            continue;
                        }
                    }
                    else if (h.side != s && b.turbos != Turbos::one && b.turbos != Turbos::sequential)
                        continue;
                    pipeLine(c, sBendPts({ h.x + h.r * 0.7f, h.y }, { start, y }, 12), 10.0f, pipeFill(p, kMain), edge);
                }
            }
            if (b.cross == 2 && b.crossAt == 0)
            {
                pipeLine(c, { { start, y }, { g.crossX0, y } }, 10.0f, pipeFill(p, kMain), edge);
                pipeLine(c, xBend(g.crossX0, g.crossX1, y, mid), 10.0f, pipeFill(p, kMain), edge);
                pipeLine(c, { { g.crossX1, y }, { g.mainX1, y } }, 10.0f, pipeFill(p, kMain), edge);
            }
            else
                pipeLine(c, { { start, y }, { g.mainX1, y } }, 10.0f, pipeFill(p, kMain), edge);
            // after the main pipe: the link (a crossover there, the resonator's tee) to the muffler
            const float linkEnd = g.mufX0 > 0 ? g.mufX0 : g.tailX0;
            if (b.cross == 2 && b.crossAt == 1)
            {
                pipeLine(c, { { g.mainX1, y }, { g.crossX0, y } }, 10.0f, pipeFill(p, kLink), edge);
                pipeLine(c, xBend(g.crossX0, g.crossX1, y, mid), 10.0f, pipeFill(p, kLink), edge);
                pipeLine(c, { { g.crossX1, y }, { linkEnd, y } }, 10.0f, pipeFill(p, kLink), edge);
            }
            else
                pipeLine(c, { { g.mainX1, y }, { linkEnd, y } }, 10.0f, pipeFill(p, kLink), edge);
            if (g.mufX0 > 0 && b.muf[k] == 0)
                pipeLine(c, { { g.mufX0, y }, { g.mufX1, y } }, 10.0f, pipeFill(p, kLink), edge);
            // the tail and its tips
            const Col tail = pipeFill(p, kTail);
            if (b.tips[k] == 2)
            {
                const float fork = g.tailX0 + (g.endX - g.tailX0) * 0.55f;
                pipeLine(c, { { g.tailX0, y }, { fork, y } }, 9.0f, tail, edge);
                for (const float dy : { -6.0f, 6.0f })
                {
                    std::vector<Point> tip = sBendPts({ fork, y }, { fork + 22.0f, y + dy }, 8);
                    tip.push_back({ g.endX, y + dy });
                    pipeLine(c, tip, 6.0f, tail, edge);
                }
            }
            else
                pipeLine(c, { { g.tailX0, y }, { g.endX, y } }, 9.0f, tail, edge);
        }

        // An X: the lane bends to the middle, meets the other side's there, and goes back.
        static std::vector<Point> xBend(float x0, float x1, float y, float mid)
        {
            const float xm = (x0 + x1) * 0.5f;
            std::vector<Point> a = sBendPts({ x0, y }, { xm, mid }, 10);
            const std::vector<Point> b2 = sBendPts({ xm, mid }, { x1, y }, 10);
            a.insert(a.end(), b2.begin() + 1, b2.end());
            return a;
        }

        // An open stack's bell and its radiating mark.
        void drawStack(Canvas& c, const Palette& p, float x, float y) const
        {
            const Col edge = pipeEdge(p), fill = pipeFill(p, kPrim);
            c.segment(x, y - 3.0f, x + 10.0f, y - 8.0f, 2.5f, edge);
            c.segment(x, y + 3.0f, x + 10.0f, y + 8.0f, 2.5f, edge);
            c.segment(x, y - 3.0f, x + 10.0f, y - 8.0f, 1.5f, fill);
            c.segment(x, y + 3.0f, x + 10.0f, y + 8.0f, 1.5f, fill);
            radiating(c, p, x + 12.0f, y, 0.7f);
        }

        // Three arcs: sound leaving an open end.
        static void radiating(Canvas& c, const Palette& p, float x, float y, float scale = 1.0f)
        {
            for (int i = 0; i < 3; ++i)
            {
                const float r = (6.0f + 5.0f * static_cast<float>(i)) * scale;
                c.arc({ x - 4.0f * scale, y }, r, -0.6f, 0.6f, 1.25f, Palette::mix(p.ground, p.text, 0.75f - 0.18f * static_cast<float>(i)));
            }
        }

        // ---- the parts ----------------------------------------------------------------------------------------------
        void drawParts(Canvas& c, const Palette& p) const
        {
            const ExhaustBuild& b = build_;
            const Geo& g = geo_;
            // the heads, a port each, numbered
            for (int bank = 0; bank < g.nBanks; ++bank)
            {
                const Rect& r = g.banks[static_cast<std::size_t>(bank)];
                c.rrect(r.x, r.y, r.w, r.h, 5.0f, p.raised, 1.0f, p.edge);
            }
            for (int i = 0; i < b.ports; ++i)
            {
                const auto k = static_cast<std::size_t>(i);
                char n[8];
                std::snprintf(n, sizeof n, "%d", i + 1);
                c.rrect(g.headX - 4.0f, g.portY[k] - 5.0f, 6.0f, 10.0f, 2.0f, partInk(p, kPrim));
                c.text(n, g.headX - 10.0f - c.textWidth(n, look::kNumberS) , g.portY[k] - 7.0f, look::kNumberS, p.text);
            }
            if (g.nBanks == 2)
            {
                c.text("L", g.banks[0].x + 4.0f, g.banks[0].y - 16.0f, look::kCaps, p.label);
                c.text("R", g.banks[1].x + 4.0f, g.banks[1].y - 16.0f, look::kCaps, p.label);
            }
            // the collectors: a cone (an area strip narrowing), a log manifold's chamber, a merge
            for (int ci = 0; ci < kCollectors; ++ci)
            {
                const auto k = static_cast<std::size_t>(ci);
                if (!b.colOn[k] || g.colExit[k] <= g.colX[k])
                    continue;
                const float x0 = g.colX[k], x1 = g.colExit[k], y = g.colY[k], h = g.colH[k];
                const Col fill = pipeFill(p, b.colFed[k] ? kMerge : kColl), edge = pipeEdge(p);
                if (b.colVol[k] > 0.0f && !b.colFed[k])
                {
                    c.rrect(x0, y - h * 0.5f, x1 - x0, h, 7.0f, fill, 1.5f, edge);
                    continue;
                }
                const float xs[2] = { x0, x1 }, top[2] = { y - h * 0.5f - 1.0f, y - 5.0f }, bot[2] = { y + h * 0.5f + 1.0f, y + 5.0f };
                c.areaStrip(xs, 2, top, bot, 0.0f, edge);
                const float top2[2] = { y - h * 0.5f, y - 4.0f }, bot2[2] = { y + h * 0.5f, y + 4.0f };
                c.areaStrip(xs, 2, top2, bot2, 0.0f, fill);
            }
            // the turbine housings: a scroll (a disc and an inner arc) with its outlet
            for (const Housing& h : g.housings)
            {
                const Col fill = partInk(p, kColl);
                c.disc(h.x, h.y, h.r, Palette::mix(p.panel, fill, 0.5f), 2.0f, fill);
                c.arc({ h.x, h.y }, h.r * 0.55f, 0.0f, 4.7f, 1.5f, fill);
                c.disc(h.x, h.y, 2.5f, fill);
            }
            if (!g.housings.empty())
            {
                const Housing& h = g.housings.front();
                centred(c, g.housings.size() > 1 ? "TURBINES" : "TURBINE", h.x, b.sides == 2 && g.housings.size() == 1 ? h.y + h.r + 8.0f : h.y - h.r - 16.0f, look::kCaps, p.label);
            }
            for (int s = 0; s < b.sides; ++s)
                if (b.sideOn[static_cast<std::size_t>(s)] && g.endX > 0)
                    drawChain(c, p, s);
            if (b.cross == 1 && g.crossX0 > 0)
            {
                const float xm = (g.crossX0 + g.crossX1) * 0.5f;
                const int sec = b.crossAt == 0 ? kMain : kLink;
                pipeLine(c, { { xm, g.laneY[0] }, { xm, g.laneY[1] } }, 8.0f, pipeFill(p, sec), pipeEdge(p));
                c.disc(xm, g.laneY[0], 4.0f, pipeEdge(p));
                c.disc(xm, g.laneY[1], 4.0f, pipeEdge(p));
                c.text("H-PIPE", xm + 12.0f, (g.laneY[0] + g.laneY[1]) * 0.5f - 6.0f, look::kCaps, p.label);
            }
            if (b.cross == 2 && g.crossX0 > 0)
            {
                const float xm = (g.crossX0 + g.crossX1) * 0.5f, mid = (g.laneY[0] + g.laneY[1]) * 0.5f;
                c.disc(xm, mid, 6.0f, pipeEdge(p));
                c.text("X-PIPE", xm + 14.0f, mid - 6.0f, look::kCaps, p.label);
            }
        }

        // One side's chain: converter, resonator, muffler (and its bypass), the end with its position and level.
        void drawChain(Canvas& c, const Palette& p, int s) const
        {
            const ExhaustBuild& b = build_;
            const Geo& g = geo_;
            const auto k = static_cast<std::size_t>(s);
            const float y = g.laneY[k];
            const bool lower = b.sides == 2 && s == 1;
            const float out = lower ? 1.0f : -1.0f;        // away from the other side
            const Col edge = pipeEdge(p);
            // a part's word: above the upper lane, under the lower one (and under a single lane)
            const auto caption = [&](const char* word, float x, float halfH) {
                const float ty = b.sides == 2 && !lower ? y - halfH - 17.0f : y + halfH + 6.0f;
                centred(c, word, x, ty, look::kCaps, p.label);
            };
            if (b.conv[k] != 0)
            {
                const Col fill = partInk(p, kMain);
                const float x0 = g.convX0, w = g.convX1 - g.convX0;
                c.rrect(x0, y - 13.0f, w, 26.0f, 9.0f, Palette::mix(p.panel, fill, 0.35f), 1.5f, fill);
                // the substrate's cells: close for a standard converter, open for a high-flow one
                const float step = b.conv[k] == 2 ? 9.0f : 4.5f;
                for (float x = x0 + 10.0f; x < x0 + w - 8.0f; x += step)
                    c.segment(x, y - 7.0f, x, y + 7.0f, 1.0f, Palette::mix(p.panel, fill, 0.8f));
                caption(b.conv[k] == 2 ? "HIGH-FLOW" : "CONVERTER", x0 + w * 0.5f, 13.0f);
            }
            if (b.res[k] != 0)
            {
                const Col fill = partInk(p, kLink), pfill = pipeFill(p, kLink);
                const float xm = (g.resX0 + g.resX1) * 0.5f;
                if (b.res[k] == 1)       // a quarter-wave stub: a dead-ended branch
                {
                    pipeLine(c, { { xm, y }, { xm, y + out * 34.0f } }, 7.0f, pfill, edge);
                    c.segment(xm - 6.0f, y + out * 38.0f, xm + 6.0f, y + out * 38.0f, 2.5f, edge);
                }
                else if (b.res[k] == 2)  // a Helmholtz: a neck into a chamber
                {
                    pipeLine(c, { { xm, y }, { xm, y + out * 20.0f } }, 5.0f, pfill, edge);
                    c.disc(xm, y + out * 31.0f, 12.0f, Palette::mix(p.panel, fill, 0.35f), 1.5f, fill);
                }
                else                     // a packed bottle in line: the packing as dots
                {
                    c.rrect(g.resX0 + 2.0f, y - 11.0f, g.resX1 - g.resX0 - 4.0f, 22.0f, 11.0f, Palette::mix(p.panel, fill, 0.35f), 1.5f, fill);
                    c.dotted(g.resX0 + 10.0f, y - 4.0f, g.resX1 - g.resX0 - 20.0f, 4.0f, Palette::mix(p.panel, fill, 0.9f));
                    c.dotted(g.resX0 + 12.0f, y + 3.0f, g.resX1 - g.resX0 - 22.0f, 4.0f, Palette::mix(p.panel, fill, 0.9f));
                }
                static const char* const kRes[] = { "", "QUARTER-WAVE", "HELMHOLTZ", "BOTTLE" };
                const float ty = b.res[k] == 3 ? (b.sides == 2 && !lower ? y - 28.0f : y + 17.0f)
                                               : (lower ? y + 46.0f : (b.sides == 2 ? y - 60.0f : y - 62.0f));
                centred(c, kRes[b.res[k]], xm, ty, look::kCaps, p.label);
            }
            if (b.muf[k] != 0)
            {
                const Col fill = partInk(p, kMuff);
                const float x0 = g.mufX0, w = g.mufX1 - g.mufX0;
                const Col body = Palette::mix(p.panel, fill, 0.3f), inner = Palette::mix(p.panel, fill, 0.85f);
                if (b.muf[k] == 1)       // straight-through: a round can, a perforated core
                {
                    c.rrect(x0, y - 14.0f, w, 28.0f, 14.0f, body, 1.5f, fill);
                    c.segment(x0 + 4.0f, y, x0 + w - 4.0f, y, 7.0f, inner);
                    c.dotted(x0 + 12.0f, y - 6.5f, w - 24.0f, 4.0f, inner);
                    c.dotted(x0 + 12.0f, y + 5.5f, w - 24.0f, 4.0f, inner);
                }
                else if (b.muf[k] == 2)  // chambered: baffles, the tube in, the tube out
                {
                    c.rrect(x0, y - 17.0f, w, 34.0f, 6.0f, body, 1.5f, fill);
                    const float b1 = x0 + w * 0.36f, b2 = x0 + w * 0.68f;
                    c.segment(b1, y - 15.0f, b1, y + 15.0f, 1.5f, fill);
                    c.segment(b2, y - 15.0f, b2, y + 15.0f, 1.5f, fill);
                    c.segment(x0 - 2.0f, y, b2 - 8.0f, y, 4.0f, inner);
                    c.segment(b1 + 8.0f, y + 8.0f, x0 + w + 2.0f, y + 8.0f, 4.0f, inner);
                    c.segment(b1 + 8.0f, y + 8.0f, b1 + 8.0f, y + 8.0f, 4.0f, inner);
                }
                else                     // three-pass: the gas's way back and forth
                {
                    c.rrect(x0, y - 18.0f, w, 36.0f, 6.0f, body, 1.5f, fill);
                    const float xs[6] = { x0 - 2.0f, x0 + w - 12.0f, x0 + w - 12.0f, x0 + 12.0f, x0 + 12.0f, x0 + w + 2.0f };
                    const float ys[6] = { y - 9.0f, y - 9.0f, y, y, y + 9.0f, y + 9.0f };
                    c.polyline(xs, ys, 6, 3.5f, inner);
                }
                static const char* const kMuf[] = { "", "STRAIGHT-THROUGH", "CHAMBERED", "THREE-PASS" };
                if (b.byp[k] != 0)   // the bypass takes the outer side: the word goes inside
                    centred(c, kMuf[b.muf[k]], x0 + w * 0.5f, b.sides == 2 && !lower ? y + 24.0f : y - 38.0f, look::kCaps, p.label);
                else
                    caption(kMuf[b.muf[k]], x0 + w * 0.5f, 18.0f);
                if (b.byp[k] != 0)
                    drawBypass(c, p, s);
            }
            // the radiating end: its arcs, its position, its level
            const float ex = g.endX + 6.0f;
            if (b.tips[k] == 2)
            {
                radiating(c, p, ex, y - 6.0f, 0.8f);
                radiating(c, p, ex, y + 6.0f, 0.8f);
            }
            else
                radiating(c, p, ex, y);
            // its level in dB (card G's endLevel), the bar 30 dB deep under the loudest end
            const double db = endDb(s);
            double loudest = db;
            for (int o = 0; o < b.sides; ++o)
                if (const double d = endDb(o); std::isfinite(d) && !(d <= loudest))
                    loudest = d;
            const float tx = ex + 22.0f, ty = y - 16.0f;
            c.text(tagName(b.tag[k]), tx, ty, look::kCaps, p.text);
            look::level(c, p, { tx, ty + 15.0f, 54.0f, 4.0f }, std::isfinite(db) ? (db - loudest + 30.0) / 30.0 : std::nan(""), p.text);
            const std::string dbs = std::isfinite(db) ? look::num(db, "%.0f dB") : "- dB";
            c.text(dbs.c_str(), tx, ty + 22.0f, look::kUnit, std::isfinite(db) ? p.label : p.dim);
        }

        // The bypass round the muffler, its butterfly drawn as it stands: along the pipe open, across it shut.
        void drawBypass(Canvas& c, const Palette& p, int s) const
        {
            const ExhaustBuild& b = build_;
            const Geo& g = geo_;
            const auto k = static_cast<std::size_t>(s);
            const float y = g.laneY[k];
            const float out = b.sides == 2 && s == 1 ? 1.0f : -1.0f;
            const float vy = y + out * 36.0f, vx = (g.mufX0 + g.mufX1) * 0.5f;
            const Col fill = pipeFill(p, kMuff), edge = pipeEdge(p);
            std::vector<Point> pts = sBendPts({ g.mufX0 - 10.0f, y }, { vx - 14.0f, vy }, 10);
            pts.push_back({ vx + 14.0f, vy });
            const std::vector<Point> back = sBendPts({ vx + 14.0f, vy }, { g.mufX1 + 10.0f, y }, 10);
            pts.insert(pts.end(), back.begin() + 1, back.end());
            pipeLine(c, pts, 6.0f, fill, edge);
            // the valve's position: telemetry, else what the field fixes (shut, open), else unknown
            // shut and open are fixed by the field; by speed and load, or by the ECU, the simulator says where it stands
            double open = b.byp[k] == 1 ? 0.0 : b.byp[k] == 2 ? 1.0 : ctx_.live.get(N::valve[s]);
            c.disc(vx, vy, 9.0f, p.panel, 1.5f, p.text);
            const double a = std::isfinite(open) ? (1.0 - std::clamp(open, 0.0, 1.0)) * 1.5707963 : 0.785398;
            const float dx = static_cast<float>(std::cos(a)) * 7.0f, dy = static_cast<float>(std::sin(a)) * 7.0f;
            c.segment(vx - dx, vy - dy, vx + dx, vy + dy, 2.5f, std::isfinite(open) ? p.value : p.dim);
            const char* word = !std::isfinite(open) ? "VALVE -" : open > 0.95 ? "OPEN" : open < 0.05 ? "SHUT" : "OPENING";
            c.text(word, vx + 14.0f, vy + (out < 0 ? -18.0f : 6.0f), look::kCaps, std::isfinite(open) ? p.text : p.dim);
        }

        // ---- the pulses: the Scope's pressure along cylinder 1's path, riding it --------------------------------
        bool pointAt(float metres, Point& at, Point& normal) const
        {
            float m = metres;
            for (const Piece& pc : geo_.path)
            {
                if (m > pc.metres && &pc != &geo_.path.back())
                {
                    m -= pc.metres;
                    continue;
                }
                // along this piece's polyline by its own fraction
                float total = 0;
                for (std::size_t i = 1; i < pc.pts.size(); ++i)
                    total += std::hypot(pc.pts[i].x - pc.pts[i - 1].x, pc.pts[i].y - pc.pts[i - 1].y);
                float want = total * std::clamp(pc.metres > 0 ? m / pc.metres : 0.0f, 0.0f, 1.0f);
                for (std::size_t i = 1; i < pc.pts.size(); ++i)
                {
                    const float dx = pc.pts[i].x - pc.pts[i - 1].x, dy = pc.pts[i].y - pc.pts[i - 1].y;
                    const float l = std::hypot(dx, dy);
                    if (want <= l || i + 1 == pc.pts.size())
                    {
                        const float t = l > 1e-4f ? std::clamp(want / l, 0.0f, 1.0f) : 0.0f;
                        at = { pc.pts[i - 1].x + dx * t, pc.pts[i - 1].y + dy * t };
                        normal = l > 1e-4f ? Point{ dy / l, -dx / l } : Point{ 0.0f, -1.0f };
                        return true;
                    }
                    want -= l;
                }
            }
            return false;
        }

        void drawPulse(Canvas& c, const Palette& p) const
        {
            if (shown_.size() < 2 || geo_.path.empty() || geo_.pathMetres <= 0)
                return;
            const int n = static_cast<int>(shown_.size()), m = 220;
            std::vector<float> xs, ys;
            xs.reserve(m);
            ys.reserve(m);
            for (int i = 0; i < m; ++i)
            {
                const float f = (static_cast<float>(i) + 0.5f) / static_cast<float>(m);
                // where along the Scope's points this is: by their distances when card G gives them, else spaced evenly
                // by length, each at (k + 0.5) / n of the path
                float u = std::clamp(f * static_cast<float>(n) - 0.5f, 0.0f, static_cast<float>(n - 1));
                if (static_cast<int>(pathX_.size()) == n)
                {
                    const float tot = pathX_[static_cast<std::size_t>(n - 1)]
                                    + 0.5f * (pathX_[static_cast<std::size_t>(n - 1)] - pathX_[static_cast<std::size_t>(n - 2)]);
                    const float x = f * tot;
                    int k = 0;
                    while (k + 2 < n && pathX_[static_cast<std::size_t>(k + 1)] < x)
                        ++k;
                    const float x0 = pathX_[static_cast<std::size_t>(k)], x1 = pathX_[static_cast<std::size_t>(k + 1)];
                    u = std::clamp(static_cast<float>(k) + (x - x0) / std::max(1e-6f, x1 - x0), 0.0f, static_cast<float>(n - 1));
                }
                const int k0 = static_cast<int>(u);
                const int k1 = std::min(k0 + 1, n - 1);
                const float w = u - static_cast<float>(k0);
                const float pa = shown_[static_cast<std::size_t>(k0)] * (1.0f - w) + shown_[static_cast<std::size_t>(k1)] * w;
                Point at{}, nr{};
                if (!pointAt(f * geo_.pathMetres, at, nr))
                    continue;
                const float off = std::clamp(pa / kPulseScale, -1.0f, 1.0f) * kPulseSwing;
                xs.push_back(at.x + nr.x * off);
                ys.push_back(at.y + nr.y * off);
            }
            if (xs.size() >= 2)
                c.polyline(xs.data(), ys.data(), static_cast<int>(xs.size()), 1.75f, p.value);
        }

        // Every other pipe's own pressure (card G's ductNear and ductFar, at each slot's two ends), riding it as a fainter
        // trace: the primaries in their firing order, the main pipes and the tails. Between its ends and its middle
        // (ductMid) a pipe's pressure is drawn straight; without the middle, straight from end to end.
        void drawDucts(Canvas& c, const Palette& p) const
        {
            if (!ducts_)
                return;
            const ExhaustBuild& b = build_;
            const Geo& g = geo_;
            const Col ink = Palette::mix(p.ground, p.value, 0.85f);
            const auto ride = [&](const std::vector<Point>& pts, int slot) {
                const auto k = static_cast<std::size_t>(slot);
                if (slot < 0 || slot >= kDucts || ductUsed_[k] < 0.5f || pts.size() < 2)
                    return;
                float total = 0;
                for (std::size_t i = 1; i < pts.size(); ++i)
                    total += std::hypot(pts[i].x - pts[i - 1].x, pts[i].y - pts[i - 1].y);
                std::vector<float> xs, ys;
                float along = 0;
                for (std::size_t i = 0; i < pts.size(); ++i)
                {
                    if (i > 0)
                        along += std::hypot(pts[i].x - pts[i - 1].x, pts[i].y - pts[i - 1].y);
                    const std::size_t a = i == 0 ? 0 : i - 1, z = i == 0 ? 1 : i;
                    const float dx = pts[z].x - pts[a].x, dy = pts[z].y - pts[a].y, l = std::max(1e-4f, std::hypot(dx, dy));
                    const float t = total > 0 ? along / total : 0.0f;
                    // near to middle to far when the Scope gives the middle; else straight between the ends
                    const float pa = !ductMidOn_ ? ductNear_[k] + (ductFar_[k] - ductNear_[k]) * t
                                     : t < 0.5f  ? ductNear_[k] + (ductMid_[k] - ductNear_[k]) * (2.0f * t)
                                                 : ductMid_[k] + (ductFar_[k] - ductMid_[k]) * (2.0f * t - 1.0f);
                    const float off = std::clamp(pa / kPulseScale, -1.0f, 1.0f) * kPulseSwing;
                    xs.push_back(pts[i].x + dy / l * off);
                    ys.push_back(pts[i].y - dx / l * off);
                }
                c.polyline(xs.data(), ys.data(), static_cast<int>(xs.size()), 1.25f, ink);
            };
            for (int i = 0; i < b.ports; ++i)
                if (i != 0 || shown_.size() < 2)   // cylinder 1's primary is the path's
                    ride(primaryPts(i), i);
            for (int sd = 0; sd < b.sides; ++sd)
            {
                if (!b.sideOn[static_cast<std::size_t>(sd)] || g.endX <= 0)
                    continue;
                const float y = g.laneY[static_cast<std::size_t>(sd)];
                const int base = 48 + sd * 15;
                const bool path = shown_.size() >= 2 && sd == std::min(b.colTo[static_cast<std::size_t>(std::max(0, b.portTo[0]))], 1);
                if (!path)
                {
                    ride({ { g.mainX0, y }, { g.mainX1, y } }, base);
                    ride({ { g.tailX0, y }, { g.endX, y } }, base + 11);
                }
            }
        }

        // ---- the numbers at the port, the collector and the tail; the pulses' legend ------------------------------
        void drawReadouts(Canvas& c, const Palette& p) const
        {
            const ExhaustBuild& b = build_;
            const Geo& g = geo_;
            const Rect& s = stage_;
            const float y = s.bottom() - 62.0f;
            c.hairlineH(s.x + 20.0f, y - 8.0f, s.w - 40.0f, p.grid);
            float x = s.x + 20.0f;
            // the gas's temperatures, each with its label; a part the build lacks (an open stack has no collector and
            // no tail) has none, and an absent value says so in its label's place
            const bool chain = b.sideOn[0] || b.sideOn[1];
            const double tp = ctx_.live.get(N::port);
            const double tc = chain ? ctx_.live.get(N::collector) : std::nan(""), tt = chain ? ctx_.live.get(N::tail) : std::nan("");
            c.text("GAS", x, y, look::kCaps, p.label);
            x += 40.0f;
            const auto gas = [&](const char* label, double k, bool has) {
                if (!has)
                    return;
                x += look::readout(c, p, x, y, label, temp(k), {}, look::kNumberM, std::isfinite(k) ? heat(p, k) : kClear) + 28.0f;
            };
            gas("PORTS", tp, true);
            gas("COLLECTOR", tc, chain);
            gas("TAIL", tt, chain);
            // the pulses' legend: what the trace is and its scale; the slow switch's state on its own line
            const float lx = std::max(x + 12.0f, s.right() - 20.0f - 250.0f);
            c.text("PULSES", lx, y, look::kCaps, p.label);
            if (shown_.size() >= 2)
            {
                c.segment(lx, y + 21.0f, lx + 18.0f, y + 21.0f, 1.75f, p.value);
                const std::string t = "cylinder 1 to its tail, +-" + ctx_.units.format(Quantity::pressure, kPulseScale, 0);
                c.text(t.c_str(), lx + 24.0f, y + 15.0f, look::kBody, p.text);
                if (slow_)
                    c.text("SLOWED 8 TIMES", lx + 24.0f, y + 32.0f, look::kCaps, p.selected);
            }
            else
                c.text("no pipe pressure from the Scope yet", lx, y + 15.0f, look::kBody, p.dim);
            // on the diagram: the numbers at cylinder 1's port and its collector, only where there are numbers
            if (b.ports > 0)
            {
                const float ty = g.banks[0].y - 16.0f;
                if (std::isfinite(tp))
                {
                    c.text("PORT", g.headX + 6.0f, ty, look::kCaps, p.label);
                    c.text(temp(tp).c_str(), g.headX + 12.0f + c.textWidth("PORT", look::kCaps), ty, look::kUnit, p.text);
                }
                const int c0 = b.portTo[0];
                if (c0 >= 0 && std::isfinite(tc))
                {
                    const auto ck = static_cast<std::size_t>(c0);
                    c.text(temp(tc).c_str(), g.colX[ck], g.colY[ck] - g.colH[ck] * 0.5f - 16.0f, look::kUnit, p.text);
                }
            }
        }

        // ---- the hovered and the selected: a ring, and the selected one's name ------------------------------------
        void drawMarks(Canvas& c, const Palette& p) const
        {
            const std::string& sel = ctx_.selection.path();
            const std::string& hov = ctx_.selection.hovered();
            for (const Hit& h : geo_.hits)
                if (!hov.empty() && h.id == hov && h.id != sel)
                    c.rrect(h.r.x - 2.0f, h.r.y - 2.0f, h.r.w + 4.0f, h.r.h + 4.0f, 6.0f, kClear, 1.0f, p.dim);
            for (const Hit& h : geo_.hits)
                if (!sel.empty() && h.id == sel)
                {
                    look::selectRing(c, p, h.r, 5.0f);
                    // its name, on a ground of its own so it reads over the pipes
                    const float w = c.textWidth(h.name.c_str(), look::kCaps) + 12.0f;
                    const float nx = std::clamp(h.r.x - 3.0f, stage_.x + 4.0f, stage_.right() - w - 4.0f);
                    const float ny = h.r.y - 24.0f < stage_.y + 70.0f ? h.r.bottom() + 6.0f : h.r.y - 24.0f;
                    c.rrect(nx, ny, w, 17.0f, 4.0f, p.selected);
                    c.text(h.name.c_str(), nx + 6.0f, ny + 3.5f, look::kCaps, p.ground);
                    break;   // the first of its id (a part on both sides is one selection)
                }
        }

        ExhaustBuild build_;
        Geo geo_;
        std::uint64_t seenDoc_ = 0;
        Button* slowButton_ = nullptr;
        Rect slowButtonRect_{};
        bool slow_ = false;
        double clock_ = 0, play_ = 0;
        float phase_ = 0.0f;
        // a two-stroke's: the sections' x (port, the four joints, the stinger's end), the silencer's end, the scale
        // (px a metre), each cylinder's lane, the port plot, cylinder 1's port pressure over the last turn
        std::array<float, 6> tsX_{};
        float tsEnd_ = 0.0f, tsScale_ = 0.0f;
        std::vector<float> tsLane_;
        Rect tsPlot_{};
        std::vector<float> portTrace_;
        static constexpr int kDucts = 84;
        bool ducts_ = false;
        bool ductMidOn_ = false;
        std::array<float, kDucts> ductUsed_{}, ductNear_{}, ductFar_{}, ductMid_{};
        std::vector<float> pathX_;   // card G's distances of the path's points from the port (m), when it gives them
        std::uint64_t scopeGen_ = ~0ull;
        std::vector<Snap> history_;
        std::vector<float> shown_;
    };

    std::unique_ptr<Workspace> makeExhaustWorkspace(WorkspaceContext& c) { return std::make_unique<ExhaustWorkspace>(c); }
}
