// tests/sim/core/EngineExplore.cpp: the engine on the dyno, printed (hidden: "[.dynosweep]").
#include "Engine.h"
#include "Presets.h"
#include "FieldDiff.h"
#include "tee/Warm.h"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>
#include <set>

using tee::test::core::Bench;

TEST_CASE("dyno sweep printed", "[.dynosweep]")
{
    for (const char* build : {"default", "engines/ref-12-v8-cross"})
    {
        Bench b(build);
        b.start();
        b.ctl().mode = 2;
        b.ctl().throttle = 1.0f;
        std::printf("%s\n", build);
        for (double rpm : {1500.0, 2500.0, 3500.0, 4500.0, 5500.0, 6200.0})
        {
            b.ctl().dynoSpeed = float(rpm / 9.5492966);
            double sp = 0, n = 0, bm = 0, im = 0, pm = 0, fm = 0, ve = 0, vm = 0, tq = 0, tb = 0;
            b.play(2.0);
            b.play(1.0, [&] {
                sp += b.tel("engine.speed"); bm += b.tel("engine.bmep"); im += b.tel("engine.imep"); pm += b.tel("engine.pmep");
                fm += b.tel("engine.fmep"); ve += b.tel("engine.veAir"); vm += b.tel("engine.veManifold");
                tq += b.tel("engine.dynoTorque"); tb += b.field("tqBrake"); n += 1; });
            std::printf("  %5.0f rpm: held %7.1f, BMEP %5.2f IMEP %5.2f PMEP %5.2f FMEP %4.2f bar, VE air %.3f man %.3f, dyno %6.1f N m, gas-fric %6.1f, peak %5.1f bar, redline %.0f\n",
                        rpm, sp / n * 9.5492966, bm / n / 1e5, im / n / 1e5, pm / n / 1e5, fm / n / 1e5, ve / n, vm / n, tq / n, tb / n,
                        b.tel("engine.peakPressure") / 1e5, b.tel("engine.redline") * 9.5492966);
        }
    }
}

TEST_CASE("dyno trace", "[.dynotrace]")
{
    Bench b("v8");
    b.start();
    b.ctl().mode = 2;
    b.ctl().throttle = 1.0f;
    b.ctl().dynoSpeed = float(2000.0 / 9.5492966);
    double t = 0;
    int k = 0;
    b.play(4.0, [&] {
        t += 512.0 / 48000.0;
        if (++k % 8 == 0)
            std::printf("%5.2f s: %7.1f rpm, absorber %7.1f, dynoOn %g, state %g, integ %g, knock %.1f, flags %g, throttle %.2f\n", t,
                        b.tel("engine.speed") * 9.5492966, b.field("tqAbs"), b.field("dynoOn"), b.tel("engine.state"), b.field("dyno.integ"),
                        b.tel("ecu.knockRetard"), b.tel("engine.flags"), b.tel("engine.throttle"));
    });
}

TEST_CASE("cost of builds", "[.cost]")
{
    for (const char* build : {"default", "engines/ref-12-v8-cross", "engines/v16-45"})
    {
        double best = 1e9;
        for (int r = 0; r < 3; ++r)
        {
            Bench b(build);
            b.start();
            b.ctl().throttle = 0.6f;
            b.play(6.0);
            best = std::min(best, b.load());
        }
        std::printf("%-24s %.1f %% of a core (48 kHz, 512 blocks; start, then 6 s at 60 %% throttle)\n", build, 100.0 * best);
    }
}

TEST_CASE("superbike split", "[.superbike]")
{
    for (int custom : {0, 1})
    {
        Bench b("default");
        std::string e;
        auto set = [&](const char* p, double v) { REQUIRE(b.player().setSpec(p, v, e)); };
        set("env.temperature", 300); set("engine.layout", 7); set("engine.displacement", 0.000998); set("engine.boreStroke", 1.38);
        set("engine.compression", 13); set("valve.model", 1); set("valve.valves", 4); set("valve.inDurSeat", 300); set("valve.inDur1mm", 270);
        set("valve.inLift", 0.0095); set("valve.inCentre", 105); set("valve.exDurSeat", 292); set("valve.exDur1mm", 262); set("valve.exLift", 0.0085);
        set("valve.exCentre", 108); set("valve.train", 3); set("intake.model", 1); set("intake.throttleKind", 1); set("intake.runnerLength", 0.12);
        if (custom)
        {
            const auto doc = tee::test::core::loadPreset("ref-07-xplane-i4");
            const auto& en = doc["spec"]["engine"];
            set("engine.custom", 1); set("engine.cylinders", 4); set("engine.bore", 0.076); set("engine.stroke", 0.055); set("engine.rodLength", 0.105);
            set("engine.pistonMass", 0.30); set("engine.rodMass", 0.32); set("engine.frictionValvetrain", 2); set("engine.alternator", 150);
            set("engine.redlineSpeed", 14200 / 9.5492966);
            for (const char* name : {"bankAngle", "bank", "pin", "fireRev", "place"})
                for (std::size_t i = 0; i < en[name].size(); ++i)
                    b.player().setSpec(std::size_t(b.player().specIndex(std::string("engine.") + name)), en[name][i].get<double>(), i);
        }
        b.start();
        b.warm();
        std::printf("superbike, custom %d (redline %.0f):\n", custom, b.tel("engine.redline") * 9.5492966);
        for (double rpm : {6000.0, 10000.0, 13500.0})
        {
            b.warm();
            b.ctl().mode = 2; b.ctl().throttle = 1.0f; b.ctl().dynoSpeed = float(rpm / 9.5492966);
            b.play(2.5);
            double n = 0, im = 0, pm = 0, fm = 0, bm = 0, ve = 0, tq = 0, held = 0;
            b.play(1.0, [&] { im += b.tel("engine.imep"); pm += b.tel("engine.pmep"); fm += b.tel("engine.fmep"); bm += b.tel("engine.bmep");
                              ve += b.tel("engine.veAir"); tq += b.tel("engine.torque"); held += b.tel("engine.speed") * 9.5492966; n += 1; });
            const double vd = b.field("cyl[0].vd"), q = b.field("cyl[0].qTot"), spark = b.field("cyl[0].sparkAt"), dur = b.field("cyl[0].burnDur");
            const double x50 = spark + 0.5177 * dur - 720.0;       // 1 - exp(-5 f^3) = 0.5 at f = 0.5177
            std::printf("  %5.0f rpm (held %.0f): IMEP %5.2f, PMEP %5.2f, FMEP %4.2f, BMEP %5.2f bar; VE %.3f; %5.1f N m, %5.1f kW; "
                        "indicated efficiency %.2f (gross work %.0f J over the burn's %.0f J); spark %.1f BTDC, burn %.1f deg, half burned at %.1f ATDC; piston %.1f m/s\n",
                        rpm, held / n, im / n / 1e5, pm / n / 1e5, fm / n / 1e5, bm / n / 1e5, ve / n, tq / n, tq / n * rpm / 9549.3,
                        (im / n) * vd / q, (im / n) * vd, q, 720.0 - spark, dur, x50, b.tel("engine.pistonSpeed"));
        }
    }
}

