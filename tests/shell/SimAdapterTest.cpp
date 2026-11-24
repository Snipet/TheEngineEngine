// tests/shell/SimAdapterTest.cpp: the simulator (sim/engine.fm through card C's SimHost) as a Player
// (shell/player/SimAdapter.h): the Spec's fields as parameters, the tel block as telemetry by Sources.h's names, and a
// ControlsFrame as the simulator's Controls: a start, an idle, a rev, a stop.
#include "audio/Ftz.h"
#include "player/SimAdapter.h"
#define TEE_REGISTER_SHARED_SIM
#include "support/SharedSim.h"
#include "tee/SimHost.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace tee;

namespace
{
    constexpr double kRadToRpm = 9.549296585513721;
}

TEST_CASE("shell: the simulator's Spec as parameters, its telemetry by name, and a start, a rev and a stop",
          "[shell][sim][sharedsim]")
{
    player::SimAdapter::Options o;
    o.directory = std::filesystem::path(TEE_SOURCE_SIM_DIR);
    std::string errors;
    const std::unique_ptr<player::SimAdapter> sim = test::sharedAdapter(errors, o);
    INFO(errors);
    REQUIRE(sim != nullptr);
    sim->prepare(48000.0, 256);
    ParamSource& params = *sim;
    TelemetrySource& tel = *sim;

    // the first Spec is the reference's dials in their sections
    REQUIRE(params.count() >= 20);
    const int size = params.findParam("engine.displacement");
    REQUIRE(size >= 0);
    CHECK(params.info(size).unit == "L");
    CHECK(params.info(size).scale == 1000.0);
    CHECK(params.get(size) * 1000.0 > 5.0);                 // the reference's 5.76 L, in m^3
    CHECK(params.get(size) * 1000.0 < 6.5);
    for (int i = 0; i < params.count(); ++i)
    {
        const ParamInfo& p = params.info(i);
        INFO(p.id);
        CHECK_FALSE(p.label.empty());
        CHECK(p.max > p.min);
        CHECK(params.get(i) >= p.min);
        CHECK(params.get(i) <= p.max);
        if (p.kind == ParamInfo::Kind::choice)
            CHECK(static_cast<double>(p.choices.size()) == std::round(p.max - p.min) + 1.0);
    }
    CHECK(params.findParam("no.such.field") == -1);

    // Sources.h's names are the tel block's paths
    const int state = tel.findValue(telemetry::kState), speed = tel.findValue(telemetry::kSpeed);
    REQUIRE(state >= 0);
    REQUIRE(speed >= 0);
    CHECK(tel.findValue(telemetry::kRedline) >= 0);
    CHECK(tel.findValue(telemetry::kThrottle) >= 0);
    CHECK(tel.findValue(telemetry::kManifold) >= 0);
    CHECK(tel.findValue(telemetry::kGear) >= 0);
    CHECK(tel.findValue(telemetry::kCarSpeed) >= 0);
    CHECK(tel.findValue(telemetry::kBoost) >= 0);
    CHECK(tel.findValue("no.such.value") == -1);

    std::vector<float> l(256), r(256);
    ControlsFrame frame;
    frame.mode = 1;                                         // MODE_DRIVE: in the car, in neutral
    double peak = 0.0;
    bool finite = true;
    const auto run = [&](double seconds) {
        const audio::ScopedFtz ftz;
        sim->setControls(frame);
        long long left = std::llround(seconds * 48000.0);
        while (left > 0)
        {
            const int n = static_cast<int>(std::min<long long>(left, 256));
            sim->frame();
            sim->process(l.data(), r.data(), n);
            for (int i = 0; i < n; ++i)
            {
                const auto u = static_cast<std::size_t>(i);
                finite = finite && std::isfinite(l[u]) && std::isfinite(r[u]);
                peak = std::max({ peak, std::fabs(static_cast<double>(l[u])), std::fabs(static_cast<double>(r[u])) });
            }
            left -= n;
        }
        sim->frame();               // the telemetry read next is the newest block's
    };

    run(0.2);
    CHECK(tel.value(state) == 0.0);
    frame.ignition = 2;                                     // the start key
    run(0.3);
    frame.ignition = 1;
    run(3.0);
    REQUIRE(tel.value(state) == 2.0);
    const double idle = tel.value(speed) * kRadToRpm;
    CHECK(idle > 400.0);
    CHECK(idle < 1600.0);
    CHECK(peak > 0.001);
    CHECK(tel.outputPeak() > 0.0f);
    std::vector<float> tail(4096);
    CHECK(tel.outputTail(tail.data(), 4096) == 4096);

    frame.throttle = 1.0f;
    run(2.0);
    CHECK(tel.value(speed) * kRadToRpm > 3.0 * idle);
    frame.throttle = 0.0f;
    run(3.0);
    CHECK(tel.value(speed) * kRadToRpm < 2.0 * idle);

    frame.ignition = 0;                                     // off
    run(3.0);
    CHECK(tel.value(state) != 2.0);
    CHECK(finite);
    CHECK(peak <= 1.0);
    CHECK(sim->why().empty());
}

