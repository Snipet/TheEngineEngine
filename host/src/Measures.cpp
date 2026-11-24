// host/src/Measures.cpp: tee::measure (Measures.h).
#include "tee/Measures.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <limits>

namespace tee::measure
{
    namespace
    {
        constexpr double kPi = 3.14159265358979323846;

        double hann(std::size_t i, std::size_t n)
        {
            return n < 2 ? 1.0 : 0.5 - 0.5 * std::cos(2.0 * kPi * static_cast<double>(i) / static_cast<double>(n - 1));
        }

        double goertzel(std::span<const float> x, double rate, double hz)
        {
            const std::size_t n = x.size();
            const double w = 2.0 * kPi * hz / rate;
            const double c = 2.0 * std::cos(w);
            double s1 = 0.0, s2 = 0.0, sumW = 0.0;
            for (std::size_t i = 0; i < n; ++i)
            {
                const double win = hann(i, n);
                sumW += win;
                const double s = static_cast<double>(x[i]) * win + c * s1 - s2;
                s2 = s1;
                s1 = s;
            }
            const double re = s1 - s2 * std::cos(w);
            const double im = s2 * std::sin(w);
            return sumW > 0.0 ? 2.0 * std::sqrt(re * re + im * im) / sumW : 0.0;
        }
    } // namespace

    bool finite(std::span<const float> x) noexcept
    {
        for (const float v : x)
            if (!std::isfinite(v))
                return false;
        return true;
    }

    double peak(std::span<const float> x) noexcept
    {
        double p = 0.0;
        for (const float v : x)
            p = std::max(p, std::fabs(static_cast<double>(v)));
        return p;
    }

    double rms(std::span<const float> x) noexcept
    {
        if (x.empty())
            return 0.0;
        double s = 0.0;
        for (const float v : x)
            s += static_cast<double>(v) * v;
        return std::sqrt(s / static_cast<double>(x.size()));
    }

    double dB(double amplitude) noexcept
    {
        return amplitude > 0.0 ? 20.0 * std::log10(amplitude) : -300.0;
    }

    double lineDb(std::span<const float> x, double rate, double hz)
    {
        double best = 0.0;
        for (int k = -6; k <= 6; ++k)
            best = std::max(best, goertzel(x, rate, hz * (1.0 + 0.002 * k)));
        return dB(best);
    }

    Spectrum spectrum(std::span<const float> x, double rate)
    {
        Spectrum out;
        std::size_t n = 1;
        while (n * 2 <= x.size() && n < (std::size_t{1} << 18))
            n *= 2;
        if (n < 2)
            return out;
        std::vector<std::complex<double>> a(n);
        double sumW = 0.0;
        for (std::size_t i = 0; i < n; ++i)
        {
            const double w = hann(i, n);
            sumW += w;
            a[i] = static_cast<double>(x[i]) * w;
        }
        // iterative radix-2
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
            const double angle = -2.0 * kPi / static_cast<double>(len);
            const std::complex<double> wl(std::cos(angle), std::sin(angle));
            for (std::size_t i = 0; i < n; i += len)
            {
                std::complex<double> w(1.0, 0.0);
                for (std::size_t j = 0; j < len / 2; ++j)
                {
                    const std::complex<double> u = a[i + j];
                    const std::complex<double> v = a[i + j + len / 2] * w;
                    a[i + j] = u + v;
                    a[i + j + len / 2] = u - v;
                    w *= wl;
                }
            }
        }
        out.binHz = rate / static_cast<double>(n);
        out.db.resize(n / 2);
        for (std::size_t k = 0; k < n / 2; ++k)
            out.db[k] = dB(2.0 * std::abs(a[k]) / sumW);
        return out;
    }

    double Spectrum::floorDb(double low, double high) const
    {
        std::vector<double> band;
        for (std::size_t k = 0; k < db.size(); ++k)
        {
            const double f = static_cast<double>(k) * binHz;
            if (f >= low && f <= high)
                band.push_back(db[k]);
        }
        if (band.empty())
            return -300.0;
        std::nth_element(band.begin(), band.begin() + static_cast<std::ptrdiff_t>(band.size() / 2), band.end());
        return band[band.size() / 2];
    }

    double Spectrum::peakHz(double low, double high) const
    {
        double best = -1e300, at = 0.0;
        for (std::size_t k = 0; k < db.size(); ++k)
        {
            const double f = static_cast<double>(k) * binHz;
            if (f >= low && f <= high && db[k] > best)
            {
                best = db[k];
                at = f;
            }
        }
        return at;
    }

    double largestStep(std::span<const float> x, double rate)
    {
        const std::size_t n = x.size();
        if (n < 3)
            return 0.0;
        const std::size_t half = std::max<std::size_t>(8, static_cast<std::size_t>(0.01 * rate));
        // prefix sums of the steps' squares
        std::vector<double> prefix(n, 0.0);
        for (std::size_t i = 1; i < n; ++i)
        {
            const double d = static_cast<double>(x[i]) - x[i - 1];
            prefix[i] = prefix[i - 1] + d * d;
        }
        double worst = 0.0;
        for (std::size_t i = 1; i < n; ++i)
        {
            const double d = static_cast<double>(x[i]) - x[i - 1];
            const std::size_t lo = i > half ? i - half : 1;
            const std::size_t hi = std::min(n - 1, i + half);
            const double around = prefix[hi] - prefix[lo - 1] - d * d;
            const double count = static_cast<double>(hi - lo + 1) - 1.0;
            const double local = count > 0.0 ? std::sqrt(std::max(around, 0.0) / count) : 0.0;
            worst = std::max(worst, std::fabs(d) / std::max(local, 1e-4));
        }
        return worst;
    }

    double nullDepth(std::span<const float> a, std::span<const float> b)
    {
        const std::size_t n = std::min(a.size(), b.size());
        double s = 0.0, d = 0.0;
        for (std::size_t i = 0; i < n; ++i)
        {
            const double x = a[i];
            const double e = x - static_cast<double>(b[i]);
            s += x * x;
            d += e * e;
        }
        if (d == 0.0)
            return std::numeric_limits<double>::infinity();
        if (s == 0.0)
            return -std::numeric_limits<double>::infinity();
        return 10.0 * std::log10(s / d);
    }
} // namespace tee::measure
