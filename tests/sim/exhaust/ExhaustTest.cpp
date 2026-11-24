// tests/sim/exhaust/ExhaustTest.cpp: card G's tests of the exhaust builder, on made-up port flows (no cylinders).
#include "Rig.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdio>

using namespace tee::test::exhaust;

TEST_CASE("the harness compiles", "[exhaust]")
{
    Rig rig(12);
    rig.configure();
    CHECK(rig.ends() == 2);
}

// Card G, test 2 (standalone form): the builder with every field at its default (the assignment table meaning the
// layout's own topology) against the reference's exhaust code (refexhaust.fm, copied from sim/ref/engine.fm), both
// on the same port flows, the noise off (its random numbers are drawn in another order), both started with their
// steepening pipes settled (refexhaust.fm says why). Every layout: topologies 0 (all into one), 1 (a collector a bank,
// one pipe) and 2 (dual with an H); and three other sets of the four dials. Measured: identical to the bit.
TEST_CASE("model 1 at its defaults is the reference's exhaust", "[exhaust][reference]")
{
    // every layout at the reference's dials, and three at others (HEADERS, PIPE, X-PIPE, MUFFLER)
    const float dialSets[4][4] = {{0.5f, 0.5f, 0.0f, 0.5f}, {0.1f, 0.9f, 0.7f, 0.95f}, {0.9f, 0.2f, 0.3f, 0.15f},
                                  {0.5f, 0.5f, 1.0f, 0.0f}};
    for (int run = 0; run < 17 + 9; ++run)
    {
        const int layout = run < 17 ? run : (run - 17) % 3 == 0 ? 5 : ((run - 17) % 3 == 1 ? 10 : 12);
        const int ds = run < 17 ? 0 : 1 + (run - 17) / 3;
        Rig rig(layout);
        rig.noise(0.0);
        rig.dials(dialSets[ds][0], dialSets[ds][1], dialSets[ds][2], dialSets[ds][3]);
        rig.configure();
        std::vector<float> q = pulses(rig, 3000.0, 1.0, 0.15, 0.01);
        std::vector<float> bl, br, rl, rr;
        rig.run(q, bl, br, false);
        rig.run(q, rl, rr, true);
        double es = 0.0;
        double ss = 0.0;
        for (std::size_t i = 0; i < bl.size(); ++i)
        {
            es += (bl[i] - rl[i]) * (bl[i] - rl[i]) + (br[i] - rr[i]) * (br[i] - rr[i]);
            ss += rl[i] * rl[i] + rr[i] * rr[i];
        }
        const double err = db(es / ss);
        std::printf("layout %2d, dials %d: builder - reference %.1f dB (reference rms %.3g)\n", layout, ds, err,
                    std::sqrt(ss / (2.0 * bl.size())));
        INFO("layout " << layout);
        CHECK(ss > 0.0);
        CHECK(err < -60.0);
    }
}


namespace
{
    // A random valid description: the assignment tables, the sides, every slot's kind, on one of six layouts
    struct Lcg
    {
        unsigned s;
        int next(int n)
        {
            s = s * 1664525u + 1013904223u;
            return static_cast<int>((s >> 8) % static_cast<unsigned>(n));
        }
        double unit() { return next(1 << 20) / double(1 << 20); }
    };

    int randomLayout(Lcg& g)
    {
        const int layouts[6] = {5, 6, 10, 12, 13, 15};
        return layouts[g.next(6)];
    }

    void randomTopology(Rig& rig, Lcg& g)
    {
        for (int i = 0; i < 16; ++i)
            rig.set("b.sp.primaryTo[" + std::to_string(i) + "]", g.next(10) == 0 ? -1 : g.next(4));
        for (int c = 0; c < 8; ++c)
        {
            const int k = g.next(10);
            rig.set("b.sp.collectorTo[" + std::to_string(c) + "]", k < 2 ? 2 + 4 + g.next(4) : g.next(2));
            rig.set("b.sp.colVol[" + std::to_string(c) + "]", g.next(4) == 0 ? 0.001 : 0.0);
        }
        rig.set("b.sp.sides", 1 + g.next(2));
        rig.set("b.sp.crossover", g.next(3));
        rig.set("b.sp.crossAt", g.next(2));
        rig.set("b.sp.primStepAt", g.next(3) == 0 ? 0.4 : 0.0);
        for (int s = 0; s < 2; ++s)
        {
            const std::string i = "[" + std::to_string(s) + "]";
            rig.set("b.sp.converter" + i, g.next(3));
            rig.set("b.sp.resonator" + i, g.next(4));
            rig.set("b.sp.resHz" + i, 80.0 + 500.0 * g.unit());
            rig.set("b.sp.muffler" + i, g.next(4));
            rig.set("b.sp.mufSize" + i, g.unit());
            rig.set("b.sp.bypass" + i, g.next(3));
            rig.set("b.sp.tips" + i, 1 + g.next(2));
        }
    }
} // namespace

