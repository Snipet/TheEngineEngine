// tests/ui/views/EngineTests.cpp: the ENGINE workspace (ui/views/engine/; docs/ui/engine.md). Its layouts against
// sim/core/layouts.fm's comments, its derived numbers, presses that select, what it draws when a name is absent, the
// SLOW switch's time, and its pictures (the software rasteriser's) at both densities, two sizes and both themes.
#include "ViewKit.h"

#include "views/engine/EngineModel.h"
#include "views/engine/EngineWorkspace.h"

#include <funkgui/core/Theme.h>

namespace
{
    using namespace tee::test::views;
    namespace en = tee::ui::engine;

    // A press at the middle of the workspace's i-th widget (0 LIVE, 1 SLOW, 2 the pace, 3 LINEAR, 4 LOG-LOG).
    void pressWidget(StageRig& rig, int i)
    {
        const funkgui::Point c = rig.ws->item(i).centre();
        rig.host->click(c.x, c.y);
    }

    // The seventeen's firing orders and intervals are layouts.fm's; a crossplane V8's two cylinders a throw share it.
    VIEW_TEST(testEngineLayouts, "engine.layouts")
    {
        struct Want
        {
            const char* name;
            const char* order;
            const char* intervals;
        };
        const Want want[] = {
            { "Inline 4", "1-3-4-2", "even, 180" },
            { "Crossplane I4", "1-3-2-4", "270-180-90-180" },
            { "V-twin 45", "1-2", "315-405" },
            { "Twin 270", "1-2", "270-450" },
            { "Inline 5", "1-2-4-5-3", "even, 144" },
            { "Inline 6", "1-5-3-6-2-4", "even, 120" },
            { "V8 crossplane", "1-8-4-3-6-5-7-2", "even, 90" },
            { "V8 flat-plane", "1-5-3-7-4-8-2-6", "even, 90" },
            { "V10", "1-6-5-10-2-7-3-8-4-9", "even, 72" },
            { "V12", "1-7-5-11-3-9-6-12-2-8-4-10", "even, 60" },
            { "Rotary", "1-4-2-5-3-6", "even, 180" },
        };
        for (const Want& w : want)
        {
            const int k = en::presetByName(w.name);
            CHECK_MSG(k >= 0, w.name);
            const en::Layout l = en::presetLayout(k);
            CHECK_MSG(l.orderText() == w.order, std::string(w.name) + ": " + l.orderText());
            CHECK_MSG(l.intervalText() == w.intervals, std::string(w.name) + ": " + l.intervalText());
            CHECK(l.ok);
        }
        CHECK(en::presetByName("V8") == en::presetByName("V8 crossplane"));
        CHECK(en::presetByName("I4") == en::presetByName("Inline 4"));
        CHECK(en::presetByName("Straight 9") == -1);
        // each pair of a crossplane V8's cylinders on one throw has one pin; the four throws are 90 degrees apart
        const en::Layout v8 = en::presetLayout(en::presetByName("V8 crossplane"));
        CHECK(v8.banks == 2 && std::fabs(v8.bankAngle[0] - v8.bankAngle[1]) == 90.0);
        for (int t = 0; t < 4; ++t)
            CHECK_NEAR(v8.pin[static_cast<std::size_t>(2 * t)], v8.pin[static_cast<std::size_t>(2 * t + 1)], 1e-9);
        // a firing angle is the bank's angle less the pin (layout.fm), plus 360 on the second revolution
        for (int c = 0; c < v8.n; ++c)
        {
            const auto i = static_cast<std::size_t>(c);
            const double tdc = en::wrap(v8.bankAngle[static_cast<std::size_t>(v8.bank[i])] - v8.pin[i], 360.0);
            CHECK_NEAR(en::wrap(tdc - v8.bankAngle[static_cast<std::size_t>(v8.bank[0])] - v8.fire[i], 360.0), 0.0, 1e-9);
        }
        // a custom layout from geometry: a 90 degree V-twin on one pin fires 270-450; on one revolution, together
        en::Layout vt;
        vt.n = 2;
        vt.banks = 2;
        vt.bankAngle = { 0.0, 90.0, 0.0, 0.0 };
        vt.bank[1] = 1;
        std::array<int, en::kMaxCyl> rev{};
        rev[1] = 1;
        en::deriveFiring(vt, rev);
        CHECK(vt.ok && vt.intervalText() == "450-270");
        rev[1] = 0;
        vt.pin[1] = 90.0;
        en::deriveFiring(vt, rev);
        CHECK(!vt.ok);
        return 0;
    }