TEST_CASE("shell: a counter of the simulator's Controls pressed once, what the frame does not carry kept",
          "[shell][sim][sharedsim]")
{
    player::SimAdapter::Options o;
    o.directory = std::filesystem::path(TEE_SOURCE_SIM_DIR);
    std::string errors;
    const std::unique_ptr<player::SimAdapter> sim = test::sharedAdapter(errors, o);
    REQUIRE(sim != nullptr);
    REQUIRE(sim->hasControl("strip"));
    CHECK(sim->hasControl("bakeMaps"));          // TUNE's BAKE (card C's SimControls carries it since 2a63c2d)
    CHECK_FALSE(sim->hasControl("no such control"));
    CHECK_FALSE(sim->pressCounter("no such control"));
    REQUIRE(sim->pressCounter("strip"));
    REQUIRE(sim->pressCounter("strip"));
    double strip = 0.0;
    REQUIRE(getControl(sim->host().controls(), "strip", strip));
    CHECK(strip == 2.0);
    // a frame of the driver's does not undo it
    ControlsFrame f;
    f.throttle = 0.5f;
    sim->setControls(f);
    REQUIRE(getControl(sim->host().controls(), "strip", strip));
    CHECK(strip == 2.0);
}

TEST_CASE("shell: an element of each kind of telemetry array is found by name and reads a plausible number",
          "[shell][sim][sharedsim]")
{
    player::SimAdapter::Options o;
    o.directory = std::filesystem::path(TEE_SOURCE_SIM_DIR);
    std::string errors;
    const std::unique_ptr<player::SimAdapter> sim = test::sharedAdapter(errors, o);
    INFO(errors);
    REQUIRE(sim != nullptr);
    const audio::ScopedFtz ftz;
    sim->prepare(48000.0, 256);
    ParamSource& params = *sim;
    TelemetrySource& tel = *sim;
    // the models whose arrays these are: the vehicle's wheels, the boost's shafts (a turbocharger)
    const auto set = [&](const char* id, double v) {
        const int i = params.findParam(id);
        INFO(id);
        REQUIRE(i >= 0);
        params.set(i, v);
    };
    set("car.model", 1.0);
    set("boost.model", 1.0);
    const int kind = params.findParam("boost.kind");
    REQUIRE(kind >= 0);
    for (std::size_t c = 0; c < params.info(kind).choices.size(); ++c)
        if (params.info(kind).choices[c].find("Turbo") != std::string::npos)
        {
            params.set(kind, params.info(kind).min + static_cast<double>(c));
            break;
        }
    sim->frame();

    // the names: an element of each, one and two digits; a whole array, an index past the end, a typo: none
    const char* names[] = { "engine.cylPeak[0]", "engine.firingAngle[1]", "car.wheelLoad[0]", "car.wheelSpeed[3]",
                            "boost.shaftSpeed[0]", "boost.ratio[0]", "valve.benchLift[10]", "ecu.knockByCylinder[2]" };
    for (const char* n : names)
    {
        INFO(n);
        CHECK(tel.findValue(n) >= 0);
    }
    CHECK(tel.findValue("car.wheelLoad") == -1);
    CHECK(tel.findValue("car.wheelLoad[4]") == -1);
    CHECK(tel.findValue("car.wheelLoud[0]") == -1);
    CHECK(tel.findValue("engine.speed[0]") == -1);
    CHECK(tel.findValue("car.wheelLoad[0]") == tel.findValue("car.wheelLoad[0]"));   // found once, kept

    // started and revved
    std::vector<float> l(256), r(256);
    ControlsFrame f;
    f.mode = 1;                 // MODE_DRIVE: the car is stepped (in neutral)
    f.ignition = 2;
    const auto play = [&](double seconds) {
        for (int b = 0; b < static_cast<int>(seconds * 48000.0 / 256.0); ++b)
        {
            sim->process(l.data(), r.data(), 256);
            if (b % 8 == 0)
                sim->frame();
        }
        sim->frame();
    };
    sim->setControls(f);
    play(1.0);
    f.ignition = 1;
    f.throttle = 1.0f;
    sim->setControls(f);
    play(2.0);
    const auto v = [&](const char* n) { return tel.value(tel.findValue(n)); };
    INFO("rpm " << v("engine.speed") * 9.549 << " cylPeak[0] " << v("engine.cylPeak[0]") << " wheelLoad[0] "
                << v("car.wheelLoad[0]") << " shaftSpeed[0] " << v("boost.shaftSpeed[0]") << " ratio[0] "
                << v("boost.ratio[0]"));
    CHECK(v("engine.speed") > 150.0);                       // running and revved
    CHECK(v("engine.cylPeak[0]") > 1.0e6);                  // a cylinder's peak pressure, Pa: more than 10 bar
    CHECK(v("engine.cylPeak[0]") < 3.0e7);
    CHECK(v("car.wheelLoad[0]") > 1000.0);                  // a wheel carries a share of the car's weight, N
    CHECK(v("boost.shaftSpeed[0]") > 100.0);                // the turbocharger's shaft turns, rad/s
    CHECK(v("boost.ratio[0]") >= 1.0);                      // its compressor's pressure ratio
    for (const char* n : names)
    {
        INFO(n);
        CHECK(std::isfinite(v(n)));
    }
}
