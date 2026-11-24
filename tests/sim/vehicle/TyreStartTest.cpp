// tests/sim/vehicle/TyreStartTest.cpp: the tyres start at the air's temperature (finding: they started near 205 K, as
// the vehicle's VehEnv was empty when init put them at rest, and every compound ran at its full cold loss), take it
// again whenever the car is put at rest, and start in their window when a host times a warm car (car.m1.warmTyres,
// the warm hold's). Through the whole engine (card H's Sim), car.model 1, at three temperatures of the air.
#ifndef TEE_SOURCE_SIM_DIR
#define TEE_SOURCE_SIM_DIR TEE_SOURCE_DIR "/sim/ref"
#endif
#ifndef TEE_SOURCE_CONTENT_DIR
#define TEE_SOURCE_CONTENT_DIR TEE_SOURCE_DIR "/content/ref"
#endif
#include "sim/ecu/Sim.h"

#include <catch2/catch_approx.hpp>

#include <cstdio>
#include <string>

namespace
{
    using tee::test::h::Sim;

    double temp(Sim& s, int i) { return s.read("car.m1.tyre[" + std::to_string(i) + "].temp"); }

    void write(Sim& s, const std::string& path, double v)
    {
        std::string error;
        const bool ok = s.player->instance().write(s.player->schema(), path, v, error);
        INFO(path << ": " << error);
        REQUIRE(ok);
    }
} // namespace

TEST_CASE("tyres.start: the tyres start at the air's temperature, again after a reset, in their window when warm",
          "[vehicle][tyre]")
{
    for (const char* build : {"combo/v8", "archetypes/v12-65", "archetypes/rotary-twin-na-side-port"})
    {
        INFO(build);
        for (double air : {253.15, 300.0, 318.0})
        {
            Sim s(build);
            s.set("car.model", 1);
            s.set("env.temperature", air);
            s.ctl().mode = tee::EnginePlayer::kModeDrive;
            // (the simulator computes nothing while it is silent: the first block it works is the start's)
            s.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
            s.run(512.0 / 48000.0);
            for (int i = 0; i < 4; ++i)
                CHECK(temp(s, i) == Catch::Approx(air).margin(1.0));
            // heated by a burnout on the line lock, then the car put at rest (out of drive and back)
            s.run(1.5);
            s.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
            s.ctl().buttons = 1;
            s.ctl().shiftUp += 1;
            s.run(0.3);
            s.ctl().throttle = 1.0f;
            s.run(3.0);
            const double hot = std::fmax(temp(s, 2), temp(s, 3));
            s.ctl().throttle = 0.0f;
            s.ctl().buttons = 0;
            s.ctl().mode = tee::EnginePlayer::kModeBench;
            s.run(0.1);
            s.ctl().mode = tee::EnginePlayer::kModeDrive;
            s.run(0.1);
            const double rest = temp(s, 2);
            // timed warm: each tyre in its compound's window
            write(s, "car.m1.warmTyres", 1.0);
            s.run(0.1);
            const double centre = s.read("car.m1.tyre[2].temp");
            std::printf("tyres (%s, air %.1f K): %.1f K at the start, %.1f after a burnout, %.1f after a reset, %.1f warm\n",
                        build, air, air, hot, rest, centre);
            CHECK(hot > air + 5.0);
            CHECK(rest == Catch::Approx(air).margin(1.0));
            for (int i = 0; i < 4; ++i)
                CHECK(s.read("car.m1.tyre[" + std::to_string(i) + "].grip") == Catch::Approx(1.0));
        }
    }
}
