// tests/ui/views/ValvesTests.cpp: the VALVES workspace (ui/views/valves/; docs/ui/valves.md). The cam generator against
// cam.fm's promises, the events a cam card quotes, a handle's drag as one step of undo, what is absent, and the
// pictures (the software rasteriser's) at both densities, two sizes and both themes.
//
// The demo's document lacks package E's lab fields; LabRig adds them (ui/views/valves/ValvesDemo.cpp) until U1's demo
// hook carries them.
#include "ViewKit.h"

#include "views/engine/EngineModel.h"
#include "views/valves/ValvesDemo.h"
#include "views/valves/ValvesModel.h"
#include "views/valves/ValvesWorkspace.h"

#include <funkgui/core/Theme.h>

namespace
{
    using namespace tee::test::views;
    namespace vv = tee::ui::valves;

    std::vector<tee::ui::FieldInfo> labFields(bool withLab = true, bool withDemoValves = true)
    {
        std::vector<tee::ui::FieldInfo> f;
        for (tee::ui::FieldInfo& i : tee::ui::demoFields(false))
            if (withDemoValves || i.section() != "valve")
                f.push_back(std::move(i));
        if (withLab)
            for (tee::ui::FieldInfo& i : tee::ui::valvesDemoFields())
                f.push_back(std::move(i));
        return f;
    }

    // The demo with the lab's fields: a whole context, a workspace and a host (as StageRig, over another document).
    struct LabRig
    {
        tee::ui::MemoryDoc doc;
        tee::ui::Units units;
        tee::ui::DemoEngine engine;
        tee::ui::Selection selection;
        tee::AppServices app{ [](const tee::PanelControls&) {}, [] { return tee::ControlsFrame{}; },
                                  [] { return std::string("valves lab"); } };
        tee::ui::WorkspaceContext ctx{ doc, engine, engine, engine, units, selection, app };
        std::unique_ptr<tee::ui::Workspace> ws;
        std::unique_ptr<funkgui::HeadlessHost> host;

