// tests/sim/twostroke/PipeTest.cpp: the expansion chamber's tuning (card T): the trapped charge peaks where the pipe's
// returning wave says, a torque band "on the pipe", and how many steps a cone needs.
#include "Ts.h"

#include <cmath>
#include <cstdio>
#include <vector>

using tee::test::twostroke::Rig;

namespace
{
    struct Sweep
    {
        std::vector<double> rpm, fresh, torque, gain;
        double tuned = 0.0, c = 0.0;

        // The pipe's band: its gain (the trapped fresh charge over a straight pipe's at the same speed: the crankcase's
        // own filling, best at low speed, divided out) in the top third between its least and its most; the centre
        // weighted by how far above that it is (steadier than the single highest point)
        double centre() const
        {
            double lo = 1e30, hi = 0;
            for (double f : gain)
            {
                lo = std::min(lo, f);
                hi = std::max(hi, f);
            }
            const double cut = lo + (hi - lo) * 2.0 / 3.0;
            double w = 0, s = 0;
            for (std::size_t i = 0; i < rpm.size(); ++i)
                if (gain[i] > cut)
                {
                    w += gain[i] - cut;
                    s += (gain[i] - cut) * rpm[i];
                }
            return s / w;
        }
    };

    Sweep run(double scale, int steps, int chamber, double from, double to, double by)
    {
        Rig rig;
        for (const char* f : {"spec.headerLen", "spec.diffuserLen", "spec.bellyLen", "spec.baffleLen", "spec.stingerLen"})
            rig.set(f, rig.get(f) * scale);
        rig.set("spec.coneSteps", steps);
        rig.set("spec.chamber", chamber);
        rig.build();
        Sweep s;
        s.tuned = rig.get("ch.tuned");
        s.c = rig.get("ch.c");
        for (double rpm = from; rpm <= to + 1; rpm += by)
        {
            rig.run(rpm, 1.0, 1.0, 50);
            CHECK(rig.get("bad") == 0);
            s.rpm.push_back(rpm);
            s.fresh.push_back(rig.meanTrapped());
            s.torque.push_back(rig.meanTorque());
        }
        return s;
    }

    Sweep sweep(double scale, int steps = 8, int chamber = 1, double from = 5000, double to = 17000, double by = 500)
    {
        Sweep s = run(scale, steps, chamber, from, to, by);
        const Sweep straight = run(scale, steps, 0, from, to, by);
        for (std::size_t i = 0; i < s.rpm.size(); ++i)
            s.gain.push_back(s.fresh[i] / straight.fresh[i]);
        return s;
    }

    // Where the returning wave says: the plug the baffle cone sends back leaves with the blowdown's peak (some 25
    // degrees after the exhaust port opens, in the traces) and is back at the port 2 L / c later, L the length to the
    // baffle cone's middle; tuned when that is the port's closing: (exhaust duration - 25) degrees = 6 N 2 L / c
    double predicted(double tunedLength, double c, double exhaustDur = 192.0)
    {
        return c * (exhaustDur - 25.0) / (12.0 * tunedLength);
    }
} // namespace

TEST_CASE("the trapped charge peaks where the pipe's returning wave says, within 15 %", "[twostroke][pipe]")
{
    for (double scale : {0.8, 1.0, 1.25})
    {
        const Sweep s = sweep(scale);
        const double want = predicted(s.tuned, s.c), got = s.centre();
        std::printf("pipe x%.2f (to the baffle's middle %.3f m, c %.0f m/s): trapped charge's band centred at %.0f rpm, the "
                    "returning wave says %.0f (%+.1f %%)\n", scale, s.tuned, s.c, got, want, 100.0 * (got - want) / want);
        CHECK(std::abs(got - want) < 0.15 * want);
    }
}

TEST_CASE("on the pipe: a torque band the straight pipe does not have", "[twostroke][pipe]")
{
    const Sweep chamber = sweep(1.0);
    const Sweep straight = run(1.0, 8, 0, 5000, 17000, 500);
    const double centre = chamber.centre();
    double band = 0, below = 0, straightBand = 0;
    for (std::size_t i = 0; i < chamber.rpm.size(); ++i)
    {
        if (std::abs(chamber.rpm[i] - centre) <= 1000)
        {
            band = std::max(band, chamber.torque[i]);
            straightBand = std::max(straightBand, straight.torque[i]);
        }
        if (std::abs(chamber.rpm[i] - 0.7 * centre) <= 500)
            below = std::max(below, chamber.torque[i]);
    }
    std::printf("on the pipe (%.0f rpm +- 1000): %.2f N m; at 0.7 of it: %.2f N m; a straight pipe there: %.2f N m\n", centre,
                band, below, straightBand);
    for (std::size_t i = 0; i < chamber.rpm.size(); ++i)
        std::printf("  %6.0f rpm: chamber %5.2f N m (%5.1f mg), straight %5.2f N m (%5.1f mg)\n", chamber.rpm[i],
                    chamber.torque[i], chamber.fresh[i] * 1e6, straight.torque[i], straight.fresh[i] * 1e6);
    CHECK(band > 1.25 * below);
    CHECK(band > 1.25 * straightBand);
}

TEST_CASE("how many steps a cone needs before the tuned speed stops moving", "[twostroke][pipe]")
{
    double last = 0;
    int settled = 0;
    for (int k : {1, 2, 3, 4, 6, 8, 12, 16})
    {
        const double c = sweep(1.0, k, 1, 7000, 14000, 250).centre();
        const double move = last > 0 ? (c - last) / last : 0.0;
        std::printf("cones in %2d steps: the band centred at %.0f rpm (%+.1f %% from the last)\n", k, c, 100.0 * move);
        if (last > 0 && std::abs(move) < 0.02 && settled == 0)
            settled = k;
        last = c;
    }
    std::printf("the tuned speed moves under 2 %% from %d steps a cone\n", settled);
    CHECK(settled > 0);
    CHECK(settled <= 8);    // the default
}
