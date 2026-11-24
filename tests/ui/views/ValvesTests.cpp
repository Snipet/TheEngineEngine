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
        for (tee::ui::FieldInfo& i : tee::ui::demoFields())
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
        tee::ui::DemoEngine engine{ &doc };
        tee::ui::Selection selection;
        tee::AppServices app{ [](const tee::PanelControls&) {}, [] { return tee::ControlsFrame{}; },
                                  [] { return std::string("valves lab"); } };
        tee::ui::WorkspaceContext ctx{ doc, engine, engine, engine, units, selection, app };
        std::unique_ptr<tee::ui::Workspace> ws;
        std::unique_ptr<funkgui::HeadlessHost> host;

        explicit LabRig(int w = 1100, int h = 700, float dpi = 2.0f, double t = 4.6,
                        std::vector<tee::ui::FieldInfo> fields = labFields())
            : doc(std::move(fields))
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
        // find the intake's peak handle: the press that selects the intake and starts a gesture
        const int fc = rig.doc.find("valve.inCentre"), fd = rig.doc.find("valve.inDurSeat"), f1 = rig.doc.find("valve.inDur1mm");
        bool done = false;
        for (float y = 60.0f; y < 420.0f && !done; y += 2.0f)
            for (float x = 360.0f; x < 560.0f && !done; x += 2.0f)
            {
                rig.host->press(x, y);
                if (rig.doc.depth() > 0)
                {
                    CHECK(rig.selection.path() == "valve.intake");
                    rig.host->moveTo(x + 30.0f, y, 4);
                    rig.host->release();
                    done = true;
                }
                else
                    rig.host->release();
            }
        CHECK(done);
        CHECK(rig.doc.steps() == 1 && rig.doc.depth() == 0);
        CHECK(rig.doc.get(fc) > t0.in.centre - 360.0 + 15.0);   // dragged later: a later centreline
        rig.doc.undo();
        CHECK_NEAR(rig.doc.get(fc), 108.0, 1e-9);
        // the intake's closing seat mark, dragged out: a longer duration, the 1 mm duration the same distance under it
        rig.host->tick(5);
        done = false;
        for (float y = 360.0f; y < 460.0f && !done; y += 2.0f)
            for (float x = 520.0f; x < 640.0f && !done; x += 2.0f)
            {
                rig.host->press(x, y);
                if (rig.doc.depth() > 0)
                {
                    rig.host->moveTo(x + 20.0f, y, 4);
                    rig.host->release();
                    done = true;
                }
                else
                    rig.host->release();
            }
        CHECK(done);
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
            golden("valves.demo", *rig.host);   // the demo's cams, no head on the bench, dashes for phasers and float
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
        in = open("valve.intake", nullptr);
        CHECK(rig.ws->inspectorPage().find("valve.inDurSeatB") < rig.ws->inspectorPage().find("valve.inDurSeat "));
        for (const char* p : { "valve.exhaust", "valve.timing", "valve.head", "valve.phasers", "valve.springs" })
            CHECK_MSG(!open(p, nullptr).empty(), p);
        open("valve.springs", "valves.inspector.springs");
        return 0;
    }
}
