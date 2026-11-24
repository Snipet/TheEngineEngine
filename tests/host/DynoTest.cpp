// tests/host/DynoTest.cpp: the dyno runner (card C, section 5): a curve that is the stand-in's torque law, steps, the
// service's cache and its dropping of older requests, and tee-dyno's curve equal to the runner's.
#include "Support.h"

#include "tee/Dyno.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <thread>

using namespace tee;
using namespace tee::test;

namespace
{
    // The stand-in's brake torque at full throttle with no torque curve and no spark map: torque - friction w - 5.
    double expected(const Build& b, double w)
    {
        return b.get("engine.torque") - b.get("engine.friction") * w - 5.0;
    }
} // namespace

TEST_CASE("a sweep's curve is the stand-in's torque law, and the runner says how fast it ran", "[dyno]")
{
    const auto sim = standinSim();
    Build build(sim->fields());
    build.set("engine.redline", 1200);
    DynoPlan plan;
    plan.from = 150;
    plan.to = 650;
    plan.rate = 100;
    plan.spacing = 25;
    plan.fields = {"engine.manifold"};
    DynoCurve curve;
    std::string error;
    double lastProgress = 0.0;
    REQUIRE(runDyno(sim, build, plan, curve, error, [&](double p) { lastProgress = p; }));
    INFO(error);
    REQUIRE(curve.points.size() >= 18);
    for (const DynoPoint& p : curve.points)
    {
        CHECK(p.speed >= 140);
        CHECK(p.speed <= 660);
        CHECK(std::fabs(p.torque - expected(build, p.speed)) < 0.5);
        CHECK(std::fabs(p.power - p.torque * p.speed) < 1e-6 * std::fabs(p.power) + 1e-9);
        REQUIRE(p.fields.size() == 1);
        CHECK(p.fields[0] == 101325.0);   // the manifold at full throttle
    }
    CHECK(lastProgress == 1.0);
    CHECK(curve.realTime() > 1.0);
    WARN("the dyno ran the stand-in at " << curve.realTime() << " times real time (" << curve.simulatedSeconds
                                         << " s simulated)");
    CHECK(curve.chart().find('T') != std::string::npos);
}

TEST_CASE("steps, a part throttle, and a torque curve the Spec gives", "[dyno]")
{
    const auto sim = standinSim();
    Build build(sim->fields());
    build.setCurve("engine.torqueShape", {0, 400, 800}, {0.5, 1.0, 0.5});
    DynoPlan plan;
    plan.steps = {200, 400, 600};
    plan.pedal = 0.5;
    DynoCurve curve;
    std::string error;
    REQUIRE(runDyno(sim, build, plan, curve, error));
    REQUIRE(curve.points.size() == 3);
    const double shape[] = {0.75, 1.0, 0.75};
    for (std::size_t k = 0; k < 3; ++k)
    {
        const double w = plan.steps[k];
        CHECK(std::fabs(curve.points[k].speed - w) < 1e-3);
        const double want = 500.0 * shape[k] * 0.5 - 0.08 * w - 5.0;
        CHECK(std::fabs(curve.points[k].torque - want) < 0.5);
    }
}

TEST_CASE("the dyno names what the program lacks", "[dyno]")
{
    const auto sim = standinSim();
    Build build(sim->fields());
    DynoPlan plan;
    plan.fields = {"boost.nothing"};
    DynoCurve curve;
    std::string error;
    CHECK_FALSE(runDyno(sim, build, plan, curve, error));
    CHECK(error.find("boost.nothing") != std::string::npos);
}

TEST_CASE("the service runs the newest request only, and answers a repeat from its cache", "[dyno][service]")
{
    const auto sim = standinSim();
    DynoService service(sim);
    Build build(sim->fields());
    DynoPlan plan;
    plan.steps = {200, 300};
    DynoCurve curve;
    for (int i = 0; i < 5; ++i)
    {
        build.set("engine.torque", 300 + 50 * i);   // the interface asks after each edit
        CHECK_FALSE(service.request(build, plan, curve));
    }
    DynoCurve done;
    for (int wait = 0; wait < 600 && !service.take(done); ++wait)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    INFO(service.error());
    REQUIRE(done.points.size() == 2);
    CHECK(std::fabs(done.points[0].torque - (500.0 - 0.08 * 200 - 5.0)) < 0.5);   // the newest edit's
    CHECK(done.key == (build.hash() ^ (plan.hash() * 31u)));
    REQUIRE(service.request(build, plan, curve));   // cached
    CHECK(curve.points.size() == 2);
}

TEST_CASE("tee-dyno's curve equals the runner's", "[dyno][tools]")
{
    const std::filesystem::path out = std::filesystem::path(TEE_SOURCE_DIR) / "out" / "tests" / "dyno";
    std::filesystem::create_directories(out);
    const auto sim = standinSim();
    Build build(sim->fields());
    build.set("engine.torque", 420);
    build.setCurve("engine.torqueShape", {0, 300, 700}, {0.7, 1.0, 0.6});
    std::string error;
    REQUIRE(build.saveFile(out / "build.json", BuildPart::Whole, error));
    DynoPlan plan;
    plan.from = 200;
    plan.to = 500;
    plan.rate = 80;
    plan.fields = {"engine.manifold", "engine.firing"};
    DynoCurve curve;
    REQUIRE(runDyno(sim, build, plan, curve, error));

    const std::string command = std::string("\"") + TEE_BIN_DIR + "/tee-dyno\" --quiet --sim \"" + standin().string()
                                + "\" --build \"" + (out / "build.json").string() + "\" --from 200 --to 500 --rate 80"
                                + " --fields engine.manifold,engine.firing --csv \"" + (out / "tool.csv").string() + "\"";
    REQUIRE(std::system(command.c_str()) == 0);
    std::ifstream in(out / "tool.csv");
    std::ostringstream text;
    text << in.rdbuf();
    CHECK(text.str() == curve.csv());
}
