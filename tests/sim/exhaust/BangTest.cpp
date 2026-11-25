// tests/sim/exhaust/BangTest.cpp: card G's bangs as heat released in the pipe (spec.exhaust.bang 1, the builder's
// PipeBurn and bangIn; docs/models/exhaust.md, "Bangs: heat released in the pipe"). The burns are handed in here as
// the facade hands the ECU account's, synthetically: the account's side (sim/ecu/overrun.fm) waits for its seam.
#include "Rig.h"
#include "sim/Twin.h"

#include <tee/EnginePlayer.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>

using namespace tee::test::exhaust;

namespace
{
    // The V8's builder (layout 12) running at 3000 rpm, bangs burned in the pipe, a slug as a spark-cut charge
    // leaves a 0.6 l cylinder near full load (0.65 g, 700 K, phi 1.15, a tenth burned gas)
    void slug(Rig& rig, double mass = 6.5e-4, double t = 700.0, double phi = 1.15, double xb = 0.1, int own = 0)
    {
        rig.set("bgMass", mass);
        rig.set("bgT", t);
        rig.set("bgPhi", phi);
        rig.set("bgXb", xb);
        rig.set("bgOwn", own);
    }
} // namespace

TEST_CASE("a burn in the pipe releases its heat whole, and nothing without one", "[exhaust][bang]")
{
    std::vector<float> l0, r0, l1, r1;
    {
        Rig rig(12);
        rig.noise(0.0);
        rig.configure();
        std::vector<float> q = pulses(rig, 3000.0, 0.6, 0.15, 0.01);
        rig.run(q, l0, r0);
    }
    Rig rig(12);
    rig.noise(0.0);
    rig.set("spec.bang", 1);
    rig.configure();
    std::vector<float> q = pulses(rig, 3000.0, 0.6, 0.15, 0.01);
    rig.run(q, l1, r1);
    // no burn handed in: the junction's source is the pulse's alone, so the sound is the carved setting's, to the bit
    CHECK(rig.get("b.bangOn") == 1.0);
    bool same = l0.size() == l1.size();
    for (std::size_t i = 0; same && i < l0.size(); ++i)
        same = l0[i] == l1[i] && r0[i] == r1[i];
    CHECK(same);
    CHECK(rig.get("b.bangPeak") == 0.0);

    // a burn of 1 kJ: what it releases plus what the gas carried out unburned is what it was given (the Wiebe curve
    // released by differences: the float's rounding)
    slug(rig);
    rig.call("XTest::bangGo", 0.0f, 1000.0f);
    std::vector<float> l2, r2;
    std::vector<float> q2 = pulses(rig, 3000.0, 0.6, 0.15, 0.01);
    rig.run(q2, l2, r2);
    const double given = rig.get("b.bangGiven");
    const double released = rig.get("b.burn[0].released");
    const double swept = rig.get("b.bangSwept");
    const double tau = rig.get("b.bangTau");
    std::printf("a 1 kJ slug (0.65 g, 700 K, phi 1.15): S_L %.2f m/s, S_T %.2f m/s, %.2f m long, burned in %.1f ms; "
                "released %.2f J, swept out %.2f J\n", rig.get("b.bangSL"), rig.get("b.bangST"), rig.get("b.bangLen"),
                tau * 1e3, released, swept);
    CHECK(given == 1000.0);
    CHECK(std::fabs(released + swept - given) <= 1e-4 * given);
    CHECK(released > 0.0);
    CHECK(rig.get("b.burn[0].dropped") == 0.0);
    // the peak: the Wiebe curve's steepest (2.02 of the mean rate, at 0.51 of its time) on the heat that burned
    const double bound = 2.03 * released / tau;
    std::printf("its peak release %.0f kW against the curve's own bound %.0f kW\n", rig.get("b.bangPeak") / 1e3, bound / 1e3);
    CHECK(rig.get("b.bangPeak") <= bound);
    CHECK(rig.get("b.bangPeak") >= 0.9 * bound);
    bool finite = true;
    for (std::size_t i = 0; i < l2.size(); ++i)
        finite = finite && std::isfinite(l2[i]) && std::isfinite(r2[i]);
    CHECK(finite);
}

