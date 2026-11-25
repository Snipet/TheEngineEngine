// tests/sim/bike/BikeTest.cpp: card B2's standalone tests of the motorcycle (sim/vehicle/bike.fm) on rig.fm's made-up
// engines: what limits a launch, the quickshifter and the auto-blipper, the slipper clutch, the chain's lash, a soak.
#include "BikeRig.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

using tee::bike::Rig;

namespace
{
    // A litre superbike: 200 kg wet, an 80 kg rider, 1.41 m, hypersport tyres, a sport bike's six on 16/43 and a
    // primary of 1.63; an engine of 113 N m at 11,000 rpm and 150 kW at 13,000, redline 14,000.
    void superbike(Rig& r)
    {
        r.set("bike.spec.mass", 200.0);
        r.set("bike.spec.wheelbase", 1.41);
        r.set("bike.spec.cgHeight", 0.58);
        r.set("bike.spec.frontWeight", 0.53);
        r.set("bike.spec.cdA", 0.50);
        r.set("bike.spec.tyreRadius", 0.31);
        r.set("bike.spec.wheelInertia", 0.8);
        r.set("bike.spec.brakeDecel", 1.1);
        r.set("bike.spec.brakeBias", 0.75);
        r.set("bike.bs.bikeTyre", tee::bike::Hypersport);
        r.set("bike.bs.primaryRatio", 1.63);
        r.set("bike.bs.frontSprocket", 16);
        r.set("bike.bs.rearSprocket", 43);
        r.set("bike.bs.cdATuck", 0.30);
        r.set("jE", 0.045);
        r.set("idleW", 1200.0 * 0.10472);
        r.set("redW", 14000.0 * 0.10472);
        r.set("peak", 113.0);
        r.set("curve", 2);
        const double rpm[8] = {1000, 4000, 7000, 9000, 11000, 12500, 13500, 14500};
        const double tq[8] = {40, 70, 92, 104, 113, 112, 104, 90};
        for (int i = 0; i < 8; ++i)
        {
            r.set("wTab[" + std::to_string(i) + "]", rpm[i] * 0.10472);
            r.set("tqTab[" + std::to_string(i) + "]", tq[i]);
        }
        r.settle();
    }

    // A big cruiser: 320 kg, 1.67 m, a low centre of mass behind the middle, cruiser tyres, five gears on a belt of
    // 30/70 and a primary of 1.45; 160 N m from a 1.9 L V-twin, redline 5,500 rpm.
    void cruiser(Rig& r)
    {
        r.set("bike.spec.mass", 320.0);
        r.set("bike.spec.wheelbase", 1.67);
        r.set("bike.spec.cgHeight", 0.52);
        r.set("bike.spec.frontWeight", 0.45);
        r.set("bike.spec.cdA", 0.75);
        r.set("bike.spec.tyreRadius", 0.33);
        r.set("bike.spec.wheelInertia", 1.1);
        r.set("bike.spec.gears", 6);
        r.set("bike.spec.autoGearing", 0);
        const double box[6] = {3.34, 2.30, 1.71, 1.35, 1.12, 0.89};
        for (int i = 0; i < 6; ++i)
            r.set("bike.spec.ratio[" + std::to_string(i) + "]", box[i]);
        r.set("bike.bs.bikeTyre", tee::bike::Cruiser);
        r.set("bike.bs.riderHeight", 0.95);
        r.set("bike.bs.riderForward", 0.40);
        r.set("bike.bs.tuck", 0.0);
        r.set("bike.bs.primaryRatio", 1.45);
        r.set("bike.bs.frontSprocket", 30);
        r.set("bike.bs.rearSprocket", 70);
        r.set("bike.bs.chainEfficiency", 0.96);
        r.set("jE", 0.12);
        r.set("idleW", 900.0 * 0.10472);
        r.set("redW", 5500.0 * 0.10472);
        r.set("peak", 160.0);
        r.set("curve", 0);
        r.settle();
    }

