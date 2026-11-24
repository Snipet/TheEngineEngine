// tests/shell/RefAdapterTest.cpp: the reference engine as a Player (shell/player/RefAdapter.h): its dials as
// parameters, its show* fields as telemetry in SI and raw, and a ControlsFrame as its pedals, paddles and start key.
#include "audio/Ftz.h"
#include "player/RefAdapter.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace tee;
using Catch::Approx;

namespace
{
    constexpr double kRadToRpm = 9.549296585513721;

    struct Rig
    {
        std::unique_ptr<player::RefAdapter> ref;
        std::vector<float> l = std::vector<float>(256), r = std::vector<float>(256);
        ControlsFrame frame;
        double peak = 0.0;
        bool finite = true;

        explicit Rig(double rate = 48000.0)
        {
            player::RefAdapter::Options o;
            o.directory = std::filesystem::path(TEE_SOURCE_SIM_DIR) / "ref";
            o.sampleRate = rate;
            std::string errors;
            ref = player::RefAdapter::create(o, errors);
            INFO(errors);
            REQUIRE(ref != nullptr);
            ref->prepare(rate, 256);
        }

        ParamSource& params() { return *ref; }
        TelemetrySource& tel() { return *ref; }
        double value(std::string_view name) { return tel().value(tel().findValue(name)); }
        double rpm() { return value(telemetry::kSpeed) * kRadToRpm; }
        void set(std::string_view id, double v)
        {
            const int i = params().findParam(id);
            REQUIRE(i >= 0);
            params().set(i, v);
        }

        void run(double seconds, int block = 256)
        {
            const audio::ScopedFtz ftz;
            ref->setControls(frame);
            long long left = std::llround(seconds * 48000.0);
            while (left > 0)
            {
                const int n = static_cast<int>(std::min<long long>(left, block));
                ref->process(l.data(), r.data(), n);
                for (int i = 0; i < n; ++i)
                {
                    finite = finite && std::isfinite(l[static_cast<std::size_t>(i)]) && std::isfinite(r[static_cast<std::size_t>(i)]);
                    peak = std::max({ peak, std::fabs(static_cast<double>(l[static_cast<std::size_t>(i)])),
                                      std::fabs(static_cast<double>(r[static_cast<std::size_t>(i)])) });
                }
                left -= n;
            }
        }
    };
}

TEST_CASE("shell: the reference's dials are parameters with their ranges, names and choices", "[shell][ref]")
{
    Rig rig;
    ParamSource& p = rig.params();
    REQUIRE(p.count() == 32);
    const int size = p.findParam("size");
    REQUIRE(size >= 0);
    CHECK(p.info(size).label == "SIZE");
    CHECK(p.info(size).unit == "L");
    CHECK(p.info(size).kind == ParamInfo::Kind::real);
    CHECK(p.info(size).min == Approx(0.05));
    CHECK(p.info(size).max == Approx(30.0));
    CHECK(p.info(size).def == Approx(5.76));
    CHECK(p.info(size).logTaper);
    CHECK(p.get(size) == Approx(5.76));
    const int engine = p.findParam("engine");
    REQUIRE(engine >= 0);
    CHECK(p.info(engine).kind == ParamInfo::Kind::choice);
    REQUIRE(p.info(engine).choices.size() == 17);
    CHECK(p.info(engine).choices[12] == "V8 CROSS");
    CHECK(p.get(engine) == 12.0);                           // the reference is a crossplane V8
    const int starter = p.findParam("starter");
    REQUIRE(starter >= 0);
    CHECK(p.info(starter).kind == ParamInfo::Kind::toggle);
    CHECK(p.findParam("no such dial") == -1);
    // every dial has a label and sits inside its range
    for (int i = 0; i < p.count(); ++i)
    {
        const ParamInfo& info = p.info(i);
        CHECK_FALSE(info.label.empty());
        CHECK(info.max > info.min);
        CHECK(p.get(i) >= info.min);
        CHECK(p.get(i) <= info.max);
    }
    // a set is held to the range, and a whole number stays whole
    p.set(size, 99.0);
    CHECK(p.get(size) == Approx(30.0));
    p.set(size, std::nan(""));
    CHECK(p.get(size) == Approx(30.0));
    p.set(engine, 13.6);
    CHECK(p.get(engine) == 14.0);
}

