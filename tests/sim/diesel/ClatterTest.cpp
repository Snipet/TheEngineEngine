// tests/sim/diesel/ClatterTest.cpp: card D2: a diesel's idle clatter is a hump, not a comb. Its premixed burn strikes the
// head and block, ringing their panels (the reference's clatter, sim/sound/extras.fm); struck at the same moment every
// cycle those modes ring as sharp lines at the firing frequency's harmonics, a standing tone (card P's idle row found
// three diesel archetypes with lines 1 to 10 dB under their exhaust's note). Under the physical diesel (fuel.model 1)
// the start of combustion takes the ignition delay's own cycle-to-cycle spread (sim/combustion/diesel.fm, socSpread),
// which smears the harmonics above a kilohertz into a hump, as a real engine's.
#include "Diesel.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdio>
#include <vector>

using d2::Engine;

namespace
{
    // x's power spectrum averaged over segments of 2^bits samples, Hann-windowed, half overlapping (Welch's), by an
    // in-place radix-2 FFT
    std::vector<double> welch(const std::vector<float>& x, int bits)
    {
        const std::size_t n = std::size_t(1) << bits;
        std::vector<double> p(n / 2, 0.0);
        std::vector<std::complex<double>> a(n);
        int segs = 0;
        for (std::size_t s0 = 0; s0 + n <= x.size(); s0 += n / 2, ++segs)
        {
            for (std::size_t i = 0; i < n; ++i)
                a[i] = (0.5 - 0.5 * std::cos(6.283185307179586 * double(i) / double(n - 1))) * double(x[s0 + i]);
            for (std::size_t i = 1, j = 0; i < n; ++i)
            {
                std::size_t bit = n >> 1;
                for (; (j & bit) != 0; bit >>= 1)
                    j ^= bit;
                j ^= bit;
                if (i < j)
                    std::swap(a[i], a[j]);
            }
            for (std::size_t len = 2; len <= n; len <<= 1)
            {
                const std::complex<double> w(std::cos(-6.283185307179586 / double(len)), std::sin(-6.283185307179586 / double(len)));
                for (std::size_t i = 0; i < n; i += len)
                {
                    std::complex<double> wk(1.0, 0.0);
                    for (std::size_t k = 0; k < len / 2; ++k)
                    {
                        const std::complex<double> u = a[i + k], v = a[i + k + len / 2] * wk;
                        a[i + k] = u + v;
                        a[i + k + len / 2] = u - v;
                        wk *= w;
                    }
                }
            }
            for (std::size_t i = 0; i < n / 2; ++i)
                p[i] += std::norm(a[i]);
        }
        return p;
    }

    struct Idle
    {
        double rpm = 0.0;
        double delay = 0.0;     // degrees, the model's ignition delay
        double depth = 0.0;     // dB: the firing harmonics between 1 and 3 kHz over the spectrum midway between them
    };

    // An archetype warm at idle in the car, in neutral (card P's p-idle-warm), a six. spread: socSpread (below 0: the
    // model's own)
    Idle warmIdle(const std::string& build, double spread)
    {
        Engine e(build);
        e.spec("env.temperature", 300.0);
        e.spec("env.pressure", 101325.0);
        e.spec("sound.autoLevel", 0);
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
        play(4.0);
        e.recording = true;
        play(5.5);
        Idle r;
        r.rpm = e.tel("engine.speed") * 9.5492966;
        r.delay = e.get("comb.diesel.delayDeg");
        // (2^16 samples: 0.73 Hz bins against the harmonics' 35 Hz; each harmonic's most within 1.5 Hz, and the mean
        // within 1.5 Hz of the point midway to the next: summed over the harmonics from 1 to 3 kHz)
        const auto p = welch(e.rec, 16);
        const double df = e.rate / 65536.0;
        // (the firing frequency from the spectrum itself, within 2 % of the revs' (an instant's reading misses the 40th
        // harmonic by some 4 Hz): the one whose harmonics from 1 to 3 kHz hold the most)
        auto at = [&](double f) { return p[static_cast<std::size_t>(f / df + 0.5)]; };
        double fire = r.rpm / 60.0 * 3.0, bestSum = 0.0;
        for (int j = -400; j <= 400; ++j)
        {
            const double f = r.rpm / 60.0 * 3.0 * (1.0 + 0.02 * j / 400.0);
            double sum = 0.0;
            for (int h = static_cast<int>(std::ceil(1000.0 / f)); h * f <= 3000.0; ++h)
                sum += at(h * f);
            if (sum > bestSum)
            {
                bestSum = sum;
                fire = f;
            }
        }
        double on = 0.0, mid = 0.0;
        for (int h = static_cast<int>(std::ceil(1000.0 / fire)); h * fire <= 3000.0; ++h)
        {
            double most = 0.0, sum = 0.0;
            int cnt = 0;
            for (std::size_t i = std::size_t((h * fire - 1.5) / df); i <= std::size_t((h * fire + 1.5) / df) + 1; ++i)
                most = std::max(most, p[i]);
            for (std::size_t i = std::size_t(((h + 0.5) * fire - 1.5) / df); i <= std::size_t(((h + 0.5) * fire + 1.5) / df) + 1; ++i)
            {
                sum += p[i];
                ++cnt;
            }
            on += most;
            mid += sum / cnt;
        }
        r.depth = 10.0 * std::log10(on / mid);
        return r;
    }
} // namespace

TEST_CASE("diesel clatter: at a warm idle the tractor's clatter is a hump, not a comb", "[diesel][injection][sound]")
{
    // (the bounds, judgement: with the spread the firing harmonics from 1 to 3 kHz stand at most 7 dB over the spectrum
    // midway between them, and some 2 dB less than without it. Measured 25 Nov: 14.2 dB without, 6.0 with the model's
    // (0.150 a degree of the 3.60-degree idle delay: 0.54 degrees); on 24 Nov's tip 9.3 and 5.5. What remains with the
    // spread is not only the clatter: the exhaust's blowdown, as periodic as the revs, has its own harmonics there, as
    // a real engine's)
    const Idle off = warmIdle("archetypes/tractor-15-inline-six", 0.0);
    const Idle on = warmIdle("archetypes/tractor-15-inline-six", -1.0);
    std::printf("tractor warm idle %.0f rpm, ignition delay %.2f deg: the firing harmonics 1-3 kHz over the spectrum between "
                "them: %.1f dB with no spread, %.1f dB with the model's\n", on.rpm, on.delay, off.depth, on.depth);
    CHECK(on.depth <= 7.0);
    CHECK(on.depth < off.depth - 2.0);
}
