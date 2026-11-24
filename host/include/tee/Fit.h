// host/include/tee/Fit.h: whether the simulator fits the machine, a verdict from the load the audio callback measures
// (HostStats::fits). The callback measures its own time over its buffer's (load) and the largest over the last second
// (loadPeak); nothing acted on them. This says, with hysteresis so it does not flicker:
//
//    1  fits
//    0  tight: loadPeak over 0.7 held for 2 s (a heavier moment, another program's burst, and it will drop out)
//   -1  over: loadPeak over 0.9 held for 2 s, or a late callback (one whose work took longer than its buffer lasts:
//       the device's queue ran dry, or would have, unless it held more than one buffer)
//
// It comes back one level at a time, after the peak has stayed under 0.6 (to fits) or 0.8 (to tight) for 5 s. It never
// changes anything by itself: the interface shows it and names the Economy quality (SimHost::setQuality).
//
// What it cannot tell: an underrun as the device saw it. SDL3 reports none (no count, no callback for it), and a
// device holding more than a buffer survives a late callback; a late callback is counted as the nearest the host knows.
// Its clock is the audio's (samples played), so an offline render's verdict is the same on every run of a machine.
//
//   tee::FitMeter meter;
//   meter.update(stats.loadPeak, stats.lateCallbacks, seconds);   // off the audio thread, every few milliseconds
//   if (meter.verdict() < 0) topBar.say("over: try Economy");
#pragma once

#include <cstdint>

namespace tee
{
    struct FitOptions
    {
        double tight = 0.7;           // loadPeak over this, held holdSeconds: tight
        double over = 0.9;            // ... over this: over
        double holdSeconds = 2.0;
        double clearTight = 0.6;      // under this, held clearSeconds: back from tight to fits
        double clearOver = 0.8;       // under this, held clearSeconds: back from over to tight
        double clearSeconds = 5.0;
    };

    class FitMeter
    {
    public:
        explicit FitMeter(FitOptions options = {}) : options_(options) {}

        // The newest loadPeak and late-callback count, `seconds` of audio after the last update. The verdict.
        int update(double loadPeak, std::uint32_t lateCallbacks, double seconds) noexcept
        {
            if (seconds < 0.0)
                seconds = 0.0;
            if (lateCallbacks < late_)   // (counted afresh: a new load)
                reset();
            if (lateCallbacks > late_)
            {
                late_ = lateCallbacks;
                verdict_ = -1;
                below_ = 0.0;
                return verdict_;
            }
            aboveOver_ = loadPeak > options_.over ? aboveOver_ + seconds : 0.0;
            aboveTight_ = loadPeak > options_.tight ? aboveTight_ + seconds : 0.0;
            if (aboveOver_ >= options_.holdSeconds)
            {
                verdict_ = -1;
                below_ = 0.0;
                return verdict_;
            }
            if (verdict_ == 1 && aboveTight_ >= options_.holdSeconds)
            {
                verdict_ = 0;
                below_ = 0.0;
                return verdict_;
            }
            const double clear = verdict_ < 0 ? options_.clearOver : options_.clearTight;
            if (verdict_ < 1 && loadPeak < clear)
            {
                below_ += seconds;
                if (below_ >= options_.clearSeconds)
                {
                    ++verdict_;
                    below_ = 0.0;
                }
            }
            else
                below_ = 0.0;
            return verdict_;
        }

        int verdict() const noexcept { return verdict_; }

        void reset() noexcept
        {
            verdict_ = 1;
            late_ = 0;
            aboveOver_ = aboveTight_ = below_ = 0.0;
        }

    private:
        FitOptions options_;
        int verdict_ = 1;
        std::uint32_t late_ = 0;
        double aboveOver_ = 0.0, aboveTight_ = 0.0, below_ = 0.0;
    };
} // namespace tee
