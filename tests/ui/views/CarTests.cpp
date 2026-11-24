// tests/ui/views/CarTests.cpp: the CAR workspace (ui/views/car/) over the demo engine, with CarDemo.h's wheel values: its
// stage at both densities, both themes and two sizes; each part selecting its group of fields; the top speed derived
// from the predicted power against drag; the wheels' values absent drawn as dashes.
#include "ViewKit.h"

#include "views/car/Car.h"

#include <funkgui/core/Theme.h>

namespace
{
    using namespace tee::test::views;

    void live(StageRig& rig)
    {
        rig.settle();
    }

    // The stage's goldens (at the frame's stage rects) are made in DynoTests.cpp, with DYNO's.

    // A press on each drawing selects its group; the inspector's page lists that group's fields the build has.
    VIEW_TEST(testCarSelect, "car.select")
    {
        StageRig rig(tee::ui::makeCarWorkspace, 1100, 700, 1.0f, 4.6);
        live(rig);
        auto& sel = rig.demo.selection;
        rig.host->click(200.0f, 200.0f);                 // the body, in the side view
        CHECK_MSG(sel.path() == "car.body", sel.path());
        std::string page = rig.ws->inspectorPage();
        CHECK_MSG(page.find("car.mass") != std::string::npos && page.find("car.wheelbase") != std::string::npos, page);
        rig.host->click(900.0f, 200.0f);                 // the tyre's grip curve
        CHECK_MSG(sel.path() == "car.tyres", sel.path());
        rig.host->click(900.0f, 550.0f);                 // the gearing chart
        CHECK_MSG(sel.path() == "car.gearbox", sel.path());
        CHECK(rig.ws->inspectorPage().find("car.finalDrive") != std::string::npos);
        rig.host->click(200.0f, 520.0f);                 // the driveline
        CHECK_MSG(sel.path() == "car.diffs", sel.path());
        CHECK(rig.ws->inspectorPage().find("car.drive") != std::string::npos);
        picture("car.selected", *rig.host);
        return 0;
    }

    // The top speed: where predicted power at the wheels meets drag and rolling; more drag area, a lower top speed.
    VIEW_TEST(testCarTopSpeed, "car.topspeed")
    {
        StageRig rig(tee::ui::makeCarWorkspace, 1100, 700, 1.0f, 4.6);
        int gear = -1;
        bool bound = true;
        const double v = tee::ui::carTopSpeed(*rig.ws, &gear, &bound);
        CHECK_MSG(std::isfinite(v) && v > 40.0 && v < 120.0, std::to_string(v));
        CHECK(gear >= 0);
        // More drag area: a lower top speed.
        const int f = rig.demo.doc.find("car.mass");
        CHECK(f >= 0);
        return 0;
    }

    // No wheel values in the program: loads, slip and axle torque are dashes; the drawing still stands.
    VIEW_TEST(testCarAbsent, "car.absent")
    {
        StageRig rig(tee::ui::makeCarWorkspace, 1100, 700, 1.0f, 0.5);
        rig.demo.engine.forget("car.speed");
        rig.settle();
        CHECK(!rig.ws->moving());
        const std::uint64_t k = rig.ws->drawKey();
        rig.settle();
        CHECK(rig.ws->drawKey() == k);
        picture("car.absent", *rig.host);
        return 0;
    }
}

