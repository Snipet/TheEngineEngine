// tests/sim/twostroke/ExploreTest.cpp: a sweep printed, for looking (tagged hidden: run with "[.explore]").
#include "Ts.h"

#include <cmath>
#include <cstdio>
#include <algorithm>
#include <cstdlib>

using tee::test::twostroke::Rig;

TEST_CASE("sweep", "[.explore]")
{
    for (int chamber : {1, 0})
    {
        Rig rig;
        rig.set("spec.chamber", chamber);
        rig.build();
        std::printf("chamber %d, tuned length %.3f m\n", chamber, rig.get("ch.tuned"));
        for (double rpm = 5000; rpm <= 16000; rpm += 1000)
        {
            rig.run(rpm, 1.0);
            std::printf("%6.0f rpm: torque %6.2f N m, trapped %.1f, fresh %.1f mg, delivered %.1f mg, purity %.3f, port %+.2f..%+.2f bar, peak %.1f bar, bad %d, misfires %d\n",
                        rpm, rig.meanTorque(), rig.get("trappedSum") / rig.get("cyc") * 1e6, rig.meanTrapped() * 1e6, rig.meanDelivered() * 1e6,
                        rig.get("puritySum") / rig.get("cyc"), rig.get("pPortMin") / 1e5, rig.get("pPortMax") / 1e5,
                        rig.get("pCylMax") / 1e5, int(rig.get("bad")), int(rig.get("cy.misfires")));
        }
    }
}

TEST_CASE("trace", "[.trace]")
{
    const char* an = std::getenv("TS_ENDLESS");
    for (double rpm : {9500.0, 12500.0, 15500.0})
    {
        Rig rig;
        rig.build();
        if (an != nullptr)
            rig.set("ch.anechoic", 1);
        rig.run(rpm, 1.0, 0.8, 5);
        std::printf("%.0f rpm: EPO %.0f EPC %.0f, TPO %.0f TPC %.0f\n", rpm, 180 - 96.0, 180 + 96.0, 180 - 65.0, 180 + 65.0);
        std::printf("fresh %.1f mg, delivered %.1f mg, purity %.3f\n", rig.meanTrapped() * 1e6, rig.meanDelivered() * 1e6,
                    rig.get("puritySum") / rig.get("cyc"));
        for (int d = 0; d < 360; d += 6)
            std::printf("%3d  port %+6.3f bar  cyl %6.2f bar  mE %+7.4f  mT %+7.4f kg/s\n", d, rig.get("trace[" + std::to_string(d) + "]") / 1e5,
                        rig.get("traceCyl[" + std::to_string(d) + "]") / 1e5, rig.get("traceE[" + std::to_string(d) + "]"),
                        rig.get("traceT[" + std::to_string(d) + "]"));
    }
}

TEST_CASE("scaled", "[.scaled]")
{
    for (double k : {0.8, 1.0, 1.25})
    {
        Rig rig;
        for (const char* f : {"spec.headerLen", "spec.diffuserLen", "spec.bellyLen", "spec.baffleLen", "spec.stingerLen"})
            rig.set(f, rig.get(f) * k);
        rig.build();
        const double L = rig.get("ch.tuned");
        std::printf("scale %.2f: tuned length %.3f m, c %.0f\n", k, L, rig.get("ch.c"));
        double best = 0, bestRpm = 0;
        for (double rpm = 6000; rpm <= 16000; rpm += 500)
        {
            rig.run(rpm, 1.0, 1.0, 50);
            const double tf = rig.meanTrapped() * 1e6;
            if (tf > best) { best = tf; bestRpm = rpm; }
            std::printf("  %6.0f: fresh %5.1f mg  torque %5.2f\n", rpm, tf, rig.meanTorque());
        }
        std::printf("  peak %.0f rpm (%.1f mg); N*L = %.0f\n", bestRpm, best, bestRpm * L);
    }
}

TEST_CASE("light", "[.light]")
{
    for (double thr : {0.0, 0.02, 0.05, 0.1})
        for (double rpm : {2000.0, 3000.0, 5000.0})
        {
            Rig rig;
            rig.build();
            rig.run(rpm, thr, 2.0, 60);
            std::printf("thr %.2f %5.0f rpm: misfires %d, purity %.2f, delivered %.1f mg, torque %.2f\n", thr, rpm,
                        int(rig.get("cy.misfires")), rig.get("puritySum") / rig.get("cyc"), rig.meanDelivered() * 1e6, rig.meanTorque());
        }
    for (double secs : {3.0, 6.0, 12.0})
    {
        Rig rig;
        rig.build();
        rig.method<void(void*, int, float, float, float, int)>("still")(rig.data(), int(Rig::kRate * secs), 180.0f, 4.0e5f, 1500.0f, 1000);
        std::printf("still %.0f s: port %.3f Pa, cyl %.3f, cc %.3f, t %.1f\n", secs, rig.get("pPortMax"), rig.get("cy.p") - 101325.0,
                    rig.get("cy.pc") - 101325.0, rig.get("cy.t"));
    }
}

