// tests/ui/views/GarageTests.cpp: GARAGE (ui/views/garage/) over its demo library: the cards, search, sorting and
// the part filter, a part loaded apart, compare, delete with its confirmation, save as, at rest. Pictures by FunkGui's
// software rasteriser, at density 2 and 1.
#include "ViewKit.h"

#include "views/garage/DemoLibrary.h"
#include "views/garage/Describe.h"
#include "views/garage/GarageWorkspace.h"

#include <cmath>
#include <vector>
#include <string>

namespace tee::test::u5
{
    funkgui::Rect frameStage(int w, int h);   // ScopeTests.cpp
    std::string textFaults(const funkgui::PrimList& prims, const funkgui::Rect& stage, const std::vector<funkgui::Rect>& panels);
}

namespace
{
    using namespace tee::test::views;
    using tee::ui::GarageWorkspace;
    namespace garage = tee::ui::garage;

    GarageWorkspace& garageOf(StageRig& rig) { return dynamic_cast<GarageWorkspace&>(*rig.ws); }

    // Presses the control whose accessible title is `title` (a button by its words); false when none is shown.
    bool press(StageRig& rig, const std::string& title)
    {
        for (const funkgui::A11yItem& it : rig.host->accessibility())
            if (it.title == title && it.visible && !it.bounds.isEmpty())
            {
                rig.host->click(it.bounds.centreX(), it.bounds.centreY());
                rig.settle();
                return true;
            }
        return false;
    }

    double field(StageRig& rig, const char* path) { return rig.demo.doc.get(rig.demo.doc.find(path)); }

    int indexOf(GarageWorkspace& g, const std::string& name)
    {
        const auto& all = g.library().builds();
        for (int k = 0; k < static_cast<int>(g.shown().size()); ++k)
            if (all[static_cast<std::size_t>(g.shown()[static_cast<std::size_t>(k)])].name == name)
                return k;
        return -1;
    }

    const garage::BuildSummary* named(GarageWorkspace& g, const std::string& name)
    {
        for (const auto& b : g.library().builds())
            if (b.name == name)
                return &b;
        return nullptr;
    }

    // The cards over the demo library: every build shown, the current one marked, the sentences made from the fields.
    VIEW_TEST(testGarageStage, "garage.stage")
    {
        StageRig rig(tee::ui::makeGarageWorkspace, 1100, 700);
        GarageWorkspace& g = garageOf(rig);
        CHECK(g.demo());
        CHECK(g.shown().size() == 10);
        const garage::BuildSummary* cur = named(g, "Street V8");
        CHECK(cur != nullptr && cur->current && g.chosen() == cur->id);
        CHECK(!g.library().unsaved());
        CHECK(garage::sentence(*named(g, "Hot Six"), rig.demo.doc) == "3.0 L straight six, twin turbo, E85");
        CHECK(garage::sentence(*cur, rig.demo.doc) == "5.7 L V8, turbo");
        CHECK(garage::sentence(*named(g, "Light Coupe"), rig.demo.doc).empty());
        CHECK(!cur->curve.empty() && named(g, "Light Coupe")->curve.empty());
        // Sorted by name at first.
        CHECK(indexOf(g, "Big Block") == 0);
        // A card pressed is chosen.
        const funkgui::Rect r = g.cardRect(indexOf(g, "Big Block"));
        rig.host->click(r.centreX(), r.centreY());
        rig.settle();
        CHECK(g.chosen() == named(g, "Big Block")->id);
        golden("garage.stage", *rig.host);
        StageRig one(tee::ui::makeGarageWorkspace, 1100, 700, 1.0f);
        golden("garage.stage.dpi1", *one.host);
        // Every card is listed for a screen reader.
        int cards = 0;
        for (const funkgui::A11yItem& it : rig.host->accessibility())
            cards += it.role == funkgui::A11yRole::listItem ? 1 : 0;
        CHECK(cards == 10);
        return 0;
    }

    VIEW_TEST(testGarageLarge, "garage.large")
    {
        StageRig big(tee::ui::makeGarageWorkspace, 1600, 1000);
        golden("garage.large", *big.host);
        StageRig one(tee::ui::makeGarageWorkspace, 1600, 1000, 1.0f);
        golden("garage.large.dpi1", *one.host);
        tee::ui::DemoRig demo(4.6, tee::ui::DemoHooks::of({ "garage", "scope" }));
        auto ws = tee::ui::makeGarageWorkspace(demo.ctx);
        funkgui::HeadlessHost host(*ws, 1, 2.0f);
        host.resize(1100, 700);
        ws->place({ 0.0f, 0.0f, 1100.0f, 700.0f });
        host.tick(90);
        golden("garage.paper", host);
        return 0;
    }

