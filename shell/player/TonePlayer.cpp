// shell/player/TonePlayer.cpp: TonePlayer.h. Arithmetic a child could follow: the speed moves toward a target at a
// fixed rate, and the sound is three partials of the firing frequency through a soft clip.
#include "player/TonePlayer.h"

#include <algorithm>
#include <cmath>

namespace tee::player
{
    namespace
    {
        constexpr double kTwoPi = 6.283185307179586;
        constexpr double kRedlineRpm = 6500.0;
        constexpr double kRpmToRad = kTwoPi / 60.0;
        constexpr std::array<std::string_view, 6> kTelNames{
            telemetry::kState, telemetry::kSpeed, telemetry::kRedline,
            telemetry::kThrottle, telemetry::kGear, telemetry::kDriven,
        };
    }

    TonePlayer::TonePlayer()
    {
        infos_[0] = { "level", "LEVEL", "dB", "TONE", ParamInfo::Kind::real, -40.0, 0.0, -12.0, {}, false, 1, 1.0 };
        infos_[1] = { "idle", "IDLE", "rpm", "TONE", ParamInfo::Kind::real, 500.0, 1500.0, 800.0, {}, false, 0, 1.0 };
        infos_[2] = { "cylinders", "CYLINDERS", "", "TONE", ParamInfo::Kind::choice, 0.0, 2.0, 2.0,
                      { "4", "6", "8" }, false, 0, 1.0 };
        for (int i = 0; i < kParams; ++i)
        {
            const auto u = static_cast<std::size_t>(i);
            shown_[u].store(infos_[u].def, std::memory_order_relaxed);
            params_.set(u, infos_[u].def);
        }
        tel_[tRedline].store(kRedlineRpm * kRpmToRad, std::memory_order_relaxed);
        tel_[tDriven].store(1.0, std::memory_order_relaxed);
    }

    const ParamInfo& TonePlayer::info(int i) const
    {
        return infos_[static_cast<std::size_t>(std::clamp(i, 0, kParams - 1))];
    }

    double TonePlayer::get(int i) const
    {
        return i >= 0 && i < kParams ? shown_[static_cast<std::size_t>(i)].load(std::memory_order_relaxed) : 0.0;
    }

    void TonePlayer::set(int i, double value)
    {
        if (i < 0 || i >= kParams || !(value == value))
            return;
        const auto u = static_cast<std::size_t>(i);
        double v = std::clamp(value, infos_[u].min, infos_[u].max);
        if (infos_[u].kind != ParamInfo::Kind::real)
            v = std::round(v);
        shown_[u].store(v, std::memory_order_relaxed);
        params_.set(u, v);
    }

    int TonePlayer::findParam(std::string_view id) const
    {
        for (int i = 0; i < kParams; ++i)
            if (infos_[static_cast<std::size_t>(i)].id == id)
                return i;
        return -1;
    }

    int TonePlayer::findValue(std::string_view name) const
    {
        for (std::size_t i = 0; i < kTelNames.size(); ++i)
            if (kTelNames[i] == name)
                return static_cast<int>(i);
        return -1;
    }

    double TonePlayer::value(int handle) const
    {
        return handle >= 0 && handle < tCount ? tel_[static_cast<std::size_t>(handle)].load(std::memory_order_relaxed)
                                              : 0.0;
    }

    void TonePlayer::prepare(double rate, int)
    {
        rate_ = rate > 1000.0 ? rate : 48000.0;
    }

    void TonePlayer::process(float* l, float* r, int n) noexcept
    {
        double v = 0.0;
        if (params_.take(0, v))
            level_ = std::pow(10.0, v / 20.0);
        if (params_.take(1, v))
            idleRpm_ = v;
        if (params_.take(2, v))
            cylinders_ = 4 + 2 * static_cast<int>(v);

        const ControlsFrame f = controls_.get();
        const double dt = static_cast<double>(n) / rate_;

        // the start key: cranks an engine that is off; held a second on one that was running, switches it off
        const bool start = f.ignition == 2;
        if (start && !startWas_)
        {
            startOnRunning_ = state_ == 2;
            offHeld_ = 0.0;
            if (state_ == 0 || state_ == 3)
                state_ = 1;
        }
        if (start && startOnRunning_ && state_ == 2)
        {
            offHeld_ += dt;
            if (offHeld_ >= 1.0)
                state_ = 3;
        }
        startWas_ = start;

        // the gears: a number, moved by the shifts' presses or the lever
        gear_ = std::clamp(gear_ + std::clamp(f.shiftUp - lastUp_, 0, 8) - std::clamp(f.shiftDown - lastDown_, 0, 8), -1, 6);
        lastUp_ = f.shiftUp;
        lastDown_ = f.shiftDown;
        if (f.lever != lastLever_)
        {
            lastLever_ = f.lever;
            gear_ = std::clamp(f.lever, -1, 6);
        }

        // the speed
        throttle_ = std::clamp(static_cast<double>(f.throttle), 0.0, 1.0);
        double target = 0.0, rate = 3000.0;                 // rpm, and rpm a second
        if (state_ == 1)
        {
            target = 250.0;
            rate = 600.0;
            if (rpm_ >= 240.0)
                state_ = 2;
        }
        if (state_ == 2)
        {
            target = idleRpm_ + throttle_ * (kRedlineRpm - idleRpm_);
            rate = target > rpm_ ? 6000.0 : 3500.0;
        }
        if (state_ == 3 && rpm_ < 5.0)
            state_ = 0;
        const double step = rate * dt;
        rpm_ = rpm_ < target ? std::min(rpm_ + step, target) : std::max(rpm_ - step, target);

        // the sound: three partials of the firing frequency
        const double firing = rpm_ / 60.0 * 0.5 * static_cast<double>(cylinders_);
        const double inc = firing / rate_;
        const double want = state_ == 0 ? 0.0 : level_ * (0.35 + 0.65 * throttle_) * std::min(rpm_ / 400.0, 1.0);
        const double glide = 1.0 - std::exp(-1.0 / (0.01 * rate_));
        for (int i = 0; i < n; ++i)
        {
            amp_ += (want - amp_) * glide;
            phase_ += inc;
            phase_ -= std::floor(phase_);
            const double a = kTwoPi * phase_;
            const double s = std::sin(a) + 0.5 * std::sin(2.0 * a) + 0.25 * std::sin(3.0 * a + 0.5);
            const auto out = static_cast<float>(std::tanh(0.6 * s) * amp_);
            l[i] = out;
            r[i] = out;
        }

        tel_[tState].store(static_cast<double>(state_), std::memory_order_relaxed);
        tel_[tSpeed].store(rpm_ * kRpmToRad, std::memory_order_relaxed);
        tel_[tThrottle].store(throttle_, std::memory_order_relaxed);
        tel_[tGear].store(static_cast<double>(gear_), std::memory_order_relaxed);
        peak_.store(blockPeak(l, r, n), std::memory_order_relaxed);
        blocks_.fetch_add(1, std::memory_order_relaxed);
        if (n > largest_.load(std::memory_order_relaxed))
            largest_.store(n, std::memory_order_relaxed);
    }
}
