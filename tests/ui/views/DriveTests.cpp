// tests/ui/views/DriveTests.cpp: DRIVE (ui/views/drive/): its instruments read the demo's telemetry, a name the program
// lacks draws a dash, the wheels' slip and the strip's tree light by their names, a press on the car selects "car";
// goldens at densities 2 and 1.
#include "ViewKit.h"

#include "app/Cockpit.h"

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

namespace
{
    using namespace tee::test::views;

    // A truck with both brakes (card D2): the scene's top left shows ENGINE BRAKE with two of three segments and
    // MEDIUM (four of six cylinders), the EXHAUST BRAKE lamp lit (the flap a quarter shut); off, the switch's setting.
    VIEW_TEST(testDriveTruck, "drive.truck")
    {
        std::vector<tee::ui::FieldInfo> fields = tee::ui::demoFields(false);
        const auto add = [&](const char* path, double v, std::vector<std::string> choices) {
            tee::ui::FieldInfo f;
            f.path = path;
            f.kind = tee::ui::FieldKind::choice;
            f.choices = std::move(choices);
            f.min = 0.0;
            f.max = static_cast<double>(f.choices.size() - 1);
            f.def = v;
            f.label = path;
            fields.push_back(f);
        };
        for (tee::ui::FieldInfo& f : fields)
        {
            if (f.path == "car.kind")
                f.def = 1.0;   // Truck
            if (f.path == "engine.cylinders")
                f.def = 6.0;
        }
        add("ecu.engineBrake", 1.0, { "None", "Compression release" });
        add("ecu.brakeSwitch", 2.0, { "Off", "Low", "Medium", "High" });
        add("ecu.exhaustBrake", 1.0, { "None", "Flap", "Turbine vanes" });
        tee::ui::MemoryDoc doc(fields);
        tee::ui::Units units;
        tee::ui::DemoEngine engine(&doc, false);
        engine.setTime(4.6);
        engine.hold(true);
        tee::ui::Selection selection;
        tee::AppServices app;
        tee::ui::WorkspaceContext ctx{ doc, engine, engine, engine, units, selection, app };
        tee::ui::EngineBrakeState s = tee::ui::engineBrakeState(ctx);
        CHECK(s.shown && s.compression && s.exhaust && s.stage == 0 && s.armed == 2 && !s.exhaustOn);
        engine.set("ecu.brakeCylinders", 4.0);
        engine.set("exhaust.flap", 0.25);
        s = tee::ui::engineBrakeState(ctx);
        CHECK(s.stage == 2 && s.exhaustOn);
        engine.set("exhaust.flap", 0.0);   // a flap fitted and open: off, whatever the pressure
        CHECK(!tee::ui::engineBrakeState(ctx).exhaustOn);
        engine.set("exhaust.flap", 0.25);
        std::unique_ptr<tee::ui::Workspace> ws = tee::ui::makeDriveWorkspace(ctx);
        funkgui::HeadlessHost host(*ws, 0, 2.0f);
        host.resize(900, 640);
        ws->place({ 0.0f, 0.0f, 900.0f, 640.0f });
        host.tick(90);
        golden("drive.truck", host);
        engine.set("ecu.brakeCylinders", 0.0);
        engine.set("exhaust.flap", 0.0);
        host.tick(4);
        picture("drive.truck.armed", host);   // SET MEDIUM, dim
        return 0;
    }
}

#include "views/drive/Dash.h"

namespace
{
    using namespace tee::test::views;

    // The item whose accessibility title is `title` (and role `role`): its bounds' centre, or (-1, -1).
    funkgui::Point centreOf(funkgui::HeadlessHost& host, const std::string& title, funkgui::A11yRole role)
    {
        for (const funkgui::A11yItem& it : host.accessibility())
            if (it.title == title && it.role == role)
                return { it.bounds.centreX(), it.bounds.y + it.bounds.h * 0.5f };
        return { -1.0f, -1.0f };
    }
    std::string valueOf(funkgui::HeadlessHost& host, const std::string& title)
    {
        for (const funkgui::A11yItem& it : host.accessibility())
            if (it.title == title)
                return it.value;
        return "(none)";
    }

