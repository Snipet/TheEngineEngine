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
        // the passivity fix off for the comparison (passFix 0: the reference's order in every pipe); with it on, how
        // far model 1 leaves the reference where a reference pipe passes more than unity (printed, not checked)
        double err = 0.0, errFix = 0.0, ss = 0.0;
        std::size_t samples = 0;
        for (int fix : {0, 1})
        {
            Rig rig(layout);
            rig.noise(0.0);
            rig.set("b.passFix", fix);
            rig.dials(dialSets[ds][0], dialSets[ds][1], dialSets[ds][2], dialSets[ds][3]);
            rig.configure();
            std::vector<float> q = pulses(rig, 3000.0, 1.0, 0.15, 0.01);
            std::vector<float> bl, br, rl, rr;
            rig.run(q, bl, br, false);
            rig.run(q, rl, rr, true);
            double es = 0.0;
            ss = 0.0;
            for (std::size_t i = 0; i < bl.size(); ++i)
            {
                es += (bl[i] - rl[i]) * (bl[i] - rl[i]) + (br[i] - rr[i]) * (br[i] - rr[i]);
                ss += rl[i] * rl[i] + rr[i] * rr[i];
            }
            (fix == 0 ? err : errFix) = db(es / ss);
            samples = bl.size();
        }
        std::printf("layout %2d, dials %d: builder - reference %.1f dB, with the passivity fix %.1f dB (reference rms "
                    "%.3g)\n", layout, ds, err, errFix, std::sqrt(ss / (2.0 * samples)));
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
            rig.set("spec.primaryTo[" + std::to_string(i) + "]", g.next(10) == 0 ? -1 : g.next(4));
        for (int c = 0; c < 8; ++c)
        {
            const int k = g.next(10);
            rig.set("spec.collectorTo[" + std::to_string(c) + "]", k < 2 ? 2 + 4 + g.next(4) : g.next(2));
            rig.set("spec.colVol[" + std::to_string(c) + "]", g.next(4) == 0 ? 0.001 : 0.0);
        }
        rig.set("spec.sides", 1 + g.next(2));
        rig.set("spec.crossKind", g.next(3));
        rig.set("spec.crossAt", g.next(2));
        rig.set("spec.primStepAt", g.next(3) == 0 ? 0.4 : 0.0);
        for (int s = 0; s < 2; ++s)
        {
            const std::string i = "[" + std::to_string(s) + "]";
            rig.set("spec.converter" + i, g.next(3));
            rig.set("spec.resonator" + i, g.next(4));
            rig.set("spec.resHz" + i, 80.0 + 500.0 * g.unit());
            rig.set("spec.mufKind" + i, g.next(4));
            rig.set("spec.mufSize" + i, g.unit());
            rig.set("spec.bypass" + i, g.next(3));
            rig.set("spec.tips" + i, 1 + g.next(2));
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
            {"no muffler", 12, {{"spec.mufKind[0]", 0}, {"spec.mufKind[1]", 0}}},
            {"straight-through", 12, {{"spec.mufKind[0]", 1}, {"spec.mufKind[1]", 1}}},
            {"three-pass", 12, {{"spec.mufKind[0]", 3}, {"spec.mufKind[1]", 3}}},
            {"no converter", 12, {{"spec.converter[0]", 0}, {"spec.converter[1]", 0}}},
            {"high-flow converter", 12, {{"spec.converter[0]", 2}, {"spec.converter[1]", 2}}},
            {"X at the head", 12, {{"spec.crossKind", 2}}},
            {"X after the main pipes", 12, {{"spec.crossKind", 2}, {"spec.crossAt", 1}}},
            {"H after the main pipes", 12, {{"spec.crossKind", 1}, {"spec.crossAt", 1}, {"spec.crossD", 0.05}}},
            {"quarter-wave", 12, {{"spec.resonator[0]", 1}, {"spec.resonator[1]", 1}}},
            {"Helmholtz", 12, {{"spec.resonator[0]", 2}, {"spec.resonator[1]", 2}}},
            {"packed bottle", 12, {{"spec.resonator[0]", 3}, {"spec.resonator[1]", 3}}},
            {"bypass open", 12, {{"spec.bypass[0]", 2}, {"spec.bypass[1]", 2}}},
            {"bypass shut", 12, {{"spec.bypass[0]", 1}, {"spec.bypass[1]", 1}}},
            {"two tips", 12, {{"spec.tips[0]", 2}, {"spec.tips[1]", 2}}},
            {"stepped primaries", 12, {{"spec.primStepAt", 0.4}}},
            {"log manifolds", 12, {{"spec.colVol[0]", 0.0012}, {"spec.colVol[1]", 0.0012}}},
            {"zoomies", 12, {{"spec.primaryTo[0]", -1}, {"spec.primaryTo[1]", -1}, {"spec.primaryTo[2]", -1},
                             {"spec.primaryTo[3]", -1}, {"spec.primaryTo[4]", -1}, {"spec.primaryTo[5]", -1},
                             {"spec.primaryTo[6]", -1}, {"spec.primaryTo[7]", -1}}},
            {"4-2-1 on an inline four", 5, {{"spec.primaryTo[0]", 2}, {"spec.primaryTo[3]", 2}, {"spec.primaryTo[1]", 3},
                                            {"spec.primaryTo[2]", 3}, {"spec.collectorTo[2]", 2}, {"spec.collectorTo[3]", 2}}},
            {"single from a V8", 12, {{"spec.sides", 1}}},
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
        rig.set("spec.crossKind", 2);
        rig.set("spec.crossAt", 1);
        for (int s = 0; s < 2; ++s)
        {
            rig.set("spec.resonator[" + std::to_string(s) + "]", 1);
            rig.set("spec.bypass[" + std::to_string(s) + "]", 3);
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

// Card G, test 10 (standalone form): a stand-in turbine on each bank of the crossplane V8 (a 0.5 L housing whose gas
// leaves through a linear nozzle into the downpipe, harness.fm's standIn; card F's own is fitted at integration). The
// housing's junction is passive: with the engine off, a pulse into a port, the energy of the waves in flight and of
// the housings' gas never rises (5 ms windows, 3 % for what the filters hold) and is gone within 4 s. Driven, the
// downpipe's head swings less than the collector's housing.
TEST_CASE("a turbine housing's junction is passive; the downpipe is smoother", "[exhaust][turbine]")
{
    {
        Rig rig(12);
        rig.noise(0.0);
        rig.set("tPort", 300.0);
        rig.set("warm", 0.0);
        rig.set("mdot", 1.0e-4);
        rig.call("XTest::standIn", 0.0005f, 60000.0f);
        rig.configure();
        CHECK(rig.get("b.jHouse[0]") >= 0);
        CHECK(rig.get("b.jDown[1]") >= 0);
        const int n = static_cast<int>(4.0 * kRate);
        std::vector<float> q(static_cast<std::size_t>(n) * kPorts, 0.0f);
        const int np = static_cast<int>(0.001 * kRate);
        for (int t = 0; t < np; ++t)
            q[static_cast<std::size_t>(t) * kPorts] = static_cast<float>(0.002 * (0.5 - 0.5 * std::cos(6.2831853 * t / np)));
        std::vector<double> e;
        const double peak = rig.energyAfter(q, e);
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
        int last = 0;
        for (int k = 0; k < n; ++k)
            if (e[static_cast<std::size_t>(k)] > 1.0e-6 * peak)
                last = k;
        std::printf("turbine housing: worst window-to-window rise %.4f, gone after %.2f s\n", rise, last / kRate);
        CHECK(rise <= 1.03);
        CHECK(last < n - w);
    }
    {
        Rig rig(12);
        rig.noise(0.0);
        rig.call("XTest::standIn", 0.0005f, 60000.0f);
        rig.configure();
        std::vector<float> q = pulses(rig, 3000.0, 1.0, 0.15, 0.01);
        std::vector<float> l, r;
        rig.run(q, l, r);
        const double house = rig.callf("XTest::swingHouse");
        const double down = rig.callf("XTest::swingDown");
        std::printf("turbine housing: pressure swing in the housing %.0f Pa, at the downpipe's head %.0f Pa (%.1f dB)\n",
                    house, down, db(down * down / (house * house)));
        CHECK(down < house);
    }
}

// The Scope: 64 points from port 0 to its tail, spaced by length, finite, with the pulse visible along them
TEST_CASE("the scope shows the path's pressures", "[exhaust][scope]")
{
    Rig rig(12);
    rig.noise(0.0);
    rig.set("b.scopeOn", 1);
    rig.configure();
    std::vector<float> q = pulses(rig, 3000.0, 0.3, 0.15, 0.01);
    std::vector<float> l, r;
    rig.run(q, l, r);
    const int n = static_cast<int>(rig.get("b.nPath"));
    double lo = 1e30, hi = -1e30, xPrev = -1.0;
    for (int i = 0; i < 64; ++i)
    {
        const double p = rig.get("b.scopeP[" + std::to_string(i) + "]");
        const double x = rig.get("b.scopeX[" + std::to_string(i) + "]");
        CHECK(std::isfinite(p));
        CHECK(x > xPrev);
        xPrev = x;
        lo = std::min(lo, p);
        hi = std::max(hi, p);
    }
    std::printf("scope: %d pipes from port 0 to its tail, %.2f m; pressures %.0f to %.0f Pa\n", n, xPrev, lo, hi);
    CHECK(n == 7);
    CHECK(hi - lo > 100.0);
}

// Card G, test 11 (standalone form): three minutes of running flows (blowdown pulses, the speed wandering 1000 to
// 7000 rpm, the gas warming and cooling) while the topology changes to a random one every 4 s (rebuilt as the engine
// rebuilds under its fade). Every output finite and bounded; after the flows stop, no pressure stays in the pipes.
TEST_CASE("random topologies changed while running stay finite and bounded", "[exhaust][soak]")
{
    Lcg g{777u};
    Rig rig(12);
    rig.noise(1.0);
    rig.configure();
    double peak = 0.0;
    double peakLater = 0.0;
    bool finite = true;
    for (int k = 0; k < 45; ++k)
    {
        randomTopology(rig, g);
        rig.set("tPort", 600.0 + 600.0 * g.unit());
        rig.set("mdot", 0.005 + 0.05 * g.unit());
        rig.rebuild();
        const double rpm = 1000.0 + 6000.0 * g.unit();
        std::vector<float> q = pulses(rig, rpm, 4.0, 0.05 + 0.25 * g.unit(), 0.02 * g.unit());
        std::vector<float> l, r;
        rig.run(q, l, r);
        for (std::size_t i = 0; i < l.size(); ++i)
        {
            finite = finite && std::isfinite(l[i]) && std::isfinite(r[i]);
            const double a = std::max(std::fabs(l[i]), std::fabs(r[i]));
            peak = std::max(peak, a);
            if (i > static_cast<std::size_t>(0.1 * kRate))
                peakLater = std::max(peakLater, a);
        }
    }
    // then nothing flows in for 3 s
    std::vector<float> q(static_cast<std::size_t>(3.0 * kRate) * kPorts, 0.0f);
    std::vector<float> l, r;
    rig.run(q, l, r);
    double tail = 0.0;
    for (std::size_t i = l.size() - static_cast<std::size_t>(0.1 * kRate); i < l.size(); ++i)
        tail = std::max(tail, static_cast<double>(std::max(std::fabs(l[i]), std::fabs(r[i]))));
    const double head = std::max(std::fabs(rig.pConv(0)), std::fabs(rig.pConv(1)));
    std::printf("soak: 45 topologies over 3 minutes: outputs finite %s, largest %.1f (%.1f past each rebuild's first "
                "0.1 s); 3 s after the flows stop: output %.3g, mean pressure at the heads %.3g Pa\n",
                finite ? "yes" : "no", peak, peakLater, tail, head);
    CHECK(finite);
    CHECK(peak < 1.0e4);
    CHECK(tail < 1.0e-3 * peak);
    CHECK(head < 1.0);
}

// At rest, nothing flows (01-rules.md §4): every element kind, no flow in for 2 s, the gas warm: the ends' flow and
// what they radiate stay at nothing (no chatter at a junction, a chamber, a neck or a valve)
TEST_CASE("at rest, nothing flows", "[exhaust][rest]")
{
    double worstQ = 0.0;
    double worstOut = 0.0;
    for (const Kind& k : everyKind())
    {
        Rig rig(k.layout);
        rig.noise(0.0);
        for (const auto& f : k.fields)
            rig.set(f.first, f.second);
        rig.configure();
        std::vector<float> q(static_cast<std::size_t>(2.0 * kRate) * kPorts, 0.0f);
        std::vector<float> l, r;
        rig.run(q, l, r);
        double out = 0.0;
        for (std::size_t i = 0; i < l.size(); ++i)
            out = std::max(out, static_cast<double>(std::max(std::fabs(l[i]), std::fabs(r[i]))));
        const double flow = std::fabs(rig.flowOut()) / (2.0 * kRate);
        worstQ = std::max(worstQ, flow);
        worstOut = std::max(worstOut, out);
        INFO(k.name);
        CHECK(flow < 1.0e-9);
        CHECK(out < 1.0e-6);
    }
    std::printf("at rest: largest mean flow out %.3g m^3/s, largest output %.3g\n", worstQ, worstOut);
}


// Card T's finding (duct.fm, filtFirst): a pipe with deep packing and little loss a pass, taking its steady flow out
// before its filters, gives back a few % more than it took near 25 Hz. The builder puts such pipes' filters first.
// Here: straight-through mufflers at their largest (keep 0.95, 26 dB of packing), the engine stopped, a pulse in: the
// energy is gone (under 1e-6) within 2 s. (Which pipes take their filters first is the exact pass of passGain, the
// sweep below; how many here is printed.)
TEST_CASE("a deeply packed pipe at rest is passive", "[exhaust][passive]")
{
    Rig rig(5);
    rig.noise(0.0);
    rig.set("tPort", 300.0);
    rig.set("warm", 0.0);
    rig.set("mdot", 1.0e-4);
    rig.set("spec.mufKind[0]", 1);
    rig.set("spec.mufSize[0]", 1.0);
    rig.configure();
    int flagged = 0;
    for (int i = 0; i < 84; ++i)
        flagged += rig.get("b.net.d[" + std::to_string(i) + "].filtFirst") != 0.0 ? 1 : 0;
    const int n = static_cast<int>(3.0 * kRate);
    std::vector<float> q(static_cast<std::size_t>(n) * kPorts, 0.0f);
    const int np = static_cast<int>(0.001 * kRate);
    for (int t = 0; t < np; ++t)
        q[static_cast<std::size_t>(t) * kPorts] = static_cast<float>(0.002 * (0.5 - 0.5 * std::cos(6.2831853 * t / np)));
    std::vector<double> e;
    const double peak = rig.energyAfter(q, e);
    int last = 0;
    for (int k = 0; k < n; ++k)
        if (e[static_cast<std::size_t>(k)] > 1.0e-6 * peak)
            last = k;
    std::printf("packed straight-through at rest: %d pipes with their filters first, gone after %.2f s\n", flagged,
                last / kRate);
    CHECK(last < static_cast<int>(2.0 * kRate));
}

// The lead's ruling on card T's finding: every packed pipe's one pass at or under unity at every frequency. A pipe
// alone (harness.fm's probe), its impulse response from near end to far, |H(f)| from 5 Hz to 20 kHz (60 points a
// decade), over a sweep of a pass's gain (0.9 to 1), a packing's depths and a low-pass, with the wall's shelves of a
// 1 m pipe; its order chosen by the builder's rule (XBuild::filterFirst: the pass's own worst gain, from its
// coefficients). And the reference's order where the rule changes it, to show the overshoot the rule removes.
TEST_CASE("every packed pipe's pass is at or under unity, the sweep", "[exhaust][passive][sweep]")
{
    Rig rig(12);
    std::string error;
    auto* set = program().function<void(void*, float, float, float, float, int)>("XTest::probeSet", error);
    auto* run = program().function<void(void*, float*, int)>("XTest::probeRun", error);
    INFO(error);
    REQUIRE(set != nullptr);
    REQUIRE(run != nullptr);
    void* inst = rig.instance();
    const int n = 1 << 15;
    std::vector<float> h(static_cast<std::size_t>(n));
    std::vector<double> freqs;
    for (double f = 5.0; f < 20000.0; f *= std::pow(10.0, 1.0 / 60.0))
        freqs.push_back(f);
    auto peakGain = [&](double& at) {
        double best = 0.0;
        for (double f : freqs)
        {
            const double w = 2.0 * 3.14159265358979 * f / kRate;
            double re = 0.0, im = 0.0;
            for (int t = 0; t < n; ++t)
            {
                re += h[static_cast<std::size_t>(t)] * std::cos(w * t);
                im -= h[static_cast<std::size_t>(t)] * std::sin(w * t);
            }
            const double m = std::sqrt(re * re + im * im);
            if (m > best)
            {
                best = m;
                at = f;
            }
        }
        return best;
    };
    double worst = 0.0, worstAt = 0.0;
    double worstRef = 0.0;
    int flagged = 0, cases = 0;
    tee::ScopedFlushDenormals flush;
    for (float keep : {0.9f, 0.94f, 0.96f, 0.98f, 1.0f})
        for (float dLow : {0.0f, 2.0f, 8.0f, 16.0f})
            for (float dHigh : {0.0f, 2.0f, 8.0f})
                for (float lpHz : {0.0f, 300.0f, 2000.0f})
                {
                    set(inst, dLow, dHigh, lpHz, keep, -1);
                    const int ff = static_cast<int>(rig.get("probe.filtFirst"));
                    run(inst, h.data(), n);
                    double at = 0.0;
                    const double g = peakGain(at);
                    ++cases;
                    flagged += ff;
                    if (g > worst)
                    {
                        worst = g;
                        worstAt = at;
                    }
                    INFO("keep " << keep << ", packing " << dLow << " + " << dHigh << " dB, low-pass " << lpHz
                                 << " Hz, filters first " << ff << ": |H| " << g << " at " << at << " Hz");
                    CHECK(g <= 1.0005);
                    if (ff != 0)
                    {
                        set(inst, dLow, dHigh, lpHz, keep, 0);
                        run(inst, h.data(), n);
                        double at0 = 0.0;
                        worstRef = std::max(worstRef, peakGain(at0));
                    }
                }
    std::printf("sweep: %d pipes, %d with their filters first; the largest |H| of a pass %.5f (at %.0f Hz); in the "
                "reference's order those would reach %.4f\n", cases, flagged, worst, worstAt, worstRef);
    CHECK(worstRef > 1.01);
}
