// ui/views/scope/Spectrum.cpp: see Spectrum.h.
#include "views/scope/Spectrum.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace tee::ui::scope
{
    namespace
    {
        constexpr float kNaNf = std::numeric_limits<float>::quiet_NaN();
    }

    Spectrum::Spectrum()
        : window_(kSize), re_(kSize), im_(kSize), power_(kBins, 0.0f), bitrev_(kSize), cos_(kSize / 2), sin_(kSize / 2)
    {
        // Hann: 0.5 - 0.5 cos(2 pi i / N), periodic (the transform's own period).
        windowSum_ = 0.0;
        for (int i = 0; i < kSize; ++i)
        {
            window_[static_cast<std::size_t>(i)] = static_cast<float>(0.5 - 0.5 * std::cos(2.0 * kPi * i / kSize));
            windowSum_ += window_[static_cast<std::size_t>(i)];
        }
        int bits = 0;
        while ((1 << bits) < kSize)
            ++bits;
        for (int i = 0; i < kSize; ++i)
        {
            int r = 0;
            for (int b = 0; b < bits; ++b)
                r |= ((i >> b) & 1) << (bits - 1 - b);
            bitrev_[static_cast<std::size_t>(i)] = r;
        }
        for (int k = 0; k < kSize / 2; ++k)
        {
            cos_[static_cast<std::size_t>(k)] = std::cos(2.0 * kPi * k / kSize);
            sin_[static_cast<std::size_t>(k)] = std::sin(2.0 * kPi * k / kSize);
        }
    }

    void Spectrum::clear()
    {
        std::fill(power_.begin(), power_.end(), 0.0f);
        frames_ = 0;
    }

    void Spectrum::analyse(const float* samples, int n, double rate, float dt, float smooth)
    {
        if (rate > 0.0)
            rate_ = rate;
        // The newest kSize samples, windowed, in bit-reversed order.
        const int have = std::clamp(n, 0, kSize);
        const int pad = kSize - have;
        for (int i = 0; i < kSize; ++i)
        {
            const float x = i < pad || samples == nullptr ? 0.0f : samples[n - have + (i - pad)];
            const int j = bitrev_[static_cast<std::size_t>(i)];
            re_[static_cast<std::size_t>(j)] = std::isfinite(x) ? x * window_[static_cast<std::size_t>(i)] : 0.0f;
            im_[static_cast<std::size_t>(j)] = 0.0f;
        }
        // Radix-2, decimation in time.
        for (int len = 2; len <= kSize; len <<= 1)
        {
            const int half = len >> 1, stride = kSize / len;
            for (int s = 0; s < kSize; s += len)
                for (int k = 0; k < half; ++k)
                {
                    const double wr = cos_[static_cast<std::size_t>(k * stride)];
                    const double wi = -sin_[static_cast<std::size_t>(k * stride)];
                    const std::size_t a = static_cast<std::size_t>(s + k), b = a + static_cast<std::size_t>(half);
                    const double tr = wr * re_[b] - wi * im_[b], ti = wr * im_[b] + wi * re_[b];
                    re_[b] = static_cast<float>(re_[a] - tr);
                    im_[b] = static_cast<float>(im_[a] - ti);
                    re_[a] = static_cast<float>(re_[a] + tr);
                    im_[a] = static_cast<float>(im_[a] + ti);
                }
        }
        // Power against a full-scale sine: |X| of a sine of amplitude A is A * sum(w) / 2.
        const double norm = 2.0 / windowSum_;
        const float a = frames_ == 0 || !(dt > 0.0f) || !(smooth > 0.0f)
                            ? 1.0f
                            : static_cast<float>(1.0 - std::exp(-static_cast<double>(dt) / static_cast<double>(smooth)));
        for (int k = 0; k < kBins; ++k)
        {
            const double m = std::hypot(static_cast<double>(re_[static_cast<std::size_t>(k)]),
                                        static_cast<double>(im_[static_cast<std::size_t>(k)])) * norm;
            const float p = static_cast<float>(m * m);
            float& q = power_[static_cast<std::size_t>(k)];
            q += (p - q) * a;
        }
        ++frames_;
    }

    float Spectrum::db(int k) const noexcept
    {
        if (k < 0 || k >= kBins)
            return kFloorDb;
        const float p = power_[static_cast<std::size_t>(k)];
        return p > 0.0f ? std::max(kFloorDb, 10.0f * std::log10(p)) : kFloorDb;
    }

    float Spectrum::bandDb(double f0, double f1) const noexcept
    {
        const double bw = binHz();
        const double k0 = f0 / bw, k1 = f1 / bw;
        const int i0 = static_cast<int>(std::ceil(k0)), i1 = static_cast<int>(std::floor(k1));
        if (i1 >= i0)
        {
            float best = kFloorDb;
            for (int k = std::max(0, i0); k <= std::min(kBins - 1, i1); ++k)
                best = std::max(best, db(k));
            return best;
        }
        // Narrower than a bin: between the two bins round its middle.
        const double km = 0.5 * (k0 + k1);
        const int k = static_cast<int>(std::floor(km));
        const float t = static_cast<float>(km - k);
        return db(k) + (db(k + 1) - db(k)) * t;
    }

    double Spectrum::peakHz(double f0, double f1) const noexcept
    {
        if (empty())
            return std::numeric_limits<double>::quiet_NaN();
        const int i0 = std::max(1, static_cast<int>(std::ceil(f0 / binHz())));
        const int i1 = std::min(kBins - 1, static_cast<int>(std::floor(f1 / binHz())));
        int best = -1;
        for (int k = i0; k <= i1; ++k)
            if (best < 0 || power_[static_cast<std::size_t>(k)] > power_[static_cast<std::size_t>(best)])
                best = k;
        return best < 0 ? std::numeric_limits<double>::quiet_NaN() : best * binHz();
    }

    // ---- Campbell ---------------------------------------------------------------------------------------------------
    Campbell::Campbell(int cols, int rows, double rpm0, double rpm1, double f0, double f1)
        : cols_(std::max(2, cols)), rows_(std::max(2, rows)), rpm0_(rpm0), rpm1_(std::max(rpm0 + 1.0, rpm1)), f0_(f0),
          f1_(std::max(f0 * 1.01, f1))
    {
        clear();
    }

    void Campbell::clear()
    {
        cells_.assign(static_cast<std::size_t>(cols_ * rows_), kNaNf);
        seen_.assign(static_cast<std::size_t>(cols_), false);
        filled_ = 0;
        ++revision_;
    }

    void Campbell::setSpeedRange(double rpm0, double rpm1)
    {
        rpm1 = std::max(rpm0 + 1.0, rpm1);
        if (rpm0 == rpm0_ && rpm1 == rpm1_)
            return;
        rpm0_ = rpm0;
        rpm1_ = rpm1;
        clear();
    }

    double Campbell::rowLo(int r) const noexcept
    {
        return f0_ * std::pow(f1_ / f0_, static_cast<double>(r) / rows_);
    }

    int Campbell::columnOf(double rpm) const noexcept
    {
        if (!std::isfinite(rpm) || rpm < rpm0_ || rpm >= rpm1_)
            return -1;
        return std::clamp(static_cast<int>((rpm - rpm0_) / (rpm1_ - rpm0_) * cols_), 0, cols_ - 1);
    }

    void Campbell::take(double rpm, const Spectrum& s)
    {
        const int c = columnOf(rpm);
        if (c < 0 || s.empty())
            return;
        for (int r = 0; r < rows_; ++r)
            cells_[static_cast<std::size_t>(c * rows_ + r)] = s.bandDb(rowLo(r), rowHi(r));
        if (!seen_[static_cast<std::size_t>(c)])
        {
            seen_[static_cast<std::size_t>(c)] = true;
            ++filled_;
        }
        ++revision_;
    }
}