TEST_CASE("rotary torque by model", "[.rotary]")
{
    struct Case { const char* name; std::vector<std::pair<const char*, double>> over; };
    const std::vector<Case> cases = {
        {"archetype, every model", {}},
        {"custom off", {{"engine.custom", 0}}},
        {"boost.model 0", {{"boost.model", 0}}},
        {"fuel.model 0", {{"fuel.model", 0}}},
        {"ecu.model 0", {{"ecu.model", 0}}},
        {"valve.model 0, intake.model 0", {{"valve.model", 0}, {"intake.model", 0}}},
        {"exhaust.model 0", {{"exhaust.model", 0}}},
        {"every model 0", {{"boost.model", 0}, {"fuel.model", 0}, {"ecu.model", 0}, {"valve.model", 0}, {"intake.model", 0}, {"exhaust.model", 0}}},
        {"every model 0, custom off", {{"engine.custom", 0}, {"boost.model", 0}, {"fuel.model", 0}, {"ecu.model", 0}, {"valve.model", 0}, {"intake.model", 0}, {"exhaust.model", 0}}},
    };
    for (const auto& c : cases)
    {
        Bench b("archetypes/rotary-twin-13");
        std::string e;
        REQUIRE(b.player().setSpec("car.model", 0, e));
        for (const auto& [p, v] : c.over)
            REQUIRE(b.player().setSpec(p, v, e));
        b.start();
        b.warm();
        std::printf("%-34s", c.name);
        for (double rpm : {4000.0, 6500.0})
        {
            b.warm();
            b.ctl().mode = 2; b.ctl().throttle = 1.0f; b.ctl().dynoSpeed = float(rpm / 9.5492966);
            b.play(4.0);
            double tq = 0, n = 0, bst = 0, imep = 0;
            b.play(1.0, [&] { tq += b.tel("engine.torque"); bst += b.tel("boost.pressure"); imep += b.tel("engine.imep"); n += 1; });
            std::printf("  %4.0f rpm: %5.0f N m %4.0f kW, boost %3.0f kPa, IMEP %4.1f bar;", rpm, tq / n, tq / n * rpm / 9549.3, bst / n / 1000, imep / n / 1e5);
        }
        std::printf("  faces %g, cycle %g\n", b.field("lay.n"), b.field("lay.cyc"));
    }
}

// The rotary under the physical models (fuel.model 1): BMEP and port gas against speed, unboosted (the targets: some
// 10 to 10.5 bar, a 1.3 L twin-rotor road engine's 210 to 216 N m) and at its own boost (some 14 bar at 0.7 to 0.8 bar).
TEST_CASE("rotary BMEP unboosted and boosted", "[.rotarybmep]")
{
    for (int boosted = 0; boosted < 2; ++boosted)
    {
        // (unboosted: card P's naturally aspirated side-port twin; boosted: the turbo archetype at its own boost)
        Bench b(boosted != 0 ? "archetypes/rotary-twin-13" : "archetypes/rotary-twin-na-side-port");
        std::string e;
        REQUIRE(b.player().setSpec("car.model", 0, e));
        b.start();
        b.warm();
        std::printf("%s:\n", boosted != 0 ? "boosted" : "unboosted");
        for (double rpm : {3000.0, 4000.0, 5000.0, 6000.0, 7000.0})
        {
            b.ctl().mode = 2; b.ctl().throttle = 1.0f; b.ctl().dynoSpeed = float(rpm / 9.5492966);
            b.play(3.0);
            double tq = 0, n = 0, bst = 0, imep = 0, fmep = 0, pmep = 0, bmep = 0, tp = 0, ve = 0;
            b.play(1.0, [&] {
                tq += b.tel("engine.torque"); bst += b.tel("boost.pressure"); imep += b.tel("engine.imep");
                fmep += b.tel("engine.fmep"); pmep += b.tel("engine.pmep"); bmep += b.tel("engine.bmep");
                tp += b.field("exh.tPort"); ve += b.tel("engine.veAir"); n += 1;
            });
            std::printf("  %4.0f rpm: %4.0f N m %4.0f kW, BMEP %5.2f IMEP %5.2f PMEP %5.2f FMEP %5.2f bar, boost %4.0f kPa, VE %4.2f, port %5.0f K, leaked %4.1f %% of the charge\n",
                        rpm, tq / n, tq / n * rpm / 9549.3, bmep / n / 1e5, imep / n / 1e5, pmep / n / 1e5, fmep / n / 1e5,
                        bst / n / 1000, ve / n, tp / n, 100.0 * b.field("cyl[0].mLeakLast") / std::max(b.field("work.w[0].trapped"), 1e-9));
        }
    }
}

// P's teeth (24 Nov): the V12 held at full load at 6500, 6673 and 6850 rpm; torque, VE, and cylinder 1's intake port
// pressure over its cycle from the Scope (every 30 degrees), and its trapped mass.
TEST_CASE("v12 teeth", "[.teeth]")
{
    for (int phys = 1; phys >= 0; --phys)
    for (double rpm : {6500.0, 6673.0, 6850.0})
    {
        Bench b("archetypes/v12-65");
        std::string e;
        REQUIRE(b.player().setSpec("car.model", 0, e));
        if (phys == 0)
            REQUIRE(b.player().setSpec("fuel.model", 0, e));
        std::printf("fuel.model %d ", phys);
        b.start();
        b.warm();
        b.ctl().mode = 2;
        b.ctl().throttle = 1.0f;
        b.ctl().scopeOn = 1;
        b.ctl().dynoSpeed = float(rpm / 9.5492966);
        b.play(4.0);
        double tq = 0, ve = 0, n = 0, tqLo = 1e9, tqHi = -1e9;
        b.play(1.0, [&] { const double t = b.tel("engine.torque"); tq += t; ve += b.tel("engine.veAir"); n += 1; tqLo = std::min(tqLo, t); tqHi = std::max(tqHi, t); });
        std::printf("%5.0f rpm: %.0f N m (%.0f to %.0f over the second), VE %.3f, trapped %.3f g, IVC at %.0f; port kPa:", rpm, tq / n, tqLo, tqHi,
                    ve / n, 1000.0 * b.field("work.w[0].trapped"), b.field("cyl[0].ivo") + b.field("cyl[0].idur"));
        for (int d = 0; d < 720; d += 30)
            std::printf(" %.0f", b.field("scope.portIntake[" + std::to_string(d) + "]") / 1000.0);
        std::printf("\n");
    }
}

// Which of Engine's fields differ between first.json written after a default init and first.json with the program
// initialised over it, after 0.2 s at rest (the key off)
TEST_CASE("first route diff", "[.firstdiff]")
{
    Bench a("first");
    a.play(0.2);
    Bench b("first", true);
    b.player().reset(Bench::kRate, true);
    b.play(0.2);
    const unsigned char* da = static_cast<const unsigned char*>(a.player().instance().data());
    const unsigned char* db = static_cast<const unsigned char*>(b.player().instance().data());
    int shown = 0;
    a.player().schema().forEach([&](const std::string& path, const tee::Schema::Node& node, std::size_t offset) {
        if (shown < 80 && std::memcmp(da + offset, db + offset, node.size) != 0)
        {
            double va = 0, vb = 0;
            std::string err;
            a.player().read(path, va, err);
            b.player().read(path, vb, err);
            std::printf("  %s: %g / %g\n", path.c_str(), va, vb);
            shown += 1;
        }
    });
}

