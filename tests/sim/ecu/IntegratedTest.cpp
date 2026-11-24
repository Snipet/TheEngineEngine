// tests/sim/ecu/IntegratedTest.cpp: card H's integrated tests on the whole simulator (Sim.h): 9 (bangs by account),
// 11 (random pedals through every limiter kind), the bake, and model 0's telemetry left alone. Test 5, the null test
// with both models at 0, is tests/sim/NullTest.cpp's; test 10 (traction on card I's car) waits for card I's slip.
#include "Sim.h"

#include <catch2/catch_test_macros.hpp>

#include <catch2/catch_approx.hpp>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>

using tee::test::h::Sim;

namespace
{
    // The V8 in neutral in the car, warm air, both models 1, started
    Sim car()
    {
        Sim s("v8");
        s.set("ecu.model", 1);
        s.set("fuel.model", 1);
        s.set("engine.wallHeat", 1);                     // (fuel.model 1 implies Woschni's walls)
        s.set("env.temperature", 300.0);
        s.ctl().mode = tee::EnginePlayer::kModeDrive;
        s.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
        s.run(1.5);
        s.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
        s.run(1.0);
        return s;
    }

    struct Bangs
    {
        double count = 0.0, burned = 0.0;
    };

    // Flat out on the limiter for `limit` s (counted when `onLimiter`), then lifted: the bangs and the fuel they burned
    // while the overrun's cut is in (ecu.flags 8), from 0.3 s after the lift for `lift` s at most. (Not from the
    // return to idle, whose own firings and blowdown are another matter, nor the first 0.3 s, which light what the
    // rich full-load running left in the pipe, as a real lift does)
    Bangs bangs(Sim& s, double limit, double lift, bool onLimiter)
    {
        s.ctl().throttle = 1.0f;
        s.run(0.5);
        double n0 = s.read("ecu.one.account.bangs"), f0 = s.read("ecu.one.account.total");
        s.run(limit);
        if (onLimiter)
            return {s.read("ecu.one.account.bangs") - n0, s.read("ecu.one.account.total") - f0};
        s.ctl().throttle = 0.0f;
        s.run(0.3);
        Bangs b;
        for (double t = 0.0; t < lift; t += 0.01)
        {
            n0 = s.read("ecu.one.account.bangs");
            f0 = s.read("ecu.one.account.total");
            s.run(0.01);
            if ((static_cast<int>(s.tel("ecu.flags")) & 8) == 0)
                break;
            b.count += s.read("ecu.one.account.bangs") - n0;
            b.burned += s.read("ecu.one.account.total") - f0;
        }
        return b;
    }
} // namespace

TEST_CASE("H9: a spark-cut limiter and a fuelled overrun bang as the fuel sent; a full cut makes none", "[ecu][integrated][nightly]")
{
    {
        Sim s = car();                                   // the fuel-cut limiter, a clean overrun: nothing to burn
        const Bangs b = bangs(s, 3.0, 2.0, false);
        WARN("a clean overrun (a full fuel cut): " << b.count << " bangs");
        CHECK(b.count == 0.0);
    }
    {
        Sim s = car();
        s.set("ecu.limiter.kind", 1);                    // spark cut
        const Bangs b = bangs(s, 3.0, 0.0, true);
        WARN("spark cut on the limiter, 3 s: " << b.count << " bangs, " << b.burned * 1e6 << " mg burned in the pipe");
        CHECK(b.count > 5.0);
    }
    double little = 0.0, much = 0.0;
    for (double share : {0.1, 0.25})       // (more would leave too little of the cut firings' air to burn it)
    {
        Sim s = car();
        s.set("ecu.overrun.fuel", share);
        s.set("ecu.overrun.retard", 30.0);
        const Bangs b = bangs(s, 1.5, 2.5, false);
        WARN("overrun fuel " << share << ": " << b.count << " bangs, " << b.burned * 1e6 << " mg burned in the pipe");
        (share < 0.2 ? little : much) = b.burned;
    }
    // a fuelled overrun bangs; how much with how much fuel is the standalone account's test (H9 standalone part: four
    // times the fuel, four times burned): here a fraction of a second of overrun cut makes a handful of bangs, whose
    // count turns on where each firing falls, and a larger share leaves less of the cut firings' air to burn it
    CHECK(little + much > 0.0);
}

