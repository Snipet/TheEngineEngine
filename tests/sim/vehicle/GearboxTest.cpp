// tests/sim/vehicle/GearboxTest.cpp: the gearboxes (sim/vehicle/gearbox.fm, converter.fm), card I's standalone test
// 5: the converter's stall speed against its K; a dual-clutch shift without a hole in the torque (and the manual's
// paddle shift, which has one); a sequential upshift's cut asked for and released, its downshift's blip; the CVT
// holding the engine where the pedal sets it; a manual that refuses a gear with the clutch up and the speeds apart.
#include "Rig.h"

#include <catch2/catch_approx.hpp>

#include <cmath>
#include <cstdio>
#include <string>

using namespace tee::vehicle;

TEST_CASE("vehicle.gearbox.stall: an automatic held on its brakes stalls the converter where (w / K)^2 meets the engine",
          "[vehicle][gearbox]")
{
    for (double k : {11.0, 13.5, 17.0})
    {
        Rig rig;
        rig.set("car.spec.gearbox", Automatic);
        rig.set("car.spec.converterK", k);
        rig.set("car.spec.brakeDecel", 2.5);
        rig.set("curve", 2);
        rig.set("peak", 300.0);
        rig.settle();
        rig.set("brake", 1.0);
        rig.set("lever", 1);
        rig.seconds(1.0);
        rig.set("throttle", 1.0);
        rig.seconds(3.0);
        // 300 - 8 - 0.02 w = (w / K)^2
        double w = 200.0;
        for (int i = 0; i < 20; ++i)
            w -= (w * w / (k * k) - (292.0 - 0.02 * w)) / (2.0 * w / (k * k) + 0.02);
        std::printf("stall, K %.1f: %.1f rad/s (%.0f rpm); (w / K)^2 says %.1f; the car moved %.4f m\n", k,
                    rig.get("w"), rig.get("w") * 9.5493, w, rig.get("car.x"));
        CHECK(rig.get("car.tel.gear") == 1.0);
        CHECK(rig.get("w") == Catch::Approx(w).epsilon(0.005));
        CHECK(std::fabs(rig.get("car.x")) < 0.01);
    }
}

namespace
{
    // A shift up from second to third at about 70 % of the redline, full throttle: the shafts' torque just before it,
    // and the least and the most while it happens
    struct Shift
    {
        double before, least, most, ratio, took;
    };

    Shift upshift(int box)
    {
        Rig rig;
        rig.set("car.spec.gearbox", box);
        rig.set("curve", 2);
        rig.set("peak", 250.0);
        rig.settle();
        rig.set("up", 1);
        rig.seconds(0.3);
        rig.set("throttle", 0.5);
        rig.until([&] { return rig.get("car.slipC") < 1.0 && rig.get("w") > 200.0; }, 5.0);
        rig.set("up", 2);
        rig.seconds(0.3);
        rig.set("throttle", 1.0);
        rig.until([&] { return rig.get("w") > 470.0; }, 10.0);
        REQUIRE(rig.get("car.tel.gear") == 2.0);
        double before = 0.0;
        for (int i = 0; i < 16; ++i)
        {
            rig.samples(96);
            before += rig.get("car.tsh") / 16.0;
        }
        const double g2 = rig.get("car.tel.ratio");
        rig.clear();
        rig.set("up", 3);
        const double t0 = rig.get("t");
        const double took = rig.until([&] { return (static_cast<int>(rig.get("car.tel.flags")) & 16) == 0 && rig.get("t") - t0 > 0.01; }, 2.0);
        rig.seconds(0.05);
        Shift s{before, rig.get("shaftMin"), rig.get("shaftMax"), rig.get("car.tel.ratio") / g2, took};
        std::printf("upshift (box %d): before %.0f N m, during %.0f to %.0f, ratio %.3f, %.3f s\n", box, s.before, s.least,
                    s.most, s.ratio, s.took);
        CHECK(rig.get("car.tel.gear") == 3.0);
        return s;
    }
} // namespace

TEST_CASE("vehicle.gearbox.dualclutch: a dual clutch's upshift hands the torque over with no hole", "[vehicle][gearbox]")
{
    const Shift dct = upshift(DualClutch);
    // the torque at the wheels falls from second's to third's by their ratio, never below that
    CHECK(dct.least > 0.8 * dct.ratio * dct.before);
    CHECK(dct.took == Catch::Approx(0.15).margin(0.01));
    // the manual's paddles (a robotised clutch) open it: a hole
    const Shift paddles = upshift(Manual);
    CHECK(paddles.least < 0.1 * paddles.before);
}

