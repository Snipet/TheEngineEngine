// tests/ui/views/TuneTests.cpp: the TUNE workspace (ui/views/tune/) over the demo engine, with TuneDemo.h's spark map and
// live ECU values: its stage at both densities, both themes and two sizes; a block chosen, nudged, smoothed, filled
// between its corners and undone; the live cell lit and trailed; a map the build lacks and ECU values the program
// lacks drawn as absent.
#include "ViewKit.h"

#include "views/tune/Tune.h"
#include "views/tune/TuneDemo.h"

#include <funkgui/core/Theme.h>

namespace
{
    using namespace tee::test::views;
    using tee::ui::MapEdit;

    // The demo's script played from t0 for `frames` frames, the ECU's values set at each, so the traces fill.
    void play(StageRig& rig, double t0, int frames)
    {
        for (int i = 0; i < frames; ++i)
        {
            const double t = t0 + i / 60.0;
            rig.demo.at(t);
            tee::ui::tunedemo::setLive(rig.demo.engine, t);
            rig.host->tick(1);
        }
    }

    StageRig& prepared(StageRig& rig)
    {
        tee::ui::tunedemo::fillDocument(rig.demo.doc);
        rig.settle();
        play(rig, 0.0, 300);   // the first five seconds: idle, a stab, the pull into knock
        return rig;
    }

    funkgui::Point centreOf(const funkgui::Rect& r) { return { r.centreX(), r.centreY() }; }

    VIEW_TEST(testTuneStage, "tune.stage")
    {
        for (float dpi : { 2.0f, 1.0f })
        {
            StageRig rig(tee::ui::makeTuneWorkspace, 1100, 700, dpi, 0.0);
            prepared(rig);
            MapEdit* m = tee::ui::tuneMapEdit(*rig.ws);
            CHECK(m != nullptr && m->shown() && m->shownMap().nx == 16);
            int c = -1, r = -1;
            CHECK(m->liveCell(c, r) && c > 0 && r > 0);    // the engine pulling: its cell is lit
            // A block of three by two chosen.
            const funkgui::Point a = centreOf(m->cellRect(9, 6)), b = centreOf(m->cellRect(11, 7));
            rig.host->click(a.x, a.y);
            rig.host->click(b.x, b.y, funkgui::Mods{ .shift = true });
            play(rig, 5.0, 12);
            golden(dpi > 1.5f ? "tune.stage" : "tune.stage.dpi1", *rig.host);
        }
        return 0;
    }

    VIEW_TEST(testTuneLarge, "tune.large")
    {
        for (float dpi : { 2.0f, 1.0f })
        {
            StageRig rig(tee::ui::makeTuneWorkspace, 1600, 1000, dpi, 0.0);
            prepared(rig);
            golden(dpi > 1.5f ? "tune.large" : "tune.large.dpi1", *rig.host);
        }
        StageRig paper(tee::ui::makeTuneWorkspace, 1100, 700, 2.0f, 0.0);
        prepared(paper);
        paper.host = std::make_unique<funkgui::HeadlessHost>(*paper.ws, 1, 2.0f);
        paper.host->resize(1100, 700);
        paper.ws->place({ 0.0f, 0.0f, 1100.0f, 700.0f });
        play(paper, 5.0, 30);
        golden("tune.paper", *paper.host);
        return 0;
    }

