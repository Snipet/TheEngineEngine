// tests/host/PredictTest.cpp: the predicted curve (tee/Predict.h): steady points that are the stand-in's torque law,
// coarse first then refined, the newest request only, the cache, cancelling; and the real V8 through the host's brake.
#include "Support.h"

#include "tee/Predict.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <thread>

using namespace tee;
using namespace tee::test;

namespace
{
    // The stand-in's brake torque at full throttle with no torque curve: torque - friction w - 5 (as DynoTest.cpp).
    double expected(const Build& b, double w)
    {
        return b.get("engine.torque") - b.get("engine.friction") * w - 5.0;
    }

    // Waits for the service's refined curve (or `seconds`), keeping each curve taken.
    std::vector<PredictCurve> until(PredictService& service, double seconds)
    {
        std::vector<PredictCurve> taken;
        const auto end = std::chrono::steady_clock::now() + std::chrono::duration<double>(seconds);
        while (std::chrono::steady_clock::now() < end)
        {
            PredictCurve c;
            if (service.take(c))
            {
                taken.push_back(c);
                if (c.refined)
                    break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return taken;
    }
} // namespace

TEST_CASE("predicted points are the stand-in's torque law, between its idle and its redline", "[predict]")
{
    const auto sim = standinSim();
    Build build(sim->fields());
    build.set("engine.redline", 700);
    PredictOptions options;
    options.threads = 2;
    options.points = 12;
    options.sampleRate = 48000.0;
    PredictCurve curve;
    std::string error;
    REQUIRE(predictCurve(sim, build, options, curve, error));
    INFO(error);
    CHECK(curve.brake == "program");
    CHECK(curve.refined);
    REQUIRE(curve.points.size() == 12);
    const double idle = build.get("engine.idle");
    CHECK(std::fabs(curve.points.front().speed - std::max(idle, 1000.0 * 3.14159265358979 / 30.0)) < 1e-6);
    CHECK(std::fabs(curve.points.back().speed - (700.0 - 100.0 * 3.14159265358979 / 30.0)) < 1e-3);
    for (const PredictPoint& p : curve.points)
    {
        CHECK(p.settled);
        CHECK(std::fabs(p.torque - expected(build, p.speed)) < 0.5);
        CHECK(std::fabs(p.power - p.torque * p.speed) < 1e-9 * std::fabs(p.power) + 1e-9);
        CHECK(p.cycles == options.minCycles);   // a steady law agrees at once
    }
}

TEST_CASE("the service gives the coarse curve first, then the refined one, and a repeat from its cache", "[predict]")
{
    const auto sim = standinSim();
    PredictOptions options;
    options.threads = 2;
    options.points = 16;
    options.coarse = 6;
    options.from = 150;
    options.to = 650;
    options.sampleRate = 24000.0;
    options.minCycles = 40;   // slow enough that the coarse curve is seen before the rest
    options.maxCycles = 40;
    PredictService service(sim, options);
    Build build(sim->fields());
    const std::uint64_t number = service.request(build);
    const std::vector<PredictCurve> taken = until(service, 60.0);
    REQUIRE(!taken.empty());
    CHECK(taken.back().refined);
    CHECK(taken.back().points.size() == 16);
    CHECK(taken.back().request == number);
    CHECK_FALSE(taken.back().cached);
    // the first one taken is the coarse curve (or more of it): every coarse speed is in it
    CHECK_FALSE(taken.front().refined);
    REQUIRE(taken.front().points.size() >= 6);
    CHECK(taken.front().points.front().speed == 150.0);
    CHECK(taken.front().points.back().speed == 650.0);
    CHECK(service.error().empty());

    // the same build again: from the cache, at once
    const std::uint64_t again = service.request(build);
    PredictCurve cached;
    REQUIRE(service.take(cached));
    CHECK(cached.cached);
    CHECK(cached.refined);
    CHECK(cached.request == again);
    CHECK(cached.points.size() == 16);
    for (std::size_t i = 0; i < cached.points.size(); ++i)
        CHECK(cached.points[i].torque == taken.back().points[i].torque);
}

TEST_CASE("a newer request replaces the one running; only its curves come", "[predict]")
{
    const auto sim = standinSim();
    PredictOptions options;
    options.threads = 2;
    options.from = 150;
    options.to = 650;
    options.sampleRate = 24000.0;
    options.minCycles = 30;
    options.maxCycles = 30;
    PredictService service(sim, options);
    Build first(sim->fields());
    Build second(sim->fields());
    second.set("engine.torque", 300);
    service.request(first);
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    const std::uint64_t number = service.request(second);
    const std::vector<PredictCurve> taken = until(service, 60.0);
    REQUIRE(!taken.empty());
    for (const PredictCurve& c : taken)
    {
        CHECK(c.request == number);
        for (const PredictPoint& p : c.points)
            CHECK(std::fabs(p.torque - expected(second, p.speed)) < 0.5);
    }
    CHECK(taken.back().refined);

    // cancel: nothing more comes, and the service is idle again
    Build third(sim->fields());
    third.set("engine.torque", 400);
    service.request(third);
    service.cancel();
    const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (service.busy() && std::chrono::steady_clock::now() < end)
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    CHECK_FALSE(service.busy());
    PredictCurve none;
    CHECK_FALSE(service.take(none));
}

TEST_CASE("the real V8's predicted curve: braked by the host, coarse within seconds, and a V8's numbers",
          "[predict][engine][sim]")
{
    const std::filesystem::path simDir = std::filesystem::path(TEE_SOURCE_DIR) / "sim";
    if (!std::filesystem::exists(simDir / "engine.fm"))
        SKIP("no simulator");
    std::string error;
    const auto sim = compiled(simDir, {}, error);
    INFO(error);
    REQUIRE(sim != nullptr);
    Build build(sim->fields());
    std::vector<std::string> warnings;
    REQUIRE(build.loadFile(std::filesystem::path(TEE_SOURCE_DIR) / "content/builds/v8.json", BuildPart::Whole, warnings,
                           error));
    PredictService service(sim);
    const auto t0 = std::chrono::steady_clock::now();
    service.request(build);
    const std::vector<PredictCurve> taken = until(service, 120.0);
    const double wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    REQUIRE(!taken.empty());
    const PredictCurve& curve = taken.back();
    REQUIRE(curve.refined);
    CHECK(curve.points.size() == 16);
    CHECK(curve.brake == (sim->fields().find("tel.engine.torque") != nullptr ? "program" : "host"));
    double peakTorque = 0.0, peakPower = 0.0;
    for (const PredictPoint& p : curve.points)
    {
        CHECK(std::isfinite(p.torque));
        peakTorque = std::max(peakTorque, p.torque);
        peakPower = std::max(peakPower, p.power);
    }
    // a 6.2 L V8 at full throttle: some 400 to 900 N m and 150 to 450 kW at its best (the model's, not a target)
    CHECK(peakTorque > 400.0);
    CHECK(peakTorque < 900.0);
    CHECK(peakPower > 150e3);
    CHECK(peakPower < 450e3);
    WARN("the V8's predicted curve: " << service.threads() << " threads, the first in " << taken.front().wallSeconds
                                      << " s, refined in " << wall << " s, " << curve.simulatedSeconds
                                      << " s of engine time; peak " << peakTorque << " N m, " << peakPower / 1000.0
                                      << " kW");
}
