// tests/sim/combustion/KnockTest.cpp: card H's standalone tests 1 and 2: the fuel table's energy, the mixture, and the
// knock integral on made-up traces (trace.fm).
#include "sim/ecu/FmHarness.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <cstdio>

using tee::test::h::Fm;

namespace
{
    enum
    {
        REGULAR,
        MIDGRADE,
        PREMIUM,
        RACE,
        E85,
        METHANOL,
        DIESEL,
        PROPANE,
        HYDROGEN,
        ROWS
    };
    const char* const kNames[ROWS] = {"regular", "midgrade", "premium", "race", "E85", "methanol", "diesel", "propane", "hydrogen"};

    Fm trace()
    {
        Fm fm = Fm::load("combustion/trace.fm", "Trace");
        fm.call<void>("Trace::defaults");
        return fm;
    }

    // Douaud and Eyzat's delay, s, in closed form
    double tauDE(double octane, double pPa, double tK)
    {
        return 17.68e-3 * std::pow(octane / 100.0, 3.402) * std::pow(pPa / 101325.0, -1.7) * std::exp(3800.0 / tK);
    }
} // namespace

TEST_CASE("H1: a kilogram of air at stoichiometric releases its row's energy", "[h][combustion][fuel]")
{
    Fm fm = trace();
    for (int k = 0; k < ROWS; ++k)
    {
        const double afr = fm.call<float>("Trace::row", k, 0);
        const double lhv = fm.call<float>("Trace::row", k, 1);
        const double heat = fm.call<float>("Trace::mixture", k, 1.0f, 1.0f, 1.0f, 0.0f);
        INFO(kNames[k] << ": " << heat / 1e6 << " MJ a kg of air");
        CHECK(heat == Catch::Approx(lhv / afr).epsilon(1e-5));
        if (k != HYDROGEN)
        {
            CHECK(heat > 2.9e6);
            CHECK(heat < 3.1e6);
        }
        else
            CHECK(heat == Catch::Approx(3.50e6).epsilon(0.01));   // hydrogen's own: 120 / 34.3 MJ (untested row)
    }
}

TEST_CASE("H1: the mixture: lean is fuel-limited, rich oxygen-limited with its top a little rich", "[h][combustion][fuel]")
{
    Fm fm = trace();
    const double stoich = fm.call<float>("Trace::mixture", REGULAR, 1.0f, 1.0f, 1.0f, 0.0f);
    // lean: the heat goes as the fuel, nothing unburned
    const double lean = fm.call<float>("Trace::mixture", REGULAR, 1.2f, 1.0f, 1.0f, 0.0f);
    CHECK(lean == Catch::Approx(stoich / 1.2).epsilon(1e-5));
    CHECK(fm.get("mix.unburned") == 0.0);
    CHECK(fm.get("mix.misfire") == 0.0);
    // rich: more heat to a top near the fastest flame (phi 1.12: lambda 0.89), and fuel left unburned
    double best = 0.0;
    double bestLambda = 0.0;
    for (int i = 0; i <= 40; ++i)
    {
        const float lambda = 0.70f + 0.0075f * static_cast<float>(i);
        const double h = fm.call<float>("Trace::mixture", REGULAR, lambda, 1.0f, 1.0f, 0.0f);
        if (h > best)
        {
            best = h;
            bestLambda = lambda;
        }
    }
    CHECK(bestLambda > 0.86);
    CHECK(bestLambda < 0.92);
    CHECK(best / stoich == Catch::Approx(1.06).epsilon(0.01));
    fm.call<float>("Trace::mixture", REGULAR, 0.85f, 1.0f, 1.0f, 0.0f);
    CHECK(fm.get("mix.unburned") > 0.005);
    // the flame: fastest a little rich
    fm.call<float>("Trace::mixture", REGULAR, 0.89f, 1.0f, 1.0f, 0.0f);
    const double fast = fm.get("mix.burnK");
    fm.call<float>("Trace::mixture", REGULAR, 1.3f, 1.0f, 1.0f, 0.0f);
    CHECK(fm.get("mix.burnK") > fast * 1.1);
    // past the limits a spark no longer lights it
    fm.call<float>("Trace::mixture", REGULAR, 1.70f, 1.0f, 1.0f, 0.0f);
    CHECK(fm.get("mix.misfire") == 1.0);
    fm.call<float>("Trace::mixture", REGULAR, 0.40f, 1.0f, 1.0f, 0.0f);
    CHECK(fm.get("mix.misfire") == 1.0);
    // nitrous's oxygen: twice the oxygen, twice the fuel and the heat at the same lambda
    const double nitrous = fm.call<float>("Trace::mixture", REGULAR, 1.0f, 1.0f, 2.0f, 0.0f);
    CHECK(nitrous == Catch::Approx(2.0 * stoich).epsilon(1e-5));
    // evaporation: methanol cools far more than gasoline; a direct injector's in the cylinder
    fm.call<float>("Trace::mixture", REGULAR, 1.0f, 1.0f, 1.0f, 1.0f);
    const double gasDi = fm.get("mix.coolCyl");
    fm.call<float>("Trace::mixture", REGULAR, 1.0f, 1.0f, 1.0f, 0.0f);
    const double gasPort = fm.get("mix.coolCyl");
    fm.call<float>("Trace::mixture", METHANOL, 1.0f, 1.0f, 1.0f, 1.0f);
    const double methDi = fm.get("mix.coolCyl");
    CHECK(gasDi > 2.0 * gasPort);
    CHECK(methDi > 2.0 * gasDi);
    CHECK(gasDi == Catch::Approx(402.0e3 / 14.0 / 1005.0 * 0.85).epsilon(1e-4));
}

