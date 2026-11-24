// tests/sim/boost/EngineTest.cpp: card F's integrated tests: the machine (boost.model 1) in the real engine
// (sim/engine.fm through tee::EnginePlayer). The exhaust side is the stand-in in Boost::exhaustStandIn until card G's
// network ends its collectors in the turbine's housings (sim/README.md, Boost).
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
    // The reference engine's exhaust leaves its ports at 450 to 560 K at full throttle (exh.tPort), some half of a
    // real gasoline engine's 900 to 1100 K; the matched sizes assume 1150 K. So: the matched turbo, as measured, and
    // one sized down (boost.size 0.55, the turbine and compressor some 0.74 of the matched diameters), which makes its
    // spring.
    for (double size : {1.0, 0.55})
    {
        Engine e;
        e.spec("boost.size", size);
        e.turboFour(1);
        auto& c = e.player->controls();
        c.throttle = 1.0f;
        double boostAt[2] = {0.0, 0.0};
        int k = 0;
        for (double rpm : {3500.0, 5500.0})
        {
            c.holdSpeed = static_cast<float>(rpm * 0.10471976);
            REQUIRE(e.run(4.0));
            const double boost = e.tel("boost.pressure");
            const double shaft = e.tel("boost.shaftSpeed[0]") * 60.0 / 6.283185307;
            const double drive = e.tel("boost.drive[0]");
            std::printf("size %.1f at %.0f rpm (held %.0f): boost %.1f kPa, shaft %.0f rpm, drive ratio %.2f, gate %.2f, "
                        "compressor %.2f at %.3f kg/s; the ports' gas %.0f K, the turbine's %.0f K\n",
                        size, rpm, e.tel("engine.speed") * 60.0 / 6.283185307, boost / 1000.0, shaft, drive,
                        e.tel("boost.gate[0]"), e.tel("boost.ratio[0]"), e.tel("boost.flow[0]"), e.get("exh.tPort"),
                        e.tel("boost.turbineInTemp[0]"));
            boostAt[k++] = boost;
            CHECK(shaft > 20000.0);
            if (boost > 30000.0)
            {
                // shaft speeds of a GT28's class on boost: 80,000 to 250,000 rpm (the card's range)
                CHECK(shaft > 80000.0);
                CHECK(shaft < 250000.0);
                // the pressure before the turbine 1 to 2 times the charge pipe's (absolute): above 1 always; the upper
                // bound only for the matched turbine (the undersized one, on this engine's cool gas, reaches 2.04)
                CHECK(drive > 1.0);
                if (size >= 1.0)
                    CHECK(drive < 2.0);
            }
        }
        CHECK(boostAt[1] > boostAt[0]);
        if (size < 1.0)
        {
            // the spring's 70 kPa held at its actuator (the charge pipe) at 5500 rpm, within 10 % (a 2 L turbo four's
            // 0.6 to 1.2 bar)
            const double pipe = e.tel("boost.chargePipe");
            std::printf("the charge pipe at 5500 rpm: %.1f kPa\n", pipe / 1000.0);
            CHECK(pipe > 0.9 * 70000.0);
            CHECK(pipe < 1.1 * 70000.0);
        }
    }
}

TEST_CASE("boost engine: a supercharger takes torque from the crank, the machine's, in the engine", "[boost][engine]")
{
    Engine e;
    e.turboFour(4);
    auto& c = e.player->controls();
    c.throttle = 1.0f;
    c.holdSpeed = static_cast<float>(4500.0 * 0.10471976);
    REQUIRE(e.run(3.0));
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
        auto& c = e.player->controls();
        c.throttle = 1.0f;
        c.holdSpeed = static_cast<float>(5500.0 * 0.10471976);
        REQUIRE(e.run(4.0));
        const double before = e.tel("boost.pressure");
        c.holdSpeed = 0.0f;
        c.throttle = 0.0f;
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
