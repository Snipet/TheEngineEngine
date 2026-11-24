// tests/sim/boost/ChargerTest.cpp: card F, standalone tests 7 and 8: superchargers (Roots, twin-screw, centrifugal)
// on the made-up engine; the intercooler against its effectiveness; nitrous's torque against its rating.
#include "Rig.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <vector>

using tee::test::boost::Rig;

namespace
{
    struct Point
    {
        double boost = 0.0;     // Pa, before the throttle, mean
        double tOut = 0.0;      // K, the charge pipe's (the blower's outlet, before the intercooler)
        double crank = 0.0;     // W the blower took from the crank, mean
        double air = 0.0;       // W of the air's work, by the textbook's formula from the measured state
    };

    // A supercharged 2 L four at a crank speed, the throttle open, after it settles
    Point charged(int charger, double rpm, double pulley, double vi = 1.5)
    {
        Rig rig;
        rig.set("spec.kind", 4);
        rig.set("spec.model", 1);
        rig.set("spec.charger", charger);
        rig.set("spec.scRatio", pulley);
        rig.set("spec.scVolumeRatio", vi);
        rig.set("spec.icEffect", 0.0);
        rig.spec("rev", 0);
        rig.set("rpm", rpm);
        rig.set("throttle", 1.0);
        rig.run(2.0);
        const double d0 = rig.get("boost.sumScDrive");
        rig.clear();
        // sampled means of the state the formulas need
        Point p;
        double n = 0.0;
        double air = 0.0;
        double tOut = 0.0;
        const double steps = 200;
        for (int i = 0; i < steps; ++i)
        {
            rig.run(0.005);
            tOut += rig.get("boost.v1.t");
            if (charger < 2)
            {
                // a positive-displacement blower: per turn, p1 D 3.5 (Vi^0.4 - 1) + (D / Vi) (p2 - p_i) under-compressed
                const double disp = rig.get("boost.pd.disp");
                const double v = rig.get("boost.pd.vi");
                const double p1 = rig.get("boost.p1");
                const double p2 = rig.get("boost.v1.p");
                const double pi = p1 * std::pow(v, 1.4);
                double w = p1 * disp * 3.5 * (std::pow(v, 0.4) - 1.0);
                if (p2 > pi)
                    w += disp / v * (p2 - pi);
                air += w * rig.get("boost.pd.n");
            }
            else
            {
                // a centrifugal: the air's enthalpy rise through it
                air += rig.get("boost.comp[0].mdot") * 1004.5 * (rig.get("boost.comp[0].t2") - rig.get("boost.comp[0].t1"));
            }
            n += 1.0;
        }
        p.boost = rig.mean("sBoost");
        p.tOut = tOut / n;
        p.crank = (rig.get("boost.sumScDrive") - d0) / (steps * 0.005);
        p.air = air / n;
        return p;
    }
} // namespace

TEST_CASE("boost: a Roots blower's boost is nearly flat with speed", "[boost][charger]")
{
    double spread[2] = {0.0, 0.0};
    for (int charger : {0, 2})
    {
        std::vector<double> b;
        for (double rpm = 2000.0; rpm <= 6500.0; rpm += 1500.0)
        {
            const Point p = charged(charger, rpm, 2.4);
            std::printf("%s at %.0f rpm: %.1f kPa, outlet %.0f K, crank %.0f W\n", charger == 0 ? "Roots" : "centrifugal",
                        rpm, p.boost / 1000.0, p.tOut, p.crank);
            b.push_back(p.boost);
        }
        double lo = b[0];
        double hi = b[0];
        for (double v : b)
        {
            lo = std::fmin(lo, v);
            hi = std::fmax(hi, v);
        }
        spread[charger == 0 ? 0 : 1] = lo / hi;
        if (charger == 0)
            CHECK(lo > 15000.0);
    }
    // from 2000 to 6500 rpm a Roots's boost keeps half of its most (its leaks cost it most at low speed: an Eaton
    // blower's published curves rise some 60 % over the same speeds); a centrifugal's least is a tenth of its most
    std::printf("least over most, 2000 to 6500 rpm: Roots %.2f, centrifugal %.2f\n", spread[0], spread[1]);
    CHECK(spread[0] > 0.5);
    CHECK(spread[0] > 3.0 * spread[1]);
}

TEST_CASE("boost: a Roots blower's outlet is hotter than a twin-screw's at the same boost", "[boost][charger]")
{
    const double rpm = 4500.0;
    const Point roots = charged(0, rpm, 3.2);
    // a twin-screw whose built-in ratio matches that boost (Vi^1.4 the pressure ratio), its pulley found so that it
    // makes the same boost (by halving)
    const double ratio = (101325.0 + roots.boost) / 101325.0;
    const double vi = std::pow(ratio, 1.0 / 1.4);
    double lo = 1.0;
    double hi = 4.0;
    Point screw;
    for (int k = 0; k < 14; ++k)
    {
        const double mid = 0.5 * (lo + hi);
        screw = charged(1, rpm, mid, vi);
        if (screw.boost < roots.boost)
            lo = mid;
        else
            hi = mid;
    }
    std::printf("at %.1f kPa: Roots %.1f K, twin-screw (Vi %.2f) %.1f K (%.1f kPa)\n", roots.boost / 1000.0, roots.tOut, vi,
                screw.tOut, screw.boost / 1000.0);
    REQUIRE(screw.boost == Catch::Approx(roots.boost).epsilon(0.02));
    CHECK(roots.tOut > screw.tOut + 5.0);
}

