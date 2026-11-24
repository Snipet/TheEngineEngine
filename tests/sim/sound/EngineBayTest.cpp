// tests/sim/sound/EngineBayTest.cpp: the fuel pump and the injectors (fuel.fm), the lifters and the float
// (valvegear.fm), the wastegate (gate.fm), the starter (starter.fm); and every voice's start and stop without a click,
// and a soak.
#include "Rig.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <random>

using namespace tee::test::sound;

namespace
{
    void near(const std::vector<float>& x, double f, double tol)
    {
        const double got = strongest(x, f * 0.98, f * 1.02, f * 0.0005);
        INFO("expected " << f << " Hz, the strongest line near it at " << got << " Hz");
        CHECK(std::fabs(got - f) <= tol * f);
    }
}

TEST_CASE("pump: a prime at key-on for its seconds, then quiet until the engine turns; its lines", "[sound][fuel]")
{
    Rig rig;
    rig.set("ignition", 0);
    rig.set("state", 0);
    rig.set("crankHz", 0.0);
    CHECK(rms(rig.run(0.5, Tank)) == 0.0);
    rig.set("ignition", 1);
    const auto x = rig.run(3.0, Tank);
    const std::size_t s = static_cast<std::size_t>(kRate * 0.1);
    CHECK(rms(x, s, 2 * s) > 1e-4);                                     // priming at 0.1 s
    CHECK(rms(x, static_cast<std::size_t>(kRate * 2.5)) == 0.0);       // stopped by 2.5 s (2 s, run down in 0.1)
    // its whine: 41 vanes at its speed under pressure (110 less 15: 95 turns a second); its commutator at 8 a turn
    const std::vector<float> held(x.begin() + static_cast<long>(kRate * 0.9), x.begin() + static_cast<long>(kRate * 1.9));
    near(held, 41.0 * 95.0, 0.005);
    near(held, 8.0 * 95.0, 0.005);
    // the engine turns: it runs on
    rig.set("state", 2);
    rig.set("crankHz", 13.33);
    rig.run(0.5, Tank);
    CHECK(rms(rig.run(0.5, Tank)) > 1e-4);
    // a longer prime
    Rig five;
    five.set("ignition", 0);
    five.set("state", 0);
    five.set("spec.prime", 5.0);
    five.run(0.1, Tank);
    five.set("ignition", 1);
    const auto y = five.run(6.0, Tank);
    CHECK(rms(y, static_cast<std::size_t>(kRate * 4.5), static_cast<std::size_t>(kRate * 4.9)) > 1e-4);
    CHECK(rms(y, static_cast<std::size_t>(kRate * 5.5)) == 0.0);
}

TEST_CASE("injectors: direct injection ticks at every injection, opening and closing; port injection adds none", "[sound][fuel]")
{
    Rig port;
    CHECK(rms(port.run(0.5, Head)) == 0.0);
    Rig di;
    di.set("spec.injection", 1);
    const auto x = di.run(1.0, Head);
    CHECK(rms(x) > 1e-5);
    // 8 cylinders, one injection a cylinder every 720 degrees at 13.33 turns a second: 53 a second
    const double n = di.get("directTicks");
    CHECK(std::fabs(n - 8.0 * 13.33 / 2.0) <= 1.0);
    // a diesel keeps the reference's
    Rig diesel;
    diesel.set("spec.injection", 1);
    diesel.set("diesel", 1);
    CHECK(rms(diesel.run(0.5, Head)) == 0.0);
}

TEST_CASE("lifters: a solid lifter ticks, harder with its lash and its speed; hydraulic lifters add none", "[sound][valves]")
{
    Rig hyd;
    CHECK(rms(hyd.run(0.5, Head)) == 0.0);
    double last = 0.0;
    for (double lash : {0.00015, 0.00025, 0.0005})
    {
        Rig rig;
        rig.set("spec.lifters", 1);
        rig.set("spec.lash", lash);
        const double a = rms(rig.run(1.0, Head));
        INFO("lash " << lash << ": " << a);
        CHECK(a > last * 1.2);
        last = a;
    }
    // its taps' rate follows the crank: twice the speed, twice the taps a second (some 3 dB more power)
    Rig slow, fast;
    slow.set("spec.lifters", 1);
    fast.set("spec.lifters", 1);
    fast.set("crankHz", 26.66);
    CHECK(db(rms(fast.run(1.0, Head)), rms(slow.run(1.0, Head))) > 2.0);
}

