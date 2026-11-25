// tests/sim/diesel/GovernorTest.cpp: card D2: the governors (spec.ecu.governor): the droop line, the hunting idle of a
// mechanical governor against an electronic one's steadiness, the truck six at 300 K, 101325 Pa.
#include "Diesel.h"

#include <algorithm>
#include <cstdio>
#include <functional>
#include <string>

using d2::Engine;

TEST_CASE("diesel governor: the droop line: full rack at the rated speed, none at high idle", "[diesel][governor]")
{
    // The rack on the engine dyno, the pedal down, at speeds across the droop: rated speed (the redline) gives full
    // rack, high idle (rated x (1 + droop)) none, half way half (the electronic governor's droop line, no lag)
    for (int kind : {1, 2})
    {
        Engine e("truck-15l-six");
        e.spec("ecu.governor", kind);
        e.start();
        // (the ECU's redline moves a little as the engine warms: read it after a few seconds at full load)
        e.dyno(e.tel("engine.redline") * 0.9 / 0.10471976, 1.0f);
        REQUIRE(e.run(4.0));
        const double droop = 0.06;
        double rack[3] = {};
        int k = 0;
        for (double x : {-1.1, -0.5, 0.05})
        {
            // (the ECU's redline, the rated speed, moves a little with the engine's state: the governor's high idle read
            // each time, hd.highIdle = rated x (1 + droop))
            const double hi = e.get("hd.highIdle");
            const double rated = hi / (1.0 + droop);
            e.dyno((hi + x * droop * rated) / 0.10471976, 1.0f);
            REQUIRE(e.run(2.0));
            rack[k] = e.mean("ecu.rack", 0.5);
            std::printf("  high idle %.0f rpm; at %.0f rpm, rack %.2f\n", hi / 0.10471976, e.tel("engine.speed") / 0.10471976, rack[k]);
            ++k;
        }
        const double red = e.tel("engine.redline");
        std::printf("%s governor, rated %.0f rpm: rack %.2f at 0.97 of it, %.2f half way up the droop, %.2f past high idle\n",
                    kind == 1 ? "mechanical" : "electronic", red / 0.10471976, rack[0], rack[1], rack[2]);
        CHECK(rack[0] > 0.95);
        CHECK(rack[1] > 0.35);
        CHECK(rack[1] < 0.65);
        CHECK(rack[2] < 0.05);
    }
}

TEST_CASE("diesel governor: a mechanical governor's idle hunts, an electronic one's is steady", "[diesel][governor]")
{
    double spread[3] = {};
    for (int kind : {1, 2})
    {
        Engine e("truck-15l-six");
        e.spec("ecu.governor", kind);
        e.start(2.0, 6.0);
        double lo = 1e9, hi = 0.0, sum = 0.0;
        const int calls = static_cast<int>(4.0 * e.rate / 512.0);
        for (int i = 0; i < calls; ++i)
        {
            REQUIRE(e.run(512.0 / e.rate));
            const double rpm = e.get("ecu.rpmAvg");     // (the ECU's smoothed speed: the firings' ripple out)
            lo = std::min(lo, rpm);
            hi = std::max(hi, rpm);
            sum += rpm;
        }
        spread[kind] = hi - lo;
        std::printf("%s governor at idle: %.0f rpm mean, %.0f to %.0f (%.0f rpm swing)\n", kind == 1 ? "mechanical" : "electronic",
                    sum / calls, lo, hi, hi - lo);
        CHECK(e.tel("engine.state") == 2.0);
    }
    // (a mechanical governor's idle: tens of rpm of slow swing, "galloping"; an electronic one's a few)
    CHECK(spread[1] > 15.0);
    CHECK(spread[1] > 2.0 * spread[2]);
    CHECK(spread[2] < 25.0);
}

TEST_CASE("diesel governor: the boost compensator caps the fuel; the telemetry reads what is injected and its lambda", "[diesel][governor]")
{
    // Floored at 1300 rpm two seconds into the reference turbo's climb the rack is full and the air a little short
    // (the rack asks some phi 1.0): with the compensator (ecu.afc 0.77) the fuel injected is capped at an equivalence
    // ratio of 0.77, lambda 1.30; with it removed (0, the tuner's stop) the rack's whole ask goes in and what burns levels
    // toward phi 1 (measured 25 Nov: 16.3 against 20.2 g/s; lambda 1.25 against 1.11). tel.ecu.fuelFlow is the fuel
    // injected (it read the rack's ask before 25 Nov), tel.ecu.lambda the charge's on the reference's ECU (it read 0)
    double flow[2] = {0.0, 0.0}, lambda[2] = {0.0, 0.0};
    int k = 0;
    const double cap = 0.77;    // (ecu.afc's default, lambda 1.30)
    for (double afc : {cap, 0.0})
    {
        Engine e("truck-15l-six");
        e.spec("ecu.afc", afc);
        e.start();
        e.dyno(1300.0, 1.0f);
        REQUIRE(e.run(2.0));
        CHECK(e.tel("ecu.rack") > 0.99);
        flow[k] = e.mean("ecu.fuelFlow", 0.2);
        lambda[k] = e.mean("ecu.lambda", 0.2);
        ++k;
    }
    std::printf("floored at 1300 rpm, 2 s in: with the compensator %.2f g/s, lambda %.2f; without it %.2f g/s, lambda %.2f\n",
                flow[0] * 1000.0, lambda[0], flow[1] * 1000.0, lambda[1]);
    CHECK(flow[0] < 0.9 * flow[1]);
    CHECK(lambda[0] > 0.98 / cap);
    CHECK(lambda[0] < 1.02 / cap);
    CHECK(lambda[1] > 0.95);
    CHECK(lambda[1] < 1.2);
}