    // The rider: in first with the lever in, the engine held at `rpm`, then the lever out over `release` seconds
    void launch(Rig& r, double rpm, double release)
    {
        r.set("clutch", 1.0);
        r.seconds(0.3);
        r.set("up", r.get("up") + 1);
        r.seconds(0.2);
        // revs held by the throttle against the open clutch
        for (int i = 0; i < 300; ++i)
        {
            const double w = r.get("w");
            r.set("throttle", w < rpm * 0.10472 ? 0.6 : 0.0);
            r.seconds(0.005);
        }
        r.set("throttle", 1.0);
        const int steps = static_cast<int>(release / 0.005);
        for (int i = 0; i <= steps; ++i)
        {
            r.set("clutch", 1.0 - static_cast<double>(i) / steps);
            r.seconds(0.005);
        }
        r.set("clutch", 0.0);
    }
} // namespace

TEST_CASE("bike.launch: a superbike's launch is limited by its wheelie, held at the angle by the clutch", "[bike]")
{
    Rig r;
    superbike(r);
    r.set("bike.bs.wheelieControl", 2);
    r.settle();
    launch(r, 9000.0, 0.3);
    r.clear();
    double aSum = 0.0;
    int n = 0, lifted = 0, limited = 0;
    double frontMax = 0.0;
    for (int i = 0; i < 150; ++i)   // 1.5 s in first
    {
        r.seconds(0.01);
        if (r.get("bike.tel.gear") != 1.0)
            break;
        aSum += r.get("bike.tel.accel");
        ++n;
        const int f = static_cast<int>(r.get("bike.tel.flags"));
        lifted += (f & 2) != 0 ? 1 : 0;
        limited += (f & 128) != 0 && i >= 50 ? 1 : 0;
        frontMax = std::max(frontMax, r.get("bike.tel.wheelLoad[0]"));
    }
    const double aMean = aSum / n;
    const double aLift = r.get("bike.btel.wheelieLimit");
    std::printf("superbike launch: mean %.2f m/s^2 over %d blocks, wheelie limit %.2f; lifted %d, traction-limited %d; "
                "pitch max %.1f deg, now %.1f; front load max %.0f N; rear slip %.3f to %.3f; %.1f m/s\n",
                aMean, n, aLift, lifted, limited, r.get("thetaMax") * 57.3, r.get("bike.btel.liftAngle"), frontMax,
                r.get("rearSlipMin"), r.get("rearSlipMax"), r.get("bike.tel.speed"));
    CHECK(r.get("finite") == 1.0);
    CHECK(lifted > n * 3 / 4);                       // the front up most of the way
    CHECK(limited < n / 10);                         // and, after the first half second, short of its grip
    CHECK(aMean > 0.85 * aLift);                     // at about the wheelie's acceleration
    CHECK(aMean < 1.10 * aLift);
    CHECK(r.get("thetaMax") * 57.3 < 15.0);          // held near its 4 degrees, not looped
    CHECK((static_cast<int>(r.get("bike.btel.bikeFlags")) & 2) == 0);
}

TEST_CASE("bike.launch.cruiser: a cruiser's launch is limited by its rear tyre's grip, the front on the road",
          "[bike]")
{
    Rig r;
    cruiser(r);
    launch(r, 3500.0, 0.25);
    r.clear();
    int n = 0, lifted = 0, limited = 0;
    double frontMin = 1e9;
    for (int i = 0; i < 100; ++i)   // 1 s
    {
        r.seconds(0.01);
        ++n;
        const int f = static_cast<int>(r.get("bike.tel.flags"));
        lifted += (f & 2) != 0 ? 1 : 0;
        limited += (f & (128 | 1)) != 0 ? 1 : 0;
        frontMin = std::min(frontMin, r.get("bike.tel.wheelLoad[0]"));
    }
    std::printf("cruiser launch: lifted %d, traction-limited or spinning %d of %d; front load least %.0f N; wheelie "
                "limit %.2f m/s^2; accel now %.2f; rear slip max %.3f\n",
                lifted, limited, n, frontMin, r.get("bike.btel.wheelieLimit"), r.get("bike.tel.accel"),
                r.get("rearSlipMax"));
    CHECK(lifted == 0);
    CHECK(limited > n / 2);
    CHECK(frontMin > 0.25 * 9.81 * 400.0 * 0.45);    // the front keeps a good part of its load
}

