// tests/sim/boost/EngineTest.cpp: card F's integrated tests: the machine (boost.model 1) in the real engine
// (sim/engine.fm through tee::EnginePlayer), at full load on the engine dyno. The exhaust side is card G's network,
// whose collectors end in the turbine's housings (boost.model 1 implies it; sim/README.md, Boost).
#include "sim/Twin.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using tee::EnginePlayer;

namespace
{
    struct Engine
    {
        std::unique_ptr<EnginePlayer> player;
        std::vector<float> l = std::vector<float>(512);
        std::vector<float> r = std::vector<float>(512);

        Engine()
        {
            player = tee::test::enginePlayer(48000.0);
            player->setSpecDefaults();
        }

        void spec(const std::string& path, double v)
        {
            std::string error;
            const bool ok = player->setSpec(path, v, error);
            INFO(path << ": " << error);
            REQUIRE(ok);
        }

        double get(const std::string& path) const
        {
            double v = 0.0;
            std::string error;
            const bool ok = player->read(path, v, error);
            INFO(path << ": " << error);
            REQUIRE(ok);
            return v;
        }

        // a telemetry field by its path under tel ("boost.shaftSpeed[0]")
        double tel(const std::string& name) const { return get("tel." + name); }

        // seconds of sound; false once it stops (a check) or writes something not finite
        bool run(double seconds)
        {
            const int calls = static_cast<int>(seconds * 48000.0 / 512.0 + 0.5);
            for (int k = 0; k < calls; ++k)
            {
                player->process(l.data(), r.data(), 512);
                for (int i = 0; i < 512; ++i)
                    if (!std::isfinite(l[i]) || std::fabs(l[i]) > 1.0f)
                        return false;
            }
            return !player->stopped();
        }

        // the dyno (MODE_DYNO_ENGINE) holds the crank at rpm, the throttle open: full load. (The bench's holdSpeed
        // driver holds a speed with almost no load: its brake torque reads near 0 and its exhaust runs cool.)
        void dyno(double rpm, float throttle = 1.0f)
        {
            auto& c = player->controls();
            c.mode = 2;
            c.dynoSpeed = static_cast<float>(rpm * 0.10471976);
            c.throttle = throttle;
        }

        // a 2 L four, with the forced induction asked for; started and running on the bench
        void turboFour(int kind, int model = 1)
        {
            spec("engine.layout", 5);
            spec("engine.displacement", 0.002);
            spec("boost.kind", kind);
            spec("boost.model", model);
            auto& c = player->controls();
            c.mode = EnginePlayer::kModeBench;
            c.ignition = EnginePlayer::kIgnitionStart;
            REQUIRE(run(1.5));
            c.ignition = EnginePlayer::kIgnitionOn;
            REQUIRE(run(1.0));
        }
    };
} // namespace

TEST_CASE("boost engine: a 2 L turbo four spools, holds its spring, and its turbine's numbers are a real one's",
          "[boost][engine]")
{
    // the matched turbo (every size 0) at full load on the dyno, 8 s at each speed (the exhaust's temperature settles
    // in some 4 s: exh.tPort follows the ports' gas over 0.8 s, from cold)
    Engine e;
    e.turboFour(1);
    double boostAt[2] = {0.0, 0.0};
    int k = 0;
    for (double rpm : {3500.0, 5500.0})
    {
        e.dyno(rpm);
        REQUIRE(e.run(8.0));
        const double boost = e.tel("boost.pressure");
        const double shaft = e.tel("boost.shaftSpeed[0]") * 60.0 / 6.283185307;
        // (the drive ratio is instantaneous, and the housing's pressure pulses: its mean over 100 blocks)
        double drive = 0.0;
        for (int i = 0; i < 100; ++i)
        {
            REQUIRE(e.run(512.0 / 48000.0));
            drive += e.tel("boost.drive[0]") / 100.0;
        }
        std::printf("matched turbo at %.0f rpm on the dyno: boost %.1f kPa (charge pipe %.1f), shaft %.0f rpm, drive ratio "
                    "%.2f, gate %.2f, compressor %.2f at %.3f kg/s, %.1f N m; the ports' gas %.0f K\n",
                    rpm, boost / 1000.0, e.tel("boost.chargePipe") / 1000.0, shaft, drive, e.tel("boost.gate[0]"),
                    e.tel("boost.ratio[0]"), e.tel("boost.flow[0]"), e.tel("engine.torque"), e.get("exh.tPort"));
        boostAt[k++] = boost;
        CHECK(e.get("exh.tPort") > 900.0);
        if (boost > 30000.0)
        {
            // shaft speeds of a GT28's class on boost: 80,000 to 250,000 rpm (the card's range)
            CHECK(shaft > 80000.0);
            CHECK(shaft < 250000.0);
            // the pressure before the turbine 1 to 2 times the charge pipe's (absolute)
            CHECK(drive > 1.0);
            CHECK(drive < 2.0);
        }
    }
    CHECK(boostAt[1] >= 0.9 * boostAt[0]);
    // by 5500 rpm the spring's 70 kPa held at its actuator (the charge pipe) within 10 %
    const double pipe = e.tel("boost.chargePipe");
    CHECK(pipe > 0.9 * 70000.0);
    CHECK(pipe < 1.1 * 70000.0);
}