        explicit LabRig(int w = 1100, int h = 700, float dpi = 2.0f, double t = 4.6,
                        std::vector<tee::ui::FieldInfo> fields = labFields(), bool hooks = true)
            : doc(std::move(fields)), engine(&doc, hooks)
        {
            engine.setTime(t);
            ws = tee::ui::makeValvesWorkspace(ctx);
            host = std::make_unique<funkgui::HeadlessHost>(*ws, 0, dpi);
            host->resize(w, h);
            ws->place({ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) });
            host->tick(90);
        }
        // E's telemetry, as it will be named (ValvesModel.h): a phaser part way, stage B, the float speed, two resting
        void live()
        {
            engine.set("valve.inPhase", 18.0);
            engine.set("valve.stage", 1.0);
            engine.set("valve.floatSpeed", 7200.0 * vv::kPi / 30.0);
            engine.set("valve.active", static_cast<double>(0xFF & ~0x06));
            ctx.live.reset();   // Live keeps a handle once found (the hook's): ask again
            host->tick(30);
        }
    };

    // The generator keeps cam.fm's promises: the lift at the peak, 0.15 mm at the seat events, the 1 mm duration, no
    // negative lift; a cam card's events from the demo's 268/274 degree cam on 108/116.
    VIEW_TEST(testValvesCam, "valves.cam")
    {
        tee::ui::MemoryDoc doc(labFields());
        const vv::Train t = vv::trainOf(doc);
        CHECK(t.editable && !t.reference && !t.labOff && t.stages);
        for (const vv::Lobe* l : { &t.in, &t.ex })
        {
            CHECK_NEAR(l->liftAt(l->centre), l->lift, l->lift * 1e-6);
            CHECK_NEAR(l->liftAt(l->seatOpen()), 0.0001524, 2e-5);
            CHECK_NEAR(l->liftAt(l->seatClose()), 0.0001524, 2e-5);
            // the 1 mm duration, measured
            double open = 0.0;
            for (double a = 0.0; a < 720.0; a += 0.05)
                open += l->liftAt(a) >= 0.001 ? 0.05 : 0.0;
            CHECK_NEAR(open, l->dur1mm, 1.0);
            for (double a = 0.0; a < 720.0; a += 1.0)
                CHECK(l->liftAt(a) >= 0.0);
        }
        const vv::Events e = vv::eventsOf(t.in, t.ex);
        CHECK_NEAR(e.ivo, 26.0, 1e-9);
        CHECK_NEAR(e.ivc, 62.0, 1e-9);
        CHECK_NEAR(e.evo, 73.0, 1e-9);
        CHECK_NEAR(e.evc, 21.0, 1e-9);
        CHECK_NEAR(e.overlap, 47.0, 1e-9);
        CHECK_NEAR(e.lsa, 112.0, 1e-9);
        // the reference's dial alone: 224 degrees on 112 when mild, 300 on 106 when wild
        std::vector<tee::ui::FieldInfo> ref;
        tee::ui::FieldInfo cam;
        cam.path = "valve.cam";
        cam.def = 1.0;
        ref.push_back(cam);
        tee::ui::MemoryDoc rd(ref);
        const vv::Train r = vv::trainOf(rd);
        CHECK(r.reference && !r.editable);
        CHECK_NEAR(r.in.durSeat, 300.0, 1e-9);
        CHECK_NEAR(vv::eventsOf(r.in, r.ex).lsa, 106.0, 1e-9);
        // the bench: a small-block's head (an intake of 0.48 of a 4 in bore) at 0.500 in flows what E measured
        // (docs/models/valvetrain.md): 172 CFM at q 1, 232 at 1.35
        CHECK_NEAR(vv::benchFlow(1, 0.48 * 0.1016, 1.0, 0.0127) * 2118.88, 172.0, 0.5);
        CHECK_NEAR(vv::benchFlow(1, 0.48 * 0.1016, 1.35, 0.0127) * 2118.88, 232.0, 0.5);
        return 0;
    }

    // A drag of the intake's peak is one step of undo through the document, selects the intake, and moves the
    // centreline the way it was dragged; a seat mark changes the duration and keeps the ramps.
    VIEW_TEST(testValvesDrag, "valves.drag")
    {
        LabRig rig(1100, 700, 1.0f);
        const vv::Train t0 = vv::trainOf(rig.doc);
        const int fc = rig.doc.find("valve.inCentre"), fd = rig.doc.find("valve.inDurSeat"), f1 = rig.doc.find("valve.inDur1mm");
        // the intake's peak handle, pressed where the stage says it is: the press selects the intake, starts a gesture
        funkgui::Point at = tee::ui::valvesHandle(*rig.ws, 0);
        CHECK(at.x >= 0.0f);
        rig.host->press(at.x, at.y);
        CHECK(rig.doc.depth() > 0);
        CHECK(rig.selection.path() == "valve.intake");
        rig.host->moveTo(at.x + 30.0f, at.y, 4);
        rig.host->release();
        CHECK(rig.doc.steps() == 1 && rig.doc.depth() == 0);
        CHECK(rig.doc.get(fc) > t0.in.centre - 360.0 + 15.0);   // dragged later: a later centreline
        rig.doc.undo();
        CHECK_NEAR(rig.doc.get(fc), 108.0, 1e-9);
        // the intake's closing seat mark, dragged out: a longer duration, the 1 mm duration the same distance under it
        rig.host->tick(5);
        at = tee::ui::valvesHandle(*rig.ws, 2);
        CHECK(at.x >= 0.0f);
        rig.host->press(at.x, at.y);
        CHECK(rig.doc.depth() > 0);
        rig.host->moveTo(at.x + 20.0f, at.y, 4);
        rig.host->release();
        CHECK(rig.doc.get(fd) > 268.0 + 10.0);
        CHECK_NEAR(rig.doc.get(fd) - rig.doc.get(f1), 268.0 - 244.0, 1e-6);
        return 0;
    }

    // What is absent: no lab fields (the demo's own three per cam draw, without a 1 mm duration or a head); no valve
    // fields at all; no telemetry of E's.
    VIEW_TEST(testValvesAbsent, "valves.absent")
    {
        {
            StageRig rig(tee::ui::makeValvesWorkspace, 1100, 700, 1.0f);
            golden("valves.demo", *rig.host);   // the demo with every hook (E's fields from ValvesDemo.cpp), at stage A
        }
        LabRig rig(1100, 700, 1.0f, 4.6, labFields(false, false));
        CHECK(!vv::trainOf(rig.doc).editable);
        CHECK(!rig.ws->moving());
        picture("valves.empty", *rig.host);
        return 0;
    }

    // A stage at rest asks for nothing; a change of E's telemetry is something to draw.
    VIEW_TEST(testValvesRest, "valves.rest")
    {
        LabRig rig(1100, 700, 1.0f);
        CHECK(!rig.ws->moving());
        const std::uint64_t k = rig.ws->drawKey();
        rig.host->tick(10);
        CHECK(rig.ws->drawKey() == k);
        rig.engine.set("valve.stage", 1.0);
        rig.ctx.live.reset();
        CHECK(rig.ws->drawKey() != k);
        return 0;
    }

    // The pictures: 1100 x 700 and 1600 x 1000, density 1 and 2, graphite; paper at 1100 x 700; E's telemetry live.
    VIEW_TEST(testValvesStage, "valves.stage")
    {
        {
            LabRig rig(1100, 700, 2.0f, 4.6);
            rig.live();
            golden("valves.stage", *rig.host);
            funkgui::HeadlessHost paper(*rig.ws, 1, 2.0f);
            paper.resize(1100, 700);
            paper.tick(60);
            golden("valves.stage.paper", paper);
        }
        {
            LabRig rig(1100, 700, 1.0f, 4.6);
            rig.live();
            golden("valves.stage.dpi1", *rig.host);
        }
        {
            LabRig rig(1600, 1000, 1.0f, 2.3);
            rig.live();
            golden("valves.large.dpi1", *rig.host);
        }
        {
            LabRig rig(1600, 1000, 2.0f, 2.3);
            rig.live();
            golden("valves.large", *rig.host);
        }
        return 0;
    }
}

