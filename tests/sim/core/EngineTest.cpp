// tests/sim/core/EngineTest.cpp: card D's tests in the whole simulator (docs/plan/w1-d-mechanics-layouts.md): 1 (custom
// off is the reference, besides tests/sim/NullTest.cpp), 6 (the engine dyno), 7 (the default V8's mean effective
// pressures and volumetric efficiency), 8 (a sixteen-cylinder build starts, runs and stops; its cost).
#include "Engine.h"
#include "Presets.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

using tee::test::core::Bench;

namespace
{
    constexpr double kRpm = 9.5492966;

    struct Point
    {
        double rpm = 0, held = 0, bmep = 0, imep = 0, pmep = 0, fmep = 0, ve = 0, veMan = 0, dyno = 0, gasLessFriction = 0;
    };

    // The engine on the dyno at full throttle at a speed: 2 s to settle, 1 s measured
    Point dynoAt(Bench& b, double rpm, double throttle = 1.0)
    {
        b.ctl().mode = 2;
        b.ctl().throttle = float(throttle);
        b.ctl().dynoRate = 0.0f;
        b.ctl().dynoSpeed = float(rpm / kRpm);
        b.play(2.0);
        Point p;
        p.rpm = rpm;
        double n = 0;
        b.play(1.0, [&] {
            p.held += b.tel("engine.speed") * kRpm;
            p.bmep += b.tel("engine.bmep");
            p.imep += b.tel("engine.imep");
            p.pmep += b.tel("engine.pmep");
            p.fmep += b.tel("engine.fmep");
            p.ve += b.tel("engine.veAir");
            p.veMan += b.tel("engine.veManifold");
            p.dyno += b.tel("engine.dynoTorque");
            p.gasLessFriction += b.field("tqBrake");
            n += 1;
        });
        for (double* v : {&p.held, &p.bmep, &p.imep, &p.pmep, &p.fmep, &p.ve, &p.veMan, &p.dyno, &p.gasLessFriction})
            *v /= n;
        return p;
    }
} // namespace

TEST_CASE("custom off: a build's custom fields filled in change nothing, to the bit", "[core][engine][null]")
{
    // the V8 as it is, and the V8 with the V16 preset's geometry written into its fields but custom off
    auto render = [](bool filled) {
        Bench b("v8");
        if (filled)
        {
            const auto doc = tee::test::core::loadPreset("v16-45");
            const auto& e = doc["spec"]["engine"];
            std::string error;
            REQUIRE(b.player().setSpec("engine.cylinders", e["cylinders"].get<double>(), error));
            for (const char* name : {"bankAngle", "bank", "pin", "fireRev", "place"})
                for (std::size_t i = 0; i < e[name].size(); ++i)
                    b.player().setSpec(std::size_t(b.player().specIndex(std::string("engine.") + name)), e[name][i].get<double>(), i);
            REQUIRE(b.player().setSpec("engine.bore", 0.08, error));
            REQUIRE(b.player().setSpec("engine.flywheelInertia", 0.3, error));
        }
        std::vector<float> out;
        b.start();
        b.ctl().throttle = 1.0f;
        b.play(1.0, [&] { out.insert(out.end(), b.left().begin(), b.left().end()); });
        b.ctl().throttle = 0.0f;
        b.play(1.0, [&] { out.insert(out.end(), b.left().begin(), b.left().end()); });
        return out;
    };
    const auto plain = render(false);
    const auto filled = render(true);
    REQUIRE(plain.size() == filled.size());
    std::size_t differ = 0;
    float peak = 0;
    for (std::size_t i = 0; i < plain.size(); ++i)
    {
        differ += plain[i] != filled[i] ? 1 : 0;
        peak = std::max(peak, std::abs(plain[i]));
    }
    std::printf("custom off with the fields filled: %zu of %zu samples differ (peak %.3f)\n", differ, plain.size(), peak);
    CHECK(peak > 0.05f);
    CHECK(differ == 0);
}

TEST_CASE("the engine dyno holds a set speed within 10 rpm at full throttle; its torque is the brake torque", "[core][engine][dyno]")
{
    for (const char* build : {"v8", "engines/ref-12-v8-cross", "engines/ref-10-inline-6"})
    {
        Bench b(build);
        b.start();
        for (double rpm : {2000.0, 4000.0, 5500.0})
        {
            const Point p = dynoAt(b, rpm);
            std::printf("%-24s dyno %4.0f rpm full throttle: held %.2f rpm, absorber %.1f N m, gas less friction %.1f N m\n", build, rpm,
                        p.held, p.dyno, p.gasLessFriction);
            CHECK(std::abs(p.held - rpm) < 10.0);
            CHECK(std::abs(p.dyno - p.gasLessFriction) < 0.02 * std::abs(p.gasLessFriction) + 0.5);
            CHECK(std::abs(b.tel("engine.torque") - b.tel("engine.dynoTorque")) < 1e-3);
        }
    }
}