TEST_CASE("float: with card E's clatter each landing bounces twice; none without it", "[sound][valves]")
{
    Rig calm;
    calm.run(1.0, Head);
    CHECK(calm.get("bounces") == 0.0);
    Rig rig;
    rig.set("clatter", 0.5);
    rig.run(1.0, Head);
    // 16 landings a cycle at 6.665 cycles a second: 107, each two bounces
    const double b = rig.get("bounces");
    INFO("bounces " << b);
    CHECK(std::fabs(b - 2.0 * 16.0 * 6.665) <= 4.0);
    Rig hard;
    hard.set("clatter", 1.0);
    hard.run(1.0, Head);
    CHECK(hard.get("bounceSum") > 1.8 * rig.get("bounceSum"));
}

TEST_CASE("wastegate: an internal gate chatters only part open, more with the exhaust's flow; an external one dumps", "[sound][gate]")
{
    auto level = [](int asp, int external, int toAir, double open, double flow) {
        Rig rig;
        rig.set("crankHz", 50.0);
        rig.set("asp", asp);
        rig.set("gateExternal", external);
        rig.set("spec.gateDump", toAir);
        rig.set("gateOpen", open);
        rig.set("flowShare", flow);
        rig.run(0.1, Gate);
        return rms(rig.run(0.5, Gate));
    };
    CHECK(level(0, 0, 0, 0.5, 1.0) == 0.0);            // no turbo
    CHECK(level(1, 0, 0, 0.0, 1.0) == 0.0);            // shut
    CHECK(level(1, 0, 0, 1.0, 1.0) == 0.0);            // wide open, held
    const double half = level(1, 0, 0, 0.5, 1.0);
    CHECK(half > 1e-5);
    CHECK(level(1, 0, 0, 0.5, 0.3) < half * 0.5);
    CHECK(level(1, 0, 0, 0.15, 1.0) < half);
    CHECK(level(1, 1, 0, 0.5, 1.0) == 0.0);            // an external gate into the exhaust: card G's pipe
    double last = 0.0;
    for (double flow : {0.2, 0.5, 1.0})
    {
        const double a = level(1, 1, 1, 0.8, flow);
        CHECK(a > last * 1.2);
        last = a;
    }
}

TEST_CASE("starter: the pinion on the ring at 130 a turn, a reduction starter's armature at 47, the bigger the louder", "[sound][starter]")
{
    for (int kind : {0, 1})
    {
        Rig rig;
        rig.set("crankHz", 3.33);
        rig.set("state", 1);
        rig.set("spec.starter", kind);
        rig.set("starterOn", 1.0);
        rig.run(0.1, Starter);
        const auto x = rig.run(1.0, Starter);
        near(x, 130.0 * 3.33, 0.005);
        // the commutator's 20 segments on the armature: 13 times the crank, 3.6 times that with the reduction
        near(x, 20.0 * 13.0 * 3.33 * (kind != 0 ? 3.6 : 1.0), 0.005);
    }
    auto level = [](double power) {
        Rig rig;
        rig.set("crankHz", 3.33);
        rig.set("spec.starterPower", power);
        rig.set("starterOn", 1.0);
        rig.run(0.1, Starter);
        return rms(rig.run(0.5, Starter));
    };
    CHECK(db(level(2400.0), level(1200.0)) > 2.5);       // sqrt: 3 dB
    Rig off;
    CHECK(rms(off.run(0.5, Starter)) == 0.0);
}

TEST_CASE("starter: the solenoid's knock and the pinion's strike at the key; a run-down after it", "[sound][starter]")
{
    Rig rig;
    rig.set("crankHz", 3.33);
    rig.run(0.1, Starter);
    rig.set("starterOn", 1.0);
    const auto on = rig.run(0.5, Starter);
    // the strike's ring at 2.6 kHz in the first 20 ms, above the steady cranking's level there
    std::vector<float> first(on.begin(), on.begin() + 1920), later(on.begin() + 38400, on.begin() + 40320);
    CHECK(line(first, 2600.0) > 3.0 * line(later, 2600.0));
    rig.set("starterOn", 0.0);
    rig.set("crankHz", 13.33);
    const auto off = rig.run(2.0, Starter);
    CHECK(rms(off, 0, 9600) > 1e-4);                             // running down
    CHECK(rms(off, off.size() - 9600) < rms(off, 0, 9600) * 1e-2);
}

