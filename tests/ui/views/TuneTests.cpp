// tests/ui/views/TuneTests.cpp: the TUNE workspace (ui/views/tune/) over the demo engine, with TuneDemo.h's spark map and
// live ECU values: its stage at both densities, both themes and two sizes; a block chosen, nudged, smoothed, filled
// between its corners and undone; the live cell lit and trailed; a map the build lacks and ECU values the program
// lacks drawn as absent.
#include "ViewKit.h"

#include "views/tune/Tune.h"
#include "views/tune/TuneDemo.h"

namespace tee::test::u4
{
    std::vector<std::string> textClashesOf(const funkgui::PrimList& list, float w, float h);
}

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

    // The stage's goldens (at the frame's stage rects) are made in DynoTests.cpp, with DYNO's.

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

    // A map the build lacks (the boost target, in the demo) says so; ECU values the program lacks draw a dash, not a number.
    VIEW_TEST(testTuneAbsent, "tune.absent")
    {
        StageRig rig(tee::ui::makeTuneWorkspace, 1100, 700, 1.0f, 4.6);
        rig.settle();
        tee::ui::tuneChoose(*rig.ws, 2);
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
        rig.host->click(100.0f, 104.0f);                 // lambda: the second map the demo has, with its formula switch
        CHECK_MSG(sel.path() == "ecu.maps.lambda", sel.path());
        {
            const std::string lp = rig.ws->inspectorPage();
            CHECK_MSG(lp.find("switch ecu.maps.lambdaAuto") != std::string::npos, lp);
            CHECK_MSG(inspectorNotes(*rig.ws, rig.demo).empty(), inspectorNotes(*rig.ws, rig.demo));
        }
        // The formula in use: the map's row says so; turned off (as the switch does), the table is in use.
        auto& doc = rig.demo.doc;
        const int autoF = doc.find("ecu.maps.lambdaAuto");
        CHECK(autoF >= 0 && doc.get(autoF) > 0.5);
        rig.settle();
        picture("tune.auto", *rig.host);
        doc.beginGesture();
        doc.set(autoF, 0.0);
        doc.endGesture();
        rig.settle();
        rig.host->click(100.0f, 224.0f);                 // wastegate duty, the fifth (after the intake cam, the knock limit)
        CHECK_MSG(sel.path() == "boost.duty", sel.path());
        // The lamps, at the traces' right under the knock by cylinder: the limiter first.
        rig.host->click(1100.0f - 16.0f - 12.0f - 150.0f, 700.0f - 16.0f - 210.0f + 30.0f + 46.0f + 10.0f);
        CHECK_MSG(sel.path() == "ecu.limiter", sel.path());
        CHECK(rig.ws->inspectorPage().empty());          // the frame's generated page of that prefix
        return 0;
    }

    // BAKE: asks the simulator (FrameServices::bakeMaps); when the Scope's bake count moves, the formula's values at the
    // breakpoints are the table and the table is in use (one step of undo).
    VIEW_TEST(testTuneBake, "tune.bake")
    {
        StageRig rig(tee::ui::makeTuneWorkspace, 1100, 700, 1.0f, 4.6);
        int asked = 0;
        tee::ui::FrameServices svc;
        svc.bakeMaps = [&asked] { ++asked; };
        rig.demo.ctx.services = &svc;
        tee::ui::tuneChoose(*rig.ws, 1);                 // lambda, on its formula
        rig.settle();
        const funkgui::Rect b = tee::ui::tuneBakeButton(*rig.ws);
        CHECK(!b.isEmpty());
        rig.host->click(b.centreX(), b.centreY());
        CHECK(asked == 1);
        // The simulator's bake: lambda (map 1) at 4 by 3 breakpoints, 0.8 + i/100 + j/10.
        float count = 0.0f;
        rig.demo.engine.addScope("bakeCount", [&count](const tee::ui::DemoEngine&, float* out, int) { out[0] = count; return 1; });
        rig.demo.engine.addScope("bakedNx", [](const tee::ui::DemoEngine&, float* out, int max) {
            for (int i = 0; i < max; ++i) out[i] = 4.0f;
            return max;
        });
        rig.demo.engine.addScope("bakedNy", [](const tee::ui::DemoEngine&, float* out, int max) {
            for (int i = 0; i < max; ++i) out[i] = 3.0f;
            return max;
        });
        rig.demo.engine.addScope("bakedX", [](const tee::ui::DemoEngine&, float* out, int max) {
            for (int i = 0; i < max; ++i) out[i] = static_cast<float>(100.0 + 100.0 * (i % 16));
            return max;
        });
        rig.demo.engine.addScope("bakedY", [](const tee::ui::DemoEngine&, float* out, int max) {
            for (int i = 0; i < max; ++i) out[i] = static_cast<float>(0.5 + 0.5 * (i % 16));
            return max;
        });
        rig.demo.engine.addScope("baked", [](const tee::ui::DemoEngine&, float* out, int max) {
            for (int k = 0; k < max; ++k)
            {
                const int i = (k % 256) / 16, j = k % 16;
                out[k] = static_cast<float>(0.8 + i / 100.0 + j / 10.0);
            }
            return max;
        });
        rig.settle();
        auto& doc = rig.demo.doc;
        const int f = doc.find("ecu.maps.lambda"), a = doc.find("ecu.maps.lambdaAuto");
        CHECK(doc.get(a) > 0.5);                          // nothing yet: the count has not moved
        const int steps = doc.steps();
        count = 1.0f;
        rig.settle();
        const tee::ui::Map16 m = doc.map(f);
        CHECK(m.nx == 4 && m.ny == 3);
        CHECK_NEAR(m.z[2][3], 0.8 + 0.03 + 0.2, 1e-6);   // z[row j][column i] = baked[i * 16 + j]
        CHECK(doc.get(a) < 0.5);
        CHECK(doc.steps() == steps + 1);
        return 0;
    }

    // The knock-limit table (card N2's, baked by card C's dyno sweep): its switch says "not baked"; no BAKE (the host
    // bakes it); the bake's state as the host says it (FrameServices::sparkMap) over the spark map and over it.
    VIEW_TEST(testTuneKnockLimit, "tune.knocklimit")
    {
        StageRig rig(tee::ui::makeTuneWorkspace, 1100, 700, 1.0f, 4.6);
        tee::ui::FrameServices svc;
        std::string state = "baking";
        svc.sparkMap = [&state] { return state; };
        rig.demo.ctx.services = &svc;
        rig.demo.engine.set("ecu.knockTable", 0.0);
        tee::ui::tuneChoose(*rig.ws, 7);
        rig.settle();
        CHECK(tee::ui::tuneMapEdit(*rig.ws) != nullptr);
        CHECK(tee::ui::tuneBakeButton(*rig.ws).isEmpty());
        const std::uint64_t k0 = rig.ws->drawKey();
        picture("tune.knocklimit", *rig.host);
        state = "baked";                                 // the host's bake lands: what is drawn moves
        rig.demo.engine.set("ecu.knockTable", 1.0);
        rig.settle();
        CHECK(rig.ws->drawKey() != k0);
        tee::ui::tuneChoose(*rig.ws, 0);                 // and it is said over the spark map too
        rig.settle();
        picture("tune.knocklimit.spark", *rig.host);
        // Cells that still knock after the bake's corrections ([speed][load] on the table's grid): ringed and hatched on the
        // knock-limit map, counted in its words.
        {
            const int kl = rig.demo.doc.find("ecu.maps.knockLimitAuto");
            rig.demo.doc.beginGesture();
            rig.demo.doc.set(kl, 0.0);                   // baked: the table in use
            rig.demo.doc.endGesture();
            std::vector<std::vector<bool>> grid(8, std::vector<bool>(8, false));
            grid[5][3] = grid[6][3] = grid[6][4] = true;
            svc.stillKnocking = [grid] { return grid; };
            tee::ui::tuneChoose(*rig.ws, 7);
            rig.settle();
            const std::uint64_t before = rig.ws->drawKey();
            picture("tune.knocklimit.still", *rig.host);
            for (const std::string& clash : tee::test::u4::textClashesOf(rig.host->draw(), 1100.0f, 700.0f))
                CHECK_MSG(false, clash);
            svc.stillKnocking = [] { return std::vector<std::vector<bool>>{}; };
            rig.settle();
            CHECK(rig.ws->drawKey() != before);
        }
        // The intake cam: set aside (its switch on) while the calibration bakes its cam pass, said in the same place.
        tee::ui::tuneChoose(*rig.ws, 3);
        state = "baking cam";
        rig.settle();
        CHECK(tee::ui::tuneMapEdit(*rig.ws) != nullptr);
        const std::uint64_t k1 = rig.ws->drawKey();
        picture("tune.knocklimit.cam", *rig.host);
        auto& doc = rig.demo.doc;
        const int camAuto = doc.find("ecu.maps.camInAuto");
        CHECK(camAuto >= 0);
        doc.beginGesture();
        doc.set(camAuto, 0.0);                           // the bake applied: the table in use
        doc.endGesture();
        state = "baked";
        rig.settle();
        CHECK(rig.ws->drawKey() != k1);
        return 0;
    }
}
