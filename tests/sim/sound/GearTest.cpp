// tests/sim/sound/GearTest.cpp: the gearbox's whine, the final drive's and the rattle in neutral (sim/sound/gears.fm).
#include "Rig.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>

using namespace tee::test::sound;

namespace
{
    const double kRatios[6] = {3.15, 2.18, 1.62, 1.28, 1.05, 0.86};

    // the teeth as gears.fm chooses them, worked out here on their own
    double pairTeeth(double rb) { return std::floor(48.0 / (1.0 + rb / 1.55) + 0.5); }

    std::vector<float> steady(Rig& rig, int which, double seconds = 1.0)
    {
        rig.run(0.05, which);
        return rig.run(seconds, which);
    }

    void near(const std::vector<float>& x, double f, double tol)
    {
        const double got = strongest(x, f * 0.99, f * 1.01, f * 0.0005);
        INFO("expected " << f << " Hz, the strongest line near it at " << got << " Hz");
        CHECK(std::fabs(got - f) <= tol * f);
    }
}

TEST_CASE("whine: the constant mesh, the gear's pair and the final drive where their teeth say, every gear and speed", "[sound][gears]")
{
    for (int g = 1; g <= 6; ++g)
        for (double w : {150.0, 300.0, 520.0})
        {
            Rig rig;
            rig.inGear(g, w, 0.8);
            const double hzIn = w / (2 * kPi);
            const double rb = kRatios[g - 1];
            INFO("gear " << g << " at " << w << " rad/s");
            const auto box = steady(rig, Gearbox);
            near(box, 19.0 * hzIn, 0.005);
            if (std::fabs(rb - 1.0) > 0.02)
                near(box, hzIn / 1.55 * pairTeeth(rb), 0.005);
            const auto axle = steady(rig, Axle);
            near(axle, hzIn / rb * 11.0, 0.005);       // 41 / 3.6 rounds to an 11-tooth pinion
        }
}

TEST_CASE("whine: reverse's straight-cut idler pair is loud; a direct gear carries no mesh's load", "[sound][gears]")
{
    const double w = 300.0, hzIn = w / (2 * kPi);
    Rig rev;
    rev.inGear(-1, w, 0.5);
    const auto r = steady(rev, Gearbox);
    near(r, hzIn / 1.55 * 13.0, 0.005);
    Rig fwd;
    fwd.inGear(2, w, 0.5);
    const auto f = steady(fwd, Gearbox);
    const double revPair = line(r, hzIn / 1.55 * 13.0);
    const double fwdPair = line(f, hzIn / 1.55 * pairTeeth(2.18));
    INFO("reverse's pair " << revPair << ", second's " << fwdPair);
    // (12 dB for the straight cut less 3.7 for the idler's fewer teeth: 13 against second's 20, so a lower mesh)
    CHECK(db(revPair, fwdPair) > 6.0);
    // a ratio of 1 (made so): the constant mesh only, at its unloaded level
    Rig direct;
    direct.inGear(4, w, 0.8);
    direct.set("ratioTotal", 1.0 * 3.6);
    const auto d = steady(direct, Gearbox);
    Rig fourth;
    fourth.inGear(4, w, 0.8);
    const auto o = steady(fourth, Gearbox);
    CHECK(db(line(o, 19.0 * hzIn), line(d, 19.0 * hzIn)) > 12.0);
}