TEST_CASE("vehicle.gearbox.sequential: a dog box's upshift asks for a 60 ms cut and lets it go; a downshift a blip",
          "[vehicle][gearbox]")
{
    Rig rig;
    rig.set("car.spec.gearbox", Sequential);
    rig.settle();
    rig.set("up", 1);
    rig.seconds(0.2);
    rig.set("throttle", 0.5);
    rig.until([&] { return rig.get("car.slipC") < 1.0 && rig.get("w") > 200.0; }, 5.0);
    rig.set("throttle", 1.0);
    rig.until([&] { return rig.get("w") > 500.0; }, 10.0);
    rig.clear();
    rig.set("up", 2);
    rig.seconds(0.3);
    std::printf("sequential: cut from %.4f to %.4f s; gear %g\n", rig.get("cutStart"), rig.get("cutEnd"),
                rig.get("car.tel.gear"));
    CHECK(rig.get("car.tel.gear") == 2.0);
    CHECK(rig.get("cutSeen") == 1.0);
    CHECK(rig.get("cutEnd") - rig.get("cutStart") == Catch::Approx(0.06).margin(0.0004));
    CHECK((static_cast<int>(rig.get("car.tel.flags")) & 32) == 0);
    // the clunk: the dogs' jolt closes the driveline's play
    CHECK(rig.get("roadPeak") > 0.0);
    // down again: a blip to first's speed
    rig.set("throttle", 0.0);
    rig.samples(16 * 4);
    const double wOut = rig.get("car.wo");
    rig.set("down", 1);
    rig.samples(16 * 3);
    const double blip = rig.get("car.box.blip");
    std::printf("sequential: blip to %.1f rad/s (first's speed %.1f)\n", blip, wOut * rig.get("car.box.r[0]") *
                rig.get("car.box.fd"));
    CHECK(blip == Catch::Approx(wOut * rig.get("car.box.r[0]") * rig.get("car.box.fd")).epsilon(0.02));
    rig.seconds(0.3);
    CHECK(rig.get("car.tel.gear") == 1.0);
    CHECK(rig.get("car.box.blip") == 0.0);
}

TEST_CASE("vehicle.gearbox.cvt: a CVT holds the engine at the speed the pedal sets while its ratio moves",
          "[vehicle][gearbox]")
{
    for (double pedal : {1.0, 0.5})
    {
        Rig rig;
        rig.set("car.spec.gearbox", Cvt);
        rig.set("curve", 1);
        rig.set("power", 70000.0);
        rig.set("peak", 300.0);
        rig.settle();
        rig.set("lever", 1);
        rig.seconds(0.5);
        rig.set("throttle", pedal);
        // (from rest it pulls away on its lowest ratio until the engine reaches the speed it is to hold)
        const double target = 85.0 + pedal * (0.9 * 680.0 - 85.0);
        rig.until([&] { return rig.get("w") > 0.95 * target; }, 8.0);
        rig.seconds(0.3);
        double least = 1e9, most = 0.0;
        for (int i = 0; i < 40; ++i)
        {
            rig.seconds(0.1);
            least = std::fmin(least, rig.get("w"));
            most = std::fmax(most, rig.get("w"));
        }
        std::printf("cvt, pedal %.1f: engine %.1f to %.1f rad/s (target %.1f), ratio %.3f, %.1f m/s\n", pedal, least,
                    most, target, rig.get("car.box.cvt"), rig.get("car.v"));
        CHECK(least > 0.95 * target);
        CHECK(most < 1.05 * target);
        CHECK(rig.get("car.box.cvt") < rig.get("car.spec.cvtLow"));
        CHECK(rig.get("car.box.cvt") > rig.get("car.spec.cvtHigh"));
    }
}

TEST_CASE("vehicle.gearbox.manual: a gear will not go in with the clutch up and the speeds apart", "[vehicle][gearbox]")
{
    Rig rig;
    rig.pullAway(1, 0.5, 200.0);
    rig.until([&] { return rig.get("w") > 450.0; }, 10.0);
    // into second with the clutch: in
    rig.set("clutch", 1.0);
    rig.seconds(0.1);
    rig.set("lever", 2);
    rig.seconds(0.1);
    CHECK(rig.get("car.tel.gear") == 2.0);
    rig.set("clutch", 0.0);
    rig.seconds(1.0);
    // into first without it, at speed: refused, in neutral, until the clutch goes down
    rig.set("throttle", 0.0);
    rig.set("lever", 1);
    rig.seconds(0.1);
    CHECK(rig.get("car.tel.gear") == 0.0);
    CHECK(rig.get("car.tel.shift") == 3.0);
    rig.set("clutch", 1.0);
    rig.seconds(0.1);
    CHECK(rig.get("car.tel.gear") == 1.0);
    // and into reverse at speed: never
    rig.set("lever", -1);
    rig.seconds(0.2);
    CHECK(rig.get("car.tel.gear") == 0.0);
}
