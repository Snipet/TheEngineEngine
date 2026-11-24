// tests/sim/calibration/na/X022Test.cpp: card N1, acting for E, holds X-022. Throttles a runner (intake.throttleKind 1)
// behind a boost machine whose charge pipe is at its smallest (0.0005 m^3) and its intercooler at its least flow: since
// 5fdc5ad the charge pipe was drawn by the valves' mass, which does not answer its pressure, and on the first throttle
// opening the pipe's pressure rang up (99 kPa, 156, 0, 973, 1738 ... 30 MPa in 60 ms) and the engine diverged (FLAG_FAULT).
// Now the trumpets' own flow draws it and only the valves' excess over a cycle is made up, slowly. Held: ten seconds
// of idle and of throttle blips without the guard's fault, everything finite.
#include "tee/EnginePlayer.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <filesystem>
#include <string>

TEST_CASE("X-022: throttles a runner behind a tiny charge pipe idle and blip ten seconds without a fault", "[nightly][calibration][na]")
{
    for (const bool blips : {false, true})
    {
        tee::EnginePlayer::Options options;
        options.directory = std::filesystem::path(TEE_SOURCE_DIR) / "sim";
        options.sampleRate = 48000.0;
        std::string errors, warnings;
        auto p = tee::EnginePlayer::create(options, errors);
        INFO(errors);
        REQUIRE(p != nullptr);
        tee::EngineBuild b;
        REQUIRE(tee::loadBuild(*p,
                               std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "combo" / "findings" /
                                   "x022-supercharger-itb-small-charge.json",
                               b, errors, warnings));
        tee::applyBuild(*p, b);
        auto& c = p->controls();
        const int block = 256;
        float l[block], r[block];
        const int flags = p->telemetryIndex("engine.flags");
        const int speed = p->telemetryIndex("engine.speed");
        int faults = 0, bad = 0;
        double top = 0.0;
        for (int i = 0; i < int(11.0 * options.sampleRate / block); ++i)
        {
            const double t = i * block / options.sampleRate;
            c.ignition = t < 1.0 ? 2 : 1;
            c.throttle = blips && t > 2.0 && std::fmod(t, 1.0) < 0.5 ? 1.0f : 0.0f;
            p->process(l, r, block);
            if ((int(p->telemetry(std::size_t(flags))) & 4) != 0)
            {
                if (faults == 0)
                    std::printf("first fault at %.3f s\n", t);
                ++faults;
            }
            for (int k = 0; k < block; ++k)
                if (!std::isfinite(l[k]) || !std::isfinite(r[k]))
                    ++bad;
            top = std::fmax(top, p->telemetry(std::size_t(speed)) * 30.0 / M_PI);
        }
        std::printf("X-022 build, %s: faults %d, non-finite samples %d, top %.0f rpm\n", blips ? "blips" : "idle", faults, bad, top);
        CHECK(faults == 0);
        CHECK(bad == 0);
    }
}
