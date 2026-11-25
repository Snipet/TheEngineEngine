// tests/sim/diesel/BrakeTest.cpp: card D2: the compression-release brake and the exhaust brake on the engine dyno, in
// the whole simulator, the 15 L truck six (content/builds/truck-15l-six.json; air at 300 K, 101325 Pa).
#include "Diesel.h"

#include <cstdio>

using d2::Engine;

namespace
{
    // The brake's retarding power (W) at rpm with the switch at stage (0: none: the engine motoring, fuel off), and the
    // exhaust brake on or off: the dyno holds the speed, the pedal up; the mean brake torque over the last second
    double retard(int stage, double rpm, int exhaustBrake = 0, double* backOut = nullptr)
    {
        Engine e("truck-15l-six");
        e.spec("ecu.brakeSwitch", stage);
        if (exhaustBrake != 0)
        {
            e.spec("ecu.exhaustBrake", exhaustBrake);
            e.spec("ecu.exBrakeSwitch", 1);
        }
        e.start();
        e.dyno(rpm, 0.0f);
        REQUIRE(e.run(4.0));
        const double tq = e.mean("engine.torque", 1.0);
        if (backOut != nullptr)
            *backOut = e.tel("ecu.backPressure");
        std::printf("stage %d, exhaust brake %d, %.0f rpm: brake torque %.0f N m, %.0f kW retarding, boost %.0f kPa, back %.0f kPa, "
                    "braking cylinders %.0f\n",
                    stage, exhaustBrake, rpm, tq, -tq * rpm * 0.10471976 / 1000.0, e.tel("boost.pressure") / 1000.0,
                    e.tel("ecu.backPressure") / 1000.0, e.tel("ecu.brakeCylinders"));
        return -tq * rpm * 0.10471976;
    }
} // namespace

TEST_CASE("diesel brake: the compression brake retards, on High far more than motoring, and less at lower speed", "[diesel][brake]")
{
    // the quick form of the next: High against motoring at 2000 rpm, and High at 1200 rpm
    const double motoring = retard(0, 2000.0);
    const double high = retard(3, 2000.0);
    CHECK(high > 3.0 * motoring);
    CHECK(high > 200000.0);
    CHECK(high < 550000.0);
    CHECK(retard(3, 1200.0) < high);
}

TEST_CASE("diesel brake, full: the compression brake's retarding power rises with speed and with stages", "[diesel][brake][nightly]")
{
    // Published: Jacobs' brakes on 15 L engines (Cummins ISX15, Detroit DD15) are rated some 450 kW (600 hp) at 2100
    // rpm (makers' brochures; Jacobs' "up to 600 bhp"); at 1500 rpm the published curves give some 250 to 300 kW. The
    // range asserted, 200 to 550 kW at 2000 rpm, is judgement around those.
    const double motoring = retard(0, 2000.0);
    double p[4] = {motoring, 0.0, 0.0, 0.0};
    for (int s = 1; s <= 3; ++s)
        p[s] = retard(s, 2000.0);
    for (int s = 1; s <= 3; ++s)
        CHECK(p[s] > p[s - 1] * 1.15);
    CHECK(p[3] > 200000.0);
    CHECK(p[3] < 550000.0);
    const double low = retard(3, 1200.0);
    const double mid = retard(3, 1600.0);
    CHECK(low < mid);
    CHECK(mid < p[3]);
}

