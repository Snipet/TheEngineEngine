// tests/ui/views/ExhaustTests.cpp: the EXHAUST workspace (ui/views/exhaust/): the system as built over several builds,
// what a press selects, what an absent value draws, the stage at rest, the slow switch; goldens and pictures at density
// 1 and 2, at 1100x700 and 1600x1000, in both themes. Every picture is the software rasteriser's.
#include "ViewKit.h"

#include "app/Look.h"
#include "views/exhaust/ExhaustDemo.h"
#include "views/exhaust/ExhaustModel.h"
#include "views/exhaust/ExhaustStage.h"
#include "views/exhaust/ExhaustWorkspace.h"

#include <funkgui/core/Theme.h>
#include <funkgui/text/FontService.h>

#include "views/air/AirDemo.h"
#include "views/air/AirWorkspace.h"

#include <algorithm>
#include <cstdio>

namespace
{
    using namespace tee::test::views;
    namespace ex = tee::ui::exhaust;

    // The demo with the exhaust builder's fields (ExhaustDemo.h) and one of its builds; the workspace alone in a
    // headless host, its stage the whole window.
    struct ExRig
    {
        tee::ui::DemoRig demo;
        tee::ui::MemoryDoc doc{ ex::exhaustDemoFields() };
        tee::ui::WorkspaceContext ctx{ doc, demo.engine, demo.engine, demo.engine, demo.units, demo.selection, demo.app };
        std::unique_ptr<tee::ui::Workspace> ws;
        std::unique_ptr<funkgui::HeadlessHost> host;
        int w, h;

