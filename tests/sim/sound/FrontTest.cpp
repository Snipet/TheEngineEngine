// tests/sim/sound/FrontTest.cpp: the front of the engine: the accessory belt, the cam drive, a supercharger's drive
// (sim/sound/drives.fm) and the cooling fan (sim/sound/fan.fm).
#include "Rig.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>

using namespace tee::test::sound;

namespace
{
    void near(const std::vector<float>& x, double f, double tol)
    {
        const double got = strongest(x, f * 0.99, f * 1.01, f * 0.0005);
        INFO("expected " << f << " Hz, the strongest line near it at " << got << " Hz");
        CHECK(std::fabs(got - f) <= tol * f);
    }

    std::vector<float> steady(Rig& rig, int which, double settle = 0.05, double seconds = 1.0)
    {
        rig.run(settle, which);
        return rig.run(seconds, which);
    }
}

TEST_CASE("drives: the alternator at 100.8 a turn, the water pump at 7.2, the steering pump at 11", "[sound][drives]")
{
    for (double hz : {13.33, 50.0, 100.0})
    {
        Rig rig;
        rig.set("crankHz", hz);
        const auto x = steady(rig, Belt);
        INFO("crank at " << hz << " Hz");
        near(x, 100.8 * hz, 0.005);
        near(x, 7.2 * hz, 0.005);
        near(x, 11.0 * hz, 0.005);
    }
}

TEST_CASE("drives: the cam drive by kind; the chain is the reference's (none here)", "[sound][drives]")
{
    const double hz = 50.0;
    Rig chain;
    chain.set("crankHz", hz);
    CHECK(rms(steady(chain, Timing)) == 0.0);
    Rig belt;
    belt.set("crankHz", hz);
    belt.set("spec.timing", 1);
    const auto b = steady(belt, Timing);
    near(b, 21.0 * hz, 0.005);
    Rig gears;
    gears.set("crankHz", hz);
    gears.set("spec.timing", 2);
    const auto g = steady(gears, Timing);
    near(g, 30.0 * hz, 0.005);
    near(g, 60.0 * hz, 0.005);           // a straight-cut spur's harmonic
    INFO("belt " << rms(b) << ", gears " << rms(g));
    CHECK(db(rms(g), rms(b)) > 6.0);     // gears the loudest
    // louder with speed
    double last = 0.0;
    for (double h : {10.0, 30.0, 60.0, 100.0})
    {
        Rig r;
        r.set("crankHz", h);
        r.set("spec.timing", 2);
        const double a = rms(steady(r, Timing));
        CHECK(a > last * 1.05);
        last = a;
    }
}

TEST_CASE("drives: a supercharger's gear drive at the crank's 40 teeth, louder with boost; none without one", "[sound][drives]")
{
    Rig none;
    none.set("crankHz", 50.0);
    none.set("boostShare", 1.0);
    CHECK(rms(steady(none, Blower)) == 0.0);
    double last = 0.0;
    for (double b : {0.0, 0.3, 1.0})
    {
        Rig rig;
        rig.set("crankHz", 50.0);
        rig.set("asp", 4);
        rig.set("spec.blowerDrive", 1);
        rig.set("boostShare", b);
        const auto x = steady(rig, Blower);
        near(x, 40.0 * 50.0, 0.005);
        CHECK(rms(x) > last * 1.05);
        last = rms(x);
    }
    Rig belt;
    belt.set("crankHz", 50.0);
    belt.set("asp", 4);
    belt.set("boostShare", 0.5);
    CHECK(rms(steady(belt, Blower)) > 0.0);
}

TEST_CASE("fan: an electric fan comes in at its temperature, out 5 K below, its blades' line at 7 x 40 Hz", "[sound][fan]")
{
    Rig rig;
    rig.set("coolant", 365.0);
    CHECK(rms(steady(rig, Fan, 0.5, 0.5)) == 0.0);
    rig.set("coolant", 374.0);
    rig.run(2.0, Fan);                              // spun up (0.35 s)
    const auto on = rig.run(1.0, Fan);
    CHECK(rms(on) > 1e-4);
    near(on, 7.0 * 40.0, 0.005);
    rig.set("coolant", 370.0);                      // within the 5 K: stays on
    CHECK(rms(steady(rig, Fan, 0.5, 0.5)) > 1e-4);
    rig.set("coolant", 367.0);                      // off: it coasts down
    const auto down = rig.run(8.0, Fan);
    CHECK(rms(down, 0, 9600) > rms(down, down.size() - 9600) * 100.0);
    CHECK(rms(down, down.size() - 9600) == 0.0);
    // its blades
    Rig five;
    five.set("coolant", 380.0);
    five.set("spec.fanBlades", 5);
    five.run(2.0, Fan);
    near(five.run(1.0, Fan), 5.0 * 40.0, 0.005);
    // the key off: no electric fan
    Rig off;
    off.set("coolant", 380.0);
    off.set("ignition", 0);
    CHECK(rms(steady(off, Fan, 1.0, 0.5)) == 0.0);
}

TEST_CASE("fan: a viscous clutch's roar grows with the coolant's heat and with the revs", "[sound][fan]")
{
    auto level = [](double coolant, double hz) {
        Rig rig;
        rig.set("spec.fan", 1);
        rig.set("coolant", coolant);
        rig.set("crankHz", hz);
        rig.run(14.0, Fan);                         // the fluid's 4 s
        return rms(rig.run(0.5, Fan));
    };
    double last = 0.0;
    for (double t : {330.0, 361.0, 367.0, 375.0})
    {
        const double a = level(t, 50.0);
        INFO(t << " K: " << a);
        CHECK(a > last * 1.05);
        last = a;
    }
    last = 0.0;
    for (double hz : {15.0, 30.0, 60.0, 100.0})
    {
        const double a = level(370.0, hz);
        INFO(hz << " Hz: " << a);
        CHECK(a > last * 1.05);
        last = a;
    }
    // locked at 90 % of 1.1 times the crank: its blades' passing
    Rig rig;
    rig.set("spec.fan", 1);
    rig.set("coolant", 380.0);
    rig.set("crankHz", 50.0);
    rig.run(25.0, Fan);
    near(rig.run(1.0, Fan), 7.0 * 50.0 * 1.1 * 0.9, 0.005);
    Rig none;
    none.set("spec.fan", 0);
    none.set("coolant", 400.0);
    CHECK(rms(steady(none, Fan, 1.0, 0.5)) == 0.0);
}