    // The slider-crank and the derived numbers, from the demo's V8 (bore 101.6 mm, stroke 88.4 mm, rod 155.4 mm).
    VIEW_TEST(testEngineDerived, "engine.derived")
    {
        tee::ui::DemoRig rig;
        const en::Layout l = en::layoutOf(rig.doc);
        CHECK(l.n == 8 && l.name == "V8");
        const en::Cylinder c = en::cylinderOf(rig.doc, l);
        CHECK_NEAR(c.bore, 0.1016, 1e-9);
        CHECK_NEAR(c.rod, 0.1554, 1e-9);
        CHECK_NEAR(en::pistonDrop(0.0, c.radius(), c.rod), 0.0, 1e-12);
        CHECK_NEAR(en::pistonDrop(180.0, c.radius(), c.rod), c.stroke, 1e-12);
        // at 90 degrees the piston is past half its stroke (the rod's angle): r + L - sqrt(L^2 - r^2)
        const double r = c.radius();
        CHECK_NEAR(en::pistonDrop(90.0, r, c.rod), r + c.rod - std::sqrt(c.rod * c.rod - r * r), 1e-12);
        const en::Derived d = en::derive(l, c, 6600.0 * en::kPi / 30.0);
        CHECK_NEAR(d.displacement, 8.0 * en::kPi / 4.0 * 0.1016 * 0.1016 * 0.0884, 1e-9);
        CHECK_NEAR(d.displacement * 1000.0, 5.733, 0.001);
        CHECK_NEAR(d.boreStroke, 0.1016 / 0.0884, 1e-9);
        CHECK_NEAR(d.rodRatio, 0.1554 / 0.0884, 1e-9);
        CHECK_NEAR(d.pistonSpeed, 2.0 * 0.0884 * 6600.0 / 60.0, 1e-9);   // 19.4 m/s
        // what is absent is NaN, never a number made up
        const en::Derived none = en::derive(en::Layout{}, en::Cylinder{}, en::kNaN);
        CHECK(std::isnan(none.displacement) && std::isnan(none.pistonSpeed) && std::isnan(none.compression));
        return 0;
    }

    // Presses select: a cylinder, the block (the layout), the ring, the section (the cylinder's geometry); hovering a
    // cylinder lights it without selecting.
    VIEW_TEST(testEngineSelect, "engine.select")
    {
        StageRig rig(tee::ui::makeEngineWorkspace, 1100, 700, 1.0f);
        const auto& sel = rig.demo.selection;
        // find cylinder 3 by scanning the block for the press that selects it
        bool found = false;
        for (float y = 120.0f; y < 380.0f && !found; y += 4.0f)
            for (float x = 80.0f; x < 440.0f && !found; x += 4.0f)
            {
                rig.host->click(x, y);
                found = sel.path() == "engine.cylinder[2]";
            }
        CHECK(found);
        CHECK(sel.is("engine"));
        rig.settle();
        golden("engine.selected", *rig.host);   // cylinder 3 ringed, the section titled CYLINDER 3
        rig.host->click(30.0f, 70.0f);           // the block's corner, no cylinder there
        CHECK(sel.path() == "engine.layout");
        rig.host->click(1000.0f, 150.0f);        // the firing ring
        CHECK(sel.path() == "engine.layout");
        rig.host->click(590.0f, 400.0f);         // the section
        CHECK(sel.path() == "engine.bore");
        rig.host->move(5.0f, 5.0f);
        CHECK(rig.demo.selection.hovered().empty());
        return 0;
    }

    // A stage at rest asks for nothing; SLOW moves the angle by the stage's own time only, and LIVE follows the crank.
    VIEW_TEST(testEngineSlow, "engine.slow")
    {
        StageRig rig(tee::ui::makeEngineWorkspace, 1100, 700, 1.0f);
        tee::ui::Workspace& ws = *rig.ws;
        CHECK(!ws.moving());
        const std::uint64_t k0 = ws.drawKey();
        rig.settle();
        CHECK(ws.drawKey() == k0);                // nothing moved, nothing to draw
        rig.demo.engine.set("engine.crank", 200.0);
        CHECK(ws.drawKey() != k0);                // the crank moved
        pressWidget(rig, 1);                      // SLOW
        rig.host->tick(1);
        CHECK(ws.moving());
        const std::uint64_t k1 = ws.drawKey();
        rig.host->tick(30);                       // half a second: an eighth of a cycle at 4 s a cycle
        CHECK(ws.drawKey() != k1);
        golden("engine.slow", *rig.host);
        pressWidget(rig, 0);                      // LIVE
        rig.settle();
        CHECK(!ws.moving());
        return 0;
    }