// Card I's: the V12 took more than 1.5 s of cranking to start in 318 K air. The start's course by air temperature, held
// warm (a hot restart) and cold-soaked
TEST_CASE("v12 hot start", "[.v12start]")
{
    for (int warm = 0; warm < 2; ++warm)
        for (double air : {288.0, 300.0, 318.0})
        {
            Bench b("archetypes/v12-65");
            std::string e;
            b.ctl().mode = 1;     // (driven: the car attached, in neutral)
            REQUIRE(b.player().setSpec("env.temperature", air, e));
            b.play(0.1);
            auto hold = [&] { if (warm) { b.write("heat.tCool", 368.0); b.write("heat.tOil", 378.0); b.write("heat.tSurf", 455.0); b.write("heat.warm", 1.0); b.write("heat.warmS", 1.0); } };
            hold();
            b.ctl().ignition = 1;
            b.play(0.1, hold);
            b.ctl().ignition = 2;
            double t = 0, caught = -1, firstFire = -1;
            b.play(3.0, [&] {
                hold();
                t += 512.0 / 48000.0;
                if (firstFire < 0 && b.field("crank.tqMean") > 0.0) firstFire = t;
                if (caught < 0 && b.tel("engine.state") == 2.0) caught = t;
            });
            std::printf("V12, %s, air %.0f K: first positive gas torque at %.2f s, running at %.2f s; cranking speed then %.0f rpm, idle valve %.2f, lambda %.2f\n",
                        warm ? "warm" : "soaked", air, firstFire, caught, b.tel("engine.speed") * 9.549, b.field("ecu.bypass"), b.tel("ecu.lambda"));
        }
}

// Which temperature-like fields move while an engine stands with the key on (first.json at 288.15 K, cold): a snapshot
// at 0.05 s against 1.0 s
TEST_CASE("standing drift", "[.standdrift]")
{
    Bench b("first");
    std::string e;
    REQUIRE(b.player().setSpec("env.temperature", 288.15, e));
    REQUIRE(b.player().setSpec("voices.level", -60.0, e));
    b.ctl().ignition = 1;
    b.play(0.05);
    std::vector<std::pair<std::string, double>> before;
    b.player().schema().forEach([&](const std::string& path, const tee::Schema::Node& node, std::size_t) {
        if (node.extents.empty() && path.rfind("spec.", 0) != 0 && path.rfind("tel.", 0) != 0)
        {
            double v = 0;
            std::string err;
            if (b.player().read(path, v, err))
                before.push_back({path, v});
        }
    });
    b.play(0.95);
    int shown = 0;
    for (const auto& [path, v0] : before)
    {
        double v1 = 0;
        std::string err;
        b.player().read(path, v1, err);
        const bool tempLike = (v0 > 200.0 && v0 < 2000.0) || (v1 > 200.0 && v1 < 2000.0);
        if (tempLike && std::fabs(v1 - v0) > 0.5 && shown < 60)
        {
            std::printf("  %s: %.2f -> %.2f\n", path.c_str(), v0, v1);
            shown += 1;
        }
    }
}

TEST_CASE("standing by model", "[.standmodel]")
{
    struct C { const char* name; std::vector<std::pair<const char*, double>> over; };
    for (const C& c : {C{"every model on", {}}, C{"boost.model 0", {{"boost.model", 0}}}, C{"exhaust.model 0 (and boost 0)", {{"exhaust.model", 0}, {"boost.model", 0}}},
                       C{"intake.model 0", {{"intake.model", 0}}}, C{"ecu.model 0", {{"ecu.model", 0}}}})
    {
        Bench b("first");
        std::string e;
        REQUIRE(b.player().setSpec("env.temperature", 288.15, e));
        REQUIRE(b.player().setSpec("voices.level", -60.0, e));
        for (const auto& [f, v] : c.over)
            REQUIRE(b.player().setSpec(f, v, e));
        b.ctl().ignition = 1;
        double sum = 0, n = 0;
        b.play(0.5, [&] { for (float s : b.left()) { sum += double(s) * s; n += 1; } });
        std::printf("%-32s first half second %.0f dBFS\n", c.name, 10 * std::log10(std::max(sum / n, 1e-40)));
    }
}

// The fan's cycling under the physical gate (engine.wallHeat 1; the lead's ruling, 24 Nov): a warm engine idling in
// still air, its electric fan's thermoswitch Heat's; for each candidate `still`, voices off and on
TEST_CASE("fan cycles", "[.fan]")
{
    for (double still : {0.02, 0.05, 0.1})
        for (int vo = 0; vo < 2; ++vo)
        {
            Bench b("default");
            std::string e;
            REQUIRE(b.player().setSpec("engine.wallHeat", 1, e));
            REQUIRE(b.player().setSpec("voices.model", vo, e));
            b.write("heat.still", still);
            b.start();
            b.write("heat.tCool", 368.0); b.write("heat.tOil", 378.0); b.write("heat.tSurf", 455.0); b.write("heat.warm", 1.0); b.write("heat.warmS", 1.0);
            double lo = 1e9, hi = 0, onT = 0, t = 0, first = -1;
            int switches = 0, last = 0;
            b.play(600.0, [&] {
                t += 512.0 / 48000.0;
                const double tc = b.field("heat.tCool");
                const int on = int(b.field("heat.fanSwitch"));
                if (t > 60.0) { lo = std::min(lo, tc); hi = std::max(hi, tc); }
                if (on != last) { switches += 1; if (first < 0) first = t; }
                last = on;
                onT += on ? 512.0 / 48000.0 : 0.0;
            });
            std::printf("still %.2f voices %d: coolant %.2f to %.2f K after the first minute, the fan first at %.1f s, on %.1f %% of 600 s, %d switchings, ends %.3f K, %.3f rpm\n",
                        still, vo, lo, hi, first, 100.0 * onT / 600.0, switches, b.field("heat.tCool"), b.tel("engine.speed") * 9.5492966);
        }
}

// The tractor's books against the settle (the lead's ask, 25 Nov: what the 1 % is made of at 1800 rpm full load):
// consecutive 2 s windows after the dyno takes it, each store's rate
TEST_CASE("tractor books settle", "[.tractorbooks]")
{
    Bench b("archetypes/tractor-15-inline-six");
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
    const int nc = int(b.tel("engine.cylinders"));
    for (int w = 0; w < 12; ++w)
    {
        const double t0 = b.field("airThrottle"), v0 = b.field("airValves"), p0 = b.field("intk.mPlenum");
        double m0 = 0;
        for (int c = 0; c < nc; ++c)
            m0 += b.field("cyl[" + std::to_string(c) + "].m");
        b.play(2.0);
        double m1 = 0;
        for (int c = 0; c < nc; ++c)
            m1 += b.field("cyl[" + std::to_string(c) + "].m");
        const double raw = (b.field("airThrottle") - t0) * 500.0, pl = (b.field("intk.mPlenum") - p0) * 500.0;
        const double thr = raw - pl, v = (b.field("airValves") - v0) * 500.0;
        std::printf("window %2d (%4.0f-%4.0f s on the dyno): throttle %.2f, plenum's store %+.3f, booked %.2f, valves %.2f g/s: %+.3f %%; "
                    "cylinders %+.3f g/s; boost %.0f kPa, tAir %.1f K, %.0f N m\n",
                    w, 2.0 * w, 2.0 * w + 2.0, raw, pl, thr, v, 100.0 * (v / thr - 1.0), (m1 - m0) * 500.0,
                    b.tel("boost.pressure") / 1000.0, b.field("intk.tAir"), b.tel("engine.torque"));
    }
}

