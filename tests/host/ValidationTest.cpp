// tests/host/ValidationTest.cpp: the validation machine (card C, section 8) and ctest's fast subset of
// content/validation/, and content/scenarios/' own assertions, on the simulator once it is there.
#include "Support.h"

#include "tee/Validation.h"

#include <catch2/catch_test_macros.hpp>

using namespace tee;
using namespace tee::test;

namespace
{
    const std::filesystem::path kRoot = TEE_SOURCE_DIR;

    bool haveSim()
    {
        return std::filesystem::exists(kRoot / "sim" / "engine.fm");
    }
} // namespace

TEST_CASE("a validation file is read, refused without its sources, and run on the stand-in", "[validation]")
{
    Validation v;
    std::string error;
    CHECK_FALSE(Validation::parse(R"({"format": "tee-validation", "targets": [{"scenario": "x", "range": [0, 1]}]})", "v",
                                  v, error));
    CHECK(error.find("source") != std::string::npos);

    const std::filesystem::path dir = kRoot / "out" / "tests" / "validation";
    std::filesystem::create_directories(dir / "scenarios");
    std::ofstream(dir / "scenarios" / "run.json") << R"({"format": "tee-scenario", "seconds": 4, "events": [
        {"t": 0, "ctl": {"ignition": 2}}, {"t": 1, "ctl": {"ignition": 1, "throttle": 1}}]})";
    REQUIRE(Validation::parse(R"({"format": "tee-validation", "version": 1, "targets": [
        {"name": "limiter", "scenario": "run", "tel": "engine.speed", "measure": "max", "during": [3, 4],
         "relativeTo": "engine.redline", "range": [0.97, 1.03], "source": "the stand-in cuts at its redline"},
        {"name": "a level", "scenario": "run", "audio": "peak", "range": [0.5, 1], "source": "too loud for the stand-in"},
        {"name": "torque", "dyno": {"steps": [300]}, "measure": "maxTorque", "range": [450, 480],
         "source": "500 - 0.08 * 300 - 5"},
        {"name": "absent", "scenario": "run", "tel": "boost.nothing", "measure": "max", "range": [0, 1], "source": "-"}]})",
                              "v", v, error));
    std::vector<ValidationRow> rows;
    REQUIRE(runValidation(standinSim(), v, dir, false, rows, error));
    REQUIRE(rows.size() == 4);
    CHECK(rows[0].pass);
    CHECK_FALSE(rows[1].pass);
    CHECK(rows[2].pass);
    CHECK(std::fabs(*rows[2].measured - 471.0) < 0.5);
    CHECK_FALSE(rows[3].measured.has_value());
    CHECK(validationTable(rows).find("FAIL") != std::string::npos);
}

TEST_CASE("the fast targets of content/validation pass on the simulator", "[validation][sim]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    auto sim = compiled(kRoot / "sim", {}, error);
    REQUIRE(sim != nullptr);
    for (const auto& entry : std::filesystem::directory_iterator(kRoot / "content" / "validation"))
    {
        CAPTURE(entry.path().filename().string());
        Validation v;
        REQUIRE(Validation::loadFile(entry.path(), v, error));
        std::vector<ValidationRow> rows;
        REQUIRE(runValidation(sim, v, kRoot / "content", true, rows, error));
        UNSCOPED_INFO(validationTable(rows));
        for (const ValidationRow& r : rows)
            CHECK((!r.measured || r.pass));
    }
}

TEST_CASE("content/scenarios' assertions hold on the default build", "[scenario][sim]")
{
    if (!haveSim())
        SKIP("sim/engine.fm is not there yet");
    std::string error;
    auto sim = compiled(kRoot / "sim", {}, error);
    REQUIRE(sim != nullptr);
    for (const auto& entry : std::filesystem::directory_iterator(kRoot / "content" / "scenarios"))
    {
        CAPTURE(entry.path().filename().string());
        Timeline t;
        REQUIRE(loadTimeline(entry.path(), t, error));
        SimHost host;
        REQUIRE(host.attach(sim, offline(t.rate, t.block), error));
        Build build(host.fields());
        build.attach(&host);
        RunResult run;
        REQUIRE(runTimeline(host, build, t, {}, run, error));
        std::vector<std::string> lines;
        const bool pass = checkAssertions(t, run, lines);
        for (const std::string& line : lines)
            UNSCOPED_INFO(line);
        CHECK(pass);
    }
}
