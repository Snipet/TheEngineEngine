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
    options.threads = 1;      // one worker takes the coarse speeds first, in order: a coarse curve is always made
    options.points = 16;
    options.coarse = 6;
    options.from = 150;
    options.to = 650;
    options.sampleRate = 24000.0;
    PredictService service(sim, options);
    Build build(sim->fields());
    const std::uint64_t number = service.request(build);
    const std::vector<PredictCurve> taken = until(service, 60.0);
    REQUIRE(!taken.empty());
    CHECK(taken.back().refined);
    CHECK(taken.back().points.size() == 16);
    CHECK(taken.back().request == number);
    CHECK_FALSE(taken.back().cached);
    // whatever take() saw (a fast machine may be done before the first take): the coarse curve, then one curve more
    // for each speed past it, the refined one last, were made; and every curve taken before it was a partial one with
    // every coarse speed in it, each with more points than the one before
    CHECK(service.curvesMade() == 1 + (16 - 6));
    for (std::size_t i = 0; i + 1 < taken.size(); ++i)
    {
        CHECK_FALSE(taken[i].refined);
        REQUIRE(taken[i].points.size() >= 6);
        CHECK(taken[i].points.front().speed == 150.0);
        CHECK(taken[i].points.back().speed == 650.0);
        CHECK(taken[i + 1].points.size() > taken[i].points.size());
    }
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
    CHECK(curve.brake == "host");   // the reference's crank: the host holds it
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

namespace
{
    void ratesAgree(const std::vector<const char*>& names)
    {
        const std::filesystem::path source = std::filesystem::path(TEE_SOURCE_DIR) / "sim";
        if (!std::filesystem::exists(source / "engine.fm"))
            SKIP("no simulator");
        std::string error;
        // the program as it is, if it takes TEE_ENGINE_RATE_48000; else a copy under out/ with the hook card D is asked
        // for (engine.fm's `rate = 96000.0f;` chosen by #ifdef), so that the service's side is tested meanwhile
        std::filesystem::path dir = source;
        auto sim = compiled(dir, {}, error);
        REQUIRE(sim != nullptr);
        std::shared_ptr<Sim> inner = simAtInnerRate(sim, 48000.0, error);
        if (!inner)
        {
            dir = std::filesystem::path(TEE_SOURCE_DIR) / "out" / "tests" / "sim48";
            std::filesystem::remove_all(dir);
            std::filesystem::copy(source, dir, std::filesystem::copy_options::recursive);
            std::ifstream in(dir / "engine.fm");
            std::stringstream text;
            text << in.rdbuf();
            std::string program = text.str();
            const std::string line = "        rate = 96000.0f;\n";
            if (program.find(line) == std::string::npos)
                SKIP("engine.fm neither takes TEE_ENGINE_RATE_48000 nor sets rate = 96000.0f where the test can hook it");
            program.replace(program.find(line), line.size(),
                            "#ifdef TEE_ENGINE_RATE_48000\n        rate = 48000.0f;\n#else\n        rate = 96000.0f;\n#endif\n");
            std::ofstream(dir / "engine.fm") << program;
            sim = compiled(dir, {}, error);
            REQUIRE(sim != nullptr);
            inner = simAtInnerRate(sim, 48000.0, error);
            WARN("sim/ does not take TEE_ENGINE_RATE_48000 yet: tested on a hooked copy");
        }
        INFO(error);
        REQUIRE(inner != nullptr);
        for (const char* name : names)
        {
            CAPTURE(name);
            Build build(sim->fields());
            std::vector<std::string> warnings;
            REQUIRE(build.loadFile(std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / (std::string(name) + ".json"),
                                   BuildPart::Whole, warnings, error));
            PredictOptions options;
            PredictCurve full, half;
            REQUIRE(predictCurve(sim, build, options, full, error));
            options.innerRate = 48000.0;
            REQUIRE(predictCurve(sim, build, options, half, error, nullptr, inner));
            CHECK(full.innerRate == 96000.0);
            const bool machine = build.fieldOf("boost.model") >= 0 && build.get("boost.model") != 0.0;
            CHECK(half.innerRate == (machine ? 96000.0 : 48000.0));
            REQUIRE(full.points.size() == half.points.size());
            double peak = 0.0, worst = 0.0;
            for (const PredictPoint& p : full.points)
                peak = std::max(peak, std::fabs(p.torque));
            // A petrol engine within 3 % (1.4 to 1.8 % seen on Linux, 2.0 on macOS). The diesel within 5 % on its plateau
            // only (its points at 90 % of its peak or more): between 1300 and 1650 rpm its charge lights only just (a jump
            // of speed misfires it for some cycles) and above its governed speed the governor takes the fuel, so a small
            // difference (another rate, another processor's rounding: 3.7 % on Linux, 11.6 % on macOS) sends a point there
            // either way. Those points are not compared: the curve's shape there is not a property of the rate.
            const bool diesel = build.fieldOf("fuel.kind") >= 0 && build.get("fuel.kind") != 0.0;
            for (std::size_t i = 0; i < full.points.size(); ++i)
            {
                CHECK(full.points[i].speed == half.points[i].speed);
                // (a point on the band's edge can be lit at one rate and not at the other: the plateau is where both are)
                if (diesel && (full.points[i].torque < 0.9 * peak || half.points[i].torque < 0.9 * peak))
                    continue;
                worst = std::max(worst, std::fabs(half.points[i].torque - full.points[i].torque) / peak);
            }
            CHECK(worst < (diesel ? 0.05 : 0.03));
            WARN(name << ": 48 kHz against 96 kHz, worst " << 100.0 * worst << " % of the peak; engine time "
                      << half.simulatedSeconds << " s against " << full.simulatedSeconds << " s, wall " << half.wallSeconds
                      << " s against " << full.wallSeconds << " s");
        }
    }
} // namespace

TEST_CASE("the curve at 48 kHz inside is the 96 kHz curve within 3 % of the peak torque: the V8", "[predict][engine][sim]")
{
    ratesAgree({"v8"});   // the quick form; every shipped build: [nightly] below
}

TEST_CASE("the curve at 48 kHz inside is the 96 kHz curve within 3 % of the peak torque (a diesel 5 % on its plateau), on each shipped build",
          "[predict][engine][sim][nightly]")
{
    ratesAgree({"default", "v8", "v10", "large-diesel"});
}
