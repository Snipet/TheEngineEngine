// tests/sim/vehicle/DiffTest.cpp: the differential element (sim/vehicle/diff.fm), card I's standalone test 2: open,
// clutch-pack, viscous and locked, on a split surface (the left wheels on snow, the right on dry asphalt), and the
// torques adding up to what the shafts bring.
#include "Rig.h"

#include <catch2/catch_approx.hpp>

#include <cmath>
#include <cstdio>
#include <string>

using namespace tee::vehicle;

namespace
{
    struct Axle
    {
        double wL, wR, tL, tR, tIn, slipL, slipR, speed;
    };

    // A rear-drive car on a split surface pulls away in first at three tenths throttle; the rear axle half a second
    // after the clutch is in
    Axle launch(double preload, double kSpeed, double kTorque, bool locked)
    {
        Rig rig;
        rig.set("car.spec.drive", RWD);
        rig.set("car.spec.rearPreload", preload);
        rig.set("car.spec.rearKSpeed", kSpeed);
        rig.set("car.spec.rearKTorque", kTorque);
        rig.set("car.spec.rearLocked", locked ? 1 : 0);
        rig.set("car.env.surfaceLeft", Snow);
        rig.settle();
        rig.pullAway(1, 0.3);
        rig.seconds(0.5);
        Axle a;
        a.wL = rig.get("car.ww[2]");
        a.wR = rig.get("car.ww[3]");
        a.tL = rig.get("car.tDrive[2]");
        a.tR = rig.get("car.tDrive[3]");
        a.tIn = rig.get("car.tsh");
        a.slipL = rig.get("car.tyre[2].kappa");
        a.slipR = rig.get("car.tyre[3].kappa");
        a.speed = rig.get("car.v");
        std::printf("diff (preload %.0f, speed %.0f, torque %.2f, locked %d): wheels %.1f %.1f rad/s, torques %.0f %.0f "
                    "of %.0f N m, slips %.3f %.3f, car %.2f m/s\n",
                    preload, kSpeed, kTorque, locked ? 1 : 0, a.wL, a.wR, a.tL, a.tR, a.tIn, a.slipL, a.slipR, a.speed);
        return a;
    }
} // namespace

TEST_CASE("vehicle.diff.open: on a split surface an open differential spins the tyre on snow, the torques equal",
          "[vehicle][diff]")
{
    const Axle a = launch(0.0, 0.0, 0.0, false);
    CHECK(a.wL > 1.5 * a.wR);
    CHECK(a.slipL > 0.5);
    CHECK(std::fabs(a.tL - a.tR) < 0.01 * (a.tL + a.tR));
    CHECK(a.tL + a.tR == Catch::Approx(a.tIn).epsilon(1e-4));
}

TEST_CASE("vehicle.diff.clutchpack: a clutch pack biases torque to the dry side by its preload and torque term",
          "[vehicle][diff]")
{
    const Axle open = launch(0.0, 0.0, 0.0, false);
    const Axle a = launch(60.0, 0.0, 0.25, false);
    // still slipping: the bias is the capacity, preload plus the torque term
    REQUIRE(a.wL > a.wR + 1.0);
    CHECK(a.tR - a.tL == Catch::Approx(60.0 + 0.25 * std::fabs(a.tIn)).epsilon(0.02));
    CHECK(a.tL + a.tR == Catch::Approx(a.tIn).epsilon(1e-4));
    CHECK(a.tL + a.tR > 1.3 * (open.tL + open.tR));   // (the dry side pushes harder: more of the engine reaches the road)
}

TEST_CASE("vehicle.diff.viscous: a viscous unit's torque grows with the speeds' difference", "[vehicle][diff]")
{
    const Axle a = launch(0.0, 15.0, 0.0, false);
    REQUIRE(a.wL > a.wR + 1.0);
    CHECK(a.tR - a.tL == Catch::Approx(15.0 * (a.wL - a.wR)).epsilon(0.03));
    CHECK(a.tL + a.tR == Catch::Approx(a.tIn).epsilon(1e-4));
}

TEST_CASE("vehicle.diff.locked: a locked axle turns both wheels as one, the dry one taking what the snow cannot",
          "[vehicle][diff]")
{
    const Axle a = launch(0.0, 0.0, 0.0, true);
    CHECK(std::fabs(a.wL - a.wR) < 0.01 * a.wR);
    CHECK(a.tR > 2.0 * a.tL);
    CHECK(a.tL + a.tR == Catch::Approx(a.tIn).epsilon(1e-4));
}

TEST_CASE("vehicle.diff.centre: an all-wheel drive's open centre splits by its ratio; the four torques add up",
          "[vehicle][diff]")
{
    Rig rig;
    rig.set("car.spec.drive", AWD);
    rig.set("car.spec.centreSplit", 0.35);
    rig.settle();
    rig.pullAway(1, 0.4);
    rig.seconds(0.5);
    const double front = rig.get("car.tDrive[0]") + rig.get("car.tDrive[1]");
    const double rearT = rig.get("car.tDrive[2]") + rig.get("car.tDrive[3]");
    const double in = rig.get("car.tsh");
    std::printf("centre: front %.0f rear %.0f of %.0f N m\n", front, rearT, in);
    REQUIRE(in > 100.0);
    CHECK(front / in == Catch::Approx(0.35).margin(0.005));
    CHECK(front + rearT == Catch::Approx(in).epsilon(1e-4));
}
