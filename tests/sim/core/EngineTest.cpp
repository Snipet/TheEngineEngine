// tests/sim/core/EngineTest.cpp: card D's tests in the whole simulator (docs/plan/w1-d-mechanics-layouts.md): 1 (custom
// off is the reference, besides tests/sim/NullTest.cpp), 6 (the engine dyno), 7 (the default V8's mean effective
// pressures and volumetric efficiency), 8 (a sixteen-cylinder build starts, runs and stops; its cost).
#include "Engine.h"
#include "tee/Program.h"
#include "Presets.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <utility>
#include <vector>
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

static void dynoHolds(const std::vector<const char*>& builds, const std::vector<double>& rpms)
{
    for (const char* build : builds)
    {
        Bench b(build);
        b.start();
        for (double rpm : rpms)
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

TEST_CASE("the engine dyno holds a set speed within 10 rpm at full throttle; its torque is the brake torque", "[core][engine][dyno]")
{
    dynoHolds({"v8"}, {2000.0, 5500.0});
}

TEST_CASE("the engine dyno holds a set speed on three builds at three speeds (the full form)", "[core][engine][dyno][nightly]")
{
    dynoHolds({"v8", "engines/ref-12-v8-cross", "engines/ref-10-inline-6"}, {2000.0, 4000.0, 5500.0});
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

static void meps(const std::vector<const char*>& builds, const std::vector<double>& rpms)
{
    for (const char* build : builds)
    {
        Bench b(build);
        b.start();
        b.warm();
        const double red = b.tel("engine.redline") * kRpm;
        std::printf("%s (redline %.0f rpm), warm, full throttle on the dyno:\n", build, red);
        for (double rpm : rpms)
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

TEST_CASE("the default V8 at full throttle: BMEP 10 to 13 bar in its mid-range, FMEP 0.8 to 2.5 bar, VE 0.75 to 1.05", "[core][engine][mep]")
{
    meps({"default"}, {1500.0, 3500.0, 6400.0});
}

TEST_CASE("the MEPs of two V8s across their range (the full form)", "[core][engine][mep][nightly]")
{
    meps({"default", "engines/ref-12-v8-cross"}, {800.0, 1500.0, 2500.0, 3500.0, 4500.0, 5500.0, 6400.0});
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

TEST_CASE("a package's model switch made while running waits for the fade, and is made", "[core][engine][models]")
{
    Bench b("default");
    b.start();
    std::string error;
    REQUIRE(b.player().setSpec("valve.model", 1, error));
    // the call the change reaches: held back (the Spec shows the model in use) and the fade asked for
    std::vector<float> l(1), r(1);
    b.player().process(l.data(), r.data(), 1);
    const double held = b.field("spec.valve.model");
    b.play(0.2);
    const double after = b.field("spec.valve.model");
    std::printf("valve.model 0 -> 1 while running: %g in the first call, %g 0.2 s later; state %g\n", held, after, b.tel("engine.state"));
    CHECK(held == 0.0);
    CHECK(after == 1.0);
    CHECK(b.tel("engine.state") == 2.0);
}

TEST_CASE("X-007: a diverging model never hangs the call or reads out of bounds; the engine is put at rest and says so", "[core][engine][guard]")
{
    // tee-fuzz's reduction: a custom single, 37 by 153 mm, an 18.4 kg piston, the new vehicle with an automatic, in neutral
    Bench b("default");
    std::string e;
    for (const auto& [path, value] : std::vector<std::pair<const char*, double>>{
             {"engine.custom", 1}, {"engine.layout", 0}, {"engine.bore", 0.0373}, {"engine.stroke", 0.1529},
             {"engine.pistonMass", 18.42}, {"engine.flywheel", 0.244}, {"car.model", 1}, {"car.gearbox", 3}, {"env.temperature", 300}})
        REQUIRE(b.player().setSpec(path, value, e));
    b.ctl().mode = 1;
    b.ctl().lever = 0;
    b.ctl().throttle = 0.3f;
    b.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
    b.play(3.0);
    b.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
    b.play(1.0);
    std::printf("X-007 after the start: state %g, %.0f rad/s, cylinders %g, refused %g\n", b.tel("engine.state"), b.tel("engine.speed"),
                b.tel("engine.cylinders"), b.tel("engine.layoutRefused"));
    b.ctl().throttle = 1.0f;
    int faults = 0;
    double top = 0;
    bool finite = true;
    const auto t0 = std::chrono::steady_clock::now();
    b.play(6.0, [&] {
        faults += (int(b.tel("engine.flags")) & 4) != 0 ? 1 : 0;
        const double w = b.tel("engine.speed");
        finite = finite && std::isfinite(w);
        top = std::max(top, w);
        for (float s : b.left())
            finite = finite && std::isfinite(s);
    });
    const double secs = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    std::printf("X-007: 6 s played in %.2f s of wall time; calls flagged a fault: %d; the fastest the crank was seen %.0f rad/s; all finite %d\n",
                secs, faults, top, finite ? 1 : 0);
    CHECK(secs < 30.0);
    CHECK(finite);
    CHECK(top < 20000.0);
    CHECK(std::isfinite(b.tel("engine.speed")));
}

TEST_CASE("the engine compiles and links at 48 kHz inside (TEE_ENGINE_RATE_48000, card C's prediction)", "[core][engine][rate]")
{
    tee::Program::Options options;
    options.defines["TEE_ENGINE_RATE_48000"] = "1";
    std::string errors;
    auto p = tee::Program::fromDirectory(std::filesystem::path(TEE_SOURCE_DIR) / "sim", "engine.fm", options, errors);
    INFO(errors);
    REQUIRE(p != nullptr);
    REQUIRE(p->link(errors));
    CHECK(p->function<void(void*, float*, float*, int)>("Engine::process", errors) != nullptr);
}

TEST_CASE("the derived telemetry stays finite and bounded through the limiter and a lift with throttles near vacuum",
          "[core][engine][telemetry]")
{
    // The whole-program tester saw veManifold -5.8 on this build after the limiter, on the lift (its throttles' manifold
    // fallen to or past 0 Pa); every derived figure is checked here through the same.
    Bench b("archetypes/straight-six-32-high-rev");
    b.start();
    b.warm();
    double veLo = 1e9, veHi = -1e9, veaLo = 1e9, veaHi = -1e9, pmLo = 1e12;
    bool finite = true;
    auto look = [&] {
        const double vm = b.tel("engine.veManifold"), va = b.tel("engine.veAir");
        veLo = std::min(veLo, vm);
        veHi = std::max(veHi, vm);
        veaLo = std::min(veaLo, va);
        veaHi = std::max(veaHi, va);
        pmLo = std::min(pmLo, b.tel("intake.manifold"));
        for (const char* k : {"engine.imep", "engine.pmep", "engine.fmep", "engine.bmep", "engine.power", "engine.torque"})
            finite = finite && std::isfinite(b.tel(k));
    };
    for (int i = 0; i < 3; ++i)
    {
        b.ctl().throttle = 1.0f;
        b.play(2.5, look);
        b.ctl().throttle = 0.0f;
        b.play(1.5, look);
    }
    std::printf("limiter and lift: veManifold %.2f to %.2f, veAir %.2f to %.2f, the manifold's least %.0f Pa\n", veLo, veHi,
                veaLo, veaHi, pmLo);
    CHECK(finite);
    CHECK(veLo >= 0.0);
    CHECK(veHi <= 4.0);
    CHECK(veaLo >= 0.0);
    CHECK(veaHi <= 4.0);
}

namespace
{
    // The books over `seconds` (g/s, from the engine's running totals): the air through the throttle, the net mass in
    // through the intake valves and through the exhaust valves, the unburned gas trapped at the intakes' closing
    void airBooks(Bench& b, double seconds, double& thr, double& fresh, double& valves, double& exhaust)
    {
        const double t0 = b.field("airThrottle"), v0 = b.field("airValves"), e0 = b.field("gasExhaust"), f0 = b.field("airTrapped");
        b.play(seconds);
        const double k = 1000.0 / seconds;
        thr = (b.field("airThrottle") - t0) * k;
        fresh = (b.field("airTrapped") - f0) * k;
        valves = (b.field("airValves") - v0) * k;
        exhaust = -(b.field("gasExhaust") - e0) * k;
    }
} // namespace

TEST_CASE("the physical cylinder conserves the gas its valves push back, and the throttle's air is the valves'",
          "[core][engine][backflow]")
{
    // N1's finding (24 Nov): the reference converts the gas its intake pushed back into the runner at the air's density
    // when it comes back, making mass each round trip (and the exhaust's at the header's). The physical cylinder
    // (fuel.model 1) brings each back as itself first (e8c6575); card E's intake (N1 acting) takes from its plenum the
    // valves' own mass. A big-overlap build, idling on the bench and at full throttle on the dyno, over 3 s: the air
    // through the throttle, the net mass in through the intake valves and out through the exhaust valves agree within
    // 1 %. (Before both: the valves passed 20 times the throttle's air at idle, 1.39 times at full throttle.) The
    // unburned gas trapped is printed (it is less than the valves' by what the overlap blows through, and counts what a
    // cylinder kept unburned from its last cycle).
    Bench b("archetypes/flat-plane-v8-45");
    std::string e;
    REQUIRE(b.player().setSpec("car.model", 0, e));
    REQUIRE(b.player().setSpec("fuel.model", 1, e));
    b.start();
    b.warm();
    b.ctl().throttle = 0.0f;
    b.play(3.0);
    double thrI = 0, frI = 0, vI = 0, eI = 0, thrW = 0, frW = 0, vW = 0, eW = 0;
    airBooks(b, 3.0, thrI, frI, vI, eI);
    const double idle = b.tel("engine.speed") * 9.5492966;
    b.ctl().mode = 2;
    b.ctl().throttle = 1.0f;
    b.ctl().dynoSpeed = float(3000.0 / 9.5492966);
    b.play(3.0);
    airBooks(b, 3.0, thrW, frW, vW, eW);
    std::printf("backflow: idle %.0f rpm: throttle %.2f, intake valves %.2f, exhaust valves %.2f, trapped unburned %.2f g/s; "
                "full throttle at 3000: %.1f, %.1f, %.1f, %.1f g/s\n", idle, thrI, vI, eI, frI, thrW, vW, eW, frW);
    CHECK(std::fabs(vI / thrI - 1.0) < 0.01);
    CHECK(std::fabs(vW / thrW - 1.0) < 0.01);
    CHECK(std::fabs(eI / vI - 1.0) < 0.01);
    CHECK(std::fabs(eW / vW - 1.0) < 0.01);
    CHECK(frW < vW);
}

TEST_CASE("a two-stroke's mechanics are its own cylinders', whatever the custom four-stroke's fields hold", "[core][engine][twostroke]")
{
    // (card T's hook, reviewed: with cycle 1 the moving masses, the shaking and the work integral took the custom
    // geometry's cylinders; a single with the custom fields of a twin had two pistons' inertia)
    Bench b("engines/two-stroke-kart");
    std::string e;
    REQUIRE(b.player().setSpec("engine.cylinders", 2, e));
    b.player().setSpec(std::size_t(b.player().specIndex("engine.pin")), 180.0, 1);
    b.start();
    b.play(0.5);
    CHECK(b.field("lay.n") == 1.0);
    CHECK(b.field("geo.n") == 1.0);
    CHECK(b.field("mm.n") == 1.0);
    CHECK(b.field("work.n") == 1.0);
    CHECK(std::fabs(b.field("work.vBmep") / (2.0 * b.field("two.vd")) - 1.0) < 1e-5);     // (2 pi T / V: T's catch)
}

TEST_CASE("boost.model 1 implies the builder's exhaust, at a build and when switched while running", "[core][engine][models]")
{
    // (the lead's ruling, 24 Nov: with boost.model 1 the cylinders exhaust into the builder's pipes, whose collectors end
    // in the turbines' housings; exhaust.model 0 with boost.model 1 cannot be selected)
    Bench b("archetypes/turbo-four-20-gdi");
    std::string e;
    REQUIRE(b.player().setSpec("exhaust.model", 0, e));
    REQUIRE(b.player().setSpec("boost.model", 1, e));
    b.start();
    b.play(0.5);
    CHECK(b.field("exh.model") == 1.0);
    CHECK(b.field("exh.fed") == 1.0);
    REQUIRE(b.player().setSpec("boost.model", 0, e));
    b.play(1.0);
    CHECK(b.field("exh.model") == 0.0);
    REQUIRE(b.player().setSpec("boost.model", 1, e));
    b.play(1.0);
    CHECK(b.field("exh.model") == 1.0);
    CHECK(std::isfinite(b.tel("engine.speed")));
}

static void booksOn(const std::vector<const char*>& builds)
{
    // (the lead's ask, 24 Nov: the backflow test ran green while every diesel archetype was dead on the dyno; each of
    // these must make torque at full load, and its throttle's (or intake's) air, its intake valves' and its exhaust
    // valves' mass agree within 1 %)
    for (const char* build : builds)
    {
        Bench b(build);
        std::string e;
        REQUIRE(b.player().setSpec("car.model", 0, e));
        b.ctl().ignition = 1;
        b.play(0.1);
        b.ctl().ignition = 2;
        b.play(2.0);
        b.ctl().ignition = 1;
        b.play(1.0);
        b.ctl().mode = 2;
        b.ctl().throttle = 1.0f;
        b.ctl().dynoSpeed = float(1800.0 / 9.5492966);
        b.play(4.0);
        double thr = 0, fr = 0, v = 0, x = 0;
        airBooks(b, 2.0, thr, fr, v, x);
        const double tq = b.tel("engine.torque");
        std::printf("%s at 1800 rpm, full load: %.0f N m; throttle %.2f, intake valves %.2f, exhaust valves %.2f g/s\n", build, tq,
                    thr, v, x);
        CHECK(tq > 50.0);
        CHECK(std::fabs(v / thr - 1.0) < 0.01);
        CHECK(std::fabs(x / v - 1.0) < 0.01);
    }
}

TEST_CASE("a refused custom layout says why (tel.engine.layoutReason)", "[core][engine][custom]")
{
    Bench b("default");
    std::string e;
    REQUIRE(b.player().setSpec("engine.custom", 1, e));
    REQUIRE(b.player().setSpec("engine.cylinders", 4, e));      // (every pin 0: all four fire together)
    b.start();
    CHECK(b.tel("engine.layoutRefused") == 1.0);
    CHECK(b.tel("engine.layoutReason") == 1.0);
}

TEST_CASE("the books hold, and the engine makes torque, on a diesel and a boosted petrol engine", "[core][engine][backflow]")
{
    booksOn({"archetypes/tractor-15-inline-six", "archetypes/turbo-four-20-gdi"});
}

TEST_CASE("the books hold, and the engine makes torque, on every diesel archetype (the full form)", "[core][engine][backflow][nightly]")
{
    booksOn({"archetypes/tractor-15-inline-six", "archetypes/diesel-20-common-rail", "archetypes/pickup-diesel-67-inline-six",
             "archetypes/turbo-four-20-gdi"});
}

TEST_CASE("a burn still running at exhaust opening lets its unburned fuel out with the gas: its heat released once", "[core][engine][pop]")
{
    // (the lead's ruling, 24 Nov, with N1: the cylinder's side of an overrun's pop. The default V8 under the physical
    // cylinder at full load with its burns made six times as long, so that each runs past the exhaust's opening: per
    // burn, the heat released in the cylinder and the heat let out unburned sum to the charge's within 1 %)
    Bench b("default");
    std::string e;
    REQUIRE(b.player().setSpec("fuel.model", 1, e));
    b.start();
    b.warm();
    b.ctl().mode = 2;
    b.ctl().throttle = 1.0f;
    b.ctl().dynoSpeed = float(3000.0 / 9.5492966);
    b.write("burnK", 6.0);          // (every burn six times as long: it runs well past the exhaust's opening)
    b.play(2.0);
    // (fuel.model 1 set before the start, on a build whose layout makes no configure: the decisions must still reach
    // Combustion and the cylinders)
    CHECK(b.field("comb.walls") == 1.0);
    CHECK(b.field("cyl[0].conserve") == 1.0);
    std::printf("spark %.1f degrees before the top, burn %.0f degrees, exhaust opens %.0f\n", b.tel("ecu.advance"), b.field("cyl[0].burnDur"),
                b.field("cyl[0].evo"));
    double worst = 0.0, outShare = 0.0;
    int n = 0;
    b.play(1.0, [&] {
        const double q = b.field("cyl[0].bookCharged");
        if (q <= 0.0)
            return;
        const double sum = b.field("cyl[0].bookReleased") + b.field("cyl[0].bookOut");
        worst = std::max(worst, std::fabs(sum / q - 1.0));
        outShare += b.field("cyl[0].bookOut") / q;
        n += 1;
    });
    std::printf("late burns: %d blocks read, released + let out over charged: worst %.4f off; let out unburned %.1f %% of the charge\n", n,
                worst, n > 0 ? 100.0 * outShare / n : 0.0);
    REQUIRE(n > 0);
    CHECK(worst < 0.01);
    CHECK(outShare / n > 0.001);
}