TEST_CASE("diesel governor: the full-load fuel curve shapes the torque; with none set, the flat line to the bit", "[diesel][governor]")
{
    // (1) A stated curve shapes the torque: the truck six with a falling curve (mg a stroke: 700 233, 800 274, 1000 and
    // 1400 311, 1800 268, 2100 229; the numbers its build carried until 25 Nov, here a test's input, not a calibration)
    // and its torque ceiling off falls to the curve's last point: its torque at 2100 under 0.8 of its most below.
    // Measured 25 Nov: 2135 N m at 2100 against 2715 at 1300 (0.79); flat without (2729 at 2100)
    double most = 0.0, rated = 0.0;
    for (double rpm : {1100.0, 1300.0, 1600.0, 2100.0})
    {
        Engine e("truck-15l-six");
        e.spec("ecu.torqueMax", 0.0);
        e.spec("ecu.powerMax", 0.0);
        {
            const double cr[6] = {700.0, 800.0, 1000.0, 1400.0, 1800.0, 2100.0};
            const double cm[6] = {233.0, 274.0, 311.0, 311.0, 268.0, 229.0};
            const int rf = e.player->specIndex("ecu.fuelCurveRpm");
            const int mf = e.player->specIndex("ecu.fuelCurveMg");
            REQUIRE(rf >= 0);
            REQUIRE(mf >= 0);
            for (int i = 0; i < 6; ++i)
            {
                e.player->setSpec(static_cast<std::size_t>(rf), cr[i], static_cast<std::size_t>(i));
                e.player->setSpec(static_cast<std::size_t>(mf), cm[i], static_cast<std::size_t>(i));
            }
        }
        e.start();
        e.dyno(rpm, 1.0f);
        REQUIRE(e.run(2.0));
        e.spec("exhaust.heatSettle", 1);
        e.spec("boost.heatSettle", 1);
        REQUIRE(e.run(6.0));
        const double tq = e.mean("engine.torque", 1.0);
        std::printf("truck-15l-six on a stated fuel curve, floored at %.0f rpm: %.0f N m\n", rpm, tq);
        if (rpm < 2000.0)
            most = std::max(most, tq);
        else
            rated = tq;
    }
    CHECK(rated < 0.8 * most);
    // (2) A curve whose points carry no fuel is no curve: the same build with its speeds set and every mg 0 sounds to
    // the bit as with none (diesel-v8-truck states none)
    std::vector<float> sound[2];
    for (int k = 0; k < 2; ++k)
    {
        Engine e("diesel-v8-truck");
        if (k == 1)
        {
            const int rpmField = e.player->specIndex("ecu.fuelCurveRpm");
            REQUIRE(rpmField >= 0);
            for (int i = 0; i < 8; ++i)
                e.player->setSpec(static_cast<std::size_t>(rpmField), 800.0 + 200.0 * i, static_cast<std::size_t>(i));
        }
        e.start();
        e.dyno(1500.0, 1.0f);
        e.recording = true;
        REQUIRE(e.run(1.0));
        sound[k] = e.rec;
    }
    REQUIRE(sound[0].size() == sound[1].size());
    CHECK(sound[0] == sound[1]);
}

