// tests/sim/diesel/ClatterTest.cpp: card D2: a diesel's idle clatter is struck at the start of combustion; struck at the
// same moment every cycle its panels ring as sharp lines at the firing harmonics (card P's idle row found three diesel
// archetypes so). Under the physical diesel (fuel.model 1) the start of combustion takes the ignition delay's own
// cycle-to-cycle spread (sim/combustion/diesel.fm, socSpread). This tests that mechanism; the sound is P's diesel row.
#include "Diesel.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdio>
#include <vector>

using d2::Engine;


TEST_CASE("diesel clatter: the start of combustion takes the ignition delay's spread at a warm idle", "[diesel][injection]")
{
    // What D2's code does, which no other package moves: over 200 charges at the tractor's warm idle the start of
    // combustion's offset from its mean (DieselFields::socLast: the reference's rough x its gauss(), whose sigma is
    // 0.577, and under fuel.model 1 socSpread x the delay, a sigma) has a standard deviation of
    // sqrt((0.577 rough)^2 + (socSpread x delay)^2) within 15 %, and without the spread, the reference's alone. (Its
    // first form read 0.58 of what it meant: the spread had taken gauss() for a unit sigma, 24-25 Nov.)
    // (It was a sound test until 25 Nov: the firing harmonics from 1 to 3 kHz over the spectrum between them, with the
    // spread against without. That comb is mostly the engine's other periodic sound (the exhaust's blowdown, the block),
    // and it moved with every valve and turbine change: without the spread, 13.2 dB before 68e683f1, 13.1 after it
    // (valve area: the port's limit), 5.5 after 6771c402 (four-valve curtain 0.66/0.63), 11.0 after 7e9bee72 (N2's
    // compressor map), 7.0 after 29684891 (design flow at the charge's temperature); with it 4.1 to 6.4 throughout. The
    // sound is card P's diesel row now, in the mix at the archetypes' own settings: comb <= 8 dB; the lead's ruling)
    for (double spread : {-1.0, 0.0})
    {
        Engine e("archetypes/tractor-15-inline-six");
        e.spec("env.temperature", 300.0);
        e.spec("env.pressure", 101325.0);
        if (spread >= 0.0)
            e.write("comb.diesel.socSpread", spread);
        auto& c = e.ctl();
        c.mode = 1;
        c.lever = 0;
        c.throttle = 0.0f;
        c.ignition = tee::EnginePlayer::kIgnitionStart;
        auto play = [&](double s) {
            const int calls = static_cast<int>(s * e.rate / 512.0 + 0.5);
            for (int k = 0; k < calls; ++k)
            {
                e.holdWarm();
                REQUIRE(e.run(512.0 / e.rate));
            }
        };
        play(1.75);
        c.ignition = tee::EnginePlayer::kIgnitionOn;
        play(3.0);
        // (a charge every 29 ms at 700 rpm on six; read every 512 samples, 11 ms: each charge's value once, as it changes)
        std::vector<double> x;
        double was = e.get("comb.diesel.socLast"), delay = 0.0;
        while (x.size() < 200)
        {
            e.holdWarm();
            REQUIRE(e.run(512.0 / e.rate));
            const double v = e.get("comb.diesel.socLast");
            if (v != was)
            {
                x.push_back(v);
                was = v;
                delay += e.get("comb.diesel.delayDeg") / 200.0;
            }
        }
        double m = 0.0, s2 = 0.0;
        for (double v : x)
            m += v / 200.0;
        for (double v : x)
            s2 += (v - m) * (v - m) / 199.0;
        const double rough = e.get("spec.engine.rough");
        const double k = spread >= 0.0 ? spread : e.get("comb.diesel.socSpread");
        const double want = std::sqrt(0.57735 * rough * 0.57735 * rough + k * delay * k * delay);
        std::printf("tractor warm idle, socSpread %.3f, delay %.2f deg, rough %.2f: the start of combustion's sigma %.3f deg "
                    "(expected %.3f)\n", k, delay, rough, std::sqrt(s2), want);
        CHECK(std::sqrt(s2) > 0.85 * want);
        CHECK(std::sqrt(s2) < 1.15 * want);
    }
}