#include "kit/bind/Binding.h"

namespace
{
    // Each selection's page opens with no notes; a cam's page leads with the stage that runs.
    VIEW_TEST(testValvesInspector, "valves.inspector")
    {
        LabRig rig(1100, 700, 1.0f);
        CHECK(rig.ws->inspectorPage().empty());
        const auto open = [&](const char* path, const char* picture) {
            rig.selection.select(path);
            const std::string gui = rig.ws->inspectorPage();
            tee::ui::GuiPanel::Options o;
            o.width = 320;
            tee::ui::FieldPage page(rig.doc, rig.units, rig.engine, { { "page.gui", gui } }, o);
            CHECK_MSG(page.notes().empty(), page.notes());
            if (picture != nullptr)
            {
                funkgui::HeadlessHost host(page, 0, 1.0f);
                host.tick(2);
                tee::test::views::picture(picture, host);
            }
            return tee::ui::fieldsOfGui(rig.doc, gui);
        };
        std::vector<int> in = open("valve.intake", "valves.inspector.intake");
        CHECK(!in.empty() && rig.doc.info(in.front()).path == "valve.inKind");
        CHECK(rig.ws->inspectorPage().find("valve.inDurSeat ") < rig.ws->inspectorPage().find("valve.inDurSeatB"));
        CHECK(rig.ws->inspectorPage().find("valve.intakeDuration") == std::string::npos);   // E's path, not the demo's too
        rig.engine.set("valve.stage", 1.0);
        rig.ctx.live.reset();
        in = open("valve.intake", nullptr);
        CHECK(rig.ws->inspectorPage().find("valve.inDurSeatB") < rig.ws->inspectorPage().find("valve.inDurSeat "));
        for (const char* p : { "valve.exhaust", "valve.timing", "valve.head", "valve.phasers", "valve.springs" })
            CHECK_MSG(!open(p, nullptr).empty(), p);
        open("valve.springs", "valves.inspector.springs");
        return 0;
    }
}

#include "views/engine/EngineWorkspace.h"

#include <funkgui/text/FontService.h>

