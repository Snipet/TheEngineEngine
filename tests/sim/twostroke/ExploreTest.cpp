// tests/sim/twostroke/ExploreTest.cpp: a sweep printed, for looking (tagged hidden: run with "[.explore]").
#include "Ts.h"

#include <cmath>
#include <cstdio>
#include <algorithm>

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
    for (double rpm : {9000.0, 12000.0})
    {
        Rig rig;
        rig.build();
        rig.run(rpm, 1.0, 0.8, 5);
        std::printf("%.0f rpm: EPO %.0f EPC %.0f, TPO %.0f TPC %.0f\n", rpm, 180 - 96.0, 180 + 96.0, 180 - 65.0, 180 + 65.0);
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
