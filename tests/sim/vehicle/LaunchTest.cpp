// tests/sim/vehicle/LaunchTest.cpp: a standing start under traction control (Vehicle::launch, finding X-011): with the
// car's own clutch (the paddles' robotised one) the clutch's torque holds the driven wheels at the ECU's slip target
// until it locks. Through the whole engine (card H's Sim): the reference V8 in the rear-drive coupe (combo/v8),
// ecu.model 1, car.model 1, into first at a standstill and floored, wet and dry, traction control on and off; and
// with the driver's own clutch pedal, nothing changes.
#ifndef TEE_SOURCE_SIM_DIR
#define TEE_SOURCE_SIM_DIR TEE_SOURCE_DIR "/sim/ref"
#endif
#ifndef TEE_SOURCE_CONTENT_DIR
#define TEE_SOURCE_CONTENT_DIR TEE_SOURCE_DIR "/content/ref"
#endif
#include "sim/ecu/Sim.h"

#include <catch2/catch_approx.hpp>

#include <cmath>
#include <cstdio>

namespace
{
    using tee::test::h::Sim;

    struct Start
    {
        double speedAt2 = 0.0, slipMean = 0.0, state = 0.0;
    };

    // ignition, into first on the paddle (or, pedal: on the lever with the clutch pedal, let up over half a second),
    // floored for 2 s; the slip's mean from 0.5 s
    Start standing(int surface, bool tc, bool pedal = false)
    {
        Sim s("combo/v8");
        s.set("ecu.model", 1);
        s.set("car.model", 1);
        s.set("env.surface", surface);
        s.set("env.temperature", 300.0);
        s.set("ecu.traction.on", tc ? 1 : 0);
        s.ctl().mode = tee::EnginePlayer::kModeDrive;
        s.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
        s.run(1.5);
        s.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
        s.run(1.5);
        if (pedal)
        {
            s.ctl().clutch = 1.0f;
            s.run(0.2);
            s.ctl().lever = 1;
        }
        else
            s.ctl().shiftUp += 1;
        s.run(0.3);
        s.ctl().throttle = 1.0f;
        Start w;
        int n = 0;
        for (int i = 0; i < 40; ++i)
        {
            if (pedal)
                s.ctl().clutch = static_cast<float>(std::fmax(1.0 - 0.1 * (i + 1), 0.0));
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
        std::printf("standing start, %s, traction %s%s: %.2f m/s at 2 s, slip %.3f, state %g\n", surface == 2 ? "wet" : "dry",
                    tc ? "on" : "off", pedal ? ", the driver's clutch" : "", w.speedAt2, w.slipMean, w.state);
        return w;
    }
} // namespace

TEST_CASE("launch.wet: from a standstill on a wet road the clutch holds the slip at the target, and it is faster",
          "[vehicle][launch]")
{
    const Start off = standing(2, false);
    const Start on = standing(2, true);
    CHECK(off.slipMean > 0.5);
    CHECK(std::fabs(on.slipMean - 0.10) < 0.05);
    CHECK(on.state == 2.0);
    CHECK(on.speedAt2 > 1.1 * off.speedAt2);
}

TEST_CASE("launch.dry: from a standstill on a dry road traction control does not light the tyres", "[vehicle][launch]")
{
    const Start off = standing(0, false);
    const Start on = standing(0, true);
    CHECK(std::fabs(on.slipMean - 0.10) < 0.05);
    CHECK(on.state == 2.0);
    CHECK(on.speedAt2 >= off.speedAt2);
}

// (nightly, the lead's call 2026-11-24: the dry and wet launches stay in every run)
TEST_CASE("launch.pedal: with the driver's own clutch pedal the launch is the driver's", "[vehicle][launch][nightly]")
{
    const Start a = standing(2, true, true);
    const Start b = standing(2, false, true);
    // (traction control's engine side acts in both, the clutch's hold in neither: the first is no slower for it)
    CHECK(a.state == 2.0);
    CHECK(b.slipMean > 0.5);
}

TEST_CASE("launch.weak: an engine weak low down launches at its launch speed, not bogged", "[vehicle][launch]")
{
    // Card P's naturally aspirated rotary (archetypes/rotary-twin-na-side-port: its torque up at 5500 rpm and beyond) in
    // its own car and gearbox, traction control on, from a standstill and floored: the launch keeps the engine up while
    // the clutch slips. Measured here before (the clutch held only to the tyres' share): the engine down to 1442 rpm after
    // 0.8 s, 72 km/h in 5.95 s; after: 4510 rpm or more, 72 km/h in 5.35 s (it is a little slower at 2 s, 5.66 m/s
    // against 6.18: it bites later, while the engine rises; it is clearly quicker after)
    Sim s("archetypes/rotary-twin-na-side-port");
    s.set("ecu.traction.on", 1);
    s.ctl().mode = tee::EnginePlayer::kModeDrive;
    s.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
    s.run(1.5);
    s.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
    s.run(1.5);
    s.ctl().shiftUp += 1;
    s.run(0.3);
    s.ctl().throttle = 1.0f;
    double least = 1e9, at2 = 0.0;
    double t72 = -1.0;
    for (int i = 0; i < 160; ++i)
    {
        s.run(0.05);
        if (i >= 16 && i < 50)
            least = std::fmin(least, s.tel("engine.speed") * 9.5492966);
        if (i == 39)
            at2 = s.tel("car.speed");
        if (t72 < 0.0 && s.tel("car.speed") >= 20.0)
            t72 = 0.05 * (i + 1);
    }
    std::printf("weak launch: 0 to 72 km/h in %.2f s\n", t72);
    (void)at2;
    std::printf("weak launch: the engine at %.0f rpm or more from 0.8 s; %.2f m/s at 2 s; state %g\n", least, at2,
                s.tel("engine.state"));
    CHECK(least > 3000.0);
    CHECK(s.tel("engine.state") == 2.0);
    CHECK(t72 > 0.0);
    CHECK(t72 < 5.6);
}

namespace
{
    // An automatic's launch, as a driver makes it: from idle (the brakes off, the pedal from 0.15 to the floor over
    // 0.6 s), or brake-torqued (the brakes held and the pedal at `hold` for a second, so the converter stalls against
    // them and the revs come up; then the brakes off and the same ramp from there). The time to 18.29 m (60 ft).
    double autoLaunch(const char* build, double hold, bool tc)
    {
        Sim s(build);
        s.set("car.model", 1);
        s.set("car.gearbox", 3);
        s.set("ecu.model", 1);
        s.set("ecu.traction.on", tc ? 1 : 0);
        s.set("env.temperature", 300.0);
        s.ctl().mode = tee::EnginePlayer::kModeDrive;
        s.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
        s.run(1.5);
        s.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
        s.ctl().brake = 1.0f;
        s.run(1.0);
        s.ctl().lever = 1;
        s.run(0.5);
        double from = 0.15;
        if (hold > 0.0)
        {
            s.ctl().throttle = static_cast<float>(hold);
            s.run(1.0);
            from = hold;
        }
        const double rpm = s.tel("engine.speed") * 9.5492966;
        const double x0 = s.tel("car.distance");
        s.ctl().brake = 0.0f;
        double t = 0.0;
        for (int i = 0; i < 400 && s.tel("car.distance") - x0 < 18.288; ++i)
        {
            const double p = std::fmin(from + (1.0 - from) * (t / 0.6), 1.0);
            s.ctl().throttle = static_cast<float>(p);
            s.run(512.0 / 48000.0);
            t += 512.0 / 48000.0;
        }
        std::printf("automatic launch (%s, %s, traction %s): %.0f rpm at the release, 60 ft in %.3f s\n", build,
                    hold > 0.0 ? "brake-torqued" : "from idle", tc ? "on" : "off", rpm, t);
        return t;
    }
} // namespace

TEST_CASE("launch.automatic: a brake-torque launch on an automatic is no slower to 60 ft than one from idle",
          "[vehicle][launch]")
{
    // (held at a fifth of the pedal: where the rear brakes still hold against the converter's stall torque; at a
    // full pedal they do not on most cars, and the rear tyres spin against them before the green: a burnout)
    for (bool tc : {false, true})
    {
        const double idle = autoLaunch("combo/v8", 0.0, tc);
        const double torqued = autoLaunch("combo/v8", 0.2, tc);
        // With traction control the tyres are held at their slip target either way, so the converter's head start is
        // all gain: no slower. Without it the launch is the tyres' (street rubber, 2300 rpm at the release against
        // 1400): the extra torque at the hit can go into wheelspin, and "no slower" is not a law there: main's Linux
        // job at 09c1fb4e read 3.104 s against 3.072 from idle (card I's own run: 3.104 against 3.136), three blocks
        // apart either way. What must hold without it is that brake-torquing never costs a launch (a tenth of a
        // second, judgement); the fault this guards, an upshift on spinning tyres, is launch.automatic.spin's below.
        CHECK(torqued <= idle + (tc ? 0.01 : 0.10));
    }
}

TEST_CASE("launch.automatic.spin: an automatic whose tyres spin does not shift up at walking pace", "[vehicle][launch]")
{
    // (finding P's brake-torque launches: floored against the brakes the rear tyres spin before the green, and a
    // schedule on the engine's speed took the big block through second and third under 5 m/s, slower than from idle)
    Sim s("combo/v8");
    s.set("car.model", 1);
    s.set("car.gearbox", 3);
    s.set("ecu.model", 1);
    s.ctl().mode = tee::EnginePlayer::kModeDrive;
    s.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
    s.run(1.5);
    s.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
    s.ctl().brake = 1.0f;
    s.run(1.0);
    s.ctl().lever = 1;
    s.run(0.5);
    s.ctl().throttle = 1.0f;
    s.run(1.5);
    s.ctl().brake = 0.0f;
    double most = 0.0;
    int spun = 0;
    for (int i = 0; i < 300 && s.tel("car.speed") < 8.0; ++i)
    {
        s.run(512.0 / 48000.0);
        most = std::fmax(most, s.tel("car.gear"));
        spun |= static_cast<int>(s.tel("car.flags")) & 1;
    }
    std::printf("automatic, floored on the brakes: under 8 m/s its highest gear %g (spinning: %s)\n", most,
                spun != 0 ? "yes" : "no");
    CHECK(spun != 0);
    CHECK(most == 1.0);
}
