// tests/sim/twostroke/FeatureTest.cpp: the two-stroke's parts (card T): at rest nothing flows; the energy kept over the
// cycles; the inlets, the reed's own ring, the power valve's effect on the band; twins and triples; the spectrum's
// strongest line at the firing frequency; the cost.
#include "Ts.h"

#include <chrono>
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <vector>

using tee::test::twostroke::Rig;

namespace
{
    // The magnitude of the recorded sound's line at frequency f (Hz), by Goertzel over the rig's last samples
    double line(Rig& rig, double f)
    {
        const int n = int(rig.get("recN"));
        const double w = 2.0 * 3.14159265358979 * f / Rig::kRate;
        const double k = 2.0 * std::cos(w);
        double s1 = 0, s2 = 0;
        for (int i = 0; i < n; ++i)
        {
            // (a Hann window: the lines between the bins do not leak into one another)
            const double hann = 0.5 - 0.5 * std::cos(2.0 * 3.14159265358979 * i / (n - 1));
            const double s0 = hann * rig.get("rec[" + std::to_string(i) + "]") + k * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        return std::sqrt(std::max(s1 * s1 + s2 * s2 - k * s1 * s2, 0.0));
    }

    // Which of the first `count` multiples of the turn's frequency is the strongest line (1: once a turn)
    int strongest(Rig& rig, double rpm, int count, std::vector<double>& mags)
    {
        int best = 1;
        mags.clear();
        for (int h = 1; h <= count; ++h)
        {
            mags.push_back(line(rig, h * rpm / 60.0));
            if (mags.back() > mags[best - 1])
                best = h;
        }
        return best;
    }
} // namespace

TEST_CASE("at rest nothing flows: stopped, every port and inlet still", "[twostroke][passive]")
{
    for (int kind : {0, 1, 2})
        for (double angle : {0.0, 90.0, 180.0, 300.0})
        {
            Rig rig;
            rig.set("spec.inletKind", kind);
            rig.build();
            // the cylinder at the air's pressure and temperature: nothing should move at all
            rig.method<void(void*, int, float, float, float, int)>("still")(rig.data(), int(Rig::kRate * 1.0), float(angle), 101325.0f,
                                                                           300.0f, int(Rig::kRate * 0.5));
            CHECK(rig.get("bad") == 0);
            CHECK(rig.get("flowMax") < 1e-5);     // (float's resolution at 1 bar, 0.008 Pa a step: 1e-4 of a running flow)
            CHECK(rig.get("pPortMax") < 0.1);
        }
    // and from 4 bar at bottom dead centre: everything has stopped moving within 3 s
    Rig rig;
    rig.build();
    rig.method<void(void*, int, float, float, float, int)>("still")(rig.data(), int(Rig::kRate * 3.0), 180.0f, 4.0e5f, 1500.0f,
                                                                   int(Rig::kRate * 0.5));
    std::printf("stopped at 180 deg from 4 bar: after 3 s the largest flow %.2g kg/s, the port within %.3f Pa\n", rig.get("flowMax"),
                rig.get("pPortMax"));
    CHECK(rig.get("flowMax") < 1e-5);
}

TEST_CASE("the energy is kept: what the gas holds changes by what crossed its bounds", "[twostroke][energy]")
{
    for (double rpm : {5000.0, 9500.0, 14000.0})
    {
        Rig rig;
        rig.build();
        rig.run(rpm, 1.0, 0.5, 5);
        const char* f[] = {"eng.cy[0].eIn", "eng.cy[0].eOut", "eng.cy[0].eBurn", "eng.cy[0].eWall", "eng.cy[0].eWork"};
        double e0[5], e1[5];
        for (int i = 0; i < 5; ++i)
            e0[i] = rig.get(f[i]);
        const double u0 = rig.method<float(void*)>("energy")(rig.data());
        rig.run(rpm, 1.0, 1.0, 5);
        for (int i = 0; i < 5; ++i)
            e1[i] = rig.get(f[i]) - e0[i];
        const double u1 = rig.method<float(void*)>("energy")(rig.data());
        const double kept = e1[0] - e1[1] + e1[2] - e1[3] - e1[4];
        const double err = (u1 - u0) - kept;
        std::printf("%5.0f rpm, 1 s: in %.0f J, out %.0f J, burned %.0f J, walls %.0f J, work %.0f J; held %+.1f J; unaccounted "
                    "%.1f J (%.2f %% of the burn)\n", rpm, e1[0], e1[1], e1[2], e1[3], e1[4], u1 - u0, err, 100.0 * std::abs(err) / e1[2]);
        CHECK(std::abs(err) < 0.02 * e1[2]);
    }
}

TEST_CASE("the inlets: a disc spits back at low speed and matches the reed high up; a soft reed cannot follow a fast engine", "[twostroke][inlet]")
{
    auto delivered = [](const char* field, double value, double rpm) {
        Rig rig;
        if (field != nullptr)
            rig.set(field, value);
        rig.build();
        rig.run(rpm, 1.0, 1.0, 40);
        return rig.meanDelivered();
    };
    const double reedLow = delivered(nullptr, 0, 4000), discLow = delivered("spec.inletKind", 1, 4000);
    const double reedHigh = delivered(nullptr, 0, 13000), discHigh = delivered("spec.inletKind", 1, 13000);
    const double soft = delivered("spec.reedHz", 200, 13000);
    std::printf("delivered at 4000 rpm: reed %.1f mg, disc %.1f mg; at 13 000: reed %.1f, disc %.1f, a 200 Hz reed %.1f mg\n",
                reedLow * 1e6, discLow * 1e6, reedHigh * 1e6, discHigh * 1e6, soft * 1e6);
    CHECK(discLow < 0.9 * reedLow);
    CHECK(std::abs(discHigh - reedHigh) < 0.05 * reedHigh);
    CHECK(soft < 0.97 * reedHigh);
}

TEST_CASE("the power valve fills in below the band and leaves the band alone", "[twostroke][powervalve]")
{
    for (double rpm : {4000.0, 6000.0, 10000.0, 12000.0})
    {
        Rig plain, valved;
        valved.set("spec.valveDur", 170.0);
        plain.build();
        valved.build();
        plain.run(rpm, 1.0, 1.0, 40);
        valved.run(rpm, 1.0, 1.0, 40);
        std::printf("%5.0f rpm: without %.2f N m, with the valve %.2f N m (exhaust open %.0f deg)\n", rpm, plain.meanTorque(),
                    valved.meanTorque(), valved.get("eng.exDur"));
        if (rpm < 5000)
            CHECK(valved.meanTorque() > 1.2 * plain.meanTorque());
        if (rpm > 9000)
            CHECK(std::abs(valved.meanTorque() - plain.meanTorque()) < 0.03 * plain.meanTorque());
    }
}

TEST_CASE("the spectrum's lines: the firing frequency, once a turn a cylinder", "[twostroke][spectrum]")
{
    // What radiates is the rate the outlet's flow changes (a monopole's sound: +6 dB an octave on the flow's lines), so the
    // strongest radiated line sits near where the silencer starts to take the highs, not at the firing frequency itself
    // unless the silencer's corner is below it. Asked: the firing frequency's line strong (within 10 dB of the strongest)
    // on a utility engine's single (a short straight pipe into a can); on the racing chambers (no silencer) every line a
    // multiple of the firing frequency: in a twin or a triple the turn's other multiples cancel. (The chamber's ring,
    // the header's quarter wave at some 800 to 1000 Hz passed by the cones, which are long against it, is the strongest
    // from the 5th to the 8th multiple of the turn; the firing frequency's own line is printed.)
    {
        Rig rig;
        rig.set("spec.chamber", 0);
        rig.set("spec.headerLen", 0.08);
        rig.set("spec.diffuserLen", 0.05);
        rig.set("spec.bellyLen", 0.0);
        rig.set("spec.baffleLen", 0.05);
        rig.set("spec.stingerLen", 0.05);
        rig.set("spec.headerDiam", 0.022);
        rig.set("spec.stingerDiam", 0.016);
        rig.set("spec.silencerDiam", 0.07);     // (a can)
        rig.set("spec.silencerLen", 0.12);
        rig.set("spec.silencer", 1);
        rig.set("spec.silencerPack", 0.0);       // (a reactive can, unpacked, as a mower's)
        rig.build();
        if (std::getenv("TS_LIN") != nullptr)
            for (int j = 0; j < int(rig.get("eng.ch[0].n")); ++j)
            {
                rig.set("eng.ch[0].kf[" + std::to_string(j) + "]", 0.0);
                rig.set("eng.ch[0].kr[" + std::to_string(j) + "]", 0.0);
            }
        for (double rpm : {3000.0, 3600.0})
        {
            rig.run(rpm, 1.0, 1.0, 20);
            std::vector<double> mags;
            const int best = strongest(rig, rpm, 8, mags);
            std::printf("a utility single at %.0f rpm: the strongest line %d x the turn's; lines 1 to 8:", rpm, best);
            for (double m : mags)
                std::printf(" %.0f", 20.0 * std::log10(std::max(m / mags[best - 1], 1e-9)));
            std::printf(" dB\n");
            CHECK(mags[0] > 0.316 * mags[best - 1]);    // (-10 dB)
        }
    }
    struct Case { int cylinders; double rpm; };
    for (Case k : {Case{1, 6000}, Case{1, 10000}, Case{2, 8000}, Case{3, 8000}})
    {
        Rig rig;
        rig.set("spec.cylinders", k.cylinders);
        rig.build();
        rig.run(k.rpm, 1.0, 1.0, 20);
        std::vector<double> mags;
        const int best = strongest(rig, k.rpm, 9, mags);
        const double top = mags[best - 1];
        std::printf("%d cylinder(s), a chamber each, at %.0f rpm: the strongest line %d x the turn's (%.0f Hz); lines 1 to 9:",
                    k.cylinders, k.rpm, best, best * k.rpm / 60.0);
        for (double m : mags)
            std::printf(" %.0f", 20.0 * std::log10(std::max(m / top, 1e-9)));
        std::printf(" dB\n");
        CHECK(best % k.cylinders == 0);
        for (int h = 1; h <= 9; ++h)
            if (h % k.cylinders != 0)
                CHECK(mags[h - 1] < top * 0.01);         // (-40 dB)
    }
}

TEST_CASE("twins and triples: each cylinder as the single, the torque shared out evenly", "[twostroke][cylinders]")
{
    Rig single;
    single.build();
    single.run(9500, 1.0, 1.0, 40);
    for (int n : {2, 3})
    {
        Rig rig;
        rig.set("spec.cylinders", n);
        rig.build();
        rig.run(9500, 1.0, 1.0, 40);
        std::printf("%d cylinders at 9500 rpm: %.2f N m (the single's x %d: %.2f)\n", n, rig.meanTorque(), n, n * single.meanTorque());
        CHECK(std::abs(rig.meanTorque() - n * single.meanTorque()) < 0.02 * n * single.meanTorque());
    }
}

TEST_CASE("the cost of a single with its chamber, a sample", "[twostroke][cost]")
{
    Rig rig;
    rig.build();
    rig.run(9500, 1.0, 0.2, 5);
    const auto t0 = std::chrono::steady_clock::now();
    rig.run(9500, 1.0, 2.0, 5);
    const double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count() / (2.0 * Rig::kRate);
    std::printf("a single with its chamber (8 steps a cone, %d ducts): %.0f ns a sample on the rig (R is about 1800 ns on a quiet "
                "core: %.2f R)\n", int(rig.get("eng.ch[0].n")), ns, ns / 1800.0);
    CHECK(ns > 0);
}