// N3-16 (1), the lead's ask, 25 Nov: the intake's reversion store on the V10 at a warm idle: its mass and burned share
// over 10 s, against the cylinder's share; a store that ratchets would grow or drift its share upward
TEST_CASE("reversion store", "[.rvstore]")
{
    Bench b("archetypes/v10-52");
    std::string e;
    REQUIRE(b.player().setSpec("car.model", 0, e));
    b.start();
    b.warm();
    b.ctl().throttle = 0.0f;
    for (int k = 0; k < 20; ++k)
    {
        b.play(0.5);
        const double rm = b.field("cyl[0].rvMass"), ri = b.field("cyl[0].rvInert");
        std::printf("t %4.1f s: store %.4f g, its burned share %.3f; cylinder's share now %.3f; %.0f rpm\n", 0.5 * (k + 1), 1e3 * rm,
                    rm > 1e-12 ? ri / rm : 0.0, b.field("cyl[0].mInert") / b.field("cyl[0].m"), b.tel("engine.speed") * 9.5492966);
    }
}

// The two routes into first.json ("written after init" and "initialised over it"), every field of the Engine instance
// compared after configure and after the key (hidden: "[.routediff]"; the lead's ask, 25 Nov 23:27)
using tee::test::core::Diff;
using tee::test::core::walk;

TEST_CASE("first.json's two routes, every field", "[.routediff]")
{
    // (the same draws written in the key's first block on both routes; snapshots after the start and 0.5 s later: a
    // field that holds still on both and differs is printed)
    std::vector<unsigned char> snap[2][2];
    const tee::Schema* schema = nullptr;
    for (int route = 0; route < 2; ++route)
    {
        Bench b("first");
        schema = &b.player().schema();
        if (route == 1)
            b.player().reset(Bench::kRate, true);
        b.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
        b.play(double(Bench::kBlock) / Bench::kRate);
        std::string e;
        auto& in = b.player().instance();
        for (int k = 0; k < 4; ++k)
            REQUIRE(in.write(*schema, "cyl[" + std::to_string(k) + "].bias", 0.01 * k, e));
        REQUIRE(in.write(*schema, "noise.rs", 12345.0, e));
        REQUIRE(in.write(*schema, "ex.noise.rs", 67890.0, e));
        b.play(1.0 - double(Bench::kBlock) / Bench::kRate);
        b.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
        b.play(1.5);
        const auto* d = static_cast<const unsigned char*>(in.data());
        snap[route][0].assign(d, d + in.size());
        b.play(0.5);
        snap[route][1].assign(d, d + in.size());
    }
    std::vector<Diff> across, still0, still1;
    walk(schema->fields(), 0, "", snap[0][0].data(), snap[1][0].data(), across);
    walk(schema->fields(), 0, "", snap[0][0].data(), snap[0][1].data(), still0);
    walk(schema->fields(), 0, "", snap[1][0].data(), snap[1][1].data(), still1);
    std::set<std::string> moving;
    for (const auto& d : still0) moving.insert(d.path);
    for (const auto& d : still1) moving.insert(d.path);
    std::size_t dynamic = 0;
    std::printf("after the key: %zu fields differ between the routes; those that hold still on both:\n", across.size());
    for (const auto& d : across)
    {
        if (moving.count(d.path) != 0)
            ++dynamic;
        else
            std::printf("  %s: %.9g | %.9g\n", d.path.c_str(), d.a, d.b);
    }
    std::printf("  and %zu that move\n", dynamic);
}

TEST_CASE("first.json's two routes, paired", "[.routepair]")
{
    // (the draws written in the key's first block, after both routes configure at the key)
    for (int draw = 0; draw < 4; ++draw)
    {
        double bias[4];
        for (int k = 0; k < 4; ++k)
            bias[k] = 0.025 * std::sin(1.7 * (draw * 4 + k) + 0.3);
        double tq[2] = {0, 0};
        for (int route = 0; route < 2; ++route)
        {
            Bench b("first");
            if (route == 1)
                b.player().reset(Bench::kRate, true);
            b.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
            b.play(double(Bench::kBlock) / Bench::kRate);
            std::string e;
            auto& in = b.player().instance();
            for (int k = 0; k < 4; ++k)
                REQUIRE(in.write(b.player().schema(), "cyl[" + std::to_string(k) + "].bias", bias[k], e));
            REQUIRE(in.write(b.player().schema(), "noise.rs", 12345.0 + draw, e));
            REQUIRE(in.write(b.player().schema(), "ex.noise.rs", 67890.0 + draw, e));
            b.play(1.0 - double(Bench::kBlock) / Bench::kRate);
            b.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
            b.play(1.5);
            b.ctl().mode = 2;
            b.ctl().throttle = 1.0f;
            b.ctl().dynoSpeed = float(3000.0 / 9.5492966);
            b.play(5.0);
            double s = 0.0, n = 0.0;
            b.play(1.0, [&] { s += b.tel("engine.torque"); n += 1.0; });
            tq[route] = s / n;
        }
        std::printf("draw %d paired: written after init %.2f N m, initialised over it %.2f N m (%+.3f %%)\n", draw, tq[0], tq[1], (tq[0] / tq[1] - 1.0) * 100.0);
    }
}

TEST_CASE("first.json: when each route configures", "[.routewhen]")
{
    for (int route = 0; route < 2; ++route)
    {
        Bench b("first");
        if (route == 1)
            b.player().reset(Bench::kRate, true);
        std::printf("route %d at 0: lay.n %g geo.n %g\n", route, b.field("lay.n"), b.field("geo.n"));
        double t = 0.0;
        for (int i = 0; i < 40; ++i)
        {
            b.play(0.05);
            t += 0.05;
            std::printf("  %.2f s key off: lay.n %g geo.n %g bias0 %.5f noise %g speed %.1f\n", t, b.field("lay.n"), b.field("geo.n"), b.field("cyl[0].bias"), b.field("noise.rs"), b.tel("engine.speed"));
            if (i == 5)
                break;
        }
        b.start();
        std::printf("  after start: lay.n %g bias0 %.5f\n", b.field("lay.n"), b.field("cyl[0].bias"));
    }
}

TEST_CASE("first.json's two routes, every field at the key", "[.routekey]")
{
    std::vector<unsigned char> snap[2];
    const tee::Schema* schema = nullptr;
    for (int route = 0; route < 2; ++route)
    {
        Bench b("first");
        schema = &b.player().schema();
        if (route == 1)
            b.player().reset(Bench::kRate, true);
        b.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
        b.play(double(Bench::kBlock) / Bench::kRate);
        std::string e;
        auto& in = b.player().instance();
        for (int k = 0; k < 4; ++k)
            REQUIRE(in.write(*schema, "cyl[" + std::to_string(k) + "].bias", 0.01 * k, e));
        REQUIRE(in.write(*schema, "noise.rs", 12345.0, e));
        REQUIRE(in.write(*schema, "ex.noise.rs", 67890.0, e));
        const auto* d = static_cast<const unsigned char*>(in.data());
        snap[route].assign(d, d + in.size());
    }
    std::vector<Diff> out;
    walk(schema->fields(), 0, "", snap[0].data(), snap[1].data(), out);
    std::printf("the key's first block: %zu fields differ\n", out.size());
    for (const auto& d : out)
        std::printf("  %s: %.9g | %.9g\n", d.path.c_str(), d.a, d.b);
}

