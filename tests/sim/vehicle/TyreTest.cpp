// tests/sim/vehicle/TyreTest.cpp: the tyre (sim/vehicle/tyre.fm), card I's standalone test 1: its curve by compound
// and surface, where its peak is and what it falls to; and the same seen through the whole car, braking on dry
// asphalt with ABS (near the peak) and without (a locked wheel, at the curve's tail).
#include "Rig.h"

#include <catch2/catch_approx.hpp>

#include <cmath>
#include <cstdio>

using namespace tee::vehicle;

TEST_CASE("compiles: the model and its rig compile, the rig idles in neutral", "[vehicle]")
{
    Rig rig;
    rig.seconds(1.0);
    CHECK(rig.get("finite") == 1.0);
    CHECK(rig.get("w") == Catch::Approx(85.0).margin(5.0));
    CHECK(rig.get("car.v") == 0.0);
}

namespace
{
    struct Shape
    {
        double kPeak, tail;
    };

    // Each compound's curve as made: the slip at its peak and what a spinning or locked tyre keeps
    Shape shape(Rig& rig, int compound)
    {
        auto* make = rig.function<void(void*, int)>("VehTyreCurve::make");
        void* curve = rig.at("car.curve");
        make(curve, compound);
        return {rig.get("car.curve.kPeak"), rig.get("car.curve.tail")};
    }
} // namespace

TEST_CASE("tyre.curve: each compound peaks where its kind does and falls off as its kind does", "[vehicle][tyre]")
{
    Rig rig;
    auto* at = rig.function<float(void*, float)>("VehTyreCurve::at");
    void* curve = rig.at("car.curve");
    // the peak slip and the tail (S(1) over the peak) each compound was fitted to (docs/models/vehicle.md)
    const double peak[10] = {0.12, 0.11, 0.10, 0.09, 0.08, 0.16, 0.20, 0.15, 0.13, 0.11};
    const double tail[10] = {0.78, 0.75, 0.70, 0.65, 0.60, 0.65, 0.60, 0.85, 0.80, 0.72};
    for (int c = 0; c < 10; ++c)
    {
        INFO("compound " << c);
        const Shape s = shape(rig, c);
        CHECK(s.kPeak == Catch::Approx(peak[c]).margin(0.006));
        CHECK(s.tail == Catch::Approx(tail[c]).margin(0.02));
        // the table: 1 at the peak, odd, rising to it and falling after it
        CHECK(at(curve, static_cast<float>(s.kPeak)) == Catch::Approx(1.0).margin(0.002));
        CHECK(at(curve, -0.05f) == Catch::Approx(-at(curve, 0.05f)));
        CHECK(at(curve, 0.5f * static_cast<float>(s.kPeak)) < 0.97);
        CHECK(at(curve, 0.5f) < 1.0);
        CHECK(at(curve, 3.0f) == Catch::Approx(s.tail));
    }
    // a slick is stiffer and peakier than an economy tyre, a drag slick works at a larger slip than both
    CHECK(shape(rig, Slick).kPeak < shape(rig, Economy).kPeak);
    CHECK(shape(rig, Slick).tail < shape(rig, Economy).tail);
    CHECK(shape(rig, DragSlick).kPeak > shape(rig, Street).kPeak);
}

TEST_CASE("tyre.surface: the grip each surface leaves each compound", "[vehicle][tyre]")
{
    Rig rig;
    auto* grip = rig.function<float(int, int)>("vehSurfaceGrip");
    auto* value = rig.function<float(int, int)>("vehCompoundValue");
    for (int c = 0; c < 10; ++c)
    {
        INFO("compound " << c);
        CHECK(grip(c, Dry) == 1.0f);
        CHECK(grip(c, Damp) < 1.0f);
        CHECK(grip(c, Wet) < grip(c, Damp));
        CHECK(grip(c, Snow) < grip(c, Wet));
        // dry peaks in the published ranges: 0.8 to 1.5 for road and race rubber
        CHECK(value(c, 3) >= 0.8f);
        CHECK(value(c, 3) <= 1.5f);
    }
    // a winter tyre grips snow best, a slick worst; a drag slick on a prepared strip grips more than any tyre on
    // asphalt
    for (int c = 0; c < 10; ++c)
        if (c != Winter)
            CHECK(grip(Winter, Snow) * value(Winter, 3) > grip(c, Snow) * value(c, 3));
    CHECK(grip(DragSlick, Strip) * value(DragSlick, 3) > 2.4f);
    CHECK(grip(Slick, Wet) < grip(Street, Wet));
}

namespace
{
    // A street car braking from 25 m/s in neutral, the pedal down: the mean deceleration from 20 to 5 m/s
    double braking(bool abs, int compound)
    {
        Rig rig;
        rig.set("car.spec.abs", abs ? 1 : 0);
        rig.set("car.spec.tyre", compound);
        rig.set("car.spec.brakeDecel", 1.8);      // more than the tyres can take: the grip decides
        rig.settle();
        rig.set("car.v", 25.0);
        for (int i = 0; i < 4; ++i)
            rig.set("car.ww[" + std::to_string(i) + "]", 25.0 / 0.33);
        rig.seconds(0.2);
        rig.set("brake", 1.0);
        rig.until([&] { return rig.get("car.v") < 20.0; }, 5.0);
        const double t = rig.until([&] { return rig.get("car.v") < 5.0; }, 10.0);
        return 15.0 / t;
    }
} // namespace

TEST_CASE("tyre.braking: with ABS a car stops near its tyres' peak, without it on their tail", "[vehicle][tyre]")
{
    // street tyres on dry asphalt: peak mu 1.0 (less what the load moved forward costs the front: the load
    // sensitivity; the rear, unloaded, gains a little), tail 0.75 of it
    const double withAbs = braking(true, Street) / 9.81;
    const double locked = braking(false, Street) / 9.81;
    std::printf("braking, street tyres: ABS %.3f g, locked %.3f g\n", withAbs, locked);
    CHECK(withAbs > 0.85);
    CHECK(withAbs < 1.03);
    CHECK(locked == Catch::Approx(0.75 * 0.99).margin(0.06));
    CHECK(locked < withAbs);
}