TEST_CASE("a slug outside petrol's flammable range does not burn", "[exhaust][bang]")
{
    Rig rig(12);
    rig.noise(0.0);
    rig.set("spec.bang", 1);
    rig.configure();
    std::vector<float> q = pulses(rig, 3000.0, 0.3, 0.15, 0.01);
    std::vector<float> l, r;
    rig.run(q, l, r);
    slug(rig, 6.5e-4, 700.0, 0.4);
    rig.call("XTest::bangGo", 0.0f, 1000.0f);
    rig.run(q, l, r);
    CHECK(rig.get("b.bangSwept") == 1000.0);
    CHECK(rig.get("b.burn[0].released") == 0.0);
    CHECK(rig.get("b.bangPeak") == 0.0);
}

// The burn times the flame speed gives (the report's numbers): a spark-cut charge cool and hot, a late burn's own
// flame, at idle's flow and at a high speed's
TEST_CASE("measure: the burn times of slugs in the collector", "[.][measure]")
{
    for (double rpm : {1000.0, 3000.0, 6000.0})
    {
        Rig rig(12);
        rig.noise(0.0);
        rig.set("spec.bang", 1);
        rig.configure();
        std::vector<float> q = pulses(rig, rpm, 1.0, 0.15 * rpm / 3000.0, 0.01 * rpm / 3000.0);
        std::vector<float> l, r;
        rig.run(q, l, r);
        struct Case
        {
            const char* what;
            double mass, t, phi, xb;
            int own;
        };
        for (const Case& c : {Case{"spark cut, 500 K", 6.5e-4, 500.0, 1.15, 0.1, 0}, Case{"spark cut, 700 K", 6.5e-4, 700.0, 1.15, 0.1, 0},
                              Case{"spark cut, 900 K", 6.5e-4, 900.0, 1.15, 0.1, 0}, Case{"a quarter left, own flame, 900 K", 1.6e-4, 900.0, 1.0, 0.1, 1}})
        {
            slug(rig, c.mass, c.t, c.phi, c.xb, c.own);
            rig.call("XTest::bangGo", 0.0f, 100.0f);
            std::printf("%5.0f rpm (collector Mach %.3f, %.0f K, its blowdowns' gas %.0f m/s): %-34s S_L %.2f, S_T %.2f m/s, "
                        "%.2f m: folded %.1f ms (one front %.1f ms), %.0f %% before it leaves\n", rpm, rig.get("b.mach[1]"),
                        rig.get("b.heat.tGas[1]"), rig.get("b.uHold[0]"), c.what, rig.get("b.bangSL"), rig.get("b.bangST"),
                        rig.get("b.bangLen"), rig.get("b.bangTau") * 1e3, rig.get("b.bangTauFront") * 1e3,
                        rig.get("b.bangShare") * 100.0);
        }
    }
}

