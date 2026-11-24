// FitTest.cpp: the load verdict (tee/Fit.h, HostStats::fits) on scripted load sequences.
#include "tee/Fit.h"

#include <catch2/catch_test_macros.hpp>

using tee::FitMeter;

namespace
{
    // `seconds` of updates every 10 ms at this peak and late count; the verdict at the end
    int run(FitMeter& meter, double peak, double seconds, std::uint32_t late = 0)
    {
        int verdict = meter.verdict();
        for (double t = 0.0; t < seconds - 1e-9; t += 0.01)
            verdict = meter.update(peak, late, 0.01);
        return verdict;
    }
} // namespace

TEST_CASE("the load verdict: fits, tight after 2 s over 0.7, over after 2 s over 0.9, back one level after 5 s under",
          "[fit][host]")
{
    FitMeter meter;
    CHECK(meter.verdict() == 1);
    CHECK(run(meter, 0.5, 10.0) == 1);

    // a moment over is not tight; held 2 s it is
    CHECK(run(meter, 0.8, 1.5) == 1);
    CHECK(run(meter, 0.5, 0.1) == 1);
    CHECK(run(meter, 0.8, 1.9) == 1);
    CHECK(run(meter, 0.8, 0.2) == 0);

    // tight holds while the peak stays over 0.6: no flicker at the threshold
    CHECK(run(meter, 0.65, 20.0) == 0);
    CHECK(run(meter, 0.72, 0.5) == 0);
    CHECK(run(meter, 0.68, 0.5) == 0);

    // over 0.9 held: over
    CHECK(run(meter, 0.95, 1.9) == 0);
    CHECK(run(meter, 0.95, 0.2) == -1);

    // back one level at a time: under 0.8 for 5 s to tight, then under 0.6 for 5 s to fits
    CHECK(run(meter, 0.85, 20.0) == -1);
    CHECK(run(meter, 0.5, 4.9) == -1);
    CHECK(run(meter, 0.5, 0.2) == 0);
    CHECK(run(meter, 0.5, 4.9) == 0);
    CHECK(run(meter, 0.5, 0.2) == 1);

    // a late callback is over at once, whatever the peak
    CHECK(run(meter, 0.3, 0.01, 1) == -1);
    CHECK(run(meter, 0.3, 5.1, 1) == 0);
    CHECK(run(meter, 0.3, 5.1, 1) == 1);

    // a count that went back is a new load: afresh
    CHECK(run(meter, 0.3, 0.01, 0) == 1);
}