// How far a 1e-6 nudge in one cylinder's gas at the key carries by 5 to 6 s at 3000 rpm full load (first.json,
// initialised over it, the same draws written on each run; hidden: "[.routenudge]")
TEST_CASE("first.json: a nudge at the key, carried to load", "[.routenudge]")
{
    for (int draw = 0; draw < 4; ++draw)
    {
        double bias[4];
        for (int k = 0; k < 4; ++k)
            bias[k] = 0.025 * std::sin(1.7 * (draw * 4 + k) + 0.3);
        double tq[2] = {0, 0};
        for (int run = 0; run < 2; ++run)
        {
            Bench b("first");
            b.player().reset(Bench::kRate, true);
            b.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
            b.play(double(Bench::kBlock) / Bench::kRate);
            std::string e;
            auto& in = b.player().instance();
            for (int k = 0; k < 4; ++k)
                REQUIRE(in.write(b.player().schema(), "cyl[" + std::to_string(k) + "].bias", bias[k], e));
            REQUIRE(in.write(b.player().schema(), "noise.rs", 12345.0 + draw, e));
            REQUIRE(in.write(b.player().schema(), "ex.noise.rs", 67890.0 + draw, e));
            if (run == 1)
                REQUIRE(in.write(b.player().schema(), "cyl[0].m", b.field("cyl[0].m") * (1.0 + 1e-6), e));
            b.play(1.0 - double(Bench::kBlock) / Bench::kRate);
            b.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
            b.play(1.5);
            b.ctl().mode = 2;
            b.ctl().throttle = 1.0f;
            b.ctl().dynoSpeed = float(3000.0 / 9.5492966);
            b.play(5.0);
            double s = 0.0, n = 0.0;
            b.play(1.0, [&] { s += b.tel("engine.torque"); n += 1.0; });
            tq[run] = s / n;
            std::printf("  run %d: knock retard %.2f, advance %.2f, boost %.4f bar, speed %.1f rpm\n", run, b.tel("ecu.knockRetard"),
                        b.tel("ecu.advance"), b.tel("boost.pressure") / 1e5, b.tel("engine.speed") * 9.5492966);
        }
        std::printf("draw %d: as run %.2f N m, nudged 1e-6 %.2f N m (%+.3f %%)\n", draw, tq[0], tq[1], (tq[1] / tq[0] - 1.0) * 100.0);
    }
}

// The large diesel's turbines with valve.model and boost.model on, engine.custom off and on: what each housing takes at
// full load on the engine dyno at 260 rad/s (the lead's item 2, with N2; hidden: "[.customturb]")
TEST_CASE("the large diesel's turbines under engine.custom", "[.customturb]")
{
    for (int custom = 0; custom < 2; ++custom)
    {
        Bench b("combo/large-diesel");
        std::string e;
        REQUIRE(b.player().setSpec("valve.model", 1, e));
        REQUIRE(b.player().setSpec("boost.model", 1, e));
        REQUIRE(b.player().setSpec("engine.custom", custom, e));
        b.start();
        b.ctl().mode = 2;
        b.ctl().throttle = 1.0f;
        b.ctl().dynoSpeed = 260.0f;
        b.play(4.0);
        double q[4] = {0, 0, 0, 0}, p[4] = {0, 0, 0, 0}, n = 0;
        b.play(1.0, [&] {
            for (int h = 0; h < 4; ++h)
            {
                q[h] += b.field("boost.mach.house[" + std::to_string(h) + "].qPipes");
                p[h] += b.field("boost.mach.house[" + std::to_string(h) + "].p");
            }
            n += 1; });
        std::printf("engine.custom %d: vTotal %.4f m3, redRpm %.0f, lay.n %g topo %g ports %g, nUsed %g, turbos %g, speed %.1f rad/s, torque %.0f N m, dpWave %.0f kPa\n",
                    custom, b.field("vTotal"), b.field("ecu.redRpm"), b.field("lay.n"), b.field("lay.topo"), b.field("lay.ports"),
                    b.field("exh.b.net.nUsed"), b.field("boost.mach.turbos"), b.tel("engine.speed"), b.tel("engine.torque"), b.tel("valve.dpWave") / 1000.0);
        std::printf("  IMEP %.2f bar, PMEP %.2f, FMEP %.2f, BMEP %.2f, boost %.3f bar\n", b.tel("engine.imep") / 1e5, b.tel("engine.pmep") / 1e5,
                    b.tel("engine.fmep") / 1e5, b.tel("engine.bmep") / 1e5, b.tel("boost.pressure") / 1e5);
        if (custom == 1)
            std::printf("  custom friction: rings %.3f bearings %.3f valves %.3f constant %.3f accessories %.3f bar; piston %.1f m/s, stroke %.4f m, peak %.0f bar\n",
                        b.field("fric.rings") / 1e5, b.field("fric.bearings") / 1e5, b.field("fric.valves") / 1e5, b.field("fric.constant") / 1e5,
                        b.field("fric.accessories") / 1e5, b.field("fric.pistonSpeed"), b.field("fric.stroke"), b.tel("engine.peakPressure") / 1e5);
        std::printf("  oilK %.3f, heat.viscK %.3f, coolant %.1f C\n", b.field("oilK"), b.field("heat.viscK"), b.tel("engine.coolant") - 273.15);
        for (int h = 0; h < 4; ++h)
            std::printf("  housing %d: pipes' flow %.5f m3/s, pressure %.3f bar\n", h, q[h] / n, p[h] / n / 1e5);
        for (int c = 0; c < 8; ++c)
            std::printf("  slot housing[%d] %g", c, b.field("exh.b.eng.housing[" + std::to_string(c) + "]"));
        std::printf("\n  hmap %g %g %g %g\n", b.field("exh.b.eng.hmap[0]"), b.field("exh.b.eng.hmap[1]"), b.field("exh.b.eng.hmap[2]"), b.field("exh.b.eng.hmap[3]"));
    }
}

// A custom engine whose custom fields are its table's: the configuration fields with engine.custom off and on
// (fuel.model 1 on both, so the friction is the same model), after the key; those that hold still on both runs and
// differ are printed (hidden: "[.customnull]")
TEST_CASE("a custom engine equal to its table, every configuration field", "[.customnull]")
{
    // (the custom fields written from the table's own engine: each cylinder's pin and revolution from the table's
    // firing angles (an inline's bank at 0), the table's redline; one-bank builds)
    for (const char* build : {"combo/large-diesel", "combo/turbo-six", "combo/small-four"})
    {
        std::vector<unsigned char> snap[2][2];
        const tee::Schema* schema = nullptr;
        double fire[12] = {}, red = 0.0;
        int n = 0;
        for (int custom = 0; custom < 2; ++custom)
        {
            Bench b(build);
            schema = &b.player().schema();
            std::string e;
            REQUIRE(b.player().setSpec("fuel.model", 1, e));
            if (custom == 1)
            {
                REQUIRE(b.player().setSpec("engine.custom", 1, e));
                REQUIRE(b.player().setSpec("engine.redlineSpeed", red / 9.5492966, e));
                const int pin = b.player().specIndex("engine.pin"), rev = b.player().specIndex("engine.fireRev");
                REQUIRE(pin >= 0);
                REQUIRE(rev >= 0);
                for (int c = 0; c < n; ++c)
                {
                    b.player().setSpec(std::size_t(pin), std::fmod(720.0 - fire[c], 360.0), std::size_t(c));
                    b.player().setSpec(std::size_t(rev), fire[c] >= 360.0 ? 1.0 : 0.0, std::size_t(c));
                }
            }
            const auto* d = static_cast<const unsigned char*>(b.player().instance().data());
            b.start();
            if (custom == 0)
            {
                n = int(b.field("geo.n"));
                for (int c = 0; c < n; ++c)
                    fire[c] = b.field("geo.fire[" + std::to_string(c) + "]");
                red = b.field("redBase");
            }
            std::printf("  custom %d: jFly %.5f mm.j0 %.5f mm.n %g mRot %.4f r %.4f set.load %.3f dynoOn %g mm.jMean %.5f\n", custom, b.field("jFly"),
                        b.field("mm.j0"), b.field("mm.n"), b.field("mm.mRot"), b.field("mm.sc.r"), b.field("set.load"), b.field("dynoOn"), b.field("mm.jMean"));
            snap[custom][0].assign(d, d + b.player().instance().size());
            b.play(0.5);
            snap[custom][1].assign(d, d + b.player().instance().size());
        }
        std::vector<Diff> across, still0, still1;
        walk(schema->fields(), 0, "", snap[0][0].data(), snap[1][0].data(), across);
        walk(schema->fields(), 0, "", snap[0][0].data(), snap[0][1].data(), still0);
        walk(schema->fields(), 0, "", snap[1][0].data(), snap[1][1].data(), still1);
        std::set<std::string> m0, m1;
        for (const auto& d : still0) m0.insert(d.path);
        for (const auto& d : still1) m1.insert(d.path);
        std::printf("%s: %zu fields differ; those still on both:\n", build, across.size());
        for (const auto& d : across)
            if (!m0.count(d.path) && !m1.count(d.path) && d.path.find(".rev") == std::string::npos && d.path.find("revSeen") == std::string::npos &&
                d.path.find("modelRev") == std::string::npos)
                std::printf("  %s: %.9g | %.9g\n", d.path.c_str(), d.a, d.b);
    }
}

