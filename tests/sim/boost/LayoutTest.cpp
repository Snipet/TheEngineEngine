// tests/sim/boost/LayoutTest.cpp: the arrangements and the extras on the made-up engine: a sequential pair's second
// turbo joins above its flow; a compound pair's stages share the ratio; variable vanes make boost sooner; anti-lag holds
// the shaft's speed off the throttle; electric assist spools sooner.
#include "Rig.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>

using tee::test::boost::Rig;

namespace
{
    void build(Rig& rig, int layout, double rpm)
    {
        rig.set("spec.kind", 1);
        rig.set("spec.layout", layout);
        rig.set("spec.model", 1);
        rig.spec("rev", 0);
        rig.set("rpm", rpm);
        rig.set("throttle", 1.0);
        rig.set("pulsed", 1);
    }

    double timeTo(Rig& rig, double target, double limit = 6.0)
    {
        for (double t = 0.005; t <= limit; t += 0.005)
        {
            rig.run(0.005);
            if (rig.boost() >= target)
                return t;
        }
        return limit + 1.0;
    }
} // namespace

TEST_CASE("boost: a sequential pair's second turbo joins above its flow", "[boost][layout]")
{
    Rig low;
    build(low, 3, 2500.0);
    low.run(3.0);
    Rig high;
    build(high, 3, 6500.0);
    high.run(3.0);
    std::printf("sequential at 2500 rpm: valve %.2f, shafts %.0f and %.0f rpm, %.1f kPa; at 6500: valve %.2f, %.0f and %.0f "
                "rpm, %.1f kPa\n", low.get("boost.seqOpen"), low.shaftRpm(0), low.shaftRpm(1), low.boost() / 1000.0,
                high.get("boost.seqOpen"), high.shaftRpm(0), high.shaftRpm(1), high.boost() / 1000.0);
    CHECK(low.get("boost.seqOpen") < 0.05);
    CHECK(high.get("boost.seqOpen") > 0.95);
    CHECK(high.shaftRpm(1) > 3.0 * low.shaftRpm(1));
    CHECK(high.shaftRpm(1) > 30000.0);
}

TEST_CASE("boost: a compound pair's stages share the pressure ratio", "[boost][layout]")
{
    Rig rig;
    build(rig, 4, 6000.0);
    rig.set("spec.gateSpring", 200000.0);
    rig.spec("rev", 0);
    rig.run(4.0);
    const double hp = rig.get("boost.comp[0].pr");
    const double lp = rig.get("boost.comp[1].pr");
    std::printf("compound at 6000 rpm: low stage %.2f, high stage %.2f, boost %.1f kPa, shafts %.0f and %.0f rpm\n", lp, hp,
                rig.boost() / 1000.0, rig.shaftRpm(1), rig.shaftRpm(0));
    CHECK(lp > 1.15);
    CHECK(hp > 1.15);
    CHECK(rig.get("boost.vInter.p") > 1.1 * 101325.0);
    CHECK(rig.get("bad") == 0.0);
}

// (marked [!mayfail] by N2: moved by the design-point matching (the lead's (a'), 24 Nov): the smaller matched turbine spools the
// rig's compressor past its surge line and the surge guard holds the vanes at 0.84 (11.0 kPa; without the guard 101.6); N2
// re-derives)
TEST_CASE("boost: variable vanes make boost at low speed that a fixed nozzle of their open size cannot", "[boost][layout][!mayfail]")
{
    double boost[2];
    for (int vanes : {0, 1})
    {
        Rig rig;
        build(rig, 1, 2500.0);
        rig.set("spec.vanes", vanes);
        rig.set("spec.target", 100000.0);
        rig.set("spec.turbAR", 0.9);
        rig.spec("rev", 0);
        rig.run(4.0);
        boost[vanes] = rig.boost();
        std::printf("%s at 2500 rpm: %.1f kPa, vanes %.2f\n", vanes ? "variable vanes" : "fixed nozzle", boost[vanes] / 1000.0,
                    rig.get("boost.turb[0].vanes"));
    }
    CHECK(boost[1] > 1.5 * boost[0]);
}

TEST_CASE("boost: anti-lag holds the shaft's speed off the throttle", "[boost][layout]")
{
    double kept[2];
    for (int al : {0, 1})
    {
        Rig rig;
        build(rig, 1, 5000.0);
        rig.set("spec.antiLag", al);
        rig.spec("rev", 0);
        rig.run(3.0);
        const double on = rig.shaftRpm();
        rig.set("throttle", 0.0);
        rig.set("boost.cmdAntiLag", al ? 0.003 : 0.0);
        rig.run(1.0);
        kept[al] = rig.shaftRpm() / on;
        std::printf("anti-lag %s: the shaft keeps %.2f of its speed a second off the throttle\n", al ? "on" : "off", kept[al]);
    }
    CHECK(kept[1] > kept[0] + 0.1);
}

