// tests/sim/exhaust/CharacterTest.cpp: card G's tests 5 to 9, what each element does to the sound, measured on
// made-up port flows (blowdown pulses at the layout's firing angles, Rig.h's pulses(): no cylinders, so these are the
// pipes' own effects on a fixed source, not the engine's response to its pipes). Each prints what it measured.
#include "Rig.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <string>
#include <utility>

using namespace tee::test::exhaust;

namespace
{
    using Fields = std::vector<std::pair<std::string, double>>;

    struct Heard
    {
        std::vector<float> left;
        std::vector<float> right;
        std::vector<float> sum;
        double pMuf = 0.0;
        double pConv = 0.0;
        int ends = 0;
    };

    // A build of `layout` with `fields`, driven at rpm for `seconds`; what came out of its last second
    Heard listen(int layout, const Fields& fields, double rpm = 3000.0, double seconds = 2.0, double peak = 0.15,
                 double steady = 0.01)
    {
        Rig rig(layout);
        rig.noise(0.0);
        for (const auto& f : fields)
            rig.set(f.first, f.second);
        rig.configure();
        std::vector<float> q = pulses(rig, rpm, seconds, peak, steady);
        Heard h;
        rig.run(q, h.left, h.right);
        const std::size_t from = h.left.size() - static_cast<std::size_t>(kRate);
        h.left.erase(h.left.begin(), h.left.begin() + static_cast<std::ptrdiff_t>(from));
        h.right.erase(h.right.begin(), h.right.begin() + static_cast<std::ptrdiff_t>(from));
        h.sum.resize(h.left.size());
        for (std::size_t i = 0; i < h.sum.size(); ++i)
            h.sum[i] = h.left[i] + h.right[i];
        h.pMuf = rig.pMuf(0);
        h.pConv = rig.pConv(0);
        h.ends = rig.ends();
        return h;
    }

    // The engine's orders up to 2 kHz in x: the power of the lines that are not multiples of `firing` half-orders
    // over the power of those that are (dB). A crossplane V8's bank fires at 180-90-180-270: its lines at the half
    // orders between its firing order's are its burble.
    double offOrders(const std::vector<float>& x, double rpm, int firing)
    {
        const double f0 = rpm / 120.0;
        double off = 0.0;
        double on = 0.0;
        for (int k = 1; k * f0 <= 2000.0; ++k)
        {
            const double p = tone(x, k * f0, 0, x.size());
            if (k % firing == 0)
                on += p;
            else
                off += p;
        }
        return db(off / on);
    }

    const std::size_t kAll = static_cast<std::size_t>(kRate);

    Fields bothSides(const std::string& field, double v)
    {
        return {{"b.sp." + field + "[0]", v}, {"b.sp." + field + "[1]", v}};
    }
} // namespace

// Card G, test 5: 180-degree headers on the crossplane V8 (each collector takes four cylinders that fire 180 degrees
// apart, pairs across the banks) against the reference's collector a bank: at each tail the lines off its firing
// order (the half orders: the burble) fall. A boxer's unequal headers (the second bank's 1.75 times as long) raise
// its off-order lines against equal ones.
TEST_CASE("180-degree headers take the burble out; unequal headers put it in", "[exhaust][character]")
{
    const Heard bank = listen(12, {});
    // the crossplane's firing angles: cylinders 0 3 5 6 at 0 180 360 540, 7 2 4 1 at 90 270 450 630
    const Heard even = listen(12, {{"b.sp.primaryTo[0]", 0}, {"b.sp.primaryTo[3]", 0}, {"b.sp.primaryTo[5]", 0},
                                   {"b.sp.primaryTo[6]", 0}, {"b.sp.primaryTo[7]", 1}, {"b.sp.primaryTo[2]", 1},
                                   {"b.sp.primaryTo[4]", 1}, {"b.sp.primaryTo[1]", 1}});
    const double bankL = offOrders(bank.left, 3000.0, 4);
    const double bankR = offOrders(bank.right, 3000.0, 4);
    const double evenL = offOrders(even.left, 3000.0, 4);
    const double evenR = offOrders(even.right, 3000.0, 4);
    std::printf("crossplane V8, 3000 rpm, off-order lines over firing-order lines at each tail: bank collectors "
                "%.1f / %.1f dB, 180-degree %.1f / %.1f dB\n", bankL, bankR, evenL, evenR);
    CHECK(evenL < bankL - 10.0);
    CHECK(evenR < bankR - 10.0);

    const Heard unequal = listen(6, {});
    Fields eq;
    for (int i = 0; i < 4; ++i)
        eq.push_back({"b.sp.primLen[" + std::to_string(i) + "]", 0.45});
    const Heard equal = listen(6, eq);
    const double u = offOrders(unequal.left, 3000.0, 4);
    const double e = offOrders(equal.left, 3000.0, 4);
    std::printf("flat four, 3000 rpm, off-order lines over firing-order lines: unequal headers %.1f dB, equal %.1f dB\n",
                u, e);
    CHECK(u > e + 6.0);
}