namespace
{
    // The contact sheet of package U2 (out/ui/sheet.u2.png): ENGINE then VALVES a row each; across, each at 1100 x 700
    // and 1600 x 1000, at density 1 then 2. Every tile is the software rasteriser's, shrunk to half its logical size.
    VIEW_TEST(testU2Sheet, "u2.sheet")
    {
        struct Size
        {
            int w, h;
            float dpi;
        };
        const Size cols[] = { { 1100, 700, 1.0f }, { 1600, 1000, 1.0f }, { 1100, 700, 2.0f }, { 1600, 1000, 2.0f } };
        constexpr int kGap = 24;
        int sheetW = kGap;
        for (const Size& c : cols)
            sheetW += c.w / 2 + kGap;
        const int rowH = 1000 / 2 + kGap, sheetH = kGap + 2 * rowH;
        funkgui::Image out;
        out.w = sheetW;
        out.h = sheetH;
        out.rgba.assign(static_cast<std::size_t>(sheetW) * static_cast<std::size_t>(sheetH) * 4, 255);
        for (std::size_t i = 0; i < out.rgba.size(); i += 4)
            out.rgba[i] = out.rgba[i + 1] = out.rgba[i + 2] = 8;
        const auto tile = [&](funkgui::HeadlessHost& host, const Size& col, int ox, int oy) {
            const funkgui::Image img = funkgui::rasterise(host.draw(), funkgui::FontService::get().atlas(), 1);
            const int tw = col.w / 2, th = col.h / 2;
            const double sc = static_cast<double>(img.w) / tw;
            for (int y = 0; y < th; ++y)
                for (int x = 0; x < tw; ++x)
                {
                    const int x0 = static_cast<int>(x * sc), x1 = std::max(x0 + 1, static_cast<int>((x + 1) * sc));
                    const int y0 = static_cast<int>(y * sc), y1 = std::max(y0 + 1, static_cast<int>((y + 1) * sc));
                    unsigned sum[3] = { 0, 0, 0 }, n = 0;
                    for (int yy = y0; yy < y1 && yy < img.h; ++yy)
                        for (int xx = x0; xx < x1 && xx < img.w; ++xx)
                        {
                            const std::size_t k = (static_cast<std::size_t>(yy) * static_cast<std::size_t>(img.w) + static_cast<std::size_t>(xx)) * 4;
                            for (int ch = 0; ch < 3; ++ch)
                                sum[ch] += img.rgba[k + static_cast<std::size_t>(ch)];
                            ++n;
                        }
                    const std::size_t d = (static_cast<std::size_t>(oy + y) * static_cast<std::size_t>(sheetW) + static_cast<std::size_t>(ox + x)) * 4;
                    for (int ch = 0; ch < 3; ++ch)
                        out.rgba[d + static_cast<std::size_t>(ch)] = static_cast<uint8_t>(n > 0 ? sum[ch] / n : 0);
                }
        };
        int ox = kGap;
        for (const Size& col : cols)
        {
            {
                StageRig rig(tee::ui::makeEngineWorkspace, col.w, col.h, col.dpi, 4.6);
                tile(*rig.host, col, ox, kGap);
            }
            {
                LabRig rig(col.w, col.h, col.dpi, 4.6);
                rig.live();
                tile(*rig.host, col, ox, kGap + rowH);
            }
            ox += col.w / 2 + kGap;
        }
        const std::string path = (outDir() / "sheet.u2.png").string();
        CHECK_MSG(tee::test::writePng(out, path), path);
        std::printf("  contact sheet: %s (software rasteriser; rows ENGINE, VALVES; columns 1100 x 700 and 1600 x 1000 "
                    "at density 1, then at 2)\n", path.c_str());
        return 0;
    }
}

namespace tee::test::u2
{
    std::string textFaults(const funkgui::PrimList& prims, const funkgui::Rect& stage, const std::vector<funkgui::Rect>& panels);
}