TEST_CASE("diesel governor: below the ECU's idle + 450 rpm at full pedal on the dyno, a governed diesel burns its fuel", "[diesel][governor]")
{
    // The reference ECU's trim (a petrol's quick spark hand against a sagging idle, below idleRpm + 450: sim/ecu/ecu.fm)
    // scaled a diesel's charged heat, at full pedal too on the dyno and the bench: the tractor archetype at 1000 rpm burned
    // 0.52 of the fuel its rack and compensator let through (3330 J a cylinder-cycle against some 6250; 638 N m, 14 kPa;
    // card D2's ledger, 25 Nov). Under D2's metered governor the charge takes none of it: the heat the cylinders' books
    // charge is the fuel the telemetry reports, within 5 % (the charges' own scatter and bias are the rest). Measured 25 Nov:
    // 1552 N m, 47 kPa, the port 1016 K, BSFC 209 g/kWh by the books, 213 by the telemetry
    Engine e("archetypes/tractor-15-inline-six");
    e.spec("env.temperature", 300.0);
    e.spec("env.pressure", 101325.0);
    e.start();
    auto play = [&](double s, const std::function<void()>& each) {
        const int calls = static_cast<int>(s * e.rate / 512.0 + 0.5);
        for (int c = 0; c < calls; ++c)
        {
            e.holdWarm();
            REQUIRE(e.run(512.0 / e.rate));
            if (each)
                each();
        }
    };
    e.dyno(1000.0, 1.0f);
    play(1.0, nullptr);
    e.spec("exhaust.heatSettle", 1);
    e.spec("boost.heatSettle", 1);
    play(5.0, nullptr);
    double was[6] = {-1, -1, -1, -1, -1, -1}, sum[6] = {0, 0, 0, 0, 0, 0}, ff = 0.0, tq = 0.0;
    int n[6] = {0, 0, 0, 0, 0, 0}, k = 0;
    play(2.0, [&] {
        for (int c = 0; c < 6; ++c)
        {
            const double v = e.get("cyl[" + std::to_string(c) + "].bookCharged");
            if (v != was[c])
            {
                was[c] = v;
                sum[c] += v;
                ++n[c];
            }
        }
        ff += e.tel("ecu.fuelFlow");
        tq += e.tel("engine.torque");
        ++k;
    });
    double books = 0.0;
    for (int c = 0; c < 6; ++c)
        books += sum[c] / std::max(n[c], 1);
    const double w = 1000.0 * 0.10471976;
    const double told = (ff / k) * 4.0 * 3.14159265 / w * 42.5e6 * 0.98;   // J a cycle, the telemetry's fuel's heat
    std::printf("tractor at 1000 rpm, full pedal on the dyno: %.0f N m; heat charged a cycle %.0f J by the books, %.0f J by the telemetry's fuel\n",
                tq / k, books, told);
    CHECK(books > 0.95 * told);
    CHECK(books < 1.05 * told);
}

TEST_CASE("diesel governor: an electronic governor's torque ceiling holds a plateau, and power above it", "[diesel][governor]")
{
    // ecu.torqueMax / powerMax (heavy.fm control16): the full-load fuel under a share an integral moves on the engine's
    // torque against the ceiling, 2 a second (judgement). The 2.0 common rail with VW's 320 N m: on the plateau it reads
    // the ceiling; a step of the pedal to full overshoots it by at most a few percent in the first second; held, it
    // ripples by little. Its stated value holds the row by construction: this tests the mechanism, not a validation
    // (and at 2500 rpm, where the plateau begins: the ceiling's integral and the vanes' boost loop meet there, and must
    // not hunt against each other)
    const double ceiling = 320.0;
    for (const double rpm : {2500.0, 3000.0})
    {
    double steady[2] = {0.0, 0.0};
    for (int k = 0; k < 2; ++k)
    {
        Engine e("archetypes/diesel-20-common-rail");
        e.spec("env.temperature", 300.0);
        e.spec("env.pressure", 101325.0);
        if (k == 1)
            e.spec("ecu.torqueMax", ceiling);
        e.start();
        // (held warm, as the dyno's rows are (host/include/tee/Warm.h), 512 samples a step)
        auto play = [&](double sec) {
            const int calls = static_cast<int>(sec * e.rate / 512.0 + 0.5);
            for (int c = 0; c < calls; ++c)
            {
                e.holdWarm();
                REQUIRE(e.run(512.0 / e.rate));
            }
        };
        // (part load first, 8 s, the boost up and the heat settled; then the pedal to full: the ceiling's integral starts
        // from none taken, as a driver's kick-down meets it)
        e.dyno(rpm, 0.6f);
        play(2.0);
        e.spec("exhaust.heatSettle", 1);
        e.spec("boost.heatSettle", 1);
        play(6.0);
        e.dyno(rpm, 1.0f);
        double most = 0.0, lo = 1.0e9, hi = 0.0;
        for (int q = 0; q < 120; ++q)
        {
            play(0.05);
            const double tq = e.tel("engine.torque");
            if (q < 20)
                most = std::max(most, tq);
            if (q >= 100)
            {
                lo = std::min(lo, tq);
                hi = std::max(hi, tq);
            }
        }
        steady[k] = e.mean("engine.torque", 1.0);
        std::printf("2.0 at %.0f rpm, ceiling %s: the first second's most %.0f N m, settled %.0f (%.0f to %.0f over its sixth second)\n",
                    rpm, k == 1 ? "320 N m" : "none", most, steady[k], lo, hi);
        if (k == 1)
        {
            CHECK(most < 1.05 * ceiling);
            CHECK(std::fabs(steady[k] - ceiling) < 0.02 * ceiling);
            CHECK(hi - lo < 0.03 * ceiling);
        }
    }
    CHECK(steady[0] > steady[1]);   // (without it the pump's line gives more: 329 N m at 3000, 25 Nov)
    }
}
