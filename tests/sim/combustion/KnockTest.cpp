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
    CHECK(gasDi > 1.5 * gasPort);
    CHECK(methDi > 2.0 * gasDi);
    // (a direct injector's: Kasseris and Heywood's measured 14 K for gasoline, taken at lambda 1: mixture.fm)
    CHECK(gasDi == Catch::Approx(14.0).epsilon(0.01));
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

// The 93 grade (card N1, 25 Nov: the owner's "Fuel controls for octane"): appended last, RON 98 / MON 88 against the
// 91's 96 / 86. At the trace's knock edge it takes more advance than the 91, by Douaud and Eyzat's own exponent and the
// table's own rows (the octane read from fuels.fm, not typed here), and less than the race fuel's; its other rows are
// the 91's
TEST_CASE("H2: the 93 grade takes more advance than the 91 at the knock edge, less than race fuel", "[h][combustion][knock]")
{
    Fm fm = trace();
    const int premium = 2, race = 3, premium93 = 9;
    REQUIRE(fm.call<float>("Trace::row", premium93, 3) > fm.call<float>("Trace::row", premium, 3));
    for (int what = 0; what < 3; ++what)
        CHECK(fm.call<float>("Trace::row", premium93, what) == fm.call<float>("Trace::row", premium, what));
    auto edge = [&](int kind) {
        fm.call<void>("Trace::defaults");
        fm.set("octane", static_cast<double>(fm.call<float>("Trace::row", kind, 3)));
        return static_cast<double>(fm.call<float>("Trace::limitAdvance"));
    };
    const double a91 = edge(premium), a93 = edge(premium93), aRace = edge(race);
    WARN("knock-limited advance at the trace's case: 91 " << a91 << ", 93 " << a93 << ", race " << aRace);
    CHECK(a93 > a91);
    CHECK(a93 < aRace);
}

// The free octane number (spec.fuel.octane, the pump's AKI; the owner's ask, the lead's design, card N1 25 Nov): 0
// is the grade's own to the bit; the grade's own AKI stated is its own RON; more octane takes more advance at the knock
// edge, monotonically; a number past Douaud and Eyzat's fit (RON 80 to 100) is accepted, not clamped, and said
TEST_CASE("H2: a stated octane: 0 and the grade's own AKI are the grade's, more takes more advance, past the fit is said",
          "[h][combustion][knock]")
{
    Fm fm = trace();
    for (int kind = 0; kind < 10; ++kind)
    {
        if (kind == 6)
            continue;   // (the diesel ignores it)
        INFO("grade " << kind);
        const float own = fm.call<float>("Trace::row", kind, 3);
        CHECK(fm.call<float>("Trace::octaneAt", kind, 0.0f) == own);   // 0: the grade's own, to the bit
        // its own AKI stated: its own RON (the table's RON and MON are whole numbers, so exactly)
        CHECK(fm.call<int>("Trace::octanePast", kind, 0.0f) == 0);
    }
    // the 91 (RON 96, MON 86): its AKI 91 is its own index, 93 is the 93's (98), 87 the regular's (91)... by sensitivity
    CHECK(fm.call<float>("Trace::octaneAt", 2, 91.0f) == fm.call<float>("Trace::row", 2, 3));
    CHECK(fm.call<float>("Trace::octaneAt", 9, 93.0f) == fm.call<float>("Trace::row", 9, 3));
    CHECK(fm.call<float>("Trace::octaneAt", 0, 87.0f) == fm.call<float>("Trace::row", 0, 3));
    CHECK(fm.call<float>("Trace::octaneAt", 6, 95.0f) == fm.call<float>("Trace::row", 6, 3));   // the diesel ignores it
    // more octane, more advance at the knock edge, monotonically (the 91's grade, AKI 85 to 105)
    double last = -1.0e9;
    for (float aki = 85.0f; aki <= 105.0f; aki += 2.5f)
    {
        fm.call<void>("Trace::defaults");
        fm.set("octane", static_cast<double>(fm.call<float>("Trace::octaneAt", 2, aki)));
        const double a = fm.call<float>("Trace::limitAdvance");
        INFO("AKI " << aki << ": knock-limited advance " << a);
        CHECK(a > last);
        last = a;
    }
    // past the fit: accepted (not clamped: its index still rises with it) and said; inside it not
    CHECK(fm.call<int>("Trace::octanePast", 2, 91.0f) == 0);
    CHECK(fm.call<int>("Trace::octanePast", 2, 100.0f) == 1);   // RON 105
    CHECK(fm.call<int>("Trace::octanePast", 2, 70.0f) == 1);    // RON 75
    CHECK(fm.call<float>("Trace::octaneAt", 2, 110.0f) > fm.call<float>("Trace::octaneAt", 2, 100.0f));
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
