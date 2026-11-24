// ui/views/air/AirWorkspace.cpp: see AirWorkspace.h. Canvas primitives only: discs and stroked arcs for the
// compressors' scrolls and the throttle, rounded rects for the boxes, hairlines for the fins and the filter's pleats,
// polylines for the map's lines and islands, segments for the trail (opaque, premixed by age), text. Colour is never the
// only sign: each tint has its number, each lamp its word, the selection a ring and a name.
#include "AirWorkspace.h"

#include "AirModel.h"

#include "app/Look.h"
#include "kit/bind/Binding.h"

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
        using namespace air;
        using funkgui::Canvas;
        using funkgui::Col;
        using funkgui::Point;
        using funkgui::Rect;
        using look::Palette;
        using N = AirNames;

        constexpr double kTwoPi = 6.28318530717958647692;
        constexpr int kTrail = 180;
        constexpr int kHistory = 90;

        struct Hit
        {
            Rect r;
            std::string id, name;
        };

        enum class Kind { box, turbo, blower, cooler, throttle, plenum, runners };
        struct Part
        {
            Kind kind;
            Rect r;            // its shape's bounds
            int index = 0;     // a compressor's
        };

        void centred(Canvas& c, const std::string& s, float x, float y, const funkgui::TextStyle& st, Col ink)
        {
            c.text(s.c_str(), x - c.textWidth(s.c_str(), st) * 0.5f, y, st, ink);
        }
        std::vector<Point> sBend(Point a, Point b, int n = 14)
        {
            std::vector<Point> out;
            const float mx = (a.x + b.x) * 0.5f;
            for (int i = 0; i <= n; ++i)
            {
                const float t = static_cast<float>(i) / static_cast<float>(n), u = 1.0f - t;
                const float k0 = u * u * u, k1 = 3 * u * u * t, k2 = 3 * u * t * t, k3 = t * t * t;
                out.push_back({ k0 * a.x + k1 * mx + k2 * mx + k3 * b.x, k0 * a.y + k1 * a.y + k2 * b.y + k3 * b.y });
            }
            return out;
        }
        void line(Canvas& c, const std::vector<Point>& pts, float w, Col ink)
        {
            std::vector<float> xs, ys;
            for (const Point& p : pts)
            {
                xs.push_back(p.x);
                ys.push_back(p.y);
            }
            if (xs.size() >= 2)
                c.polyline(xs.data(), ys.data(), static_cast<int>(xs.size()), w, ink);
        }
        // equal, or both absent (NaN is never equal to itself)
        bool same(float a, float b) { return a == b || (std::isnan(a) && std::isnan(b)); }
        double niceStep(double span)
        {
            const double raw = span / 5.0, mag = std::pow(10.0, std::floor(std::log10(std::max(raw, 1e-9))));
            const double f = raw / mag;
            return (f < 1.5 ? 1.0 : f < 3.5 ? 2.0 : f < 7.5 ? 5.0 : 10.0) * mag;
        }
    }

    class AirWorkspace final : public Workspace
    {
    public:
        explicit AirWorkspace(WorkspaceContext& c) : Workspace(c) {}

        const char* id() const override { return "air"; }
        const char* title() const override { return "AIR"; }
        std::vector<std::string> sections() const override { return { "intake", "boost" }; }
        bool moving() const override { return wantsFullRate(); }

        // The selected part's fields on a page of their own (those the build has; none: the frame's pages).
        std::string inspectorPage() const override
        {
            struct Group
            {
                const char* sel;
                const char* title;
                std::vector<const char*> fields;
            };
            static const std::vector<const char*> kCompressor = { "boost.kind", "boost.layout", "boost.size", "boost.compInducer",
                                                                 "boost.compExducer", "boost.compAR", "boost.compEta" };
            static const Group kGroups[] = {
                { "intake.airbox", "Filter and air box", { "intake.boxVolume", "intake.snorkelLength", "intake.snorkelBore", "intake.filter" } },
                { "boost.turbo", "Turbo", { "boost.kind", "boost.layout", "boost.size", "boost.compInducer", "boost.compExducer",
                                            "boost.compAR", "boost.compEta", "boost.turbWheel", "boost.turbAR", "boost.scrolls",
                                            "boost.vanes", "boost.vaneMin", "boost.seqFlow" } },
                { "boost.compressor", "Compressor", kCompressor },
                { "boost.supercharger", "Supercharger", { "boost.kind", "boost.charger", "boost.scDisplacement", "boost.scRatio",
                                                          "boost.scVolumeRatio", "boost.scStepUp", "boost.scBypass" } },
                { "boost.intercooler", "Intercooler", { "boost.icEffect", "boost.icDrop", "boost.icFlow", "boost.icMass",
                                                        "boost.chargeVolume" } },
                { "intake.throttle", "Throttle", { "intake.throttleKind", "intake.throttleArea", "intake.trumpets", "intake.throttle" } },
                { "intake.plenum", "Plenum", { "intake.plenumVolume", "intake.plenum" } },
                { "intake.runners", "Runners", { "intake.throttleKind", "intake.trumpets", "intake.runnerLength", "intake.runnerBore",
                                                 "intake.runnerLength2", "intake.runnerSwitch", "intake.runner" } },
                { "boost.shaft", "Shaft", { "boost.inertia", "boost.bearing", "boost.assist", "boost.maxSpeed" } },
                { "boost.gate", "Wastegate", { "boost.gate", "boost.gateDiameter", "boost.gateSpring", "boost.target",
                                               "boost.overBoost", "boost.level", "boost.wastegateSpring" } },
                { "boost.blowOff", "Blow-off valve", { "boost.blowOffKind", "boost.blowOffSpring", "boost.blowOffDiameter", "boost.blowOff" } },
                { "boost.antiLag", "Anti-lag", { "boost.antiLag" } },
                { "boost.nitrous", "Nitrous", { "boost.nitrousShot", "boost.nitrousBottle" } },
            };
            for (const Group& g : kGroups)
                if (ctx_.selection.is(g.sel))
                {
                    std::vector<int> fields;
                    for (const char* path : g.fields)
                        if (const int f = ctx_.doc.find(path); f >= 0 && !ctx_.doc.info(f).hidden)
                            fields.push_back(f);
                    return fields.empty() ? std::string() : pageGui(ctx_.doc, fields, g.title);
                }
            return {};
        }

        std::uint64_t drawKey() const override
        {
            DrawKey k;
            k.mix(ctx_.doc.revision()).mix(ctx_.selection.revision()).mix(ctx_.units.revision());
            for (const char* n : { N::boost, N::manifold, N::throttle, N::target, N::speed[0], N::speed[1], N::ratio[0],
                                   N::ratio[1], N::flow[0], N::flow[1], N::outT[0], N::outT[1], N::gate, N::vanes,
                                   N::icOut, N::scPower, N::scBypass, N::blowOff, N::antiLag, N::nitrous, N::bottle,
                                   N::box, N::plenumT })
                k.mix(static_cast<float>(ctx_.live.get(n)));
            k.mix(static_cast<int>(trail_.size())).mix(static_cast<int>(history_.size()));
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
            if (const Hit* h = hitAt({ e.x, e.y }); h != nullptr)
                ctx_.selection.select(h->id);
            else
                Board::pointerDown(e);
        }

    protected:
        void layout(const Rect&) override { rebuild(); }

        void step(float) override
        {
            if (ctx_.doc.revision() != seenDoc_)
                rebuild();
            // the boost's and the shaft's recent history (a point each time either moves)
            {
                const float bo = static_cast<float>(ctx_.live.get(N::boost)), sp = static_cast<float>(ctx_.live.get(N::speed[0]));
                if ((std::isfinite(bo) || std::isfinite(sp))
                    && (history_.empty() || !same(history_.back().x, bo) || !same(history_.back().y, sp)))
                {
                    history_.push_back({ bo, sp });
                    if (history_.size() > kHistory)
                        history_.erase(history_.begin());
                }
            }
            // the operating point's trail: a point each time it moves (a spool, a shift, a surge's flutter)
            const double f = ctx_.live.get(N::flow[0]), r = ctx_.live.get(N::ratio[0]);
            if (std::isfinite(f) && std::isfinite(r))
            {
                const Point pt{ static_cast<float>(f), static_cast<float>(r) };
                if (trail_.empty() || std::fabs(trail_.back().x - pt.x) > 1e-5f || std::fabs(trail_.back().y - pt.y) > 1e-5f)
                {
                    trail_.push_back(pt);
                    if (trail_.size() > kTrail)
                        trail_.erase(trail_.begin());
                }
            }
        }

        void drawStage(Canvas& c, const funkgui::Theme& th) const override
        {
            const Palette p = Palette::of(th);
            const Rect& s = stage_;
            c.rrect(s.x, s.y, s.w, s.h, 0.0f, p.ground);
            c.text("AIR", s.x + 20.0f, s.y + 14.0f, look::kTitle, p.label);
            c.text(build_.sentence().c_str(), s.x + 20.0f, s.y + 34.0f, look::kBody, p.text);
            drawPath(c, p);
            drawMap(c, p);
            drawShaft(c, p);
            drawValves(c, p);
            drawMarks(c, p);
        }

    private:
        // ---- layout -------------------------------------------------------------------------------------------------
        void rebuild()
        {
            seenDoc_ = ctx_.doc.revision();
            build_ = resolveAir(ctx_.doc);
            lines_ = build_.hasMap() ? mapLines(build_.map) : MapLines{};
            parts_.clear();
            hits_.clear();
            const Rect& s = stage_;
            // the row: the parts' names, the parts (h tall), their pressures and temperatures just beneath
            const float h = std::clamp(s.h * 0.17f, 100.0f, 150.0f);
            yc_ = s.y + 62.0f + 22.0f + 10.0f + h * 0.5f;
            readY_ = yc_ + h * 0.5f + 12.0f;
            row_ = { s.x + 20.0f, s.y + 62.0f, s.w - 40.0f, readY_ + 46.0f - (s.y + 62.0f) };
            const AirBuild& b = build_;
            // the parts and their widths
            struct W
            {
                Kind k;
                float w;
                int index;
            };
            std::vector<W> ws;
            if (!(b.itb && b.trumpetsToAir))   // trumpets open to the air have no box
                ws.push_back({ Kind::box, 86.0f, 0 });
            if (b.charger == Charger::turbo)
            {
                if (b.arrangement == Arrangement::compound)
                {
                    ws.push_back({ Kind::turbo, 62.0f, 0 });
                    ws.push_back({ Kind::turbo, 50.0f, 1 });
                }
                else
                    ws.push_back({ Kind::turbo, 62.0f, 0 });
            }
            else if (b.charger == Charger::supercharger)
                ws.push_back({ Kind::blower, b.blower == Blower::centrifugal ? 62.0f : 96.0f, 0 });
            if (b.intercooler)
                ws.push_back({ Kind::cooler, 52.0f, 0 });
            if (!b.itb)
            {
                ws.push_back({ Kind::throttle, 48.0f, 0 });
                ws.push_back({ Kind::plenum, 104.0f, 0 });
            }
            ws.push_back({ Kind::runners, b.itb ? 210.0f : 150.0f, 0 });
            float sum = 0;
            for (const W& w : ws)
                sum += w.w;
            const float gap = std::max(24.0f, (row_.w - sum - 70.0f) / static_cast<float>(std::max<std::size_t>(1, ws.size() - 1)));
            float x = row_.x + 14.0f;
            for (const W& w : ws)
            {
                Part part{ w.k, { x, yc_ - h * 0.5f, w.w, h }, w.index };
                parts_.push_back(part);
                x += w.w + gap;
            }
            for (const Part& part : parts_)
            {
                std::string id, name;
                switch (part.kind)
                {
                    case Kind::box: id = "intake.airbox"; name = "FILTER AND AIR BOX"; break;
                    case Kind::turbo:
                        id = "boost.turbo[" + std::to_string(part.index) + "]";
                        name = b.arrangement == Arrangement::compound ? (part.index == 0 ? "LOW-PRESSURE TURBO" : "HIGH-PRESSURE TURBO")
                             : b.turbos > 1 ? "TURBOS" : "TURBO";
                        break;
                    case Kind::blower: id = "boost.supercharger"; name = "SUPERCHARGER"; break;
                    case Kind::cooler: id = "boost.intercooler"; name = "INTERCOOLER"; break;
                    case Kind::throttle: id = "intake.throttle"; name = "THROTTLE"; break;
                    case Kind::plenum: id = "intake.plenum"; name = "PLENUM"; break;
                    case Kind::runners: id = "intake.runners"; name = b.itb ? "RUNNERS, A THROTTLE EACH" : "RUNNERS"; break;
                }
                hits_.push_back({ part.r, id, name });
            }
            // the lower row: the map, the shaft, the valves
            const float top = row_.bottom() + 12.0f, bottom = s.bottom() - 16.0f;
            const float w = s.w - 40.0f, g = 14.0f;
            mapR_ = { s.x + 20.0f, top, w * 0.46f, bottom - top };
            shaftR_ = { mapR_.right() + g, top, w * 0.25f - g, bottom - top };
            valveR_ = { shaftR_.right() + g, top, s.right() - 20.0f - shaftR_.right() - g, bottom - top };
            if (b.hasMap())
                hits_.push_back({ mapR_, "boost.compressor", "COMPRESSOR MAP" });
            if (b.charger != Charger::none)
                hits_.push_back({ shaftR_, b.charger == Charger::turbo ? "boost.shaft" : "boost.supercharger",
                                  b.charger == Charger::turbo ? "SHAFT" : "SUPERCHARGER" });
            gateR_ = { valveR_.x + 12.0f, valveR_.y + 30.0f, 84.0f, 82.0f };
            bovR_ = { gateR_.right() + 10.0f, gateR_.y, 84.0f, 82.0f };
            if (b.charger == Charger::turbo)
                hits_.push_back({ gateR_, "boost.gate", b.externalGate ? "EXTERNAL WASTEGATE" : "WASTEGATE" });
            if (b.blowOff != 0)
                hits_.push_back({ bovR_, "boost.blowOff", "BLOW-OFF VALVE" });
            const float ly = valveR_.bottom() - 34.0f;
            if (b.antiLag)
                hits_.push_back({ { valveR_.x + 10.0f, ly - 8.0f, 120.0f, 18.0f }, "boost.antiLag", "ANTI-LAG" });
            if (b.nitrous)
                hits_.push_back({ { valveR_.x + 140.0f, ly - 8.0f, 150.0f, 18.0f }, "boost.nitrous", "NITROUS" });
        }

        const Hit* hitAt(Point pt) const
        {
            for (auto it = hits_.rbegin(); it != hits_.rend(); ++it)
                if (it->r.contains(pt))
                    return &*it;
            return nullptr;
        }

        // ---- the values at each part ---------------------------------------------------------------------------------
        double live(const char* n) const { return ctx_.live.get(n); }
        double ambientP() const { return build_.ambientP; }
        // Pressure (absolute) and temperature after a part
        void after(const Part& part, double& pa, double& k) const
        {
            const double nan = std::nan("");
            pa = nan;
            k = nan;
            const double boost = live(N::boost);
            switch (part.kind)
            {
                case Kind::box: pa = live(N::box); k = build_.ambientT; break;
                case Kind::turbo:
                {
                    const int i = std::min(part.index, 1);
                    const double r = live(N::ratio[i]);
                    pa = std::isfinite(r) ? r * ambientP() : (!build_.intercooler && std::isfinite(boost) ? boost + ambientP() : nan);
                    k = live(N::outT[i]);
                    break;
                }
                case Kind::blower:
                    pa = std::isfinite(boost) ? boost + ambientP() : nan;
                    k = live(N::outT[0]);
                    break;
                case Kind::cooler:
                    pa = std::isfinite(boost) ? boost + ambientP() : nan;
                    k = live(N::icOut);
                    break;
                case Kind::throttle:
                case Kind::plenum:
                case Kind::runners:
                    pa = live(N::manifold);
                    k = live(N::plenumT);
                    break;
            }
        }
        std::string pres(double pa) const { return std::isfinite(pa) ? ctx_.units.format(Quantity::pressure, pa) : "-"; }
        std::string temp(double k) const { return std::isfinite(k) ? ctx_.units.format(Quantity::temperature, k, 0) : "-"; }
        Col airTint(const Palette& p, double k) const
        {
            return std::isfinite(k) ? p.heat(k, build_.ambientT + 5.0, build_.ambientT + 35.0, build_.ambientT + 190.0)
                                    : Palette::mix(p.panel, p.text, 0.32f);
        }

        // ---- the charge path ---------------------------------------------------------------------------------------
        void drawPath(Canvas& c, const Palette& p) const
        {
            const Col edge = p.dark ? Palette::mix(p.ground, p.edge, 0.6f) : p.edge;
            // the pipes first: each from a part to the next, tinted by the air leaving the first, its number above
            for (std::size_t i = 0; i + 1 < parts_.size(); ++i)
            {
                const Part& a = parts_[i];
                const Part& b = parts_[i + 1];
                double pa = 0, k = 0;
                after(a, pa, k);
                const Col tint = airTint(p, k);
                const float x0 = a.r.right(), x1 = b.r.x;
                c.segment(x0 - 4.0f, yc_, x1 + 4.0f, yc_, 12.0f, edge);
                c.segment(x0 - 4.0f, yc_, x1 + 4.0f, yc_, 10.0f, tint);
                const std::string t = temp(k);
                centred(c, t, (x0 + x1) * 0.5f, yc_ - 24.0f, look::kUnit, std::isfinite(k) ? p.text : p.dim);
                if (b.kind == Kind::throttle && build_.blowOff != 0)
                    drawBlowOffStub(c, p, x0 + (x1 - x0) * 0.22f, edge, tint);
            }
            for (const Part& part : parts_)
            {
                double pa = 0, k = 0;
                after(part, pa, k);
                const Col tint = airTint(p, k);
                drawPart(c, p, part, tint, edge);
                // its name above, its pressure and temperature beneath
                std::string name;
                for (const Hit& h : hits_)
                    if (h.r.x == part.r.x && h.r.y == part.r.y)
                        name = h.name;
                {
                    const float nw = c.textWidth(name.c_str(), look::kCaps);
                    const float nx = std::clamp(part.r.centreX() - nw * 0.5f, stage_.x + 8.0f, stage_.right() - 8.0f - nw);
                    c.text(name.c_str(), nx, row_.y, look::kCaps, p.label);
                }
                const float ry = readY_;
                const std::string ps = pres(pa), ts = temp(k);
                centred(c, ps, part.r.centreX(), ry, look::kNumberS, std::isfinite(pa) ? p.value : p.dim);
                centred(c, ts, part.r.centreX(), ry + 18.0f, look::kNumberS, std::isfinite(k) ? tint : p.dim);
            }
            c.hairlineH(row_.x, row_.bottom() - 2.0f, row_.w, p.grid);
        }

        void drawBlowOffStub(Canvas& c, const Palette& p, float x, Col edge, Col tint) const
        {
            const double lift = live(N::blowOff);
            c.segment(x, yc_, x, yc_ - 30.0f, 8.0f, edge);
            c.segment(x, yc_, x, yc_ - 30.0f, 6.0f, tint);
            const float up = std::isfinite(lift) ? static_cast<float>(std::clamp(lift, 0.0, 1.0)) * 6.0f : 0.0f;
            c.rrect(x - 8.0f, yc_ - 36.0f - up, 16.0f, 4.0f, 2.0f, std::isfinite(lift) ? p.value : p.dim);
        }

        void drawPart(Canvas& c, const Palette& p, const Part& part, Col tint, Col edge) const
        {
            const Rect& r = part.r;
            const AirBuild& b = build_;
            const Col body = Palette::mix(p.panel, tint, 0.28f), ink = Palette::mix(p.panel, tint, 0.9f);
            switch (part.kind)
            {
                case Kind::box:
                {
                    const float h = std::min(r.h, 86.0f), y = yc_ - h * 0.5f;
                    c.rrect(r.x, y, r.w, h, 6.0f, body, 1.5f, tint);
                    // the filter's pleats
                    std::vector<float> xs, ys;
                    for (int i = 0; i <= 12; ++i)
                    {
                        xs.push_back(r.x + r.w * 0.5f + (i % 2 == 0 ? -10.0f : 10.0f));
                        ys.push_back(y + 8.0f + (h - 16.0f) * static_cast<float>(i) / 12.0f);
                    }
                    c.polyline(xs.data(), ys.data(), static_cast<int>(xs.size()), 1.25f, ink);
                    // the snorkel in
                    c.segment(r.x - 14.0f, yc_ + h * 0.25f, r.x, yc_ + h * 0.25f, 10.0f, body);
                    break;
                }
                case Kind::turbo:
                {
                    const bool compound = b.arrangement == Arrangement::compound;
                    const int n = compound ? 1 : b.turbos;
                    const float rr = compound ? (part.index == 0 ? 27.0f : 21.0f) : n == 1 ? 27.0f : n == 2 ? 21.0f : 14.0f;
                    const float pitch = n == 1 ? 0.0f : std::min(r.h / static_cast<float>(n), 2.0f * rr + 10.0f);
                    for (int i = 0; i < n; ++i)
                    {
                        const float cy = yc_ + (static_cast<float>(i) - 0.5f * static_cast<float>(n - 1)) * pitch;
                        const float cx = r.centreX();
                        if (n > 1)   // split from the pipe in, merged into the pipe out
                        {
                            line(c, sBend({ r.x - 8.0f, yc_ }, { cx - rr, cy }), 6.0f, ink);
                            line(c, sBend({ cx + rr * 0.7f, cy - rr * 0.6f }, { r.right() + 8.0f, yc_ }), 6.0f, ink);
                        }
                        c.disc(cx, cy, rr, body, 2.0f, tint);
                        c.arc({ cx, cy }, rr * 0.6f, 0.0f, 4.7f, 1.5f, ink);
                        c.disc(cx, cy, 3.0f, ink);
                        if (n > 1 && n <= 2)
                        {
                            const char* tag = b.arrangement == Arrangement::sequential ? (i == 0 ? "PRIMARY" : "SECONDARY")
                                                                                        : (i == 0 ? "1" : "2");
                            c.text(tag, cx + rr + 6.0f, cy - 6.0f, look::kCaps, p.label);
                        }
                    }
                    break;
                }
                case Kind::blower:
                {
                    if (b.blower == Blower::centrifugal)
                    {
                        c.disc(r.centreX(), yc_, 26.0f, body, 2.0f, tint);
                        c.arc({ r.centreX(), yc_ }, 15.0f, 0.0f, 4.7f, 1.5f, ink);
                        c.text("BELT", r.centreX() - 12.0f, yc_ + 32.0f, look::kCaps, p.label);
                        break;
                    }
                    const float h = std::min(r.h, 70.0f), y = yc_ - h * 0.5f;
                    c.rrect(r.x, y, r.w, h, 12.0f, body, 1.5f, tint);
                    if (b.blower == Blower::roots)   // two lobed rotors
                    {
                        for (const float dx : { -16.0f, 16.0f })
                        {
                            c.disc(r.centreX() + dx, yc_, 15.0f, kClear, 1.5f, ink);
                            c.disc(r.centreX() + dx, yc_ - 7.0f, 4.0f, ink);
                            c.disc(r.centreX() + dx, yc_ + 7.0f, 4.0f, ink);
                        }
                    }
                    else                             // two screws: their flights as slanted strokes
                    {
                        for (int k = 0; k < 7; ++k)
                        {
                            const float x = r.x + 14.0f + static_cast<float>(k) * (r.w - 28.0f) / 6.0f;
                            c.segment(x - 4.0f, yc_ - 18.0f, x + 4.0f, yc_ - 4.0f, 1.5f, ink);
                            c.segment(x + 4.0f, yc_ + 4.0f, x - 4.0f, yc_ + 18.0f, 1.5f, ink);
                        }
                    }
                    centred(c, b.blower == Blower::roots ? "ROOTS" : "TWIN-SCREW", r.centreX(), y + h + 6.0f, look::kCaps, p.label);
                    break;
                }
                case Kind::cooler:
                {
                    const float h = std::min(r.h, 104.0f), y = yc_ - h * 0.5f;
                    c.rrect(r.x, y, r.w, h, 4.0f, body, 1.5f, tint);
                    for (float fy = y + 6.0f; fy < y + h - 4.0f; fy += 5.0f)
                        c.hairlineH(r.x + 6.0f, fy, r.w - 12.0f, ink);
                    break;
                }
                case Kind::throttle:
                {
                    const double open = live(N::throttle);
                    drawThrottle(c, p, r.centreX(), yc_, 20.0f, open, body, tint);
                    const std::string t = std::isfinite(open) ? look::num(open * 100.0, "%.0f %%") : "-";
                    centred(c, t, r.centreX(), yc_ + 26.0f, look::kUnit, std::isfinite(open) ? p.text : p.dim);
                    break;
                }
                case Kind::plenum:
                {
                    const float h = std::min(r.h, 104.0f);
                    c.rrect(r.x, yc_ - h * 0.5f, r.w, h, 16.0f, body, 1.5f, tint);
                    break;
                }
                case Kind::runners:
                {
                    // the runners fan from the plenum (or each from its own trumpet) to the head, numbered
                    const int n = std::max(1, b.cylinders);
                    const float head = r.right() - 18.0f;
                    const float span = std::min(r.h + 20.0f, static_cast<float>(n) * 20.0f);
                    c.rrect(head, yc_ - span * 0.5f - 8.0f, 18.0f, span + 16.0f, 4.0f, p.raised, 1.0f, p.edge);
                    const double open = live(N::throttle);
                    for (int i = 0; i < n; ++i)
                    {
                        const float y = yc_ + (n == 1 ? 0.0f : (static_cast<float>(i) / static_cast<float>(n - 1) - 0.5f) * span);
                        if (b.itb)
                        {
                            const float x0 = r.x + 10.0f;
                            line(c, { { x0, y }, { head, y } }, 6.0f, tint);
                            // the trumpet's bell, and the runner's own throttle
                            c.arc({ x0 + 8.0f, y }, 8.0f, 2.2f, 4.08f, 1.5f, ink);
                            if (n <= 8)
                                drawThrottle(c, p, x0 + 40.0f, y, std::min(7.0f, span / static_cast<float>(n) * 0.45f), open, body, tint);
                            else
                                c.segment(x0 + 40.0f, y - 3.0f, x0 + 40.0f, y + 3.0f, 2.0f, ink);
                        }
                        else
                            line(c, sBend({ r.x - 6.0f, yc_ + (static_cast<float>(i) / static_cast<float>(std::max(1, n - 1)) - 0.5f) * 30.0f },
                                          { head, y }, 10), 5.0f, tint);
                    }
                    if (b.itb)
                        c.text(b.trumpetsToAir ? "TO THE AIR" : "IN THE BOX", r.x, yc_ + span * 0.5f + 12.0f, look::kCaps, p.label);
                    break;
                }
            }
            (void)edge;
        }

        // A butterfly in its bore: the plate across (shut) to along (open), and the opening in words beside it.
        static void drawThrottle(Canvas& c, const Palette& p, float x, float y, float r, double open, Col body, Col tint)
        {
            c.disc(x, y, r, body, 1.5f, tint);
            const double a = std::isfinite(open) ? (1.0 - std::clamp(open, 0.0, 1.0)) * 1.4 + 0.12 : 0.785398;
            const float dx = static_cast<float>(std::cos(a)) * (r - 3.0f), dy = static_cast<float>(std::sin(a)) * (r - 3.0f);
            c.segment(x - dx, y - dy, x + dx, y + dy, 2.5f, std::isfinite(open) ? p.value : p.dim);
        }

        // ---- the compressor's map ----------------------------------------------------------------------------------
        void drawMap(Canvas& c, const Palette& p) const
        {
            const AirBuild& b = build_;
            if (!b.hasMap())
            {
                look::region(c, p, mapR_, "COMPRESSOR MAP");
                look::placeholder(c, p, mapR_, b.charger == Charger::none ? "NO COMPRESSOR" : "A DISPLACEMENT BLOWER",
                                  b.charger == Charger::none ? "This engine breathes the air outside: no map to work on."
                                                             : "A Roots or twin-screw blower moves a volume a turn: its boost follows the engine's speed, not a map.");
                return;
            }
            const Rect in = look::region(c, p, mapR_, b.turbos > 1 ? "COMPRESSOR MAP, TURBO 1'S" : "COMPRESSOR MAP");
            const Rect plot{ in.x + 34.0f, in.y + 4.0f, in.w - 70.0f, in.h - 30.0f };
            const double fMax = lines_.maxFlow * 1.08, prMax = std::max(1.5, lines_.maxPr * 1.06);
            const auto X = [&](double f) { return plot.x + static_cast<float>(f / fMax) * plot.w; };
            const auto Y = [&](double pr) { return plot.bottom() - static_cast<float>((pr - 1.0) / (prMax - 1.0)) * plot.h; };
            c.pushClip({ mapR_.x, mapR_.y, mapR_.w, mapR_.h });
            // the grid and its numbers
            const double fs = niceStep(fMax);
            for (double f = 0; f <= fMax + 1e-9; f += fs)
            {
                c.hairlineV(X(f), plot.y, plot.h, p.grid);
                centred(c, look::num(f, fs < 0.1 ? "%.2f" : "%.1f"), X(f), plot.bottom() + 4.0f, look::kUnit, p.label);
            }
            const double ps = prMax > 3.0 ? 0.5 : 0.25;
            for (double r = 1.0; r <= prMax + 1e-9; r += ps)
            {
                c.hairlineH(plot.x, Y(r), plot.w, p.grid);
                const std::string t = look::num(r, "%.2f");
                c.text(t.c_str(), plot.x - 6.0f - c.textWidth(t.c_str(), look::kUnit), Y(r) - 6.0f, look::kUnit, p.label);
            }
            c.text("kg/s", plot.right() - c.textWidth("kg/s", look::kUnit), plot.bottom() + 16.0f, look::kUnit, p.label);
            c.text("RATIO", mapR_.right() - 12.0f - c.textWidth("RATIO", look::kCaps), mapR_.y + 9.0f, look::kCaps, p.dim);
            // efficiency islands, innermost last
            for (int v = MapLines::kLevels - 1; v >= 0; --v)
            {
                const auto vi = static_cast<std::size_t>(v);
                // runs of crossings that exist, each drawn alone (an island cut by the map's edge is not closed across)
                std::vector<std::vector<Point>> runs(1);
                std::vector<Point> pts;
                bool gap = false;
                for (int k = 0; k < 2 * MapLines::kIslandSteps; ++k)
                {
                    const float f = lines_.islandFlow[vi][static_cast<std::size_t>(k)];
                    if (f < 0.0f)
                    {
                        gap = gap || !runs.back().empty();
                        if (!runs.back().empty())
                            runs.emplace_back();
                        continue;
                    }
                    const Point q{ X(f), Y(lines_.islandPr[vi][static_cast<std::size_t>(k)]) };
                    runs.back().push_back(q);
                    pts.push_back(q);
                }
                if (pts.size() < 2)
                    continue;
                if (!gap && runs.size() == 1)
                    runs.back().push_back(runs.back().front());
                else if (runs.size() > 1 && !runs.front().empty() && !runs.back().empty())
                {
                    // the first run and the last meet round the low speeds: join them
                    runs.back().insert(runs.back().end(), runs.front().begin(), runs.front().end());
                    runs.front().clear();
                }
                const Col ink = Palette::mix(p.panel, p.ok, 0.30f + 0.12f * static_cast<float>(MapLines::kLevels - 1 - v));
                for (const std::vector<Point>& run : runs)
                    line(c, run, 1.25f, ink);
                if (v == 0)
                {
                    float top = 1e9f, tx = 0;
                    for (const Point& q : pts)
                        if (q.y < top)
                        {
                            top = q.y;
                            tx = q.x;
                        }
                    centred(c, look::num(lines_.islandEta[vi] * 100.0, "%.0f %%"), tx, top - 14.0f, look::kUnit, p.ok);
                }
            }
            // the speed lines, numbered in thousands of rpm at their choke ends
            for (int l = 0; l < MapLines::kLines; ++l)
            {
                const auto li = static_cast<std::size_t>(l);
                std::vector<Point> pts;
                for (int i = 0; i < MapLines::kPoints; ++i)
                    pts.push_back({ X(lines_.flow[li][static_cast<std::size_t>(i)]), Y(lines_.pr[li][static_cast<std::size_t>(i)]) });
                line(c, pts, 1.25f, p.dim);
                if (l % 2 == 1 || l == MapLines::kLines - 1)
                    c.text(look::num(lines_.speed[li] * 60.0 / kTwoPi / 1000.0, "%.0fk").c_str(), pts.back().x + 4.0f,
                           pts.back().y - 6.0f, look::kUnit, p.label);
            }
            // the surge line (the knees) and the choke line (the lines' ends)
            {
                std::vector<Point> surge, choke;
                for (int l = 0; l < MapLines::kLines; ++l)
                {
                    const auto li = static_cast<std::size_t>(l);
                    surge.push_back({ X(lines_.flow[li][0]), Y(lines_.pr[li][0]) });
                    choke.push_back({ X(lines_.flow[li][MapLines::kPoints - 1]), Y(lines_.pr[li][MapLines::kPoints - 1]) });
                }
                line(c, surge, 2.0f, p.warning);
                line(c, choke, 1.25f, p.label);
                c.text("SURGE", surge.back().x - 46.0f, surge.back().y - 4.0f, look::kCaps, p.warning);
                c.text("CHOKE", choke[2].x + 6.0f, choke[2].y + 4.0f, look::kCaps, p.label);
            }
            // the trail, fading with age, and the operating points
            for (std::size_t i = 1; i < trail_.size(); ++i)
            {
                const float age = static_cast<float>(i) / static_cast<float>(trail_.size());
                const Col ink = Palette::mix(p.panel, p.selected, 0.12f + 0.75f * age * age);
                c.segment(X(trail_[i - 1].x), Y(trail_[i - 1].y), X(trail_[i].x), Y(trail_[i].y), 1.5f, ink);
            }
            const double f1 = live(N::flow[0]), r1 = live(N::ratio[0]);
            if (std::isfinite(f1) && std::isfinite(r1))
                c.disc(X(f1), Y(r1), 5.5f, p.selected, 1.5f, p.ground);
            const double f2 = live(N::flow[1]), r2 = live(N::ratio[1]);
            if (b.turbos > 1 && std::isfinite(f2) && std::isfinite(r2))
            {
                c.disc(X(f2), Y(r2), 5.0f, p.panel, 2.0f, p.value);
                c.text("2", X(f2) + 8.0f, Y(r2) - 6.0f, look::kNumberS, p.value);
                c.text("1", X(f1) + 8.0f, Y(r1) - 6.0f, look::kNumberS, p.selected);
            }
            if (!std::isfinite(f1))
                centred(c, "no operating point yet", plot.centreX(), plot.y + 8.0f, look::kBody, p.dim);
            c.popClip();
        }

        // ---- the shaft (a turbo), the drive (a supercharger), the manifold (a natural engine) -----------------------
        void drawShaft(Canvas& c, const Palette& p) const
        {
            const AirBuild& b = build_;
            if (b.charger == Charger::none)
            {
                const Rect in = look::region(c, p, shaftR_, "MANIFOLD");
                const double m = live(N::manifold);
                look::readout(c, p, in.x, in.y, "PRESSURE", pres(m));
                const double vac = std::isfinite(m) ? b.ambientP - m : std::nan("");
                look::readout(c, p, in.x, in.y + 52.0f, "VACUUM", pres(vac));
                const double th = live(N::throttle);
                c.text("THROTTLE", in.x, in.y + 104.0f, look::kCaps, p.label);
                look::level(c, p, { in.x, in.y + 122.0f, in.w, 6.0f }, th, p.cold);
                return;
            }
            if (b.charger == Charger::supercharger && b.blower != Blower::centrifugal)
            {
                const Rect in = look::region(c, p, shaftR_, "DRIVE", ctx_.selection.is("boost.supercharger"));
                look::readout(c, p, in.x, in.y, "PULLEY RATIO", look::num(b.scRatio, "%.2f"), ": 1");
                const double bp = live(N::scBypass);
                const bool known = std::isfinite(bp) || !b.scBypass;
                look::lamp(c, p, in.x, in.y + 62.0f, !b.scBypass ? "NO BYPASS" : !std::isfinite(bp) ? "BYPASS -" : bp > 0.5 ? "BYPASS OPEN" : "BYPASS SHUT",
                           known && std::isfinite(bp) && bp > 0.5, p.cold);
                const double pw = live(N::scPower);
                look::readout(c, p, in.x, in.y + 84.0f, "POWER IT TAKES", std::isfinite(pw) ? ctx_.units.format(Quantity::power, pw) : "-");
                return;
            }
            const Rect in = look::region(c, p, shaftR_, b.charger == Charger::turbo ? "SHAFT" : "IMPELLER");
            const double w = live(N::speed[0]);
            const auto krpm = [](double rad) { return look::num(rad * 60.0 / kTwoPi / 1000.0, "%.0f"); };
            look::readout(c, p, in.x, in.y, b.turbos > 1 ? "SPEED, TURBO 1" : "SPEED", std::isfinite(w) ? krpm(w) : "-", "krpm", look::kNumberL);
            const double frac = std::isfinite(w) && b.omegaMax > 0 ? w / b.omegaMax : std::nan("");
            look::level(c, p, { in.x, in.y + 50.0f, in.w, 6.0f }, frac, frac > 0.95 ? p.warning : p.text, 1.0);
            c.text(("LIMIT " + krpm(b.omegaMax) + " krpm").c_str(), in.x, in.y + 62.0f, look::kCaps, p.label);
            if (b.turbos > 1)
            {
                const double w2 = live(N::speed[1]);
                look::readout(c, p, in.x, in.y + 84.0f, "TURBO 2", std::isfinite(w2) ? krpm(w2) : "-", "krpm", look::kNumberS);
            }
            const float ty = in.y + (b.turbos > 1 ? 128.0f : 88.0f);
            const float tw = b.vanes ? in.w - 2.0f * std::clamp(in.h * 0.2f, 22.0f, 36.0f) - 64.0f : in.w;
            trace(c, p, { in.x, b.vanes ? ty + 28.0f : ty, tw, in.bottom() - (b.vanes ? ty + 28.0f : ty) }, true, b.omegaMax, b.omegaMax,
                  p.text, "SPEED, THE LIMIT DOTTED");
            if (b.vanes)
                drawVanes(c, p, in);
        }

        // A variable nozzle: the ring of vanes, more nearly radial (open) or tangential (shut), with the opening in words.
        void drawVanes(Canvas& c, const Palette& p, const Rect& in) const
        {
            const double open = live(N::vanes);
            const float r = std::clamp(in.h * 0.2f, 22.0f, 36.0f), cx = in.right() - r - 6.0f;
            const float cy = std::min(in.y + (build_.turbos > 1 ? 156.0f : 116.0f) + r, in.bottom() - r - 26.0f);
            c.disc(cx, cy, r + 4.0f, kClear, 1.0f, p.edge);
            c.disc(cx, cy, r * 0.42f, p.raised, 1.0f, p.edge);
            const double tilt = std::isfinite(open) ? 1.25 - 0.95 * std::clamp(open, 0.0, 1.0) : 0.8;
            for (int k = 0; k < 11; ++k)
            {
                const double a = kTwoPi * k / 11.0;
                const float px = cx + static_cast<float>(std::cos(a)) * r * 0.76f, py = cy + static_cast<float>(std::sin(a)) * r * 0.76f;
                const double va = a + kTwoPi / 4.0 - tilt;
                const float dx = static_cast<float>(std::cos(va)) * r * 0.22f, dy = static_cast<float>(std::sin(va)) * r * 0.22f;
                c.segment(px - dx, py - dy, px + dx, py + dy, 2.0f, std::isfinite(open) ? p.value : p.dim);
            }
            const std::string t = std::isfinite(open) ? look::num(open * 100.0, "VANES %.0f %% OPEN") : "VANES -";
            const float tw = c.textWidth(t.c_str(), look::kCaps);
            c.text(t.c_str(), std::min(cx - tw * 0.5f, in.right() - tw), cy + r + 10.0f, look::kCaps, std::isfinite(open) ? p.text : p.dim);
        }

        // ---- the gate, the blow-off valve, boost against target, anti-lag and nitrous -------------------------------
        void drawValves(Canvas& c, const Palette& p) const
        {
            const AirBuild& b = build_;
            const Rect in = look::region(c, p, valveR_, b.charger == Charger::none ? "EXTRAS" : "GATE AND BLOW-OFF");
            if (b.charger == Charger::turbo)
                poppet(c, p, gateR_, b.externalGate ? "EXT. GATE" : "WASTEGATE", live(N::gate), "OPEN");
            if (b.blowOff != 0)
                poppet(c, p, bovR_, b.blowOff == 2 ? "RECIRC. BOV" : "BLOW-OFF", live(N::blowOff), "LIFT");
            if (b.charger != Charger::none)
            {
                // boost against its target: two bars on one scale, each with its number
                const double boost = live(N::boost);
                double target = live(N::target);
                if (!std::isfinite(target))
                    target = b.target > 0 ? b.target : b.gateSpring;
                const double scale = std::max({ 100e3, target * 1.4, std::isfinite(boost) ? boost * 1.1 : 0.0 });
                // beside the valves where there is room, else under them
                float x = bovR_.right() + 18.0f, w = valveR_.right() - 12.0f - x, y = gateR_.y + 4.0f, below = gateR_.bottom();
                const bool narrow = w < 90.0f;
                if (narrow)
                {
                    x = in.x;
                    w = in.w;
                    y = gateR_.bottom() + 8.0f;
                }
                look::readout(c, p, x, y, "BOOST", pres(boost), {}, look::kNumberS);
                look::level(c, p, { x, y + 34.0f, w, 6.0f }, std::isfinite(boost) ? boost / scale : std::nan(""), p.hot, target / scale);
                if (narrow)
                {
                    look::readout(c, p, x + w * 0.5f, y, "TARGET", pres(target), {}, look::kNumberS);
                    below = y + 44.0f;
                }
                else
                    look::readout(c, p, x, y + 48.0f, "TARGET", pres(target), {}, look::kNumberS);
                const float ty = below + 24.0f;
                trace(c, p, { in.x, ty, in.w, valveR_.bottom() - 52.0f - ty }, false, scale, target, p.hot,
                      "BOOST, THE TARGET DOTTED");
            }
            // the extras, in words with a lamp
            const float ly = valveR_.bottom() - 34.0f;
            if (b.antiLag)
            {
                const double on = live(N::antiLag);
                look::lamp(c, p, valveR_.x + 12.0f, ly, std::isfinite(on) && on > 0.5 ? "ANTI-LAG ACTIVE" : "ANTI-LAG ARMED",
                           std::isfinite(on) && on > 0.5, p.warning);
            }
            if (b.nitrous)
            {
                const double on = live(N::nitrous), left = live(N::bottle);
                const float x = valveR_.x + 142.0f;
                const float w = look::lamp(c, p, x, ly, std::isfinite(on) && on > 0.5 ? "NITROUS ON" : "NITROUS ARMED",
                                           std::isfinite(on) && on > 0.5, p.cold);
                char t[48];
                std::snprintf(t, sizeof t, "%s OF %s", std::isfinite(left) ? ctx_.units.format(Quantity::mass, left).c_str() : "-",
                              ctx_.units.format(Quantity::mass, b.nitrousBottle).c_str());
                c.text(t, x + 15.0f, ly + 9.0f, look::kUnit, p.label);
                (void)w;
            }
            if (!b.antiLag && !b.nitrous && b.charger == Charger::none)
                c.text("no forced induction, anti-lag or nitrous", in.x, in.y + 4.0f, look::kBody, p.dim);
        }

        // A recent history as a small trace in r: values (x or y of history_) against their scale, with a mark line.
        void trace(Canvas& c, const Palette& p, const Rect& r, bool speed, double scale, double mark, Col ink,
                   const char* title) const
        {
            if (r.h < 40.0f || r.w < 60.0f || !(scale > 0))
                return;
            c.text(title, r.x, r.y, look::kCaps, p.label);
            const Rect a{ r.x, r.y + 16.0f, r.w, r.h - 16.0f };
            c.hairlineH(a.x, a.bottom(), a.w, p.grid);
            c.hairlineH(a.x, a.y, a.w, p.grid);
            if (std::isfinite(mark) && mark > 0)
                c.dotted(a.x, a.bottom() - a.h * static_cast<float>(std::min(mark / scale, 1.0)), a.w, 4.0f, p.text);
            std::vector<float> xs, ys;
            const std::size_t n = history_.size();
            for (std::size_t i = 0; i < n; ++i)
            {
                const double v = speed ? history_[i].y : history_[i].x;
                if (!std::isfinite(v))
                    continue;
                xs.push_back(a.x + a.w * static_cast<float>(i + kHistory - n) / static_cast<float>(kHistory - 1));
                ys.push_back(a.bottom() - a.h * static_cast<float>(std::clamp(v / scale, 0.0, 1.0)));
            }
            if (xs.size() >= 2)
                c.polyline(xs.data(), ys.data(), static_cast<int>(xs.size()), 1.5f, ink);
            else
                c.text("-", a.x + 4.0f, a.y + a.h * 0.5f - 6.0f, look::kNumberS, p.dim);
        }

        // A poppet valve in its seat, its head lifted by `open` (0..1), its name above, its opening below.
        static void poppet(Canvas& c, const Palette& p, const Rect& r, const char* name, double open, const char* word)
        {
            const float cx = r.centreX(), seat = r.y + 54.0f;
            const bool known = std::isfinite(open);
            const float lift = known ? static_cast<float>(std::clamp(open, 0.0, 1.0)) * 14.0f : 0.0f;
            centred(c, name, cx, r.y, look::kCaps, p.label);
            c.segment(cx - 26.0f, seat, cx - 11.0f, seat, 3.0f, p.text);
            c.segment(cx + 11.0f, seat, cx + 26.0f, seat, 3.0f, p.text);
            const Col ink = known ? p.value : p.dim;
            c.segment(cx, seat - lift - 4.0f, cx, seat - lift - 20.0f, 2.0f, ink);
            c.rrect(cx - 12.0f, seat - lift - 4.0f, 24.0f, 5.0f, 2.0f, ink);
            const std::string t = known ? std::string(word) + look::num(open * 100.0, " %.0f %%") : std::string(word) + " -";
            centred(c, t, cx, seat + 10.0f, look::kCaps, known ? p.text : p.dim);
        }

        // ---- the hovered and the selected ----------------------------------------------------------------------------
        void drawMarks(Canvas& c, const Palette& p) const
        {
            const std::string& sel = ctx_.selection.path();
            const std::string& hov = ctx_.selection.hovered();
            for (const Hit& h : hits_)
                if (!hov.empty() && h.id == hov && h.id != sel)
                    c.rrect(h.r.x - 2.0f, h.r.y - 2.0f, h.r.w + 4.0f, h.r.h + 4.0f, 6.0f, kClear, 1.0f, p.dim);
            for (const Hit& h : hits_)
                if (!sel.empty() && h.id == sel)
                {
                    look::selectRing(c, p, h.r, 5.0f);
                    const float w = c.textWidth(h.name.c_str(), look::kCaps) + 12.0f;
                    const float nx = std::clamp(h.r.x - 3.0f, stage_.x + 4.0f, stage_.right() - w - 4.0f);
                    const float ny = h.r.y - 24.0f < stage_.y + 50.0f ? h.r.bottom() + 6.0f : h.r.y - 24.0f;
                    c.rrect(nx, ny, w, 17.0f, 4.0f, p.selected);
                    c.text(h.name.c_str(), nx + 6.0f, ny + 3.5f, look::kCaps, p.ground);
                    break;
                }
        }

        AirBuild build_;
        MapLines lines_;
        std::vector<Part> parts_;
        std::vector<Hit> hits_;
        Rect row_{}, mapR_{}, shaftR_{}, valveR_{}, gateR_{}, bovR_{};
        float yc_ = 0, readY_ = 0;
        std::uint64_t seenDoc_ = 0;
        std::vector<Point> trail_;
        std::vector<Point> history_;   // x: boost (Pa over the air outside), y: shaft speed (rad/s)
    };

    std::unique_ptr<Workspace> makeAirWorkspace(WorkspaceContext& c) { return std::make_unique<AirWorkspace>(c); }
}