// Card G, test 6: an X in place of an H (both after the main pipes, the H the main pipe's bore): what each does to the
// level at the tails above some frequencies, and to the difference between the two tails, against no crossover.
TEST_CASE("an X and an H tie the sides together", "[exhaust][character]")
{
    const Heard none = listen(12, {{"b.sp.crossover", 0}});
    const Heard h = listen(12, {{"b.sp.crossover", 1}, {"b.sp.crossAt", 1}, {"b.sp.crossD", 0.064}});
    const Heard x = listen(12, {{"b.sp.crossover", 2}, {"b.sp.crossAt", 1}});
    auto diff = [](const Heard& k) {
        std::vector<float> d(k.left.size());
        for (std::size_t i = 0; i < d.size(); ++i)
            d[i] = k.left[i] - k.right[i];
        return db(rms(d, 0, d.size()) * rms(d, 0, d.size()) / (rms(k.sum, 0, kAll) * rms(k.sum, 0, kAll)));
    };
    const double dN = diff(none);
    const double dH = diff(h);
    const double dX = diff(x);
    std::printf("tails' difference over their sum: none %.1f dB, H %.1f dB, X %.1f dB\n", dN, dH, dX);
    CHECK(dH < dN - 3.0);
    CHECK(dX < dN - 3.0);
    for (double f : {250.0, 500.0, 1000.0, 2000.0, 4000.0})
    {
        const double pN = powerAbove(none.sum, f, 0, kAll);
        const double pH = powerAbove(h.sum, f, 0, kAll);
        const double pX = powerAbove(x.sum, f, 0, kAll);
        std::printf("above %4.0f Hz, both tails summed: H %+.1f dB, X %+.1f dB against none; X - H %+.1f dB\n", f,
                    db(pH / pN), db(pX / pN), db(pX / pH));
    }
    CHECK(powerAbove(x.sum, 1000.0, 0, kAll) > powerAbove(h.sum, 1000.0, 0, kAll));
}

// Card G, test 7: a quarter-wave branch tuned to 200 Hz cuts that line at the tails by at least 10 dB at 3000 rpm,
// where the crossplane V8's firing order (4: 200 Hz) sits on it; a Helmholtz chamber tuned to 150 Hz (order 3) its own.
TEST_CASE("resonators cut their notes", "[exhaust][character]")
{
    const Heard plain = listen(12, {});
    Fields qw = bothSides("resonator", 1);
    qw.push_back({"b.sp.resHz[0]", 200.0});
    qw.push_back({"b.sp.resHz[1]", 200.0});
    const Heard quarter = listen(12, qw);
    Fields hz = bothSides("resonator", 2);
    hz.push_back({"b.sp.resHz[0]", 150.0});
    hz.push_back({"b.sp.resHz[1]", 150.0});
    const Heard helm = listen(12, hz);
    const double q200 = db(tone(quarter.sum, 200.0, 0, kAll) / tone(plain.sum, 200.0, 0, kAll));
    const double h150 = db(tone(helm.sum, 150.0, 0, kAll) / tone(plain.sum, 150.0, 0, kAll));
    const double q100 = db(tone(quarter.sum, 100.0, 0, kAll) / tone(plain.sum, 100.0, 0, kAll));
    const double q400 = db(tone(quarter.sum, 400.0, 0, kAll) / tone(plain.sum, 400.0, 0, kAll));
    std::printf("quarter-wave at 200 Hz: the 200 Hz line %+.1f dB (100 Hz %+.1f, 400 Hz %+.1f); Helmholtz at 150 Hz: "
                "the 150 Hz line %+.1f dB\n", q200, q100, q400, h150);
    CHECK(q200 <= -10.0);
    CHECK(h150 <= -10.0);
}