TEST_CASE("stilldebug", "[.stilldebug]")
{
    Rig rig;
    rig.build();
    rig.method<void(void*, int, float, float, float, int)>("still")(rig.data(), int(Rig::kRate * 3), 180.0f, 4.0e5f, 1500.0f, 1000);
    for (const char* f : {"cy.p", "cy.t", "cy.m", "cy.pc", "cy.tc", "cy.qE", "cy.qEa", "cy.mE", "cy.mT", "cy.mI", "cy.reed", "ch.seg[0].near", "ch.seg[0].far",
                          "ch.seg[0].mF", "ch.seg[0].mB", "ch.q[0]", "ch.n", "ch.seg[1].near", "ch.seg[1].mB", "ch.seg[1].mF", "ch.sl[0]", "cy.vE.r"})
        std::printf("%s = %.6g\n", f, rig.get(f));
}

TEST_CASE("overrun", "[.overrun]")
{
    for (double rpm : {3000.0, 5000.0, 7000.0, 9000.0, 11000.0, 13000.0})
    {
        Rig rig;
        rig.build();
        rig.run(rpm, 0.0, 2.0, 100);
        const double n = rig.get("cyc"), mb = rig.get("burnSum") / n, sd = std::sqrt(std::max(rig.get("burnSq") / n - mb * mb, 0.0));
        std::printf("closed %5.0f rpm: purity %.2f, burn %.2f +- %.2f, flips %d of %d cycles, misfires %d\n", rpm,
                    rig.get("puritySum") / n, mb, sd, int(rig.get("flips")), int(n), int(rig.get("cy.misfires")));
    }
}

// Why the tuned speed follows the pipe's length weakly: for each scale, the trapped fresh charge with the chamber, with
// an endless pipe (no reflection at all: no tuning of its own) and with the straight pipe; the port's pressure at the
// exhaust port's closing (the plug's arrival)
TEST_CASE("tuning", "[.tuning]")
{
    const char* env = std::getenv("TS_FULLLOSS");
    for (double k : {0.8, 1.0, 1.25})
    {
        Rig ch, an, st;
        for (Rig* r : {&ch, &an, &st})
            for (const char* f : {"spec.headerLen", "spec.diffuserLen", "spec.bellyLen", "spec.baffleLen", "spec.stingerLen"})
                r->set(f, r->get(f) * k);
        st.set("spec.chamber", 0);
        an.set("spec.chamber", 0);
        for (Rig* r : {&ch, &an, &st})
            r->build();
        an.set("ch.anechoic", 1);
        if (env != nullptr)
            ch.set("ch.fullLoss", std::atoi(env));
        const double L = ch.get("ch.tuned"), c = ch.get("ch.c");
        std::printf("scale %.2f: L %.3f m, c %.0f; 2L/c = %.2f ms\n", k, L, c, 2000 * L / c);
        for (double rpm = 5000; rpm <= 17000; rpm += 500)
        {
            for (Rig* r : {&ch, &an, &st})
                r->run(rpm, 1.0, 1.0, 50);
            std::printf("  %6.0f: fresh %5.1f / endless %5.1f / straight %5.1f mg; torque %5.2f / %5.2f; port at EPC %+6.3f bar\n", rpm,
                        ch.meanTrapped() * 1e6, an.meanTrapped() * 1e6, st.meanTrapped() * 1e6, ch.meanTorque(), an.meanTorque(),
                        ch.get("pEpcSum") / ch.get("cyc") / 1e5);
        }
    }
}

TEST_CASE("endlessdebug", "[.endlessdebug]")
{
    Rig rig;
    rig.set("spec.chamber", 0);
    rig.build();
    rig.set("ch.anechoic", 1);
    rig.run(15500, 1.0, 0.8, 5);
    for (const char* f : {"trace[0]", "trace[30]", "traceE[0]", "traceE[100]", "ch.anechoic", "ch.n", "ch.seg[0].near", "ch.seg[0].far", "ch.seg[0].mB", "ch.seg[0].mF", "ch.seg[0].gain", "ch.q[0]", "ch.seg[0].dNom", "ch.seg[0].z", "cy.qEa"})
        std::printf("%s = %.6g\n", f, rig.get(f));
}

TEST_CASE("spread", "[.spread]")
{
    for (double rpm = 8500; rpm <= 14000; rpm += 250)
    {
        Rig rig;
        rig.build();
        rig.run(rpm, 1.0, 1.5, 80);
        const double n = rig.get("cyc"), m = rig.get("freshSum") / n, sd = std::sqrt(std::max(rig.get("freshSq") / n - m * m, 0.0));
        std::printf("%6.0f: fresh %5.1f +- %4.1f mg, delivered %5.1f, torque %5.2f, EPC port %+.3f\n", rpm, m * 1e6, sd * 1e6,
                    rig.meanDelivered() * 1e6, rig.meanTorque(), rig.get("pEpcSum") / n / 1e5);
    }
}
