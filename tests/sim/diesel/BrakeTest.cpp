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

namespace
{
    struct VgtBrake
    {
        double kw = 0.0;            // retarding, over the last 2 s (settled)
        double man = 0.0;           // the manifold's mean over the same 2 s, Pa abs
        double shaftMost = 0.0;     // the shaft's most over the brake's whole engagement, its rise included, rad/s
        double omegaMax = 0.0;
        double cylinders = 0.0;     // cylinders braking at the end
        double cylMin = 99.0;       // the fewest braking over the engagement
        double shaftLate = 0.0;     // the shaft's most over the last 2 s, rad/s
    };

    // card F's machine (boost.model 1), one turbo with variable vanes, and card G's exhaust network (exhaust.model 1: the
    // turbine's back pressure reaches the cylinders). A brake after a climb, as a truck meets one: held warm
    // (host/include/tee/Warm.h), 5 s floored at the speed with the exhaust's and the cooler's heat settled 1 s in, then
    // the pedal up and the brake at `stage` for `seconds`. inducer: the compressor's wheel (0: matched to the engine).
    // (Straight after a cold start the turbine has a cold exhaust's gas and no fuel to warm it, and on the valves' true
    // mass (G's 15fcdd84) it spooled to some 7000 rad/s: 247 kW at 183 kPa, a cold engine's brake, not a truck's)
    VgtBrake vgtBrake(int stage, double inducer, double rpm, double seconds)
    {
        Engine e("truck-15l-six");
        e.spec("exhaust.model", 1);
        e.spec("boost.model", 1);
        e.spec("boost.kind", 1);
        e.spec("boost.layout", 1);
        e.spec("boost.vanes", 1);
        e.spec("boost.gateSpring", 220000.0);
        e.spec("boost.target", 260000.0);   // (the fixture's boost target: the tractor archetype's, the same 15 L class on the same valve events; that archetype's judgement. A VGT on the spring law with no target limit-cycles, N2: nothing shipped uses it)
        e.spec("boost.size", 1.0);   // (the matched size: 1.6 until G closed the exhaust's mean flow (aaedbf4))
        if (inducer > 0.0)
            e.spec("boost.compInducer", inducer);
        e.spec("ecu.brakeSwitch", stage);
        e.start();
        auto play = [&](double sec) {
            const int calls = static_cast<int>(sec * e.rate / 512.0 + 0.5);
            for (int c = 0; c < calls; ++c)
            {
                e.holdWarm();
                REQUIRE(e.run(512.0 / e.rate));
            }
        };
        e.dyno(rpm, 1.0f);
        play(1.0);
        e.spec("exhaust.heatSettle", 1);
        e.spec("boost.heatSettle", 1);
        play(4.0);
        e.dyno(rpm, 0.0f);
        VgtBrake b;
        b.omegaMax = e.get("boost.mach.omegaMax[0]");
        const int calls = static_cast<int>(e.rate / 512.0 + 0.5);    // (a second, every 512 samples)
        const int total = static_cast<int>(seconds + 0.5) * calls;
        double tq = 0.0, man = 0.0;
        int n = 0;
        for (int q = 0; q < total; ++q)
        {
            e.holdWarm();
            REQUIRE(e.run(512.0 / e.rate));
            b.shaftMost = std::max(b.shaftMost, e.tel("boost.shaftSpeed[0]"));
            if (q > calls / 2)
                b.cylMin = std::min(b.cylMin, e.tel("ecu.brakeCylinders"));
            if (q >= total - 2 * calls)
            {
                b.shaftLate = std::max(b.shaftLate, e.tel("boost.shaftSpeed[0]"));
                tq += e.tel("engine.torque");
                man += e.tel("engine.manifold");
                ++n;
            }
        }
        b.kw = -tq / n * rpm * 0.10471976 / 1000.0;
        b.man = man / n;
        b.cylinders = e.tel("ecu.brakeCylinders");
        std::printf("VGT, brake %d at %.0f rpm, wheel %s: settled %.0f kW, manifold %.0f kPa abs, %.0f cylinders braking (%.0f "
                    "at the fewest); the shaft at most %.0f of omegaMax %.0f rad/s (%.3f), %.3f over the last 2 s\n",
                    stage, rpm, inducer > 0.0 ? "small" : "matched", b.kw, b.man / 1000.0, b.cylinders, b.cylMin, b.shaftMost,
                    b.omegaMax, b.shaftMost / b.omegaMax, b.shaftLate / b.omegaMax);
        return b;
    }
} // namespace

