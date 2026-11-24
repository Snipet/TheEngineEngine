// tests/ui/views/AirTests.cpp: the AIR workspace (ui/views/air/): the charge path over several builds, the compressor's
// map made as the simulator makes it, the trail of a pull, what a press selects, absent values, rest; goldens and
// pictures at density 1 and 2, 1100x700 and 1600x1000, both themes. Every picture is the software rasteriser's.
#include "ViewKit.h"

#include "app/Look.h"
#include "views/air/AirDemo.h"
#include "views/air/AirModel.h"
#include "views/air/AirWorkspace.h"

#include <algorithm>
#include <array>
#include <cstring>

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
        for (const char* b : { "na4-itb", "turbo4", "tt-v8", "sc-v8", "seq6", "cmp6", "cf-v8", "var-v8" })
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
        for (const char* id : { "boost.compressor", "boost.shaft", "boost.gate", "boost.blowOff", "boost.antiLag", "boost.nitrous" })
        {
            bool found = false;
            for (const air::AirHit& h : air::airHits(*rig.ws))
                if (h.id == id)
                {
                    found = true;
                    rig.host->click(h.r.x + 6.0f, h.r.y + 6.0f);
                    CHECK_MSG(rig.demo.selection.path() == id, std::string(id) + " selected " + rig.demo.selection.path());
                }
            CHECK_MSG(found, id);
        }
        rig.host->click(rig.w * 0.2f, rig.h * 0.75f);
        CHECK(rig.demo.selection.path() == "boost.compressor");
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
        // the demo's held values are numbered by their place: a name forgotten moves the rest, so the handles the
        // context keeps are asked afresh (else the picture shows other names' values)
        rig.ctx.live.reset();
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
        // every page reads cleanly in U1's demo with this package's hook
        StageRig hooked(tee::ui::makeAirWorkspace, 1100, 700, 1.0f);
        for (const char* id : { "intake.airbox", "boost.turbo[0]", "boost.compressor", "boost.supercharger", "boost.intercooler",
                                "intake.throttle", "intake.plenum", "intake.runners", "boost.shaft", "boost.gate",
                                "boost.blowOff", "boost.antiLag", "boost.nitrous" })
        {
            hooked.demo.selection.select(id);
            const std::string notes = inspectorNotes(*hooked.ws, hooked.demo);
            CHECK_MSG(notes.empty(), std::string(id) + ": " + notes);
        }
        return 0;
    }
}

namespace tee::test::u3
{
    funkgui::Rect frameStage(int w, int h);   // ExhaustTests.cpp
    std::string textCollisions(const funkgui::PrimList& frame);
}

namespace
{
    // At the frame's own stage sizes: every part inside the stage, no two parts or panels overlapping.
    VIEW_TEST(testAirFrameStage, "air.framestage")
    {
        for (const auto& win : { std::pair<int, int>{ 1100, 700 }, std::pair<int, int>{ 1600, 1000 } })
        {
            const funkgui::Rect st = tee::test::u3::frameStage(win.first, win.second);
            const int w = static_cast<int>(st.w), h = static_cast<int>(st.h);
            for (const char* b : { "na4-itb", "turbo4", "tt-v8", "sc-v8", "seq6", "cmp6", "cf-v8", "var-v8" })
            {
                AirRig rig(b, w, h, 1.0f);
                const std::vector<air::AirHit> hits = air::airHits(*rig.ws);
                CHECK(!hits.empty());
                for (std::size_t i = 0; i < hits.size(); ++i)
                {
                    const funkgui::Rect& r = hits[i].r;
                    CHECK_MSG(r.x >= 0.0f && r.right() <= static_cast<float>(w) && r.y >= 0.0f && r.bottom() <= static_cast<float>(h),
                              std::string(b) + " " + hits[i].id + " outside at " + std::to_string(win.first));
                    for (std::size_t j = i + 1; j < hits.size(); ++j)
                    {
                        const funkgui::Rect& q = hits[j].r;
                        // a panel holds its valves; the parts of the row and the panels must not meet
                        const bool inside = (q.x >= r.x && q.right() <= r.right() && q.y >= r.y && q.bottom() <= r.bottom())
                                         || (r.x >= q.x && r.right() <= q.right() && r.y >= q.y && r.bottom() <= q.bottom());
                        const bool meet = r.x < q.right() && q.x < r.right() && r.y < q.bottom() && q.y < r.bottom();
                        CHECK_MSG(inside || !meet, std::string(b) + " " + hits[i].id + " meets " + hits[j].id);
                    }
                }
            }
            const std::string tag = win.first == 1100 ? "small" : "large";
            AirRig two("tt-v8", w, h, 2.0f);
            golden("air.frame." + tag, *two.host);
            AirRig one("turbo4", w, h, 1.0f);
            golden("air.frame." + tag + ".dpi1", *one.host);
        }
        return 0;
    }
}

