// tests/sim/bike/ClassTest.cpp: three bikes' quarter miles and top speeds against what their classes do, the
// motorcycle model (sim/vehicle/bike.fm) driven by rig.fm's engine given each class's power curve.
//
// Why the rig's engine and not the simulator's: the engine model (the reference's breathing, card D's friction) gives a
// 998 cc four about 74 kW at 11,500 rpm where a superbike's makes about 140 (tee-dyno on content/builds/bike-superbike;
// docs/models/bike.md). These tests check the vehicle: the class's power in, the class's times out. The integrated
// builds' own times are BikeEngineTest.cpp's, reported, not held to the class.
//
// The figures (each test's comment): Cycle World's instrumented tests, as cited; the engines' curves are the class's
// published crank figures in round numbers, the shapes judgement.
#include "BikeRig.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

using tee::bike::Rig;

namespace
{
    struct Bike
    {
        const char* name;
        double mass, wheelbase, cg, front, cdA, cdATuck, tuck, radius, wheelJ;
        int tyre, gears;
        double ratio[6];
        double primary;
        int fs, rs;
        double chainEff;
        bool quick;
        int wheelie;
        double jE, idle, red;
        double rpm[8], tq[8];
        double et, trapMph, topMph;     // the class's
    };

    void setUp(Rig& r, const Bike& b)
    {
        r.set("bike.spec.mass", b.mass);
        r.set("bike.spec.wheelbase", b.wheelbase);
        r.set("bike.spec.cgHeight", b.cg);
        r.set("bike.spec.frontWeight", b.front);
        r.set("bike.spec.cdA", b.cdA);
        r.set("bike.spec.tyreRadius", b.radius);
        r.set("bike.spec.wheelInertia", b.wheelJ);
        r.set("bike.spec.gears", b.gears);
        r.set("bike.spec.autoGearing", 0);
        for (int i = 0; i < b.gears; ++i)
            r.set("bike.spec.ratio[" + std::to_string(i) + "]", b.ratio[i]);
        r.set("bike.bs.cdATuck", b.cdATuck);
        r.set("bike.bs.tuck", b.tuck);
        r.set("bike.bs.bikeTyre", b.tyre);
        r.set("bike.bs.primaryRatio", b.primary);
        r.set("bike.bs.frontSprocket", b.fs);
        r.set("bike.bs.rearSprocket", b.rs);
        r.set("bike.bs.chainEfficiency", b.chainEff);
        r.set("bike.bs.quickshifter", b.quick ? 1 : 0);
        r.set("bike.bs.wheelieControl", b.wheelie);
        r.set("jE", b.jE);
        r.set("idleW", b.idle * 0.10472);
        r.set("redW", b.red * 0.10472);
        r.set("curve", 2);
        double peak = 0.0;
        for (int i = 0; i < 8; ++i)
        {
            r.set("wTab[" + std::to_string(i) + "]", b.rpm[i] * 0.10472);
            r.set("tqTab[" + std::to_string(i) + "]", b.tq[i]);
            peak = std::max(peak, b.tq[i]);
        }
        r.set("peak", peak);
        // (the rig's friction is in the curves, which are the crank's: none more but a little pumping off throttle)
        r.set("fric0", 2.0);
        r.set("fric1", 0.002);
        r.set("drag2", 0.000004);
        r.settle();
    }

    // A rider at the strip: in first, staged, the revs held at `launchRpm` on the open clutch until the green, the lever
    // out over 0.3 s, flat out, up a gear at 97 % of the redline. Returns false if the run did not finish in 30 s.
    bool quarter(Rig& r, double launchRpm)
    {
        r.set("clutch", 1.0);
        r.seconds(0.3);
        r.set("up", r.get("up") + 1);
        r.seconds(0.3);
        r.set("strip", r.get("strip") + 1);
        while (r.get("bike.tel.stripState") < 3.0 && r.get("t") < 30.0)
        {
            r.set("throttle", r.get("w") < launchRpm * 0.10472 ? 0.7 : 0.0);
            r.seconds(0.005);
        }
        r.set("throttle", 1.0);
        for (int i = 1; i <= 60; ++i)
        {
            r.set("clutch", 1.0 - i / 60.0);
            r.seconds(0.005);
        }
        const double t0 = r.get("t");
        while (r.get("bike.tel.stripState") != 5.0 && r.get("t") - t0 < 30.0)
        {
            if (r.get("w") > 0.97 * r.get("redW") && r.get("bike.box.shifting") == 0.0
                && r.get("bike.tel.gear") < r.get("bike.box.n"))
                r.set("up", r.get("up") + 1);
            r.seconds(0.005);
        }
        return r.get("bike.tel.stripState") == 5.0;
    }