// Card G, test 8: the mufflers' insertion loss (the level at the tails against none) in the order chambered,
// three-pass, straight-through; their back pressure (the mean before the muffler) in the same order; the bypass valve
// open is louder and brighter than shut, and its opening makes no click. A click is a burst of highs: opening over its
// default 0.3 s, the level above 10 kHz is no more than running open (snapped open in 10 ms, it is). The card's own
// measure, the largest step between samples no larger than running open makes, holds at a second's travel; at 0.3 s
// the level overshoots for a moment as the steady flow moves into the bypass, and the largest step with it (the test
// prints both).
TEST_CASE("mufflers in their order; the bypass valve opens without a click", "[exhaust][character]")
{
    const Heard none = listen(12, bothSides("muffler", 0));
    const Heard straight = listen(12, bothSides("muffler", 1));
    const Heard chambered = listen(12, bothSides("muffler", 2));
    const Heard three = listen(12, bothSides("muffler", 3));
    const double r0 = rms(none.sum, 0, kAll);
    const double ilS = db(r0 * r0 / (rms(straight.sum, 0, kAll) * rms(straight.sum, 0, kAll)));
    const double ilC = db(r0 * r0 / (rms(chambered.sum, 0, kAll) * rms(chambered.sum, 0, kAll)));
    const double ilT = db(r0 * r0 / (rms(three.sum, 0, kAll) * rms(three.sum, 0, kAll)));
    std::printf("insertion loss: chambered %.1f dB, three-pass %.1f dB, straight-through %.1f dB\n", ilC, ilT, ilS);
    std::printf("back pressure before the muffler: chambered %.0f Pa, three-pass %.0f, straight-through %.0f, none %.0f\n",
                chambered.pMuf, three.pMuf, straight.pMuf, none.pMuf);
    CHECK(ilC > ilT);
    CHECK(ilT > ilS);
    CHECK(ilS > 0.0);
    CHECK(chambered.pMuf > three.pMuf);
    CHECK(three.pMuf > straight.pMuf);
    CHECK(straight.pMuf > none.pMuf);

    Fields shutF = bothSides("bypass", 1);
    Fields openF = bothSides("bypass", 2);
    const Heard shut = listen(12, shutF);
    const Heard open = listen(12, openF);
    const double rs = rms(shut.sum, 0, kAll);
    const double ro = rms(open.sum, 0, kAll);
    const double bs = powerAbove(shut.sum, 1000.0, 0, kAll) / (rs * rs);
    const double bo = powerAbove(open.sum, 1000.0, 0, kAll) / (ro * ro);
    std::printf("bypass open against shut: %+.1f dB, its share above 1 kHz %+.1f dB\n", db(ro * ro / (rs * rs)),
                db(bo / bs));
    CHECK(ro > rs);
    CHECK(bo > bs);

    // the valve by the ECU's word: shut for a second, then opened; open, it settles over a second (its steady flow
    // finding its way), so "open" is the second after that
    struct Opening
    {
        double stepTravel;
        double stepOpen;
        double hfTravel;
        double hfOpen;
    };
    auto opening = [](double travel) {
        Rig rig(12);
        rig.noise(0.0);
        rig.set("b.sp.bypass[0]", 4);
        rig.set("b.sp.bypass[1]", 4);
        rig.set("b.sp.bypassTime", travel);
        rig.configure();
        std::vector<float> q = pulses(rig, 3000.0, 1.0, 0.15, 0.01);
        std::vector<float> l1, r1, l2, r2, l3, r3;
        rig.run(q, l1, r1);
        rig.set("b.eng.ecuValve", 1.0);
        rig.run(q, l2, r2);
        rig.run(q, l3, r3);
        CHECK(rig.valve(0) == 1.0);
        auto maxStep = [](const std::vector<float>& x) {
            double m = 0.0;
            for (std::size_t i = 1; i < x.size(); ++i)
                m = std::max(m, static_cast<double>(std::fabs(x[i] - x[i - 1])));
            return m;
        };
        const std::size_t half = static_cast<std::size_t>(0.5 * kRate);
        return Opening{std::max(maxStep(l2), maxStep(r2)), std::max(maxStep(l3), maxStep(r3)),
                       powerAbove(l2, 10000.0, 0, half) + powerAbove(r2, 10000.0, 0, half),
                       powerAbove(l3, 10000.0, 0, half) + powerAbove(r3, 10000.0, 0, half)};
    };
    const Opening snap = opening(0.01);
    const Opening normal = opening(0.3);
    const Opening slow = opening(1.0);
    std::printf("bypass opening, largest step between samples while it opens against open: in 10 ms %.2f / %.2f, "
                "0.3 s %.2f / %.2f, 1 s %.2f / %.2f; above 10 kHz while it opens against open: %+.1f dB, %+.1f dB, %+.1f dB\n",
                snap.stepTravel, snap.stepOpen, normal.stepTravel, normal.stepOpen, slow.stepTravel, slow.stepOpen,
                db(snap.hfTravel / snap.hfOpen), db(normal.hfTravel / normal.hfOpen), db(slow.hfTravel / slow.hfOpen));
    // a valve that snaps open clicks: the measure sees it
    CHECK(snap.hfTravel > 2.0 * snap.hfOpen);
    // at its default travel no burst of highs; at a second's travel not even a larger step than running open makes
    CHECK(normal.hfTravel <= 1.05 * normal.hfOpen);
    CHECK(slow.stepTravel <= 1.01 * slow.stepOpen);
}

// Card G, test 9: no converter: less back pressure at the head, more highs at the tails. Zoomies: as many radiating
// ends as cylinders.
TEST_CASE("no converter is freer and brighter; zoomies radiate from every cylinder", "[exhaust][character]")
{
    const Heard cat = listen(12, {});
    const Heard bare = listen(12, bothSides("converter", 0));
    const double hiCat = powerAbove(cat.sum, 2000.0, 0, kAll);
    const double hiBare = powerAbove(bare.sum, 2000.0, 0, kAll);
    std::printf("no converter: back pressure at the head %.0f Pa against %.0f; above 2 kHz %+.1f dB\n", bare.pConv,
                cat.pConv, db(hiBare / hiCat));
    CHECK(bare.pConv < cat.pConv);
    CHECK(hiBare > hiCat);

    for (int layout : {5, 12, 15})
    {
        Rig rig(layout);
        const int n = rig.cylinders();
        for (int i = 0; i < n; ++i)
            rig.set("b.sp.primaryTo[" + std::to_string(i) + "]", -1);
        rig.configure();
        INFO("layout " << layout);
        CHECK(rig.ends() == n);
    }
}