namespace
{
    // In `gear` at about `rpm`, the throttle at `throttle`, settled (the bike rolling, the clutch in)
    void cruise(Rig& r, int gear, double rpm, double throttle)
    {
        r.set("noStall", 1);
        r.set("clutch", 1.0);
        r.seconds(0.2);
        r.set("up", r.get("up") + 1);
        r.seconds(0.1);
        r.set("up", r.get("up") + gear - 1);
        r.seconds(0.2 * gear);
        // put the bike at the speed that gear and rpm give, at rest in its chain and tyres
        const double g = r.get("bike.box.primary") * r.get("bike.box.r[" + std::to_string(gear - 1) + "]")
                         * r.get("bike.box.chain");
        const double ww = rpm * 0.10472 / g;
        r.set("bike.v", ww * r.get("bike.radius"));
        r.set("bike.vPrev", ww * r.get("bike.radius"));
        r.set("bike.wr", ww);
        r.set("bike.wf", ww);
        r.set("bike.wo", ww);
        r.set("w", rpm * 0.10472);
        r.set("clutch", 0.0);
        r.set("throttle", throttle);
        r.seconds(0.15);
    }
} // namespace

TEST_CASE("bike.quickshift: an upshift under power cuts the ignition for its time, the drive dips and comes back, no "
          "over-rev, no stall", "[bike]")
{
    Rig r;
    superbike(r);
    r.set("bike.bs.wheelieControl", 0);
    r.settle();
    cruise(r, 3, 10500.0, 1.0);
    r.clear();
    const double before = r.get("w");
    const double chainBefore = r.get("bike.tsh");
    r.set("up", r.get("up") + 1);
    r.seconds(0.015);
    const double chainDuring = r.get("chainMin");
    r.seconds(0.3);
    const double after = r.get("w");
    const double ratio = r.get("bike.box.r[3]") / r.get("bike.box.r[2]");
    std::printf("quickshift: cut %.1f ms (asked %.0f); engine %.0f -> %.0f rpm (the ratio's step: %.3f, seen %.3f); chain "
                "%.0f N m, %.0f in the cut, %.0f after; least crank speed %.0f rpm, most %.0f; gear %d; quickshifts %d\n",
                (r.get("cutEnd") - r.get("cutStart")) * 1000.0, r.get("bike.bs.quickshiftCut") * 1000.0,
                before * 9.549, after * 9.549, ratio, after / before, chainBefore, chainDuring, r.get("bike.tsh"),
                r.get("wMin") * 9.549, r.get("wMax") * 9.549, static_cast<int>(r.get("bike.tel.gear")),
                static_cast<int>(r.get("bike.btel.quickshifts")));
    CHECK(r.get("bike.tel.gear") == 4.0);
    CHECK(r.get("bike.btel.quickshifts") == 1.0);
    CHECK(std::fabs(r.get("cutEnd") - r.get("cutStart") - r.get("bike.bs.quickshiftCut")) < 0.0015);
    CHECK(chainDuring < 0.2 * chainBefore);         // the dogs out: the drive gone
    CHECK(r.get("bike.tsh") > 0.5 * chainBefore * ratio);
    CHECK(r.get("wMax") < 1.01 * r.get("redW"));
    CHECK(r.get("wMin") > 0.8 * before * ratio);
    CHECK(std::fabs(after / before - ratio) < 0.05);
    CHECK(r.get("running") == 1.0);
    // the gearbox's values the mechanical voices read: the input shaft turns as the clutch's basket (the crank over the
    // primary) with the clutch in, and the ratio is fourth's
    auto* input = r.function<float(void*)>("Bike::gearboxInput");
    auto* ratio4 = r.function<float(void*)>("Bike::gearboxRatio");
    CHECK(ratio4(r.at("bike")) == static_cast<float>(r.get("bike.box.r[3]")));
    CHECK(std::fabs(input(r.at("bike")) * r.get("bike.box.primary") / r.get("w") - 1.0) < 0.02);
}