    // The dash's styles (the owner's ask, 25 Nov: more gauges, styles to pick, kept in the settings): the DASH
    // buttons show where the shell keeps the choice, a press sets it there and lays the stage out again; CLASSIC's
    // small dials, DIGITAL's and RACE's drawn readouts read telemetry by name; RACE's alarm boxes fill with their word.
    // At the frame's smallest stage (density 2), and the smallest window's at density 1.
    VIEW_TEST(testDriveStyles, "drive.styles")
    {
        StageRig rig(tee::ui::makeDriveWorkspace, FrameSize::smallest, 2.0f, 4.6, false);
        int kept = 0, sets = 0;
        tee::ui::FrameServices fs;
        fs.driveStyle = [&kept] { return kept; };
        fs.setDriveStyle = [&kept, &sets](int s) {
            kept = s;
            ++sets;
        };
        rig.demo.ctx.services = &fs;
        // What the demo's script does not publish, as the simulator does.
        rig.demo.engine.set("engine.oilTemperature", 372.0);
        rig.demo.engine.set("ecu.lambda", 0.88);
        rig.demo.engine.set("ecu.fuelFlow", 0.0124);
        rig.demo.engine.set("car.accel", 3.4);
        rig.ws->place(rig.ws->stage());
        rig.settle();
        golden("drive.styles.standard", *rig.host);   // STANDARD with the DASH buttons
        const char* const names[4] = { "Standard", "Classic", "Digital", "Race" };
        for (int s = 1; s < 4; ++s)
        {
            const funkgui::Point at = centreOf(*rig.host, names[s], funkgui::A11yRole::button);
            CHECK_MSG(at.x >= 0.0f, names[s]);
            rig.host->click(at.x, at.y);
            rig.settle();
            CHECK_MSG(kept == s && sets == s, std::to_string(kept));
            golden(std::string("drive.styles.") + tee::ui::dash::styleKey(s), *rig.host);
            if (s == 1)
            {
                // CLASSIC: the small dials read their channels in the user's units, the tachometer still the revs.
                CHECK_MSG(valueOf(*rig.host, "Oil temp").find("99") == 0, valueOf(*rig.host, "Oil temp"));
                CHECK_MSG(valueOf(*rig.host, "Lambda").find("0.88") == 0, valueOf(*rig.host, "Lambda"));
                CHECK(valueOf(*rig.host, "Engine").find("50") == 0);
            }
            else
            {
                // DIGITAL, RACE: the drawn readouts as a screen reader hears them.
                CHECK_MSG(valueOf(*rig.host, "LAMBDA") == "0.88", valueOf(*rig.host, "LAMBDA"));
                CHECK_MSG(valueOf(*rig.host, "Longitudinal g") == "+0.35", valueOf(*rig.host, "Longitudinal g"));
            }
        }
        // RACE's alarms: water hot, knock, lean, oil pressure low; each box says its word.
        rig.demo.engine.set("engine.coolant", 390.0);
        rig.demo.engine.set("ecu.knockRetard", 2.5);
        rig.demo.engine.set("ecu.lambda", 1.12);
        rig.demo.engine.set("engine.oilPressure", 40e3);
        rig.settle();
        CHECK_MSG(valueOf(*rig.host, "WATER").find("HOT") != std::string::npos, valueOf(*rig.host, "WATER"));
        CHECK_MSG(valueOf(*rig.host, "KNOCK").find("KNOCK") != std::string::npos, valueOf(*rig.host, "KNOCK"));
        CHECK_MSG(valueOf(*rig.host, "LAMBDA").find("LEAN") != std::string::npos, valueOf(*rig.host, "LAMBDA"));
        CHECK_MSG(valueOf(*rig.host, "OIL PRESS").find("LOW") != std::string::npos, valueOf(*rig.host, "OIL PRESS"));
        golden("drive.styles.race.alarms", *rig.host);
        // The shell's choice moved by itself (a reload): the stage follows it.
        kept = 0;
        rig.settle();
        CHECK(centreOf(*rig.host, "Oil temp", funkgui::A11yRole::progressBar).x < 0.0f);
        CHECK(valueOf(*rig.host, "LAMBDA") == "(none)");
        // Without the shell's choice no DASH buttons: STANDARD as it always was.
        StageRig bare(tee::ui::makeDriveWorkspace, 900, 640, 2.0f, 4.6, false);
        CHECK(centreOf(*bare.host, "Race", funkgui::A11yRole::button).x < 0.0f);
        // The smallest window at density 1: each style still placed, nothing under 10 px.
        StageRig small(tee::ui::makeDriveWorkspace, 696, 574, 1.0f, 4.6, false);
        small.demo.ctx.services = &fs;
        for (int s = 1; s < 4; ++s)
        {
            kept = s;
            small.settle();
            golden(std::string("drive.styles.") + tee::ui::dash::styleKey(s) + ".dpi1", *small.host);
        }
        return 0;
    }

    // The settings line: "drive.style=race" taken out of the interface's settings text (the units' lines left), and
    // written back; STANDARD (the default) is not written, a word that is no style is none.
    VIEW_TEST(testDriveStyleSetting, "drive.styles.setting")
    {
        using namespace tee::ui::dash;
        std::string text = "pressure=bar\ndrive.style=race\ntemperature=F\n";
        CHECK(takeStyle(text) == kRace);
        CHECK_MSG(text == "pressure=bar\ntemperature=F\n", text);
        CHECK(styleLine(kRace) == "drive.style=race\n" && styleLine(kStandard).empty());
        std::string none = "drive.style=sporty\n";
        CHECK(takeStyle(none) == -1 && none.empty());
        for (int s = 0; s < kStyles; ++s)
            CHECK(styleOf(styleKey(s)) == s);
        return 0;
    }
}