namespace
{
    VIEW_TEST(testAirText, "air.text")
    {
        for (const auto& win : { std::pair<int, int>{ 1100, 700 }, std::pair<int, int>{ 1600, 1000 } })
        {
            const funkgui::Rect st = tee::test::u3::frameStage(win.first, win.second);
            for (const char* b : { "na4-itb", "turbo4", "tt-v8", "sc-v8", "seq6", "cmp6", "cf-v8", "var-v8" })
            {
                AirRig rig(b, static_cast<int>(st.w), static_cast<int>(st.h), 1.0f);
                const std::string hits = tee::test::u3::textCollisions(rig.host->draw());
                CHECK_MSG(hits.empty(), std::string(b) + " at " + std::to_string(win.first) + ":\n" + hits);
            }
        }
        return 0;
    }
}

namespace
{
    // The collision check itself catches two texts drawn over each other, and passes two drawn apart.
    struct TwoTexts final : public funkgui::Panel
    {
        float dx = 0.0f;
        void attach(funkgui::HostServices&) override {}
        void tick(float) override {}
        bool wantsFullRate() const override { return false; }
        void accessibility(std::vector<funkgui::A11yItem>&) const override {}
        uint32_t a11yRevision() const override { return 0; }
        void a11yAction(uint32_t, funkgui::A11yAction, double) override {}
        void closeGestures() override {}
        int width() const override { return 300; }
        int height() const override { return 100; }
        void draw(funkgui::Canvas& c, const funkgui::Theme& th) override
        {
            c.text("THROTTLE", 20.0f, 20.0f, tee::ui::look::kCaps, th.ink70);
            c.text("121 C", 40.0f + dx, 22.0f, tee::ui::look::kUnit, th.ink70);
        }
    };
    VIEW_TEST(testTextCheck, "air.textcheck")
    {
        TwoTexts t;
        funkgui::HeadlessHost host(t, 0, 1.0f);
        CHECK(!tee::test::u3::textCollisions(host.draw()).empty());
        t.dx = 120.0f;
        CHECK(tee::test::u3::textCollisions(host.draw()).empty());
        AirRig seq("seq6", 700, 574, 1.0f);
        picture("air.text.seq6", *seq.host);
        return 0;
    }
}

namespace
{
    // The Scope's map, as card F's simulator sends it, is drawn in place of the mirror of its arithmetic: here a map
    // made from other sizes (a larger wheel), so the picture must differ from the mirror's; its trail is the
    // simulator's ring (newest at boostTrailAt - 1). Without the arrays, the mirror again.
    // Card N2's mismatch: the cooler, the gate's spring, the charge pipes and the compressor's size, each an amber outline on its part and a
    // line of words under the row (no bits: nothing); every text clear of every other at the frame's narrowest stage.
    VIEW_TEST(testAirMismatch, "air.mismatch")
    {
        const funkgui::Rect st = tee::test::u3::frameStage(1100, 700);
        AirRig rig("turbo4", static_cast<int>(st.w), static_cast<int>(st.h), 2.0f);
        const std::uint64_t k = rig.ws->drawKey();
        rig.demo.engine.set("boost.mismatch", 15.0);
        rig.demo.engine.set("boost.compressorHas", 1.94);
        rig.demo.engine.set("boost.compressorWants", 0.03);
        rig.demo.engine.set("boost.coolerHas", 0.25);
        rig.demo.engine.set("boost.coolerWants", 0.68);
        rig.demo.engine.set("boost.springHas", 10e3);
        rig.demo.engine.set("boost.springWants", 120e3);
        rig.demo.engine.set("boost.volumeHas", 0.005);
        rig.demo.engine.set("boost.volumeWants", 0.037);
        rig.host->tick(2);
        CHECK(rig.ws->drawKey() != k);
        const std::string hits = tee::test::u3::textCollisions(rig.host->draw());
        CHECK_MSG(hits.empty(), hits);
        golden("air.mismatch", *rig.host);
        // the bits cleared: the row as it was
        rig.demo.engine.set("boost.mismatch", 0.0);
        rig.host->tick(2);
        CHECK(rig.ws->drawKey() == k);
        // the values the stage shows are the demo's, not disturbed by the warnings' names (held from the start)
        CHECK(std::fabs(rig.ctx.live.get("boost.coolerHas") - 0.25) < 1e-9);
        return 0;
    }