TEST_CASE("diesel brake: the bark: lines at the firing frequency, louder than the engine motoring", "[diesel][brake][sound]")
{
    // 2000 rpm, six cylinders: a release every 120 degrees, 100 Hz, and its harmonics. The same engine motoring with the
    // fuel off (stage 0) is the comparison: its exhaust events push out air at about the manifold's pressure. The exhaust
    // alone (the intake's and the mechanical sources' levels 0), the level at -24 dB, below the mix's compressor (it
    // starts at -30 dB of envelope) and its limiter, so levels compare
    const double rpm = 2000.0;
    const double fire = rpm / 60.0 * 3.0;
    double rms[2] = {0.0, 0.0};
    double harm[2] = {0.0, 0.0};     // the mean of harmonics 2 to 10 (dB)
    double gap[2] = {0.0, 0.0};      // the mean of the bins half way between them
    for (int k = 0; k < 2; ++k)
    {
        Engine e("truck-15l-six");
        e.spec("sound.intake", 0.0);
        e.spec("sound.mechanical", 0.0);
        e.spec("sound.level", -24.0);
        e.spec("ecu.brakeSwitch", k == 0 ? 0 : 3);
        e.start();
        e.dyno(rpm, 0.0f);
        REQUIRE(e.run(3.0));
        e.recording = true;
        REQUIRE(e.run(1.0));
        rms[k] = d2::rmsDb(e.rec);
        std::printf("%s at %.0f rpm: rms %.1f dB; harmonics of %.0f Hz (line / between):", k == 0 ? "motoring" : "compression brake",
                    rpm, rms[k], fire);
        for (int h = 1; h <= 10; ++h)
        {
            const double on = d2::lineDb(e.rec, e.rate, fire * h, 2.0);
            const double off = d2::lineDb(e.rec, e.rate, fire * (h + 0.5), 2.0);
            std::printf(" %.0f/%.0f", on, off);
            if (h >= 2)
            {
                harm[k] += on / 9.0;
                gap[k] += off / 9.0;
            }
        }
        std::printf("\n");
    }
    // measured (this model): +8.4 dB rms; the harmonics some 25 dB up, the fundamental itself about level (the release
    // and the exhaust stroke's push 250 degrees apart partly cancel at 100 Hz)
    CHECK(rms[1] > rms[0] + 6.0);
    CHECK(harm[1] > harm[0] + 15.0);
    CHECK(harm[1] > gap[1] + 15.0);
}

TEST_CASE("diesel brake: the exhaust brake holds its back pressure and retards", "[diesel][brake]")
{
    double back = 0.0;
    const double flap = retard(0, 2000.0, 1, &back);
    CHECK(back > 0.8 * 300000.0);
    CHECK(back <= 300001.0);
    CHECK(flap > 56000.0 + 40000.0);     // (motoring at 2000 rpm: 56 kW, the full form measures it)
}

TEST_CASE("diesel brake, full: the exhaust brake holds its back pressure and retards, alone and with the compression brake",
          "[diesel][brake][nightly]")
{
    // A regulated flap holds some 2.5 to 4 bar (makers' figures: Jacobs' and Pacbrake's exhaust brakes on 15 L engines
    // retard some 150 to 250 kW at governed speed); its 3 bar here, at 2000 rpm
    double back = 0.0;
    const double motoring = retard(0, 2000.0);
    const double flap = retard(0, 2000.0, 1, &back);
    const double both = retard(3, 2000.0, 1);
    const double jake = retard(3, 2000.0);
    CHECK(back > 0.8 * 300000.0);
    CHECK(back <= 300001.0);
    CHECK(flap > motoring + 50000.0);
    CHECK(both > jake);
}

TEST_CASE("diesel brake: the driver's button engages it at High with the dash switch Off", "[diesel][brake]")
{
    Engine e("truck-15l-six");
    e.start();
    e.dyno(1800.0, 0.0f);
    REQUIRE(e.run(1.0));
    CHECK(e.tel("ecu.brakeCylinders") == 0.0);
    e.ctl().buttons = 64;   // BTN_ENGINE_BRAKE
    REQUIRE(e.run(1.0));
    CHECK(e.tel("ecu.brakeCylinders") == 6.0);
    e.ctl().buttons = 0;
    REQUIRE(e.run(1.0));
    CHECK(e.tel("ecu.brakeCylinders") == 0.0);
}

