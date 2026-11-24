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
        std::printf("chamber %d, tuned length %.3f m\n", chamber, rig.get("eng.ch[0].tuned"));
        for (double rpm = 5000; rpm <= 16000; rpm += 1000)
        {
            rig.run(rpm, 1.0);
            std::printf("%6.0f rpm: torque %6.2f N m, trapped %.1f, fresh %.1f mg, delivered %.1f mg, purity %.3f, port %+.2f..%+.2f bar, peak %.1f bar, bad %d, misfires %d\n",
                        rpm, rig.meanTorque(), rig.get("trappedSum") / rig.get("cyc") * 1e6, rig.meanTrapped() * 1e6, rig.meanDelivered() * 1e6,
                        rig.get("puritySum") / rig.get("cyc"), rig.get("pPortMin") / 1e5, rig.get("pPortMax") / 1e5,
                        rig.get("pCylMax") / 1e5, int(rig.get("bad")), int(rig.get("eng.cy[0].misfires")));
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
            rig.set("eng.ch[0].anechoic", 1);
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
        const double L = rig.get("eng.ch[0].tuned");
        std::printf("scale %.2f: tuned length %.3f m, c %.0f\n", k, L, rig.get("eng.ch[0].c"));
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
                        int(rig.get("eng.cy[0].misfires")), rig.get("puritySum") / rig.get("cyc"), rig.meanDelivered() * 1e6, rig.meanTorque());
        }
    for (double secs : {3.0, 6.0, 12.0})
    {
        Rig rig;
        rig.build();
        rig.method<void(void*, int, float, float, float, int)>("still")(rig.data(), int(Rig::kRate * secs), 180.0f, 4.0e5f, 1500.0f, 1000);
        std::printf("still %.0f s: port %.3f Pa, cyl %.3f, cc %.3f, t %.1f\n", secs, rig.get("pPortMax"), rig.get("eng.cy[0].p") - 101325.0,
                    rig.get("eng.cy[0].pc") - 101325.0, rig.get("eng.cy[0].t"));
    }
}

TEST_CASE("stilldebug", "[.stilldebug]")
{
    Rig rig;
    rig.build();
    rig.method<void(void*, int, float, float, float, int)>("still")(rig.data(), int(Rig::kRate * 3), 180.0f, 4.0e5f, 1500.0f, 1000);
    for (const char* f : {"eng.cy[0].p", "eng.cy[0].t", "eng.cy[0].m", "eng.cy[0].pc", "eng.cy[0].tc", "eng.cy[0].qE", "eng.cy[0].qEa", "eng.cy[0].mE", "eng.cy[0].mT", "eng.cy[0].mI", "eng.cy[0].reed", "eng.ch[0].seg[0].near", "eng.ch[0].seg[0].far",
                          "eng.ch[0].seg[0].mF", "eng.ch[0].seg[0].mB", "eng.ch[0].q[0]", "eng.ch[0].n", "eng.ch[0].seg[1].near", "eng.ch[0].seg[1].mB", "eng.ch[0].seg[1].mF", "eng.ch[0].sl[0]", "eng.cy[0].vE.r"})
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
                    rig.get("puritySum") / n, mb, sd, int(rig.get("flips")), int(n), int(rig.get("eng.cy[0].misfires")));
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
        an.set("eng.ch[0].anechoic", 1);
        if (env != nullptr)
            ch.set("eng.ch[0].fullLoss", std::atoi(env));
        const double L = ch.get("eng.ch[0].tuned"), c = ch.get("eng.ch[0].c");
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

TEST_CASE("silencerdebug", "[.silencerdebug]")
{
    Rig rig;
    rig.build();
    rig.run(10000, 1.0, 0.3, 5);
    for (const char* f : {"eng.ch[0].n", "eng.ch[0].seg[19].packOn", "eng.ch[0].seg[19].pb0[0]", "eng.ch[0].seg[19].pb1[0]",
                          "eng.ch[0].seg[19].pa1[0]", "eng.ch[0].seg[19].len", "eng.ch[0].seg[19].diam", "eng.ch[0].seg[20].len",
                          "eng.ch[0].seg[20].diam"})
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

TEST_CASE("features", "[.features]")
{
    auto line = [](const char* what, Rig& r, double rpm) {
        r.run(rpm, 1.0, 1.0, 40);
        std::printf("%-28s %6.0f rpm: torque %5.2f, delivered %5.1f mg, fresh %5.1f mg, reed max? cc %.2f bar\n", what, rpm, r.meanTorque(),
                    r.meanDelivered() * 1e6, r.meanTrapped() * 1e6, r.get("eng.cy[0].pc") / 1e5);
    };
    for (double rpm : {4000.0, 10000.0, 13000.0})
    {
        for (double hz : {550.0})
        {
            Rig r;
            r.set("spec.reedHz", hz);
            r.build(800);
            char name[64];
            std::snprintf(name, sizeof name, "reed %.0f Hz", hz);
            line(name, r, rpm);
        }
        for (int kind : {1, 2})
        {
            Rig r;
            r.set("spec.inletKind", kind);
            r.build(800);
            line(kind == 1 ? "disc" : "piston port", r, rpm);
        }
        for (double pv : {0.0, 170.0})
        {
            Rig r;
            r.set("spec.valveDur", pv);
            r.build(800);
            line(pv > 0 ? "power valve 170" : "no power valve", r, rpm);
        }
    }
}

TEST_CASE("stillsil", "[.stillsil]")
{
    struct C { double d, pack; int sil; };
    for (C k : {C{0.026, 16, 1}, C{0.026, 0, 1}, C{0.026, 4, 1}, C{0.065, 10, 1}, C{0.065, 16, 1}, C{0.026, 16, 0}})
    {
        Rig rig;
        rig.set("spec.silencerDiam", k.d);
        rig.set("spec.silencerPack", k.pack);
        rig.set("spec.silencer", k.sil);
        rig.build();
        rig.method<void(void*, int, float, float, float, int)>("still")(rig.data(), int(Rig::kRate * 3.0), 180.0f, 4.0e5f, 1500.0f,
                                                                       int(Rig::kRate * 0.5));
        std::printf("silencer %d core %.3f pack %4.1f: port %.3f Pa, flow %.2g kg/s\n", k.sil, k.d, k.pack, rig.get("pPortMax"),
                    rig.get("flowMax"));
    }
}
