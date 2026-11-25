// tests/sim/exhaust/BangTest.cpp: card G's bangs as heat released in the pipe (spec.exhaust.bang 1, the builder's
// PipeBurn and bangIn; docs/models/exhaust.md, "Bangs: heat released in the pipe"). The burns are handed in here as
// the facade hands the ECU account's, synthetically: the account's side (sim/ecu/overrun.fm) waits for its seam.
#include "Rig.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>

using namespace tee::test::exhaust;

namespace
{
    // The V8's builder (layout 12) running at 3000 rpm, bangs burned in the pipe, a slug as a spark-cut charge
    // leaves a 0.6 l cylinder near full load (0.65 g, 700 K, phi 1.15, a tenth burned gas)
    void slug(Rig& rig, double mass = 6.5e-4, double t = 700.0, double phi = 1.15, double xb = 0.1, int own = 0)
    {
        rig.set("bgMass", mass);
        rig.set("bgT", t);
        rig.set("bgPhi", phi);
        rig.set("bgXb", xb);
        rig.set("bgOwn", own);
    }
} // namespace

TEST_CASE("a burn in the pipe releases its heat whole, and nothing without one", "[exhaust][bang]")
{
    std::vector<float> l0, r0, l1, r1;
    {
        Rig rig(12);
        rig.noise(0.0);
        rig.configure();
        std::vector<float> q = pulses(rig, 3000.0, 0.6, 0.15, 0.01);
        rig.run(q, l0, r0);
    }
    Rig rig(12);
    rig.noise(0.0);
    rig.set("spec.bang", 1);
    rig.configure();
    std::vector<float> q = pulses(rig, 3000.0, 0.6, 0.15, 0.01);
    rig.run(q, l1, r1);
    // no burn handed in: the junction's source is the pulse's alone, so the sound is the carved setting's, to the bit
    CHECK(rig.get("b.bangOn") == 1.0);
    bool same = l0.size() == l1.size();
    for (std::size_t i = 0; same && i < l0.size(); ++i)
        same = l0[i] == l1[i] && r0[i] == r1[i];
    CHECK(same);
    CHECK(rig.get("b.bangPeak") == 0.0);

    // a burn of 1 kJ: what it releases plus what the gas carried out unburned is what it was given (the Wiebe curve
    // released by differences: the float's rounding)
    slug(rig);
    rig.call("XTest::bangGo", 0.0f, 1000.0f);
    std::vector<float> l2, r2;
    std::vector<float> q2 = pulses(rig, 3000.0, 0.6, 0.15, 0.01);
    rig.run(q2, l2, r2);
    const double given = rig.get("b.bangGiven");
    const double released = rig.get("b.burn[0].released");
    const double swept = rig.get("b.bangSwept");
    const double tau = rig.get("b.bangTau");
    std::printf("a 1 kJ slug (0.65 g, 700 K, phi 1.15): S_L %.2f m/s, S_T %.2f m/s, %.2f m long, burned in %.1f ms; "
                "released %.2f J, swept out %.2f J\n", rig.get("b.bangSL"), rig.get("b.bangST"), rig.get("b.bangLen"),
                tau * 1e3, released, swept);
    CHECK(given == 1000.0);
    CHECK(std::fabs(released + swept - given) <= 1e-4 * given);
    CHECK(released > 0.0);
    CHECK(rig.get("b.burn[0].dropped") == 0.0);
    // the peak: the Wiebe curve's steepest (2.02 of the mean rate, at 0.51 of its time) on the heat that burned
    const double bound = 2.03 * released / tau;
    std::printf("its peak release %.0f kW against the curve's own bound %.0f kW\n", rig.get("b.bangPeak") / 1e3, bound / 1e3);
    CHECK(rig.get("b.bangPeak") <= bound);
    CHECK(rig.get("b.bangPeak") >= 0.9 * bound);
    bool finite = true;
    for (std::size_t i = 0; i < l2.size(); ++i)
        finite = finite && std::isfinite(l2[i]) && std::isfinite(r2[i]);
    CHECK(finite);
}

TEST_CASE("a slug outside petrol's flammable range does not burn", "[exhaust][bang]")
{
    Rig rig(12);
    rig.noise(0.0);
    rig.set("spec.bang", 1);
    rig.configure();
    std::vector<float> q = pulses(rig, 3000.0, 0.3, 0.15, 0.01);
    std::vector<float> l, r;
    rig.run(q, l, r);
    slug(rig, 6.5e-4, 700.0, 0.4);
    rig.call("XTest::bangGo", 0.0f, 1000.0f);
    rig.run(q, l, r);
    CHECK(rig.get("b.bangSwept") == 1000.0);
    CHECK(rig.get("b.burn[0].released") == 0.0);
    CHECK(rig.get("b.bangPeak") == 0.0);
}

// The burn times the flame speed gives (the report's numbers): a spark-cut charge cool and hot, a late burn's own
// flame, at idle's flow and at a high speed's
TEST_CASE("measure: the burn times of slugs in the collector", "[.][measure]")
{
    for (double rpm : {1000.0, 3000.0, 6000.0})
    {
        Rig rig(12);
        rig.noise(0.0);
        rig.set("spec.bang", 1);
        rig.configure();
        std::vector<float> q = pulses(rig, rpm, 1.0, 0.15 * rpm / 3000.0, 0.01 * rpm / 3000.0);
        std::vector<float> l, r;
        rig.run(q, l, r);
        struct Case
        {
            const char* what;
            double mass, t, phi, xb;
            int own;
        };
        for (const Case& c : {Case{"spark cut, 500 K", 6.5e-4, 500.0, 1.15, 0.1, 0}, Case{"spark cut, 700 K", 6.5e-4, 700.0, 1.15, 0.1, 0},
                              Case{"spark cut, 900 K", 6.5e-4, 900.0, 1.15, 0.1, 0}, Case{"a quarter left, own flame, 900 K", 1.6e-4, 900.0, 1.0, 0.1, 1}})
        {
            slug(rig, c.mass, c.t, c.phi, c.xb, c.own);
            rig.call("XTest::bangGo", 0.0f, 100.0f);
            std::printf("%5.0f rpm (collector Mach %.3f, %.0f K, pulsing %.0f Pa rms): %-34s S_L %.2f, S_T %.2f m/s, %.2f m: %.1f ms, "
                        "%.0f %% before it leaves\n", rpm, rig.get("b.mach[1]"), rig.get("b.heat.tGas[1]"),
                        std::sqrt(rig.get("b.pVar[0]")), c.what, rig.get("b.bangSL"), rig.get("b.bangST"),
                        rig.get("b.bangLen"), rig.get("b.bangTau") * 1e3, rig.get("b.bangShare") * 100.0);
        }
    }
}
