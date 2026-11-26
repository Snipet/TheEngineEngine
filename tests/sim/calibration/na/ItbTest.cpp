// tests/sim/calibration/na/ItbTest.cpp: card N1, acting for card E. With a throttle a runner (intake.throttleKind 1)
// the manifold the ECU reads is the runners' far ends' mean pressure, a sum of linear waves: on the lift after the
// limiter it read below zero absolute (-4330 Pa, found by card D), and the ECU's load with it. Held: through a
// limiter-and-lift on the 3.2 six, the manifold reading never falls below 5 kPa absolute.
#include "Player.h"
#include "tee/EnginePlayer.h"
#include "tee/Warm.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>

// (a section of the fast tier's one case, FastTest.cpp: "individual throttles: the manifold reading stays above 5 kPa
// through a limiter and a lift")
void n1::itbManifoldAbove5kPa()
{
    struct
    {
        double sampleRate = 48000.0;
    } options;
    std::string errors, warnings;
    tee::EnginePlayer* p = &n1::player(options.sampleRate);
    tee::EngineBuild b;
    REQUIRE(tee::loadBuild(*p, std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "archetypes" / "straight-six-32-high-rev.json",
                           b, errors, warnings));
    tee::applyBuild(*p, b);
    auto& c = p->controls();
    const int block = 64;
    float l[block], r[block];
    const int man = p->telemetryIndex("intake.manifold");
    const int load = p->telemetryIndex("ecu.load");
    const int speed = p->telemetryIndex("engine.speed");
    double lowest = 1e9, lowestLoad = 1e9, top = 0.0;
    for (int i = 0; i < int(9.0 * options.sampleRate / block); ++i)
    {
        for (const tee::HeldValue& h : tee::warmEngine())
            p->instance().write(p->schema(), h.path, h.value, errors);
        const double t = i * block / options.sampleRate;
        c.ignition = t < 1.0 ? 2 : 1;
        c.throttle = t > 2.0 && t < 5.0 ? 1.0f : 0.0f;
        p->process(l, r, block);
        top = std::max(top, p->telemetry(std::size_t(speed)) * 30.0 / 3.14159265);
        if (t > 2.0)
        {
            lowest = std::min(lowest, p->telemetry(std::size_t(man)));
            lowestLoad = std::min(lowestLoad, p->telemetry(std::size_t(load)));
        }
    }
    std::printf("3.2 six, limiter and lift: top %.0f rpm, manifold at its lowest %.0f Pa absolute, ECU load %.3f\n", top, lowest, lowestLoad);
    CHECK(top > 7500.0);
    CHECK(lowest > 5000.0);
    CHECK(lowestLoad >= 0.0);
}
