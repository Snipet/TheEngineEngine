// tests/sim/vehicle/EnergyTest.cpp: the vehicle against what physics says it must do, card I's standalone test 4: a
// coast-down against the analytic solution, a top speed against power and drag, and every joule accounted for (the
// work the engine put in equals the kinetic energy gained plus every loss) to 1 %.
#include "Rig.h"

#include <catch2/catch_approx.hpp>

#include <cmath>
#include <cstdio>
#include <string>

using namespace tee::vehicle;

namespace
{
    void rolling(Rig& rig, double v)
    {
        rig.set("car.v", v);
        rig.set("car.vPrev", v);
        for (int i = 0; i < 4; ++i)
            rig.set("car.ww[" + std::to_string(i) + "]", v / rig.get("car.radius"));
    }
} // namespace

TEST_CASE("coastdown: in neutral the car slows as drag and rolling resistance say", "[vehicle][energy]")
{
    Rig rig;
    rig.seconds(0.2);
    rolling(rig, 30.0);
    // m_eff dv/dt = -(k v^2 + c): drag (half rho CdA) and the street tyre's rolling resistance (0.010 m g, rising
    // with v^2 / 19600); the wheels' inertia is the car's too
    const double m = 1400.0, g = 9.81, crr = 0.010;
    const double c = crr * m * g;
    const double k = 0.5 * 1.2 * 0.7 + c * 0.00005102;
    // (and the propeller shaft's, turning with them in neutral: 0.05 kg m^2 at the final drive's input)
    const double mEff = m + (4.0 * 1.1 + 0.05 * rig.get("car.box.fd") * rig.get("car.box.fd")) / (0.33 * 0.33);
    const double v0 = 30.0;
    auto analytic = [&](double t) {
        const double r = std::sqrt(c / k);
        return r * std::tan(std::atan(v0 / r) - std::sqrt(k * c) * t / mEff);
    };
    const double t0 = rig.get("t");
    for (double t : {10.0, 30.0, 60.0})
    {
        rig.seconds(t - (rig.get("t") - t0));
        const double v = rig.get("car.v");
        std::printf("coast-down: %.0f s, %.3f m/s (analytic %.3f)\n", t, v, analytic(t));
        CHECK(v == Catch::Approx(analytic(t)).epsilon(0.01));
    }
}

TEST_CASE("topspeed: a car of constant power tops out where power meets drag and rolling", "[vehicle][energy]")
{
    Rig rig;
    rig.set("curve", 1);
    rig.set("power", 60000.0);
    rig.set("peak", 2000.0);
    rig.set("redW", 3000.0);
    rig.set("fric0", 0.0);
    rig.set("fric1", 0.0);
    rig.set("car.spec.autoGearing", 0);
    rig.set("car.spec.ratio[0]", 1.0);
    rig.set("car.spec.finalDrive", 3.0);
    rig.settle();
    rig.set("clutch", 1.0);
    rig.seconds(0.2);
    rig.set("lever", 1);
    rig.seconds(0.2);
    const double v0 = 48.0;
    rolling(rig, v0);
    rig.set("car.wo", v0 / 0.33);
    rig.set("w", 3.0 * v0 / 0.33);
    rig.set("throttle", 1.0);
    rig.set("clutch", 0.0);
    rig.seconds(60.0);
    // 0.97 P = (k v^2 + c) v
    const double m = 1400.0, g = 9.81, c = 0.010 * m * g;
    const double k = 0.5 * 1.2 * 0.7 + c * 0.00005102;
    double v = 50.0;
    for (int i = 0; i < 20; ++i)
        v -= (k * v * v * v + c * v - 0.97 * 60000.0) / (3.0 * k * v * v + c);
    const double vEnd = rig.get("car.v");
    std::printf("top speed: %.3f m/s, power against drag and rolling %.3f (the tyres' slip takes the rest)\n", vEnd, v);
    CHECK(rig.get("car.tel.gear") == 1.0);
    CHECK(vEnd == Catch::Approx(v).epsilon(0.015));
    CHECK(vEnd < v);
}

TEST_CASE("energy: the engine's work is the energy gained plus every loss, to 1 %", "[vehicle][energy]")
{
    for (int drive = 0; drive < 3; ++drive)
    {
        INFO("drive " << drive);
        Rig rig;
        rig.set("car.spec.drive", drive);
        rig.set("car.spec.liveAxle", drive == RWD ? 1 : 0);
        rig.set("car.spec.rearPreload", 40.0);
        rig.set("car.spec.frontKSpeed", 10.0);
        rig.set("car.env.grade", 0.03);
        rig.set("car.env.surfaceLeft", Wet);
        rig.settle();
        rig.set("clutch", 1.0);
        rig.seconds(0.2);
        rig.set("lever", 1);
        rig.seconds(0.2);
        rig.audit();
        const double e0 = rig.kinetic();
        // pull away, accelerate, then the clutch down and the brakes on
        rig.pullAway(1, 0.7, 250.0);
        rig.seconds(3.0);
        const double mid = rig.kinetic();
        rig.set("clutch", 1.0);
        rig.set("throttle", 0.0);
        rig.set("brake", 0.7);
        rig.seconds(1.5);
        const double e1 = rig.kinetic();
        const double in = rig.get("car.eIn");
        const char* names[] = {"eClutch", "eGear", "eDiff", "eTyre", "eHold", "eAero", "eGrade", "eAbsorb", "eShaft",
                               "eLift"};
        double losses = 0.0;
        for (const char* n : names)
            losses += rig.get(std::string("car.") + n);
        std::printf("energy (drive %d): in %.0f J, kinetic %.0f -> %.0f (peak %.0f), losses %.0f: clutch %.0f, tyres "
                    "%.0f, brakes and rolling %.0f, air %.0f, grade %.0f, diff %.0f, shafts %.0f; unaccounted %.3f %%\n",
                    drive, in, e0, e1, mid, losses, rig.get("car.eClutch"), rig.get("car.eTyre"), rig.get("car.eHold"),
                    rig.get("car.eAero"), rig.get("car.eGrade"), rig.get("car.eDiff"), rig.get("car.eShaft"),
                    100.0 * (in - (e1 - e0) - losses) / in);
        CHECK(in > 1.0e5);
        CHECK(std::fabs(in - (e1 - e0) - losses) < 0.01 * in);
        for (const char* n : names)
            if (std::string(n) != "eGrade" && std::string(n) != "eShaft")
                CHECK(rig.get(std::string("car.") + n) >= -1.0);    // every loss is a loss
    }
}
