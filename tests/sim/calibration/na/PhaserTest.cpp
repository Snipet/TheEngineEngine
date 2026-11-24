// tests/sim/calibration/na/PhaserTest.cpp: card N1's hold on the intake phaser's own schedule (sim/valvetrain/phaser.fm,
// scheduleIntake: a target intake closing by speed, capped by overlap). Before it, the schedule put the whole range of
// advance in at full load below half the redline and cut holes into the long-cammed engines' curves (the 3.2 six 206
// N m at 2700 rpm against 290 with the rule; the V12 495 at 5000 against 582: docs/measurements/calibration/na/causes.md,
// C). Held: at those speeds, on the engine dyno at full throttle and warm, the schedule gives at least 0.97 of the
// torque with the phaser parked.
#include "tee/EnginePlayer.h"
#include "tee/Warm.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

namespace
{
    // The mean brake torque (N m) on the engine dyno at `rpm`, full throttle, warm: 1.5 s to settle, 1.5 s measured
    double dynoTorque(const std::string& slug, double rpm, bool parked)
    {
        tee::EnginePlayer::Options options;
        options.directory = std::filesystem::path(TEE_SOURCE_DIR) / "sim";
        options.sampleRate = 48000.0;
        std::string errors, warnings;
        auto p = tee::EnginePlayer::create(options, errors);
        INFO(errors);
        REQUIRE(p != nullptr);
        tee::EngineBuild b;
        REQUIRE(tee::loadBuild(*p, std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "archetypes" / (slug + ".json"), b,
                               errors, warnings));
        tee::applyBuild(*p, b);
        if (parked)
            REQUIRE(p->setSpec("valve.inPhaseRange", 0.0, errors));
        auto& c = p->controls();
        c.mode = 2;
        c.dynoSpeed = float(rpm * M_PI / 30.0);
        c.ignition = 2;
        const int block = 256;
        float l[block], r[block];
        const int torque = p->telemetryIndex("engine.torque");
        double sum = 0.0;
        int n = 0;
        for (int i = 0; i < int(6.0 * options.sampleRate / block); ++i)
        {
            for (const tee::HeldValue& h : tee::warmEngine())
                p->instance().write(p->schema(), h.path, h.value, errors);
            const double t = i * block / options.sampleRate;
            c.ignition = t < 1.0 ? 2 : 1;
            c.throttle = t < 1.5 ? 0.0f : 1.0f;
            p->process(l, r, block);
            if (t >= 4.5)
                sum += p->telemetry(std::size_t(torque)), ++n;
        }
        return sum / n;
    }
} // namespace

TEST_CASE("the intake phaser's schedule cuts no hole in a long cam's full-load curve", "[calibration][na]")
{
    struct Point
    {
        const char* build;
        double rpm;
    };
    for (const Point pt : {Point{"straight-six-32-high-rev", 2700.0}, Point{"v12-65", 5000.0}})
    {
        const double scheduled = dynoTorque(pt.build, pt.rpm, false);
        const double parked = dynoTorque(pt.build, pt.rpm, true);
        std::printf("%s at %.0f rpm: %.0f N m scheduled, %.0f parked\n", pt.build, pt.rpm, scheduled, parked);
        CHECK(scheduled >= 0.97 * parked);
    }
}
