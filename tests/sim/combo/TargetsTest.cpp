// tests/sim/combo/TargetsTest.cpp: package X's targeted combinations, where two packages meet on purpose (the lead's
// list, 14:50): the turbo with either ECU, anti-lag and nitrous switched while running; the ECU with the vehicle
// (launch control, a dog box's cut and blip, traction control on a wet road, every limiter kind against a wheel that
// spins up); full load on the engine dyno (MODE_DYNO_ENGINE: tel.engine.torque is brake torque); the idle mapped
// against its set speed, the cam and the air (a measurement, run by hand: "[idle-map]").
#include "Combo.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>

using namespace combo;

namespace
{
    constexpr double kRpm = 3.14159265358979 / 30.0;   // rad/s a rpm

    tee::test::Scenario started(const std::string& build, double air = 293.15)
    {
        tee::test::Scenario s(buildPath(build));
        s.at(0.0).spec("env.temperature", air).control("mode", 1).lever(0).clutch(0).throttle(0).ignition(tee::On);
        s.at(0.25).ignition(tee::Start);
        s.at(2.0).ignition(tee::On);
        return s;
    }

    Times runningTimes(double end)
    {
        Times t;
        t.idleFrom = 2.5;
        t.idleTo = 3.0;
        t.end = end;
        return t;
    }

    // The reference's own largest step through the same scenario with the targeted switches off (`make(false)`): the
    // click bound's baseline for a targeted run.
    template <class Make>
    double baseline(Make make, const Times& tm)
    {
        const tee::test::Run r = make(false).run(tm.end);
        const double from = tm.started + 0.5, to = tm.off > 0.0 ? tm.off : tm.end;
        return std::max(r.left().during(from, to).largestStep(), r.right().during(from, to).largestStep());
    }

    // For a cut that sends fuel to the pipe (a spark cut, a dog box's shift cut on the new ECU): the bang's bound is
    // the reference's own limiter with its pops at their default (0.3) through the same scenario, not more (the lead's
    // ruling with card H, 15:26). judge() bounds at twice its refStep, so this gives half of that step.
    template <class Make>
    double bangBaseline(Make make, const Times& tm)
    {
        tee::test::Scenario s = make(false);
        s.at(0.0).spec("ecu.pops", 0.3);
        const tee::test::Run r = s.run(tm.end);
        const double from = tm.started + 0.5, to = tm.off > 0.0 ? tm.off : tm.end;
        return 0.5 * std::max(r.left().during(from, to).largestStep(), r.right().during(from, to).largestStep());
    }

    // the share of [from, to] for which a telemetry bit is set
    double bitShare(const tee::test::Run& r, const char* field, int bit, double from, double to)
    {
        const auto s = r.tel(field).during(from, to);
        double on = 0.0, all = 0.0;
        for (std::size_t i = 1; i < s.size(); ++i)
        {
            const double dt = s.times()[i] - s.times()[i - 1];
            all += dt;
            if ((static_cast<int>(s.values()[i]) & bit) != 0)
                on += dt;
        }
        return all > 0.0 ? on / all : 0.0;
    }
} // namespace

TEST_CASE("targets: the new turbo with either ECU makes boost; anti-lag and nitrous switched while it pulls",
          "[combo][targets]")
{
    for (int ecu : {0, 1})
    {
        INFO("ecu.model " << ecu);
        Times tm = runningTimes(13.0);
        tm.driveFrom = 3.3;
        tm.driveTo = 11.0;
        tee::test::Scenario s = started("turbo-six");
        s.at(0.0).spec("boost.model", 1).spec("ecu.model", ecu).spec("ecu.nitrous.on", 1).spec("boost.nitrousShot", 50000);
        s.at(3.0).shiftUp();
        s.at(3.3).throttle(1.0).ramp(0.2);
        s.at(5.5).shiftUp();
        s.at(6.0).spec("ecu.antiLag.on", 1).spec("boost.antiLag", 1);
        s.at(7.0).buttons(4);   // BTN_NITROUS, in second
        s.at(8.0).shiftUp();
        s.at(8.5).spec("ecu.antiLag.on", 0).spec("boost.antiLag", 0);
        s.at(9.0).buttons(0);
        s.at(10.0).throttle(0.0).ramp(0.05);   // a lift with the turbo up: the blow-off
        s.at(10.6).throttle(1.0).ramp(0.1);
        s.at(11.0).throttle(0.0).ramp(0.05);
        const tee::test::Run r = s.run(tm.end);
        Verdict v = judge(r, tm, archetype("turbo-six"), true, referenceStep("turbo-six", Kind::Pull));
        const double boost = r.tel("boost.pressure").during(4.0, 10.0).max();
        std::printf("targets: turbo six, boost.model 1, ecu.model %d: boost up to %.0f kPa over the air\n", ecu,
                    boost / 1000.0);
        if (!(boost > 20000.0))
            v.failures.push_back({"boost", "the turbo made only " + std::to_string(boost) + " Pa at full throttle"});
        report(v, ecu ? "boost.model+ecu.model" : "boost.model", "turbo-six", "boost-extras");
    }
}

