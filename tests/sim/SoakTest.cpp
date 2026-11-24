// tests/sim/SoakTest.cpp: the simulator stays finite and within full scale under random pedals and dials (the whole
// Spec moved at random within its ranges, the ignition, the mode, the lever and the paddles), and a reset gives the
// samples a fresh instance gives. TEE_SOAK_SECONDS sets the soak's length (20 s in ctest; an hour by hand).
#include "Twin.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <random>

using namespace tee;

TEST_CASE("sim: random pedals and dials stay finite and within full scale", "[sim][soak]")
{
    double seconds = 20.0;
    if (const char* env = std::getenv("TEE_SOAK_SECONDS"); env != nullptr && std::atof(env) > 0.0)
        seconds = std::atof(env);
    auto player = test::enginePlayer(48000.0);
    std::mt19937 random(20261124u);
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    std::vector<float> l(512), r(512);
    double peak = 0.0;
    bool finite = true;
    int running = 0, calls = 0;
    const int total = static_cast<int>(seconds * 48000.0 / 512.0);
    EnginePlayer::Controls& c = player->controls();
    for (int k = 0; k < total; ++k)
    {
        // every quarter of a second something changes; a Spec field now and then (half the time a slow one)
        if (k % 23 == 0)
        {
            c.throttle = static_cast<float>(unit(random) < 0.3 ? 0.0 : unit(random));
            c.brake = static_cast<float>(unit(random) < 0.8 ? 0.0 : unit(random));
            c.clutch = static_cast<float>(unit(random) < 0.7 ? 0.0 : unit(random));
            if (unit(random) < 0.1)
                c.mode = unit(random) < 0.6 ? EnginePlayer::kModeDrive : EnginePlayer::kModeBench;
            if (unit(random) < 0.15)
                c.ignition = unit(random) < 0.5 ? EnginePlayer::kIgnitionStart
                                                : (unit(random) < 0.8 ? EnginePlayer::kIgnitionOn : EnginePlayer::kIgnitionOff);
            else if (c.ignition == EnginePlayer::kIgnitionStart)
                c.ignition = EnginePlayer::kIgnitionOn;
            if (unit(random) < 0.2)
                (unit(random) < 0.6 ? c.shiftUp : c.shiftDown) += 1;
            if (unit(random) < 0.05)
                c.lever = static_cast<int>(unit(random) * 8.0) - 1;
            c.holdSpeed = unit(random) < 0.1 ? static_cast<float>(80.0 + 600.0 * unit(random)) : 0.0f;
            c.scopeOn = unit(random) < 0.5 ? 1 : 0;
            if (unit(random) < 0.3)
            {
                const auto& fields = player->specFields();
                const std::size_t i = static_cast<std::size_t>(unit(random) * static_cast<double>(fields.size()));
                const EnginePlayer::SpecField& f = fields[std::min(i, fields.size() - 1)];
                player->setSpec(std::min(i, fields.size() - 1), f.minimum + (f.maximum - f.minimum) * unit(random));
            }
        }
        player->process(l.data(), r.data(), 512);
        ++calls;
        for (int i = 0; i < 512; ++i)
        {
            finite = finite && std::isfinite(l[i]) && std::isfinite(r[i]);
            peak = std::max(peak, static_cast<double>(std::max(std::fabs(l[i]), std::fabs(r[i]))));
        }
        running += player->telemetry(static_cast<std::size_t>(player->telemetryIndex("engine.state"))) == 2.0 ? 1 : 0;
        if (!finite)
            break;
    }
    std::printf("soak: %.0f s, %d calls, running in %d, peak %.4f, %s\n", seconds, calls, running, peak,
                finite ? "finite" : "NOT FINITE");
    CHECK(finite);
    CHECK(peak <= 1.0);
    CHECK(running > calls / 10);
}

TEST_CASE("sim: a reset gives what a fresh instance gives", "[sim][reset]")
{
    auto player = test::enginePlayer(48000.0);
    const EngineBuild build = test::engineBuild(*player, "v8");
    const EngineScenario scenario = test::engineScenario("drive-car");
    RenderOptions options;
    options.seconds = 6.0;
    RenderResult first, second;
    std::string error;
    REQUIRE(renderEngine(*player, build, scenario, options, first, error));
    REQUIRE(renderEngine(*player, build, scenario, options, second, error));   // renderEngine resets first
    CHECK(test::maxDifference(first.audio.left, second.audio.left) == 0.0);
    CHECK(test::maxDifference(first.audio.right, second.audio.right) == 0.0);
    auto fresh = test::enginePlayer(48000.0);
    RenderResult third;
    REQUIRE(renderEngine(*fresh, build, scenario, options, third, error));
    CHECK(test::maxDifference(first.audio.left, third.audio.left) == 0.0);
}