// The engine's own collector at full throttle and on its limiter (first.json, the rally five): the gas's speed its
// blowdowns drive (uHold), its temperature and pressure, and the burn time those give a spark-cut charge at 700 K
// (burnTime's arithmetic, repeated here: the account's slugs wait for its seam)
TEST_CASE("measure: the engine's collector, and a cut charge's burn time there", "[.][measure]")
{
    auto player = tee::test::enginePlayer(48000.0);
    for (const char* name : {"first", "archetypes/inline-five-turbo-rally"})
    {
        player->reset(48000.0);
        tee::applyBuild(*player, tee::test::engineBuild(*player, name));
        {
            const int i = player->specIndex("exhaust.bang");
            REQUIRE(i >= 0);
            player->setSpec(static_cast<std::size_t>(i), 1.0, 0);
        }
        auto rd = [&](const std::string& path) {
            double v = 0.0;
            std::string error;
            INFO(path << ": " << error);
            REQUIRE(player->read(path, v, error));
            return v;
        };
        std::vector<float> l(512), r(512);
        auto run = [&](double s) {
            for (int b = 0; b < static_cast<int>(s * 48000.0 / 512.0 + 0.5); ++b)
                player->process(l.data(), r.data(), 512);
        };
        player->controls().mode = tee::EnginePlayer::kModeBench;
        player->controls().ignition = tee::EnginePlayer::kIgnitionStart;
        run(1.5);
        player->controls().ignition = tee::EnginePlayer::kIgnitionOn;
        run(0.5);
        const int sct = rd("exh.b.heat.on[1]") != 0.0 ? 1 : (rd("exh.b.heat.on[2]") != 0.0 ? 2 : 0);
        for (double thr : {0.0, 1.0})
        {
            player->controls().throttle = static_cast<float>(thr);
            for (int k = 0; k < (thr > 0.0 ? 6 : 1); ++k)
            {
                run(thr > 0.0 ? 0.4 : 1.0);
                const double u = std::max(rd("exh.b.uHold[0]"), rd("exh.b.uHold[1]"));
                const double tg = rd("exh.b.heat.tGas[" + std::to_string(sct) + "]");
                const double dia = rd("exh.b.heat.dia[" + std::to_string(sct) + "]");
                const double br = std::max(rd("exh.b.heat.br[" + std::to_string(sct) + "]"), 1.0);
                const double p = 101325.0 + rd("exh.b.pTurb");
                const double m = rd("cyl[0].m");
                const double tu = 700.0, phi = 1.15, xb = 0.1;
                const double sl = (0.2632 - 0.8472 * (phi - 1.13) * (phi - 1.13)) * std::pow(tu / 298.0, 2.18 - 0.8 * (phi - 1.0))
                                  * std::pow(p / 101325.0, -0.16 + 0.22 * (phi - 1.0)) * (1.0 - 2.1 * xb);
                const double st = sl + 0.1 * u;
                const double len = m * 287.0 * tu / p / (0.7853982 * dia * dia * br);
                const double front = len / st;
                const double fold = std::min(len / std::max(u, 1.0) + 0.5 * dia / st, front);
                std::printf("%-36s throttle %.0f, %5.0f rpm: collector %.0f K, %.0f kPa, bore %.0f mm x %.0f; its blowdowns' gas "
                            "%.0f m/s; a cut charge of %.2f g at 700 K: S_T %.1f m/s, %.2f m, folded %.1f ms, one front %.1f ms\n",
                            name, thr, rd("crank.rpm"), tg, p / 1000.0, dia * 1000.0, br, u, m * 1000.0, st, len, fold * 1e3,
                            front * 1e3);
            }
        }
    }
}

// Each bang the account hands over on a spark-cut limiter (exhaust.bang 1, the V8 archetype): its slug and what the
// exhaust made of it
TEST_CASE("measure: the bangs of a spark-cut limiter, one by one", "[.][measure]")
{
    auto player = tee::test::enginePlayer(48000.0);
    tee::applyBuild(*player, tee::test::engineBuild(*player, "archetypes/flat-plane-v8-45"));
    auto rd = [&](const std::string& path) {
        double v = 0.0;
        std::string error;
        INFO(path << ": " << error);
        REQUIRE(player->read(path, v, error));
        return v;
    };
    auto set = [&](const std::string& path, double v) {
        const int i = player->specIndex(path);
        REQUIRE(i >= 0);
        player->setSpec(static_cast<std::size_t>(i), v, 0);
    };
    set("exhaust.bang", 1);
    set("ecu.limiter.kind", 1);
    std::vector<float> l(48), r(48);
    auto run = [&](double s) {
        for (int b = 0; b < static_cast<int>(s * 1000.0); ++b)
            player->process(l.data(), r.data(), 48);
    };
    player->controls().mode = tee::EnginePlayer::kModeBench;
    player->controls().ignition = tee::EnginePlayer::kIgnitionStart;
    run(1.5);
    player->controls().ignition = tee::EnginePlayer::kIgnitionOn;
    run(0.5);
    player->controls().throttle = 1.0f;
    run(1.0);
    double given = rd("exh.b.bangGiven");
    int shown = 0;
    for (int k = 0; k < 2000 && shown < 25; ++k)
    {
        run(0.001);
        const double g = rd("exh.b.bangGiven");
        if (g != given)
        {
            std::printf("bang: %.0f J given, %.0f J burned (share %.2f); S_L %.2f, S_T %.1f m/s, slug %.3f m, gas %.0f m/s, "
                        "burn %.2f ms (one front %.2f ms); rpm %.0f\n", g - given, rd("exh.b.bangHeat"), rd("exh.b.bangShare"),
                        rd("exh.b.bangSL"), rd("exh.b.bangST"), rd("exh.b.bangLen"),
                        std::max(rd("exh.b.uHold[0]"), rd("exh.b.uHold[1]")), rd("exh.b.bangTau") * 1e3,
                        rd("exh.b.bangTauFront") * 1e3, rd("crank.rpm"));
            given = g;
            shown += 1;
        }
    }
}

