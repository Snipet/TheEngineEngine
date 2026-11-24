// host/include/tee/Measures.h: what is measured of rendered sound, for the tests (tee/test/Scenario.h) and the tools
// (tee-validate). Nobody here can listen, so a test claims numbers: a level, a line's level at a frequency, a
// spectrum's floor, the largest step between samples against the local level (a click), how far two renders are apart.
// Levels are in dB relative to a full-scale sine's amplitude (a sine of amplitude 1 reads 0 dB). Double arithmetic.
#pragma once

#include <cstddef>
#include <span>
#include <vector>

namespace tee::measure
{
    bool finite(std::span<const float> x) noexcept;
    double peak(std::span<const float> x) noexcept;
    double rms(std::span<const float> x) noexcept;
    // 20 log10 of a value, -300 for 0.
    double dB(double amplitude) noexcept;

    // A sine's amplitude at `hz` (Goertzel over the whole span, Hann window), in dB: a sine of amplitude A at hz reads
    // 20 log10 A. The largest over hz +- 1.2 %, so that a line that wanders a little is still found.
    double lineDb(std::span<const float> x, double rate, double hz);

    // A spectrum: each bin's amplitude in dB on the line's scale (a plain radix-2 FFT of the span's first 2^k samples,
    // Hann window); bin k is at k * rate / size.
    struct Spectrum
    {
        double binHz = 0.0;
        std::vector<double> db;
        // The median bin between `low` and `high` Hz: the floor a line is compared with.
        double floorDb(double low, double high) const;
        // The strongest bin's frequency between low and high.
        double peakHz(double low, double high) const;
    };
    Spectrum spectrum(std::span<const float> x, double rate);

    // The largest step between two samples over the RMS of the steps around it (10 ms each side, the step itself left
    // out, at least 1e-4): a smooth signal of any level stays under a few, a click is far above.
    double largestStep(std::span<const float> x, double rate);

    // 10 log10(sum a^2 / sum (a - b)^2) over the samples both have; +inf when they are the same.
    double nullDepth(std::span<const float> a, std::span<const float> b);
} // namespace tee::measure