    // What is not there draws a dash or an empty plot with its label: no crank, no Scope, no layout.
    VIEW_TEST(testEngineAbsent, "engine.absent")
    {
        StageRig rig(tee::ui::makeEngineWorkspace, 1100, 700, 1.0f);
        rig.demo.engine.forget("engine.crank");
        rig.demo.engine.forget("engine.redline");
        rig.demo.ctx.live.reset();   // Live keeps a handle once found: ask again
        rig.settle();
        CHECK(std::isnan(rig.demo.ctx.live.get("engine.crank")));
        CHECK(!rig.ws->moving());
        picture("engine.absent", *rig.host);
        return 0;
    }

    // The pictures: 1100 x 700 and 1600 x 1000, density 1 and 2, graphite; paper at 1100 x 700.
    VIEW_TEST(testEngineStage, "engine.stage")
    {
        {
            StageRig rig(tee::ui::makeEngineWorkspace, 1100, 700, 2.0f);
            golden("engine.stage", *rig.host);
            funkgui::HeadlessHost paper(*rig.ws, 1, 2.0f);
            paper.resize(1100, 700);
            paper.tick(60);
            golden("engine.stage.paper", paper);
        }
        {
            StageRig rig(tee::ui::makeEngineWorkspace, 1100, 700, 1.0f);
            golden("engine.stage.dpi1", *rig.host);
        }
        {
            StageRig rig(tee::ui::makeEngineWorkspace, 1600, 1000, 1.0f, 2.3);
            golden("engine.large.dpi1", *rig.host);
        }
        {
            StageRig rig(tee::ui::makeEngineWorkspace, 1600, 1000, 2.0f, 2.3);
            golden("engine.large", *rig.host);
        }
        return 0;
    }
}

#include "kit/bind/Binding.h"

namespace
{
    // Each selection's inspector page opens as a page of fields with nothing the reader could not use, names only
    // fields the build has, and keeps its order; nothing selected leaves the frame's generated pages.
    VIEW_TEST(testEngineInspector, "engine.inspector")
    {
        StageRig rig(tee::ui::makeEngineWorkspace, 1100, 700, 1.0f);
        auto& sel = rig.demo.selection;
        CHECK(rig.ws->inspectorPage().empty());
        const auto open = [&](const char* path, const char* picture) {
            sel.select(path);
            const std::string gui = rig.ws->inspectorPage();
            CHECK_MSG(!gui.empty(), path);
            tee::ui::GuiPanel::Options o;
            o.width = 320;
            tee::ui::FieldPage page(rig.demo.doc, rig.demo.units, rig.demo.engine, { { "page.gui", gui } }, o);
            CHECK_MSG(page.notes().empty(), page.notes());
            funkgui::HeadlessHost host(page, 0, 1.0f);
            host.tick(2);
            tee::test::views::picture(picture, host);
            return tee::ui::fieldsOfGui(rig.demo.doc, gui);
        };
        const std::vector<int> geo = open("engine.bore", "engine.inspector.cylinder");
        // the demo's: bore, stroke, rod, compression, flywheel, friction; the hook's piston and rod masses
        std::vector<std::string> names;
        for (int f : geo)
            names.push_back(rig.demo.doc.info(f).path);
        // (in the document's order: the demo's own, then the masses ENGINE's hook adds)
        CHECK(names.size() == 8 && names[0] == "engine.bore" && names[1] == "engine.stroke" && names[2] == "engine.rod"
              && names[3] == "engine.compression" && names[4] == "engine.flywheel" && names[5] == "engine.friction"
              && names[6] == "engine.pistonMass" && names[7] == "engine.rodMass");
        const std::vector<int> lay = open("engine.layout", "engine.inspector.layout");
        CHECK(lay.size() == 3);   // the layout menu, the cycle (card T's, through the hook) and the demo's cylinders
        const std::vector<int> cyl = open("engine.cylinder[2]", "engine.inspector.cylinder3");
        CHECK(cyl.size() == 1 && rig.demo.doc.info(cyl[0]).path == "engine.layout");
        return 0;
    }
}