    // Search as it is typed; sorting; the part filter.
    VIEW_TEST(testGarageSearch, "garage.search")
    {
        StageRig rig(tee::ui::makeGarageWorkspace, 1100, 700);
        GarageWorkspace& g = garageOf(rig);
        for (const funkgui::A11yItem& it : rig.host->accessibility())
            if (it.title == "Search")
                rig.host->click(it.bounds.centreX(), it.bounds.centreY());
        typeKeys(*rig.host, "s i x");
        rig.settle();
        CHECK(indexOf(g, "Hot Six") >= 0 && indexOf(g, "Tow Diesel") >= 0);   // "straight six" in its sentence
        CHECK(indexOf(g, "Street V8") < 0);
        picture("garage.search", *rig.host);
        typeKeys(*rig.host, "escape");
        g.setSearch("");
        rig.settle();
        CHECK(g.shown().size() == 10);
        // By power, the most first; a car and a tune (no curve) last.
        g.setSort(GarageWorkspace::Sort::power);
        double last = 1e30;
        bool tail = false;
        for (int i : g.shown())
        {
            const double pw = g.library().builds()[static_cast<std::size_t>(i)].curve.peakPower().value;
            if (!std::isfinite(pw))
                tail = true;
            else
            {
                CHECK(!tail && pw <= last);
                last = pw;
            }
        }
        g.setSort(GarageWorkspace::Sort::displacement);
        CHECK(g.library().builds()[static_cast<std::size_t>(g.shown()[0])].name == "Big Block");
        CHECK(press(rig, "ENGINES"));
        CHECK(g.shown().size() == 1 && indexOf(g, "Supercharged V6") == 0);
        CHECK(press(rig, "TUNES"));
        CHECK(g.shown().size() == 1 && indexOf(g, "Track Tune") == 0);
        CHECK(press(rig, "ALL"));
        CHECK(g.shown().size() == 10);
        return 0;
    }

    // The three parts load apart: an engine into this car, a tune on this engine; the whole build replaces all.
    VIEW_TEST(testGarageParts, "garage.parts")
    {
        StageRig rig(tee::ui::makeGarageWorkspace, 1100, 700);
        GarageWorkspace& g = garageOf(rig);
        const double mass = field(rig, "car.mass");
        g.choose(named(g, "Hot Six")->id);
        rig.settle();
        CHECK(press(rig, "ITS ENGINE"));
        CHECK(field(rig, "engine.layout") == 1.0 && field(rig, "fuel.kind") == 1.0);   // I6 on E85
        CHECK(field(rig, "car.mass") == mass);                                        // the car is this one
        CHECK(named(g, "Street V8")->current && g.library().unsaved());
        CHECK(g.message().find("engine") != std::string::npos);
        picture("garage.parts", *rig.host);
        g.choose(named(g, "Track Tune")->id);
        rig.settle();
        CHECK(press(rig, "TRY THIS TUNE"));
        CHECK(field(rig, "ecu.pops") == 1.0 && field(rig, "engine.layout") == 1.0);
        g.choose(named(g, "Hot Six")->id);
        rig.settle();
        CHECK(press(rig, "LOAD THIS BUILD"));
        CHECK(named(g, "Hot Six")->current && !named(g, "Street V8")->current && !g.library().unsaved());
        CHECK(field(rig, "car.mass") == 1480.0);
        // Undo takes the load back in one step.
        rig.demo.doc.undo();
        CHECK(field(rig, "car.mass") == mass);
        return 0;
    }

    // Compare: the curves overlaid, the differing fields grouped by section in the user's units.
    VIEW_TEST(testGarageCompare, "garage.compare")
    {
        StageRig rig(tee::ui::makeGarageWorkspace, 1100, 700);
        GarageWorkspace& g = garageOf(rig);
        g.choose(named(g, "Hot Six")->id);
        rig.settle();
        CHECK(press(rig, "COMPARE"));
        CHECK(g.comparing());
        bool layout = false, mass = false;
        std::string lastSection;
        for (const garage::Difference& d : g.differences())
        {
            if (d.path == "engine.layout")
                layout = d.a == "V8" && d.b == "I6";
            if (d.path == "car.mass")
                mass = d.a != d.b && d.a.find("kg") != std::string::npos;
            lastSection = d.section;
        }
        CHECK(layout && mass);
        golden("garage.compare", *rig.host);
        StageRig one(tee::ui::makeGarageWorkspace, 1100, 700, 1.0f);
        GarageWorkspace& g1 = garageOf(one);
        g1.compareWith(named(g1, "Hot Six")->id);
        one.settle();
        golden("garage.compare.dpi1", *one.host);
        CHECK(press(rig, "BACK TO BUILDS"));
        CHECK(!g.comparing());
        return 0;
    }

