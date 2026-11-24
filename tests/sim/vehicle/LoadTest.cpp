// tests/sim/vehicle/LoadTest.cpp: the loads on the tyres (sim/vehicle/vehicle.fm, loads()), card I's standalone test
// 3, against a hand calculation: at rest, accelerating at about half a g, with downforce at speed, on a live axle;
// and the wheelie's limit.
#include "Rig.h"

#include <catch2/catch_approx.hpp>

#include <cmath>
#include <cstdio>
#include <string>

using namespace tee::vehicle;

namespace
{
    double front(Rig& rig) { return rig.get("car.fz[0]") + rig.get("car.fz[1]"); }
    double rear(Rig& rig) { return rig.get("car.fz[2]") + rig.get("car.fz[3]"); }

    void rolling(Rig& rig, double v)
    {
        rig.set("car.v", v);
        rig.set("car.vPrev", v);
        for (int i = 0; i < 4; ++i)
            rig.set("car.ww[" + std::to_string(i) + "]", v / rig.get("car.radius"));
    }
} // namespace

TEST_CASE("vehicle.loads.rest: at rest each axle carries its share of the weight", "[vehicle][loads]")
{
    Rig rig;
    rig.seconds(0.5);
    const double w = 1400.0 * 9.81;
    CHECK(front(rig) == Catch::Approx(0.53 * w).epsilon(1e-4));
    CHECK(rear(rig) == Catch::Approx(0.47 * w).epsilon(1e-4));
    CHECK(rig.get("car.fz[0]") == Catch::Approx(rig.get("car.fz[1]")));
    // on a 10 % grade, nose up: the weight's normal part, and its slope part moved rearward through the height
    Rig hill;
    hill.set("car.env.grade", 0.1);
    hill.set("car.spec.abs", 0);
    hill.settle();
    hill.set("brake", 1.0);
    hill.seconds(1.0);
    const double c = 1.0 / std::sqrt(1.01);
    const double s = 0.1 * c;
    const double moved = 1400.0 * 9.81 * s * 0.5 / 2.6;
    CHECK(front(hill) == Catch::Approx(0.53 * w * c - moved).epsilon(2e-3));
    CHECK(rear(hill) == Catch::Approx(0.47 * w * c + moved).epsilon(2e-3));
}

TEST_CASE("vehicle.loads.accel: accelerating, m a h / L moves rearward through the pitch's lag", "[vehicle][loads]")
{
    Rig rig;
    rig.set("car.spec.drive", AWD);
    rig.set("curve", 2);
    rig.set("peak", 250.0);
    rig.settle();
    rig.pullAway(1, 1.0, 300.0);
    const double a0 = rig.get("car.aLag") / rig.get("car.aNow");
    rig.seconds(0.6);
    const double a = rig.get("car.aNow");
    const double lag = rig.get("car.aLag");
    std::printf("accelerating: %.3f g, lagged %.3f g (lag ratio at the clutch's lock %.2f)\n", a / 9.81, lag / 9.81, a0);
    CHECK(a / 9.81 > 0.4);
    CHECK(a / 9.81 < 0.6);
    CHECK(lag == Catch::Approx(a).epsilon(0.05));
    // the hand calculation, with the lagged acceleration
    const double moved = 1400.0 * lag * 0.5 / 2.6;
    CHECK(front(rig) == Catch::Approx(0.53 * 1400.0 * 9.81 - moved).epsilon(1e-3));
    CHECK(rear(rig) == Catch::Approx(0.47 * 1400.0 * 9.81 + moved).epsilon(1e-3));
    CHECK(rig.get("car.tel.pitch") == Catch::Approx(moved).epsilon(1e-3));
}

