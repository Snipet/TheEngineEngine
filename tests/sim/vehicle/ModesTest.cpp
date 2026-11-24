// tests/sim/vehicle/ModesTest.cpp: the chassis dyno (the driven wheels on a roller that holds a speed or sweeps; the
// power at the wheels against the engine's), and a burnout on the line lock bringing slicks up to temperature.
#include "Rig.h"

#include <catch2/catch_approx.hpp>

#include <cmath>
#include <cstdio>

using namespace tee::vehicle;

TEST_CASE("dyno.hold: on a chassis dyno the roller holds the engine's speed; the wheels give the engine's power less "
          "the driveline's", "[vehicle][dyno]")
{
    Rig rig;
    rig.set("curve", 2);
    rig.set("peak", 300.0);
    rig.settle();
    rig.set("mode", DynoChassis);
    rig.pullAway(1, 0.5, 200.0);
    rig.set("clutch", 1.0);
    rig.seconds(0.2);
    rig.set("lever", 3);
    rig.seconds(0.2);
    rig.set("clutch", 0.0);
    rig.set("dynoSpeed", 400.0);
    rig.set("throttle", 1.0);
    rig.seconds(3.0);
    const double w = rig.get("w");
    const double engine = rig.get("tqEngine") * w;
    const double wheels = rig.get("car.tel.wheelPower");
    const double roller = (rig.get("car.tyre[2].force") + rig.get("car.tyre[3].force")) * rig.get("car.vr");
    std::printf("dyno: crank %.1f rad/s, engine %.1f kW, at the wheels %.1f kW, on the roller %.1f kW, the car %.4f m/s\n",
                w, engine / 1000.0, wheels / 1000.0, roller / 1000.0, rig.get("car.v"));
    CHECK(w == Catch::Approx(400.0).epsilon(0.01));
    CHECK(rig.get("car.v") == 0.0);
    CHECK(rig.get("car.tel.speed") == Catch::Approx(rig.get("car.vr")));
    // the gears' 3 %, then the tyres' slip (some 4 % under 4 kN on a roller)
    CHECK(wheels / engine == Catch::Approx(0.97).epsilon(0.005));
    CHECK(roller / wheels > 0.95);
    CHECK(roller / wheels < 1.0);
}

TEST_CASE("dyno.sweep: a sweep takes the engine up at the rate asked", "[vehicle][dyno]")
{
    Rig rig;
    rig.set("curve", 2);
    rig.set("peak", 300.0);
    rig.settle();
    rig.set("mode", DynoChassis);
    rig.pullAway(2, 0.5, 200.0);
    rig.set("dynoSpeed", 250.0);
    rig.set("throttle", 1.0);
    rig.seconds(2.0);
    rig.set("dynoRate", 100.0);
    rig.seconds(0.5);
    const double w0 = rig.get("w");
    rig.seconds(2.0);
    const double rate = (rig.get("w") - w0) / 2.0;
    std::printf("dyno sweep: %.1f rad/s^2\n", rate);
    CHECK(rate == Catch::Approx(100.0).epsilon(0.03));
}

TEST_CASE("tyre.burnout: a burnout on the line lock brings slicks up to their working temperature", "[vehicle][tyre]")
{
    Rig rig;
    rig.set("car.spec.drive", RWD);
    rig.set("car.spec.tyre", Slick);
    rig.set("curve", 2);
    rig.set("peak", 450.0);
    rig.set("noStall", 1);
    rig.settle();
    rig.seconds(0.1);
    const double cold = rig.get("car.tyre[2].grip");
    rig.set("buttons", 1);
    rig.set("clutch", 1.0);
    rig.seconds(0.2);
    rig.set("lever", 1);
    rig.seconds(0.2);
    rig.set("throttle", 0.7);
    rig.seconds(0.4);
    rig.set("clutch", 0.0);
    const double t = rig.until([&] { return rig.get("car.tyre[2].temp") > 340.0 && rig.get("car.tyre[3].temp") > 340.0; },
                               15.0);
    std::printf("burnout: slicks from 293 K to %.0f K and %.0f K in %.1f s; grip %.2f to %.2f; the car moved %.2f m\n",
                rig.get("car.tyre[2].temp"), rig.get("car.tyre[3].temp"), t, cold, rig.get("car.tyre[2].grip"),
                rig.get("car.x"));
    CHECK(cold == Catch::Approx(0.7));
    CHECK(t < 8.0);
    CHECK(rig.get("car.tyre[2].grip") == Catch::Approx(1.0));
    CHECK(std::fabs(rig.get("car.x")) < 0.5);
}
