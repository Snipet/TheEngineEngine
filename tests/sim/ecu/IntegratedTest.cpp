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
    // over the lift's `lift` s from half a second in (the pipe's store of the rich full-load running gone by then)
    Bangs bangs(Sim& s, double limit, double lift, bool onLimiter)
    {
        s.ctl().throttle = 1.0f;
        s.run(0.5);
        double n0 = s.read("ecu.one.account.bangs"), f0 = s.read("ecu.one.account.total");
        s.run(limit);
        if (onLimiter)
            return {s.read("ecu.one.account.bangs") - n0, s.read("ecu.one.account.total") - f0};
        s.ctl().throttle = 0.0f;
        s.run(0.5);
        n0 = s.read("ecu.one.account.bangs");
        f0 = s.read("ecu.one.account.total");
        s.run(lift);
        return {s.read("ecu.one.account.bangs") - n0, s.read("ecu.one.account.total") - f0};
    }
} // namespace

TEST_CASE("H9: a spark-cut limiter and a fuelled overrun bang as the fuel sent; a full cut makes none", "[ecu][integrated]")
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
    for (double share : {0.1, 0.4})
    {
        Sim s = car();
        s.set("ecu.overrun.fuel", share);
        s.set("ecu.overrun.retard", 30.0);
        const Bangs b = bangs(s, 1.5, 2.5, false);
        WARN("overrun fuel " << share << ": " << b.count << " bangs, " << b.burned * 1e6 << " mg burned in the pipe");
        (share < 0.2 ? little : much) = b.burned;
    }
    CHECK(little > 0.0);
    CHECK(much > 2.0 * little);
}

TEST_CASE("H11: random pedals through every limiter kind: finite, bounded, never runs away", "[ecu][integrated][soak]")
{
    // ctest runs 40 s a kind; the card's ten minutes is the same test with H_SOAK_SECONDS=600 (run by hand)
    const char* env = std::getenv("H_SOAK_SECONDS");
    const double seconds = env != nullptr ? std::atof(env) : 40.0;
    for (int kind = 0; kind < 4; ++kind)
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

TEST_CASE("H: a bake writes every map's formula at its breakpoints into the Scope", "[ecu][integrated][bake]")
{
    Sim s = car();
    s.ctl().bakeMaps += 1;
    s.run(0.1);
    CHECK(s.read("scope.bakeCount") == 1.0);
    CHECK(s.read("scope.bakedNx[0]") == 16.0);
    CHECK(s.read("scope.bakedNy[0]") == 16.0);
    CHECK(s.read("scope.bakedNy[2]") == 6.0);                    // the boost target's gears
    // the spark at 500 rpm (52.4 rad/s) and 0.1 load: the reference's 12 + 22 sqrt(500 / red) - 8 (0.1 - 0.4) + 22 (burnK - 1)
    const double red = s.tel("engine.redline") * 9.5492966;
    const double burnK = s.read("burnK");
    CHECK(s.read("scope.bakedX[0][0]") == Catch::Approx(500.0 * 0.10471976).epsilon(1e-5));
    CHECK(s.read("scope.baked[0][0]") == Catch::Approx(12.0 + 22.0 * std::sqrt(500.0 / red) + 2.4 + 22.0 * (burnK - 1.0)).epsilon(1e-4));
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