TEST_CASE("diesel brake: with a variable-vane turbo the brake shuts the vanes and the manifold reaches 3 bar", "[diesel][brake][turbo]")
{
    // The brake on High at 2000 rpm puts the turbo in its brake position (the vanes the brake's to command, the gate held
    // shut) and holds the shaft under its limit, as a production brake mode does (heavy.fm vaneCommand: the vanes first,
    // then cylinders held off). Its power is read settled, over the last 2 s of 12 s braking: the bound is a published
    // steady figure. Its shaft is read over the whole engagement, the rise after the climb included.
    // (History: marked [!mayfail] 24-25 Nov after G's 15fcdd84, measured straight after a cold start, 247-289 kW; read
    // over one second 5 s in until 25 Nov, 365 kW at 319 kPa; the shaft's limit broken on N2's 74.7 mm wheel (1.048,
    // the relief) and the matched one at 2400 rpm (1.050) until the brake gave cylinders up, 25 Nov; on the diesel
    // package (N2's wheel and nozzle, the truck's own valve events, a 260 kPa target) its vanes fed forward by engine
    // speed hold it: 359 kW settled, 0.972 at most, the fired shaft as the brake engages.)
    const VgtBrake off = vgtBrake(0, 0.0, 2000.0, 4.0);
    const VgtBrake high = vgtBrake(3, 0.0, 2000.0, 12.0);
    // (250 kPa: the lead's ruling, 24 Nov, on N2's point that a matched variable-vane brake runs some 2.5 to 4 bar in the
    // manifold at 2000 rpm, recalled from makers' figures, not cited)
    CHECK(high.man >= 250000.0);
    CHECK(high.shaftMost <= high.omegaMax);
    // published 15 L brakes ~450 kW at 2100 rpm, steady (above)
    CHECK(high.kw > off.kw + 150.0);
    CHECK(high.kw > 330.0);
    CHECK(high.kw < 550.0);
}

TEST_CASE("diesel brake: where the vanes wide open cannot hold the shaft, the brake gives up cylinders and brings it back under its limit", "[diesel][brake][turbo]")
{
    // The brake's last hand (heavy.fm vaneCommand (c)): on the package's wheel it never acts, the vanes hold the shaft.
    // A compressor wheel too small for the brake's flow (64 mm against the rule's 69.8, its limit higher with it) lets the
    // brake drive the shaft past 0.985 of its limit with the vanes wide (1.048 at 1.1 s, 25 Nov): the brake drops
    // cylinders, and the shaft stands under the limit once it has. No test hook: a mismatched build's physics
    const VgtBrake b = vgtBrake(3, 0.064, 2000.0, 12.0);
    CHECK(b.shaftMost > 0.985 * b.omegaMax);   // (it is reached: the case tests the cut, not the vanes)
    CHECK(b.cylMin < 6.0);
    CHECK(b.shaftLate < b.omegaMax);
}

TEST_CASE("diesel brake, full: the variable-vane brake holds the shaft's limit at 1500, 2000 and 2400 rpm", "[diesel][brake][turbo][nightly]")
{
    // 25 s on High at each speed, the wheel N2's rule picks: the shaft at or under its limit over the whole
    // engagement; the settled power printed (25 Nov, the diesel package: 288 / 359 / 331 kW, the shaft at most 0.957 /
    // 0.972 / 0.713 of its limit, those two the fired shaft at the moment the brake engages)
    for (const double rpm : {1500.0, 2000.0, 2400.0})
    {
        const VgtBrake b = vgtBrake(3, 0.0, rpm, 25.0);
        CHECK(b.shaftMost <= b.omegaMax);
        if (rpm == 2000.0)
            CHECK(b.kw > 330.0);
    }
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