// Card G, test 3: passivity, as a property test. Fifty random valid topologies, the engine off (the gas at the air's
// temperature, no flow); a 1 ms pulse into a port. The energy of the waves in flight (each pipe's lines, what is
// written and not yet read, over its impedance) does not rise after the pulse has gone in, over 5 ms windows, by more
// than 3 % (the pipes' filters, the wall's and the packings' shelves and the steady flow's trackers, hold some energy
// for a while and give it back, which this measure does not see; measured worst: 2.3 %), and all of it is gone (below
// 1e-6 of its peak) within 4 s (measured longest: 0.62 s).
TEST_CASE("every random topology is passive", "[exhaust][passive]")
{
    Lcg g{12345u};
    double worstRise = 0.0;
    double worstTail = 0.0;
    for (int trial = 0; trial < 50; ++trial)
    {
        const int layout = randomLayout(g);
        Rig rig(layout);
        rig.noise(0.0);
        rig.set("tPort", 300.0);
        rig.set("warm", 0.0);
        rig.set("mdot", 1.0e-4);
        randomTopology(rig, g);
        rig.configure();
        const int n = static_cast<int>(4.0 * kRate);
        std::vector<float> q(static_cast<std::size_t>(n) * kPorts, 0.0f);
        // a Hann pulse 1 ms long (band-limited, so the pipes' filters hold little of it at a time), some 1 kPa
        const int np = static_cast<int>(0.001 * kRate);
        for (int t = 0; t < np; ++t)
            q[static_cast<std::size_t>(t) * kPorts] = static_cast<float>(0.002 * (0.5 - 0.5 * std::cos(6.2831853 * t / np)));
        std::vector<double> e;
        const double peak = rig.energyAfter(q, e);
        // after the pulse has gone in, the windows' mean energy may only fall
        const int w = static_cast<int>(0.005 * kRate);
        double prev = 0.0;
        double rise = 0.0;
        for (int t = np; t + w <= n; t += w)
        {
            double m = 0.0;
            for (int k = t; k < t + w; ++k)
                m += e[static_cast<std::size_t>(k)];
            m /= w;
            if (t > np && prev > 1.0e-12 * peak)
                rise = std::max(rise, m / prev);
            prev = m;
        }
        // how long until it has all gone: the last sample above 1e-6 of the peak
        int last = 0;
        for (int k = 0; k < n; ++k)
        {
            if (e[static_cast<std::size_t>(k)] > 1.0e-6 * peak)
                last = k;
        }
        const double tail = last / kRate;
        worstRise = std::max(worstRise, rise);
        worstTail = std::max(worstTail, tail);
        INFO("trial " << trial << " layout " << layout << " rise " << rise << " gone after " << tail << " s");
        CHECK(std::isfinite(peak));
        CHECK(rise <= 1.03);
        CHECK(last < n - w);
    }
    std::printf("passivity: worst window-to-window rise %.4f, longest until under 1e-6 of the peak %.2f s\n",
                worstRise, worstTail);
}


namespace
{
    // A build of each element kind (on the crossplane V8 unless it says): its name and its fields
    struct Kind
    {
        const char* name;
        int layout;
        std::vector<std::pair<std::string, double>> fields;
    };

    std::vector<Kind> everyKind()
    {
        return {
            {"the reference's (chambered, H at the head)", 12, {}},
            {"no muffler", 12, {{"b.sp.muffler[0]", 0}, {"b.sp.muffler[1]", 0}}},
            {"straight-through", 12, {{"b.sp.muffler[0]", 1}, {"b.sp.muffler[1]", 1}}},
            {"three-pass", 12, {{"b.sp.muffler[0]", 3}, {"b.sp.muffler[1]", 3}}},
            {"no converter", 12, {{"b.sp.converter[0]", 0}, {"b.sp.converter[1]", 0}}},
            {"high-flow converter", 12, {{"b.sp.converter[0]", 2}, {"b.sp.converter[1]", 2}}},
            {"X at the head", 12, {{"b.sp.crossover", 2}}},
            {"X after the main pipes", 12, {{"b.sp.crossover", 2}, {"b.sp.crossAt", 1}}},
            {"H after the main pipes", 12, {{"b.sp.crossover", 1}, {"b.sp.crossAt", 1}, {"b.sp.crossD", 0.05}}},
            {"quarter-wave", 12, {{"b.sp.resonator[0]", 1}, {"b.sp.resonator[1]", 1}}},
            {"Helmholtz", 12, {{"b.sp.resonator[0]", 2}, {"b.sp.resonator[1]", 2}}},
            {"packed bottle", 12, {{"b.sp.resonator[0]", 3}, {"b.sp.resonator[1]", 3}}},
            {"bypass open", 12, {{"b.sp.bypass[0]", 2}, {"b.sp.bypass[1]", 2}}},
            {"bypass shut", 12, {{"b.sp.bypass[0]", 1}, {"b.sp.bypass[1]", 1}}},
            {"two tips", 12, {{"b.sp.tips[0]", 2}, {"b.sp.tips[1]", 2}}},
            {"stepped primaries", 12, {{"b.sp.primStepAt", 0.4}}},
            {"log manifolds", 12, {{"b.sp.colVol[0]", 0.0012}, {"b.sp.colVol[1]", 0.0012}}},
            {"zoomies", 12, {{"b.sp.primaryTo[0]", -1}, {"b.sp.primaryTo[1]", -1}, {"b.sp.primaryTo[2]", -1},
                             {"b.sp.primaryTo[3]", -1}, {"b.sp.primaryTo[4]", -1}, {"b.sp.primaryTo[5]", -1},
                             {"b.sp.primaryTo[6]", -1}, {"b.sp.primaryTo[7]", -1}}},
            {"4-2-1 on an inline four", 5, {{"b.sp.primaryTo[0]", 2}, {"b.sp.primaryTo[3]", 2}, {"b.sp.primaryTo[1]", 3},
                                            {"b.sp.primaryTo[2]", 3}, {"b.sp.collectorTo[2]", 2}, {"b.sp.collectorTo[3]", 2}}},
            {"single from a V8", 12, {{"b.sp.sides", 1}}},
        };
    }
} // namespace

