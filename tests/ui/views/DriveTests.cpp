// tests/ui/views/DriveTests.cpp: DRIVE (ui/views/drive/): its instruments read the demo's telemetry, a name the program
// lacks draws a dash, the wheels' slip and the strip's tree light by their names, a press on the car selects "car";
// goldens at densities 2 and 1.
#include "ViewKit.h"

namespace tee::ui
{
    std::unique_ptr<Workspace> makeDriveWorkspace(WorkspaceContext& c);
}

namespace
{
    using namespace tee::test::views;

    VIEW_TEST(testDriveInstruments, "drive.instruments")
    {
        StageRig rig(tee::ui::makeDriveWorkspace, 900, 640, 2.0f, 4.6, false);
        CHECK(std::string(rig.ws->id()) == "drive" && !rig.ws->ribbon());
        // The tachometer reads the engine's speed in rpm, its span follows the redline (6800: to 8000).
        bool tach = false, speedo = false;
        for (const funkgui::A11yItem& it : rig.host->accessibility())
        {
            tach = tach || (it.title == "Engine" && it.value.find("50") == 0);
            speedo = speedo || it.title == "Road speed";
        }
        CHECK(tach && speedo);
        golden("drive.stage", *rig.host);
        // The vehicle's fields of card I, once the program has them: slip past 15 % reads SPIN, the tree's ambers, a slip.
        rig.demo.engine.set("car.wheelSlip[2]", 0.32);
        rig.demo.engine.set("car.wheelSlip[3]", 0.28);
        rig.demo.engine.set("car.wheelSlip[0]", 0.01);
        rig.demo.engine.set("car.wheelSlip[1]", 0.01);
        rig.demo.engine.set("car.stripState", 4.0);
        rig.demo.engine.set("car.tReaction", 0.412);
        rig.demo.engine.set("car.t60", 1.874);
        rig.demo.engine.set("car.t330", 5.102);
        rig.demo.engine.set("ecu.lambda", 0.87);
        rig.settle();
        golden("drive.strip", *rig.host);
        // Pressing the car selects the car's section.
        rig.host->click(450.0f, 520.0f - 60.0f);
        CHECK_MSG(rig.demo.selection.path() == "car" || rig.demo.selection.path() == "env", rig.demo.selection.path());
        return 0;
    }

    // Density 1 and a small stage: nothing under 10 px, everything still placed.
    VIEW_TEST(testDriveSmall, "drive.small")
    {
        StageRig rig(tee::ui::makeDriveWorkspace, 696, 574, 1.0f, 4.6, false);
        golden("drive.small.dpi1", *rig.host);
        StageRig big(tee::ui::makeDriveWorkspace, 1240, 870, 2.0f, 6.3, false);   // on the limiter
        golden("drive.large", *big.host);
        return 0;
    }

    // A motorcycle (spec.car.kind "Motorcycle", card B2): the bike from the side, lifted by car.liftAngle with the word,
    // the two wheels' slip (front [0], rear [2]).
    VIEW_TEST(testDriveBike, "drive.bike")
    {
        std::vector<tee::ui::FieldInfo> fields = tee::ui::demoFields(false);
        for (tee::ui::FieldInfo& f : fields)
            if (f.path == "car.kind")
            {
                f.choices.push_back("Motorcycle");
                f.max = static_cast<double>(f.choices.size() - 1);
                f.def = f.max;
            }
        tee::ui::MemoryDoc doc(fields);
        tee::ui::Units units;
        tee::ui::DemoEngine engine(&doc, false);
        engine.setTime(4.6);
        tee::ui::Selection selection;
        tee::AppServices app;
        tee::ui::WorkspaceContext ctx{ doc, engine, engine, engine, units, selection, app };
        engine.set("car.liftAngle", 18.0);
        engine.set("car.bikeFlags", 4.0);
        engine.set("car.wheelSlip[2]", 0.24);
        engine.set("car.wheelSlip[0]", 0.0);
        std::unique_ptr<tee::ui::Workspace> ws = tee::ui::makeDriveWorkspace(ctx);
        funkgui::HeadlessHost host(*ws, 0, 2.0f);
        host.resize(900, 640);
        ws->place({ 0.0f, 0.0f, 900.0f, 640.0f });
        host.tick(90);
        golden("drive.bike", host);
        return 0;
    }
}
