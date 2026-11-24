// tests/sim/twostroke/CylinderTest.cpp: the two-stroke cylinder's bookkeeping and stability (card T): mass kept,
// passive with the engine stopped, finite under a soak; four-stroking at light load, unscripted.
#include "Ts.h"

#include <cmath>
#include <cstdio>
#include <random>

using tee::test::twostroke::Rig;

TEST_CASE("the cylinder and crankcase keep their mass: what is in is what came in less what went out", "[twostroke][mass]")
{
    Rig rig;
    rig.build();
    for (double rpm : {4000.0, 9000.0, 13000.0})
    {
        const double m0 = rig.get("eng.cy[0].m") + rig.get("eng.cy[0].mc");
        const double in0 = rig.get("eng.cy[0].massIn"), out0 = rig.get("eng.cy[0].massOut");
        rig.run(rpm, rpm < 5000 ? 0.3 : 1.0, 1.0, 10);
        const double m1 = rig.get("eng.cy[0].m") + rig.get("eng.cy[0].mc");
        const double in = rig.get("eng.cy[0].massIn") - in0, out = rig.get("eng.cy[0].massOut") - out0;
        const double err = (m1 - m0) - (in - out);
        std::printf("%5.0f rpm, 1 s: in %.3f g, out %.3f g, held %+.4f mg; unaccounted %.3g mg (%.1e of the flow)\n", rpm,
                    in * 1e3, out * 1e3, (m1 - m0) * 1e6, err * 1e6, std::abs(err) / in);
        CHECK(std::abs(err) < 1e-4 * in);
    }
}

TEST_CASE("stopped, the cylinder and the pipe settle to the air: passive", "[twostroke][passive]")
{
    for (double angle : {180.0, 120.0, 0.0})
    {
        Rig rig;
        rig.build();
        // the cylinder at 4 bar and 1500 K, the piston still (at bottom dead centre the ports are open)
        rig.method<void(void*, int, float, float, float, int)>("still")(rig.data(), int(Rig::kRate * 3.0), float(angle), 4.0e5f,
                                                                       1500.0f, int(Rig::kRate * 0.5));
        std::printf("stopped at %3.0f deg from 4 bar: after 3 s the port within %.3f Pa of the air, the cylinder at %.1f Pa, "
                    "the crankcase %.1f Pa\n", angle, rig.get("pPortMax"), rig.get("eng.cy[0].p") - 101325.0, rig.get("eng.cy[0].pc") - 101325.0);
        CHECK(rig.get("bad") == 0);
        CHECK(rig.get("pPortMax") < 20.0);
        if (angle != 0.0)
            CHECK(std::abs(rig.get("eng.cy[0].p") - 101325.0) < 50.0);
    }
}

TEST_CASE("finite under a soak: 60 s of speeds and throttles at random", "[twostroke][soak]")
{
    Rig rig;
    rig.build();
    std::mt19937 random(1234);
    std::uniform_real_distribution<double> speed(2000.0, 17000.0), pedal(0.0, 1.0);
    double peak = 0;
    for (int i = 0; i < 60; ++i)
    {
        rig.run(speed(random), pedal(random), 1.0, 5);
        REQUIRE(rig.get("bad") == 0);
        peak = std::max(peak, rig.get("pCylMax"));
        CHECK(std::isfinite(rig.meanTorque()));
    }
    std::printf("soak: 60 s, finite throughout, the highest cylinder pressure %.1f bar\n", peak / 1e5);
    CHECK(peak < 150e5);
}

TEST_CASE("at light load it four-strokes: fire and misfire alternate from poor scavenging, not scripted", "[twostroke][fourstroke]")
{
    // closed throttle at 7000 rpm (an overrun): the charge's burned share sits at the reference's limit of dilution, a
    // misfire leaves its unburned charge, the next cycle's purer charge fires, and so on
    Rig rig;
    rig.build();
    rig.run(7000.0, 0.0, 2.0, 100);
    const double n = rig.get("cyc");
    const int flips = int(rig.get("flips"));
    Rig full;
    full.build();
    full.run(7000.0, 1.0, 2.0, 100);
    std::printf("7000 rpm, throttle closed: the burn %.2f on average, fire and misfire alternated %d times in %d cycles (purity %.2f);"
                " at full throttle %d misfires (purity %.2f)\n", rig.get("burnSum") / n, flips, int(n), rig.get("puritySum") / n,
                int(full.get("eng.cy[0].misfires")), full.get("puritySum") / full.get("cyc"));
    CHECK(flips > 0.8 * n);
    CHECK(full.get("eng.cy[0].misfires") == 0);
}
