// tests/sim/vehicle/StripTest.cpp: the drag strip (sim/vehicle/strip.fm), card I's standalone test 6: its timers
// against a car of constant acceleration, where the formulas are exact; and the tree as the whole car meets it
// (staged at rest, the ambers, the green, a red light for leaving early).
#include "Rig.h"

#include <catch2/catch_approx.hpp>

#include <cmath>
#include <cstdio>

using namespace tee::vehicle;

namespace
{
    // The strip timed by its own control(), fed the distance of a car that accelerates at `a` from `go` seconds after
    // staging, at the vehicle's control rate
    void constantAcceleration(bool pro, double a, double go)
    {
        Rig rig;
        auto* stage = rig.function<void(void*, bool)>("VehStrip::stage");
        auto* control = rig.function<void(void*, float, double, bool)>("VehStrip::control");
        void* strip = rig.at("car.strip");
        stage(strip, pro);
        const float d16 = 16.0f / 96000.0f;
        double t = 0.0;
        for (int i = 0; i < 200000 && rig.get("car.strip.state") < 5.0; ++i)
        {
            t += d16;
            const double x = t > go ? 0.5 * a * (t - go) * (t - go) : 0.0;
            control(strip, d16, x, pro);
        }
        const double r = 0.3;                      // the roll-out
        const double ts = std::sqrt(2.0 * r / a);  // from moving to leaving the beam
        auto et = [&](double d) { return std::sqrt(2.0 * (r + d) / a) - ts; };
        const double ft = 0.3048;
        const double green = pro ? 1.4 : 2.5;
        std::printf("strip (%s, %.2f m/s^2, moving at %.3f s): rt %.4f, 60 ft %.4f, 330 %.4f, 1/8 %.4f @ %.3f m/s, "
                    "1000 %.4f, 1/4 %.4f @ %.3f m/s\n",
                    pro ? "pro" : "sportsman", a, go, rig.get("car.strip.reaction"), rig.get("car.strip.et[0]"),
                    rig.get("car.strip.et[1]"), rig.get("car.strip.et[3]"), rig.get("car.strip.v660"),
                    rig.get("car.strip.et[4]"), rig.get("car.strip.et[6]"), rig.get("car.strip.v1320"));
        const double tol = 2e-5;
        CHECK(rig.get("car.strip.reaction") == Catch::Approx(go + ts - green).margin(tol));
        CHECK(rig.get("car.strip.et[0]") == Catch::Approx(et(60 * ft)).margin(tol));
        CHECK(rig.get("car.strip.et[1]") == Catch::Approx(et(330 * ft)).margin(tol));
        CHECK(rig.get("car.strip.et[3]") == Catch::Approx(et(660 * ft)).margin(tol));
        CHECK(rig.get("car.strip.et[4]") == Catch::Approx(et(1000 * ft)).margin(tol));
        CHECK(rig.get("car.strip.et[6]") == Catch::Approx(et(1320 * ft)).margin(tol));
        // the traps: the mean speed over the last 66 ft
        CHECK(rig.get("car.strip.v660") == Catch::Approx(66 * ft / (et(660 * ft) - et(594 * ft))).epsilon(1e-5));
        CHECK(rig.get("car.strip.v1320") == Catch::Approx(66 * ft / (et(1320 * ft) - et(1254 * ft))).epsilon(1e-5));
        CHECK(rig.get("car.strip.state") == (go + ts < green ? 6.0 : 5.0));
    }
} // namespace

TEST_CASE("vehicle.strip.timers: the timers of a car of constant acceleration are the textbook's", "[vehicle][strip]")
{
    constantAcceleration(false, 9.81, 2.75);
    constantAcceleration(true, 4.0, 1.6);
    constantAcceleration(false, 15.0, 2.62);
    // leaving before the green: a red light, the run still timed
    constantAcceleration(false, 9.81, 2.2);
}

TEST_CASE("vehicle.strip.tree: staged at rest, the ambers, the green, the run; a red light for leaving early",
          "[vehicle][strip]")
{
    Rig rig;
    rig.set("car.spec.drive", RWD);
    rig.settle();
    rig.pullAway(1, 0.4, 200.0);
    rig.seconds(1.0);
    REQUIRE(rig.get("car.v") > 2.0);
    // staged: at rest at the line, whatever it was doing; the clutch down and the brakes on
    rig.set("clutch", 1.0);
    rig.set("throttle", 0.0);
    rig.set("brake", 1.0);
    rig.set("strip", 1);
    rig.seconds(0.1);
    CHECK(rig.get("car.tel.stripState") == 1.0);
    CHECK(std::fabs(rig.get("car.v")) < 0.01);
    CHECK(std::fabs(rig.get("car.x")) < 0.01);
    rig.seconds(1.0);
    CHECK(rig.get("car.tel.stripState") == 2.0);
    rig.seconds(1.5);
    CHECK(rig.get("car.tel.stripState") == 3.0);
    // go
    rig.set("brake", 0.0);
    rig.pullAway(1, 0.7, 250.0);
    rig.set("throttle", 1.0);
    rig.until([&] { return rig.get("car.tel.stripState") >= 5.0; }, 60.0);
    std::printf("strip run: rt %.3f, 60 ft %.3f, 1/8 %.3f @ %.1f km/h, 1/4 %.3f @ %.1f km/h\n",
                rig.get("car.tel.tReaction"), rig.get("car.tel.t60"), rig.get("car.tel.t660"),
                rig.get("car.tel.v660") * 3.6, rig.get("car.tel.t1320"), rig.get("car.tel.v1320") * 3.6);
    CHECK(rig.get("car.tel.stripState") == 5.0);
    CHECK(rig.get("car.tel.tReaction") > 0.0);
    CHECK(rig.get("car.tel.t60") < rig.get("car.tel.t330"));
    CHECK(rig.get("car.tel.t330") < rig.get("car.tel.t660"));
    CHECK(rig.get("car.tel.t660") < rig.get("car.tel.t1000"));
    CHECK(rig.get("car.tel.t1000") < rig.get("car.tel.t1320"));
    CHECK(rig.get("car.tel.v1320") >= rig.get("car.tel.v660"));   // (in first, on its limiter by the eighth)
    // again, leaving before the green
    rig.set("throttle", 0.0);
    rig.set("clutch", 1.0);
    rig.set("brake", 1.0);
    rig.set("strip", 2);
    rig.seconds(1.0);
    rig.set("brake", 0.0);
    rig.pullAway(1, 0.7, 250.0);
    rig.set("throttle", 1.0);
    rig.until([&] { return rig.get("car.tel.stripState") >= 4.0; }, 5.0);
    CHECK(rig.get("car.tel.tReaction") < 0.0);
    rig.until([&] { return rig.get("car.tel.stripState") >= 5.0; }, 60.0);
    CHECK(rig.get("car.tel.stripState") == 6.0);
}