    // At the frame's own stages: the cards fit (none wider than the room), the bar wraps, no text leaves the stage or
    // overlaps other text; the cards and the comparison pictured at both densities.
    VIEW_TEST(testGarageFrameStage, "garage.framestage")
    {
        for (const auto& win : { std::pair<int, int>{ 1100, 700 }, std::pair<int, int>{ 1600, 1000 } })
        {
            const funkgui::Rect st = tee::test::u5::frameStage(win.first, win.second);
            const int w = static_cast<int>(st.w), h = static_cast<int>(st.h);
            const std::string tag = win.first == 1100 ? "small" : "large";
            for (const float dpi : { 2.0f, 1.0f })
            {
                StageRig rig(tee::ui::makeGarageWorkspace, w, h, dpi);
                GarageWorkspace& g = garageOf(rig);
                for (int k = 0; k < static_cast<int>(g.shown().size()); ++k)
                {
                    const funkgui::Rect r = g.cardRect(k);
                    CHECK(r.x >= 0.0f && r.right() <= st.w);
                }
                std::string why = tee::test::u5::textFaults(rig.host->draw(), { 0.0f, 0.0f, st.w, st.h }, {});
                CHECK_MSG(why.empty(), tag + " cards: " + why);
                golden("garage.frame." + tag + (dpi == 1.0f ? ".dpi1" : ""), *rig.host);
                g.compareWith(named(g, "Hot Six")->id);
                rig.settle();
                why = tee::test::u5::textFaults(rig.host->draw(), { 0.0f, 0.0f, st.w, st.h }, {});
                CHECK_MSG(why.empty(), tag + " compare: " + why);
                golden("garage.frame." + tag + ".compare" + (dpi == 1.0f ? ".dpi1" : ""), *rig.host);
            }
        }
        return 0;
    }

    // Delete asks first; a shipped build has no delete; duplicate and save as make the user's own.
    VIEW_TEST(testGarageFiles, "garage.files")
    {
        StageRig rig(tee::ui::makeGarageWorkspace, 1100, 700);
        GarageWorkspace& g = garageOf(rig);
        g.choose(named(g, "Big Block")->id);
        rig.settle();
        CHECK(!press(rig, "DELETE"));            // shipped: none
        g.choose(named(g, "My V10")->id);
        rig.settle();
        CHECK(press(rig, "DELETE"));
        CHECK(named(g, "My V10") != nullptr);    // asked, not done
        picture("garage.confirm", *rig.host);
        CHECK(press(rig, "KEEP IT"));
        CHECK(press(rig, "DELETE") && press(rig, "CONFIRM DELETE"));
        CHECK(named(g, "My V10") == nullptr && g.shown().size() == 9);
        // Duplicate a shipped build: a copy of the user's.
        g.choose(named(g, "Big Block")->id);
        rig.settle();
        CHECK(press(rig, "DUPLICATE"));
        CHECK(named(g, "Big Block copy") != nullptr && named(g, "Big Block copy")->user);
        // Save the current build as a name (typed): it becomes the current build, saved.
        g.choose(named(g, "Street V8")->id);
        rig.settle();
        const int f = rig.demo.doc.find("engine.compression");
        rig.demo.doc.beginGesture();
        rig.demo.doc.set(f, 11.0);
        rig.demo.doc.endGesture();
        rig.settle();
        CHECK(g.library().unsaved());
        for (const funkgui::A11yItem& it : rig.host->accessibility())
            if (it.title == "Save as")
                rig.host->click(it.bounds.centreX(), it.bounds.centreY());
        typeKeys(*rig.host, "M i n e enter");
        rig.settle();
        CHECK(named(g, "Mine") != nullptr && named(g, "Mine")->current && !g.library().unsaved());
        CHECK(!named(g, "Street V8")->current);
        // A shipped build is not saved over.
        std::string error;
        g.library().load(named(g, "Big Block")->id, garage::Part::whole, error);
        CHECK(!g.library().save(error) && !error.empty());
        return 0;
    }

    // At rest: nothing moves, the draw key stands; the layout's words.
    VIEW_TEST(testGarageRest, "garage.rest")
    {
        StageRig rig(tee::ui::makeGarageWorkspace, 1100, 700);
        GarageWorkspace& g = garageOf(rig);
        CHECK(!g.moving());
        const std::uint64_t key = g.drawKey();
        rig.host->tick(30);
        CHECK(g.drawKey() == key);
        // The wheel scrolls the cards, eased; then it rests again.
        const funkgui::Rect r = g.cardRect(0);
        rig.host->wheel(r.centreX(), r.centreY(), -3.0f);
        rig.host->tick(1);
        CHECK(g.moving());
        rig.settle();
        CHECK(!g.moving() && g.cardRect(0).y < r.y);
        // A library given after the workspace is made (the shell's, through AppFrame::setLibrary) takes over at once.
        garage::DemoLibrary given(rig.demo.doc);
        rig.demo.ctx.library = &given;
        rig.host->tick(2);
        CHECK(&g.library() == &given && !g.demo());
        rig.demo.ctx.library = nullptr;
        bool registered = false;
        for (const auto& e : tee::ui::registeredWorkspaces())
            registered = registered || std::string(e.id) == "garage";
        CHECK(registered);
        CHECK(garage::layoutWords("Inline 6") == "straight six" && garage::layoutWords("Flat 4") == "flat four");
        CHECK(garage::layoutWords("V8 crossplane") == "V8 crossplane" && garage::layoutWords("Rotary") == "twin rotor");
        CHECK(garage::layoutWords("V-twin 45") == "V-twin");
        return 0;
    }
}
