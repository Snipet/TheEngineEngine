// tests/host/ScenarioTest.cpp: the test library (card C, section 7) on the stand-in, written as a sim package will
// write its own, and its measures on signals made here; and scenario files.
#include "Support.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>

using namespace tee;
using namespace tee::test;

TEST_CASE("a scenario reads as a sentence", "[scenario]")
{
    const Run r = Scenario()
                      .sim(standin())
                      .at(0.0).ignition(tee::Start).until(1.5).ignition(tee::On)
                      .at(3.0).throttle(1.0).ramp(0.3)
                      .run(8.0);
    INFO(r.error());
    REQUIRE(r.ok());
    CHECK(r.tel("engine.state").at(1.0) == 2);
    CHECK(r.tel("engine.speed").during(6.0, 8.0).max() > 0.95 * r.tel("engine.redline").last());
    CHECK(r.audio().finite());
    CHECK(r.audio().peak() <= 1.0f);
    const Signal held = r.audio().during(6.0, 8.0);
    CHECK(held.line(r.firingHz()).above(held.floor(), 12 /*dB*/));
    CHECK(r.audio().largestStep() < tee::test::clickLimit);
    CHECK(r.tel("engine.throttle").crossing(0.5) > 3.0);
    CHECK(r.tel("engine.throttle").crossing(0.5) < 3.3);
    CHECK(r.host().guardTrips == 0);
    WARN("the stand-in ran at " << r.speed() << " times real time");
}

TEST_CASE("Spec changes, presses and ramps in a scenario", "[scenario]")
{
    const Run r = Scenario()
                      .sim(standin())
                      .at(0.0).ignition(tee::Start).spec("engine.idle", 60.0)
                      .at(1.0).ignition(tee::On).spec("engine.idle", 140.0).ramp(1.0)
                      .at(1.5).shiftUp().shiftUp()
                      .at(2.5).spec("engine.layout", "Inline four")
                      .run(3.0);
    INFO(r.error());
    REQUIRE(r.ok());
    const Series idle = r.tel("engine.idle");
    CHECK(idle.at(0.9) == 60.0);
    CHECK(std::fabs(idle.at(1.5) - 100.0) < 2.0);   // half-way up the ramp
    CHECK(idle.at(2.2) == 140.0);
    CHECK(r.build()->get("engine.layout") == 0);
    // the inline four fires half as often as the V8 at one speed
    const double v8 = r.tel("engine.firing").at(2.4) / r.tel("engine.speed").at(2.4);
    const double i4 = r.tel("engine.firing").last() / r.tel("engine.speed").last();
    CHECK(std::fabs(i4 / v8 - 0.5) < 1e-3);
}

TEST_CASE("a scenario's result does not depend on the block size", "[scenario]")
{
    const auto scenario = [](int block) {
        return Scenario()
            .sim(standin())
            .block(block)
            .at(0.0).ignition(tee::Start)
            .at(0.8).ignition(tee::On).throttle(0.7).ramp(0.2)
            .at(1.3).spec("sound.level", 0.6)
            .run(2.0);
    };
    const Run a = scenario(64), b = scenario(333);
    REQUIRE(a.ok());
    REQUIRE(b.ok());
    CHECK(a.audio().nullDepth(b.audio()) == std::numeric_limits<double>::infinity());
}

TEST_CASE("an error is said, not thrown: a build that is not there, a field that is not", "[scenario]")
{
    CHECK_FALSE(Scenario("no-such-build").sim(standin()).run(0.1).ok());
    const Run r = Scenario().sim(standin()).spec("engine.nothing", 1).run(0.1);
    CHECK_FALSE(r.ok());
    CHECK(r.error().find("engine.nothing") != std::string::npos);
}

TEST_CASE("the measures: a line above the floor, a click, a level, a null", "[scenario][measures]")
{
    const double rate = 48000.0;
    auto tone = std::make_shared<std::vector<float>>(48000);
    for (std::size_t i = 0; i < tone->size(); ++i)
        (*tone)[i] = static_cast<float>(0.5 * std::sin(2.0 * 3.14159265358979 * 440.0 * static_cast<double>(i) / rate)
                                        + 0.001 * std::sin(static_cast<double>(i) * 1.7));
    const Signal s(tone, rate, 0, tone->size());
    CHECK(std::fabs(s.line(440.0).db - 20.0 * std::log10(0.5)) < 0.1);
    CHECK(s.line(440.0).above(s.floor(), 40.0));
    CHECK(s.line(1000.0).below(s.line(440.0), 40.0));
    CHECK(std::fabs(s.level().db - 20.0 * std::log10(0.5)) < 0.1);
    CHECK(s.largestStep() < 2.0);
    CHECK(std::fabs(s.spectrum().peakHz(100.0, 2000.0) - 440.0) < 2.0);

    auto clicked = std::make_shared<std::vector<float>>(*tone);
    (*clicked)[24000] += 0.3f;
    const Signal c(clicked, rate, 0, clicked->size());
    CHECK(c.largestStep() > clickLimit);
    CHECK(c.nullDepth(s) < 60.0);
    CHECK(s.nullDepth(s) == std::numeric_limits<double>::infinity());
    CHECK(s.during(0.25, 0.5).seconds() == 0.25);

    const Series ramp({0, 1, 2, 3, 4, 5}, {0, 2, 4, 5, 5.01, 5});
    CHECK(ramp.crossing(3.0) == 2);
    CHECK(std::isnan(ramp.crossing(9.0)));
    CHECK(ramp.settledWithin(0.05) == 3);
    CHECK(ramp.during(1, 3).mean() == (2 + 4 + 5) / 3.0);
}

TEST_CASE("a scenario file: parsed, run and its assertions checked", "[scenario][file]")
{
    const char* text = R"({
        "format": "tee-scenario", "version": 1, "name": "rev", "description": "start, rev, hold",
        "rate": 44100, "block": 256, "seconds": 5,
        "events": [
            {"t": 0, "controls": {"ignition": 2}},
            {"t": 1, "controls": {"ignition": 1}},
            {"t": 2, "controls": {"throttle": 1}, "ramp": 0.2},
            {"t": 2.5, "press": ["shiftUp", "shiftUp"]},
            {"t": 3, "spec": {"engine.redline": 600, "engine.layout": "V12"}}
        ],
        "assertions": [
            {"tel": "engine.speed", "measure": "max", "during": [3.5, 5], "within": [550, 640],
             "source": "the limiter holds it at the redline"},
            {"tel": "engine.state", "measure": "last", ">=": 2},
            {"audio": "finite"},
            {"audio": "peak", "<=": 1},
            {"audio": "step", "during": [1, 5], "<": 12}
        ]})";
    Timeline t;
    std::string error;
    REQUIRE(parseTimeline(text, "rev.json", t, error));
    CHECK(t.rate == 44100);
    CHECK(t.assertions.size() == 5);

    SimHost host;
    REQUIRE(host.attach(standinSim(), offline(t.rate, t.block), error));
    Build build(host.fields());
    build.attach(&host);
    RunResult run;
    REQUIRE(runTimeline(host, build, t, {}, run, error));
    std::vector<std::string> lines;
    const bool pass = checkAssertions(t, run, lines);
    for (const std::string& line : lines)
        UNSCOPED_INFO(line);
    CHECK(pass);
    CHECK(host.controls().shiftUp == 2);
    CHECK(build.get("engine.layout") == 2);

    CHECK_FALSE(parseTimeline(R"({"format": "tee-scenario", "events": [{"t": 1, "controls": {"warp": 1}}]})", "x", t, error));
    CHECK(error.find("warp") != std::string::npos);
}
