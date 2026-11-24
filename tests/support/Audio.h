// tests/support/Audio.h: what the tests measure of rendered sound (namespace tee::test). Nobody here can listen, so a
// test claims numbers: how far two renders are apart, a level, a band's level, where a spectrum's strongest line is.
// Header-only; double arithmetic throughout.
#pragma once

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <limits>
#include <vector>

namespace tee::test
{
    inline constexpr double kPi = 3.14159265358979323846;

    // How far the difference of two renders lies under the first, in dB: 10 log10(sum a^2 / sum (a - b)^2) over the
    // samples both have. 60 means the difference is a thousandth of the signal; identical renders give infinity; a
    // silent `a` with a different `b` gives minus infinity. The measure the port is held to the reference by.
    inline double nullDepth(const std::vector<float>& a, const std::vector<float>& b)
    {
        const std::size_t n = std::min(a.size(), b.size());
        double signal = 0.0;
        double difference = 0.0;
        for (std::size_t i = 0; i < n; ++i)
        {
            const double x = a[i];
            const double d = x - static_cast<double>(b[i]);
            signal += x * x;
            difference += d * d;
        }
        if (difference == 0.0)
            return std::numeric_limits<double>::infinity();
        if (signal == 0.0)
            return -std::numeric_limits<double>::infinity();
        return 10.0 * std::log10(signal / difference);
    }

    // The largest difference between two renders' samples (over the samples both have).
    inline double maxDifference(const std::vector<float>& a, const std::vector<float>& b)
    {
        const std::size_t n = std::min(a.size(), b.size());
        double worst = 0.0;
        for (std::size_t i = 0; i < n; ++i)
            worst = std::max(worst, std::fabs(static_cast<double>(a[i]) - static_cast<double>(b[i])));
        return worst;
    }

    inline bool allFinite(const std::vector<float>& x)
    {
        return std::all_of(x.begin(), x.end(), [](float v) { return std::isfinite(v); });
    }

    inline double peak(const std::vector<float>& x)
    {
        double worst = 0.0;
        for (const float v : x)
            worst = std::max(worst, std::fabs(static_cast<double>(v)));
        return worst;
    }

    // The root mean square of samples [from, to), in dB re full scale (-infinity for silence).
    inline double levelDb(const std::vector<float>& x, std::size_t from, std::size_t to)
    {
        to = std::min(to, x.size());
        if (from >= to)
            return -std::numeric_limits<double>::infinity();
        double sum = 0.0;
        for (std::size_t i = from; i < to; ++i)
            sum += static_cast<double>(x[i]) * x[i];
        return 10.0 * std::log10(sum / static_cast<double>(to - from));
    }

    // x resampled from rate `from` to rate `to`: a Kaiser-windowed sinc (beta 9, 32 zero crossings a side), its cut at
    // 0.45 of the lower rate. Good to some 90 dB under 0.4 of the lower rate: what comparing renders made at different
    // rates needs, not a product's converter.
    inline std::vector<float> resample(const std::vector<float>& x, double from, double to)
    {
        if (from == to || x.empty())
            return x;
        const auto bessel = [](double v) {
            double sum = 1.0;
            double term = 1.0;
            for (int k = 1; k < 40; ++k)
            {
                term *= (v / (2.0 * k)) * (v / (2.0 * k));
                sum += term;
            }
            return sum;
        };
        const double cut = 0.45 * std::min(from, to) / from * 2.0;   // the cut over the input's Nyquist
        const double half = 32.0 / cut;                              // the kernel's half width, in input samples
        // the window, tabled over its half width (read between entries)
        const int steps = 8192;
        const double beta = 9.0;
        std::vector<double> window(static_cast<std::size_t>(steps) + 2);
        for (int i = 0; i <= steps + 1; ++i)
        {
            const double w = std::min(static_cast<double>(i) / steps, 1.0);
            window[static_cast<std::size_t>(i)] = bessel(beta * std::sqrt(1.0 - w * w)) / bessel(beta);
        }
        const std::size_t count = static_cast<std::size_t>(std::floor(static_cast<double>(x.size()) * to / from));
        std::vector<float> y(count);
        for (std::size_t k = 0; k < count; ++k)
        {
            const double t = static_cast<double>(k) * from / to;
            const long first = std::max(static_cast<long>(std::ceil(t - half)), 0L);
            const long last = std::min(static_cast<long>(std::floor(t + half)), static_cast<long>(x.size()) - 1);
            double sum = 0.0;
            for (long n = first; n <= last; ++n)
            {
                const double u = static_cast<double>(n) - t;
                const double at = std::fabs(u) / half * steps;
                const std::size_t i = static_cast<std::size_t>(at);
                const double shape = window[i] + (at - static_cast<double>(i)) * (window[i + 1] - window[i]);
                const double arg = kPi * cut * u;
                const double sinc = std::fabs(arg) < 1.0e-9 ? 1.0 : std::sin(arg) / arg;
                sum += static_cast<double>(x[static_cast<std::size_t>(n)]) * cut * sinc * shape;
            }
            y[k] = static_cast<float>(sum);
        }
        return y;
    }

