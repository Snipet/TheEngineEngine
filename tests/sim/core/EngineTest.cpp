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
    // (and, card G's: what the exhaust's pipes were given over what its valves let out, a volume ratio: 1 when the pipes
    // are given the valves' mean flow)
    double pipesGiven = 0.0;

    void airBooks(Bench& b, double seconds, double& thr, double& fresh, double& valves, double& exhaust)
    {
        const double t0 = b.field("airThrottle"), v0 = b.field("airValves"), e0 = b.field("gasExhaust"), f0 = b.field("airTrapped");
        const double p0 = b.field("intk.mPlenum");
        const double g0 = b.field("exh.flowGiven"), l0 = b.field("exh.flowLet");
        // (what the cylinders hold at the window's ends: with a big overlap at idle it moves over a few seconds, and the
        // exhaust valves' out plus what the cylinders gained is what came in. The ports' stores swing within a cycle and
        // a sample at each end of the window is no measure of them: left out, the window long against a cycle)
        const int nc = int(b.tel("engine.cylinders"));
        auto held = [&](const char* f) {
            double h = 0.0;
            for (int c = 0; c < nc; ++c)
                h += b.field("cyl[" + std::to_string(c) + "]." + f);
            return h;
        };
        const double m0 = held("m");
        b.play(seconds);
        const double dm = held("m") - m0;
        pipesGiven = (b.field("exh.flowGiven") - g0) / std::max(b.field("exh.flowLet") - l0, 1e-12);
        const double k = 1000.0 / seconds;
        // (the throttle's air less what the plenum stored over the window: it is not quite steady on a boosted diesel;
        // N1's point)
        thr = (b.field("airThrottle") - t0 - (b.field("intk.mPlenum") - p0)) * k;
        fresh = (b.field("airTrapped") - f0) * k;
        valves = (b.field("airValves") - v0) * k;
        exhaust = -(b.field("gasExhaust") - e0) * k + dm * k;
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
    std::printf("  the exhaust's pipes given over the valves' let out at full throttle: %.4f\n", pipesGiven);
    CHECK(std::fabs(pipesGiven - 1.0) < 0.01);
    CHECK(std::fabs(vI / thrI - 1.0) < 0.01);
    CHECK(std::fabs(vW / thrW - 1.0) < 0.01);
    CHECK(std::fabs(eI / vI - 1.0) < 0.01);
    CHECK(std::fabs(eW / vW - 1.0) < 0.01);
    // (the unburned gas trapped at intake closing against the air the intake valves let in: less by what the overlap
    // blows through to the exhaust, more by what a cylinder kept unburned from its last burn; at full throttle the two
    // nearly cancel (measured 102.94 against 102.88 g/s on 24 Nov), so no direction holds, only that it is no more than
    // the valves' air within 1 %: no mass made in the cylinder)
    CHECK(frW < 1.01 * vW);
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

static void booksOn(const std::vector<const char*>& builds, double settle = 4.0)
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
        b.play(settle);         // (a boosted diesel's plenum settles slowly: the full form gives it 10 s)
        double thr = 0, fr = 0, v = 0, x = 0;
        airBooks(b, 2.0, thr, fr, v, x);
        const double tq = b.tel("engine.torque");
        std::printf("%s at 1800 rpm, full load: %.0f N m; throttle %.2f, intake valves %.2f, exhaust valves %.2f g/s\n", build, tq,
                    thr, v, x);
        std::printf("  the exhaust's pipes given over the valves' let out: %.4f\n", pipesGiven);
        CHECK(std::fabs(pipesGiven - 1.0) < 0.01);
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
             "archetypes/turbo-four-20-gdi"}, 10.0);
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

TEST_CASE("X-025: a two-stroke rotary is refused, says so, and runs as the rotary it is", "[core][engine][custom]")
{
    // (X's reduced build: a custom one-rotor rotary with engine.cycle 1, in the vehicle. No model defines a two-stroke
    // rotary: refused, tel.engine.layoutReason 4, the rotary run as a four-stroke-equivalent, never a silent engine
    // that reports itself running)
    Bench b("combo/findings/x025-two-stroke-rotary-silent");
    b.start();
    b.ctl().throttle = 0.3f;
    double peak = 0.0;
    b.play(2.0, [&] {
        for (float s : b.left())
            peak = std::max(peak, double(std::fabs(s)));
    });
    std::printf("X-025: state %g, %.0f rpm, refused %g, reason %g, cycleUse %g, faces %g, peak %.4f\n", b.tel("engine.state"),
                b.tel("engine.speed") * 9.549, b.tel("engine.layoutRefused"), b.tel("engine.layoutReason"), b.field("cycleUse"),
                b.field("lay.n"), peak);
    CHECK(b.tel("engine.layoutRefused") == 1.0);
    CHECK(b.tel("engine.layoutReason") == 4.0);
    CHECK(b.field("cycleUse") == 0.0);
    if (b.tel("engine.state") == 2.0)
        CHECK(peak > 1e-3);
}