namespace
{
    // At the frame's own stages (its smallest window's and a 1600 x 1000 window's), with E's fields and telemetry, and
    // with none of them (the lower panels folded to a line each, the editor taller): no text leaves the stage, crosses
    // a panel's edge or overlaps other text. Goldens at both densities.
    VIEW_TEST(testValvesFrameStage, "valves.framestage")
    {
        for (const FrameSize size : { FrameSize::smallest, FrameSize::large })
        {
            const std::string tag = size == FrameSize::smallest ? "small" : "large";
            for (const float dpi : { 2.0f, 1.0f })
            {
                StageRig rig(tee::ui::makeValvesWorkspace, size, dpi);
                const funkgui::Rect st = rig.ws->stage();
                const std::string why = tee::test::u2::textFaults(rig.host->draw(), st, tee::ui::valvesPanels(*rig.ws));
                CHECK_MSG(why.empty(), tag + ": " + why);
                golden("valves.frame." + tag + (dpi == 1.0f ? ".dpi1" : ""), *rig.host);
            }
            // a build with no lab and no valve telemetry: the panels fold, the editor grows
            const funkgui::Rect st = size == FrameSize::smallest ? funkgui::Rect{ 64, 48, 696, 574 } : funkgui::Rect{ 112, 48, 1128, 874 };
            LabRig bare(static_cast<int>(st.w), static_cast<int>(st.h), 1.0f, 4.6, labFields(false, true), false);
            const std::vector<funkgui::Rect> panels = tee::ui::valvesPanels(*bare.ws);
            CHECK(panels.size() == 5);
            if (size == FrameSize::large)
                CHECK(panels[3].h < 40.0f && panels[4].h < 40.0f);
            const std::string why = tee::test::u2::textFaults(bare.host->draw(), { 0, 0, st.w, st.h }, panels);
            CHECK_MSG(why.empty(), tag + ", bare: " + why);
            picture("valves.frame." + tag + ".bare", *bare.host);
        }
        return 0;
    }
}

namespace
{
    // E's integration: a free valve is drawn where it opens and closes, and dragged there (one undo step); the second
    // exhaust event is drawn when on; the ring takes the simulator's events when telemetry has them; the bench
    // takes the simulator's own when telemetry has every element.
    VIEW_TEST(testValvesLab, "valves.lab")
    {
        tee::ui::MemoryDoc doc(labFields());
        const auto set = [&](const char* path, double v) { doc.set(doc.find(path), v); };
        set("valve.inKind", 1.0);
        set("valve.auxOn", 1.0);
        const vv::Train t = vv::trainOf(doc);
        CHECK(t.in.free && !t.ex.free);
        CHECK_NEAR(t.in.seatOpen(), 350.0, 1e-9);
        CHECK_NEAR(t.in.seatClose(), 590.0, 1e-9);
        CHECK_NEAR(t.in.liftAt(470.0), 0.0124, 1e-9);   // dwelling at its lift between the flanks
        CHECK_NEAR(t.in.liftAt(350.0), 0.0, 1e-9);
        CHECK(t.aux.known());
        CHECK_NEAR(t.aux.liftAt(710.0), 0.002, 0.002 * 1e-6);   // the brake's lobe peaks mid-event (680 + 30)

        LabRig rig(1100, 700, 1.0f);
        rig.doc.set(rig.doc.find("valve.inKind"), 1.0);
        rig.doc.set(rig.doc.find("valve.auxOn"), 1.0);
        rig.host->tick(5);
        const int fo = rig.doc.find("valve.freeInOpen"), fc = rig.doc.find("valve.freeInClose");
        const int steps = rig.doc.steps();
        // the free valve's handle, pressed where the stage says it is, and dragged later
        const funkgui::Point at = tee::ui::valvesHandle(*rig.ws, 0);
        CHECK(at.x >= 0.0f);
        rig.host->press(at.x, at.y);
        CHECK(rig.doc.depth() > 0);
        CHECK(rig.selection.path() == "valve.intake");
        rig.host->moveTo(at.x + 20.0f, at.y, 4);
        rig.host->release();
        CHECK(rig.doc.steps() == steps + 1);
        CHECK(rig.doc.get(fo) > 350.0 + 5.0 && rig.doc.get(fc) > 590.0 + 5.0);
        CHECK_NEAR(rig.doc.get(fc) - rig.doc.get(fo), 240.0, 1e-6);   // moved, not stretched
        golden("valves.lab", *rig.host);   // a free intake, the brake's lobe, the demo's live events on the ring

        // the bench: the simulator's when every element is in telemetry
        tee::ui::DemoRig demo;
        for (int i = 0; i < 16; ++i)
        {
            const std::string n = std::to_string(i);
            demo.engine.set("valve.benchLift[" + n + "]", 0.00127 * (i + 1));
            demo.engine.set("valve.benchIntake[" + n + "]", 0.05 + 0.001 * i);
            demo.engine.set("valve.benchExhaust[" + n + "]", 0.04);
        }
        const vv::Bench b = vv::benchOf(demo.doc, 0.1016, demo.ctx.live);
        CHECK(b.simulated && b.known);
        CHECK_NEAR(b.flowIn[3], 0.053, 1e-12);
        demo.engine.forget("valve.benchExhaust[7]");
        demo.ctx.live.reset();
        CHECK(!vv::benchOf(demo.doc, 0.1016, demo.ctx.live).simulated);   // one missing: this port's
        return 0;
    }
}