TEST_CASE("bike.autoblip: a clutchless downshift blips the engine to the lower gear's speed; the rear tyre hardly feels "
          "it, no over-rev, no stall", "[bike]")
{
    double backWith = 0.0, backWithout = 0.0;
    for (int blip = 1; blip >= 0; --blip)
    {
        Rig r;
        superbike(r);
        r.set("bike.bs.wheelieControl", 0);
        r.set("bike.bs.autoBlip", blip);
        r.set("bike.bs.slipper", 0);
        r.settle();
        cruise(r, 4, 8000.0, 0.0);
        r.clear();
        const double before = r.get("w");
        r.set("down", r.get("down") + 1);
        r.seconds(0.4);
        const double after = r.get("w");
        const double ratio = r.get("bike.box.r[2]") / r.get("bike.box.r[3]");
        std::printf("downshift %s: engine %.0f -> %.0f rpm (ratio %.3f, seen %.3f); most %.0f rpm; back torque at most "
                    "%.0f N m; rear slip least %.3f; blips %d\n", blip ? "blipped" : "not blipped", before * 9.549,
                    after * 9.549, ratio, after / before, r.get("wMax") * 9.549, r.get("backMax"),
                    r.get("rearSlipMin"), static_cast<int>(r.get("bike.btel.blips")));
        CHECK(r.get("bike.tel.gear") == 3.0);
        CHECK(r.get("wMax") < 1.01 * r.get("redW"));
        CHECK(r.get("running") == 1.0);
        CHECK(std::fabs(after / before - ratio * r.get("bike.v") / r.get("vMax")) < 0.06);
        (blip ? backWith : backWithout) = r.get("rearSlipMin");
        if (blip)
            CHECK(r.get("bike.btel.blips") == 1.0);
    }
    CHECK(backWith > 0.3 * backWithout);           // (both negative: the blipped one far less so)
}

TEST_CASE("bike.slipper: a clumsy downshift at speed hops or locks the rear tyre without a slipper clutch, and does not "
          "with one", "[bike]")
{
    double slipWith = 0.0, slipWithout = 0.0, r0kPeak = 0.0;
    int hopWith = 0, hopWithout = 0;
    for (int slipper = 0; slipper <= 1; ++slipper)
    {
        Rig r;
        superbike(r);
        r.set("bike.bs.wheelieControl", 0);
        r.set("bike.bs.autoBlip", 0);
        r.set("bike.bs.slipper", slipper);
        r.settle();
        // braking hard into a corner (the front's lever at 0.4: the load comes off the rear), down a gear
        cruise(r, 3, 10500.0, 0.0);
        r.set("brake", 0.4);
        r.seconds(0.2);
        r.clear();
        r.set("down", r.get("down") + 1);
        r.seconds(0.5);
        std::printf("clumsy downshift, %s: rear slip least %.3f (the tyre's peak %.3f); %d blocks hopping or locked; back "
                    "torque at most %.0f N m; engine most %.0f rpm; gear %d\n", slipper ? "slipper" : "no slipper",
                    r.get("rearSlipMin"), r.get("bike.curve.kPeak"), static_cast<int>(r.get("hopBlocks")),
                    r.get("backMax"), r.get("wMax") * 9.549, static_cast<int>(r.get("bike.tel.gear")));
        CHECK(r.get("bike.tel.gear") == 2.0);
        CHECK(r.get("finite") == 1.0);
        (slipper ? slipWith : slipWithout) = r.get("rearSlipMin");
        r0kPeak = r.get("bike.curve.kPeak");
        (slipper ? hopWith : hopWithout) = static_cast<int>(r.get("hopBlocks"));
    }
    CHECK(hopWithout > 0);
    CHECK(hopWith == 0);
    CHECK(slipWith > -1.2 * r0kPeak);
    CHECK(slipWithout < slipWith);
}