        ExRig(const char* build, int width = 1100, int height = 700, float dpi = 2.0f, double t = 4.6, int theme = 0)
            : demo(t), w(width), h(height)
        {
            if (build != nullptr)
                ex::setExhaustDemoBuild(doc, build);
            ex::exhaustDemoLive(demo.engine);
            ws = tee::ui::makeExhaustWorkspace(ctx);
            host = std::make_unique<funkgui::HeadlessHost>(*ws, theme, dpi);
            host->resize(w, h);
            ws->place({ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) });
            host->tick(90);
        }
        // The geometry the workspace lays out (the same arithmetic, from the same build and stage).
        ex::Geo geo() const
        {
            const float W = static_cast<float>(w), H = static_cast<float>(h);
            return ex::layoutExhaust(ex::resolveExhaust(doc), { 20.0f, 84.0f, W - 40.0f, H - 84.0f - 74.0f });
        }
        const ex::Hit* hit(const std::string& id) const
        {
            static ex::Geo g;
            g = geo();
            for (const ex::Hit& x : g.hits)
                if (x.id == id)
                {
                    static ex::Hit keep;
                    keep = x;
                    return &keep;
                }
            return nullptr;
        }
    };

    // The model resolves each build as the simulator's builder would (sim/exhaust/slots.fm resolve()).
    VIEW_TEST(testExhaustModel, "exhaust.model")
    {
        tee::ui::MemoryDoc doc{ ex::exhaustDemoFields() };
        CHECK(ex::setExhaustDemoBuild(doc, "na4-421"));
        ex::ExhaustBuild b = ex::resolveExhaust(doc);
        CHECK(b.layout == 5 && b.ports == 4 && b.sides == 1 && b.anyMerge() && b.turbos == ex::Turbos::none);
        CHECK(b.portTo[0] == 0 && b.portTo[1] == 1 && b.portTo[2] == 1 && b.portTo[3] == 0);
        CHECK(b.colOn[2] && b.colFed[2] && b.colTo[0] == 4 && b.colTo[2] == 0 && b.res[0] == 1 && b.muf[0] == 2);

        CHECK(ex::setExhaustDemoBuild(doc, "tt-v8-x"));
        b = ex::resolveExhaust(doc);
        CHECK(b.layout == 12 && b.ports == 8 && b.sides == 2 && b.cross == 2 && b.turbos == ex::Turbos::perBank);
        CHECK(b.portTo[0] == 0 && b.portTo[1] == 1 && b.colTo[0] == 0 && b.colTo[1] == 1);   // a collector a bank
        CHECK(b.tips[0] == 2 && b.byp[1] == 3 && b.conv[0] == 2 && b.tag[0] == ex::Tag::rearLeft);

        CHECK(ex::setExhaustDemoBuild(doc, "sc-v8-180"));
        b = ex::resolveExhaust(doc);
        CHECK(b.supercharger && b.turbos == ex::Turbos::none && b.cross == 1 && b.crossAt == 1 && b.muf[1] == 3);
        CHECK(b.portTo[1] == 1 && b.portTo[2] == 1 && b.bank[2] == 0);   // cylinder 3 (left bank) to the right collector
        CHECK(b.primLen[2] > b.primLen[0]);                               // the crossing primaries are longer

        CHECK(ex::setExhaustDemoBuild(doc, "single-stack"));
        b = ex::resolveExhaust(doc);
        CHECK(b.ports == 1 && b.openStacks() && !b.sideOn[0] && !b.sideOn[1]);

        CHECK(ex::setExhaustDemoBuild(doc, "flat4"));
        b = ex::resolveExhaust(doc);
        CHECK(b.ports == 4 && b.bank[1] == 1 && b.primLen[1] > b.primLen[0] * 1.5f);   // the boxer's unequal lengths

        // the demo's own Spec (no builder fields): the layout by its label, the crossover by its switch
        tee::ui::MemoryDoc plain{ tee::ui::demoFields() };
        b = ex::resolveExhaust(plain);
        CHECK(b.layout == 12 && b.sides == 2 && b.cross == 2 && b.turbos == ex::Turbos::one);
        CHECK(ex::layoutFromLabel("I4") == 5 && ex::layoutFromLabel("V8 flat-plane") == 13 && ex::layoutFromLabel("x") == -1);
        return 0;
    }

    // The stage over U1's demo alone (its made-up Spec, no builder fields): the default system, density 2 and 1.
    VIEW_TEST(testExhaustStage, "exhaust.stage")
    {
        StageRig rig(tee::ui::makeExhaustWorkspace, 1100, 700, 2.0f);
        ex::exhaustDemoLive(rig.demo.engine);
        rig.settle();
        golden("exhaust.stage", *rig.host);
        StageRig one(tee::ui::makeExhaustWorkspace, 1100, 700, 1.0f);
        ex::exhaustDemoLive(one.demo.engine);
        one.settle();
        golden("exhaust.stage.dpi1", *one.host);
        return 0;
    }

    // The four builds the card names, and two more, each its own picture and golden (density 2; the first also at 1).
    VIEW_TEST(testExhaustBuilds, "exhaust.builds")
    {
        const char* const builds[] = { "na4-421", "tt-v8-x", "sc-v8-180", "single-stack", "v12-log", "flat4" };
        for (const char* b : builds)
        {
            ExRig rig(b);
            golden(std::string("exhaust.build.") + b, *rig.host);
        }
        ExRig one("na4-421", 1100, 700, 1.0f);
        golden("exhaust.build.na4-421.dpi1", *one.host);
        return 0;
    }

    // A press on each element selects its prefix; the selected one is ringed and named; a press on nothing keeps it.
    VIEW_TEST(testExhaustSelect, "exhaust.select")
    {
        ExRig rig("tt-v8-x");
        const char* const ids[] = { "exhaust.primary[0]", "exhaust.collector[1]", "boost", "exhaust.converter[0]",
                                    "exhaust.main[1]",    "exhaust.crossover",    "exhaust.muffler[0]",
                                    "exhaust.bypass[1]",  "exhaust.tail[0]",      "exhaust.primary[7]" };
        for (const char* id : ids)
        {
            const ex::Hit* h = rig.hit(id);
            CHECK_MSG(h != nullptr, id);
            if (h == nullptr)
                continue;
            rig.host->click(h->r.centreX(), h->r.centreY());
            CHECK_MSG(rig.demo.selection.path() == id, std::string(id) + " selected " + rig.demo.selection.path());
        }
        rig.host->click(rig.w * 0.5f, 60.0f);   // the header: nothing
        CHECK(rig.demo.selection.path() == "exhaust.primary[7]");
        const ex::Hit* m = rig.hit("exhaust.muffler[1]");
        rig.host->click(m->r.centreX(), m->r.centreY());
        CHECK(rig.demo.selection.is("exhaust.muffler") && rig.demo.selection.is("exhaust.muffler[1]"));
        rig.host->move(rig.hit("exhaust.converter[1]")->r.centreX(), rig.hit("exhaust.converter[1]")->r.centreY());
        CHECK(rig.demo.selection.hovered() == "exhaust.converter[1]");
        rig.host->tick(20);
        golden("exhaust.select", *rig.host);

        // the 4-2-1's merge and the single's stack
        ExRig four("na4-421");
        const ex::Hit* merge = four.hit("exhaust.collector[2]");
        CHECK(merge != nullptr && merge->name == "MERGE 3");
        ExRig single("single-stack");
        const ex::Hit* stack = single.hit("exhaust.primary[0]");
        CHECK(stack != nullptr);
        single.host->click(stack->r.right() - 4.0f, stack->r.centreY());
        CHECK(single.demo.selection.path() == "exhaust.primary[0]");
        CHECK(single.hit("exhaust.tail[0]") == nullptr);   // no chain on an open stack
        return 0;
    }

    // Nothing live: the walls neutral, every number a dash, no trace; nothing crashes and nothing is invented.
    VIEW_TEST(testExhaustAbsent, "exhaust.absent")
    {
        ExRig rig("tt-v8-x");
        for (const char* n : { "exhaust.portTemperature", "exhaust.collectorTemperature", "exhaust.tailTemperature",
                               "exhaust.wallPrimaries", "exhaust.wallCollectors", "exhaust.wallMerges", "exhaust.wallMain",
                               "exhaust.wallLink", "exhaust.wallMufflers", "exhaust.wallTails", "exhaust.tailLevel[0]",
                               "exhaust.tailLevel[1]", "exhaust.valve[0]", "exhaust.valve[1]" })
            rig.demo.engine.forget(n);
        rig.host->tick(5);
        golden("exhaust.absent", *rig.host);
        return 0;
    }

    // At rest the stage asks for nothing: not moving, its key the same frame after frame. A new Scope copy moves it.
    VIEW_TEST(testExhaustRest, "exhaust.rest")
    {
        ExRig rig("sc-v8-180");
        CHECK(!rig.ws->moving());
        const std::uint64_t k = rig.ws->drawKey();
        rig.host->tick(30);
        CHECK(rig.ws->drawKey() == k && !rig.ws->moving());
        rig.demo.engine.tick(1.0f / 60.0f);    // the demo's script on: a new Scope copy
        rig.host->tick(1);
        CHECK(rig.ws->drawKey() != k);
        return 0;
    }

    // SLOW: the copies replayed at an eighth of their pace, so the stage keeps moving while it catches up.
    VIEW_TEST(testExhaustSlow, "exhaust.slow")
    {
        ExRig rig("tt-v8-x");
        rig.host->click(rig.w - 16.0f - 32.0f, 25.0f);   // the SLOW button
        for (int i = 0; i < 12; ++i)
        {
            rig.demo.engine.tick(1.0f / 60.0f);
            rig.host->tick(1);
        }
        CHECK(rig.ws->moving());
        const std::uint64_t a = rig.ws->drawKey();
        rig.host->tick(1);
        CHECK(rig.ws->drawKey() != a);         // still replaying, with no new copy
        picture("exhaust.slow", *rig.host);
        rig.host->click(rig.w - 16.0f - 32.0f, 25.0f);   // off: the newest at once
        rig.host->tick(30);
        CHECK(!rig.ws->moving());
        return 0;
    }

    // The large window and the paper theme, both densities.
    VIEW_TEST(testExhaustSizes, "exhaust.sizes")
    {
        ExRig big("tt-v8-x", 1600, 1000, 2.0f);
        golden("exhaust.large", *big.host);
        ExRig big1("tt-v8-x", 1600, 1000, 1.0f);
        golden("exhaust.large.dpi1", *big1.host);
        ExRig paper("sc-v8-180", 1100, 700, 2.0f, 4.6, 1);
        golden("exhaust.paper", *paper.host);
        ExRig paper1("sc-v8-180", 1100, 700, 1.0f, 4.6, 1);
        golden("exhaust.paper.dpi1", *paper1.host);
        return 0;
    }
}

