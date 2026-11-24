// ListenerTest.cpp: package L's listener on its rig (rig.fm): Doppler at a pass, the ground's comb, the level with
// distance, the reference's five views as presets of the new model, the other cameras' balance, no click at a change.
#include "Rig.h"

#include <catch2/catch_approx.hpp>

#include <complex>
#include <cstdio>

using namespace tee::test::listen;

namespace
{
    constexpr double kC = 343.2;    // Scene's speed of sound

    // DISTANCE's dial for r metres (0.5 m e^(4.094 d))
    double dialFor(double r) { return std::log(r / 0.5) / 4.0943446; }

    // The frequency of x[from, to) by its upward zero crossings, interpolated
    double frequency(const std::vector<float>& x, std::size_t from, std::size_t to)
    {
        double first = -1.0;
        double last = -1.0;
        int cycles = -1;
        for (std::size_t i = from + 1; i < to; ++i)
        {
            if (x[i - 1] < 0.0f && x[i] >= 0.0f)
            {
                const double t = double(i - 1) + double(-x[i - 1]) / double(x[i] - x[i - 1]);
                if (first < 0.0)
                    first = t;
                last = t;
                ++cycles;
            }
        }
        return cycles > 0 ? double(cycles) * kRate / (last - first) : 0.0;
    }

    // |H(f)| of an impulse response, by its DFT at f
    double magnitude(const std::vector<float>& h, double f)
    {
        std::complex<double> s(0.0, 0.0);
        for (std::size_t n = 0; n < h.size(); ++n)
            s += double(h[n]) * std::polar(1.0, -2.0 * kPi * f * double(n) / kRate);
        return std::abs(s);
    }
}

TEST_CASE("Doppler: a pass by trackside bends the pitch as the formula says", "[listen]")
{
    Rig rig;
    const double v = 30.0;
    const double f0 = 1000.0;
    const double r = 4.0;
    rig.set("ss.model", 1);
    rig.set("ss.camera", 5);
    rig.set("ss.ground", 2);
    rig.set("distance", dialFor(r));
    rig.set("ln.sc.windOff", 1);
    rig.set("driven", 1);
    rig.set("carSpeed", v);
    rig.set("src", kTailL);
    rig.set("sig", 1);
    rig.set("freq", f0);
    rig.reset();
    // the trackside listener: 0.9 + 2 r beside the car's middle line, 1.5 m up, facing the road; its left ear 8.5 cm
    // back along the road. The left tailpipe at x = -0.55, 0.35 m up, at the rear bumper (the car's y = 0)
    const double lx = 0.9 + 2.0 * r;
    const double ex = lx + 0.55;
    const double ez = 1.5 - 0.35;
    const double chunk = 0.05;
    const int chunks = 54;
    double posPrev = 0.0;
    int checked = 0;
    double worst = 0.0;
    double lowest = 10.0;
    double highest = 0.0;
    rig.play(0.3, false);   // the first sound arrives 0.12 s after the start: past its edge
    posPrev = rig.get("st.roadPosition");
    for (int k = 0; k < chunks; ++k)
    {
        const std::size_t from = rig.left().size();
        rig.play(chunk);
        const double pos = rig.get("st.roadPosition");
        // where the left ear is from the pipe, along the road, at the chunk's middle (the car's middle is 2.2 m ahead of
        // its rear bumper; roadPosition is how far that middle is past the listener)
        const double ey = -(0.5 * (pos + posPrev) - 2.2) - 0.085;
        posPrev = pos;
        // the sound heard now left the pipe tau ago, from vt behind where it is: |D + V tau| = c tau
        const double dd = ex * ex + ey * ey + ez * ez;
        const double dv = ey * v;
        const double den = kC * kC - v * v;
        const double tau = (dv + std::sqrt(dv * dv + den * dd)) / den;
        const double ry = ey + v * tau;
        const double cosE = ry / (kC * tau);      // the car's heading against the way to the ear, at emission
        const double want = kC / (kC - v * cosE);
        const double got = frequency(rig.left(), from, rig.left().size()) / f0;
        INFO("chunk " << k << " car past the listener " << pos << " m: heard " << got << ", formula " << want);
        CHECK(got == Catch::Approx(want).epsilon(0.002));
        worst = std::max(worst, std::fabs(got / want - 1.0));
        lowest = std::min(lowest, got);
        highest = std::max(highest, got);
        ++checked;
    }
    std::printf("Doppler at 30 m/s, %.1f m beside: heard from %.4f to %.4f of the pitch (formula %.4f to %.4f), worst %.3f %%\n",
                lx, highest, lowest, kC / (kC - v), kC / (kC + v), 100.0 * worst);
    CHECK(highest > 1.07);
    CHECK(lowest < 0.94);
    CHECK(checked == chunks);
}