namespace tee::test::u2
{
    // What a frame's text does wrong, in words ("" : nothing): a glyph outside the stage, a glyph across the edge of
    // one of `panels` (inside one or outside all, never half), two glyphs of different words overlapping. The glyph
    // boxes are the text prims' quads less their antialiasing apron (U5's way: tests/ui/views/ScopeTests.cpp).
    std::string textFaults(const funkgui::PrimList& prims, const funkgui::Rect& stage, const std::vector<funkgui::Rect>& panels)
    {
        struct Box
        {
            float x0, y0, x1, y1;
        };
        std::vector<Box> glyphs;
        for (const funkgui::Prim& p : prims.prims)
            if (static_cast<int>(p.d2[2] + 0.5f) == static_cast<int>(funkgui::PrimKind::text))
                glyphs.push_back({ p.x0 + 1.5f, p.y0 + 1.5f, p.x1 - 1.5f, p.y1 - 1.5f });
        const auto inside = [](const Box& b, const funkgui::Rect& r) {
            return b.x0 >= r.x - 0.5f && b.y0 >= r.y - 0.5f && b.x1 <= r.right() + 0.5f && b.y1 <= r.bottom() + 0.5f;
        };
        const auto meets = [](const Box& b, const funkgui::Rect& r) {
            return b.x1 > r.x && b.x0 < r.right() && b.y1 > r.y && b.y0 < r.bottom();
        };
        char buf[160];
        for (const Box& b : glyphs)
        {
            if (!inside(b, stage))
            {
                std::snprintf(buf, sizeof buf, "a glyph at %.0f,%.0f leaves the stage", b.x0, b.y0);
                return buf;
            }
            for (const funkgui::Rect& r : panels)
                if (!r.isEmpty() && meets(b, r) && !inside(b, r))
                {
                    std::snprintf(buf, sizeof buf, "a glyph at %.0f,%.0f crosses the edge of the panel at %.0f,%.0f", b.x0,
                                  b.y0, r.x, r.y);
                    return buf;
                }
        }
        if (glyphs.size() < 20)
            return "too few glyphs to judge: " + std::to_string(glyphs.size());
        for (std::size_t i = 0; i < glyphs.size(); ++i)
            for (std::size_t j = i + 1; j < glyphs.size(); ++j)
            {
                const Box& a = glyphs[i];
                const Box& b = glyphs[j];
                const float ox = std::min(a.x1, b.x1) - std::max(a.x0, b.x0), oy = std::min(a.y1, b.y1) - std::max(a.y0, b.y0);
                if (ox > 1.0f && oy > 1.0f)
                {
                    std::snprintf(buf, sizeof buf, "glyphs overlap at %.0f,%.0f and %.0f,%.0f", a.x0, a.y0, b.x0, b.y0);
                    return buf;
                }
            }
        return {};
    }
}

namespace
{
    // At the frame's own stages (its smallest window's and a 1600 x 1000 window's): the small one folds the ring, the
    // loop and the numbers into one panel of tabs; no text leaves the stage, crosses a panel's edge or overlaps other
    // text, on any tab, with the crank or without, at the SLOW switch, and with a cylinder selected. Goldens at both
    // densities.
    VIEW_TEST(testEngineFrameStage, "engine.framestage")
    {
        for (const FrameSize size : { FrameSize::smallest, FrameSize::large })
        {
            const std::string tag = size == FrameSize::smallest ? "small" : "large";
            for (const float dpi : { 2.0f, 1.0f })
            {
                StageRig rig(tee::ui::makeEngineWorkspace, size, dpi);
                CHECK(tee::ui::engineCompact(*rig.ws) == (size == FrameSize::smallest));
                const funkgui::Rect st = rig.ws->stage();
                const auto faults = [&](const std::string& when) {
                    const std::string why = tee::test::u2::textFaults(rig.host->draw(), st, tee::ui::enginePanels(*rig.ws));
                    CHECK_MSG(why.empty(), tag + ", " + when + ": " + why);
                };
                faults("at rest");
                golden("engine.frame." + tag + (dpi == 1.0f ? ".dpi1" : ""), *rig.host);
                if (dpi == 1.0f)
                {
                    if (size == FrameSize::smallest)
                        for (int t = 5; t <= 7; ++t)   // the tabs: FIRING, P-V LOOP, DERIVED
                        {
                            pressWidget(rig, t);
                            rig.settle();
                            faults("tab " + std::to_string(t - 5));
                            picture("engine.frame.small.tab" + std::to_string(t - 5), *rig.host);
                        }
                    pressWidget(rig, 1);   // SLOW
                    rig.host->tick(20);
                    faults("slow");
                    rig.demo.selection.select("engine.cylinder[4]");
                    rig.settle();
                    faults("cylinder 5");
                    rig.demo.engine.forget("engine.crank");
                    rig.demo.ctx.live.reset();
                    pressWidget(rig, 0);   // LIVE, with no crank
                    rig.settle();
                    faults("no crank");
                }
            }
        }
        return 0;
    }
}