TEST_CASE("targets: launch control holds its speed on the new ECU and the new vehicle", "[combo][targets]")
{
    // a manual: the driver holds the clutch down and the brake, presses the launch button and floors it; at 6 s lets
    // the brake go and the clutch up over 0.3 s
    Times tm = runningTimes(10.0);
    tm.driveFrom = 6.0;
    tm.driveTo = 9.0;
    auto make = [](bool on) {
        tee::test::Scenario s = started("v8");
        s.at(0.0).spec("ecu.model", on ? 1 : 0).spec("car.model", on ? 1 : 0).spec("ecu.launch.on", 1);
        s.at(3.0).brake(1.0).clutch(1.0);
        s.at(3.1).shiftUp();
        s.at(3.2).buttons(2);   // BTN_LAUNCH held
        s.at(3.5).throttle(1.0).ramp(0.2);
        s.at(6.0).brake(0.0).buttons(0);
        s.at(6.0).clutch(0.0).ramp(0.3);
        s.at(8.0).clutch(1.0).shiftUp();
        s.at(8.2).clutch(0.0).ramp(0.2);
        return s;
    };
    const tee::test::Run r = make(true).run(tm.end);
    Verdict v = judge(r, tm, archetype("v8"), true, baseline(make, tm));
    const double held = r.tel("engine.speed").during(4.5, 5.9).mean();
    const double target = 418.879;   // ecu.launch.speed's default, 4000 rpm
    std::printf("targets: launch control held %.0f rpm (set %.0f)\n", held / kRpm, target / kRpm);
    if (std::fabs(held - target) > 0.15 * target)
        v.failures.push_back({"launch", "held " + std::to_string(held / kRpm) + " rpm against 4000 set"});
    report(v, "ecu.model+car.model", "v8", "launch-control");
}

TEST_CASE("targets: a dog box's upshifts cut and its downshifts blip on the new ECU", "[combo][targets]")
{
    Times tm = runningTimes(12.0);
    tm.driveFrom = 3.3;
    tm.driveTo = 9.0;
    auto make = [](bool on) {
        tee::test::Scenario s = started("v8");
        s.at(0.0).spec("ecu.model", on ? 1 : 0).spec("car.model", on ? 1 : 0).spec("car.gearbox", "Sequential");
        s.at(3.0).shiftUp();
        s.at(3.3).throttle(1.0).ramp(0.2);
        s.at(5.0).shiftUp();
        s.at(6.5).shiftUp();
        s.at(8.5).throttle(0.0).ramp(0.05);
        s.at(9.0).shiftDown();
        s.at(10.0).shiftDown();
        return s;
    };
    const tee::test::Run r = make(true).run(tm.end);
    Verdict v = judge(r, tm, archetype("v8"), true, bangBaseline(make, tm));
    const double cut = bitShare(r, "car.flags", 32, 4.9, 5.3) + bitShare(r, "car.flags", 32, 6.4, 6.8);
    const double blip = r.tel("engine.speed").during(9.0, 9.4).max() - r.tel("engine.speed").during(8.8, 9.0).min();
    std::printf("targets: dog box: the cut asked %.0f %% of the upshifts' windows; the downshift blipped %.0f rpm\n",
                50.0 * cut, blip / kRpm);
    if (!(cut > 0.0))
        v.failures.push_back({"cut", "no cut asked on a dog box's upshift"});
    if (!(blip > 200.0 * kRpm))
        v.failures.push_back({"blip", "the downshift blipped only " + std::to_string(blip / kRpm) + " rpm"});
    report(v, "ecu.model+car.model", "v8", "dog-box");
}