    // On from the quarter mile in the top gears for 90 s: the speed it holds
    double top(Rig& r)
    {
        for (int i = 0; i < 9000; ++i)
        {
            if (r.get("w") > 0.97 * r.get("redW") && r.get("bike.box.shifting") == 0.0
                && r.get("bike.tel.gear") < r.get("bike.box.n"))
                r.set("up", r.get("up") + 1);
            r.seconds(0.01);
        }
        return r.get("bike.tel.speed");
    }

    void check(const Bike& b, double launchRpm)
    {
        Rig r;
        setUp(r, b);
        REQUIRE(quarter(r, launchRpm));
        const double et = r.get("bike.tel.t1320"), mph = r.get("bike.tel.v1320") / 0.44704;
        const double vTop = top(r) / 0.44704;
        std::printf("%s: 60 ft %.2f s, ET %.2f s (class %.2f), trap %.1f mph (class %.1f), top %.1f mph (class %.0f); "
                    "peak pitch %.1f deg\n", b.name, r.get("bike.tel.t60"), et, b.et, mph, b.trapMph, vTop, b.topMph,
                    r.get("thetaMax") * 57.3);
        CHECK(r.get("finite") == 1.0);
        CHECK(et == Catch::Approx(b.et).epsilon(0.08));
        CHECK(mph == Catch::Approx(b.trapMph).epsilon(0.08));
        CHECK(vTop == Catch::Approx(b.topMph).epsilon(0.08));
    }
} // namespace

TEST_CASE("bike.class.superbike: a litre superbike's quarter mile and top speed within 8 % of the class's", "[bike][class]")
{
    // Cycle World, Superbikes 2012 (magazine.cycleworld.com/article/2012/8/1/superbikes-2012): the GSX-R1000, 159.8 hp
    // at the wheel, 9.9 s at 145 mph; the S1000RR 9.78 s, top speed 185 mph, the GSX-R1000 7 mph less. Class: 9.9 s,
    // 145 mph, 180 mph (judgement: between the two). Crank: about 138 kW at 12,000 rpm, 113 N m at 10,500.
    const Bike b{"superbike", 203, 1.41, 0.56, 0.53, 0.50, 0.30, 1.0, 0.31, 0.8, tee::bike::Hypersport, 6,
                 {2.56, 2.00, 1.67, 1.44, 1.30, 1.18}, 1.62, 17, 43, 0.97, true, 2, 0.045, 1200, 13500,
                 {1000, 4000, 7000, 9000, 10500, 12000, 13000, 14000}, {40, 72, 92, 105, 113, 110, 101, 88},
                 9.9, 145.0, 180.0};
    check(b, 9000.0);
}

TEST_CASE("bike.class.supersport: a 600's quarter mile and top speed within 8 % of the class's", "[bike][class]")
{
    // The Yamaha R6: 10.84 s at 127.8 mph (en.wikipedia.org/wiki/List_of_fastest_production_motorcycles_by_acceleration,
    // a 2003 test); 10.8 s in zeroto60times.com's years; top speed about 161 mph (motostatz.com, slashgear.com). Crank:
    // about 88 kW at 14,500 rpm, 66 N m at 11,500 (Yamaha's claims, in round numbers).
    const Bike b{"supersport 600", 190, 1.38, 0.55, 0.53, 0.50, 0.30, 1.0, 0.31, 0.75, tee::bike::Sport, 6,
                 {2.58, 2.00, 1.67, 1.44, 1.29, 1.18}, 2.07, 16, 45, 0.97, true, 2, 0.03, 1300, 16000,
                 {1000, 5000, 8000, 10000, 11500, 13000, 14500, 16000}, {25, 42, 55, 62, 66, 63, 58, 48},
                 10.84, 127.8, 161.0};
    check(b, 10500.0);
}

TEST_CASE("bike.class.cruiser: a big twin tourer's quarter mile and top speed within 8 % of the class's", "[bike][class]")
{
    // Cycle World's Harley-Davidson 131 crate engine review (cycleworld.com/story/motorcycle-reviews/
    // harley-davidson-131-performance-review): the touring bike with the 114 (1868 cc), 13.67 s at 99.84 mph. Top
    // speed: 110 to 125 mph for 114 cruisers (secondary sources; slashgear.com gives 115 for the FXDR): 115 mph,
    // judgement. Crank: about 160 N m at 3,000 rpm, 70 kW at 5,000 (Harley's claims, round numbers). A bagger's mass and
    // fairing: 375 kg, Cd A 0.85.
    const Bike b{"cruiser", 375, 1.63, 0.55, 0.45, 0.85, 0.85, 0.0, 0.33, 1.2, tee::bike::Cruiser, 6,
                 {3.34, 2.30, 1.71, 1.35, 1.12, 0.89}, 1.45, 30, 70, 0.96, false, 0, 0.12, 900, 5500,
                 {800, 1500, 2500, 3000, 3700, 4500, 5000, 5800}, {110, 140, 158, 160, 155, 142, 133, 110},
                 13.67, 99.84, 115.0};
    check(b, 2500.0);
}