TEST_CASE("a kept player reset is a fresh player, to the bit", "[core][engine][reset]")
{
    // (the cases share one compiled player, reset between them (Engine.h): the default V8's first second from a fresh
    // player and from the kept one reset after another build has run on it are the same samples)
    auto second = [](Bench& b) {
        std::vector<float> out;
        b.start(0.0);
        b.ctl().throttle = 0.6f;
        b.play(1.0, [&] { out.insert(out.end(), b.left().begin(), b.left().end()); });
        return out;
    };
    {
        Bench other("archetypes/turbo-four-20-gdi");
        other.start();
        other.play(1.0);
    }
    Bench kept("default");
    const std::vector<float> a = second(kept);
    Bench fresh("default", true);
    const std::vector<float> b = second(fresh);
    REQUIRE(a.size() == b.size());
    CHECK(a == b);
}

TEST_CASE("a model switch written while stopped makes its decisions with no configure between (f89d40c)", "[core][engine][models]")
{
    // (the sequence that showed the hole before f89d40c: the player made and configured with every switch off (its
    // init), the default build applied (same layout and size: no configure follows), the switch written while stopped,
    // the key turned. holdModel took the switch at once; comb.walls, Cylinder::conserve and Exhaust::implied stayed as
    // the configure had made them)
    struct Case { const char* field; const char* decision; };
    for (const Case& c : {Case{"fuel.model", "comb.walls"}, Case{"fuel.model", "cyl[0].conserve"}, Case{"engine.wallHeat", "comb.walls"},
                          Case{"boost.model", "exh.implied"}})
    {
        Bench b("default");
        std::string e;
        REQUIRE(b.player().setSpec(c.field, 1, e));
        b.start(0.5);
        INFO(c.field << " -> " << c.decision);
        CHECK(b.field(c.decision) == 1.0);
    }
}

TEST_CASE("the redline shown is a stated limiter's speed, not the idle's floor (ecu.model 1)", "[core][engine][redline]")
{
    // (D2's finding: the tractor's limiter at 2014 rpm showed 2200, idle 700 + 1500; a petrol engine with a low limiter
    // likewise)
    {
        Bench b("archetypes/tractor-15-inline-six");
        // (the build's own ecu.limiter block: 210.906 rad/s, 2014 rpm)
        b.start();
        std::printf("tractor: redline %.0f rpm (limiter 2014)\n", b.tel("engine.redline") * kRpm);
        CHECK(std::fabs(b.tel("engine.redline") * kRpm - 2014.0) < 5.0);
    }
    {
        Bench b("default");
        std::string e;
        REQUIRE(b.player().setSpec("ecu.model", 1, e));
        REQUIRE(b.player().setSpec("ecu.limiter.speed", 2000.0 / kRpm, e));
        b.start();
        std::printf("default V8, limiter at 2000: redline %.0f rpm\n", b.tel("engine.redline") * kRpm);
        CHECK(std::fabs(b.tel("engine.redline") * kRpm - 2000.0) < 5.0);
    }
}

