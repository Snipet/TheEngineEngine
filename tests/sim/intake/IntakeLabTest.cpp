// tests/sim/intake/IntakeLabTest.cpp: card E's standalone tests of the throttle a runner (sim/intake/itb.fm) and the
// runner of two lengths (sim/intake/runner.fm).
#include "sim/valvetrain/Lab.h"

#include <catch2/catch_approx.hpp>

#include <cmath>
#include <cstdio>
#include <random>

using tee::test::lab::Obj;

namespace
{
    tee::Program& intakeProgram() { return tee::test::lab::program("intake/lab.fm"); }
} // namespace

TEST_CASE("ITB: the trumpet's end is passive at every plate opening, and an open end wide open", "[intake][itb]")
{
    const float aRun = 0.0016f;                       // a 45 mm runner
    const float z = 1.204f * 343.0f / aRun;
    for (const float share : {1.2f, 1.0f, 0.3f, 0.05f, 0.002f})
    {
        Obj end(intakeProgram(), "ItbEnd");
        end.call<void>("setup", aRun, z, 96000.0f);
        end.call<void>("clear");
        auto* tick = end.fn<float(void*, float, float, float)>("tick");
        std::mt19937 rng(7);
        std::normal_distribution<float> noise(0.0f, 3000.0f);
        double in = 0.0;
        double out = 0.0;
        double worst = 0.0;
        for (int i = 0; i < 96000; ++i)
        {
            const float a = noise(rng);
            const float b = tick(end.data(), a, 0.0f, share * aRun);
            REQUIRE(std::isfinite(b));
            in += double(a) * a;
            out += double(b) * b;
            worst = std::min(worst, in - out);
        }
        INFO("plate at " << share << " of the runner: energy back over energy in " << out / in);
        // what comes back never exceeds what went in, at any moment of the run
        CHECK(worst >= -1e-6 * in);
        // (wide open, white noise mostly leaves: an open end of this size radiates what is above some 2 kHz)
        if (share <= 0.002f)
            CHECK(out / in > 0.95);     // a shut plate is a closed end
    }
    // wide open, a low tone comes back inverted, nearly whole: an open end
    Obj end(intakeProgram(), "ItbEnd");
    end.call<void>("setup", aRun, z, 96000.0f);
    end.call<void>("clear");
    auto* tick = end.fn<float(void*, float, float, float)>("tick");
    double corr = 0.0;
    double aa = 0.0;
    for (int i = 0; i < 96000; ++i)
    {
        const float a = 1000.0f * std::sin(2.0f * 3.14159265f * 100.0f * i / 96000.0f);
        const float b = tick(end.data(), a, 0.0f, 2.0f * aRun);
        if (i > 48000)
        {
            corr += double(a) * b;
            aa += double(a) * a;
        }
    }
    CHECK(corr / aa < -0.9);
}

TEST_CASE("runner of two lengths: switches past its speed with hysteresis, and glides", "[intake][runner]")
{
    Obj len(intakeProgram(), "RunnerLength");
    len.call<void>("setup", 0.45f, 0.25f, 500.0f);
    len.call<void>("reset");
    auto* control = len.fn<void(void*, float, float)>("control");
    const float d16 = 16.0f / 96000.0f;
    for (int i = 0; i < 600; ++i)
        control(len.data(), d16, 400.0f);
    CHECK(len.get("now") == Catch::Approx(0.45));
    double last = len.get("now");
    bool smooth = true;
    for (int i = 0; i < 6000; ++i)
    {
        control(len.data(), d16, 520.0f);
        const double now = len.get("now");
        smooth = smooth && std::fabs(now - last) < 0.01 * 0.2;   // under 1 % of the change a tick
        last = now;
    }
    CHECK(smooth);
    CHECK(len.get("now") == Catch::Approx(0.25).margin(1e-4));
    for (int i = 0; i < 600; ++i)
        control(len.data(), d16, 490.0f);          // inside the hysteresis: stays short
    CHECK(len.get("now") == Catch::Approx(0.25).margin(1e-4));
    for (int i = 0; i < 6000; ++i)
        control(len.data(), d16, 480.0f);
    CHECK(len.get("now") == Catch::Approx(0.45).margin(1e-4));
}
