// tests/ui/views/DynoTests.cpp: the DYNO workspace (ui/views/dyno/) over the demo engine (ui/app/Demo.h): its stage at
// both densities, both themes and two sizes; a run started, watched, landed, renamed, hidden and copied as CSV; the
// cursor reading the traces; a quantity the runs did not record drawn as a dash, not invented.
#include "ViewKit.h"

#include "views/car/Car.h"
#include "views/car/CarDemo.h"
#include "views/tune/Tune.h"
#include "views/tune/TuneDemo.h"

#include <funkgui/core/Theme.h>
#include <funkgui/text/FontService.h>

#include <algorithm>

namespace tee::ui
{
    std::unique_ptr<Workspace> makeDynoWorkspace(WorkspaceContext& c);
}

namespace
{
    using namespace tee::test::views;

    // The same rig on another theme (index 1: paper).
    void retheme(StageRig& rig, int theme, int w, int h, float dpi)
    {
        rig.host = std::make_unique<funkgui::HeadlessHost>(*rig.ws, theme, dpi);
        rig.host->resize(w, h);
        rig.ws->place({ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) });
        rig.settle();
    }

    // An edit made and the predicted curve answered, so "before last edit" is there to overlay.
    void editBoost(StageRig& rig, double pa)
    {
        auto& doc = rig.demo.doc;
        const int f = doc.find("boost.level");
        doc.beginGesture();
        doc.set(f, pa);
        doc.endGesture();
        for (int i = 0; i < 90; ++i)
        {
            rig.demo.engine.tick(1.0f / 60.0f);
            rig.host->tick(1);
        }
    }

    // A run started from the plan and ticked until it lands.
    void runOnce(StageRig& rig)
    {
        rig.host->click(1100.0f - 16.0f - 12.0f - 200.0f, 700.0f - 16.0f - 290.0f + 30.0f + 36.0f + 150.0f + 6.0f + 14.0f);
        for (int i = 0; i < 200 && rig.demo.engine.running(); ++i)
        {
            rig.demo.engine.tick(1.0f / 60.0f);
            rig.host->tick(1);
        }
        rig.settle();
    }

    // The stage over the demo: the predicted curve, the one before an edit, two runs, the cursor at 4500 rpm.
    VIEW_TEST(testDynoStage, "dyno.stage")
    {
        for (float dpi : { 2.0f, 1.0f })
        {
            StageRig rig(tee::ui::makeDynoWorkspace, 1100, 700, dpi);
            editBoost(rig, 110e3);
            CHECK(!rig.demo.engine.before().empty());
            runOnce(rig);
            CHECK(rig.demo.engine.runCount() == 1);
            editBoost(rig, 60e3);
            runOnce(rig);
            CHECK(rig.demo.engine.runCount() == 2);
            editBoost(rig, 85e3);                        // the build edited since: predicted and before differ from the runs
            rig.host->move(560.0f, 300.0f);
            rig.settle();
            golden(dpi > 1.5f ? "dyno.stage" : "dyno.stage.dpi1", *rig.host);
        }
        return 0;
    }

    // At a large size, and on the paper theme.
    VIEW_TEST(testDynoLarge, "dyno.large")
    {
        for (float dpi : { 2.0f, 1.0f })
        {
            StageRig rig(tee::ui::makeDynoWorkspace, 1600, 1000, dpi);
            editBoost(rig, 120e3);
            rig.host->move(800.0f, 420.0f);
            rig.settle();
            golden(dpi > 1.5f ? "dyno.large" : "dyno.large.dpi1", *rig.host);
        }
        StageRig paper(tee::ui::makeDynoWorkspace, 1100, 700, 2.0f);
        editBoost(paper, 120e3);
        retheme(paper, 1, 1100, 700, 2.0f);
        golden("dyno.paper", *paper.host);
        return 0;
    }

    // A run: started from the plan, in progress (the words and the swept span), landed as a row chosen at once, renamed,
    // hidden by its box, copied as CSV. Stop ends one early.
    VIEW_TEST(testDynoRun, "dyno.run")
    {
        StageRig rig(tee::ui::makeDynoWorkspace, 1100, 700, 1.0f);
        CHECK(!rig.ws->moving());
        const std::uint64_t still = rig.ws->drawKey();
        rig.settle();
        CHECK(rig.ws->drawKey() == still);                   // at rest it asks for nothing
        // START RUN (the plan's button under the steppers).
        const float sx = 1100.0f - 16.0f - 12.0f - 200.0f, sy = 700.0f - 16.0f - 290.0f + 30.0f + 36.0f + 150.0f + 6.0f + 14.0f;
        rig.host->click(sx, sy);
        CHECK(rig.demo.engine.running());
        CHECK(rig.ws->moving());
        for (int i = 0; i < 30; ++i)
        {
            rig.demo.engine.tick(1.0f / 60.0f);
            rig.host->tick(1);
        }
        picture("dyno.running", *rig.host);
        for (int i = 0; i < 200 && rig.demo.engine.running(); ++i)
        {
            rig.demo.engine.tick(1.0f / 60.0f);
            rig.host->tick(1);
        }
        rig.settle();
        CHECK(rig.demo.engine.runCount() == 1);
        // The newest run is chosen; rename it through the NAME field.
        const float nameX = 1100.0f - 16.0f - 12.0f - 260.0f, nameY = 700.0f - 16.0f - 290.0f - 10.0f - 12.0f - 14.0f;
        rig.host->click(nameX, nameY);
        typeKeys(*rig.host, "B a s e space p u l l enter");
        rig.settle();
        picture("dyno.renamed", *rig.host);
        // Copy CSV.
        rig.host->click(1100.0f - 16.0f - 12.0f - 40.0f, nameY);
        const std::string csv = rig.host->clipboardText();
        CHECK_MSG(csv.rfind("speed_rad_s,rpm,torque_N_m,power_kW,boost.pressure\n", 0) == 0, csv.substr(0, 80));
        CHECK(std::count(csv.begin(), csv.end(), '\n') == 1 + static_cast<long>(rig.demo.engine.run(0).speed.size()));
        // Stop: a second run ended early lands nothing in the demo.
        rig.host->click(sx, sy);
        CHECK(rig.demo.engine.running());
        rig.host->click(1100.0f - 16.0f - 12.0f - 30.0f, sy);
        CHECK(!rig.demo.engine.running());
        return 0;
    }

    // A quantity the runs did not record: the band says so with a dash; nothing is invented. A dyno with no curve says
    // why instead of drawing axes over nothing.
    VIEW_TEST(testDynoAbsent, "dyno.absent")
    {
        StageRig rig(tee::ui::makeDynoWorkspace, 1100, 700, 1.0f);
        // LAMBDA: the second of the band's buttons after NONE and BOOST (right-aligned, 68 px each).
        const float bandY = 16.0f + 58.0f + (700.0f - 32.0f - 58.0f) - std::clamp((700.0f - 32.0f - 58.0f) * 0.26f, 96.0f, 170.0f) + 16.0f;
        const float plotRight = 16.0f + (1100.0f - 48.0f - std::clamp(1100.0f * 0.28f, 250.0f, 330.0f));
        rig.host->click(plotRight - 12.0f - 68.0f * 1.5f, bandY);
        rig.settle();
        picture("dyno.absent", *rig.host);
        return 0;
    }

    // The contact sheet of package U4: DYNO, TUNE and CAR a row each; across, each at 1100 x 700 and 1600 x 1000, at
    // density 1 then 2 (out/ui/sheet.u4.png). Every tile is the software rasteriser's, shrunk to half its logical size.
    VIEW_TEST(testU4Sheet, "dyno.sheet")
    {
        struct Col
        {
            int w, h;
            float dpi;
        };
        const Col cols[] = { { 1100, 700, 1.0f }, { 1600, 1000, 1.0f }, { 1100, 700, 2.0f }, { 1600, 1000, 2.0f } };
        constexpr int kGap = 24;
        int sheetW = kGap;
        for (const Col& c : cols)
            sheetW += c.w / 2 + kGap;
        const int rowH = 1000 / 2 + kGap, sheetH = kGap + 3 * rowH;
        funkgui::Image out;
        out.w = sheetW;
        out.h = sheetH;
        out.rgba.assign(static_cast<std::size_t>(sheetW) * static_cast<std::size_t>(sheetH) * 4, 255);
        for (std::size_t i = 0; i < out.rgba.size(); i += 4)
            out.rgba[i] = out.rgba[i + 1] = out.rgba[i + 2] = 8;
        for (int row = 0; row < 3; ++row)
        {
            int ox = kGap;
            for (const Col& col : cols)
            {
                const tee::ui::WorkspaceFactory make =
                    row == 0 ? tee::ui::makeDynoWorkspace : row == 1 ? tee::ui::makeTuneWorkspace : tee::ui::makeCarWorkspace;
                StageRig rig(make, col.w, col.h, col.dpi, row == 1 ? 0.0 : 4.6);
                if (row == 0)
                {
                    editBoost(rig, 110e3);
                    runOnce(rig);
                    editBoost(rig, 85e3);
                    rig.host->move(col.w * 0.45f, col.h * 0.4f);
                }
                else if (row == 1)
                {
                    tee::ui::tunedemo::fillDocument(rig.demo.doc);
                    for (int i = 0; i < 300; ++i)
                    {
                        rig.demo.at(i / 60.0);
                        tee::ui::tunedemo::setLive(rig.demo.engine, i / 60.0);
                        rig.host->tick(1);
                    }
                }
                else
                    tee::ui::cardemo::setLive(rig.demo.engine);
                rig.settle();
                const funkgui::Image img = funkgui::rasterise(rig.host->draw(), funkgui::FontService::get().atlas(), 1);
                const int tw = col.w / 2, th = col.h / 2, oy = kGap + row * rowH;
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
                ox += tw + kGap;
            }
        }
        const std::string path = (outDir() / "sheet.u4.png").string();
        CHECK_MSG(tee::test::writePng(out, path), path);
        std::printf("  contact sheet: %s (software rasteriser; rows DYNO, TUNE, CAR; columns 1100 x 700 and 1600 x 1000 "
                    "at density 1, then at 2)\n", path.c_str());
        return 0;
    }
}