namespace tee::test::u2
{
    class TwoStrokeScope;
    void makeTwoStroke(tee::ui::MemoryDoc& doc, int cylinders);
    std::unique_ptr<tee::ui::ScopeSource> twoStrokeScope(const tee::ui::FieldDoc& doc, bool inlet = false);
}

namespace
{
    // A two-stroke (card T's engine.cycle 1): VALVES becomes PORT TIMING. At the frame's stages, every tab of the small
    // one, no text out of place; the inspector's pages read clean; a port's mark dragged is one undo step that sets its
    // duration about bottom dead centre.
    VIEW_TEST(testValvesTwoStroke, "valves.twostroke")
    {
        const funkgui::Rect sizes[] = { { 64, 48, 696, 574 }, { 112, 48, 1128, 874 } };
        for (const funkgui::Rect& st : sizes)
        {
            const std::string tag = st.w < 700.0f ? "small" : "large";
            for (const float dpi : { 2.0f, 1.0f })
            {
                tee::ui::DemoRig demo;
                tee::test::u2::makeTwoStroke(demo.doc, 1);
                demo.engine.set("engine.crank", 150.0);
                demo.engine.set("engine.speed", 9000.0 * vv::kPi / 30.0);
                const auto scope = tee::test::u2::twoStrokeScope(demo.doc);
                tee::ui::WorkspaceContext ctx{ demo.doc, demo.engine, *scope, demo.engine, demo.units, demo.selection, demo.app };
                auto ws = tee::ui::makeValvesWorkspace(ctx);
                funkgui::HeadlessHost host(*ws, 0, dpi);
                host.resize(static_cast<int>(st.w), static_cast<int>(st.h));
                ws->place({ 0, 0, st.w, st.h });
                host.tick(60);
                const auto faults = [&](const std::string& when) {
                    const std::string why = tee::test::u2::textFaults(host.draw(), { 0, 0, st.w, st.h }, tee::ui::valvesPanels(*ws));
                    CHECK_MSG(why.empty(), tag + ", " + when + ": " + why);
                };
                faults("at rest");
                golden("valves.twostroke." + tag + (dpi == 1.0f ? ".dpi1" : ""), host);
                if (dpi != 1.0f)
                    continue;
                if (tag == "small")
                    for (int t = 0; t < 3; ++t)
                    {
                        const funkgui::Point c = ws->item(t).centre();
                        host.click(c.x, c.y);
                        host.tick(5);
                        faults("tab " + std::to_string(t));
                        picture("valves.twostroke.small.tab" + std::to_string(t), host);
                    }
                // the pages
                for (const char* path : { "", "valve.ports", "valve.powervalve", "valve.crankcase", "valve.scavenging" })
                {
                    demo.selection.select(path);
                    const std::string gui = ws->inspectorPage();
                    tee::ui::GuiPanel::Options o;
                    o.width = 320;
                    tee::ui::FieldPage page(demo.doc, demo.units, demo.engine, { { "page.gui", gui } }, o);
                    CHECK_MSG(page.notes().empty(), std::string(path) + ": " + page.notes());
                    CHECK_MSG(!tee::ui::fieldsOfGui(demo.doc, gui).empty(), path);
                }
                // the exhaust's opening mark, dragged later: a shorter duration, one step of undo
                if (tag == "large")
                {
                    const int f = demo.doc.find("engine.tsExhaustDur");
                    const funkgui::Point at = tee::ui::valvesHandle(*ws, 100);
                    CHECK(at.x > 0.0f);
                    const int steps = demo.doc.steps();
                    host.drag(at.x, at.y - 3.0f, at.x + 20.0f, at.y - 3.0f, 6);
                    CHECK(demo.doc.steps() == steps + 1 && demo.doc.get(f) < 192.0 - 5.0);
                    CHECK(demo.selection.path() == "valve.ports");
                    demo.doc.undo();
                    CHECK_NEAR(demo.doc.get(f), 192.0, 1e-9);
                }
            }
        }
        return 0;
    }
}