TEST_CASE("the application's first build, written after a default init, runs as an init with it would (first.json)", "[core][engine][models]")
{
    // (the application's new first minute, B's: content/builds/first.json, every model on, written as Spec writes after
    // the program's init with the defaults, then the key. The same build with the program initialised over it
    // (EnginePlayer::reset keeping the Spec) is the reference: the decisions the switches make, and the torque on the
    // engine dyno at 3000 rpm full load, the same)
    double tq[3] = {0.0, 0.0, 0.0};
    for (int route = 0; route < 3; ++route)
    {
        Bench b("first");
        if (route == 1)
            b.player().reset(Bench::kRate, true);
        b.start();
        INFO("route " << route);
        CHECK(b.field("comb.walls") == 1.0);
        CHECK(b.field("cyl[0].conserve") == 1.0);
        CHECK(b.field("exh.implied") == 1.0);
        CHECK(b.field("exh.model") == 1.0);
        b.ctl().mode = 2;
        b.ctl().throttle = 1.0f;
        b.ctl().dynoSpeed = float(3000.0 / kRpm);
        b.play(3.0);
        double s = 0.0, n = 0.0;
        b.play(1.0, [&] { s += b.tel("engine.torque"); n += 1.0; });
        tq[route] = s / n;
    }
    std::printf("first.json at 3000 rpm full load: written after init %.1f N m, initialised over it %.1f N m, written after init again %.1f\n", tq[0], tq[1], tq[2]);
    // (the decisions are held exactly above; the torque within 2 %: each cylinder's small lasting difference in what it
    // burns (Cylinder::bias, some 1.5 %) is drawn from the noise stream at each configure, and the two routes configure
    // a different number of times, so they draw different ones: measured 474.7 against 470.6 N m, the same route twice
    // 474.7 to the bit; [.firstdiff] lists the fields that differ)
    CHECK(std::fabs(tq[0] / tq[1] - 1.0) < 0.02);
    CHECK(tq[0] == tq[2]);
}

TEST_CASE("a governed diesel's launch and its redline follow its limiter (ecu.model 1, card I's readers)", "[core][engine][redline]")
{
    // (e0bcf8f's two readers in the vehicle: the redline it is handed (Vehicle::redW) is the limiter's 2014 rpm, not the
    // idle's 2200 floor, and the launch's fallback (no launch control) is 60 % of it)
    Bench b("archetypes/tractor-15-inline-six");
    std::string e;
    REQUIRE(b.player().setSpec("ecu.launch.on", 0, e));
    b.start();
    b.ctl().mode = 1;
    b.play(1.0);
    // (the launch's fallback, Vehicle::wLaunch = 0.6 redW, is worked out only while a launch runs: redW is what it reads)
    const double red = b.field("car.m1.redW") * kRpm;
    std::printf("tractor driving: the vehicle's redline %.0f rpm\n", red);
    CHECK(std::fabs(red - 2014.0) < 5.0);
}

TEST_CASE("an engine set warm stays warm when the air moves while it stands; a cold one soaks to the air", "[core][engine][heat]")
{
    // (B's finding, the lead's ruling: the application writes a warm engine once at load, then the build's air arrives:
    // Heat::stood took the warm engine for one that had stood in that air. A cold engine still soaks: -20 C and a cold
    // start can be tried)
    for (int warm = 0; warm < 2; ++warm)
    {
        Bench b("first");
        b.play(0.1);
        if (warm != 0)
        {
            b.write("heat.tCool", 368.0); b.write("heat.tOil", 378.0); b.write("heat.tSurf", 455.0);
            b.write("heat.warm", 1.0); b.write("heat.warmS", 1.0);
        }
        std::string e;
        REQUIRE(b.player().setSpec("env.temperature", 253.15, e));
        b.play(0.5);
        // (a standing engine advances nothing, rendered or not (X-028): the air it stood in is taken at the start)
        b.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
        b.play(0.02);
        std::printf("%s, the air set to -20 C while it stands: oil %.1f K at the start\n", warm != 0 ? "set warm" : "cold", b.field("heat.tOil"));
        if (warm != 0)
            CHECK(b.field("heat.tOil") > 370.0);
        else
            CHECK(b.field("heat.tOil") < 260.0);
    }
}


TEST_CASE("the cylinders' books hold at idle under a big overlap, as a person can dial it", "[core][engine][backflow]")
{
    // (P's case: the flat-plane V8 at idle with its intake phased far ahead (its baked map at 50 degrees, aa190456),
    // where the exhaust valves' air read 3.8 % off the intake valves' over 3 s. Not a hole: the cylinders' own mass
    // moves over the window under such an overlap, and the books count it now. Here dialled as an intake centreline of
    // 60 degrees: the exhaust valves' out plus the cylinders' gain is the intake valves' in, within 0.5 %)
    Bench b("archetypes/flat-plane-v8-45");
    std::string e;
    REQUIRE(b.player().setSpec("car.model", 0, e));
    REQUIRE(b.player().setSpec("valve.inCentre", 60.0, e));
    b.start();
    b.warm();
    b.ctl().throttle = 0.0f;
    b.play(6.0);
    double thr = 0, fr = 0, v = 0, x = 0;
    airBooks(b, 3.0, thr, fr, v, x);
    std::printf("flat-plane at idle, intake centreline 60: %.0f rpm, throttle %.3f, intake valves %.3f, exhaust valves plus the cylinders' gain %.3f g/s\n",
                b.tel("engine.speed") * kRpm, thr, v, x);
    CHECK(std::fabs(x / v - 1.0) < 0.005);
}


