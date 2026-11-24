// tests/ui/views/DynoTests.cpp: the DYNO workspace (ui/views/dyno/) over the demo engine (ui/app/Demo.h): its stage at
// both densities, both themes and two sizes; a run started, watched, landed, renamed, hidden and copied as CSV; the
// cursor reading the traces; a quantity the runs did not record drawn as a dash, not invented.
#include "ViewKit.h"

#include <funkgui/core/Theme.h>

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
}
