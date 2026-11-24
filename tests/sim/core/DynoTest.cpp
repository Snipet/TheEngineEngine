// tests/sim/core/DynoTest.cpp: card D's test 6, standalone: the absorber's loop against a made-up engine (a full-load
// torque curve of 330 to 520 N m with a firing ripple of nine tenths of it, eight pulses a cycle) on a V8's crank.
#include "Presets.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>

using namespace tee::test::core;

namespace
{
    constexpr double kRpm = 30.0 / M_PI;
    constexpr double kDt = 1.0 / 96000.0;

    void v8(Rig& rig)
    {
        applyLayout(rig, loadPreset("ref-12-v8-cross"));
        rig.build(0.1016, 0.0920, 0.155, 10.0, 0.2);
    }
} // namespace

TEST_CASE("the dyno holds a set speed within 10 rpm at full load, and its torque is the brake torque", "[core][dyno]")
{
    auto run = [](double fromRpm, double setRpm) {
        Rig rig;
        v8(rig);
        rig.set("omega", fromRpm / kRpm);
        auto dyno = rig.method<void(void*, int, int, float, float, float, float)>("dynoRun");
        // 3 s: 2 to settle, the last 1 measured
        dyno(rig.data(), 96000 * 3, 96000, float(kDt), float(setRpm / kRpm), 0.0f, 8.0f);
        const double n = rig.get("count");
        const double mean = rig.get("omegaSum") / n * kRpm;
        const double brake = rig.get("absSum") / n;
        const double engine = rig.get("engSum") / n;
        std::printf("dyno from %.0f to %.0f rpm: mean %.2f rpm (ripple %.1f to %.1f), absorber %.1f N m, engine less "
                    "friction %.1f N m\n", fromRpm, setRpm, mean, rig.get("omegaMin") * kRpm, rig.get("omegaMax") * kRpm,
                    brake, engine);
        CHECK(std::abs(mean - setRpm) < 10.0);
        CHECK(std::abs(brake - engine) < 0.01 * std::abs(engine));
        CHECK(std::abs(rig.get("dy.tqMean") - brake) < 0.03 * std::abs(brake));
    };
    run(3000.0, 3000.0);
    run(2000.0, 5500.0);
    run(6500.0, 1500.0);
}

TEST_CASE("the dyno sweeps at its rate", "[core][dyno]")
{
    Rig rig;
    v8(rig);
    rig.set("omega", 2000.0 / kRpm);
    auto dyno = rig.method<void(void*, int, int, float, float, float, float)>("dynoRun");
    // 300 rpm a second (31.4 rad/s^2) for 10 s, from 2000 rpm: 5000 at the end
    const double rate = 300.0 / kRpm;
    dyno(rig.data(), 96000 * 10, 9600, float(kDt), 0.0f, float(rate), 8.0f);
    const double ref = rig.get("dy.ref") * kRpm;
    const double mean = rig.get("omegaSum") / rig.get("count") * kRpm;
    std::printf("sweep at 300 rpm/s for 10 s from 2000: the set speed %.1f rpm, the engine's over the last 0.1 s %.1f "
                "rpm\n", ref, mean);
    CHECK(std::abs(ref - 5000.0) < 1.0);
    // (the last 0.1 s's mean lags the set speed's end by half of it: 15 rpm)
    CHECK(std::abs(mean - (ref - 15.0)) < 10.0);
}
