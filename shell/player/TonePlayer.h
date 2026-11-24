// shell/player/TonePlayer.h: a stand-in for an engine, with no simulator and no compiler behind it. It follows the
// same controls the real one does (the start key cranks it, it idles, the throttle revs it to a limiter, the shifts
// move a gear number) and makes a plain sound whose pitch is its firing frequency. It is not a model of anything.
//
// What it is for: the shell's own tests (the audio device, the controls, the application's loop) without compiling the
// simulator; and `--engine tone` in the application, which tells a silent audio device apart from a simulator that did
// not start.
#pragma once

#include "player/Boxes.h"
#include "tee/app/Sources.h"

#include <array>
#include <atomic>

namespace tee::player
{
    class TonePlayer final : public Player
    {
    public:
        TonePlayer();

        // ParamSource
        int count() const override { return kParams; }
        const ParamInfo& info(int i) const override;
        double get(int i) const override;
        void set(int i, double value) override;
        int findParam(std::string_view id) const override;

        // TelemetrySource
        int findValue(std::string_view name) const override;
        double value(int handle) const override;
        float outputPeak() const override { return peak_.load(std::memory_order_relaxed); }
        int outputTail(float* mono, int max) const override { return tail_.read(mono, max); }

        // ControlSink
        void setControls(const ControlsFrame& f) override { controls_.set(f); }

        // AudioSource
        void prepare(double rate, int maxBlock) override;
        void process(float* l, float* r, int n) noexcept override;

        // Blocks processed and the largest asked for, for a test of the device.
        long long blocks() const noexcept { return blocks_.load(std::memory_order_relaxed); }
        int largestBlock() const noexcept { return largest_.load(std::memory_order_relaxed); }

        static constexpr int kParams = 3;       // level (dB), idle (rpm), cylinders (4, 6, 8)

    private:
        enum Tel { tState, tSpeed, tRedline, tThrottle, tGear, tDriven, tCount };

        std::array<ParamInfo, kParams> infos_;
        ValueBox params_{ kParams };            // UI to audio
        std::array<std::atomic<double>, kParams> shown_{};   // what get() answers
        std::array<std::atomic<double>, tCount> tel_{};      // audio to UI
        std::atomic<float> peak_{ 0.0f };
        std::atomic<long long> blocks_{ 0 };
        std::atomic<int> largest_{ 0 };
        TailRing tail_;
        ControlsBox controls_;

        // the audio thread's own
        double rate_ = 48000.0;
        double level_ = 0.25, idleRpm_ = 800.0;
        int cylinders_ = 8;
        int state_ = 0;                         // 0 off, 1 cranking, 2 running, 3 running down
        double rpm_ = 0.0, phase_ = 0.0, throttle_ = 0.0, amp_ = 0.0;
        double offHeld_ = 0.0;
        bool startWas_ = false, startOnRunning_ = false;
        int gear_ = 0, lastUp_ = 0, lastDown_ = 0, lastLever_ = 0;
    };
}