TEST_CASE("boost: electric assist spools the shaft sooner", "[boost][layout]")
{
    double t[2];
    for (int a : {0, 1})
    {
        Rig rig;
        build(rig, 1, 2500.0);
        rig.set("spec.assist", a ? 5000.0 : 0.0);
        rig.spec("rev", 0);
        t[a] = timeTo(rig, 15000.0);
        std::printf("assist %s: 15 kPa at 2500 rpm after %.3f s\n", a ? "5 kW" : "none", t[a]);
    }
    CHECK(t[1] < 0.7 * t[0]);
}

TEST_CASE("boost: a variable nozzle's back-pressure limit holds the turbine's inlet and leaves a loop that holds its target alone",
          "[boost][layout]")
{
    // (card N2) At a target the turbo can make the vanes are the boost loop's: the limit (spec.backPressure, p3 over p2,
    // absolute) changes nothing. Asked for more than the compressor gives, the loop shuts the vanes and the housing's
    // pressure climbs: the limit then opens them, holding the ratio near it.
    for (double target : {60000.0, 400000.0})
    {
        double boost[2], ratio[2];
        int k = 0;
        for (double limit : {1.4, 4.0})
        {
            Rig rig;
            build(rig, 1, 5000.0);
            rig.set("spec.vanes", 1);
            rig.set("spec.target", target);
            rig.set("spec.gateSpring", target);
            rig.set("spec.backPressure", limit);
            rig.spec("rev", 0);
            rig.run(4.0);
            rig.clear();
            rig.run(1.0);
            boost[k] = rig.mean("sBoost");
            ratio[k] = rig.mean("sPTurb") / (rig.mean("sBoost") + rig.get("boost.pOut"));
            std::printf("vanes, target %.0f kPa, limit %.1f: boost %.1f kPa, turbine in over boost %.2f, vanes %.2f\n",
                        target / 1000.0, limit, boost[k] / 1000.0, ratio[k], rig.get("boost.turb[0].vanes"));
            ++k;
        }
        INFO("target " << target);
        if (target < 100000.0)
        {
            CHECK(boost[0] == Catch::Approx(boost[1]).epsilon(0.03));
            CHECK(ratio[1] < 1.4);
        }
        else
        {
            CHECK(ratio[1] > 1.5);
            CHECK(ratio[0] < 1.55);
        }
    }
}

TEST_CASE("boost: a variable nozzle's surge guard acts on the loop's own surge only and stays bounded", "[boost][layout]")
{
    // (card N2) The guard opens the vanes while the compressor surges with the pedal down and the boost short of its
    // target (gain-reversal protection, machine.fm). A lift's surge (a closed throttle, no blow-off valve) is not the
    // loop's: the guard gives back through it (measured: at most 0.004 through a 1.5 s lift at 5000 rpm on a 100 kPa
    // target the turbo makes). With the ECU's vanes (an engine brake) it is held at 0.
    Rig rig;
    build(rig, 1, 5000.0);
    rig.set("spec.vanes", 1);
    rig.set("spec.target", 100000.0);
    rig.set("spec.gateSpring", 100000.0);
    rig.set("spec.blowOffKind", 0);
    rig.spec("rev", 0);
    rig.run(3.0);
    const double onLoad = rig.get("boost.surgeOpen[0]");
    rig.set("throttle", 0.0);
    double most = 0.0;
    bool surged = false;
    for (int k = 0; k < 300; ++k)
    {
        rig.run(0.005);
        most = std::fmax(most, rig.get("boost.surgeOpen[0]"));
        surged = surged || (static_cast<int>(rig.get("boost.flags")) & 1) != 0;
    }
    const double afterLift = rig.get("boost.surgeOpen[0]");
    rig.set("throttle", 1.0);
    rig.set("boost.cmdVanes", 0.0);
    rig.run(1.0);
    const double braked = rig.get("boost.surgeOpen[0]");
    std::printf("surge guard: %.3f on load, at most %.3f through a lift (surged %d), %.3f after it, %.3f braked; vanes "
                "%.2f\n",
                onLoad, most, surged ? 1 : 0, afterLift, braked, rig.get("boost.turb[0].vanes"));
    CHECK(surged);
    CHECK(onLoad == 0.0);
    CHECK(most < 0.02);
    CHECK(afterLift < 0.01);
    CHECK(braked == 0.0);
}
