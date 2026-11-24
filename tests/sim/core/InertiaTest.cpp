// tests/sim/core/InertiaTest.cpp: card D's test 4, the reciprocating masses' inertia torque; and the crank with them
// turning freely keeps its energy (the soak test of 01-rules.md §4).
#include "Presets.h"

static constexpr double kPi = 3.14159265358979323846;

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>

using namespace tee::test::core;

namespace
{
    // A V8 crossplane with given masses: piston 0.6 kg, rod 0.6 kg (a third reciprocating): m = 0.8 kg
    void v8(Rig& rig, double flywheel = 0.2)
    {
        applyLayout(rig, loadPreset("ref-12-v8-cross"));
        rig.set("spec.pistonMass", 0.6);
        rig.set("spec.rodMass", 0.6);
        rig.set("spec.rodRecip", 1.0 / 3.0);
        rig.build(0.1016, 0.0920, 0.155, 10.0, flywheel);
    }
} // namespace

TEST_CASE("the inertia torque does no work over a cycle at a steady speed", "[core][inertia]")
{
    Rig rig;
    v8(rig);
    auto at = rig.method<float(void*, float, float)>("inertiaAt");
    for (double rpm : {1000.0, 4000.0, 8000.0})
    {
        const float w = float(rpm * kPi / 30.0);
        double work = 0.0, absWork = 0.0;
        const int steps = 7200;
        for (int i = 0; i < steps; ++i)
        {
            const double t = at(rig.data(), float(720.0 * i / steps), w);
            work += t * (4.0 * kPi / steps);
            absWork += std::abs(t) * (4.0 * kPi / steps);
        }
        std::printf("V8 at %.0f rpm: inertia torque's work over a cycle %.3g J of %.3g J gross\n", rpm, work, absWork);
        CHECK(std::abs(work) < 1e-4 * absWork);
    }
}

TEST_CASE("one cylinder's inertia torque is the closed form's, and goes as the speed squared", "[core][inertia]")
{
    Rig rig;
    applyLayout(rig, loadPreset("ref-00-single"));
    rig.set("spec.pistonMass", 0.5);
    rig.set("spec.rodMass", 0.45);
    rig.set("spec.rodRecip", 1.0 / 3.0);
    const double stroke = 0.09, rod = 0.15;
    rig.build(0.095, stroke, rod, 11.0, 0.1);
    auto at = rig.method<float(void*, float, float)>("inertiaAt");
    const double m = 0.5 + 0.45 / 3.0;
    const double r = stroke / 2.0, lambda = rod / r;
    CHECK(std::abs(rig.get("mm.mRec") - m) < 1e-6);
    const double w = 6000.0 * kPi / 30.0;
    double worst = 0.0, peak = 0.0;
    for (int deg = 0; deg < 360; ++deg)
    {
        const double t = deg * kPi / 180.0, s = std::sin(t), c = std::cos(t);
        const double q = std::sqrt(lambda * lambda - s * s);
        const double d1 = r * s * (1.0 + c / q);
        const double d2 = r * (c + (c * c - s * s) / q + s * s * c * c / (q * q * q));
        const double closed = -m * w * w * d1 * d2;
        const double got = at(rig.data(), float(deg), float(w));
        worst = std::max(worst, std::abs(got - closed));
        peak = std::max(peak, std::abs(closed));
        // J(theta) = J0 + m x'^2 (J0: the flywheel and the rod's turning two thirds at the pin's radius)
        const double j = 0.1 + (0.45 * 2.0 / 3.0) * r * r + m * d1 * d1;
        CHECK(std::abs(rig.get("mm.jNow") - j) < 1e-6 * j);
        // twice the speed, four times the torque
        const double got2 = at(rig.data(), float(deg), float(2.0 * w));
        CHECK(std::abs(got2 - 4.0 * got) <= 1e-5 * peak + 1e-4);
    }
    std::printf("single at 6000 rpm: inertia torque's peak %.1f N m, worst error %.3g N m\n", peak, worst);
    CHECK(worst < 1e-5 * peak);
}

TEST_CASE("a crank with its moving masses turning freely keeps its energy (soak, 20 s)", "[core][inertia][soak]")
{
    for (double fly : {0.05, 0.25})
    {
        Rig rig;
        v8(rig, fly);
        rig.set("omega", 7000.0 * kPi / 30.0);
        auto spin = rig.method<void(void*, int, float, float)>("spin");
        const double dt = 1.0 / 96000.0;
        spin(rig.data(), 96000 * 20, float(dt), 0.0f);
        const double e0 = rig.get("energy0"), e1 = rig.get("energy1");
        const double wMin = rig.get("omegaMin") * 30.0 / kPi, wMax = rig.get("omegaMax") * 30.0 / kPi;
        std::printf("V8, flywheel %.2f kg m^2, 7000 rpm free for 20 s: energy %.6g -> %.6g J (%.2e), ripple %.1f to %.1f rpm, "
                    "inertia torque's work %.3g J\n", fly, e0, e1, (e1 - e0) / e0, wMin, wMax, rig.get("workInertia"));
        CHECK(std::abs(e1 - e0) < 1e-4 * e0);
    }
    // a single, whose J varies most
    Rig rig;
    applyLayout(rig, loadPreset("ref-00-single"));
    rig.build(0.1, 0.09, 0.15, 11.0, 0.03);
    rig.set("omega", 9000.0 * kPi / 30.0);
    auto spin = rig.method<void(void*, int, float, float)>("spin");
    spin(rig.data(), 96000 * 20, float(1.0 / 96000.0), 0.0f);
    const double e0 = rig.get("energy0"), e1 = rig.get("energy1");
    std::printf("single, 9000 rpm free for 20 s: energy %.6g -> %.6g J (%.2e), %.1f to %.1f rpm\n", e0, e1, (e1 - e0) / e0,
                rig.get("omegaMin") * 30.0 / kPi, rig.get("omegaMax") * 30.0 / kPi);
    CHECK(std::abs(e1 - e0) < 1e-4 * e0);
}