TEST_CASE("the ground: the comb's notches sit where the geometry puts them, and move with the distance", "[listen]")
{
    Rig rig;
    rig.set("ss.model", 1);
    rig.set("ss.camera", 2);      // SIDE: the listener 0.9 + r beside the car, 1.3 m forward of the bumper, 1.2 m up
    rig.set("ss.ground", 0);
    rig.set("ln.sc.windOff", 1);
    rig.set("src", kTailL);
    for (double r : {1.5, 4.0})
    {
        rig.set("distance", dialFor(r));
        rig.set("sig", 3);
        rig.reset();
        rig.clear();
        rig.play(0.1, false);
        rig.set("sig", 2);
        rig.play(8192.0 / kRate);
        const std::vector<float> h = rig.left();
        // the left ear (8.5 cm back along the car) by its direct path; the ground's image (the pipe 0.35 m below the
        // ground) at the ears' middle
        const double lx = 0.9 + r + 0.55;
        const double dDirect = std::sqrt(lx * lx + (1.3 - 0.085) * (1.3 - 0.085) + 0.85 * 0.85);
        const double dImage = std::sqrt(lx * lx + 1.3 * 1.3 + 1.55 * 1.55);
        const double f1 = kC / (2.0 * (dImage - dDirect));
        // the notch: the least |H| within 15 % of f1, against the peaks either side (at 0 and 2 f1)
        double fMin = 0.0;
        double hMin = 1e30;
        for (double f = 0.85 * f1; f <= 1.15 * f1; f += f1 * 0.002)
        {
            const double m = magnitude(h, f);
            if (m < hMin)
            {
                hMin = m;
                fMin = f;
            }
        }
        const double around = std::sqrt(magnitude(h, 0.5 * f1) * magnitude(h, 2.0 * f1));
        const double depth = 20.0 * std::log10(around / hMin);
        std::printf("ground at %.1f m: first notch %.0f Hz, the geometry's %.0f Hz, %.1f dB deep; next at %.0f Hz\n", r, fMin,
                    f1, depth, 3.0 * f1);
        CHECK(fMin == Catch::Approx(f1).epsilon(0.02));
        CHECK(depth > 12.0);
        // and the third harmonic's notch
        double f3 = 0.0;
        double h3 = 1e30;
        for (double f = 2.8 * f1; f <= 3.2 * f1; f += f1 * 0.002)
        {
            const double m = magnitude(h, f);
            if (m < h3)
            {
                h3 = m;
                f3 = f;
            }
        }
        CHECK(f3 == Catch::Approx(3.0 * f1).epsilon(0.02));
    }
}

TEST_CASE("the level falls 6 dB a doubling of the distance", "[listen]")
{
    Rig rig;
    rig.set("ss.model", 1);
    rig.set("ss.camera", 0);
    rig.set("ss.ground", 2);
    rig.set("ln.sc.windOff", 1);
    double prev = 0.0;
    for (double r : {2.0, 4.0, 8.0, 16.0, 30.0})
    {
        rig.set("distance", dialFor(r));
        const double p = dB(heard(rig, kTails));
        if (r > 2.0)
        {
            const double step = (p - prev) / std::log2(r / (r > 16.0 ? 16.0 : r / 2.0));
            std::printf("tailpipe camera, %4.1f m: %.1f dB, %.2f dB a doubling\n", r, p, step);
            CHECK(step < -5.0);
            CHECK(step > -7.0);
        }
        prev = p;
    }
}