    // An in-place radix-2 FFT (the size a power of two).
    inline void fft(std::vector<std::complex<double>>& a)
    {
        const std::size_t n = a.size();
        for (std::size_t i = 1, j = 0; i < n; ++i)
        {
            std::size_t bit = n >> 1;
            for (; (j & bit) != 0; bit >>= 1)
                j ^= bit;
            j ^= bit;
            if (i < j)
                std::swap(a[i], a[j]);
        }
        for (std::size_t length = 2; length <= n; length <<= 1)
        {
            const double angle = -2.0 * kPi / static_cast<double>(length);
            const std::complex<double> step(std::cos(angle), std::sin(angle));
            for (std::size_t i = 0; i < n; i += length)
            {
                std::complex<double> w(1.0, 0.0);
                for (std::size_t k = 0; k < length / 2; ++k)
                {
                    const std::complex<double> u = a[i + k];
                    const std::complex<double> v = a[i + k + length / 2] * w;
                    a[i + k] = u + v;
                    a[i + k + length / 2] = u - v;
                    w *= step;
                }
            }
        }
    }

    // The power spectrum of `size` samples of x from `from` (size a power of two; a Hann window): bin k is at
    // k * rate / size Hz. Samples past x's end count as 0.
    inline std::vector<double> powerSpectrum(const std::vector<float>& x, std::size_t from, std::size_t size)
    {
        std::vector<std::complex<double>> a(size);
        for (std::size_t i = 0; i < size; ++i)
        {
            const double window = 0.5 - 0.5 * std::cos(2.0 * kPi * static_cast<double>(i) / static_cast<double>(size));
            const double sample = from + i < x.size() ? static_cast<double>(x[from + i]) : 0.0;
            a[i] = {sample * window, 0.0};
        }
        fft(a);
        std::vector<double> power(size / 2 + 1);
        for (std::size_t k = 0; k < power.size(); ++k)
            power[k] = std::norm(a[k]);
        return power;
    }

    struct Line
    {
        double hz = 0.0;        // the line's frequency, between bins by its neighbours' weights
        double power = 0.0;     // its bin's power
    };

    // The strongest line of a power spectrum between lowHz and highHz.
    inline Line strongestLine(const std::vector<double>& power, double rate, double lowHz, double highHz)
    {
        const std::size_t size = (power.size() - 1) * 2;
        const double bin = rate / static_cast<double>(size);
        const std::size_t first = std::max<std::size_t>(static_cast<std::size_t>(std::ceil(lowHz / bin)), 1);
        const std::size_t last = std::min(static_cast<std::size_t>(std::floor(highHz / bin)), power.size() - 2);
        Line line;
        std::size_t best = first;
        for (std::size_t k = first; k <= last; ++k)
            if (power[k] > power[best])
                best = k;
        line.power = power[best];
        // a parabola through the peak's bin and its neighbours, in dB
        const double l = 10.0 * std::log10(power[best - 1] + 1.0e-300);
        const double c = 10.0 * std::log10(power[best] + 1.0e-300);
        const double r = 10.0 * std::log10(power[best + 1] + 1.0e-300);
        const double denominator = l - 2.0 * c + r;
        const double shift = denominator != 0.0 ? 0.5 * (l - r) / denominator : 0.0;
        line.hz = (static_cast<double>(best) + std::clamp(shift, -0.5, 0.5)) * bin;
        return line;
    }

    // The power of a spectrum between two frequencies, in dB (relative: for comparing two spectra of one size).
    inline double bandDb(const std::vector<double>& power, double rate, double lowHz, double highHz)
    {
        const std::size_t size = (power.size() - 1) * 2;
        const double bin = rate / static_cast<double>(size);
        double sum = 0.0;
        for (std::size_t k = 0; k < power.size(); ++k)
        {
            const double hz = static_cast<double>(k) * bin;
            if (hz >= lowHz && hz < highHz)
                sum += power[k];
        }
        return 10.0 * std::log10(sum + 1.0e-300);
    }
} // namespace tee::test