TEST_CASE("bike.lugging: a heavy bike floored at low revs in a tall gear keeps its clutch locked once it has locked",
          "[bike]")
{
    // The launch helper closes the clutch with the revs from just over idle. Lugging the cruiser in sixth at 1,300 rpm
    // with the throttle floored, the engine sags under the load: without the latch the helper opened the clutch again
    // as it sagged and let it slip; once it has locked in a gear it is held closed by the output's speed (the patch of
    // card D, the lead's ruling of 25 Nov), so after the first lock the slip stays at the drive's own few tenths.
    Rig r;
    cruiser(r);
    cruise(r, 6, 1300.0, 0.0);
    r.set("throttle", 1.0);
    double firstLock = -1.0, slipAfter = 0.0, wMin = 1e9;
    for (int i = 0; i < 600; ++i)   // 3 s
    {
        r.seconds(0.005);
        const double slip = std::fabs(r.get("bike.slipC"));
        if (firstLock < 0.0 && slip < 0.5)
            firstLock = i * 0.005;
        else if (firstLock >= 0.0)
            slipAfter = std::max(slipAfter, slip);
        wMin = std::min(wMin, r.get("w"));
    }
    std::printf("lugging: first lock at %.3f s; slip after it at most %.2f rad/s; crank at least %.0f rpm, at the end "
                "%.0f rpm; %.1f m/s\n", firstLock, slipAfter, wMin * 9.549, r.get("w") * 9.549, r.get("bike.tel.speed"));
    REQUIRE(firstLock >= 0.0);
    CHECK(slipAfter < 2.0);
    CHECK(r.get("bike.tel.gear") == 6.0);
}

TEST_CASE("bike.lash: the driveline's play closes as an event each time the throttle opens and shuts", "[bike]")
{
    Rig r;
    superbike(r);
    r.set("bike.bs.wheelieControl", 0);
    r.settle();
    cruise(r, 2, 6000.0, 0.3);
    const double e0 = r.get("bike.drive.events");
    const int seen0 = static_cast<int>(r.get("lashSeen"));
    int sides[4] = {-1, -1, -1, -1};
    double speeds[4] = {0, 0, 0, 0};
    for (int k = 0; k < 4; ++k)
    {
        r.set("throttle", k % 2 == 0 ? 0.0 : 0.5);
        r.seconds(0.6);
        sides[k] = static_cast<int>(r.get("bike.btel.chainLashSide"));
        speeds[k] = r.get("bike.btel.chainLashSpeed");
    }
    const double events = r.get("bike.btel.chainLash") - e0;
    std::printf("lash: %.0f events over four changes of throttle; sides %d %d %d %d; closing speeds %.2f %.2f %.2f %.2f "
                "rad/s; strikes seen %d\n", events, sides[0], sides[1], sides[2], sides[3], speeds[0], speeds[1],
                speeds[2], speeds[3], static_cast<int>(r.get("lashSeen")) - seen0);
    CHECK(events >= 4.0);
    CHECK(events <= 12.0);
    CHECK(sides[0] == 1);
    CHECK(sides[1] == 0);
    CHECK(sides[2] == 1);
    CHECK(sides[3] == 0);
    for (double s : speeds)
        CHECK(s > 0.05);
    CHECK(static_cast<int>(r.get("lashSeen")) - seen0 == static_cast<int>(events));
}

TEST_CASE("bike.soak: ten minutes of random riding stay finite and bounded", "[bike][soak][nightly]")
{
    Rig r;
    superbike(r);
    r.set("noStall", 1);
    r.clear();
    int up = 0, down = 0;
    for (int i = 0; i < 1200; ++i)   // 600 s in half seconds
    {
        const float a = r.random(), b = r.random(), c = r.random(), d = r.random();
        r.set("throttle", a < 0.2f ? 0.0 : (a > 0.7f ? 1.0 : a));
        r.set("clutch", b < 0.1f ? 1.0 : 0.0);
        r.set("brake", c < 0.15f ? c * 6.0 : 0.0);
        r.set("rearBrake", c > 0.9f ? 1.0 : 0.0);
        if (d < 0.12f)
            r.set("up", ++up);
        else if (d > 0.9f)
            r.set("down", ++down);
        if (i % 200 == 0)
        {
            r.set("bike.bs.wheelieControl", (i / 200) % 3);
            r.set("bike.bs.slipper", (i / 200) % 2);
            r.set("bike.bs.autoBlip", (i / 400) % 2);
            r.set("bike.bs.quickshifter", (i / 300) % 2);
            r.settle();
        }
        r.seconds(0.5);
        REQUIRE(r.get("finite") == 1.0);
    }
    std::printf("soak: top speed %.1f m/s, crank %.0f to %.0f rpm, pitch at most %.1f deg, road peak %.3f\n",
                r.get("vMax"), r.get("wMin") * 9.549, r.get("wMax") * 9.549, r.get("thetaMax") * 57.3,
                r.get("roadPeak"));
    CHECK(r.get("vMax") < 120.0);
    CHECK(r.get("wMax") < 1.3 * r.get("redW"));
    CHECK(r.get("roadPeak") < 50.0);
}