TEST_CASE("clicks: no voice steps when it starts or stops", "[sound][clicks]")
{
    // each voice's largest step between samples as it comes in and goes out, against its largest in the steady state
    struct Case { const char* name; const char* field; double on; double off; int channel; };
    const Case cases[] = {
        {"whine", "torque", 400.0, 0.0, Gearbox},
        {"belt", "spec.belt", 0.0, -60.0, Belt},
        {"cam gears", "spec.timingLevel", 0.0, -60.0, Timing},
        {"fan", "spec.fanLevel", 0.0, -60.0, Fan},
        {"pump", "spec.pump", 0.0, -60.0, Tank},
        {"all", "spec.level", 0.0, -60.0, All},
    };
    for (const auto& c : cases)
    {
        Rig rig;
        rig.inGear(3, 300.0, 0.0);
        rig.set("spec.timing", 2);
        rig.set("coolant", 380.0);
        rig.set(c.field, c.off);
        rig.run(1.0, c.channel);
        rig.set(c.field, c.on);
        const auto in = rig.run(0.1, c.channel);
        const auto held = rig.run(0.5, c.channel);
        rig.set(c.field, c.off);
        const auto out = rig.run(0.1, c.channel);
        const double steadyStep = maxStep(held);
        INFO(c.name << ": steady " << steadyStep << ", in " << maxStep(in, 1, 64) << ", out " << maxStep(out, 1, 64));
        CHECK(steadyStep > 0.0);
        CHECK(maxStep(in, 1, 64) <= 1.05 * steadyStep);
        CHECK(maxStep(out, 1, 64) <= 1.05 * steadyStep);
    }
    // the starter: its motor eased in over 3 ms (its solenoid's knock aside: a knock is meant to be one)
    Rig st;
    st.set("crankHz", 3.33);
    st.set("spec.starterPower", 1200.0);
    st.run(0.1, Starter);
    st.set("starterOn", 1.0);
    const auto x = st.run(0.5, Starter);
    CHECK(maxStep(x, 1, 3) <= 1.05 * maxStep(x, 38400, 48000) + 1e-9);
}

namespace
{
void soak(int seconds)
{
    Rig rig;
    rig.set("spec.timing", 2);
    rig.set("spec.lifters", 1);
    rig.set("spec.injection", 1);
    rig.set("spec.gateDump", 1);
    rig.set("spec.blowerDrive", 1);
    std::mt19937 gen(12345);
    std::uniform_real_distribution<double> u(0.0, 1.0);
    for (int step = 0; step < seconds; ++step)
    {
        const int g = static_cast<int>(u(gen) * 8.0) - 1;
        rig.inGear(g > 6 ? 6 : g, 50.0 + 700.0 * u(gen), 1.5 * u(gen) - 0.3);
        rig.set("boxKind", static_cast<int>(u(gen) * 5.0));
        rig.set("ripple", 0.06 * u(gen));
        rig.set("engaged", u(gen));
        rig.set("coolant", 300.0 + 100.0 * u(gen));
        rig.set("asp", static_cast<int>(u(gen) * 5.0));
        rig.set("gateExternal", u(gen) > 0.5 ? 1 : 0);
        rig.set("gateOpen", u(gen));
        rig.set("flowShare", 1.5 * u(gen));
        rig.set("boostShare", u(gen));
        rig.set("clatter", u(gen) > 0.7 ? u(gen) : 0.0);
        rig.set("starterOn", u(gen) > 0.9 ? 1.0 : 0.0);
        rig.set("ignition", u(gen) > 0.1 ? 1 : 0);
        rig.set("spec.fan", static_cast<int>(u(gen) * 3.0));
        rig.run(1.0, All);
    }
    INFO("peak " << rig.get("peakAbs"));
    CHECK(rig.get("finite") == 1.0);
    CHECK(rig.get("peakAbs") < 1.0);
}
}

TEST_CASE("soak: a minute of every voice under random inputs stays finite and within bounds", "[sound][soak]")
{
    soak(60);
}