namespace
{
    // An engine of `build`, Bangs burned in the pipe, on the bench flat out for `seconds` on its limiter (kind: -1 the
    // build's own): the account's fuel handed to the pipe (kg) and the exhaust's heat given, released and its peak
    struct PipeBooks
    {
        double handed = 0.0, given = 0.0, released = 0.0, peak = 0.0, peakOut = 0.0;
    };
    PipeBooks limiterBooks(const char* build, int kind, double seconds)
    {
        static tee::EnginePlayer* player = tee::test::enginePlayer(48000.0).release();
        player->reset(48000.0);
        tee::applyBuild(*player, tee::test::engineBuild(*player, build));
        auto set = [&](const std::string& path, double v) {
            const int i = player->specIndex(path);
            REQUIRE(i >= 0);
            player->setSpec(static_cast<std::size_t>(i), v, 0);
        };
        auto rd = [&](const std::string& path) {
            double v = 0.0;
            std::string error;
            INFO(path << ": " << error);
            REQUIRE(player->read(path, v, error));
            return v;
        };
        set("exhaust.bang", 1);
        if (kind >= 0)
            set("ecu.limiter.kind", kind);
        std::vector<float> l(512), r(512);
        double peak = 0.0;
        auto run = [&](double s) {
            for (int b = 0; b < static_cast<int>(s * 48000.0 / 512.0 + 0.5); ++b)
            {
                player->process(l.data(), r.data(), 512);
                for (int i = 0; i < 512; ++i)
                    peak = std::max(peak, static_cast<double>(std::max(std::fabs(l[i]), std::fabs(r[i]))));
            }
        };
        player->controls().mode = tee::EnginePlayer::kModeBench;
        player->controls().ignition = tee::EnginePlayer::kIgnitionStart;
        run(1.5);
        player->controls().ignition = tee::EnginePlayer::kIgnitionOn;
        run(0.5);
        player->controls().throttle = 1.0f;
        const double h0 = rd("ecu.one.account.total"), g0 = rd("exh.b.bangGiven"), r0 = rd("exh.b.burn[0].released") + rd("exh.b.burn[1].released");
        peak = 0.0;
        run(seconds);
        PipeBooks b;
        b.handed = rd("ecu.one.account.total") - h0;
        b.given = rd("exh.b.bangGiven") - g0;
        b.released = rd("exh.b.burn[0].released") + rd("exh.b.burn[1].released") - r0;
        b.peak = rd("exh.b.bangPeak");
        b.peakOut = peak;
        return b;
    }
} // namespace

// The account and the exhaust keep one book: the heat the exhaust is given is the fuel the account hands over times
// the fuel's lower heating value (premium petrol's 42.2 MJ/kg, fuels.fm), to the float's rounding of the sums; and a
// spark cut's premixed charges bang where a fuel-cut limiter's rich gas and cut air make none (docs/models/exhaust.md)
TEST_CASE("the bangs' books: the account's fuel is the exhaust's heat; a spark cut bangs, a fuel cut does not", "[exhaust][engine][bang]")
{
    const PipeBooks spark = limiterBooks("archetypes/flat-plane-v8-45", 1, 1.5);
    std::printf("spark-cut limiter (the V8), 1.5 s: %.1f g handed, %.0f kJ given (%.4f MJ/kg), %.0f kJ released; the largest "
                "release %.0f kW, the output's peak %.3f\n", spark.handed * 1e3, spark.given / 1e3,
                spark.handed > 0.0 ? spark.given / spark.handed / 1e6 : 0.0, spark.released / 1e3, spark.peak / 1e3, spark.peakOut);
    REQUIRE(spark.handed > 0.0);
    CHECK(std::fabs(spark.given / spark.handed - 42.2e6) <= 1e-4 * 42.2e6);
    CHECK(spark.released > 0.0);
    CHECK(spark.released <= spark.given * (1.0 + 1e-4));
    CHECK(spark.peakOut <= 1.0);
    const PipeBooks fuel = limiterBooks("first", -1, 1.5);
    std::printf("fuel-cut limiter (first.json), 1.5 s: %.3f g handed, %.0f J given, peak release %.0f W, the output's peak %.3f\n",
                fuel.handed * 1e3, fuel.given, fuel.peak, fuel.peakOut);
    CHECK(fuel.handed == 0.0);
    CHECK(fuel.given == 0.0);
    CHECK(fuel.peak == 0.0);
    CHECK(spark.peak > fuel.peak);
}