TEST_CASE("the reference's five views are presets of the new model, within the stated tolerance", "[listen]")
{
    Rig rig;
    const int srcs[] = {kTails, kIntake, kMech, kBlock, kShell, kTurbo, kStarter, kComb, kRoad};
    const char* names[] = {"exhaust", "intake", "valvetrain", "block", "headers", "turbo", "starter", "combustion", "tyres"};
    double sum = 0.0;
    int count = 0;
    for (int k = 0; k < 9; ++k)
    {
        rig.set("driven", srcs[k] == kRoad ? 1 : 0);
        for (int v = 0; v < 5; ++v)
        {
            rig.set("view", v);
            rig.set("ss.camera", v);
            rig.set("ss.model", 0);
            const double a = dB(heard(rig, srcs[k]));
            rig.set("ss.model", 1);
            const double b = dB(heard(rig, srcs[k]));
            INFO(names[k] << " in view " << v << ": " << b - a << " dB from the reference's");
            // the exhaust within 1.5 dB; the rest within 7 dB (the reference's table is not geometric: its REAR hears
            // the bay better than its EXHAUST, 2 m nearer)
            CHECK(std::fabs(b - a) < (k == 0 ? 1.5 : 7.0));
            sum += (b - a) * (b - a);
            ++count;
        }
    }
    const double rms = std::sqrt(sum / count);
    std::printf("the five views as presets: %d source-view pairs, %.2f dB rms from the reference's levels\n", count, rms);
    CHECK(rms < 3.5);
    // the cabin's boom at 100 Hz, where it was matched
    rig.set("view", 4);
    rig.set("ss.camera", 4);
    rig.set("src", kTorque);
    rig.set("sig", 1);
    rig.set("freq", 100.0);
    double p[2];
    for (int m = 0; m < 2; ++m)
    {
        rig.set("ss.model", m);
        rig.reset();
        rig.clear();
        rig.play(0.5, false);
        rig.play(1.0);
        p[m] = rig.power();
    }
    CHECK(std::fabs(dB(p[1] / p[0])) < 1.0);
}

TEST_CASE("the trackside and chase cameras: each source against the exhaust as the nearest reference view has it", "[listen]")
{
    // the chase camera is the reference's REAR moved back with the car; trackside (the car standing beside it,
    // the listener 4.3 m out from its middle line) is SIDE from further. Each source's level against the exhaust's, against
    // the same in that view, within 8 dB: the geometry moves the balance, the strengths are the views' fit
    Rig rig;
    rig.set("ln.sc.windOff", 1);
    const int srcs[] = {kIntake, kMech, kTurbo, kComb};
    const char* names[] = {"intake", "valvetrain", "turbo", "combustion"};
    const int cams[] = {6, 5};
    const int views[] = {1, 2};
    for (int c = 0; c < 2; ++c)
    {
        rig.set("view", views[c]);
        rig.set("ss.camera", cams[c]);
        rig.set("ss.model", 0);
        const double ex0 = dB(heard(rig, kTails));
        rig.set("ss.model", 1);
        const double ex1 = dB(heard(rig, kTails));
        for (int k = 0; k < 4; ++k)
        {
            rig.set("ss.model", 0);
            const double a = dB(heard(rig, srcs[k])) - ex0;
            rig.set("ss.model", 1);
            const double b = dB(heard(rig, srcs[k])) - ex1;
            std::printf("camera %d: %-10s %6.1f dB against the exhaust (the reference's view %d: %6.1f)\n", cams[c], names[k], b,
                        views[c], a);
            CHECK(std::fabs(b - a) < 8.0);
        }
    }
}

TEST_CASE("no click: changing the camera, the place and the distance while it sounds", "[listen]")
{
    Rig rig;
    rig.set("ss.model", 1);
    rig.set("ln.sc.windOff", 1);
    rig.set("src", kTails);
    rig.set("sig", 1);
    rig.set("freq", 180.0);
    rig.set("amp", 0.002);
    rig.reset();
    rig.play(0.3, false);
    // the steady sound's largest step a sample, against its level
    rig.clear();
    rig.play(0.5);
    auto stepRatio = [&](std::size_t from) {
        double worst = 0.0;
        const auto& l = rig.left();
        const auto& r = rig.right();
        const std::size_t win = 960;
        for (std::size_t i = from + win; i + win < l.size(); i += 16)
        {
            double s = 0.0;
            for (std::size_t j = i - win; j < i + win; ++j)
                s += double(l[j]) * l[j] + double(r[j]) * r[j];
            const double rms = std::sqrt(s / double(2 * win));
            double step = 0.0;
            for (std::size_t j = i; j < i + 16; ++j)
                step = std::max({step, std::fabs(double(l[j]) - l[j - 1]), std::fabs(double(r[j]) - r[j - 1])});
            worst = std::max(worst, step / std::max(rms, 1e-12));
        }
        return worst;
    };
    const double steady = stepRatio(0);
    // every camera to every other, a place, the distance, held 0.4 s each
    rig.clear();
    for (int a = 0; a < 7; ++a)
    {
        rig.set("ss.camera", a);
        rig.set("ss.place", a % 5);
        rig.set("distance", 0.15 * double(a % 4));
        rig.play(0.4);
    }
    const double moving = stepRatio(0);
    std::printf("largest step a sample over the local rms: steady %.4f, through every camera %.4f\n", steady, moving);
    // a 180 Hz sine's largest step is 2 pi 180 / 96000 sqrt(2) of its rms: 0.017; a crossfade's change of balance adds
    // little; a click would be many times it
    CHECK(moving < 1.5 * steady + 0.01);
}