TEST_CASE("shell: the reference starts, idles, revs and switches off from a ControlsFrame", "[shell][ref][nightly]")
{
    Rig rig;
    rig.set("drive", 1.0);                                  // in a car, in neutral: the application's own start
    rig.run(0.2);
    CHECK(rig.value(telemetry::kState) == 0.0);
    CHECK(rig.peak == 0.0);

    rig.frame.ignition = 2;                                 // the start key
    rig.run(0.05);
    CHECK(rig.value(telemetry::kState) == 1.0);             // cranking, on the starter
    rig.run(0.25);
    rig.frame.ignition = 1;
    rig.run(3.0);
    REQUIRE(rig.value(telemetry::kState) == 2.0);           // running
    const double idle = rig.rpm();
    CHECK(idle > 400.0);
    CHECK(idle < 1600.0);
    CHECK(rig.value(telemetry::kDriven) == 1.0);
    CHECK(rig.value(telemetry::kGear) == 0.0);
    CHECK(rig.value(telemetry::kRedline) * kRadToRpm > 3000.0);
    CHECK(rig.value(telemetry::kCoolant) > 250.0);          // kelvin
    CHECK(rig.value(telemetry::kCoolant) < 400.0);
    CHECK(rig.value(telemetry::kManifold) > 10000.0);       // Pa absolute: a vacuum at idle
    CHECK(rig.value(telemetry::kManifold) < 101325.0);
    CHECK(rig.value(telemetry::kOilPressure) > 0.0);
    CHECK(rig.peak > 0.001);

    // the raw fields, as the reference writes them, for its own panel
    for (const char* name : { "showLamps", "showRpm", "showRed", "showGear", "showKmh", "showThr", "showMap", "showPedals",
                              "showCycle", "showAir", "showPipes", "showBoost" })
    {
        INFO(name);
        CHECK(rig.tel().findValue(name) >= 0);
    }
    CHECK(rig.value("showRpm") == Approx(idle).epsilon(1e-6));
    CHECK((static_cast<long>(rig.value("showLamps")) & 3) == 2);
    CHECK(rig.tel().findValue("no such value") == -1);

    // the output's newest samples, mono, oldest first: the last block's are its end
    {
        rig.run(256.0 / 48000.0);                           // one whole block more: l and r hold all of it
        std::vector<float> tail(100000);
        REQUIRE(rig.tel().outputTail(tail.data(), 4096) == 4096);
        bool same = true;
        for (std::size_t i = 0; i < 256; ++i)
            same = same && tail[4096 - 256 + i] == 0.5f * (rig.l[i] + rig.r[i]);
        CHECK(same);
        CHECK(rig.tel().outputTail(tail.data(), 100000) == 8192);
        CHECK(rig.tel().outputTail(tail.data(), 0) == 0);
    }

    rig.frame.throttle = 1.0f;                              // the pedal
    rig.run(2.0);
    CHECK(rig.rpm() > 3.0 * idle);
    CHECK(rig.value(telemetry::kThrottle) > 0.8);
    rig.frame.throttle = 0.0f;
    rig.run(3.0);
    CHECK(rig.rpm() < 2.0 * idle);

    rig.frame.shiftUp = 1;                                  // a press of the shift: first gear
    rig.run(0.6);
    CHECK(rig.value(telemetry::kGear) == 1.0);
    rig.frame.shiftDown = 1;
    rig.run(0.6);
    CHECK(rig.value(telemetry::kGear) == 0.0);

    rig.frame.ignition = 2;                                 // held a second on a running engine: off
    rig.run(1.3);
    rig.frame.ignition = 0;
    rig.run(2.5);
    CHECK(rig.value(telemetry::kState) != 2.0);
    CHECK(rig.finite);
    CHECK(rig.peak <= 1.0);
    CHECK(rig.tel().outputPeak() <= 1.0f);
    CHECK_FALSE(rig.ref->stopped());
}

TEST_CASE("shell: played from the keys, the reference runs while the ignition is on and the throttle is by hand",
          "[shell][ref]")
{
    Rig rig;                                                // the reference's own default: DRIVE KEYS
    REQUIRE(rig.params().get(rig.params().findParam("drive")) == 0.0);
    rig.frame.ignition = 2;
    rig.run(0.3);
    rig.frame.ignition = 1;
    rig.run(3.0);
    REQUIRE(rig.value(telemetry::kState) == 2.0);
    CHECK(rig.value(telemetry::kDriven) == 0.0);
    const double idle = rig.rpm();
    rig.frame.throttle = 0.8f;
    rig.run(2.0);
    CHECK(rig.rpm() > 2.5 * idle);
}