TEST_CASE("H2: the knock integral's tables give Douaud and Eyzat's delay", "[h][combustion][knock]")
{
    Fm fm = trace();
    fm.set("octane", 91.0);
    double worst = 0.0;
    for (double atm = 5.0; atm <= 150.0; atm += 7.0)
        for (double t = 550.0; t <= 1300.0; t += 23.0)
        {
            const double tables = fm.call<float>("Trace::rate", static_cast<float>(atm * 101325.0), static_cast<float>(t));
            const double exact = 1.0 / tauDE(91.0, atm * 101325.0, t);
            worst = std::fmax(worst, std::fabs(tables / exact - 1.0));
        }
    INFO("worst relative error " << worst);
    CHECK(worst < 0.01);
    // held at 40 atm and 800 K for one delay, the integral is 1 (within the tables' error)
    fm.set("octane", 90.0);
    const double tau = tauDE(90.0, 40.0 * 101325.0, 800.0);
    const double sum = fm.call<float>("Trace::hold", static_cast<float>(40.0 * 101325.0), 800.0f, static_cast<float>(tau), 64);
    CHECK(sum == Catch::Approx(1.0).epsilon(0.01));
    // that delay against the literature's: a primary reference fuel of 90 near 40 bar and 800 K lights in a few
    // milliseconds in shock tubes (Fieweger, Blumenthal and Adomeit, Combust. Flame 109, 1997: some 1 to 3 ms there,
    // read from their figures); Douaud and Eyzat's fit gives 2.7 ms
    WARN("tau(PRF 90, 40 atm, 800 K) = " << tau * 1000.0 << " ms");
    CHECK(tau > 1.0e-3);
    CHECK(tau < 5.0e-3);
}

TEST_CASE("H2: more boost, more advance, hotter air and lower octane each bring the knock sooner", "[h][combustion][knock]")
{
    Fm fm = trace();
    const double base = fm.call<float>("Trace::limitAdvance");
    WARN("knock-limited advance, the base case (10:1, 2000 rpm, 1 atm and 330 K at closing, octane 91): " << base);
    auto limited = [&](const char* field, double value) {
        fm.call<void>("Trace::defaults");
        fm.set(field, value);
        const double a = fm.call<float>("Trace::limitAdvance");
        WARN(field << " = " << value << ": knock-limited advance " << a);
        return a;
    };
    CHECK(limited("pIvc", 1.3 * 101325.0) < base - 2.0);
    CHECK(limited("tIvc", 360.0) < base - 2.0);
    CHECK(limited("octane", 87.0) < base - 1.0);
    CHECK(limited("octane", 108.0) > base + 2.0);
    CHECK(limited("cr", 11.0) < base - 2.0);
    // more advance at one case: the integral is further along when the burn ends, then knocks, earlier and earlier
    fm.call<void>("Trace::defaults");
    double lastSum = 0.0;
    double lastAt = 1000.0;
    for (float adv = 0.0f; adv <= 40.0f; adv += 5.0f)
    {
        fm.set("advance", adv);
        fm.call<void>("Trace::run");
        const double at = fm.get("knockAt");
        const double sum = fm.get("sumEnd");
        INFO("advance " << adv << ": integral " << sum << ", knock at " << at);
        if (at >= 999.0)
            CHECK(sum > lastSum);
        else
            CHECK(at < lastAt);
        lastSum = sum;
        lastAt = at;
    }
    CHECK(lastAt < 999.0);
}

TEST_CASE("H2: the knock limit at a known operating point", "[h][combustion][knock]")
{
    // The case: a 10:1 naturally aspirated engine at full load and 2000 rpm on regular (octane index 91), the charge at
    // 1 atm and 330 K at the valve's closing, a 50-degree burn, Douaud and Eyzat's delay as published (delayK 1). An
    // engine like it on regular is knock-limited at full load and low speed (Heywood, 2nd ed., §9.6: spark-ignition
    // engines are commonly knock-limited there), short of its best-torque advance; a fuel of higher octane lets it
    // nearer. This trace (single zone, no gas exchange) puts the limit further short than engines show: the delay's
    // calibration is set against the simulator's own cylinder, not this trace (docs/models/combustion.md).
    Fm fm = trace();
    const double best = fm.call<float>("Trace::bestAdvance");
    const double a50 = fm.get("a50");
    const double limit = fm.call<float>("Trace::limitAdvance");
    fm.set("octane", 108.0);
    const double limitE85 = fm.call<float>("Trace::limitAdvance");
    WARN("best-torque advance " << best << " (half burned " << a50 << " after the top); knock-limited " << limit
                                << " on regular, " << limitE85 << " at octane 108");
    CHECK(best > 15.0);
    CHECK(best < 35.0);
    CHECK(limit > 0.0);
    CHECK(limit < best);
    CHECK(limitE85 > limit + 5.0);
}

TEST_CASE("H2: what a tick of the end gas's integral costs", "[h][combustion][knock][cost]")
{
    Fm fm = trace();
    const int n = 4000000;
    fm.call<float>("Trace::cost", 1000);
    const auto t0 = std::chrono::steady_clock::now();
    const float s = fm.call<float>("Trace::cost", n);
    const double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count() / n;
    WARN("one tick: " << ns << " ns (checksum " << s << ")");
    CHECK(std::isfinite(s));
    // (a report, not a limit: this machine's cores are shared, and a time measured under another build's load is not
    // the integral's; 9.7 ns was measured at a load of about 3)
}