namespace
{
    // The section's valves open where the simulator says (valve.ivo, valve.evo) when telemetry has it: moving the
    // intake's event alone moves what is drawn; with the event where the Scope's own lift opens, nothing moves.
    VIEW_TEST(testEngineEvents, "engine.events")
    {
        StageRig rig(tee::ui::makeEngineWorkspace, 1100, 700, 1.0f, 4.6);
        rig.demo.selection.select("engine.bore");
        const auto frameHash = [&] {
            std::uint64_t h = 1469598103934665603ull;
            for (const funkgui::Prim& p : rig.host->draw().prims)
                h = (h ^ static_cast<std::uint64_t>(std::lround(p.y0 * 4.0f) + 7919 * std::lround(p.x0 * 4.0f))) * 1099511628211ull;
            return h;
        };
        // the demo's Scope: the intake leaves its seat at 346, the exhaust at 131 (ui/app/Demo.cpp: opening at 345, 130); cylinder 1 at 400 degrees,
        // the intake well open
        rig.demo.engine.set("engine.crank", 400.0);
        rig.demo.engine.set("valve.ivo", 346.0);
        rig.demo.engine.set("valve.evo", 131.0);
        rig.demo.ctx.live.reset();
        rig.settle();
        const std::uint64_t asScope = frameHash();
        rig.demo.engine.forget("valve.ivo");
        rig.demo.engine.forget("valve.evo");
        rig.demo.ctx.live.reset();
        rig.settle();
        CHECK(frameHash() == asScope);   // absent: the Scope's own timing, the same picture
        rig.demo.engine.set("valve.ivo", 385.0);   // the intake opening 40 degrees later
        rig.demo.ctx.live.reset();
        rig.settle();
        CHECK(frameHash() != asScope);
        picture("engine.events", *rig.host);
        return 0;
    }
}

#include "views/engine/TwoStroke.h"

namespace tee::test::u2
{
    // A two-stroke's Scope for tests (the demo's is a four-stroke's 720 degrees): 360 degrees, 0 the firing top; the
    // cylinder's pressure (compressed from the exhaust's closing, burnt over 45 degrees, expanded to the exhaust's
    // opening, then near the crankcase's), its volume, the ports' shares from the geometry, the crankcase's pressure
    // (pulled below the air outside as the piston rises, pushed above as it falls, evened out while the transfers are
    // open). Made-up but plausible, from the document's fields.
    class TwoStrokeScope final : public tee::ui::ScopeSource
    {
    public:
        explicit TwoStrokeScope(const tee::ui::FieldDoc& doc) : doc_(doc) {}
        int find(std::string_view name) const override
        {
            const char* names[] = { "degrees", "pressure", "volume", "liftExhaust", "liftIntake", "portIntake", "inletOpen" };
            for (int i = 0; i < (inlet_ ? 7 : 6); ++i)
                if (name == names[i])
                    return i;
            return -1;
        }
        int read(int h, float* out, int max) override
        {
            namespace en = tee::ui::engine;
            const tee::ui::engine::Layout l = en::layoutOf(doc_);
            const en::Cylinder c = en::cylinderOf(doc_, l);
            const en::TwoStroke t = en::twoStrokeOf(doc_);
            if (h == 0)
            {
                out[0] = 360.0f;
                return 1;
            }
            const double r = c.radius(), L = c.rod, cr = std::isfinite(c.compression) ? c.compression : 12.0;
            const double area = en::kPi / 4.0 * c.bore * c.bore, clear = c.swept() / (cr - 1.0);
            const auto vol = [&](double a) { return clear + area * en::pistonDrop(a, r, L); };
            const double vTrap = vol(t.exCloses()), pTrap = 105e3;
            const int n = std::min(max, 360);
            for (int i = 0; i < n; ++i)
            {
                const double a = i;
                double v = 0.0;
                switch (h)
                {
                    case 1:
                    {
                        const double vc = vol(a);
                        const double comp = pTrap * std::pow(vTrap / vc, 1.3);
                        const double peak = pTrap * std::pow(vTrap / vol(0.0), 1.3) * 3.2;
                        if (a >= t.exOpens() && a < t.exCloses())
                            v = 101325.0 + 60e3 * std::exp(-(a - t.exOpens()) / 12.0);
                        else if (a < t.exOpens())
                            v = std::max(peak * std::pow(vol(15.0) / std::max(vc, vol(15.0)), 1.3) * std::min(1.0, 0.45 + a / 30.0), comp);
                        else
                            v = comp;
                        break;
                    }
                    case 2: v = vol(a); break;
                    case 3: v = en::portShare(a, t.exDur, r, L); break;
                    case 4: v = en::portShare(a, t.trDur, r, L); break;
                    case 5:
                    {
                        const double ratio = std::isfinite(t.crankcaseRatio) ? t.crankcaseRatio : 1.4;
                        v = 101325.0 * (1.0 + (ratio - 1.0) * 0.6 * (en::pistonDrop(a, r, L) / (2.0 * r) - 0.5) * 2.0);
                        if (a >= t.trOpens() && a < t.trCloses())
                            v = 101325.0 + (v - 101325.0) * 0.4;
                        break;
                    }
                    case 6:   // a reed: open while the rising piston pulls the crankcase below the air outside
                        v = std::clamp(std::sin((a - 200.0) / 160.0 * en::kPi), 0.0, 1.0) * 0.8;
                        break;
                    default: return 0;
                }
                out[i] = static_cast<float>(v);
            }
            return n;
        }
        std::uint64_t generation() const override { return 1; }
        bool inlet_ = false;   // with card T's inletOpen