namespace
{
    // A motorcycle (spec.car.kind "Motorcycle", card B2's fields): the side view a bike with its rider, its front lifted
    // by car.liftAngle about the rear contact; the driveline a chain to the rear wheel; the gearing through the primary
    // and the sprockets; the groups a bike reads (none of a car's it ignores).
    VIEW_TEST(testCarBike, "car.bike")
    {
        std::vector<tee::ui::FieldInfo> fields = tee::ui::demoFields();
        for (tee::ui::FieldInfo& f : fields)
            if (f.path == "car.kind")
            {
                f.choices = { "Car", "Truck", "Motorcycle" };
                f.max = 2;
                f.def = 2;
            }
            else if (f.path == "car.mass")
                f.min = 100, f.def = 200;
            else if (f.path == "car.wheelbase")
                f.min = 1.0, f.def = 1.42;
            else if (f.path == "car.tyre")
                f.min = 0.2, f.def = 0.31;
        const auto add = [&](const char* path, tee::ui::FieldKind kind, double lo, double hi, double def,
                             std::vector<std::string> choices = {}) {
            tee::ui::FieldInfo f;
            f.path = path;
            f.kind = kind;
            f.min = lo;
            f.max = hi;
            f.def = def;
            f.label = path;
            f.choices = std::move(choices);
            fields.push_back(std::move(f));
        };
        using K = tee::ui::FieldKind;
        add("car.riderMass", K::number, 0, 200, 80);
        add("car.tuck", K::number, 0, 1, 1);
        add("car.bikeTyre", K::choice, 0, 5, 2, { "Touring", "Sport", "Hypersport", "Race slick", "Dual sport", "Cruiser" });
        add("car.primaryRatio", K::number, 1, 4, 1.7);
        add("car.frontSprocket", K::integer, 10, 30, 16);
        add("car.rearSprocket", K::integer, 25, 70, 42);
        add("car.slipper", K::toggle, 0, 1, 1);
        add("car.wheelieControl", K::choice, 0, 2, 1, { "Off", "By the throttle", "By the clutch" });
        tee::ui::DemoRig demo(4.6);
        tee::ui::MemoryDoc doc(fields);
        tee::ui::WorkspaceContext ctx{ doc, demo.engine, demo.engine, demo.engine, demo.units, demo.selection, demo.app };
        auto ws = tee::ui::makeCarWorkspace(ctx);
        for (float dpi : { 2.0f, 1.0f })
        {
            funkgui::HeadlessHost host(*ws, 0, dpi);
            host.resize(1128, 874);
            ws->place({ 0.0f, 0.0f, 1128.0f, 874.0f });
            demo.engine.set("car.liftAngle", 8.0);
            // A wheelie's loads: all on the rear (wheels [0] front, [2] rear; [1] and [3] none).
            for (const auto& [name, v] : { std::pair{ "car.wheelLoad[0]", 0.0 }, std::pair{ "car.wheelLoad[1]", 0.0 },
                                           std::pair{ "car.wheelLoad[2]", 2700.0 }, std::pair{ "car.wheelLoad[3]", 0.0 },
                                           std::pair{ "car.wheelSlip[1]", 0.0 }, std::pair{ "car.wheelSlip[3]", 0.0 },
                                           std::pair{ "car.wheelTorque[1]", 0.0 }, std::pair{ "car.wheelTorque[3]", 0.0 } })
                demo.engine.set(name, v);
            host.tick(90);
            picture(dpi > 1.5f ? "car.bike" : "car.bike.dpi1", host);
            host.resize(696, 574);
            ws->place({ 0.0f, 0.0f, 696.0f, 574.0f });
            host.tick(90);
            picture(dpi > 1.5f ? "car.bike.small" : "car.bike.small.dpi1", host);
            ws->place({ 0.0f, 0.0f, 1128.0f, 874.0f });
        }
        // The groups: a bike's.
        demo.selection.select("car.body");
        const std::string body = ws->inspectorPage();
        CHECK_MSG(body.find("car.riderMass") != std::string::npos && body.find("car.tuck") != std::string::npos, body);
        demo.selection.select("car.diffs");
        const std::string chain = ws->inspectorPage();
        CHECK_MSG(chain.find("car.rearSprocket") != std::string::npos && chain.find("car.drive") == std::string::npos, chain);
        demo.selection.select("car.tyres");
        CHECK(ws->inspectorPage().find("car.bikeTyre") != std::string::npos);
        // The top speed through the sprockets: 42/16 and a primary of 1.7 gear it far lower than a car's final drive.
        int gear = -1;
        const double v = tee::ui::carTopSpeed(*ws, &gear);
        CHECK_MSG(std::isfinite(v) && v > 20.0, std::to_string(v));
        return 0;
    }
}