namespace
{
    // Card T's measured inlet (scope.inletOpen): drawn in place of the window and the reed's words, its span on the
    // ring; without it the stage is as before. No text out of place either way.
    VIEW_TEST(testValvesInletOpen, "valves.inlet")
    {
        const auto frame = [](bool inlet, std::string* why, const char* pictureName) {
            tee::ui::DemoRig demo;
            tee::test::u2::makeTwoStroke(demo.doc, 1);
            demo.engine.set("engine.crank", 300.0);
            const auto scope = tee::test::u2::twoStrokeScope(demo.doc, inlet);
            tee::ui::WorkspaceContext ctx{ demo.doc, demo.engine, *scope, demo.engine, demo.units, demo.selection, demo.app };
            auto ws = tee::ui::makeValvesWorkspace(ctx);
            funkgui::HeadlessHost host(*ws, 0, 1.0f);
            host.resize(1128, 874);
            ws->place({ 0, 0, 1128, 874 });
            host.tick(30);
            const funkgui::PrimList& prims = host.draw();
            *why = tee::test::u2::textFaults(prims, { 0, 0, 1128, 874 }, tee::ui::valvesPanels(*ws));
            std::uint64_t h = 1469598103934665603ull;
            for (const funkgui::Prim& p : prims.prims)
                h = (h ^ static_cast<std::uint64_t>(std::lround(p.y0 * 4.0f) + 7919 * std::lround(p.x0 * 4.0f))) * 1099511628211ull;
            if (pictureName != nullptr)
                picture(pictureName, host);
            return h;
        };
        std::string why;
        const std::uint64_t without = frame(false, &why, nullptr);
        CHECK_MSG(why.empty(), why);
        const std::uint64_t with = frame(true, &why, "valves.twostroke.inlet");
        CHECK_MSG(why.empty(), why);
        CHECK(with != without);
        return 0;
    }
}