    private:
        const tee::ui::FieldDoc& doc_;
    };

    std::unique_ptr<tee::ui::ScopeSource> twoStrokeScope(const tee::ui::FieldDoc& doc, bool inlet)
    {
        auto sc = std::make_unique<TwoStrokeScope>(doc);
        sc->inlet_ = inlet;
        return sc;
    }

    // The demo as a two-stroke: a 125 cc racing single's fields (two-stroke-mx125.json), or `cylinders` evenly.
    void makeTwoStroke(tee::ui::MemoryDoc& doc, int cylinders = 1)
    {
        const auto set = [&](const char* path, double v) {
            const int f = doc.find(path);
            if (f >= 0)
                doc.set(f, v);
        };
        set("engine.cycle", 1.0);
        set("engine.tsCylinders", cylinders);
        set("engine.bore", 0.054);
        set("engine.stroke", 0.0545);
        set("engine.rod", 0.11);
        set("engine.compression", 13.0);
        set("engine.tsValveDur", 178.0);
        set("engine.tsValveFrom", 837.758);
        set("engine.tsValveTo", 994.838);
        set("engine.tsExhaustWidth", 0.85);
        set("engine.tsTransferWidth", 1.4);
    }
}

namespace
{
    // A two-stroke host for the ENGINE or VALVES stage: the demo with card T's fields set, a two-stroke's Scope.
    struct TwoStrokeRig
    {
        tee::ui::DemoRig demo;
        tee::test::u2::TwoStrokeScope scope{ demo.doc };
        tee::ui::WorkspaceContext ctx{ demo.doc, demo.engine, scope, demo.engine, demo.units, demo.selection, demo.app };
        std::unique_ptr<tee::ui::Workspace> ws;
        std::unique_ptr<funkgui::HeadlessHost> host;
        TwoStrokeRig(tee::ui::WorkspaceFactory make, int cylinders, int w, int h, float dpi, double crank)
        {
            tee::test::u2::makeTwoStroke(demo.doc, cylinders);
            demo.engine.set("engine.crank", crank);
            demo.engine.set("engine.speed", 9000.0 * tee::ui::engine::kPi / 30.0);
            ws = make(ctx);
            host = std::make_unique<funkgui::HeadlessHost>(*ws, 0, dpi);
            host->resize(w, h);
            ws->place({ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) });
            host->tick(60);
        }
    };

    // A two-stroke (card T): a 360-degree cycle, its cylinders evenly or together; its ports open about bottom dead
    // centre, open as the geometry says; its phases named; and its section, ring and loop at the frame's stages with no
    // text out of place. The inlet's three kinds pictured.
    VIEW_TEST(testEngineTwoStroke, "engine.twostroke")
    {
        namespace en = tee::ui::engine;
        {
            tee::ui::DemoRig d;
            tee::test::u2::makeTwoStroke(d.doc, 2);
            const en::Layout l = en::layoutOf(d.doc);
            CHECK(l.twoStroke && l.cyc == 360.0 && l.n == 2);
            CHECK_MSG(l.intervalText() == "even, 180", l.intervalText());
            d.doc.set(d.doc.find("engine.tsFiring"), 1.0);
            CHECK_MSG(en::layoutOf(d.doc).intervalText() == "together, 360", en::layoutOf(d.doc).intervalText());
            const en::TwoStroke t = en::twoStrokeOf(d.doc);
            const en::Cylinder c = en::cylinderOf(d.doc, en::layoutOf(d.doc));
            CHECK_NEAR(t.exOpens(), 84.0, 1e-9);
            CHECK_NEAR(en::portShare(t.exOpens(), t.exDur, c.radius(), c.rod), 0.0, 1e-9);
            CHECK_NEAR(en::portShare(180.0, t.exDur, c.radius(), c.rod), 1.0, 1e-9);
            CHECK(std::string(en::twoStrokePhase(30.0, t)) == "POWER" && std::string(en::twoStrokePhase(100.0, t)) == "BLOWDOWN"
                  && std::string(en::twoStrokePhase(180.0, t)) == "SCAVENGE" && std::string(en::twoStrokePhase(260.0, t)) == "TRAPPING"
                  && std::string(en::twoStrokePhase(300.0, t)) == "COMPRESSION");
            // the power valve: shut below its speed, open above, between in between
            CHECK_NEAR(t.exDurAt(500.0), 178.0, 1e-9);
            CHECK_NEAR(t.exDurAt(2000.0), 192.0, 1e-9);
        }
        const funkgui::Rect sizes[] = { { 64, 48, 696, 574 }, { 112, 48, 1128, 874 } };
        for (const funkgui::Rect& st : sizes)
        {
            const std::string tag = st.w < 700.0f ? "small" : "large";
            for (const float dpi : { 2.0f, 1.0f })
            {
                TwoStrokeRig rig(tee::ui::makeEngineWorkspace, 2, static_cast<int>(st.w), static_cast<int>(st.h), dpi, 150.0);
                const std::string why = tee::test::u2::textFaults(rig.host->draw(), { 0, 0, st.w, st.h }, tee::ui::enginePanels(*rig.ws));
                CHECK_MSG(why.empty(), tag + ": " + why);
                golden("engine.twostroke." + tag + (dpi == 1.0f ? ".dpi1" : ""), *rig.host);
            }
        }
        // the inlet's kinds, at the small stage, at 300 degrees (the piston rising, the crankcase drawing in)
        const char* kinds[] = { "reed", "disc", "pistonport" };
        for (int k = 0; k < 3; ++k)
        {
            TwoStrokeRig rig(tee::ui::makeEngineWorkspace, 1, 696, 574, 1.0f, 300.0);
            rig.demo.doc.set(rig.demo.doc.find("engine.tsInletKind"), k);
            rig.host->tick(5);
            const std::string why = tee::test::u2::textFaults(rig.host->draw(), { 0, 0, 696, 574 }, tee::ui::enginePanels(*rig.ws));
            CHECK_MSG(why.empty(), std::string(kinds[k]) + ": " + why);
            picture(std::string("engine.twostroke.") + kinds[k], *rig.host);
        }
        return 0;
    }
}

