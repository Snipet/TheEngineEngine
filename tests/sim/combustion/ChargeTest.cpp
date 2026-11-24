// tests/sim/combustion/ChargeTest.cpp: fuel.model 1's charge (charge1.fm): what each firing the ECU decides burns and
// what it sends to the pipe.
#include "sim/ecu/FmHarness.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using tee::test::h::Fm;

namespace
{
    enum
    {
        BURN,
        FUEL_CUT,
        SPARK_CUT,
        OVERRUN
    };
    const float kAir = 6.0e-4f;          // a half-litre cylinder's air at 1 atm
} // namespace

TEST_CASE("H: model 1's charge: burns, cuts, misfires and the overrun's late fuel", "[h][combustion][charge]")
{
    Fm fm = Fm::load("combustion/charge.fm", "ChargeRig");
    // a stoichiometric burn of regular: the heat of the air at its row's energy and the walls' share, nothing left of
    // the fuel, nothing of the air
    const double q = fm.call<float>("ChargeRig::make", 0, BURN, kAir, 1.0f, -20.0f, 0.5f);
    CHECK(q == Catch::Approx(kAir * 42.2e6 / 14.0 * 0.8).epsilon(1e-5));
    CHECK(fm.get("c.outFuel") == Catch::Approx(0.0).margin(1e-12));
    CHECK(fm.get("c.outAir") == Catch::Approx(0.0).margin(1e-9));
    CHECK(fm.get("c.fuel") == Catch::Approx(kAir / 14.0).epsilon(1e-5));
    // rich: fuel left over; lean: air left over
    fm.call<float>("ChargeRig::make", 0, BURN, kAir, 0.85f, -20.0f, 0.5f);
    CHECK(fm.get("c.outFuel") > 1.0e-6);
    fm.call<float>("ChargeRig::make", 0, BURN, kAir, 1.2f, -20.0f, 0.5f);
    CHECK(fm.get("c.outAir") == Catch::Approx(kAir * (1.0 - 1.0 / 1.2)).epsilon(1e-4));
    // a fuel cut: no heat, no fuel, its air out; a spark cut: no heat, its whole charge out
    CHECK(fm.call<float>("ChargeRig::make", 0, FUEL_CUT, kAir, 1.0f, -20.0f, 0.5f) == 0.0f);
    CHECK(fm.get("c.outFuel") == 0.0);
    CHECK(fm.get("c.outAir") == Catch::Approx(kAir));
    CHECK(fm.get("c.lambdaOut") == 10.0);
    CHECK(fm.call<float>("ChargeRig::make", 0, SPARK_CUT, kAir, 1.0f, -20.0f, 0.5f) == 0.0f);
    CHECK(fm.get("c.outFuel") == Catch::Approx(kAir / 14.0).epsilon(1e-5));
    CHECK(fm.get("c.outAir") == Catch::Approx(kAir));
    // past the lean limit it misfires (u under the chance), its charge out
    CHECK(fm.call<float>("ChargeRig::make", 0, BURN, kAir, 1.8f, -20.0f, 0.5f) == 0.0f);
    CHECK(fm.get("c.outFuel") > 0.0);
    // an overrun firing (a stoichiometric charge whatever lambda the map asks): burned before the top in the cylinder,
    // 30 degrees after the top half of it, 60 after none
    const double early = fm.call<float>("ChargeRig::make", 0, OVERRUN, kAir, 0.85f, -10.0f, 0.5f);
    CHECK(fm.get("c.outFuel") == Catch::Approx(0.0).margin(1e-12));
    const double late = fm.call<float>("ChargeRig::make", 0, OVERRUN, kAir, 0.85f, 30.0f, 0.5f);
    CHECK(late == Catch::Approx(0.5 * early).epsilon(1e-4));
    CHECK(fm.get("c.outFuel") == Catch::Approx(0.5 * kAir / 14.0).epsilon(1e-3));
    CHECK(fm.call<float>("ChargeRig::make", 0, OVERRUN, kAir, 0.85f, 60.0f, 0.5f) == 0.0f);
    CHECK(fm.get("c.outFuel") == Catch::Approx(kAir / 14.0).epsilon(1e-4));
    // methanol cools its end gas far more than regular
    fm.call<float>("ChargeRig::make", 5, BURN, kAir, 1.0f, -20.0f, 0.5f);
    const double methanol = fm.get("c.tEnd");
    fm.call<float>("ChargeRig::make", 0, BURN, kAir, 1.0f, -20.0f, 0.5f);
    CHECK(methanol < fm.get("c.tEnd") - 15.0);
}
