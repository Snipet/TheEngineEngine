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
            return wantsFullRate() || (slow_ && history_.size() >= 2 && play_ < history_.back().t - 1e-6);
        }

        std::uint64_t drawKey() const override
        {
            DrawKey k;
            k.mix(ctx_.doc.revision()).mix(ctx_.selection.revision()).mix(ctx_.units.revision()).mix(slow_ ? 1 : 0);
            for (float v : shown_)
                k.mix(v);
            k.mix(static_cast<int>(shown_.size()));
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
            drawHeader(c, p);
            drawPipes(c, p);
            drawParts(c, p);
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
                return { at("exhaust.muffler", i), at("exhaust.mufSize", i), at("exhaust.mufLen", i), at("exhaust.bypass", i),
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
                return { "exhaust.crossover", "exhaust.crossAt", "exhaust.crossD", "exhaust.crossLen", "exhaust.postLen" };
            }
            if (sel == "boost")
            {
                title = "Turbines";
                return { "boost.kind", "boost.layout", "boost.turbWheel", "boost.turbAR", "boost.turbEta", "boost.scrolls",
                         "boost.housingVolume", "boost.vanes", "boost.vaneMin", "boost.gate", "boost.gateDiameter",
                         "exhaust.gateLen", "exhaust.gateD" };
            }
            if (sel == "engine")
            {
                title = "Layout";
                return { "engine.layout", "exhaust.model" };
            }
            return {};
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
            geo_ = layoutExhaust(build_, d);
        }

        void readScope()
        {
            if (scopeHandle_ < 0 || ctx_.scope.generation() != scopeGen_ || history_.empty())
            {
                if (scopeHandle_ < 0)
                    scopeHandle_ = ctx_.scope.find(N::scopePipe);
                if (scopeHandle_ < 0)
                    return;
                float buf[256];
                const int n = ctx_.scope.read(scopeHandle_, buf, 256);
                scopeGen_ = ctx_.scope.generation();
                if (n <= 1)
                    return;
                // over the air outside (the demo gives absolute pressures; the simulator's are over the air)
                double ambient = fieldOr(ctx_.doc, N::ambient, -1, kAmbient);
                double mean = 0;
                for (int i = 0; i < n; ++i)
                    mean += buf[i];
                mean /= n;
                if (mean < ambient * 0.5)
                    ambient = 0.0;
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
        }

        std::array<double, 15> liveValues() const
        {
            std::array<double, 15> v{};
            v[0] = ctx_.live.get(N::port);
            v[1] = ctx_.live.get(N::collector);
            v[2] = ctx_.live.get(N::tail);
            for (int i = 0; i < 7; ++i)
                v[static_cast<std::size_t>(3 + i)] = ctx_.live.get(N::walls[i]);
            v[10] = ctx_.live.get(N::level[0]);
            v[11] = ctx_.live.get(N::level[1]);
            v[12] = ctx_.live.get(N::valve[0]);
            v[13] = ctx_.live.get(N::valve[1]);
            v[14] = 0;
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
        Col pipeFill(const Palette& p, int section) const
        {
            const double t = wall(section);
            return std::isfinite(t) ? p.heat(t) : Palette::mix(p.panel, p.text, 0.32f);
        }
        Col pipeEdge(const Palette& p) const { return p.dark ? Palette::mix(p.ground, p.edge, 0.6f) : p.edge; }
        std::string temp(double k) const
        {
            return std::isfinite(k) ? ctx_.units.format(Quantity::temperature, k, 0) : "-";
        }

        // ---- the header: the build in a sentence, the heat's scale --------------------------------------------------
        void drawHeader(Canvas& c, const Palette& p) const
        {
            const Rect& s = stage_;
            c.text("EXHAUST", s.x + 20.0f, s.y + 14.0f, look::kTitle, p.label);
            c.text(build_.sentence().c_str(), s.x + 20.0f, s.y + 34.0f, look::kBody, p.text);
            // the walls' scale: a strip of the tint with its ends in the user's unit (or a word while there is none)
            const float x1 = slowButtonRect_.x - 24.0f, w = 168.0f, x0 = x1 - w, y = s.y + 30.0f;
            c.text("WALL", x0 - c.textWidth("WALL", look::kCaps) - 8.0f, y - 3.0f, look::kCaps, p.label);
            const int n = 24;
            for (int i = 0; i < n; ++i)
            {
                const double k = 350.0 + (1100.0 - 350.0) * (i + 0.5) / n;
                c.rrect(x0 + w * static_cast<float>(i) / n, y, w / n + 0.5f, 6.0f, 0.0f, p.heat(k));
            }
            c.text(temp(350.0).c_str(), x0, y + 10.0f, look::kUnit, p.label);
            const std::string hot = temp(1100.0);
            c.text(hot.c_str(), x1 - c.textWidth(hot.c_str(), look::kUnit), y + 10.0f, look::kUnit, p.label);
            if (!std::isfinite(wall(kPrim)) && !std::isfinite(wall(kMain)))
                c.text("NO WALL TEMPERATURES YET", x0, y - 16.0f, look::kCaps, p.dim);
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
                c.rrect(g.headX - 4.0f, g.portY[k] - 5.0f, 6.0f, 10.0f, 2.0f, pipeFill(p, kPrim));
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
                const Col fill = pipeFill(p, kColl);
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
                const Col fill = pipeFill(p, kMain);
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
                const Col fill = pipeFill(p, kLink);
                const float xm = (g.resX0 + g.resX1) * 0.5f;
                if (b.res[k] == 1)       // a quarter-wave stub: a dead-ended branch
                {
                    pipeLine(c, { { xm, y }, { xm, y + out * 34.0f } }, 7.0f, fill, edge);
                    c.segment(xm - 6.0f, y + out * 38.0f, xm + 6.0f, y + out * 38.0f, 2.5f, edge);
                }
                else if (b.res[k] == 2)  // a Helmholtz: a neck into a chamber
                {
                    pipeLine(c, { { xm, y }, { xm, y + out * 20.0f } }, 5.0f, fill, edge);
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
                const Col fill = pipeFill(p, kMuff);
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
            const double lv = ctx_.live.get(N::level[s]);
            const float tx = ex + 22.0f, ty = y - 16.0f;
            c.text(tagName(b.tag[k]), tx, ty, look::kCaps, p.text);
            look::level(c, p, { tx, ty + 15.0f, 54.0f, 4.0f }, lv, p.text);
            const std::string db = std::isfinite(lv) && lv > 0.0 ? look::num(20.0 * std::log10(lv), "%.0f dB") : "- dB";
            c.text(db.c_str(), tx, ty + 22.0f, look::kUnit, std::isfinite(lv) ? p.label : p.dim);
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
                // the Scope's points are spaced evenly by length, each at (k + 0.5) / n of the path
                const float u = std::clamp(f * static_cast<float>(n) - 0.5f, 0.0f, static_cast<float>(n - 1));
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

        // ---- the numbers at the port, the collector and the tail; the pulses' legend ------------------------------
        void drawReadouts(Canvas& c, const Palette& p) const
        {
            const ExhaustBuild& b = build_;
            const Geo& g = geo_;
            const Rect& s = stage_;
            const float y = s.bottom() - 62.0f;
            c.hairlineH(s.x + 20.0f, y - 8.0f, s.w - 40.0f, p.grid);
            float x = s.x + 20.0f;
            const double tp = ctx_.live.get(N::port);
            x += look::readout(c, p, x, y, "GAS AT THE PORTS", temp(tp), {}, look::kNumberM, std::isfinite(tp) ? p.heat(tp) : kClear) + 36.0f;
            // a part the build lacks has no number (an open stack has no collector and no tail)
            const bool chain = b.sideOn[0] || b.sideOn[1];
            const double tc = chain ? ctx_.live.get(N::collector) : std::nan(""), tt = chain ? ctx_.live.get(N::tail) : std::nan("");
            x += look::readout(c, p, x, y, "AT THE COLLECTOR", temp(tc), {}, look::kNumberM, std::isfinite(tc) ? p.heat(tc) : kClear) + 36.0f;
            x += look::readout(c, p, x, y, "AT THE TAIL", temp(tt), {}, look::kNumberM, std::isfinite(tt) ? p.heat(tt) : kClear) + 48.0f;
            // the pulses' legend: what the trace is, its scale, and the slow switch's state
            const float lx = std::max(x, s.right() - 330.0f);
            c.text("PULSES", lx, y, look::kCaps, p.label);
            if (shown_.size() >= 2)
            {
                c.segment(lx, y + 23.0f, lx + 22.0f, y + 23.0f, 1.75f, p.value);
                char t[96];
                std::snprintf(t, sizeof t, "cylinder 1 to the tail, +-%s full swing%s",
                              ctx_.units.format(Quantity::pressure, kPulseScale, 0).c_str(), slow_ ? ", slowed 8x" : "");
                c.text(t, lx + 30.0f, y + 17.0f, look::kBody, p.text);
            }
            else
                c.text("no pipe pressure from the Scope yet", lx, y + 17.0f, look::kBody, p.dim);
            // small numbers on the diagram: at cylinder 1's port, at its collector, at the first tail
            if (b.ports > 0)
            {
                const std::string t = temp(ctx_.live.get(N::port));
                const float ty = g.banks[0].y - 16.0f;
                c.text("PORT", g.headX + 6.0f, ty, look::kCaps, p.label);
                c.text(t.c_str(), g.headX + 12.0f + c.textWidth("PORT", look::kCaps), ty, look::kUnit, p.text);
                const int c0 = b.portTo[0];
                if (c0 >= 0)
                {
                    const auto ck = static_cast<std::size_t>(c0);
                    const std::string tcol = temp(ctx_.live.get(N::collector));
                    c.text(tcol.c_str(), g.colX[ck], g.colY[ck] - g.colH[ck] * 0.5f - 16.0f, look::kUnit, p.text);
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
        int scopeHandle_ = -1;
        std::uint64_t scopeGen_ = ~0ull;
        std::vector<Snap> history_;
        std::vector<float> shown_;
    };

    std::unique_ptr<Workspace> makeExhaustWorkspace(WorkspaceContext& c) { return std::make_unique<ExhaustWorkspace>(c); }
}