TEST_CASE("diesel brake: with a variable-vane turbo the brake shuts the vanes and the manifold reaches 3 bar", "[diesel][brake][turbo]")
{
    // (marked [!mayfail] 24-25 Nov since G's 15fcdd84 (the housings given the valves' true mass): measured straight after
    // a cold start the brake made 247-289 kW at 183-227 kPa. After a climb, warm, on N2's matched turbine (25 Nov):
    // 365 kW, 319 kPa over a second, the shaft at most 9673 of omegaMax 10149, the brake's own vane command at 0.68.)
    // (No cold-start brake beside it: a second engine's run would cost some 5 s of the default tier (the lead's ruling:
    // a print only if under a couple of seconds). What a cold brake read is above, 247-289 kW: braking straight after a
    // start, the turbine has a cold exhaust's gas and no fuel to warm it, on the valves' true mass)
    // (marked [!mayfail] for an hour on 24 Nov: read over one second, the shaft peaked at 12539 rad/s against omegaMax
    // 11932, the brake's PID past 95 % hunting into card F's 105 % relief. The governor rebuilt on the plant's measured
    // response (heavy.fm vaneCommand): the shaft at most 11713 rad/s over the whole run (on the way up), 11090 settled. Plain again.)
    // card F's machine (boost.model 1), one turbo with variable vanes: the compression brake on High at 2000 rpm puts it in
    // its brake position (vanes shut, gate held shut). Real brakes run 3 to 3.5 bar absolute in the manifold there.
    // With card G's exhaust network (exhaust.model 1) the turbine's back pressure reaches the cylinders.
    double kw[2] = {0.0, 0.0};
    const double sizeK = 1.0;    // (the matched size: 1.6 until G closed the exhaust's mean flow (aaedbf4), when a choked release's phantom 45 % of mass over-sped the matched shaft)
    for (int k = 0; k < 2; ++k)
    {
        Engine e("truck-15l-six");
        e.spec("exhaust.model", 1);     // (card G's network: the turbine's back pressure reaches the cylinders)
        e.spec("boost.model", 1);
        e.spec("boost.kind", 1);
        e.spec("boost.layout", 1);
        e.spec("boost.vanes", 1);
        e.spec("boost.gateSpring", 220000.0);
        e.spec("boost.size", sizeK);
        e.spec("ecu.brakeSwitch", k == 0 ? 0 : 3);
        e.start();
        // (a brake after a climb, as a truck meets one: held warm (host/include/tee/Warm.h), 5 s floored at 2000 rpm with
        // the exhaust's and the cooler's heat settled 1 s in, then the pedal up and the brake on. Straight after a cold
        // start the turbine has a cold exhaust's gas and no fuel to warm it, and on the valves' true mass (G's 15fcdd84)
        // it spooled to some 7000 rad/s: 247 kW at 183 kPa, which was a cold engine's brake, not a truck's)
        auto play = [&](double sec) {
            const int calls = static_cast<int>(sec * e.rate / 512.0 + 0.5);
            for (int c = 0; c < calls; ++c)
            {
                e.holdWarm();
                REQUIRE(e.run(512.0 / e.rate));
            }
        };
        e.dyno(2000.0, 1.0f);
        play(1.0);
        e.spec("exhaust.heatSettle", 1);
        e.spec("boost.heatSettle", 1);
        play(4.0);
        e.dyno(2000.0, 0.0f);
        double shaftMost = 0.0;   // (the shaft's most over the brake's whole run, its first moments included)
        for (int q = 0; q < 50; ++q)
        {
            play(0.1);
            shaftMost = std::max(shaftMost, e.tel("boost.shaftSpeed[0]"));
        }
        // (over one second, every 512 samples: the torque's and the manifold's means, the shaft's most. The brake's own
        // shaft governor swings, some 0.6 s a cycle, so an instant's reading lands anywhere on it)
        double tq = 0.0, man = 0.0;
        const int calls = static_cast<int>(e.rate / 512.0 + 0.5);
        for (int q = 0; q < calls; ++q)
        {
            e.holdWarm();
            REQUIRE(e.run(512.0 / e.rate));
            tq += e.tel("engine.torque") / calls;
            man += e.tel("engine.manifold") / calls;
            shaftMost = std::max(shaftMost, e.tel("boost.shaftSpeed[0]"));
        }
        kw[k] = -tq * 2000.0 * 0.10471976 / 1000.0;
        std::printf("VGT, brake %s at 2000 rpm: manifold %.0f kPa abs (now %.0f), vanes %.2f, gate %.2f, shaft at most %.0f rad/s, %.0f kW retarding\n",
                    k == 0 ? "off" : "High", man / 1000.0, e.tel("engine.manifold") / 1000.0, e.tel("boost.vanes[0]"),
                    e.tel("boost.gate[0]"), shaftMost, kw[k]);
        std::printf("  (the shaft's limit, omegaMax: %.0f rad/s)\n", e.get("boost.mach.omegaMax[0]"));
        if (k == 1)
        {
            // (250 kPa: the lead's ruling, 24 Nov, on N2's point that a matched variable-vane brake runs some 2.5 to 4 bar
            // in the manifold at 2000 rpm, recalled from makers' figures, not cited. It was 300 when this case ran an
            // oversized turbo on G's phantom exhaust flow; at the matched size it reads 295 kPa over a second.)
            CHECK(man >= 250000.0);
            // the brake's vane command holds the shaft under its limit (as a production brake mode), not the machine's
            // 105 % relief: 10001 against 9433 rad/s before (N2, 2026-11-24)
            CHECK(shaftMost <= e.get("boost.mach.omegaMax[0]"));
        }
    }
    // published 15 L brakes ~450 kW at 2100 rpm (above); measured 406 kW at 2000 with the manifold at 3.4 bar
    CHECK(kw[1] > kw[0] + 150.0);
    CHECK(kw[1] > 330.0);
    CHECK(kw[1] < 550.0);
}