TEST_CASE("targets: traction control on a wet road spins the tyres less than without", "[combo][targets]")
{
    double spin[2] = {0.0, 0.0};
    for (int tc : {0, 1})
    {
        Times tm = runningTimes(9.0);
        tm.driveFrom = 3.3;
        tm.driveTo = 8.0;
        auto make = [tc](bool on) {
            tee::test::Scenario s = started("v8");
            s.at(0.0).spec("ecu.model", on ? 1 : 0).spec("car.model", on ? 1 : 0).spec("env.surface", 2);
            s.at(0.0).spec("ecu.traction.on", tc);
            s.at(3.0).shiftUp();
            s.at(3.3).throttle(1.0).ramp(0.1);
            s.at(6.0).shiftUp();
            return s;
        };
        const tee::test::Run r = make(true).run(tm.end);
        Verdict v = judge(r, tm, archetype("v8"), true, baseline(make, tm));
        spin[tc] = bitShare(r, "car.flags", 1, 3.3, 8.0);
        report(v, "ecu.model+car.model", "v8", tc ? "wet-traction-on" : "wet-traction-off");
    }
    std::printf("targets: on a wet road the tyres spin %.0f %% of the pull without traction control, %.0f %% with\n",
                100.0 * spin[0], 100.0 * spin[1]);
    CHECK(spin[1] < spin[0]);
}

TEST_CASE("targets: every limiter kind against a wheel that spins up in first on a wet road", "[combo][targets]")
{
    for (int kind = 0; kind <= 3; ++kind)
    {
        Times tm = runningTimes(8.0);
        tm.driveFrom = 3.3;
        tm.driveTo = 7.5;
        auto make = [kind](bool on) {
            tee::test::Scenario s = started("v8");
            s.at(0.0).spec("ecu.model", on ? 1 : 0).spec("car.model", on ? 1 : 0).spec("env.surface", 2);
            s.at(0.0).spec("ecu.limiter.kind", kind);
            s.at(3.0).shiftUp();
            s.at(3.3).throttle(1.0).ramp(0.05);
            return s;
        };
        const tee::test::Run r = make(true).run(tm.end);
        // a spark cut (kind 1) bangs: the bang's bound; the others cut fuel: twice the reference without pops
        Verdict v = judge(r, tm, archetype("v8"), true, kind == 1 ? bangBaseline(make, tm) : baseline(make, tm));
        const double top = r.tel("engine.speed").during(3.3, 7.5).max();
        const double red = r.tel("engine.redline").last();
        std::printf("targets: limiter kind %d: the wheel spun the engine up to %.0f rpm (redline %.0f)\n", kind,
                    top / kRpm, red / kRpm);
        if (top > 1.08 * red)
            v.failures.push_back({"limiter", "kind " + std::to_string(kind) + " let it reach " +
                                                 std::to_string(top / kRpm) + " rpm, 8 % past the redline"});
        report(v, "ecu.model+car.model", "v8", "limiter-" + std::to_string(kind));
    }
}

TEST_CASE("targets: full throttle on the engine dyno makes torque with every switch on", "[combo][targets]")
{
    for (const Archetype& a : archetypes())
    {
        const double dynoRpm = a.diesel ? 1500.0 : 3000.0;   // the diesel governs near 2600
        double torque[2] = {0.0, 0.0};
        for (int all : {0, 1})
        {
            Times tm;
            tm.idleFrom = -1.0;
            tm.end = 9.0;
            tee::test::Scenario s(buildPath(a.name));
            s.at(0.0).spec("env.temperature", 293.15).control("mode", 1).lever(0).ignition(tee::On);
            if (all)
                for (const Switch& w : switches())
                    s.spec(w.path, w.on);
            s.at(0.25).ignition(tee::Start);
            s.at(2.0).ignition(tee::On);
            s.at(3.0).mode(2).dyno(dynoRpm * kRpm);   // MODE_DYNO_ENGINE holding it
            s.at(3.5).throttle(1.0).ramp(0.3);
            const tee::test::Run r = s.run(tm.end);
            Verdict v = judge(r, tm, &a, false, referenceStep(a.name, Kind::Tour));
            torque[all] = r.tel("engine.torque").during(6.0, 9.0).mean();
            const double held = r.tel("engine.speed").during(6.0, 9.0).mean();
            if (!(torque[all] > 0.0))
                v.failures.push_back({"torque", "brake torque " + std::to_string(torque[all]) + " N m at full throttle"});
            if (std::fabs(held - dynoRpm * kRpm) > 100.0 * kRpm)
                v.failures.push_back({"dyno", "the dyno held " + std::to_string(held / kRpm) + " rpm"});
            report(v, all ? "all" : "none", a.name, "dyno");
        }
        std::printf("targets: %s on the dyno at %.0f rpm, full throttle: %.0f N m with every switch off, %.0f N m on\n",
                    a.name, dynoRpm, torque[0], torque[1]);
    }
}