TEST_CASE("the engine dyno sweeps at its rate", "[core][engine][dyno]")
{
    Bench b("v8");
    b.start();
    dynoAt(b, 2000.0);
    // 300 rpm a second for 10 s (this build has the reference's biggest cam: its torque surges at low speed)
    b.ctl().dynoRate = float(300.0 / kRpm);
    double worst = 0, sum = 0, n = 0;
    double t = 0;
    b.play(10.0, [&] {
        t += double(Bench::kBlock) / Bench::kRate;
        const double err = b.tel("engine.speed") * kRpm - b.tel("engine.dynoSpeed") * kRpm;
        if (t > 1.0)
        {
            worst = std::max(worst, std::abs(err));
            sum += err;
            n += 1;
        }
    });
    const double end = b.tel("engine.dynoSpeed") * kRpm;
    std::printf("sweep at 300 rpm/s for 10 s from 2000: the set speed ends at %.1f rpm; the engine %+.1f rpm from it on average, "
                "%.1f at worst (the cam's surges)\n", end, sum / n, worst);
    CHECK(std::abs(end - 5000.0) < 15.0);
    CHECK(std::abs(sum / n) < 10.0);
    CHECK(worst < 75.0);
}

TEST_CASE("the default V8 at full throttle: BMEP 10 to 13 bar in its mid-range, FMEP 0.8 to 2.5 bar, VE 0.75 to 1.05", "[core][engine][mep]")
{
    for (const char* build : {"default", "engines/ref-12-v8-cross"})
    {
        Bench b(build);
        b.start();
        b.warm();
        const double red = b.tel("engine.redline") * kRpm;
        std::printf("%s (redline %.0f rpm), warm, full throttle on the dyno:\n", build, red);
        for (double rpm : {800.0, 1500.0, 2500.0, 3500.0, 4500.0, 5500.0, 6400.0})
        {
            b.warm();
            const Point p = dynoAt(b, rpm);
            std::printf("  %4.0f rpm: BMEP %5.2f, IMEP %5.2f, PMEP %4.2f, FMEP %4.2f bar; VE %.3f (air), %.3f (manifold)\n", rpm,
                        p.bmep / 1e5, p.imep / 1e5, p.pmep / 1e5, p.fmep / 1e5, p.ve, p.veMan);
            CHECK(p.fmep >= 0.8e5);
            CHECK(p.fmep <= 2.5e5);
            if (rpm >= 1500.0 && rpm <= 4500.0)
            {
                CHECK(p.bmep >= 10.0e5);
                CHECK(p.bmep <= 13.0e5);
                CHECK(p.ve >= 0.75);
                CHECK(p.ve <= 1.05);
            }
        }
    }
}

TEST_CASE("a sixteen-cylinder build starts, runs and stops; its cost", "[core][engine][v16]")
{
    Bench b("engines/v16-45");
    b.start();
    CHECK(b.tel("engine.state") == 2);
    CHECK(int(b.tel("engine.cylinders")) == 16);
    b.ctl().throttle = 1.0f;
    double top = 0;
    b.play(2.0, [&] { top = std::max(top, b.tel("engine.speed") * kRpm); });
    b.ctl().throttle = 0.0f;
    b.play(2.0);
    const double idle = b.tel("engine.speed") * kRpm;
    b.ctl().ignition = tee::EnginePlayer::kIgnitionOff;
    b.play(3.0);
    std::printf("V16: started, %.0f rpm at full throttle within 2 s, %.0f rpm idling, off: state %d; it took %.1f %% of a core at 48 kHz\n",
                top, idle, int(b.tel("engine.state")), 100.0 * b.load());
    CHECK(top > 4000.0);
    CHECK(idle > 400.0);
    CHECK(idle < 1500.0);
    CHECK(b.tel("engine.state") == 0);
    CHECK(b.tel("engine.speed") == 0.0);
}

TEST_CASE("custom on with the Spec's own geometry (every pin 0) refuses it and runs the layout dial's (X-001)", "[core][engine][custom]")
{
    Bench b("default");
    std::string error;
    REQUIRE(b.player().setSpec("engine.custom", 1, error));
    b.start();
    const double idle = b.tel("engine.speed") * kRpm;
    b.ctl().throttle = 1.0f;
    double top = 0;
    b.play(2.0, [&] { top = std::max(top, b.tel("engine.speed") * kRpm); });
    std::printf("custom on, the default geometry: refused %d, %d cylinders, idling at %.0f rpm, %.0f within 2 s at full throttle\n",
                int(b.tel("engine.layoutRefused")), int(b.tel("engine.cylinders")), idle, top);
    CHECK(b.tel("engine.layoutRefused") == 1.0);
    CHECK(b.tel("engine.cylinders") == 8.0);
    CHECK(idle > 500.0);
    CHECK(top > 4000.0);
    // and a geometry that runs is not refused
    Bench g("engines/ref-12-v8-cross");
    g.start();
    CHECK(g.tel("engine.layoutRefused") == 0.0);
    CHECK(g.tel("engine.speed") * kRpm > 500.0);
}
