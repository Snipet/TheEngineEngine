// tests/sim/intake/AirBoxTest.cpp: card E's standalone test of the air box (sim/intake/airbox.fm): its resonance is
// where its volume and its snorkel say, its defaults are the reference's 90 Hz, and it never makes energy.
#include "sim/valvetrain/Lab.h"

#include <catch2/catch_approx.hpp>

#include <cmath>
#include <cstdio>

using tee::test::lab::Obj;

namespace
{
    tee::Program& boxProgram() { return tee::test::lab::program("intake/airbox.fm"); }

    // The neck's flow against the throttle's at f Hz: its phase (radians, 0 to -pi), by driving a sine and
    // demodulating the last half of it
    double phaseAt(Obj& box, double f, double rate)
    {
        auto* tick = box.fn<void(void*, float)>("tick");
        const auto qn = box.field<float>("qn");
        box.call<void>("clear");
        const int n = static_cast<int>(rate * 0.6);
        double re = 0.0;
        double im = 0.0;
        for (int i = 0; i < n; ++i)
        {
            const double w = 2.0 * 3.14159265358979 * f * i / rate;
            tick(box.data(), static_cast<float>(0.01 * std::sin(w)));
            if (i > n / 2)
            {
                const double q = box.inst->get(qn);
                re += q * std::sin(w);
                im += q * std::cos(w);
            }
        }
        return std::atan2(im, re);
    }

    // Where the phase passes -90 degrees: the undamped resonance of a second-order low-pass
    double resonance(Obj& box, double rate)
    {
        double lo = 20.0;
        double hi = 400.0;
        for (int it = 0; it < 22; ++it)
        {
            const double mid = std::sqrt(lo * hi);
            if (phaseAt(box, mid, rate) > -1.5707963)
                lo = mid;
            else
                hi = mid;
        }
        return std::sqrt(lo * hi);
    }
} // namespace

TEST_CASE("air box: its resonance is where its volume and its snorkel say", "[intake][airbox]")
{
    const double rate = 96000.0;
    struct Box
    {
        double vol, len, bore;
    };
    for (const Box b : {Box{0.010, 0.15, 0.07}, Box{0.020, 0.15, 0.07}, Box{0.010, 0.40, 0.06}})
    {
        Obj box(boxProgram(), "AirBox");
        box.call<void>("setup", static_cast<float>(b.vol), static_cast<float>(b.len), static_cast<float>(b.bore), 0.0057f,
                       0.004f, static_cast<float>(rate));
        // Helmholtz's: c / 2 pi sqrt(A / (V Le)), Le the neck and its end corrections (0.85 r and 0.61 r)
        const double area = 0.785398 * b.bore * b.bore;
        const double le = b.len + 1.46 * 0.5 * b.bore;
        const double predicted = 343.0 / (2.0 * 3.14159265358979) * std::sqrt(area / (b.vol * le));
        const double measured = resonance(box, rate);
        std::printf("[intake] air box %.0f L, snorkel %.0f mm long, %.0f mm bore: predicted %.1f Hz, measured %.1f Hz\n",
                    b.vol * 1000.0, b.len * 1000.0, b.bore * 1000.0, predicted, measured);
        CHECK(box.get("f0") == Catch::Approx(predicted).epsilon(1e-4));
        CHECK(measured == Catch::Approx(predicted).epsilon(0.01));
    }
    // left at 0, from the engine: the reference's 90 Hz
    Obj box(boxProgram(), "AirBox");
    box.call<void>("setup", 0.0f, 0.0f, 0.0f, 0.0057f, 0.0047f, static_cast<float>(rate));
    CHECK(box.get("f0") == Catch::Approx(90.0).epsilon(1e-3));
    CHECK(resonance(box, rate) == Catch::Approx(90.0).epsilon(0.01));
}

TEST_CASE("air box: passive, its energy only falls once the throttle stops drawing", "[intake][airbox]")
{
    Obj box(boxProgram(), "AirBox");
    box.call<void>("setup", 0.008f, 0.12f, 0.065f, 0.0057f, 0.004f, 96000.0f);
    box.call<void>("clear");
    auto* tick = box.fn<void(void*, float)>("tick");
    for (int i = 0; i < 2000; ++i)
        tick(box.data(), 0.05f);
    const double rho = 1.204;
    const double c = 343.0;
    const double vol = box.get("vol");
    const double mass = rho * box.get("le") / box.get("neckArea");
    auto energy = [&] {
        const double p = box.get("p");
        const double q = box.get("qn");
        return 0.5 * p * p * vol / (rho * c * c) + 0.5 * mass * q * q;
    };
    tick(box.data(), 0.0f);     // (the trapezoid's input is the mean of two samples: one more to stop it)
    double last = energy();
    bool grew = false;
    for (int i = 0; i < 96000; ++i)
    {
        tick(box.data(), 0.0f);
        const double e = energy();
        grew = grew || e > last * (1.0 + 1e-5) + 1e-15;
        last = e;
    }
    CHECK_FALSE(grew);
    CHECK(std::isfinite(last));
    CHECK(last < 1e-9);
}