TEST_CASE("measure: the idle against its set speed, the cam and the air, under both ECUs", "[.][measure][idle-map]")
{
    std::printf("idle map (V8, neutral, 6 to 10 s after the key): ecu, set rpm, cam, air C: mean, min, max rpm\n");
    for (int ecu : {0, 1})
        for (double set : {0.0, 600.0, 800.0, 1000.0})
            for (double cam : {0.3, 1.0})
                for (double air : {253.15, 293.15})
                {
                    tee::test::Scenario s = started("v8", air);
                    s.at(0.0).spec("ecu.model", ecu).spec("ecu.idle.speed", set * kRpm).spec("valve.cam", cam);
                    const tee::test::Run r = s.run(10.0);
                    const auto w = r.tel("engine.speed").during(6.0, 10.0);
                    const bool stalled = r.tel("engine.state").during(2.0, 10.0).min() != 2.0;
                    std::printf("idle map | ecu %d | set %4.0f | cam %.1f | air %+4.0f C | mean %5.0f | min %5.0f | max "
                                "%5.0f | %s\n",
                                ecu, set, cam, air - 273.15, w.mean() / kRpm, w.min() / kRpm, w.max() / kRpm,
                                stalled ? "STALLED" : "runs");
                    std::fflush(stdout);
                }
}

TEST_CASE("fixed X-001: the custom engine over the default build, its geometry at the Spec's defaults, starts and runs",
          "[combo][fixed]")
{
    // found 2026-11-24 (every pin at 0: the eight compressed together and the starter stalled); fixed by D in 3d4b400:
    // such a geometry is refused and the layout dial's firing runs, tel.engine.layoutRefused 1
    Times tm;
    tee::test::Scenario s = tee::test::Scenario().at(0.0).spec("engine.custom", 1);
    s.at(0.0).spec("env.temperature", 293.15).control("mode", 1).lever(0).clutch(0).ignition(tee::On);
    s.at(tm.start).ignition(tee::Start);
    s.at(tm.started).ignition(tee::On);
    s.at(5.0).throttle(1.0).ramp(0.15);
    s.at(8.5).throttle(0.0).ramp(0.05);
    tm.revFrom = 6.0;
    tm.revTo = 8.5;
    tm.end = 10.0;
    const tee::test::Run r = s.run(tm.end);
    const Verdict v = judge(r, tm, archetype("v8"), true, referenceStep("v8", Kind::Tour));
    report(v, "engine.custom", "defaults", "X-001");
    CHECK(r.tel("engine.layoutRefused").last() == 1.0);
}

TEST_CASE("fixed X-003: the new combustion and ECU together start every archetype at -20 C", "[combo][fixed]")
{
    // found 2026-11-24 (no start in 10 s at 253 K with fuel.model and ecu.model on); fixed by H in b09a0a6
    std::uint32_t mask = 0;
    const auto& sw = switches();
    for (std::size_t i = 0; i < sw.size(); ++i)
        if (sw[i].path == "fuel.model" || sw[i].path == "ecu.model")
            mask |= 1u << i;
    REQUIRE(mask != 0);
    for (const char* b : {"v8", "small-four", "turbo-six", "turbo-v8-dual"})
    {
        Times tm;
        const tee::test::Run r = scenario(Kind::Cold, b, mask, tm).run(tm.end);
        const Verdict v = judge(r, tm, archetype(b), true, referenceStep(b, Kind::Cold));
        report(v, comboName(mask), b, "X-003");
    }
}

TEST_CASE("fixed X-007: the reduced Spec (an 18 kg piston on a single with the automatic) runs", "[combo][fixed]")
{
    // found 2026-11-24 (the crank ran away; then Engine::process never returned, or SIGSEGV); D's ae5c99b made the
    // audio thread safe (no state-counted loop, a diverged engine put at rest with FLAG_FAULT), I's f9e26f1 removed the
    // cause (Car::step with the crank's real inertia; the converter's torque capped)
    const auto file = tee::test::contentDirectory() / "builds" / "combo" / "findings" / "x007-heavy-piston.json";
    Times tm;
    tee::test::Scenario s = scenario(Kind::Tour, file.string(), 0u, tm);
    const tee::test::Run r = s.run(tm.end);
    const Verdict v = judge(r, tm, nullptr, false, referenceStep("v8", Kind::Tour));
    report(v, "as reduced", "x007-heavy-piston", "X-007");
    CHECK(r.tel("engine.state").during(3.0, 10.5).min() == 2.0);   // it runs throughout
}