TEST_CASE("soak: ten minutes of every voice under random inputs stays finite and within bounds", "[sound][soak][nightly]")
{
    soak(600);
}

TEST_CASE("robust: a NaN or an infinity in any input reads no table out of bounds, and the voices come back after it",
          "[sound][robust]")
{
    // card X's X-019: a NaN crank speed (from a build at the Spec's corners) gave the sine table the index INT_MIN
    const double nan = std::nan("");
    const double inf = HUGE_VAL;
    for (const char* field : {"crankHz", "wIn", "wSprocket", "ratioTotal", "torque", "coolant", "gateOpen", "flowShare"})
        for (double bad : {nan, inf, -inf})
        {
            Rig rig;
            rig.inGear(3, 300.0, 0.5);
            rig.set("spec.timing", 2);
            rig.set("spec.lifters", 1);
            rig.set("spec.injection", 1);
            rig.set("asp", 1);
            rig.set("gateOpen", 0.5);
            rig.set("flowShare", 1.0);
            rig.set("bike", 1);
            rig.set("wSprocket", 100.0);
            rig.set("dryClutch", 1);
            rig.set("starterOn", 1.0);
            rig.set("coolant", 380.0);
            rig.run(0.1, All);
            const double before = rig.get(field);
            rig.set(field, bad);
            rig.run(0.1, All);
            rig.set(field, before);
            rig.run(0.2, All);
            const auto after = rig.run(0.3, All);
            bool finite = true;
            for (float v : after)
                finite = finite && std::isfinite(v);
            INFO(field << " = " << bad);
            CHECK(finite);
            CHECK(rms(after) > 0.0);
        }
}

TEST_CASE("robust: every voices field at a corner of its range, under extreme but finite inputs, stays finite", "[sound][robust]")
{
    // (the lead's request after X-019: no corner of the declared ranges may make a voice non-finite from finite inputs)
    std::mt19937 gen(19);
    std::uniform_real_distribution<double> u(0.0, 1.0);
    for (int seed = 0; seed < 24; ++seed)
    {
        Rig rig;
        rig.corners([&] { return u(gen) > 0.5; });
        auto ends = [&](double lo, double hi) { return u(gen) > 0.5 ? hi : lo; };
        const int g = static_cast<int>(u(gen) * 8.0) - 1;
        rig.inGear(g > 6 ? 6 : g, ends(0.0, 1200.0), ends(-1.5, 1.5));
        rig.set("ratioTotal", ends(0.0, 240.0));
        rig.set("fd", ends(1.0, 12.0));
        rig.set("wOut", ends(0.0, 2000.0));
        rig.set("boxKind", static_cast<int>(u(gen) * 5.0));
        rig.set("ripple", ends(0.0, 0.5));
        rig.set("engaged", ends(0.0, 1.0));
        rig.set("coolant", ends(230.0, 420.0));
        rig.set("asp", static_cast<int>(u(gen) * 5.0));
        rig.set("gateExternal", u(gen) > 0.5 ? 1 : 0);
        rig.set("gateOpen", ends(0.0, 1.0));
        rig.set("flowShare", ends(0.0, 1.5));
        rig.set("boostShare", ends(0.0, 1.0));
        rig.set("clatter", ends(0.0, 1.0));
        rig.set("starterOn", ends(0.0, 1.0));
        rig.set("cylinders", ends(1.0, 16.0));
        rig.set("displacement", ends(5e-5, 0.03));
        rig.set("diesel", u(gen) > 0.5 ? 1 : 0);
        rig.set("bike", u(gen) > 0.5 ? 1 : 0);
        rig.set("wSprocket", ends(0.0, 800.0));
        rig.set("frontTeeth", ends(10.0, 30.0));
        rig.set("primaryRatio", ends(1.0, 4.0));
        rig.set("dryClutch", u(gen) > 0.5 ? 1 : 0);
        rig.set("lever", ends(0.0, 1.0));
        rig.set("strike", ends(0.0, 50.0));
        const auto x = rig.run(0.5, All);
        bool finite = true;
        for (float v : x)
            finite = finite && std::isfinite(v);
        INFO("seed " << seed);
        CHECK(finite);
        CHECK(rig.get("finite") == 1.0);
    }
}
