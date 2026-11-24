// tests/sim/calibration/balance/BalanceTest.cpp: what card N3's balance holds (one compile a process: the cases share
// one kept player, reset per measurement to a fresh one's state; tags [sim], one ctest entry).
//
// 1. The air's books close within 1 %: through the throttle, in through the intake valves net, trapped fresh at intake
//    closing, out through the exhaust valves, and the air the fuel was metered for, on three NA engines of three head
//    classes at full load. (Measured 24 Nov, 4bf0fe0: within 0.3 % on all seven over-delivering engines; the
//    disagreement of the two VEs the lead read is a definition: tel veManifold is taken at the charge after the
//    intake's heating, core/work.fm.)
// 2. The first law closes within 1 % of the fuel's heat (the probe's own check: released - walls - work - the net
//    enthalpy through the valves - the gas's change).
// 3. Gross indicated fuel-conversion efficiency at full load, three speeds of the small block, inside 0.33 to 0.38:
//    Heywood, Internal Combustion Engine Fundamentals, §5.7 and §15 (a real engine near 0.8 of the fuel-air cycle;
//    full-load brake efficiency 0.28 to 0.33, BSFC 260 to 300 g/kWh, plus friction 0.04 to 0.06 of the fuel).
//    It was 0.374 to 0.433 on all sixteen NA petrol archetypes until card D took Gatowski's gamma for the physical
//    cylinder (finding N3-1, 3990ee47 and a845b07f); 0.345 to 0.358 since.
#include "Balance.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>

using namespace tee::balance;

namespace
{
    struct Keep
    {
        Keep() { keepPlayer(true); }
        ~Keep() { keepPlayer(false); }
    };

    Books run(const std::string& slug, double rpm)
    {
        Options o;
        o.rpm = rpm;
        Books b;
        std::string error;
        const bool ok = measure(slug, o, b, error);
        INFO(slug << " at " << rpm << " rpm: " << error);
        REQUIRE(ok);
        return b;
    }
} // namespace

TEST_CASE("N3 balance: the air's and the energy's books close at full load", "[n3][balance][sim]")
{
    Keep keep;
    // a two-valve pushrod, a two-valve overhead cam, a four-valve: each at its target's peak-torque speed
    for (const auto& [slug, rpm] : {std::pair<const char*, double>{"small-block-57-carb", 3000.0}, {"vr6-28", 4200.0},
                                    {"twin-cam-roadster-18", 5000.0}})
    {
        const Books b = run(slug, rpm);
        const double thr = b.airThrottle;
        std::printf("%s %.0f rpm: throttle %.4g, valves %.4g, trapped fresh %.4g, exhaust %.4g, metered %.4g kg; "
                    "first law %.4f of the fuel\n",
                    slug, b.rpm, thr, b.airValves, b.trappedFresh, -b.gasExhaust, b.fuelAir, b.firstLaw() / b.fuelEnergy);
        INFO(slug);
        REQUIRE(thr > 0.0);
        CHECK(std::fabs(b.airValves / thr - 1.0) < 0.01);
        CHECK(std::fabs(b.trappedFresh / thr - 1.0) < 0.01);
        CHECK(std::fabs(-b.gasExhaust / thr - 1.0) < 0.01);
        CHECK(std::fabs(b.fuelAir / thr - 1.0) < 0.01);
        CHECK(std::fabs(b.firstLaw() / b.fuelEnergy) < 0.01);
    }
}

TEST_CASE("N3 balance: gross indicated efficiency at full load is a real engine's", "[n3][balance][sim]")
{
    Keep keep;
    for (const double rpm : {2000.0, 3000.0, 4600.0})
    {
        const Books b = run("small-block-57-carb", rpm);
        const double eta = b.workGross / b.fuelEnergy;
        std::printf("small block %.0f rpm: gross indicated efficiency %.3f (0.33 to 0.38)\n", b.rpm, eta);
        CHECK(eta > 0.33);
        CHECK(eta < 0.38);
    }
}