TEST_CASE("boost: a centrifugal supercharger's boost rises with the speed squared", "[boost][charger]")
{
    std::vector<double> rpm;
    std::vector<double> b;
    for (double r = 2500.0; r <= 6500.0; r += 1000.0)
    {
        const Point p = charged(2, r, 2.4);
        rpm.push_back(r);
        b.push_back(p.boost);
        std::printf("centrifugal at %.0f rpm: %.1f kPa, outlet %.0f K\n", r, p.boost / 1000.0, p.tOut);
    }
    // the slope of log boost against log speed, by least squares
    double sx = 0.0;
    double sy = 0.0;
    double sxx = 0.0;
    double sxy = 0.0;
    const double n = static_cast<double>(rpm.size());
    for (std::size_t i = 0; i < rpm.size(); ++i)
    {
        const double x = std::log(rpm[i]);
        const double y = std::log(b[i]);
        sx += x;
        sy += y;
        sxx += x * x;
        sxy += x * y;
    }
    const double slope = (n * sxy - sx * sy) / (n * sxx - sx * sx);
    std::printf("centrifugal: boost as the speed to the %.2f\n", slope);
    CHECK(slope > 1.7);
    CHECK(slope < 2.4);
}

TEST_CASE("boost: a supercharger's torque on the crank is the air's work over its efficiency", "[boost][charger]")
{
    for (int charger : {0, 1, 2})
    {
        const Point p = charged(charger, 5000.0, 2.4);
        const double eff = charger == 2 ? 0.95 : 0.9;
        std::printf("charger %d: crank %.1f W, the air's work %.1f W\n", charger, p.crank, p.air);
        INFO("charger " << charger);
        CHECK(p.air > 1000.0);
        CHECK(p.crank * eff == Catch::Approx(p.air).epsilon(0.03));
    }
}

TEST_CASE("boost: the intercooler's outlet against its effectiveness", "[boost][intercooler]")
{
    std::vector<double> soaked;
    for (double eff : {0.5, 0.7, 0.9})
    {
        Rig rig;
        rig.set("spec.kind", 1);
        rig.set("spec.model", 1);
        rig.set("spec.icEffect", eff);
        rig.spec("rev", 0);
        rig.set("rpm", 5000.0);
        rig.set("throttle", 1.0);
        rig.run(1.0);
        // T_out = T_in - eff (T_in - T_core), sample by sample
        const double tin = rig.get("boost.v1.t");
        const double core = rig.get("boost.tCore");
        const double out = rig.get("boost.tIcOut");
        CHECK(out == Catch::Approx(tin - eff * (tin - core)).margin(1.5));
        CHECK(tin > 320.0);
        const double early = out;
        rig.run(30.0);
        soaked.push_back(rig.get("boost.tIcOut"));
        std::printf("effectiveness %.1f: in %.1f K, out %.1f K after 1 s (core %.1f K); %.1f K after 31 s (core %.1f K)\n",
                    eff, tin, early, core, soaked.back(), rig.get("boost.tCore"));
        // the core soaks: the outlet warms as it does
        CHECK(soaked.back() > early);
    }
    CHECK(soaked[1] < soaked[0]);
    CHECK(soaked[2] < soaked[1]);
}

TEST_CASE("boost: nitrous's torque step is its rating's, within 15 %", "[boost][nitrous]")
{
    for (double rating : {37285.0, 74570.0, 111855.0})    // 50, 100 and 150 hp shots
    {
        Rig rig;
        rig.set("spec.kind", 0);
        rig.set("spec.model", 1);
        rig.set("spec.nitrousShot", rating);
        rig.spec("rev", 0);
        const double rpm = 4000.0;
        rig.set("rpm", rpm);
        rig.set("throttle", 1.0);
        rig.run(1.0);
        rig.clear();
        rig.run(0.5);
        const double dry = rig.mean("sTorque");
        rig.set("boost.cmdNitrous", 1);
        rig.run(0.5);
        rig.clear();
        rig.run(0.5);
        const double wet = rig.mean("sTorque");
        const double expected = rating / (rpm * 6.283185307 / 60.0);
        std::printf("%.0f hp shot at %.0f rpm: %.1f N m dry, %.1f wet: a step of %.1f against %.1f; the charge's oxygen "
                    "%.3f, %.1f K\n", rating / 745.7, rpm, dry, wet, wet - dry, expected, rig.get("boost.v2.o2"),
                    rig.get("boost.v2.t"));
        CHECK(wet - dry == Catch::Approx(expected).epsilon(0.15));
    }
}
