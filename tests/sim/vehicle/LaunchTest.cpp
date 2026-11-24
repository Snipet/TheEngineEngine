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

TEST_CASE("launch.pedal: with the driver's own clutch pedal the launch is the driver's", "[vehicle][launch]")
{
    const Start a = standing(2, true, true);
    const Start b = standing(2, false, true);
    // (traction control's engine side acts in both, the clutch's hold in neither: the first is no slower for it)
    CHECK(a.state == 2.0);
    CHECK(b.slipMean > 0.5);
}