namespace
{
    // Each element's inspector page holds its own fields (the side's, the port's), and only those the build has.
    VIEW_TEST(testExhaustInspector, "exhaust.inspector")
    {
        ExRig rig("tt-v8-x");
        CHECK(rig.ws->inspectorPage().empty());          // nothing selected: the frame's section pages
        const struct
        {
            const char* id;
            const char* has;
            const char* hasNot;
        } cases[] = {
            { "exhaust.muffler[1]", "exhaust.mufLen[1]", "exhaust.mufLen[0]" },
            { "exhaust.primary[3]", "exhaust.primLen[3]", "exhaust.primLen[2]" },
            { "exhaust.collector[1]", "exhaust.colVol[1]", "exhaust.tailLen[0]" },
            { "exhaust.crossover", "exhaust.crossAt", "exhaust.muffler[0]" },
            { "exhaust.bypass[0]", "exhaust.bypassRpm", "exhaust.bypass[1]" },
            { "exhaust.tail[1]", "exhaust.exitAt[1]", "exhaust.exitAt[0]" },
            { "boost", "boost.kind", "exhaust.sides" },
        };
        for (const auto& k : cases)
        {
            const tee::ui::exhaust::Hit* h = rig.hit(k.id);
            CHECK_MSG(h != nullptr, k.id);
            if (h == nullptr)
                continue;
            rig.host->click(h->r.centreX(), h->r.centreY());
            const std::string page = rig.ws->inspectorPage();
            CHECK_MSG(page.find(k.has) != std::string::npos, std::string(k.id) + " lacks " + k.has);
            CHECK_MSG(page.find(k.hasNot) == std::string::npos, std::string(k.id) + " has " + k.hasNot);
        }
        // U1's demo Spec has none of the builder's fields: a muffler's page is the demo's own dial, or nothing
        StageRig plain(tee::ui::makeExhaustWorkspace, 1100, 700, 1.0f);
        plain.demo.selection.select("exhaust.tail[0]");
        CHECK(plain.ws->inspectorPage().empty());
        return 0;
    }
}

