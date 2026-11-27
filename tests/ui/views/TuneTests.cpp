// tests/ui/views/TuneTests.cpp: the TUNE workspace (ui/views/tune/) over the demo engine, with TuneDemo.h's spark map and
// live ECU values: its stage at both densities, both themes and two sizes; a block chosen, nudged, smoothed, filled
// between its corners and undone; the live cell lit and trailed; a map the build lacks and ECU values the program
// lacks drawn as absent.
#include "ViewKit.h"

#include "views/tune/Tune.h"
#include "views/tune/TuneDemo.h"
#include "views/tune/Fuel.h"

namespace tee::test::u4
{
    std::vector<std::string> textClashesOf(const funkgui::PrimList& list, float w, float h);
}

#include <funkgui/core/Theme.h>

#include <fstream>
#include <iterator>

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
        rig.host->click(100.0f, 126.0f);                 // the first row: spark advance (under the fuel's band)
        CHECK_MSG(sel.path() == "ecu.spark", sel.path());
        const std::string page = rig.ws->inspectorPage();
        const std::vector<int> fields = tee::ui::fieldsOfGui(rig.demo.doc, page);
        CHECK_MSG(fields.size() == 1 && fields[0] == rig.demo.doc.find("ecu.spark"), page);
        rig.host->click(100.0f, 170.0f);                 // lambda: the second map the demo has, with its formula switch
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
        rig.host->click(100.0f, 290.0f);                 // wastegate duty, the fifth (after the intake cam, the knock limit)
        CHECK_MSG(sel.path() == "boost.duty", sel.path());
        // The lamps, at the traces' right under the knock by cylinder: the limiter first.
        rig.host->click(1100.0f - 16.0f - 12.0f - 150.0f, 700.0f - 16.0f - 210.0f + 30.0f + 46.0f + 10.0f);
        CHECK_MSG(sel.path() == "ecu.limiter", sel.path());
        CHECK(rig.ws->inspectorPage().empty());          // the frame's generated page of that prefix
        rig.host->click(600.0f, 60.0f);                  // the fuel's band, off its buttons: the grade
        CHECK_MSG(sel.path() == "fuel.grade", sel.path());
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

    // The simulator's own source, as the application serves it (FrameServices::simSource): this checkout's sim/.
    std::string simSource(const std::string& file)
    {
        std::ifstream in(std::string(TEE_VIEW_GOLDEN_DIR) + "/../../../../sim/" + file, std::ios::binary);
        return in ? std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()) : std::string();
    }
    bool has(const std::string& text, const std::string& part) { return text.find(part) != std::string::npos; }

    // The fuel (sim/spec/fuel.fm's grade): stepped in TUNE's band, one step of undo; its octane numbers from the
    // simulator's table (sim/combustion/fuels.fm, read through simSource; never typed in); the knock-limit table shown
    // after a change, the bake's state said and then what the re-baked table gives; a diesel without octane; the
    // untested grades marked; the reference burn said to ignore it; no table served: dashes.
    VIEW_TEST(testTuneFuel, "tune.fuel")
    {
        StageRig rig(tee::ui::makeTuneWorkspace, 1100, 700, 1.0f, 4.6);
        auto& doc = rig.demo.doc;
        const int grade = doc.find("fuel.grade"), model = doc.find("fuel.model");
        CHECK(grade >= 0 && model >= 0);
        // Not served: the grade by name, no number.
        rig.settle();
        std::string t = tee::ui::tuneFuelText(*rig.ws);
        CHECK_MSG(has(t, "Premium (91) | AKI -   RON -   MON -") && has(t, "not read"), t);
        // Served: the table's numbers (Premium: RON 96, MON 86 in fuels.fm; AKI their mean).
        tee::ui::FrameServices svc;
        std::string state = "baked";
        svc.simSource = simSource;
        svc.sparkMap = [&state] { return state; };
        rig.demo.ctx.services = &svc;
        rig.settle();
        t = tee::ui::tuneFuelText(*rig.ws);
        CHECK_MSG(has(t, "Premium (91) | AKI 91   RON 96   MON 86 | Against Regular (87), the end gas waits 1.20 times"), t);
        picture("tune.fuel", *rig.host);
        for (const std::string& clash : tee::test::u4::textClashesOf(rig.host->draw(), 1100.0f, 700.0f))
            CHECK_MSG(false, clash);
        // Next: by octane, Premium (93) (one gesture), the knock-limit table shown, its bake said.
        const int steps = doc.steps();
        const funkgui::Rect next = tee::ui::tuneFuelButton(*rig.ws, 1);
        CHECK(!next.isEmpty());
        rig.host->click(next.centreX(), next.centreY());
        state = "baking spark map 3 of 9";
        rig.settle();
        CHECK(std::lround(doc.get(grade)) == 9);
        CHECK(doc.steps() == steps + 1);
        CHECK(tee::ui::tuneShownMap(*rig.ws) == 7);
        t = tee::ui::tuneFuelText(*rig.ws);
        CHECK_MSG(has(t, "Premium (93) | AKI 93   RON 98   MON 88 | Premium (91) to Premium (93): the knock-limit table re-bakes (baking the spark map 3 of 9)."), t);
        picture("tune.fuel.baking", *rig.host);
        // The bake lands: the table (baked before at 4 deg everywhere, now 3 off boost and 1 on it) in degrees.
        const int kl = doc.find("ecu.maps.knockLimit"), kla = doc.find("ecu.maps.knockLimitAuto");
        tee::ui::Map16 m;
        m.nx = 2;
        m.ny = 2;
        m.x[0] = 100.0;
        m.x[1] = 600.0;
        m.y[0] = 0.5;
        m.y[1] = 1.8;
        m.z[0][0] = m.z[0][1] = m.z[1][0] = m.z[1][1] = 4.0;
        doc.beginGesture();
        doc.set(grade, 2.0);                 // back to Premium (91) with a baked table, then Premium (93) again
        doc.setMap(kl, m);
        doc.set(kla, 0.0);
        doc.endGesture();
        rig.settle();
        rig.host->click(next.centreX(), next.centreY());
        rig.settle();
        CHECK(has(tee::ui::tuneFuelText(*rig.ws), "re-bakes"));
        m.z[0][0] = m.z[0][1] = 3.0;
        m.z[1][0] = m.z[1][1] = 1.0;
        doc.beginGesture();
        doc.setMap(kl, m);
        doc.endGesture();
        state = "baked";
        rig.settle();
        t = tee::ui::tuneFuelText(*rig.ws);
        CHECK_MSG(has(t, "Premium (91) to Premium (93): 1.0 deg more advance off boost, 3.0 deg more advance on boost"), t);
        picture("tune.fuel.baked", *rig.host);
        for (const std::string& clash : tee::test::u4::textClashesOf(rig.host->draw(), 1100.0f, 700.0f))
            CHECK_MSG(false, clash);
        // Up the octanes (E85, Methanol, Race, then the untested grades): marked; past the end, nothing moves.
        for (int i = 0; i < 6; ++i)
        {
            rig.host->click(next.centreX(), next.centreY());
            rig.settle();
        }
        CHECK(std::lround(doc.get(grade)) == 8);   // Diesel (6) stepped over
        t = tee::ui::tuneFuelText(*rig.ws);
        CHECK_MSG(has(t, "Hydrogen (untested) |") && has(t, "Untested: nothing has run this fuel"), t);
        // A diesel: its own fuel, no octane, no stepping.
        doc.beginGesture();
        doc.set(grade, 6.0);
        doc.endGesture();
        rig.settle();
        t = tee::ui::tuneFuelText(*rig.ws);
        CHECK_MSG(has(t, "Diesel | No octane: lit by compression | A diesel's knock is its premixed burn"), t);
        CHECK(tee::ui::tuneFuelButton(*rig.ws, 1).isEmpty() && tee::ui::tuneFuelButton(*rig.ws, -1).isEmpty());
        picture("tune.fuel.diesel", *rig.host);
        // The reference burn: the grade not read, said so.
        doc.beginGesture();
        doc.set(grade, 0.0);
        doc.set(model, 0.0);
        doc.endGesture();
        rig.settle();
        CHECK(has(tee::ui::tuneFuelText(*rig.ws), "The reference burn does not read the grade"));
        // The literals' reader, whatever the platform's stream parser does with a suffix (libc++'s took 'f' into the
        // number): suffixed, capital, bare and exponent forms read; a malformed list reads as nothing.
        {
            const std::vector<double> v = tee::ui::fuel::numberList("{ 91.0f, 96.0f, 100.0F, 87 }");
            CHECK(v.size() == 4);
            if (v.size() == 4)
                CHECK(v[0] == 91.0 && v[1] == 96.0 && v[2] == 100.0 && v[3] == 87.0);
            const std::vector<double> e = tee::ui::fuel::numberList("{1100.0e3f, -3.402f}");
            CHECK(e.size() == 2 && e[0] == 1100.0e3 && e[1] == -3.402);
            CHECK(tee::ui::fuel::numberList("{ 91.0f, x, 87 }").empty());
            CHECK(tee::ui::fuel::numberList("{ 91.0f 96.0f }").empty());
            CHECK(tee::ui::fuel::numberList("{ 91.0ff }").empty());
            CHECK(tee::ui::fuel::numberList("91.0f, 87").empty());
        }
        // The table's reader on the simulator's file itself: every grade a row, the diesel none, two untested.
        const tee::ui::fuel::Table table = tee::ui::fuel::parse(simSource("combustion/fuels.fm"), simSource("combustion/knock.fm"));
        // (every grade a row: ten since the 93 was appended last, card N1 25 Nov; it a tested spark-lit grade)
        CHECK(table.rows.size() == 10);
        if (table.rows.size() < 10)
            return 0;                                    // (said above; never indexed past what was read)
        CHECK(!table.rows[9].compression && !table.rows[9].untested && table.aki(9) == 93.0);
        CHECK(std::isnan(table.aki(6)) && table.rows[6].compression);
        CHECK(table.rows[7].untested && table.rows[8].untested && !table.rows[2].untested);
        CHECK(table.exponent > 3.0 && table.exponent < 4.0);
        return 0;
    }

    // The octane as a number (spec.fuel.octane, the pump's AKI; 0: the grade's own) beside the grade: typed in the band
    // (one gesture; a word that is no number in range refused), its RON and MON as the simulator derives them (by the
    // grade's sensitivity, fuels.fm fuelOctaneAt), a mark in words past the knock law's fit (fuelOctaneExtrapolated's
    // limits, read from the simulator's text), the change re-baking as a grade's does, one press back to the grade's
    // own; none for a diesel.
    VIEW_TEST(testTuneOctane, "tune.octane")
    {
        StageRig rig(tee::ui::makeTuneWorkspace, 1100, 700, 1.0f, 4.6);
        auto& doc = rig.demo.doc;
        const int grade = doc.find("fuel.grade"), octane = doc.find("fuel.octane");
        CHECK(grade >= 0 && octane >= 0);
        tee::ui::FrameServices svc;
        std::string state = "baked";
        svc.simSource = simSource;
        svc.sparkMap = [&state] { return state; };
        rig.demo.ctx.services = &svc;
        {
            // the knock-limit table baked, as the shipped builds have it (its unbaked note lies over the cells)
            doc.beginGesture();
            doc.set(doc.find("ecu.maps.knockLimitAuto"), 0.0);
            doc.endGesture();
        }
        rig.settle();
        // The grade's own: empty, its AKI shown dim over the field.
        std::string t = tee::ui::tuneFuelText(*rig.ws);
        CHECK_MSG(has(t, "Premium (91) | AKI 91   RON 96   MON 86 | Against"), t);
        const funkgui::Rect field = tee::ui::tuneFuelButton(*rig.ws, 2), own = tee::ui::tuneFuelButton(*rig.ws, 3);
        CHECK(!field.isEmpty() && !own.isEmpty());
        picture("tune.octane.own", *rig.host);
        // 95: RON 95 + (96 - 86) / 2 = 100, inside the fit; one gesture.
        const int steps = doc.steps();
        rig.host->click(field.centreX(), field.centreY());
        typeKeys(*rig.host, "9 5 enter");
        state = "baking spark map 1 of 9";
        rig.settle();
        CHECK_NEAR(doc.get(octane), 95.0, 1e-9);
        CHECK(doc.steps() == steps + 1);
        CHECK(tee::ui::tuneShownMap(*rig.ws) == 7);
        t = tee::ui::tuneFuelText(*rig.ws);
        CHECK_MSG(has(t, "Premium (91) | AKI 95   RON 100   MON 90 | Premium (91) to Premium (91) at 95: the knock-limit table re-bakes (baking the spark map 1 of 9)."), t);
        CHECK_MSG(!has(t, "EXTRAPOLATED"), t);
        // 97: RON 102, past the fit: marked in words.
        rig.host->click(field.centreX(), field.centreY());
        typeKeys(*rig.host, "9 7 enter");
        rig.settle();
        t = tee::ui::tuneFuelText(*rig.ws);
        CHECK_MSG(has(t, "AKI 97   RON 102   MON 92 | EXTRAPOLATED: the knock fit is RON 80 to 100 | Premium (91) at 95 to Premium (91) at 97"), t);
        picture("tune.octane", *rig.host);
        for (const std::string& clash : tee::test::u4::textClashesOf(rig.host->draw(), 1100.0f, 700.0f))
            CHECK_MSG(false, clash);
        // Not a number, or out of the field's range: refused, nothing moves.
        const int before = doc.steps();
        rig.host->click(field.centreX(), field.centreY());
        typeKeys(*rig.host, "x enter");
        typeKeys(*rig.host, "escape");
        rig.settle();
        CHECK_NEAR(doc.get(octane), 97.0, 1e-9);
        CHECK(doc.steps() == before);
        // One press back to the grade's own.
        rig.host->click(own.centreX(), own.centreY());
        rig.settle();
        CHECK(doc.get(octane) == 0.0);
        t = tee::ui::tuneFuelText(*rig.ws);
        CHECK_MSG(has(t, "Premium (91) | AKI 91   RON 96   MON 86 | Premium (91) at 97 to Premium (91):"), t);
        // A diesel: no octane, no field.
        doc.beginGesture();
        doc.set(octane, 95.0);
        doc.set(grade, 6.0);
        doc.endGesture();
        rig.settle();
        t = tee::ui::tuneFuelText(*rig.ws);
        CHECK_MSG(has(t, "Diesel | No octane: lit by compression |"), t);
        CHECK(tee::ui::tuneFuelButton(*rig.ws, 2).isEmpty() && tee::ui::tuneFuelButton(*rig.ws, 3).isEmpty());
        // The smallest stage: the numbers on a row of their own, nothing over anything.
        {
            StageRig small(tee::ui::makeTuneWorkspace, tee::test::views::FrameSize::smallest, 1.0f, 4.6);
            small.demo.ctx.services = &svc;
            auto& d = small.demo.doc;
            d.beginGesture();
            d.set(d.find("fuel.octane"), 97.0);
            d.set(d.find("ecu.maps.knockLimitAuto"), 0.0);
            d.endGesture();
            small.settle();
            picture("tune.octane.small", *small.host);
            for (const std::string& clash : tee::test::u4::textClashesOf(small.host->draw(), 1100.0f, 700.0f))
                CHECK_MSG(false, clash);
        }
        // The fit's limits, read from the simulator's text.
        const tee::ui::fuel::Table table = tee::ui::fuel::parse(simSource("combustion/fuels.fm"), simSource("combustion/knock.fm"));
        CHECK(table.fitLo == 80.0 && table.fitHi == 100.0);
        CHECK(!table.extrapolated(2, 0.0) && !table.extrapolated(6, 200.0));
        return 0;
    }
}