namespace
{
    // Geometry no engine has: amber words in the cylinder's page beside the fields concerned, appearing with the
    // oddity and going with it (the engine survives it; the interface warns). Four rules: a piston too heavy for its
    // bore, stroke / bore outside 0.5 to 2, a mean piston speed over 30 m/s at the limiter, rod / stroke under 1.4.
    VIEW_TEST(testEngineWarnings, "engine.warnings")
    {
        StageRig rig(tee::ui::makeEngineWorkspace, 1100, 700, 1.0f);
        auto& doc = rig.demo.doc;
        const auto set = [&](const char* path, double v) { doc.set(doc.find(path), v); };
        rig.demo.selection.select("engine.bore");
        const auto page = [&] { return rig.ws->inspectorPage(); };
        const char* words[] = { "A PISTON THIS HEAVY", "THIS FAR OVER- OR", "MEAN PISTON SPEED OVER 30", "A ROD THIS SHORT" };
        const auto has = [&](int i) { return page().find(words[i]) != std::string::npos; };
        for (int i = 0; i < 4; ++i)
            CHECK_MSG(!has(i), words[i]);   // the demo's V8: none
        set("engine.pistonMass", 18.4);       // the diverging build's piston (X-007)
        CHECK(has(0));
        set("engine.bore", 0.12);
        set("engine.stroke", 0.05);           // stroke / bore 0.42
        CHECK(has(1));
        set("engine.bore", 0.07);
        set("engine.stroke", 0.12);           // a long stroke at 6600 rpm: 26 m/s, at 9000: 36
        set("ecu.limiter", 9000.0 * en::kPi / 30.0);
        CHECK(has(2));
        set("engine.rod", 0.15);              // 1.25 of the stroke
        CHECK(has(3));
        CHECK(page().find("color=yellow") != std::string::npos);
        // the page still reads clean, and pictures
        {
            tee::ui::GuiPanel::Options o;
            o.width = 320;
            tee::ui::FieldPage fp(doc, rig.demo.units, rig.demo.engine, { { "page.gui", page() } }, o);
            CHECK_MSG(fp.notes().empty(), fp.notes());
            funkgui::HeadlessHost host(fp, 0, 1.0f);
            host.tick(2);
            picture("engine.inspector.warnings", host);
        }
        doc.reset();                          // the defaults: every warning gone
        for (int i = 0; i < 4; ++i)
            CHECK_MSG(!has(i), words[i]);
        return 0;
    }
}