namespace
{
    // U3's contact sheet, out/ui/sheet.u3.png: a row a workspace (AIR, then EXHAUST and a second EXHAUST build), each at
    // 1100 x 700 and 1600 x 1000, density 1 then 2, scaled into tiles of the same size. The software rasteriser's.
    VIEW_TEST(testU3Sheet, "exhaust.sheet")
    {
        constexpr int kTileW = 560, kTileH = 360, kGap = 16, kCols = 4, kRows = 3;
        funkgui::Image out;
        out.w = kCols * kTileW + (kCols + 1) * kGap;
        out.h = kRows * kTileH + (kRows + 1) * kGap;
        out.rgba.assign(static_cast<std::size_t>(out.w) * static_cast<std::size_t>(out.h) * 4, 8);
        for (std::size_t i = 3; i < out.rgba.size(); i += 4)
            out.rgba[i] = 255;
        const int sizes[2][2] = { { 1100, 700 }, { 1600, 1000 } };
        const float dpis[2] = { 1.0f, 2.0f };
        for (int row = 0; row < kRows; ++row)
            for (int col = 0; col < kCols; ++col)
            {
                const int w = sizes[col / 2][0], h = sizes[col / 2][1];
                const float dpi = dpis[col % 2];
                std::unique_ptr<ExRig> ex;
                tee::ui::DemoRig demo(2.0);
                tee::ui::MemoryDoc doc{ tee::ui::air::airDemoFields() };
                tee::ui::WorkspaceContext ctx{ doc, demo.engine, demo.engine, demo.engine, demo.units, demo.selection, demo.app };
                std::unique_ptr<tee::ui::Workspace> ws;
                std::unique_ptr<funkgui::HeadlessHost> host;
                funkgui::HeadlessHost* shown = nullptr;
                if (row == 0)
                {
                    tee::ui::air::setAirDemoBuild(doc, "tt-v8");
                    ws = tee::ui::makeAirWorkspace(ctx);
                    host = std::make_unique<funkgui::HeadlessHost>(*ws, 0, dpi);
                    host->resize(w, h);
                    ws->place({ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) });
                    for (double t = 2.0; t <= 4.6; t += 0.05)
                    {
                        demo.engine.setTime(t);
                        tee::ui::air::airDemoLive(demo.engine, doc);
                        host->tick(1);
                    }
                    host->tick(60);
                    shown = host.get();
                }
                else
                {
                    ex = std::make_unique<ExRig>(row == 1 ? "tt-v8-x" : "sc-v8-180", w, h, dpi);
                    shown = ex->host.get();
                }
                const funkgui::Image img = funkgui::rasterise(shown->draw(), funkgui::FontService::get().atlas(), 1);
                const int ox = kGap + col * (kTileW + kGap), oy = kGap + row * (kTileH + kGap);
                const double sc = std::max(static_cast<double>(img.w) / kTileW, static_cast<double>(img.h) / kTileH);
                for (int y = 0; y < kTileH; ++y)
                    for (int x = 0; x < kTileW; ++x)
                    {
                        const int x0 = static_cast<int>(x * sc), x1 = std::max(x0 + 1, static_cast<int>((x + 1) * sc));
                        const int y0 = static_cast<int>(y * sc), y1 = std::max(y0 + 1, static_cast<int>((y + 1) * sc));
                        if (x0 >= img.w || y0 >= img.h)
                            continue;
                        unsigned sum[3] = { 0, 0, 0 }, n = 0;
                        for (int yy = y0; yy < y1 && yy < img.h; ++yy)
                            for (int xx = x0; xx < x1 && xx < img.w; ++xx)
                            {
                                const std::size_t k = (static_cast<std::size_t>(yy) * static_cast<std::size_t>(img.w) + static_cast<std::size_t>(xx)) * 4;
                                for (int ch = 0; ch < 3; ++ch)
                                    sum[ch] += img.rgba[k + static_cast<std::size_t>(ch)];
                                ++n;
                            }
                        const std::size_t d = (static_cast<std::size_t>(oy + y) * static_cast<std::size_t>(out.w) + static_cast<std::size_t>(ox + x)) * 4;
                        for (int ch = 0; ch < 3; ++ch)
                            out.rgba[d + static_cast<std::size_t>(ch)] = static_cast<uint8_t>(n > 0 ? sum[ch] / n : 0);
                    }
                host.reset();
                ws.reset();
            }
        const std::string path = (outDir() / "sheet.u3.png").string();
        CHECK_MSG(tee::test::writePng(out, path), path);
        std::printf("  contact sheet: %s (software rasteriser; rows AIR, EXHAUST twin-turbo V8, EXHAUST supercharged V8 180;"
                    " columns 1100x700 at density 1 and 2, 1600x1000 at 1 and 2)\n", path.c_str());
        return 0;
    }
}