TEST_CASE("diesel brake: card G's butterfly as the exhaust brake holds its back pressure and retards", "[diesel][brake]")
{
    // exhaust.model 1 with flapKind Butterfly: the flap in the pipes (its relief at 350 kPa) in place of D2's stand-in
    double kw[2] = {0.0, 0.0};
    for (int k = 0; k < 2; ++k)
    {
        Engine e("truck-15l-six");
        e.spec("exhaust.model", 1);
        e.spec("exhaust.flapKind", 1);
        e.spec("ecu.exBrakeSwitch", k);
        e.start();
        e.dyno(2000.0, 0.0f);
        REQUIRE(e.run(4.0));
        const double tq = e.mean("engine.torque", 1.0);
        kw[k] = -tq * 2000.0 * 0.10471976 / 1000.0;
        std::printf("butterfly %s at 2000 rpm: flap %.2f, %.0f kPa before it, stand-in %.0f kPa, %.0f kW retarding\n", k ? "shut" : "open",
                    e.tel("exhaust.flap"), e.tel("exhaust.pFlap") / 1000.0, e.tel("ecu.backPressure") / 1000.0, kw[k]);
        if (k == 1)
        {
            CHECK(e.tel("exhaust.pFlap") > 150000.0);
            CHECK(e.tel("exhaust.pFlap") < 450000.0);
            CHECK(e.tel("ecu.backPressure") < 1.0);
            // every D2 field switched off while it is shut: the flap is released, not left stuck
            e.spec("ecu.exhaustBrake", 0);
            e.spec("ecu.engineBrake", 0);
            e.spec("ecu.governor", 0);
            REQUIRE(e.run(1.0));
            CHECK(e.get("hd.on") == 0.0);
            CHECK(e.tel("exhaust.flap") < 0.05);
        }
    }
    CHECK(kw[1] > kw[0] + 40.0);
}