TEST_CASE("boost engine: a supercharger takes torque from the crank, the machine's, in the engine", "[boost][engine]")
{
    Engine e;
    e.turboFour(4);
    e.dyno(4500.0);
    REQUIRE(e.run(4.0));
    const double tq = e.get("boost.mach.crankTorque");
    const double power = e.tel("boost.superchargerPower");
    std::printf("Roots on a 2 L four at 4500 rpm: boost %.1f kPa, crank torque %.1f N m, %.0f W\n",
                e.tel("boost.pressure") / 1000.0, tq, power);
    CHECK(e.tel("boost.pressure") > 10000.0);
    CHECK(tq > 1.0);
    CHECK(power == Catch::Approx(tq * 4500.0 * 0.10471976).epsilon(0.1));
}

TEST_CASE("boost engine: a throttle chop on boost with no blow-off valve surges, with one it does not", "[boost][engine]")
{
    for (int bov : {0, 1})
    {
        Engine e;
        e.spec("boost.blowOffKind", bov);
        e.spec("boost.gateSpring", 100000.0);
        e.turboFour(1);
        e.dyno(5500.0);
        REQUIRE(e.run(6.0));
        const double before = e.tel("boost.pressure");
        e.player->controls().throttle = 0.0f;   // (the dyno holds the speed: the throttle shuts on boost)
        int surging = 0;
        for (int i = 0; i < 100; ++i)
        {
            REQUIRE(e.run(512.0 / 48000.0));
            if (static_cast<int>(e.tel("boost.flags")) & 1)
                ++surging;
        }
        std::printf("chop from %.1f kPa, blow-off %s: surge flagged in %d of 100 blocks (1 s)\n", before / 1000.0,
                    bov ? "to the air" : "none", surging);
        if (bov == 0)
            CHECK(surging > 5);
        else
            CHECK(surging == 0);
    }
}

TEST_CASE("boost engine: on the dyno, a turbo four's brake mean effective pressure is a turbo engine's", "[boost][engine]")
{
    // BMEP = 4 pi T / Vd from the absorber's brake torque (tel.engine.torque) at 5500 rpm, the throttle open: a natural
    // 2 L four against the same engine with its matched turbo. Production 2 L turbo fours make some 15 to 25 bar (300
    // to 400 N m) and natural ones 10 to 13 (makers' published torques); this engine's natural four makes 9.
    double bmep[2] = {0.0, 0.0};
    for (int kind : {0, 1})
    {
        Engine e;
        e.turboFour(kind);
        e.dyno(5500.0);
        REQUIRE(e.run(8.0));
        double sum = 0.0;
        for (int i = 0; i < 20; ++i)
        {
            REQUIRE(e.run(0.05));
            sum += e.tel("engine.torque");
        }
        const double torque = sum / 20.0;
        bmep[kind] = 4.0 * 3.14159265 * torque / 0.002;
        std::printf("dyno at 5500 rpm, %s: %.1f N m brake, BMEP %.1f bar, boost %.1f kPa\n", kind ? "turbo" : "natural",
                    torque, bmep[kind] / 1.0e5, e.tel("boost.pressure") / 1000.0);
    }
    CHECK(bmep[1] > 1.3 * bmep[0]);
    CHECK(bmep[1] > 13.0e5);
    CHECK(bmep[1] < 25.0e5);
}


TEST_CASE("boost engine: the intercooler's core settles at a press, and a test cell holds it at the air",
          "[boost][engine]")
{
    // (card N2) spec.boost.heatSettle (card C's dynos): at each press the core is put where the present flow and the
    // air through it hold it. Standing (no absorber, no road: still air) at full throttle the core warms over some
    // 15 to 25 s; a press puts it at that end at once. On the engine dyno (Boost::testCell, Engine's) the core is held
    // at the air's temperature: SAE J1349's laboratory charge-air cooling.
    Engine e;
    e.turboFour(1);
    e.dyno(4000.0);
    REQUIRE(e.run(3.0));
    const double air = e.get("boost.mach.tOut");
    const double held = e.get("boost.mach.tCore");
    // the bench: a dead-air core, the throttle open at the limiter
    auto& c = e.player->controls();
    c.mode = EnginePlayer::kModeBench;
    c.throttle = 1.0f;
    REQUIRE(e.run(2.0));
    const double before = e.get("boost.mach.tCore");
    e.spec("boost.heatSettle", 1);
    REQUIRE(e.run(0.1));
    const double settled = e.get("boost.mach.tCore");
    REQUIRE(e.run(15.0));
    const double later = e.get("boost.mach.tCore");
    std::printf("intercooler core: air %.1f K; held on the dyno %.1f K; on the bench %.1f K after 2 s, %.1f K at the "
                "press, %.1f K 15 s on\n",
                air, held, before, settled, later);
    CHECK(held == Catch::Approx(air).margin(0.5));
    // (measured: 305.5 K after 2 s, 321.6 at the press, 321.8 after 40 s more: the core's own end; 15 s here)
    CHECK(settled > before + 8.0);
    CHECK(std::fabs(later - settled) < 1.5);
}