// The oil at a predicted point: Warm.h's warmEngine() held every 64 blocks as Predict.cpp holds it, first.json on
// the engine dyno at 3000 rpm full load, oilK and viscK read (the lead's question, 26 Nov; hidden: "[.warmoil]")
TEST_CASE("first.json: the oil at a warm-held point", "[.warmoil]")
{
    Bench b("first");
    std::string e;
    auto hold = [&] {
        for (const tee::HeldValue& h : tee::warmEngine())
            REQUIRE(b.player().instance().write(b.player().schema(), h.path, h.value, e));
    };
    hold();
    b.start();
    b.ctl().mode = 2;
    b.ctl().throttle = 1.0f;
    b.ctl().dynoSpeed = float(3000.0 / 9.5492966);
    int k = 0;
    b.play(4.0, [&] { if (++k % 64 == 0) hold(); });
    std::printf("warm-held, 3000 rpm full load: oilK %.4f, viscK %.4f, oil %.1f K, coolant %.1f K, FMEP %.3f bar\n", b.field("oilK"), b.field("heat.viscK"),
                b.field("heat.tOil"), b.field("heat.tCool"), b.tel("engine.fmep") / 1e5);
}

TEST_CASE("first.json's two routes: the exhaust's counts", "[.routecounts]")
{
    for (int route = 0; route < 2; ++route)
    {
        Bench b("first");
        if (route == 1)
            b.player().reset(Bench::kRate, true);
        b.start();
        std::printf("route %d: nBands %g nj %g drawOrder", route, b.field("exh.b.net.nBands"), b.field("exh.b.net.nj"));
        for (int o = 0; o < 8; ++o)
            std::printf(" %g", b.field("exh.b.net.drawOrder[" + std::to_string(o) + "]"));
        std::printf("; ends[0].duct %g ends[1].duct %g\n", b.field("exh.b.net.ends[0].duct"), b.field("exh.b.net.ends[1].duct"));
    }
}

// Where first.json's knock loop is quiet: at each point, the routes paired (same draws in the key's first block) and
// route 1 nudged 1e-6, torque read 5 to 6 s; the knock retard printed (hidden: "[.quietpoint]")
TEST_CASE("first.json: a knock-quiet point for the routes' torque", "[.quietpoint]")
{
    const double pts[][2] = {{1500, 1.0}};
    for (const auto& pt : pts)
    {
        for (int draw = 3; draw < 9; ++draw)
        {
            double tq[3] = {0, 0, 0}, ret[3] = {0, 0, 0};
            for (int run = 0; run < 3; ++run)   // route 0, route 1, route 1 nudged
            {
                Bench b("first");
                if (run >= 1)
                    b.player().reset(Bench::kRate, true);
                b.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
                b.play(double(Bench::kBlock) / Bench::kRate);
                std::string e;
                auto& in = b.player().instance();
                for (int k = 0; k < 4; ++k)
                    REQUIRE(in.write(b.player().schema(), "cyl[" + std::to_string(k) + "].bias", 0.025 * std::sin(1.7 * (draw * 4 + k) + 0.3), e));
                REQUIRE(in.write(b.player().schema(), "noise.rs", 12345.0 + draw, e));
                REQUIRE(in.write(b.player().schema(), "ex.noise.rs", 67890.0 + draw, e));
                if (run == 2)
                    REQUIRE(in.write(b.player().schema(), "cyl[0].m", b.field("cyl[0].m") * (1.0 + 1e-6), e));
                b.play(1.0 - double(Bench::kBlock) / Bench::kRate);
                b.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
                b.play(1.5);
                b.ctl().mode = 2;
                b.ctl().throttle = float(pt[1]);
                b.ctl().dynoSpeed = float(pt[0] / 9.5492966);
                b.play(5.0);
                double s = 0.0, r = 0.0, n = 0.0;
                b.play(1.0, [&] { s += b.tel("engine.torque"); r = std::max(r, double(b.tel("ecu.knockRetard"))); n += 1.0; });
                tq[run] = s / n;
                ret[run] = r;
            }
            std::printf("%4.0f rpm pedal %.1f draw %d: %.2f | %.2f | nudged %.2f N m: routes %+.3f %%, nudge %+.3f %%; max retard %.2f %.2f %.2f\n", pt[0], pt[1],
                        draw, tq[0], tq[1], tq[2], (tq[0] / tq[1] - 1) * 100, (tq[2] / tq[1] - 1) * 100, ret[0], ret[1], ret[2]);
        }
    }
}

TEST_CASE("first.json: why the key configures again", "[.routekeywhy]")
{
    for (int route = 0; route < 2; ++route)
    {
        Bench b("first");
        if (route == 1)
            b.player().reset(Bench::kRate, true);
        b.play(double(Bench::kBlock) / Bench::kRate);
        std::printf("route %d before the key: layout %g/%g fuel %g/%g asp %g/%g geoSig %g/%g size %g/%g ambF %g/%g\n", route, b.field("set.layout"),
                    b.field("lastLayout"), b.field("set.fuel"), b.field("fuelUse"), b.field("set.asp"), b.field("aspUse"), b.field("geoSig"),
                    b.field("geoUsed"), b.field("set.size"), b.field("lastSize"), b.field("set.ambF"), b.field("heat.ambSeen"));
        std::printf("  the stopped engine reports: redline %.0f rpm, cylinders %g, vTotal %.3f L, idle %.0f rpm\n", b.tel("engine.redline") * 9.5492966,
                    b.tel("engine.cylinders"), b.field("vTotal") * 1000.0, b.field("ecu.idleRpm"));
    }
}

