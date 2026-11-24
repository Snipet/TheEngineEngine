// tests/sim/core/KinematicsTest.cpp: card D's test 2, the slider-crank against its closed form.
#include "Core.h"

static constexpr double kPi = 3.14159265358979323846;

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <string>

using tee::test::core::Rig;

namespace
{
    // The closed form in double precision: x from top dead centre, r the crank's radius, lambda the rod over it
    double xAt(double r, double lambda, double t) { return r * (1.0 - std::cos(t) + lambda - std::sqrt(lambda * lambda - std::sin(t) * std::sin(t))); }
} // namespace

TEST_CASE("the slider-crank's volume, slope and acceleration match the closed form for rod ratios 2.8 to 4.5", "[core][kinematics]")
{
    Rig rig;
    auto kin = rig.method<void(void*, float)>("kin");
    const double bore = 0.0971;
    const double stroke = 0.0971;
    const double cr = 10.0;
    const double area = kPi / 4.0 * bore * bore;
    const double vd = area * stroke;
    const double vc = vd / (cr - 1.0);
    for (double lambda : {2.8, 3.3, 3.6, 4.0, 4.5})
    {
        const double r = stroke / 2.0;
        rig.build(bore, stroke, lambda * r, cr, 0.2);
        double worstV = 0.0, worstD = 0.0, worstDD = 0.0;
        for (int deg = 0; deg < 720; ++deg)
        {
            kin(rig.data(), float(deg));
            const double t = deg * kPi / 180.0;
            const double h = 1e-4;
            const double x = xAt(r, lambda, t);
            const double d1 = (xAt(r, lambda, t + h) - xAt(r, lambda, t - h)) / (2 * h);
            const double d2 = (xAt(r, lambda, t + h) - 2 * x + xAt(r, lambda, t - h)) / (h * h);
            // the volume against the closed form, and against the reference's own formula (sim/ref/cylinder.fm)
            const double v = vc + area * x;
            const double s = std::sin(t);
            const double vRef = vc + 0.5 * vd * (1.0 + lambda - std::cos(t) - std::sqrt(lambda * lambda - s * s));
            REQUIRE(std::abs(v - vRef) < 1e-12);
            worstV = std::max(worstV, std::abs(rig.get("one.vol") - v) / (vc + vd));
            worstD = std::max(worstD, std::abs(rig.get("one.dx") - d1) / r);
            worstDD = std::max(worstDD, std::abs(rig.get("one.ddx") - d2) / r);
            // dV/dtheta is the area times dx/dtheta
            REQUIRE(std::abs(rig.get("one.dvol") - area * rig.get("one.dx")) <= 1e-6 * vd);
        }
        std::printf("rod ratio %.1f: worst error of V %.2e of the largest volume, of x' %.2e r, of x'' %.2e r\n", lambda,
                    worstV, worstD, worstDD);
        CHECK(worstV < 2e-6);
        CHECK(worstD < 2e-5);
        CHECK(worstDD < 5e-4);    // (the double's finite difference itself is good to some 1e-7)
    }
}

TEST_CASE("the size and bore/stroke dials and the bore and stroke are one setting", "[core][kinematics]")
{
    Rig rig;
    auto dials = rig.method<void(void*, float, float, int)>("sizeFromDials");
    auto red = rig.method<float(void*, int)>("redline");
    for (double bs : {0.5, 0.8, 1.0, 1.25, 2.0})
    {
        // the reference's: a square cylinder's bore (the cube root of 4 vd / pi), times the ratio's cube root
        const double litres = 5.76, n = 8.0, vd = litres * 1e-3 / n;
        dials(rig.data(), float(litres), float(bs), 8);
        const double boreSq = std::cbrt(1.2732395 * vd);
        const double bore = boreSq * std::cbrt(bs);
        CHECK(std::abs(rig.get("size.bore") - bore) < 1e-6 * bore);
        CHECK(std::abs(rig.get("size.stroke") - bore / bs) < 1e-6 * bore);
        CHECK(std::abs(kPi / 4 * rig.get("size.bore") * rig.get("size.bore") * rig.get("size.stroke") - vd) < 1e-5 * vd);
    }
    // the redline by piston speed: an 86 mm stroke to 25 m/s is 8721 rpm; a diesel's 13.5 m/s 4709
    dials(rig.data(), 2.0f, 1.0f, 4);
    rig.set("size.stroke", 0.086);
    CHECK(std::abs(red(rig.data(), 0) * 30.0 / kPi - 25.0 * 60.0 / (2 * 0.086)) < 1.0);
    CHECK(std::abs(red(rig.data(), 1) * 30.0 / kPi - 13.5 * 60.0 / (2 * 0.086)) < 1.0);
}