namespace
{
    // Random pedals (and the launch button) for `seconds` on each of `kinds` of the limiter: finite, within +-1, never past
    // the redline by 500 rpm, still running
    void soak(std::initializer_list<int> kinds, double seconds)
    {
        for (int kind : kinds)
        {
            Sim s = car();
            s.set("ecu.limiter.kind", kind);
            s.set("ecu.overrun.fuel", 0.3);
            s.set("ecu.launch.on", 1);
            std::mt19937 rng(1234u + static_cast<unsigned>(kind));
            std::uniform_real_distribution<double> u(0.0, 1.0);
            const double red = s.tel("engine.redline") * 9.5492966;
            double most = 0.0;
            for (double t = 0.0; t < seconds; t += 0.25)
            {
                if (u(rng) < 0.3)
                    s.ctl().throttle = static_cast<float>(u(rng) < 0.5 ? 1.0 : u(rng));
                if (u(rng) < 0.05)
                    s.ctl().buttons = u(rng) < 0.5 ? 2 : 0;          // the launch button
                s.run(0.25);
                most = std::fmax(most, s.tel("engine.speed") * 9.5492966);
            }
            std::printf("H11 kind %d: %.0f s, the most %.0f rpm (redline %.0f), peak %.3f, state %g\n", kind, seconds, most, red,
                        s.peak, s.tel("engine.state"));
            CHECK(s.finite);
            CHECK(s.peak <= 1.0);
            CHECK(most < red + 500.0);
            CHECK(s.tel("engine.state") == 2.0);
        }
    }
} // namespace

TEST_CASE("H11: random pedals on the fuel-cut and spark-cut limiters, briefly", "[ecu][integrated]")
{
    // the quick form of the soak below (CI's default run: under 30 s)
    soak({0, 1}, 8.0);
}

TEST_CASE("H11: random pedals through every limiter kind: finite, bounded, never runs away", "[ecu][integrated][nightly]")
{
    // 40 s a kind; the card's ten minutes is the same with H_SOAK_SECONDS=600 (run by hand)
    const char* env = std::getenv("H_SOAK_SECONDS");
    soak({0, 1, 2, 3}, env != nullptr ? std::atof(env) : 40.0);
}

TEST_CASE("H: a bake writes every map's formula at its breakpoints into the Scope", "[ecu][integrated][bake]")
{
    Sim s = car();
    s.ctl().bakeMaps += 1;
    s.run(0.1);
    CHECK(s.read("scope.bakeCount") == 1.0);
    CHECK(s.read("scope.bakedNx[0]") == 16.0);
    CHECK(s.read("scope.bakedNy[0]") == 16.0);
    CHECK(s.read("scope.bakedNy[2]") == 6.0);                    // the boost target's gears
    // the spark at 500 rpm (52.4 rad/s) and 0.1 load: the half of the burn 8 after the top, 0.517 (44 + 11 x 0.7)
    // burnK (cr / 10)^-0.3 - 8
    const double burnK = s.read("burnK");
    const double cr = s.read("smCR");
    CHECK(s.read("scope.bakedX[0][0]") == Catch::Approx(500.0 * 0.10471976).epsilon(1e-5));
    CHECK(s.read("scope.baked[0][0]") == Catch::Approx(0.517 * (44.0 + 11.0 * 0.7) * burnK * std::pow(cr * 0.1, -0.3) - 8.0).epsilon(1e-3));
    // a second press, a second bake
    s.ctl().bakeMaps += 1;
    s.run(0.1);
    CHECK(s.read("scope.bakeCount") == 2.0);
}

TEST_CASE("H: under ecu.model 0 and fuel.model 0 model 1's telemetry stays at 0", "[ecu][integrated]")
{
    Sim s("v8");
    s.set("env.temperature", 300.0);
    s.start();
    s.ctl().throttle = 0.5f;
    s.run(1.0);
    CHECK(s.tel("ecu.advance") > 5.0);                          // (the engine writes cylinder 1's under both models)
    CHECK(s.tel("ecu.lambda") == 0.0);
    CHECK(s.tel("ecu.flags") == 0.0);
    CHECK(s.tel("engine.speed") > 100.0);
}