TEST_CASE("whine: louder with load and with speed; straight cut some 12 dB over helical", "[sound][gears]")
{
    double last = 0.0;
    for (double load : {0.0, 0.2, 0.5, 1.0})
    {
        Rig rig;
        rig.inGear(3, 300.0, load);
        const double a = line(steady(rig, Gearbox), 19.0 * 300.0 / (2 * kPi));
        INFO("load " << load << ": " << a);
        CHECK(a > last * 1.05);
        last = a;
    }
    last = 0.0;
    for (double w : {100.0, 200.0, 350.0, 600.0})
    {
        Rig rig;
        rig.inGear(3, w, 0.5);
        const double a = rms(steady(rig, Gearbox));
        INFO("speed " << w << ": " << a);
        CHECK(a > last * 1.05);
        last = a;
    }
    Rig helical, straight;
    helical.inGear(3, 300.0, 0.5);
    straight.inGear(3, 300.0, 0.5);
    straight.set("spec.gearCut", 2);
    const double f = 19.0 * 300.0 / (2 * kPi);
    const double d = db(line(steady(straight, Gearbox), f), line(steady(helical, Gearbox), f));
    INFO("straight over helical " << d << " dB");
    CHECK(std::fabs(d - 12.04) < 0.5);
    // by gearbox: a sequential box is straight cut
    Rig seq;
    seq.inGear(3, 300.0, 0.5);
    seq.set("boxKind", 1);
    CHECK(std::fabs(db(line(steady(seq, Gearbox), f), line(steady(helical, Gearbox), f)) - 12.04) < 0.5);
}

TEST_CASE("whine: eccentricity's sidebands a shaft's speed either side of the mesh", "[sound][gears]")
{
    Rig rig;
    rig.inGear(3, 300.0, 0.8);
    const double hzIn = 300.0 / (2 * kPi), f = 19.0 * hzIn;
    const auto x = steady(rig, Gearbox, 1.0);
    const double main = line(x, f);
    const double side = line(x, f + hzIn);
    const double gapLine = line(x, f + 0.5 * hzIn);
    INFO("mesh " << main << ", sideband " << side << ", between " << gapLine);
    CHECK(db(main, side) > 15.0);      // 0.06 of it: -24 dB
    CHECK(db(main, side) < 30.0);
    CHECK(side > 4.0 * gapLine);
}

TEST_CASE("whine: silent on the bench, and the final drive silent with the car at rest", "[sound][gears]")
{
    Rig bench;
    bench.inGear(3, 300.0, 1.0);
    bench.set("driven", 0);
    CHECK(rms(steady(bench, Gearbox)) == 0.0);
    CHECK(rms(steady(bench, Axle)) == 0.0);
    Rig rest;
    rest.inGear(0, 80.0, 0.0);
    CHECK(rms(steady(rest, Axle)) == 0.0);
}

TEST_CASE("rattle: a dog box idling in neutral rattles from the crank's own fluctuation, and stops when it should", "[sound][gears]")
{
    auto rattle = [](int kind, int gear, double ripple, double engaged, double hz) {
        Rig rig;
        rig.inGear(gear, hz * 2 * kPi, 0.0);
        rig.set("boxKind", kind);
        rig.set("ripple", ripple);
        rig.set("engaged", engaged);
        rig.run(0.2, Rattle);
        return rms(rig.run(0.5, Rattle));
    };
    const double dog = rattle(1, 0, 0.04, 1.0, 13.33);
    INFO("dog box at idle " << dog);
    CHECK(dog > 1e-4);
    CHECK(rattle(1, 0, 0.0, 1.0, 13.33) < dog * 1e-3);           // a smooth crank: nothing strikes
    CHECK(rattle(1, 0, 0.04, 0.0, 13.33) == 0.0);                // the clutch out: the box stands still
    CHECK(rattle(1, 2, 0.04, 1.0, 13.33) == 0.0);                // in gear, the teeth held together
    CHECK(rattle(3, 0, 0.04, 1.0, 13.33) == 0.0);                // an automatic's converter
    const double manual = rattle(0, 0, 0.04, 1.0, 13.33);
    INFO("manual " << manual);
    CHECK(manual < dog * 0.5);
    CHECK(manual > 0.0);
    // the revs up and the firing's fluctuation smaller (as a crank's is): it fades
    CHECK(rattle(1, 0, 0.005, 1.0, 40.0) < dog * 0.5);
}