// Card G, test 4: conservation. A steady flow into every port, the gas's temperatures held; after 2.5 s, the flow out
// of the radiating ends over one second equals the flow in within 0.5 %, for every element kind. (The model's flows
// are volume flows at the pipes' pressure, as the reference's: what it keeps is the volume the waves carry, which is
// the mass at a constant density.)
TEST_CASE("every element kind conserves the flow", "[exhaust][conservation]")
{
    double worst = 0.0;
    for (const Kind& k : everyKind())
    {
        Rig rig(k.layout);
        rig.noise(0.0);
        rig.set("tPort", 650.0);    // where the gas rests: its temperatures steady
        for (const auto& f : k.fields)
            rig.set(f.first, f.second);
        rig.configure();
        const int ports = static_cast<int>(rig.get("b.nPorts"));
        std::vector<float> q = steadyFlow(ports, 2.5, 0.01);
        std::vector<float> l, r;
        rig.run(q, l, r);
        rig.flowIn();
        rig.flowOut();
        q = steadyFlow(ports, 1.0, 0.01);
        rig.run(q, l, r);
        const double in = rig.flowIn();
        const double out = rig.flowOut();
        const double err = (out - in) / in;
        worst = std::max(worst, std::fabs(err));
        std::printf("conservation, %-45s ends %2d: out - in %+.4f %%, at the head %.0f Pa\n", k.name, rig.ends(),
                    100.0 * err, rig.pConv(0));
        INFO(k.name);
        CHECK(std::fabs(err) < 0.005);
    }
    std::printf("conservation: worst %.4f %%\n", 100.0 * worst);
}


// The cost: the builder's samples against the reference's exhaust code on the same flows (made-up flows; ns a 96 kHz
// sample, this machine, the least of three), for the reference's topology and a typical dual system with an X,
// resonators and valves; and per pipe in use. Not a pass or a fail: printed for the handoff.
TEST_CASE("what a sample costs", "[exhaust][cost]")
{
    auto timeIt = [](Rig& rig, bool reference) {
        std::vector<float> q = pulses(rig, 4000.0, 0.5, 0.15, 0.01);
        std::vector<float> l, r;
        double best = 1e30;
        for (int k = 0; k < 3; ++k)
        {
            const auto t0 = std::chrono::steady_clock::now();
            rig.run(q, l, r, reference);
            const auto t1 = std::chrono::steady_clock::now();
            best = std::min(best, std::chrono::duration<double, std::nano>(t1 - t0).count() / (q.size() / kPorts));
        }
        return best;
    };
    {
        Rig rig(12);
        rig.configure();
        const double b = timeIt(rig, false);
        const double r = timeIt(rig, true);
        int n = 0;
        for (int i = 0; i < 80; ++i)
            n += rig.get("b.net.dUse[" + std::to_string(i) + "]") != 0 ? 1 : 0;
        std::printf("cost, V8 reference topology: builder %.0f ns a sample (%d pipes: %.1f ns a pipe), the reference's "
                    "exhaust %.0f ns\n", b, n, b / n, r);
    }
    {
        Rig rig(12);
        rig.set("b.sp.crossover", 2);
        rig.set("b.sp.crossAt", 1);
        for (int s = 0; s < 2; ++s)
        {
            rig.set("b.sp.resonator[" + std::to_string(s) + "]", 1);
            rig.set("b.sp.bypass[" + std::to_string(s) + "]", 3);
        }
        rig.configure();
        const double b = timeIt(rig, false);
        int n = 0;
        for (int i = 0; i < 80; ++i)
            n += rig.get("b.net.dUse[" + std::to_string(i) + "]") != 0 ? 1 : 0;
        std::printf("cost, V8 dual with an X, quarter-wave stubs and bypass valves: %.0f ns a sample (%d pipes: %.1f ns "
                    "a pipe)\n", b, n, b / n);
    }
}