namespace
{
    // A custom layout the simulator refused (D's tel.engine.layoutRefused, finding X-001): while the flag reads 1 an
    // amber line in words on the stage and in the layout's page, and the block draws the layout dial's (what runs),
    // not the custom one asked for; both go when the flag does. The demo hook gives the flag as 0.
    VIEW_TEST(testEngineRefused, "engine.refused")
    {
        tee::ui::DemoRig base;
        // the demo's fields and D's custom-layout switch, as a two-cylinder custom whose cylinders fire together
        std::vector<tee::ui::FieldInfo> fields;
        for (int i = 0; i < base.doc.count(); ++i)
            fields.push_back(base.doc.info(i));
        tee::ui::FieldInfo custom;
        custom.path = "engine.custom";
        custom.kind = tee::ui::FieldKind::integer;
        custom.max = 1.0;
        custom.label = "Custom layout";
        fields.push_back(custom);
        tee::ui::MemoryDoc doc(fields);
        doc.set(doc.find("engine.custom"), 1.0);
        doc.set(doc.find("engine.cylinders"), 2.0);
        tee::ui::WorkspaceContext ctx{ doc, base.engine, base.engine, base.engine, base.units, base.selection, base.app };
        auto ws = tee::ui::makeEngineWorkspace(ctx);
        funkgui::HeadlessHost host(*ws, 0, 1.0f);
        host.resize(696, 574);
        ws->place({ 0.0f, 0.0f, 696.0f, 574.0f });
        host.tick(5);
        base.selection.select("engine.layout");
        const auto refusedText = [&] { return ws->inspectorPage().find("WAS REFUSED") != std::string::npos; };
        CHECK(std::isfinite(base.ctx.live.get(en::kTelLayoutRefused)));   // the hook gives it
        CHECK(!refusedText());
        CHECK_MSG(tee::ui::engineRunningLayout(*ws) == "Custom", tee::ui::engineRunningLayout(*ws));
        const std::uint64_t calm = ws->drawKey();
        base.engine.set(en::kTelLayoutRefused, 1.0);
        ctx.live.reset();
        host.tick(1);
        CHECK(refusedText());
        CHECK(ws->inspectorPage().find("FIRE TOGETHER") != std::string::npos);   // the reason, from the build
        CHECK(ws->drawKey() != calm);
        const std::string dial = en::dialLayoutOf(doc).name;
        CHECK_MSG(!dial.empty() && tee::ui::engineRunningLayout(*ws) == dial, tee::ui::engineRunningLayout(*ws));
        const std::string why = tee::test::u2::textFaults(host.draw(), { 0, 0, 696, 574 }, tee::ui::enginePanels(*ws));
        CHECK_MSG(why.empty(), why);
        picture("engine.refused", host);
        {
            tee::ui::GuiPanel::Options o;
            o.width = 320;
            tee::ui::FieldPage fp(doc, base.units, base.engine, { { "page.gui", ws->inspectorPage() } }, o);
            CHECK_MSG(fp.notes().empty(), fp.notes());
            funkgui::HeadlessHost ph(fp, 0, 1.0f);
            ph.tick(2);
            picture("engine.inspector.refused", ph);
        }
        base.engine.set(en::kTelLayoutRefused, 0.0);
        ctx.live.reset();
        host.tick(1);
        CHECK(!refusedText());
        CHECK(tee::ui::engineRunningLayout(*ws) == "Custom");
        return 0;
    }
}
