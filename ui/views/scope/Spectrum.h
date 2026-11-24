// ui/views/scope/Spectrum.h: the output's spectrum, and the spectrum against engine speed (SCOPE's two lower plots).
//
// Spectrum: a 4096-point FFT of the output's newest samples (TelemetrySource::outputTail), in C++ on the interface's
// thread, through a Hann window, its power smoothed over about a tenth of a second so a steady engine reads steady. A
// radix-2 transform of 4096 real samples is about 50 thousand multiply-adds: nothing that shows at 60 frames a second.
// Levels are dB against a full-scale sine (a sine of amplitude 1 reads 0 dB).
//
// Campbell: a grid of cells, engine speed across and log frequency up, each the level last seen at that speed and
// band, filled in as the engine sweeps; empty cells (never seen) are NaN. The orders of the crank stand as slanted
// lines f = order * rpm / 60 when it is drawn.
//
// Plain C++: no FunkGui, no clock. Time only from the dt given.
#pragma once

#include <cstdint>
#include <vector>

namespace tee::ui::scope
{
    inline constexpr double kPi = 3.14159265358979323846;

    class Spectrum
    {
    public:
        static constexpr int kSize = 4096;            // samples per transform (a power of two)
        static constexpr int kBins = kSize / 2 + 1;   // 0 Hz to half the rate
        static constexpr float kFloorDb = -140.0f;

        Spectrum();

        // The newest `n` samples (oldest first; the last kSize of them are taken, zeros before them if fewer) at `rate`
        // samples a second. `smooth` seconds of smoothing at a step of `dt` (dt 0: no smoothing, this frame alone).
        void analyse(const float* samples, int n, double rate, float dt, float smooth = 0.12f);
        void clear();

        bool empty() const noexcept { return frames_ == 0; }
        double rate() const noexcept { return rate_; }
        double binHz() const noexcept { return rate_ / kSize; }
        // The level of bin k, dB.
        float db(int k) const noexcept;
        // The level of a band [f0, f1) Hz: the loudest bin in it; a band narrower than a bin reads the bin under its
        // middle, by straight lines between bins.
        float bandDb(double f0, double f1) const noexcept;
        // The loudest bin between f0 and f1 Hz: its frequency (NaN when the spectrum is empty).
        double peakHz(double f0, double f1) const noexcept;

    private:
        std::vector<float> window_, re_, im_, power_;
        std::vector<int> bitrev_;
        std::vector<double> cos_, sin_;
        double rate_ = 48000.0, windowSum_ = 1.0;
        std::uint64_t frames_ = 0;
    };

    class Campbell
    {
    public:
        // `cols` speeds from rpm0 to rpm1, `rows` bands from f0 to f1 Hz (log).
        Campbell(int cols = 72, int rows = 56, double rpm0 = 0.0, double rpm1 = 8000.0, double f0 = 25.0,
                 double f1 = 4000.0);

        // The spectrum at an engine speed (rpm) into its column; a speed outside the range is not kept.
        void take(double rpm, const Spectrum& s);
        void clear();
        // A new speed range (the cells are cleared when it changes).
        void setSpeedRange(double rpm0, double rpm1);

        int cols() const noexcept { return cols_; }
        int rows() const noexcept { return rows_; }
        double rpm0() const noexcept { return rpm0_; }
        double rpm1() const noexcept { return rpm1_; }
        double f0() const noexcept { return f0_; }
        double f1() const noexcept { return f1_; }
        // Row r's band, Hz (row 0 the lowest).
        double rowLo(int r) const noexcept;
        double rowHi(int r) const noexcept { return rowLo(r + 1); }
        int columnOf(double rpm) const noexcept;      // -1 outside
        float cell(int col, int row) const noexcept { return cells_[static_cast<std::size_t>(col * rows_ + row)]; }
        int filled() const noexcept { return filled_; }   // columns with anything in them
        // Moves at every change (a drawKey's).
        std::uint64_t revision() const noexcept { return revision_; }

    private:
        int cols_, rows_;
        double rpm0_, rpm1_, f0_, f1_;
        std::vector<float> cells_;
        std::vector<bool> seen_;
        int filled_ = 0;
        std::uint64_t revision_ = 1;
    };
}