TEST_CASE("bike.trace: a superbike's launch, printed", "[.trace]")
{
    Rig r;
    superbike(r);
    r.set("bike.bs.wheelieControl", 2);
    r.settle();
    r.set("clutch", 1.0);
    r.seconds(0.3);
    r.set("up", 1);
    r.seconds(0.2);
    for (int i = 0; i < 300; ++i)
    {
        r.set("throttle", r.get("w") < 8000 * 0.10472 ? 0.6 : 0.0);
        r.seconds(0.005);
    }
    r.set("throttle", 1.0);
    for (int i = 0; i < 400; ++i)
    {
        const double t = i * 0.005;
        r.set("clutch", std::max(0.0, 1.0 - t / 0.3));
        r.seconds(0.005);
        if (i % 10 == 0)
            std::printf("t %.2f eng %.2f rob %.2f w %4.0f v %5.2f a %5.2f kap %6.3f fzR %5.0f d %5.0f F %5.0f th %5.2f thK %.2f cap %5.0f ts %6.0f\n",
                        t, r.get("bike.engage"), r.get("bike.box.robot"), r.get("w") * 9.549, r.get("bike.v"),
                        r.get("bike.aNow"), r.get("bike.rear.kappa"), r.get("bike.fzR"), r.get("bike.rear.d"),
                        r.get("bike.rear.force"), r.get("bike.theta") * 57.3, r.get("bike.throttleK"),
                        r.get("bike.capA"), r.get("bike.tsh"));
    }
}

TEST_CASE("bike.trace.blip: an auto-blipped downshift, printed", "[.trace]")
{
    Rig r;
    superbike(r);
    r.set("bike.bs.wheelieControl", 0);
    r.set("bike.bs.slipper", 0);
    r.settle();
    cruise(r, 4, 8000.0, 0.0);
    r.set("down", r.get("down") + 1);
    for (int i = 0; i < 200; ++i)
    {
        r.samples(192);
        std::printf("t %5.1f ms gear %d shifting %d blip %5.0f w %6.0f rpm thr %.2f tc %6.1f ts %6.0f\n", i * 2.0,
                    static_cast<int>(r.get("bike.box.gear")), static_cast<int>(r.get("bike.box.shifting")),
                    r.get("bike.box.blip") * 9.549, r.get("w") * 9.549, r.get("thr"), r.get("bike.tClutch"),
                    r.get("bike.tsh"));
    }
}

TEST_CASE("bike.passive.crank: a crank whose inertia varies round the turn, spinning free in neutral, gains no energy "
          "when the bike is given its inertia of the moment (finding X-007)", "[bike][property]")
{
    double gain[2] = {0.0, 0.0};
    for (int give = 0; give <= 1; ++give)
    {
        Rig r;
        superbike(r);
        // a heavy piston on a light flywheel: the inertia swings 60 % either way of its mean twice a turn
        r.set("varyJ", 0.6);
        r.set("giveJ", give);
        r.set("running", 0);
        r.set("noStall", 1);
        r.set("fric0", 0.0);
        r.set("fric1", 0.0);
        r.set("drag2", 0.0);
        r.set("w", 600.0);
        r.seconds(0.2);
        auto* energy = r.function<double(void*)>("BikeRig::crankEnergy");
        const double e0 = energy(r.instance().data());
        r.seconds(5.0);
        const double e1 = energy(r.instance().data());
        gain[give] = e1 / e0 - 1.0;
        std::printf("varying inertia, free in neutral, %s: energy %.1f -> %.1f J (%+.3f %%), crank %.0f rad/s\n",
                    give ? "given its inertia" : "stepped with configure's", e0, e1, 100.0 * gain[give], r.get("w"));
        CHECK(r.get("finite") == 1.0);
    }
    // (the wet clutch's drag takes a little; what matters is that nothing is made)
    CHECK(gain[0] > 0.05);      // (the test sees the defect: stepped with configure's inertia the crank makes energy)
    CHECK(gain[1] <= 0.001);
    CHECK(gain[1] > -0.05);
}
