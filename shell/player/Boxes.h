// shell/player/Boxes.h: how values cross between the UI thread and the audio thread in the shell's Players, with no
// lock and no allocation: every value is an atomic of its own. A reader may see two fields of different writes for a
// block (a throttle of this frame beside a brake of the last), which nothing here minds: each field is a level or a
// count that stands on its own.
#pragma once

#include "tee/app/Sources.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <memory>

namespace tee::player
{
    // The newest ControlsFrame. set() from any thread; get() on the audio thread.
    class ControlsBox
    {
    public:
        void set(const ControlsFrame& f) noexcept
        {
            ignition_.store(f.ignition, std::memory_order_relaxed);
            throttle_.store(f.throttle, std::memory_order_relaxed);
            brake_.store(f.brake, std::memory_order_relaxed);
            clutch_.store(f.clutch, std::memory_order_relaxed);
            handbrake_.store(f.handbrake, std::memory_order_relaxed);
            steer_.store(f.steer, std::memory_order_relaxed);
            lever_.store(f.lever, std::memory_order_relaxed);
            shiftUp_.store(f.shiftUp, std::memory_order_relaxed);
            shiftDown_.store(f.shiftDown, std::memory_order_relaxed);
            buttons_.store(f.buttons, std::memory_order_relaxed);
            mode_.store(f.mode, std::memory_order_relaxed);
        }

        ControlsFrame get() const noexcept
        {
            ControlsFrame f;
            f.ignition = ignition_.load(std::memory_order_relaxed);
            f.throttle = throttle_.load(std::memory_order_relaxed);
            f.brake = brake_.load(std::memory_order_relaxed);
            f.clutch = clutch_.load(std::memory_order_relaxed);
            f.handbrake = handbrake_.load(std::memory_order_relaxed);
            f.steer = steer_.load(std::memory_order_relaxed);
            f.lever = lever_.load(std::memory_order_relaxed);
            f.shiftUp = shiftUp_.load(std::memory_order_relaxed);
            f.shiftDown = shiftDown_.load(std::memory_order_relaxed);
            f.buttons = buttons_.load(std::memory_order_relaxed);
            f.mode = mode_.load(std::memory_order_relaxed);
            return f;
        }

    private:
        std::atomic<int> ignition_{ 0 }, lever_{ 0 }, shiftUp_{ 0 }, shiftDown_{ 0 }, buttons_{ 0 }, mode_{ 0 };
        std::atomic<float> throttle_{ 0 }, brake_{ 0 }, clutch_{ 0 }, handbrake_{ 0 }, steer_{ 0 };
    };

    // n values one thread writes and another reads (the parameters one way, the telemetry the other). A writer marks
    // a value changed; the reader takes the changed ones.
    class ValueBox
    {
    public:
        explicit ValueBox(std::size_t n = 0) { resize(n); }

        // Not while another thread reads or writes.
        void resize(std::size_t n)
        {
            n_ = n;
            values_ = std::make_unique<std::atomic<double>[]>(n);
            changed_ = std::make_unique<std::atomic<bool>[]>(n);
            for (std::size_t i = 0; i < n; ++i)
            {
                values_[i].store(0.0, std::memory_order_relaxed);
                changed_[i].store(false, std::memory_order_relaxed);
            }
        }

        std::size_t size() const noexcept { return n_; }

        void set(std::size_t i, double v) noexcept
        {
            if (i < n_)
            {
                values_[i].store(v, std::memory_order_relaxed);
                changed_[i].store(true, std::memory_order_release);
            }
        }

        double get(std::size_t i) const noexcept
        {
            return i < n_ ? values_[i].load(std::memory_order_relaxed) : 0.0;
        }

        // True, and the value, when it was set since the last take.
        bool take(std::size_t i, double& v) noexcept
        {
            if (i >= n_ || !changed_[i].exchange(false, std::memory_order_acquire))
                return false;
            v = values_[i].load(std::memory_order_relaxed);
            return true;
        }

    private:
        std::size_t n_ = 0;
        std::unique_ptr<std::atomic<double>[]> values_;
        std::unique_ptr<std::atomic<bool>[]> changed_;
    };

    // The output's newest samples, mono, for whoever draws them (TelemetrySource::outputTail): the audio thread
    // writes each block's, the UI thread copies the newest out. No lock: the reader copies, then looks whether the
    // writer came round to what it copied meanwhile, and copies again if so (a few times at most; a picture of the
    // sound survives a stale sample).
    class TailRing
    {
    public:
        static constexpr std::size_t kSize = 8192;      // a power of two; outputTail gives up to all of it

        // The audio thread: n samples of both channels.
        void write(const float* l, const float* r, int n) noexcept
        {
            std::uint64_t w = written_.load(std::memory_order_relaxed);
            for (int i = 0; i < n; ++i)
                ring_[static_cast<std::size_t>((w + static_cast<std::uint64_t>(i)) & (kSize - 1))].store(
                    0.5f * (l[i] + r[i]), std::memory_order_relaxed);
            written_.store(w + static_cast<std::uint64_t>(n), std::memory_order_release);
        }

        // The UI thread: the newest min(max, kSize, written) samples, oldest first. Returns how many.
        int read(float* mono, int max) const noexcept
        {
            if (mono == nullptr || max <= 0)
                return 0;
            int n = 0;
            for (int attempt = 0; attempt < 4; ++attempt)
            {
                const std::uint64_t w = written_.load(std::memory_order_acquire);
                const std::uint64_t want = std::min<std::uint64_t>({ static_cast<std::uint64_t>(max), kSize, w });
                n = static_cast<int>(want);
                const std::uint64_t first = w - want;
                for (std::uint64_t i = 0; i < want; ++i)
                    mono[i] = ring_[static_cast<std::size_t>((first + i) & (kSize - 1))].load(std::memory_order_relaxed);
                // whole, unless the writer has since written over the oldest of them
                if (written_.load(std::memory_order_acquire) - first <= kSize)
                    break;
            }
            return n;
        }

    private:
        std::array<std::atomic<float>, kSize> ring_{};
        std::atomic<std::uint64_t> written_{ 0 };
    };

    // The largest magnitude in a block of both channels, 0 to 1; a sample that is not finite counts as full scale.
    inline float blockPeak(const float* l, const float* r, int n) noexcept
    {
        float peak = 0.0f;
        for (int i = 0; i < n; ++i)
        {
            const float a = std::fabs(l[i]), b = std::fabs(r[i]);
            const float m = a > b ? a : b;
            if (!(m <= peak))               // also true for a NaN
                peak = m == m ? m : 1.0f;
        }
        return peak > 1.0f ? 1.0f : peak;
    }
}