    // Edits: each one gesture of the document (one step of undo), so the dyno's prediction answers.
    VIEW_TEST(testTuneEdit, "tune.edit")
    {
        StageRig rig(tee::ui::makeTuneWorkspace, 1100, 700, 1.0f, 0.0);
        prepared(rig);
        auto& doc = rig.demo.doc;
        const int f = doc.find("ecu.spark");
        MapEdit* m = tee::ui::tuneMapEdit(*rig.ws);
        const tee::ui::Map16 before = doc.map(f);
        const int steps0 = doc.steps();
        // A block: (2, 3) to (5, 6), by a click and a Shift-click.
        const funkgui::Point a = centreOf(m->cellRect(2, 3)), b = centreOf(m->cellRect(5, 6));
        rig.host->click(a.x, a.y);
        rig.host->click(b.x, b.y, funkgui::Mods{ .shift = true });
        int c0 = 0, r0 = 0, c1 = 0, r1 = 0;
        CHECK(m->block(c0, r0, c1, r1) && c0 == 2 && r0 == 3 && c1 == 5 && r1 == 6);
        // Page Up: every cell of the block up half a degree, the rest as it was.
        rig.host->keys("pageup");
        tee::ui::Map16 now = doc.map(f);
        CHECK(doc.steps() == steps0 + 1);
        CHECK_NEAR(now.z[4][3], before.z[4][3] + 0.5, 1e-9);
        CHECK_NEAR(now.z[2][3], before.z[2][3], 1e-12);
        CHECK_NEAR(now.z[4][6], before.z[4][6], 1e-12);
        // Shift-arrow grows the block by a column.
        rig.host->keys("shift+right");
        CHECK(m->block(c0, r0, c1, r1) && c1 == 6);
        // Fill between corners: the inside is the corners' bilinear blend in the axes' values.
        rig.host->keys("i");
        now = doc.map(f);
        CHECK(doc.steps() == steps0 + 2);
        const double tx = (now.x[4] - now.x[2]) / (now.x[6] - now.x[2]), ty = (now.y[5] - now.y[3]) / (now.y[6] - now.y[3]);
        const double want = (now.z[3][2] * (1 - tx) + now.z[3][6] * tx) * (1 - ty) + (now.z[6][2] * (1 - tx) + now.z[6][6] * tx) * ty;
        CHECK_NEAR(now.z[5][4], want, 1e-9);
        // Smooth: each cell of the block halfway to the mean of its four neighbours.
        const tee::ui::Map16 rough = doc.map(f);
        rig.host->keys("s");
        const double mean = (rough.z[5][3] + rough.z[5][5] + rough.z[4][4] + rough.z[6][4]) / 4.0;
        CHECK_NEAR(doc.map(f).z[5][4], 0.5 * rough.z[5][4] + 0.5 * mean, 1e-9);
        CHECK(doc.steps() == steps0 + 3);
        // Typed: a digit starts the cell's value, Enter keeps it.
        rig.host->click(a.x, a.y);
        typeKeys(*rig.host, "1 2 . 5 enter");
        CHECK_NEAR(doc.map(f).z[3][2], 12.5, 1e-9);
        // Undo, four times: the map as it was.
        for (int i = 0; i < 4; ++i)
            doc.undo();
        CHECK(doc.map(f) == before);
        // The dyno answers the edit (a new prediction is asked once the document moves).
        rig.host->keys("pageup");
        bool asked = false;
        for (int i = 0; i < 40 && !asked; ++i)
        {
            rig.demo.engine.tick(1.0f / 60.0f);
            asked = rig.demo.engine.busy();
        }
        CHECK(asked);
        picture("tune.edited", *rig.host);
        return 0;
    }

    // A map the build lacks (lambda, in the demo) says so; ECU values the program lacks draw a dash, not a number.
    VIEW_TEST(testTuneAbsent, "tune.absent")
    {
        StageRig rig(tee::ui::makeTuneWorkspace, 1100, 700, 1.0f, 4.6);
        rig.settle();
        tee::ui::tuneChoose(*rig.ws, 1);
        CHECK(tee::ui::tuneMapEdit(*rig.ws) == nullptr);
        rig.settle();
        picture("tune.absent", *rig.host);
        // At rest (the engine off) it asks for nothing.
        rig.demo.engine.set("engine.state", 0.0);
        rig.settle();
        CHECK(!rig.ws->moving());
        const std::uint64_t k = rig.ws->drawKey();
        rig.settle();
        CHECK(rig.ws->drawKey() == k);
        return 0;
    }
}

#include "kit/bind/Binding.h"

namespace
{
    // Pressing a map's row selects its field, and the inspector's page names it (and its formula switch when the build
    // has one); a lamp selects its part of the ECU; a lane its quantity's section.
    VIEW_TEST(testTuneInspector, "tune.inspector")
    {
        StageRig rig(tee::ui::makeTuneWorkspace, 1100, 700, 1.0f, 4.6);
        tee::ui::tunedemo::fillDocument(rig.demo.doc);
        rig.settle();
        auto& sel = rig.demo.selection;
        rig.host->click(100.0f, 60.0f);                  // the first row: spark advance
        CHECK_MSG(sel.path() == "ecu.spark", sel.path());
        const std::string page = rig.ws->inspectorPage();
        const std::vector<int> fields = tee::ui::fieldsOfGui(rig.demo.doc, page);
        CHECK_MSG(fields.size() == 1 && fields[0] == rig.demo.doc.find("ecu.spark"), page);
        rig.host->click(100.0f, 340.0f);                 // wastegate duty
        CHECK_MSG(sel.path() == "boost.duty", sel.path());
        // The lamps, at the traces' right: the limiter first.
        rig.host->click(1100.0f - 16.0f - 12.0f - 150.0f, 700.0f - 16.0f - 210.0f + 30.0f + 15.0f);
        CHECK_MSG(sel.path() == "ecu.limiter", sel.path());
        CHECK(rig.ws->inspectorPage().empty());          // the frame's generated page of that prefix
        return 0;
    }
}