TEST_CASE("an engine standing takes the build's air at once: nothing in its pipes glides to it", "[core][engine][heat]")
{
    // (M's finding: first.json's 288.15 K air was glided to from 300 K over 2 s while the engine stood, moving the
    // pipes' gas, heard from the key with the voices on. Since X-028 a standing engine advances nothing, its voices
    // rendered or not: with the key on it stands as it was, the same with voices on or off, and the start takes the
    // build's air at once)
    for (int warm = 0; warm < 2; ++warm)
    {
        double air[2] = {0.0, 0.0}, port[2] = {0.0, 0.0}, idle[2] = {0.0, 0.0};
        for (int vo = 0; vo < 2; ++vo)
        {
            Bench b("first");
            std::string e;
            REQUIRE(b.player().setSpec("voices.model", vo, e));
            if (warm != 0)
            {
                b.write("heat.tCool", 368.0); b.write("heat.tOil", 378.0); b.write("heat.tSurf", 455.0);
                b.write("heat.warm", 1.0); b.write("heat.warmS", 1.0);
            }
            b.ctl().ignition = 1;
            b.play(0.2);
            const double a0 = b.field("heat.tAmb"), p0 = b.field("exh.tPort"), i0 = b.field("ecu.idleRpm");
            b.play(2.0);
            CHECK(b.field("heat.tAmb") == a0);
            CHECK(b.field("exh.tPort") == p0);
            CHECK(b.field("ecu.idleRpm") == i0);
            air[vo] = a0;
            port[vo] = p0;
            idle[vo] = i0;
            b.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
            b.play(0.02);
            std::printf("%s, voices %d, key on, standing: air %.2f, port gas %.2f K, idle target %.0f rpm, unchanged over 2 s; "
                        "the air %.2f K at the start\n", warm ? "warm" : "cold", vo, a0, p0, i0, b.field("heat.tAmb"));
            CHECK(std::fabs(b.field("heat.tAmb") - 288.15) < 0.05);
        }
        CHECK(air[0] == air[1]);
        CHECK(port[0] == port[1]);
        CHECK(idle[0] == idle[1]);
    }
}

TEST_CASE("a key on with the engine standing is silent with every voice silenced, in any air, cold or set warm", "[core][engine][heat]")
{
    // (the lead's ruling after M's measure: a resting engine takes the build's air at once in every store that holds a
    // temperature or a pressure, and nothing rings at load. first.json, voices.model 1 with every voice at -60 dB, the
    // key on and no start, the first three seconds: each half second's rms under the floor. Since X-028 a standing
    // engine rendered for its voices advances nothing, so the boost machine's 300 K stores (N2's) no longer ring here:
    // measured 24 Nov, exact silence in every air, where it rang -54 dBFS at 288 K)
    const double floorDb = -90.0;
    for (int warm = 0; warm < 2; ++warm)
        for (double air : {253.15, 288.15, 300.0, 318.0})
        {
            Bench b("first");
            std::string e;
            REQUIRE(b.player().setSpec("env.temperature", air, e));
            REQUIRE(b.player().setSpec("voices.level", -60.0, e));
            if (warm != 0)
            {
                // (as the application's warm start: written once before the build's air reaches a configure)
                b.write("heat.tCool", 368.0); b.write("heat.tOil", 378.0); b.write("heat.tSurf", 455.0);
                b.write("heat.warm", 1.0); b.write("heat.warmS", 1.0);
            }
            b.ctl().ignition = 1;
            double worst = -400.0;
            std::string row;
            for (int h = 0; h < 6; ++h)
            {
                double sum = 0.0, n = 0.0;
                b.play(0.5, [&] {
                    for (float s : b.left())
                    {
                        sum += double(s) * s;
                        n += 1.0;
                    }
                });
                const double db = 10.0 * std::log10(std::max(sum / std::max(n, 1.0), 1e-40));
                worst = std::max(worst, db);
                row += " " + std::to_string(int(std::lround(db)));
            }
            std::printf("key on, standing, %s, air %.2f K: rms per half second (dBFS)%s\n", warm ? "set warm" : "cold", air, row.c_str());
            INFO(air << (warm ? " warm" : " cold"));
            CHECK(worst < floorDb);
        }
}