// Every custom build's headers: its topology, banks and how many collectors the builder makes (hidden: "[.customtopo]")
TEST_CASE("custom builds: their headers", "[.customtopo]")
{
    std::FILE* f = std::fopen(std::getenv("TEE_CUSTOM_LIST"), "r");
    REQUIRE(f != nullptr);
    char line[512];
    while (std::fgets(line, sizeof line, f) != nullptr)
    {
        std::string path(line);
        path.erase(path.find_last_not_of("\r\n") + 1);
        const auto at = path.find("content/builds/");
        std::string name = path.substr(at + 15);
        name = name.substr(0, name.size() - 5);
        Bench b(name);
        b.play(double(Bench::kBlock) / Bench::kRate);
        std::printf("%-55s n %2g rotary %g topo %g banks", name.c_str(), b.field("lay.n"), b.field("lay.rotary"), b.field("lay.topo"));
        const int n = int(b.field("lay.n"));
        for (int c = 0; c < n; ++c)
            std::printf("%g", b.field("lay.bank[" + std::to_string(c) + "]"));
        std::printf(" boost %g/%g\n", b.field("spec.boost.kind"), b.field("spec.boost.model"));
    }
    std::fclose(f);
}

// The general split exercised: a custom one-bank engine whose table does not match it, with twin turbos (hidden:
// "[.splitturb]")
TEST_CASE("custom one-bank twin turbos split by firing order", "[.splitturb]")
{
    struct Case { const char* build; int layout; };
    for (const Case& k : {Case{"combo/large-diesel", 5}, Case{"archetypes/turbo-four-20-gdi", -1}, Case{"archetypes/rotary-twin-13", -2},
                          Case{"archetypes/rotary-twin-13", -3}})
    {
        Bench b(k.build);
        std::string e;
        REQUIRE(b.player().setSpec("valve.model", 1, e));
        REQUIRE(b.player().setSpec("boost.model", 1, e));
        REQUIRE(b.player().setSpec("boost.kind", 2, e));
        REQUIRE(b.player().setSpec("engine.custom", k.layout == -3 ? 0 : 1, e));
        if (k.layout >= 0)
            REQUIRE(b.player().setSpec("engine.layout", k.layout, e));
        b.start();
        b.ctl().mode = 2;
        b.ctl().throttle = 1.0f;
        b.ctl().dynoSpeed = 260.0f;
        b.play(4.0);
        double q[4] = {0, 0, 0, 0}, n = 0;
        b.play(1.0, [&] { for (int h = 0; h < 4; ++h) q[h] += b.field("boost.mach.house[" + std::to_string(h) + "].qPipes"); n += 1; });
        std::printf("%s layout %d: n %g topo %g banks", k.build, k.layout, b.field("lay.n"), b.field("lay.topo"));
        for (int c = 0; c < int(b.field("lay.n")); ++c)
            std::printf("%g", b.field("lay.bank[" + std::to_string(c) + "]"));
        std::printf("; housings %.4f %.4f %.4f %.4f m3/s, dpWave %.0f kPa, torque %.0f N m\n", q[0] / n, q[1] / n, q[2] / n, q[3] / n,
                    b.tel("valve.dpWave") / 1000.0, b.tel("engine.torque"));
    }
}

TEST_CASE("first.json: the runners' wall in use", "[.runnerwall]")
{
    Bench b("first");
    b.start();
    for (int k = 0; k < 4; ++k)
        std::printf("intk.run[%d]: wallOn %g, len %.3f m, diam %.4f m, c %.1f\n", k, b.field("intk.run[" + std::to_string(k) + "].wallOn"),
                    b.field("intk.run[" + std::to_string(k) + "].len"), b.field("intk.run[" + std::to_string(k) + "].diam"),
                    b.field("intk.run[" + std::to_string(k) + "].c"));
}

TEST_CASE("custom twin turbos arranged in sequence keep one collector", "[.seqturb]")
{
    Bench b("archetypes/turbo-four-20-gdi");
    std::string e;
    REQUIRE(b.player().setSpec("boost.model", 1, e));
    REQUIRE(b.player().setSpec("boost.kind", 2, e));
    REQUIRE(b.player().setSpec("boost.layout", 3, e));
    REQUIRE(b.player().setSpec("engine.custom", 1, e));
    b.start();
    std::printf("custom four, twin turbos in sequence: topo %g\n", b.field("lay.topo"));
}

// The owner's report (27 Nov): shifting works only on the manual. Each gearbox kind driven from rest by the counters
// alone (lever 0, as the application sends under its automatic clutch), then by the lever (hidden: "[.shiftkinds]")
TEST_CASE("each gearbox kind from rest: the counters, then the lever", "[.shiftkinds]")
{
    struct K { const char* build; const char* kind; };
    for (const K& k : {K{"archetypes/hot-hatch-16-16v", "manual"}, K{"archetypes/inline-five-turbo-rally", "sequential"},
                       K{"archetypes/turbo-four-20-gdi", "dual clutch"}, K{"archetypes/v6-35-na", "automatic"},
                       K{"archetypes/economy-four-15", "CVT"}, K{"archetypes/tractor-15-inline-six", "eighteen"}})
    {
        for (int byLever = 0; byLever < 2; ++byLever)
        {
            Bench b(k.build);
            b.start();
            b.ctl().mode = 1;
            b.play(0.2);
            const double g0 = b.tel("car.gear");
            if (byLever == 0)
                b.ctl().shiftUp += 1;
            else
                b.ctl().lever = 1;
            b.play(0.5);
            const double g1 = b.tel("car.gear");
            b.ctl().throttle = 0.4f;
            b.play(3.0);
            const double g2 = b.tel("car.gear");
            if (byLever == 0)
                b.ctl().shiftUp += 1;
            b.play(1.0);
            std::printf("%-12s %s: gear %g, after the input %g, after 3 s at 0.4 %g (%.1f km/h), after another up press %g\n", k.kind,
                        byLever ? "lever 1    " : "up counter ", g0, g1, g2, b.tel("car.speed") * 3.6, b.tel("car.gear"));
        }
    }
}

// X-056 (B's trace, 27 Nov): a sequential box shifted 1 to 2 at a standstill; the engine's speed, the robotised
// clutch and the engine's state through it, and two moving cases (hidden: "[.x056]")
TEST_CASE("X-056: a sequential's dog shift at rest and while the launch clutch slips", "[.x056]")
{
    struct C { const char* what; float throttle; double before; int downTo1; };
    for (const C& c : {C{"at rest, 1 to 2", 0.0f, 0.0, 0}, C{"pulling away (pedal 0.3, 0.4 s in), 1 to 2", 0.3f, 0.4, 0},
                       C{"a crawl, 2 to 1", 0.15f, 1.5, 1}})
    {
        Bench b("archetypes/inline-five-turbo-rally");
        b.start();
        b.ctl().mode = 1;
        b.play(0.2);
        b.ctl().shiftUp += 1;          // into first
        b.play(0.2);
        if (c.downTo1 != 0)
        {
            b.ctl().shiftUp += 1;      // second, then let it roll at a crawl
            b.play(0.2);
        }
        b.ctl().throttle = c.throttle;
        b.play(c.before);
        if (c.downTo1 != 0)
        {
            b.ctl().throttle = 0.0f;
            b.ctl().shiftDown += 1;
        }
        else
            b.ctl().shiftUp += 1;
        double minRpm = 1e9, maxRobot = 0, minRobot = 1;
        int state = 9;
        b.play(1.0, [&] {
            minRpm = std::min(minRpm, b.tel("engine.speed") * 9.5492966);
            maxRobot = std::max(maxRobot, b.field("car.m1.box.robot"));
            minRobot = std::min(minRobot, b.field("car.m1.box.robot"));
            state = std::min(state, int(b.field("ecu.state")));
        });
        std::printf("%-46s gear %g, %.1f km/h; through the shift: engine down to %.0f rpm, robot %.2f to %.2f, least engine state %d, now %g\n",
                    c.what, b.tel("car.gear"), b.tel("car.speed") * 3.6, minRpm, minRobot, maxRobot, state, b.field("ecu.state"));
    }
}

