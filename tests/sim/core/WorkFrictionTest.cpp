// tests/sim/core/WorkFrictionTest.cpp: the work integral against an ideal cycle's closed form, and the friction model's
// parts and range (card D's test 7 needs the whole engine; here the model alone, at the reference V8's size).
#include "Presets.h"

static constexpr double kPi = 3.14159265358979323846;

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>

using namespace tee::test::core;

TEST_CASE("the work integral of an ideal Otto cycle is its closed form, split into its loops", "[core][work]")
{
    Rig rig;
    applyLayout(rig, loadPreset("ref-05-inline-4"));
    const double bore = 0.086, stroke = 0.086, rod = 0.143, cr = 10.0;
    rig.build(bore, stroke, rod, cr, 0.12);
    const double pI = 0.6e5, pE = 1.1e5, rp = 3.0, k = 1.3;
    auto otto = rig.method<void(void*, int, int, float, float, float, float)>("ottoRun");
    otto(rig.data(), 3, 7200, float(pI), float(pE), float(rp), float(k));
    const double vd = kPi / 4 * bore * bore * stroke, vc = vd / (cr - 1), vm = vc + vd;
    const double p2 = pI * std::pow(vm / vc, k), p3 = rp * p2, p4 = p3 * std::pow(vc / vm, k);
    const double wGross = (p2 * vc - pI * vm) / (1 - k) + (p4 * vm - p3 * vc) / (1 - k);
    const double imep = wGross / vd, pmep = pE - pI;
    const double gotImep = rig.get("wk.w[0].imep"), gotPmep = rig.get("wk.w[0].pmep");
    std::printf("Otto cycle: IMEP gross %.4f bar (closed form %.4f), PMEP %.4f bar (%.4f), peak %.2f bar at %.1f deg; %d cycles\n",
                gotImep / 1e5, imep / 1e5, gotPmep / 1e5, pmep / 1e5, rig.get("wk.w[0].peak") / 1e5,
                rig.get("wk.w[0].peakAt"), int(rig.get("wk.w[0].cycles")));
    CHECK(int(rig.get("wk.w[0].cycles")) == 2);
    CHECK(std::abs(gotImep - imep) < 0.002 * imep);
    CHECK(std::abs(gotPmep - pmep) < 0.002 * pmep);
    CHECK(std::abs(rig.get("wk.w[0].peak") - p3) < 1e-3 * p3);
    CHECK(rig.get("wk.w[0].peakAt") < 0.2);
}

TEST_CASE("the friction model's parts sum to its total, within 0.8 to 2.5 bar from idle to the redline", "[core][friction]")
{
    // the reference's V8: 0.72 L a cylinder, square (97.1 mm), its redline 6500 rpm
    Rig rig;
    applyLayout(rig, loadPreset("ref-12-v8-cross"));
    rig.build(0.0971, 0.0971, 0.0971 / 2 * 3.3, 10.0, 0.24);
    auto fric = rig.method<float(void*, float, float, float)>("friction");
    for (double rpm : {700.0, 2000.0, 4000.0, 6500.0})
    {
        const double w = rpm * kPi / 30.0;
        const double pk = 20e5 + 40e5 * std::min(rpm / 3000.0, 1.0);      // peak pressure: 20 bar at idle, 60 at load
        const double tq = fric(rig.data(), float(w), float(pk), 1.0f);
        const double fmep = rig.get("fr.fmep");
        const double sum = rig.get("fr.rings") + rig.get("fr.bearings") + rig.get("fr.valves") + rig.get("fr.constant") +
                           rig.get("fr.accessories");
        const double nk = rpm / 1000.0;
        const double ref = (1.0 + 0.10 * nk + 0.02 * nk * nk) * 1e5;
        std::printf("V8 at %4.0f rpm: FMEP %.2f bar (rings %.2f, bearings %.2f, valves %.2f, constant %.2f, accessories %.2f); "
                    "the reference's %.2f; %.1f N m\n", rpm, fmep / 1e5, rig.get("fr.rings") / 1e5, rig.get("fr.bearings") / 1e5,
                    rig.get("fr.valves") / 1e5, rig.get("fr.constant") / 1e5, rig.get("fr.accessories") / 1e5, ref / 1e5, tq);
        CHECK(std::abs(sum - fmep) < 1e-4 * fmep);
        CHECK(fmep >= 0.8e5);
        CHECK(fmep <= 2.5e5);
        CHECK(std::abs(fmep - ref) < 0.3 * ref);
    }
    // thick oil multiplies the rubbing, not the accessories
    fric(rig.data(), float(3000 * kPi / 30), 50e5f, 1.0f);
    const double warm = rig.get("fr.fmep"), acc = rig.get("fr.accessories");
    fric(rig.data(), float(3000 * kPi / 30), 50e5f, 2.0f);
    CHECK(std::abs(rig.get("fr.fmep") - (2.0 * (warm - acc) + acc)) < 1e-3 * warm);
}

TEST_CASE("friction by piston speed: three bores at three piston speeds", "[core][friction]")
{
    // square cylinders of 60, 86 and 104 mm (a 170 cc, a 500 cc and an 880 cc cylinder), four of them, at 10, 17 and 24 m/s
    std::printf("FMEP (bar) by bore and mean piston speed, 60 bar peak, warm oil:\n");
    for (double bore : {0.060, 0.086, 0.104})
    {
        Rig rig;
        applyLayout(rig, loadPreset("ref-05-inline-4"));
        rig.build(bore, bore, 1.65 * bore, 11.0, 0.1);
        auto fric = rig.method<float(void*, float, float, float)>("friction");
        std::printf("  %3.0f mm:", bore * 1000);
        double first = 0;
        for (double sp : {10.0, 17.0, 24.0})
        {
            const double rpm = sp * 30.0 / bore;
            fric(rig.data(), float(rpm * kPi / 30.0), 60e5f, 1.0f);
            const double f = rig.get("fr.fmep") / 1e5;
            std::printf("  %4.1f m/s (%5.0f rpm) %.2f", sp, rpm, f);
            CHECK(f > 0.8);
            CHECK(f < 3.2);
            if (sp == 24.0)
                CHECK(f < 3.0);       // a real superbike's 2.5 to 3 bar at 24 m/s
            if (first == 0)
                first = f;
        }
        std::printf("\n");
    }
}
