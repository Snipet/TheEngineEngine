// tests/sim/sound/BikeTest.cpp: a motorcycle's voices (sim/sound/bike.fm): the chain's whine by its front sprocket's
// teeth, the lash's clunk as hard as it closed, a dry clutch's rattle at idle.
#include "Rig.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>

using namespace tee::test::sound;

TEST_CASE("bike: the chain sings at the front sprocket's teeth times its turns, louder with load and speed; no bike, none",
          "[sound][bike]")
{
    for (int teeth : {14, 16, 18})
        for (double w : {60.0, 150.0})
        {
            Rig rig;
            rig.set("bike", 1);
            rig.set("wSprocket", w);
            rig.set("frontTeeth", teeth);
            rig.set("torque", 200.0);
            rig.run(0.05, Chain);
            const auto x = rig.run(1.0, Chain);
            const double f = w / (2 * kPi) * teeth;
            const double got = strongest(x, f * 0.99, f * 1.01, f * 0.0005);
            INFO(teeth << " teeth at " << w << " rad/s: expected " << f << ", found " << got);
            CHECK(std::fabs(got - f) < 0.005 * f);
        }
    double last = 0.0;
    for (double load : {0.0, 0.3, 1.0})
    {
        Rig rig;
        rig.set("bike", 1);
        rig.set("wSprocket", 100.0);
        rig.set("torque", load * 400.0);
        rig.run(0.05, Chain);
        const double a = rms(rig.run(0.5, Chain));
        CHECK(a > last * 1.05);
        last = a;
    }
    Rig car;
    car.set("wSprocket", 100.0);
    CHECK(rms(car.run(0.5, Chain)) == 0.0);
}

TEST_CASE("bike: the lash clunks as hard as it closes, once a closing", "[sound][bike]")
{
    auto clunk = [](double strike) {
        Rig rig;
        rig.set("bike", 1);
        rig.set("spec.chain", -60.0);
        rig.run(0.1, Chain);
        rig.set("strike", strike);
        const auto x = rig.run(0.3, Chain);
        double p = 0.0;
        for (float v : x)
            p = std::max(p, std::fabs(double(v)));
        return p;
    };
    CHECK(clunk(0.0) == 0.0);
    const double a = clunk(1.9), b = clunk(4.5);
    INFO("1.9 rad/s: " << a << ", 4.5 rad/s: " << b);
    CHECK(a > 0.0);
    CHECK(b > 2.0 * a);
}

TEST_CASE("bike: a dry clutch rattles at idle with the lever out; pulled, under load, or wet: quiet", "[sound][bike]")
{
    auto rattle = [](int dry, double lever, double load) {
        Rig rig;
        rig.set("bike", 1);
        rig.set("spec.chain", -60.0);
        rig.set("dryClutch", dry);
        rig.set("lever", lever);
        rig.set("torque", load * 400.0);
        rig.set("ripple", 0.05);
        rig.set("crankHz", 18.0);
        rig.run(0.2, Clutch);
        return rms(rig.run(0.5, Clutch));
    };
    const double idle = rattle(1, 1.0, 0.0);
    INFO("dry at idle " << idle);
    CHECK(idle > 1e-5);
    CHECK(rattle(0, 1.0, 0.0) == 0.0);
    CHECK(rattle(1, 0.0, 0.0) == 0.0);
    CHECK(rattle(1, 1.0, 0.5) == 0.0);
    CHECK(rattle(1, 1.0, 0.15) < idle);
}