namespace
{
    // What the simulator says holds the cams back (tel.valve.waiting: a phaser parked without oil pressure, slowed by
    // cold oil; the stage waiting for the coolant at tel.valve.waitTemp, or for oil pressure): said in amber words in the PHASERS AND STAGES panel and on the editor's phaser band, at the frame's
    // stages with no text out of place, and gone once the oil is warm.
    VIEW_TEST(testValvesOil, "valves.oil")
    {
        const funkgui::Rect sizes[] = { { 64, 48, 696, 574 }, { 112, 48, 1128, 874 } };
        for (const funkgui::Rect& st : sizes)
        {
            const std::string tag = st.w < 700.0f ? "small" : "large";
            LabRig rig(static_cast<int>(st.w), static_cast<int>(st.h), 1.0f);
            rig.live();
            rig.engine.set("valve.stage", 0.0);
            // just started cold: the intake phaser without oil pressure yet, the stage waiting for the coolant
            rig.engine.set(vv::kTelOilTemp, 288.0);
            rig.engine.set(vv::kTelCoolant, 278.0);
            rig.engine.set(vv::kTelWaitTemp, 283.0);
            rig.engine.set(vv::kTelWaiting, vv::kWaitInParked | vv::kWaitSlowed | vv::kWaitCoolant);
            rig.ctx.live.reset();
            const std::uint64_t k = rig.ws->drawKey();
            rig.host->tick(5);
            if (st.w < 700.0f)
            {
                // the small stage: the PHASERS tab (the workspace's second item: BENCH, PHASERS, SPRINGS)
                const funkgui::Point tab = rig.ws->item(1).centre();
                rig.host->click(tab.x, tab.y);
                rig.host->tick(5);
            }
            const std::string words = tee::ui::valvesOilWords(*rig.ws);
            CHECK_MSG(words.find("PARKED: NO OIL PRESSURE") != std::string::npos && words.find("B WAITS: COOLANT 5") != std::string::npos
                          && words.find("NEEDS 10") != std::string::npos, words);
            const std::string why = tee::test::u2::textFaults(rig.host->draw(), { 0, 0, st.w, st.h }, tee::ui::valvesPanels(*rig.ws));
            CHECK_MSG(why.empty(), tag + ": " + why);
            picture("valves.oil." + tag, *rig.host);
            // warm enough for the phasers, not for B; then warm
            // oil pressure up, still cold: the phaser moves, slowly; the stage still waits
            rig.engine.set(vv::kTelWaiting, vv::kWaitSlowed | vv::kWaitCoolant);
            CHECK(rig.ws->drawKey() != k);
            CHECK_MSG(tee::ui::valvesOilWords(*rig.ws).find("SLOWED: COLD OIL, 15") != std::string::npos, tee::ui::valvesOilWords(*rig.ws));
            CHECK(tee::ui::valvesOilWords(*rig.ws).find("B WAITS") != std::string::npos);
            rig.engine.set(vv::kTelWaiting, 0.0);
            CHECK(tee::ui::valvesOilWords(*rig.ws).empty());
        }
        return 0;
    }
}

namespace
{
    // The overlap's pressure (N1's tel.valve.dpOverlap): a stat over the cam editor beside the overlap, "+12 kPa
    // SCAVENGES" in the intake's ink, "-63 kPa BACK-FILLS" in amber; nothing with no reading (absent or 0). At the
    // frame's stages with no text out of place.
    VIEW_TEST(testValvesOverlapPressure, "valves.overlap")
    {
        const funkgui::Rect sizes[] = { { 64, 48, 696, 574 }, { 112, 48, 1128, 874 } };
        for (const funkgui::Rect& st : sizes)
        {
            const std::string tag = st.w < 700.0f ? "small" : "large";
            LabRig rig(static_cast<int>(st.w), static_cast<int>(st.h), 1.0f);
            rig.live();
            CHECK(tee::ui::valvesOverlapWords(*rig.ws).empty());   // the demo gives none
            const std::uint64_t k = rig.ws->drawKey();
            rig.engine.set(vv::kTelDpOverlap, -63000.0);           // the turbo four at 4040 rpm
            rig.ctx.live.reset();
            CHECK(rig.ws->drawKey() != k);
            rig.host->tick(5);
            CHECK_MSG(tee::ui::valvesOverlapWords(*rig.ws) == "-63.0 kPa BACK-FILLS", tee::ui::valvesOverlapWords(*rig.ws));
            std::string why = tee::test::u2::textFaults(rig.host->draw(), { 0, 0, st.w, st.h }, tee::ui::valvesPanels(*rig.ws));
            CHECK_MSG(why.empty(), tag + ": " + why);
            picture("valves.overlap." + tag, *rig.host);
            rig.engine.set(vv::kTelDpOverlap, 12000.0);            // an NA engine near its top
            CHECK_MSG(tee::ui::valvesOverlapWords(*rig.ws) == "+12.0 kPa SCAVENGES", tee::ui::valvesOverlapWords(*rig.ws));
            why = tee::test::u2::textFaults(rig.host->draw(), { 0, 0, st.w, st.h }, tee::ui::valvesPanels(*rig.ws));
            CHECK_MSG(why.empty(), tag + ": " + why);
            rig.engine.set(vv::kTelDpOverlap, 0.0);                // no reading
            CHECK(tee::ui::valvesOverlapWords(*rig.ws).empty());
        }
        return 0;
    }
}