TEST_CASE("vehicle.loads.lag: the load follows a step of acceleration over about 0.2 s", "[vehicle][loads]")
{
    Rig rig;
    rig.set("car.spec.drive", AWD);
    rig.set("curve", 2);
    rig.set("peak", 250.0);
    rig.settle();
    rig.set("clutch", 1.0);
    rig.seconds(0.2);
    rig.set("lever", 1);
    rig.seconds(0.2);
    rig.set("throttle", 1.0);
    rig.seconds(0.3);
    // dumped: the acceleration arrives within some 50 ms; the lagged one takes its time
    rig.set("clutch", 0.0);
    rig.until([&] { return rig.get("car.aNow") > 3.0; }, 1.0);
    rig.seconds(0.2);
    const double r1 = rig.get("car.aLag") / rig.get("car.aNow");
    rig.seconds(0.8);
    const double r2 = rig.get("car.aLag") / rig.get("car.aNow");
    std::printf("lag: %.2f of the acceleration 0.2 s after it arrived, %.2f a second after\n", r1, r2);
    CHECK(r1 > 0.45);
    CHECK(r1 < 0.85);
    CHECK(r2 > 0.95);
}

TEST_CASE("vehicle.loads.downforce: downforce at speed adds to each axle by the balance", "[vehicle][loads]")
{
    Rig rig;
    rig.set("car.spec.clA", 2.0);
    rig.set("car.spec.aeroBalance", 0.4);
    rig.settle();
    rolling(rig, 50.0);
    rig.seconds(1.0);
    const double v = rig.get("car.v");
    const double down = 0.5 * 1.2 * 2.0 * v * v;
    const double moved = 1400.0 * rig.get("car.aLag") * 0.5 / 2.6;
    std::printf("downforce at %.1f m/s: %.0f N\n", v, down);
    CHECK(down > 2500.0);
    // (the speed falls a little within a control step: 0.2 % in load)
    CHECK(front(rig) == Catch::Approx(0.53 * 1400.0 * 9.81 + 0.4 * down - moved).epsilon(2e-3));
    CHECK(rear(rig) == Catch::Approx(0.47 * 1400.0 * 9.81 + 0.6 * down + moved).epsilon(2e-3));
}

TEST_CASE("vehicle.loads.liveaxle: a live axle's torque reaction unloads the right rear tyre", "[vehicle][loads]")
{
    Rig rig;
    rig.set("car.spec.liveAxle", 1);
    rig.settle();
    rig.pullAway(1, 0.6, 250.0);
    rig.seconds(0.3);
    const double tsh = rig.get("car.tsh");
    const double live = tsh / rig.get("car.box.fd") / 1.55;
    std::printf("live axle: shafts %.0f N m, rear loads %.0f %.0f N\n", tsh, rig.get("car.fz[2]"), rig.get("car.fz[3]"));
    REQUIRE(tsh > 500.0);
    // (the loads were set at the last control step, from that step's torque: within a few per cent of now's)
    CHECK(rig.get("car.fz[2]") - rig.get("car.fz[3]") == Catch::Approx(2.0 * live).epsilon(0.05));
}

TEST_CASE("vehicle.loads.wheelie: the front cannot go below zero; the acceleration is what the wheelie allows",
          "[vehicle][loads]")
{
    Rig rig;
    // a tall, short, rear-heavy machine on slicks on a prepared strip, with torque to spare
    rig.set("car.spec.drive", RWD);
    rig.set("car.spec.mass", 400.0);
    rig.set("car.spec.wheelbase", 1.5);
    rig.set("car.spec.cgHeight", 1.0);
    rig.set("car.spec.frontWeight", 0.4);
    rig.set("car.spec.tyre", Slick);
    rig.set("car.env.surface", Strip);
    rig.set("curve", 2);
    rig.set("peak", 200.0);
    rig.settle();
    rig.pullAway(1, 0.6, 300.0);
    rig.seconds(1.2);       // (the clutch's bite spins the tyres for a moment: they hook up again)
    const double aMax = 9.81 * 0.4 * 1.5 / 1.0;
    const double a = rig.get("car.aNow");
    std::printf("wheelie: %.2f m/s^2 against a limit of %.2f; front load %.0f N, flags %g\n", a, aMax, front(rig),
                rig.get("car.tel.flags"));
    CHECK((static_cast<int>(rig.get("car.tel.flags")) & 2) != 0);
    CHECK(front(rig) < 0.02 * 0.4 * 400.0 * 9.81);
    CHECK(a < 1.001 * aMax);
    CHECK(a > 0.97 * aMax);
    CHECK((static_cast<int>(rig.get("car.tel.flags")) & 1) == 0);   // (pushing, not spinning)
}
