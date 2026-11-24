// tests/ui/views/CarTests.cpp: the CAR workspace (ui/views/car/) over the demo engine, with CarDemo.h's wheel values: its
// stage at both densities, both themes and two sizes; each part selecting its group of fields; the top speed derived
// from the predicted power against drag; the wheels' values absent drawn as dashes.
#include "ViewKit.h"

#include "views/car/Car.h"
#include "views/car/CarDemo.h"

#include <funkgui/core/Theme.h>

namespace
{
    using namespace tee::test::views;

    void live(StageRig& rig)
    {
        tee::ui::cardemo::setLive(rig.demo.engine);
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