TEST_CASE("H10: traction control holds a spinning car's slip near its target", "[ecu][integrated][traction]")
{
    // The V8 in the rear-drive coupe (card I's vehicle, car.model 1) on street tyres, ecu.model 1: away gently in first,
    // then the pedal floored while rolling. The slip is the driven wheels' (Car::drivenSlip, as the ECU is given it);
    // measured over the second from 1.5 s after the pedal goes down (before that, under some 9 m/s, the slip ratio of
    // wheels geared to an engine at 2000 rpm stays high whatever the ECU takes out; traction control brings it down meanwhile). (Dumping the clutch at a standstill is no test of
    // it: the wheels are then geared to an engine that cannot turn slower than its idle while the car stands, so the
    // slip ratio stays near 1 whatever the ECU does; a launch slips its clutch)
    auto launch = [](bool tc, double& slipMean, double& slipMost, int& flags, double& state) {
        Sim s("v8");
        s.over("cars/rwd-v8-coupe");
        s.set("car.tyre", 1);                            // street
        s.set("ecu.model", 1);
        s.set("fuel.model", 1);
        s.set("engine.wallHeat", 1);                     // (fuel.model 1 implies Woschni's walls)
        s.set("env.temperature", 300.0);
        s.set("ecu.traction.on", tc ? 1 : 0);
        s.ctl().mode = tee::EnginePlayer::kModeDrive;
        s.ctl().lever = 0;
        s.ctl().clutch = 1.0f;
        s.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
        s.run(1.5);
        s.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
        s.run(1.0);
        s.ctl().lever = 1;
        s.run(0.5);
        // away gently in first (the clutch in over a second at a fifth of the pedal), rolling at some 3 m/s; then floored
        s.ctl().throttle = 0.2f;
        for (int i = 0; i < 20; ++i)
        {
            s.ctl().clutch = 1.0f - 0.05f * static_cast<float>(i + 1);
            s.run(0.05);
        }
        s.run(0.5);
        s.ctl().throttle = 1.0f;
        s.run(1.5);
        slipMean = 0.0;
        slipMost = 0.0;
        flags = 0;
        const int n = 90;
        for (int i = 0; i < n; ++i)
        {
            s.run(512.0 / 48000.0);
            const double slip = s.read("ecu.vSlip");
            slipMean += slip / n;
            slipMost = std::fmax(slipMost, slip);
            flags |= static_cast<int>(s.tel("ecu.flags"));
        }
        state = s.tel("engine.state");
        std::printf("H10 traction %s: slip mean %.3f, most %.3f, flags %d, state %g, %.1f m/s\n", tc ? "on" : "off",
                    slipMean, slipMost, flags, state, s.tel("car.speed"));
    };
    double offMean = 0.0, offMost = 0.0, onMean = 0.0, onMost = 0.0, offState = 0.0, onState = 0.0;
    int offFlags = 0, onFlags = 0;
    launch(false, offMean, offMost, offFlags, offState);
    launch(true, onMean, onMost, onFlags, onState);
    CHECK(offMean > 0.2);                                // it spins without it
    CHECK((onFlags & 256) != 0);
    CHECK(onMean > 0.07);
    CHECK(onMean < 0.13);                                // near the target, 0.10
    CHECK(onState == 2.0);                               // and no stall
}

TEST_CASE("H: both models on, the V8 starts at -20 C and at the coldest air", "[ecu][integrated][cold]")
{
    // X-003: with fuel.model 1 and ecu.model 1 the cold start's enrichment made the burn's own mixture lambda 0.62
    // (0.88 at the cranking's full manifold times 0.7), at gasoline's rich limit, where no heat comes: it cranked for
    // ever. The enrichment is now fuel for the cold walls (Ecu1::fuelFactor), the burn seeing the lambda asked
    for (double air : {253.15, 233.15})
    {
        Sim s("v8");
        s.set("ecu.model", 1);
        s.set("fuel.model", 1);
        s.set("engine.wallHeat", 1);                     // (fuel.model 1 implies Woschni's walls)
        s.set("env.temperature", air);
        s.ctl().mode = tee::EnginePlayer::kModeDrive;
        s.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
        double ranAt = -1.0;
        for (int i = 0; i < 400 && ranAt < 0.0; ++i)
        {
            s.run(0.01);
            if (s.tel("engine.state") == 2.0)
                ranAt = 0.01 * (i + 1);
        }
        s.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
        const double rpm = s.mean("engine.speed", 3.0) * 9.5492966;
        std::printf("cold start at %.2f K: running at %.2f s, fast idle %.0f rpm\n", air, ranAt, rpm);
        CHECK(ranAt > 0.0);
        CHECK(ranAt < 2.0);
        CHECK(s.tel("engine.state") == 2.0);
        CHECK(rpm > 1000.0);
    }
}