// X-056's edge (the lead's question): an upshift at speed with the engine under idle + 110 rad/s: the synced latch and
// the robotised clutch through the shift, step by step (hidden: "[.x056edge]")
TEST_CASE("X-056: the synced latch through a low-revs dog upshift", "[.x056edge]")
{
    for (float ped : {0.06f, 0.08f, 0.12f, 0.2f, 0.35f})
    {
        Bench b("archetypes/inline-five-turbo-rally");
        b.start();
        b.ctl().mode = 1;
        b.play(0.2);
        b.ctl().shiftUp += 1;
        b.play(0.2);
        b.ctl().throttle = ped;
        b.play(ped < 0.1f ? 2.0 : 4.0);
        const double idle = b.field("ecu.idleRpm");
        std::printf("pedal %.2f: before the shift %.0f rpm (idle %.0f, idle + 110 rad/s = %.0f), synced %g, robot %.3f, %.1f km/h\n", ped,
                    b.tel("engine.speed") * 9.5492966, idle, idle + 110.0 * 9.5492966, b.field("car.m1.box.synced"), b.field("car.m1.box.robot"),
                    b.tel("car.speed") * 3.6);
        b.ctl().shiftUp += 1;
        int k = 0;
        b.play(0.15, [&] {
            if (k++ < 9)
                std::printf("   step %d: shifting %g gear %g synced %g robot %.3f engine %.0f rpm\n", k, b.field("car.m1.box.shifting"),
                            b.field("car.m1.box.gear"), b.field("car.m1.box.synced"), b.field("car.m1.box.robot"), b.tel("engine.speed") * 9.5492966);
        });
    }
}

TEST_CASE("X-056: a synced low-revs upshift on the overrun", "[.x056coast]")
{
    for (double coast : {3.5, 5.0, 7.0})
    {
        Bench b("archetypes/inline-five-turbo-rally");
        b.start();
        b.ctl().mode = 1;
        b.play(0.2);
        b.ctl().shiftUp += 1;
        b.play(0.2);
        b.ctl().throttle = 0.2f;
        b.play(3.0);
        b.ctl().throttle = 0.0f;
        b.play(coast);
        const double idle = b.field("ecu.idleRpm");
        const double w = b.tel("engine.speed") * 9.5492966;
        const double x = std::min(std::max((b.tel("engine.speed") - idle / 9.5492966 - 15.7) / 94.2, 0.0), 1.0);
        std::printf("coast %.1f s: %.0f rpm (idle %.0f), synced %g, robot %.3f; the revs' launch term %.3f (what the cure would hold through the shift)\n",
                    coast, w, idle, b.field("car.m1.box.synced"), b.field("car.m1.box.robot"), x * x * (3.0 - 2.0 * x));
    }
}

// X-057 (B's trace, 27 Nov): the tuned diesel pickup in D at full throttle, the redline the vehicle holds, the
// automatic's shift point, the governor (hidden: "[.x057]")
TEST_CASE("X-057: a governed diesel's automatic and its shift point", "[.x057]")
{
    for (const char* build : {"diesel-pickup-tuned", "archetypes/pickup-diesel-67-inline-six"})
    {
        Bench b(build);
        b.start();
        b.ctl().mode = 1;
        b.ctl().lever = 1;
        b.ctl().throttle = 1.0f;
        b.play(20.0);
        const double k = 9.5492966;
        const double idleW = b.field("car.m1.idleW"), redW = b.field("car.m1.redW");
        const double wUp = std::min(idleW + 0.9 * (redW - idleW), 0.85 * redW);
        const int n = int(b.field("car.m1.box.n"));
        const double fd = b.field("car.m1.box.fd"), r1 = b.field("car.m1.box.r[0]"), rTop = b.field("car.m1.box.r[" + std::to_string(n - 1) + "]");
        const double radius = b.field("car.m1.radius");
        const double v = b.tel("car.speed");
        std::printf("%s: gear %g at %.1f km/h, engine %.0f rpm; the vehicle's idleW %.0f rpm, redW %.0f rpm; wUp at full pedal %.0f rpm; the input in first "
                    "at this speed %.0f rpm; ecu.redRpm %.0f, rated %.0f rpm; gears %d, first %.2f, top %.2f, final %.2f; top gear at %.0f rpm: %.0f km/h\n",
                    build, b.tel("car.gear"), v * 3.6, b.tel("engine.speed") * k, idleW * k, redW * k, wUp * k, v / radius * r1 * fd * k,
                    b.field("ecu.redRpm"), b.field("spec.ecu.ratedSpeed") * k, n, r1, rTop, fd, b.tel("engine.speed") * k,
                    b.tel("engine.speed") / (rTop * fd) * radius * 3.6);
    }
}

// Item 19 (X-057's rule): the three governed builds whose high idle sits under the reference ECU's idle + 1500 floor;
// their stated governor and the redline the ECU gives them today (hidden: "[.x057figs]")
TEST_CASE("Item 19: the governed builds under the idle + 1500 floor", "[.x057figs]")
{
    const double k = 9.5492966;
    for (const char* build : {"diesel-v8-truck", "diesel-mechanical-classic", "diesel-marine-v12"})
    {
        Bench b(build);
        b.start();
        b.play(2.0);
        const double rated = b.field("spec.ecu.ratedSpeed") * k, droop = b.field("spec.ecu.droop");
        const double hiStated = b.field("spec.ecu.highIdle") * k, idle = b.field("ecu.idleRpm"), red = b.field("ecu.redRpm");
        std::printf("%s: ratedSpeed %.1f rpm, droop %.3f, highIdle stated %.1f rpm -> governed high idle %.1f rpm; ecu.idleRpm %.1f, "
                    "floor (idle + 1500) %.1f, ecu.redRpm today %.1f, ecu.model %g\n",
                    build, rated, droop, hiStated, hiStated > 0 ? hiStated : rated * (1 + droop), idle, idle + 1500, red,
                    b.field("spec.ecu.model"));
    }
}

// X-056's guard measured (27 Nov): the synced overrun 1-2 at about 1500 rpm, step by step (hidden: "[.x056guard]")
TEST_CASE("X-056: the overrun 1-2's robot, step by step", "[.x056guard]")
{
    Bench b("archetypes/inline-five-turbo-rally");
    b.start();
    b.ctl().mode = 1;
    b.play(0.2);
    b.ctl().shiftUp += 1;
    b.play(0.2);
    b.ctl().throttle = 0.2f;
    b.play(3.0);
    b.ctl().throttle = 0.0f;
    b.play(5.0);
    b.ctl().shiftUp += 1;
    int k = 0;
    b.play(0.5, [&] {
        if (k++ % 3 == 0)
            std::printf("t+%3d: shifting %g gear %g synced %g robot %.3f engine %.0f rpm\n", k, b.field("car.m1.box.shifting"),
                        b.field("car.m1.box.gear"), b.field("car.m1.box.synced"), b.field("car.m1.box.robot"), b.tel("engine.speed") * 9.5492966);
    });
}
