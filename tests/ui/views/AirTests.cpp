// tests/ui/views/AirTests.cpp: the AIR workspace (ui/views/air/): the charge path over several builds, the compressor's
// map made as the simulator makes it, the trail of a pull, what a press selects, absent values, rest; goldens and
// pictures at density 1 and 2, 1100x700 and 1600x1000, both themes. Every picture is the software rasteriser's.
#include "ViewKit.h"

#include "views/air/AirDemo.h"
#include "views/air/AirModel.h"
#include "views/air/AirWorkspace.h"

#include <algorithm>

namespace
{
    using namespace tee::test::views;
    namespace air = tee::ui::air;

    struct AirRig
    {
        tee::ui::DemoRig demo;
        tee::ui::MemoryDoc doc{ air::airDemoFields() };
        tee::ui::WorkspaceContext ctx{ doc, demo.engine, demo.engine, demo.engine, demo.units, demo.selection, demo.app };
        std::unique_ptr<tee::ui::Workspace> ws;
        std::unique_ptr<funkgui::HeadlessHost> host;
        int w, h;

        AirRig(const char* build, int width = 1100, int height = 700, float dpi = 2.0f, int theme = 0)
            : demo(2.0), w(width), h(height)
        {
            air::setAirDemoBuild(doc, build);
            ws = tee::ui::makeAirWorkspace(ctx);
            host = std::make_unique<funkgui::HeadlessHost>(*ws, theme, dpi);
            host->resize(w, h);
            ws->place({ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) });
            pull(2.0, 4.6);
        }
        // The demo's script from t0 to t1 (a pull through third), the live values made at each step: the trail.
        void pull(double t0, double t1)
        {
            for (double t = t0; t <= t1 + 1e-9; t += 0.05)
            {
                demo.engine.setTime(t);
                air::airDemoLive(demo.engine, doc);
                host->tick(1);
            }
            host->tick(60);
        }
    };

    // The map is the simulator's: the matched sizes of machine.fm, the lines from knee to choke, the islands.
    VIEW_TEST(testAirModel, "air.model")
    {
        tee::ui::MemoryDoc doc{ air::airDemoFields() };
        CHECK(air::setAirDemoBuild(doc, "turbo4"));
        air::AirBuild b = air::resolveAir(doc);
        CHECK(b.charger == air::Charger::turbo && b.turbos == 1 && b.vanes && b.externalGate && b.antiLag && b.nitrous);
        // machine.fm's note: a 2 L four at 7000 rpm gets a 45 mm inducer and a 61 mm exducer
        CHECK_NEAR(2.0 * b.map.rTip, 0.061, 0.004);
        const air::MapLines m = air::mapLines(b.map);
        CHECK(m.maxPr > 2.5 && m.maxPr < 4.5 && m.maxFlow > 0.1 && m.maxFlow < 0.4);
        for (int l = 1; l < air::MapLines::kLines; ++l)
            CHECK(m.pr[l][0] > m.pr[l - 1][0] && m.flow[l][0] >= m.flow[l - 1][0]);   // faster lines higher, no narrower
        for (int l = 0; l < air::MapLines::kLines; ++l)
            CHECK(m.pr[l][0] >= m.pr[l][air::MapLines::kPoints - 1]);               // from the knee down to choke
        CHECK(*std::max_element(m.islandFlow[0].begin(), m.islandFlow[0].end()) > 0.0f);   // the best island exists

        CHECK(air::setAirDemoBuild(doc, "na4-itb"));
        b = air::resolveAir(doc);
        CHECK(b.charger == air::Charger::none && !b.hasMap() && b.itb && b.trumpetsToAir && b.cylinders == 4);
        CHECK(air::setAirDemoBuild(doc, "sc-v8"));
        b = air::resolveAir(doc);
        CHECK(b.charger == air::Charger::supercharger && b.blower == air::Blower::roots && !b.hasMap() && b.intercooler);
        CHECK(air::setAirDemoBuild(doc, "cf-v8"));
        CHECK(air::resolveAir(doc).hasMap());
        CHECK(air::setAirDemoBuild(doc, "seq6"));
        b = air::resolveAir(doc);
        CHECK(b.arrangement == air::Arrangement::sequential && b.turbos == 2 && b.blowOff == 2);
        // U1's demo Spec alone: the layout by label, a turbo by label, matched sizes
        tee::ui::MemoryDoc plain{ tee::ui::demoFields() };
        b = air::resolveAir(plain);
        CHECK(b.layout == 12 && b.charger == air::Charger::turbo && b.turbos == 1 && b.hasMap());
        return 0;
    }

    // The stage over U1's demo alone (only boost.pressure, engine.manifold, engine.throttle live): dashes elsewhere.
    VIEW_TEST(testAirStage, "air.stage")
    {
        StageRig rig(tee::ui::makeAirWorkspace, 1100, 700, 2.0f);
        golden("air.stage", *rig.host);
        StageRig one(tee::ui::makeAirWorkspace, 1100, 700, 1.0f);
        golden("air.stage.dpi1", *one.host);
        return 0;
    }

    // The builds, each after a pull (the trail on the map), density 2; one also at 1.
    VIEW_TEST(testAirBuilds, "air.builds")
    {
        for (const char* b : { "na4-itb", "turbo4", "tt-v8", "sc-v8", "seq6", "cmp6", "cf-v8" })
        {
            AirRig rig(b);
            golden(std::string("air.build.") + b, *rig.host);
        }
        AirRig one("turbo4", 1100, 700, 1.0f);
        golden("air.build.turbo4.dpi1", *one.host);
        return 0;
    }

    // A press on each part selects it; ringed and named; nothing under the pointer keeps it.
    VIEW_TEST(testAirSelect, "air.select")
    {
        AirRig rig("turbo4");
        const std::pair<float, float> ys[] = { { 0.0f, 0.0f } };
        (void)ys;
        // the parts sit along the row's middle: walk it and collect what each press selects
        std::vector<std::string> got;
        for (float x = 30.0f; x < rig.w - 30.0f; x += 6.0f)
        {
            rig.demo.selection.clear();
            rig.host->click(x, 62.0f + 0.36f * rig.h * 0.5f);
            if (!rig.demo.selection.empty() && (got.empty() || got.back() != rig.demo.selection.path()))
                got.push_back(rig.demo.selection.path());
        }
        const std::vector<std::string> want = { "intake.airbox", "boost.turbo[0]", "boost.intercooler", "intake.throttle",
                                                "intake.plenum", "intake.runners" };
        CHECK_MSG(got == want, [&] { std::string s; for (auto& g : got) s += g + " "; return s; }());
        // the lower row: the map, the shaft, the gate, the blow-off valve
        rig.host->click(rig.w * 0.2f, rig.h * 0.75f);
        CHECK(rig.demo.selection.path() == "boost.compressor");
        rig.host->click(rig.w * 0.6f, rig.h * 0.62f);
        CHECK(rig.demo.selection.path() == "boost.shaft");
        golden("air.select", *rig.host);
        AirRig na("na4-itb");
        na.host->click(na.w * 0.2f, na.h * 0.75f);   // no map on a natural engine: nothing there to select
        CHECK(na.demo.selection.empty());
        return 0;
    }

    // Nothing live: every number a dash, no point and no trail on the map; nothing invented, nothing crashes.
    VIEW_TEST(testAirAbsent, "air.absent")
    {
        AirRig rig("tt-v8");
        rig.host.reset();
        rig.ws.reset();
        for (const char* n : { "boost.pressure", "engine.manifold", "engine.throttle", "boost.target", "boost.shaftSpeed[0]",
                               "boost.shaftSpeed[1]", "boost.ratio[0]", "boost.ratio[1]", "boost.flow[0]", "boost.flow[1]",
                               "boost.compressorTemperature[0]", "boost.compressorTemperature[1]", "boost.gate[0]",
                               "boost.vanes[0]", "boost.intercoolerTemperature", "boost.blowOff", "intake.boxPressure",
                               "intake.plenumTemperature" })
            rig.demo.engine.forget(n);
        rig.ws = tee::ui::makeAirWorkspace(rig.ctx);
        rig.host = std::make_unique<funkgui::HeadlessHost>(*rig.ws, 0, 2.0f);
        rig.host->resize(rig.w, rig.h);
        rig.ws->place({ 0.0f, 0.0f, static_cast<float>(rig.w), static_cast<float>(rig.h) });
        rig.host->tick(30);
        golden("air.absent", *rig.host);
        return 0;
    }

    // At rest nothing moves and the key holds; a new operating point moves it.
    VIEW_TEST(testAirRest, "air.rest")
    {
        AirRig rig("tt-v8");
        CHECK(!rig.ws->moving());
        const std::uint64_t k = rig.ws->drawKey();
        rig.host->tick(30);
        CHECK(rig.ws->drawKey() == k && !rig.ws->moving());
        rig.demo.engine.setTime(5.2);
        air::airDemoLive(rig.demo.engine, rig.doc);
        rig.host->tick(1);
        CHECK(rig.ws->drawKey() != k);
        return 0;
    }

    VIEW_TEST(testAirSizes, "air.sizes")
    {
        AirRig big("turbo4", 1600, 1000, 2.0f);
        golden("air.large", *big.host);
        AirRig big1("turbo4", 1600, 1000, 1.0f);
        golden("air.large.dpi1", *big1.host);
        AirRig paper("tt-v8", 1100, 700, 2.0f, 1);
        golden("air.paper", *paper.host);
        AirRig paper1("tt-v8", 1100, 700, 1.0f, 1);
        golden("air.paper.dpi1", *paper1.host);
        return 0;
    }
}

namespace
{
    // Each part's inspector page holds its fields; a part's page has none of another's.
    VIEW_TEST(testAirInspector, "air.inspector")
    {
        AirRig rig("turbo4");
        CHECK(rig.ws->inspectorPage().empty());
        const struct
        {
            const char* id;
            const char* has;
            const char* hasNot;
        } cases[] = {
            { "boost.turbo[0]", "boost.compInducer", "boost.icEffect" },
            { "boost.intercooler", "boost.icEffect", "boost.compAR" },
            { "intake.runners", "intake.runnerLength", "boost.kind" },
            { "boost.gate", "boost.gateSpring", "boost.blowOffKind" },
            { "boost.nitrous", "boost.nitrousShot", "boost.antiLag" },
            { "boost.shaft", "boost.maxSpeed", "boost.compAR" },
        };
        for (const auto& k : cases)
        {
            rig.demo.selection.select(k.id);
            const std::string page = rig.ws->inspectorPage();
            CHECK_MSG(page.find(k.has) != std::string::npos, std::string(k.id) + " lacks " + k.has);
            CHECK_MSG(page.find(k.hasNot) == std::string::npos, std::string(k.id) + " has " + k.hasNot);
        }
        return 0;
    }
}
