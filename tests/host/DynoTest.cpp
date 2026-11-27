// tests/host/DynoTest.cpp: the dyno runner (card C, section 5): a curve that is the stand-in's torque law, steps, the
// service's cache and its dropping of older requests, and tee-dyno's curve equal to the runner's.
#include "Support.h"

#include "tee/Dyno.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <thread>
#if !defined(_WIN32)
#include <sys/wait.h>
#endif

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

// X-062: a curve for a build before the last request is never handed back as the newest. The service ran request A to its
// end and held its curve unread (take() not yet called) when request B came; take() then gave A's curve, the dyno of the
// build before the owner's last edit (macOS, efec3052's run 46019040854: "the newest request only" read another
// request's curve). Here made certain: A is run to its end first (asked again until its cache answers), then B is asked.
TEST_CASE("a curve finished before a newer request is never taken as the newest", "[dyno][service]")
{
    const auto sim = standinSim();
    DynoService service(sim);
    Build build(sim->fields());
    DynoPlan plan;
    plan.steps = {200, 300};
    DynoCurve curve;
    build.set("engine.torque", 300);
    CHECK_FALSE(service.request(build, plan, curve));
    for (int wait = 0; wait < 600 && !service.request(build, plan, curve); ++wait)   // A done: its cache answers
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    REQUIRE(service.request(build, plan, curve));
    build.set("engine.torque", 500);   // B, the owner's next edit
    CHECK_FALSE(service.request(build, plan, curve));
    DynoCurve done;
    for (int wait = 0; wait < 600 && !service.take(done); ++wait)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    INFO(service.error());
    REQUIRE(done.points.size() == 2);
    CHECK(done.key == (build.hash() ^ (plan.hash() * 31u)));   // B's, not A's
    CHECK(std::fabs(done.points[0].torque - (500.0 - 0.08 * 200 - 5.0)) < 0.5);
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
#if defined(_WIN32)
    const std::string line = "\"" + command + "\"";   // cmd /c strips the outer pair of quotes
#else
    const std::string& line = command;
#endif
    REQUIRE(std::system(line.c_str()) == 0);
    std::ifstream in(out / "tool.csv");
    std::ostringstream text;
    text << in.rdbuf();
    CHECK(text.str() == curve.csv());
}

TEST_CASE("tee-dyno: an unknown --fields field is an error naming the nearest, and the last run's csv is gone",
          "[dyno][tools]")
{
    const std::filesystem::path out = std::filesystem::path(TEE_SOURCE_DIR) / "out" / "tests" / "dyno-unknown";
    std::filesystem::create_directories(out);
    std::ofstream(out / "tool.csv") << "last run's";
    const std::string command = std::string("\"") + TEE_BIN_DIR + "/tee-dyno\" --quiet --sim \"" + standin().string()
                                + "\" --from 200 --to 300 --rate 80 --fields engine.manifld --csv \""
                                + (out / "tool.csv").string() + "\" 2> \"" + (out / "stderr.txt").string() + "\"";
#if defined(_WIN32)
    const std::string line = "\"" + command + "\"";   // cmd /c strips the outer pair of quotes
    CHECK(std::system(line.c_str()) == 1);
#else
    const int status = std::system(command.c_str());
    CHECK((WIFEXITED(status) ? WEXITSTATUS(status) : -1) == 1);
#endif
    std::ifstream in(out / "stderr.txt");
    std::ostringstream said;
    said << in.rdbuf();
    INFO(said.str());
    CHECK(said.str().find("engine.manifld") != std::string::npos);
    CHECK(said.str().find("nearest: engine.manifold") != std::string::npos);
    CHECK_FALSE(std::filesystem::exists(out / "tool.csv"));
}
