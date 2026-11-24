// tests/sim/core/EngineExplore.cpp: the engine on the dyno, printed (hidden: "[.dynosweep]").
#include "Engine.h"
#include "Presets.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

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

// The lead's fan cycling (M's switch at 373 K on, 368 off): a warm engine idling in still air, voices.model 1, for
// each candidate `still`: the coolant's course and the fan's
TEST_CASE("fan cycles", "[.fan]")
{
    for (double still : {0.02, 0.05, 0.1, 0.2})
    {
        Bench b("default");
        std::string e;
        REQUIRE(b.player().setSpec("voices.model", 1, e));
        b.write("heat.still", still);
        b.start();
        b.write("heat.tCool", 368.0); b.write("heat.tOil", 378.0); b.write("heat.tSurf", 455.0); b.write("heat.warm", 1.0); b.write("heat.warmS", 1.0);
        double lo = 1e9, hi = 0, onT = 0, t = 0;
        int switches = 0, last = 0;
        b.play(600.0, [&] {
            t += 512.0 / 48000.0;
            const double tc = b.field("heat.tCool");
            const int on = int(b.field("ex.vo.fan.on"));
            if (t > 60.0) { lo = std::min(lo, tc); hi = std::max(hi, tc); }
            if (on != last) switches += 1;
            last = on;
            onT += on ? 512.0 / 48000.0 : 0.0;
        });
        std::printf("still %.2f: coolant %.1f to %.1f K after the first minute, the fan on %.0f %% of 600 s, %d switchings, ends %.1f K\n",
                    still, lo, hi, 100.0 * onT / 600.0, switches, b.field("heat.tCool"));
    }
}