    VIEW_TEST(testAirScopeMap, "air.scopemap")
    {
        AirRig rig("turbo4", 696, 574, 2.0f);
        const std::vector<funkgui::Prim> mirror = rig.host->draw().prims;
        air::CompressorMap other;
        other.size(0.056f, 0.076f, 0.7f, 0.78f);
        const air::MapLines m = air::mapLines(other);
        auto& e = rig.demo.engine;
        e.addScope("boostMapRev", [](const tee::ui::DemoEngine&, float* out, int max) { return max >= 1 ? (out[0] = 3.0f, 1) : 0; });
        e.addScope("boostLineSpeed", [m](const tee::ui::DemoEngine&, float* out, int max) {
            for (int l = 0; l < 8 && l < max; ++l)
                out[l] = m.speed[l];
            return std::min(max, 8);
        });
        const auto grid = [](const std::array<std::array<float, 16>, 8>& g) {
            return [g](const tee::ui::DemoEngine&, float* out, int max) {
                for (int i = 0; i < 128 && i < max; ++i)
                    out[i] = g[static_cast<std::size_t>(i / 16)][static_cast<std::size_t>(i % 16)];
                return std::min(max, 128);
            };
        };
        e.addScope("boostLineFlow", grid(m.flow));
        e.addScope("boostLineRatio", grid(m.pr));
        e.addScope("boostContourEta", [m](const tee::ui::DemoEngine&, float* out, int max) {
            for (int v = 0; v < 5 && v < max; ++v)
                out[v] = m.islandEta[v];
            return std::min(max, 5);
        });
        // a contour's 16: the mirror's rings sampled at the simulator's eight speeds (left up, right down)
        const auto contour = [m](bool ratio) {
            return [m, ratio](const tee::ui::DemoEngine&, float* out, int max) {
                constexpr int kS = air::MapLines::kIslandSteps;
                for (int v = 0; v < 5; ++v)
                    for (int j = 0; j < 16; ++j)
                    {
                        const int l = j < 8 ? j : 15 - j;
                        const int step = l * (kS - 1) / 7;
                        const int slot = j < 8 ? step : 2 * kS - 1 - step;
                        const auto& src = ratio ? m.islandPr : m.islandFlow;
                        if (v * 16 + j < max)
                            out[v * 16 + j] = src[static_cast<std::size_t>(v)][static_cast<std::size_t>(slot)];
                    }
                return std::min(max, 80);
            };
        };
        e.addScope("boostContourFlow", contour(false));
        e.addScope("boostContourRatio", contour(true));
        e.addScope("boostTrailAt", [](const tee::ui::DemoEngine&, float* out, int max) { return max >= 1 ? (out[0] = 40.0f, 1) : 0; });
        e.addScope("boostTrailFlow", [](const tee::ui::DemoEngine&, float* out, int max) {
            for (int i = 0; i < 256 && i < max; ++i)
                out[i] = i < 40 ? 0.02f + 0.006f * static_cast<float>(i) : 0.0f;
            return std::min(max, 256);
        });
        e.addScope("boostTrailRatio", [](const tee::ui::DemoEngine&, float* out, int max) {
            for (int i = 0; i < 256 && i < max; ++i)
                out[i] = i < 40 ? 1.0f + 0.035f * static_cast<float>(i) : 0.0f;
            return std::min(max, 256);
        });
        rig.host->tick(4);
        const funkgui::PrimList& fromScope = rig.host->draw();
        CHECK(fromScope.prims.size() != mirror.size()
              || std::memcmp(fromScope.prims.data(), mirror.data(), mirror.size() * sizeof(funkgui::Prim)) != 0);
        golden("air.scopemap", *rig.host);
        AirRig one("turbo4", 696, 574, 1.0f);
        one.demo.engine.addScope("boostMapRev", [](const tee::ui::DemoEngine&, float* out, int max) { return max >= 1 ? (out[0] = 0.0f, 1) : 0; });
        one.host->tick(4);
        golden("air.scopemap.none.dpi1", *one.host);   // a revision of 0: no map from the simulator yet, the mirror's
        return 0;
    }
}

namespace
{
    // Where the runners are tuned: card E's measured bumps on a 2.0 L four (main 7640 rpm at 0.40 m, 5250 at 0.60;
    // lower 3180 and 2300) from the loaded-pipe relation, within 10 %; longer runners tune lower.
    VIEW_TEST(testAirRam, "air.ram")
    {
        tee::ui::MemoryDoc doc{ air::airDemoFields() };
        CHECK(air::setAirDemoBuild(doc, "na4-itb"));
        const int c = doc.find("engine.compression");
        if (c >= 0)
            doc.set(c, 10.5);   // the inline four's (sim/core/layouts.fm)
        const air::AirBuild b = air::resolveAir(doc);
        const air::RamTune a = air::ramTune(b, 0.40), z = air::ramTune(b, 0.60);
        CHECK_MSG(std::fabs(a.mainRpm / 7640.0 - 1.0) < 0.10, std::to_string(a.mainRpm) + " rpm at 0.40 m");
        CHECK_MSG(std::fabs(z.mainRpm / 5250.0 - 1.0) < 0.10, std::to_string(z.mainRpm) + " rpm at 0.60 m");
        CHECK_MSG(std::fabs(a.lowerRpm / 3180.0 - 1.0) < 0.10, std::to_string(a.lowerRpm) + " rpm, lower, at 0.40 m");
        CHECK(z.mainRpm < a.mainRpm && z.hz < a.hz && air::ramTune(b, 0.0).mainRpm == 0.0);
        // a drag of the length moves the stage's number: the picture of a variable manifold
        AirRig rig("var-v8", 696, 574, 1.0f);
        golden("air.ram.var-v8.dpi1", *rig.host);
        return 0;
    }
}