namespace
{
    struct Wet
    {
        double speedAt2 = 0.0, slipMean = 0.0, state = 0.0;
    };

    // The reference V8 in the coupe (combo/v8: card I's vehicle, its manual on the paddles), a wet road, ecu.model 1:
    // into first at a standstill; from rolling (two seconds at 0.15 of the pedal) or not, floored for two seconds. The
    // slip is the driven wheels' (Car::drivenSlip); its mean from half a second after the pedal goes down
    Wet wetLaunch(bool tc, bool rolling)
    {
        Sim s("combo/v8");
        s.set("ecu.model", 1);
        s.set("car.model", 1);
        s.set("env.surface", 2);
        s.set("env.temperature", 300.0);
        s.set("ecu.traction.on", tc ? 1 : 0);
        s.ctl().mode = tee::EnginePlayer::kModeDrive;
        s.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
        s.run(1.5);
        s.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
        s.run(1.5);
        s.ctl().shiftUp += 1;
        s.run(0.3);
        if (rolling)
        {
            s.ctl().throttle = 0.15f;
            s.run(2.0);
        }
        s.ctl().throttle = 1.0f;
        Wet w;
        int n = 0;
        for (int i = 0; i < 40; ++i)
        {
            s.run(0.05);
            if (i >= 10)
            {
                w.slipMean += s.read("ecu.vSlip");
                n += 1;
            }
        }
        w.slipMean /= n;
        w.speedAt2 = s.tel("car.speed");
        w.state = s.tel("engine.state");
        std::printf("H10 wet, %s, traction %s: %.2f m/s at 2 s, slip %.3f, state %g\n", rolling ? "rolling" : "standstill",
                    tc ? "on" : "off", w.speedAt2, w.slipMean, w.state);
        return w;
    }
} // namespace

TEST_CASE("H10: on a wet road from rolling, traction control holds the slip and the car is faster", "[ecu][integrated][traction]")
{
    // X-011: its authority faded from 1500 rpm over the idle, so on a wet road it held the engine at 2650 rpm in first
    // and the wheels spun as without it. It now takes the torque down to none (the plate, the spark, then every firing)
    // and lets go only near where the automatic clutch opens
    const Wet off = wetLaunch(false, true);
    const Wet on = wetLaunch(true, true);
    CHECK(off.slipMean > 0.5);
    CHECK(std::fabs(on.slipMean - 0.10) < 0.05);
    CHECK(on.speedAt2 > off.speedAt2);
    CHECK(on.state == 2.0);
}

TEST_CASE("H10: on a wet road from a standstill: no stall, no slower", "[ecu][integrated][traction]")
{
    const Wet off = wetLaunch(false, false);
    const Wet on = wetLaunch(true, false);
    CHECK(on.state == 2.0);
    CHECK(on.speedAt2 >= off.speedAt2 * 0.98);
}

TEST_CASE("H10: on a wet road from a standstill the slip is held", "[ecu][integrated][traction]")
{
    // Expected to fail: at walking pace the wheels' speed for 10 % slip is the engine's under the speed where the clutch
    // closes, so only a slipping clutch holds it (a launch: card I's automatic clutch, or launch control's hold);
    // traction control alone keeps the engine above where the clutch lets go (measured: slip 0.88)
    const Wet on = wetLaunch(true, false);
    CHECK(std::fabs(on.slipMean - 0.10) < 0.05);
}

TEST_CASE("H9 (quick): a spark-cut limiter bangs; a clean overrun does not", "[ecu][integrated]")
{
    Sim s = car();
    s.set("ecu.limiter.kind", 1);
    CHECK(bangs(s, 1.5, 0.0, true).count > 5.0);
    Sim f = car();                                   // the fuel-cut limiter: a spark cut's store would linger
    CHECK(bangs(f, 1.0, 1.0, false).count == 0.0);
}
