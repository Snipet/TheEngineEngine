// ExploreTest.cpp: tables for the fit (hidden: run with "[.explore]"): each source's level in each of the reference's
// views and in each of model 1's cameras, white noise in.
#include "Rig.h"

#include <cstdio>

using namespace tee::test::listen;

TEST_CASE("explore: each source in each view and camera", "[.explore]")
{
    Rig rig;
    const int srcs[] = {kTails, kIntake, kMech, kBlock, kShell, kTurbo, kStarter, kComb, kRoad, kTorque};
    const char* names[] = {"tails", "intake", "mech", "block", "shell", "turbo", "starter", "comb", "road", "torque"};
    std::printf("%-8s", "src");
    for (int v = 0; v < 5; ++v)
        std::printf("   ref%d  new%d  diff", v, v);
    std::printf("\n");
    for (int k = 0; k < 10; ++k)
    {
        std::printf("%-8s", names[k]);
        rig.set("driven", srcs[k] == kRoad ? 1 : 0);
        rig.set("shake", 0.0);
        for (int v = 0; v < 5; ++v)
        {
            rig.set("view", v);
            rig.set("ss.camera", v);
            rig.set("ss.model", 0);
            const double a = dB(heard(rig, srcs[k]));
            rig.set("ss.model", 1);
            const double b = dB(heard(rig, srcs[k]));
            std::printf(" %6.1f %6.1f %5.1f", a, b, b - a);
        }
        std::printf("\n");
    }
}

TEST_CASE("explore: the cabin's boom by frequency", "[.explore]")
{
    Rig rig;
    rig.set("view", 4);
    rig.set("ss.camera", 4);
    rig.set("src", kTorque);
    for (double f : {30.0, 50.0, 100.0, 200.0, 400.0})
    {
        rig.set("sig", 1);
        rig.set("freq", f);
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
        std::printf("%6.0f Hz  ref %6.1f  new %6.1f  diff %5.1f\n", f, dB(p[0]), dB(p[1]), dB(p[1] / p[0]));
    }
}

TEST_CASE("explore: steps at each change", "[.explore]")
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
    for (int a = 0; a < 7; ++a)
    {
        rig.clear();
        rig.set("ss.camera", a);
        const bool pl = std::getenv("PLACE") != nullptr;
        if (pl)
            rig.set("ss.place", a % 5);
        if (std::getenv("DIST") != nullptr)
            rig.set("distance", 0.15 * double(a % 4));
        rig.play(0.4);
        const auto& l = rig.left();
        double worst = 0.0;
        std::size_t at = 0;
        for (std::size_t i = 960; i + 960 < l.size(); ++i)
        {
            double s = 0.0;
            for (std::size_t j = i - 960; j < i + 960; j += 4)
                s += double(l[j]) * l[j];
            const double rms = std::sqrt(s / 480.0);
            const double st = std::fabs(double(l[i]) - l[i - 1]) / std::max(rms, 1e-12);
            if (st > worst)
            {
                worst = st;
                at = i;
            }
        }
        std::printf("to camera %d: worst %.4f at %.1f ms\n", a, worst, double(at) / 96.0);
    }
}

namespace
{
    // Power in [lo, hi) Hz of x (both channels summed), by a plain DFT over 2^k samples (radix 2, a Hann window)
    void bands(const std::vector<float>& l, const std::vector<float>& r, std::size_t from, double out[3])
    {
        const int k = 17;
        const std::size_t n = std::size_t(1) << k;
        double p[3] = {0.0, 0.0, 0.0};
        for (int ch = 0; ch < 2; ++ch)
        {
            const auto& x = ch == 0 ? l : r;
            std::vector<double> re(n), im(n, 0.0);
            for (std::size_t i = 0; i < n; ++i)
                re[i] = double(x[from + i]) * (0.5 - 0.5 * std::cos(2.0 * 3.14159265358979 * double(i) / double(n)));
            for (std::size_t i = 1, j = 0; i < n; ++i)
            {
                std::size_t bit = n >> 1;
                for (; j & bit; bit >>= 1)
                    j ^= bit;
                j ^= bit;
                if (i < j)
                {
                    std::swap(re[i], re[j]);
                    std::swap(im[i], im[j]);
                }
            }
            for (std::size_t len = 2; len <= n; len <<= 1)
            {
                const double a = -2.0 * 3.14159265358979 / double(len);
                for (std::size_t i = 0; i < n; i += len)
                    for (std::size_t j = 0; j < len / 2; ++j)
                    {
                        const double c = std::cos(a * double(j)), s = std::sin(a * double(j));
                        const double ur = re[i + j], ui = im[i + j];
                        const double vr = re[i + j + len / 2] * c - im[i + j + len / 2] * s;
                        const double vi = re[i + j + len / 2] * s + im[i + j + len / 2] * c;
                        re[i + j] = ur + vr;
                        im[i + j] = ui + vi;
                        re[i + j + len / 2] = ur - vr;
                        im[i + j + len / 2] = ui - vi;
                    }
            }
            const double hz = kRate / double(n);
            const double edges[4] = {100.0, 1500.0, 6000.0, 16000.0};
            for (int b = 0; b < 3; ++b)
                for (std::size_t i = std::size_t(edges[b] / hz); i < std::size_t(edges[b + 1] / hz); ++i)
                    p[b] += re[i] * re[i] + im[i] * im[i];
        }
        for (int b = 0; b < 3; ++b)
            out[b] = 10.0 * std::log10(p[b] + 1e-30);
    }
}

TEST_CASE("explore: each source's highs in each view and camera", "[.explore]")
{
    // white noise into each source; model 1's camera minus the reference's view, in three bands: 100 Hz to 1.5 kHz,
    // 1.5 to 6 kHz, 6 to 16 kHz
    Rig rig;
    const int srcs[] = {kTails, kIntake, kMech, kBlock, kShell, kTurbo, kComb};
    const char* names[] = {"tails", "intake", "mech", "block", "shell", "turbo", "comb"};
    std::printf("%-8s", "src");
    for (int v = 0; v < 5; ++v)
        std::printf("   view %d: lo / mid / hi  ", v);
    std::printf("\n");
    for (int k = 0; k < 7; ++k)
    {
        std::printf("%-8s", names[k]);
        rig.set("driven", 0);
        rig.set("shake", 0.0);
        for (int v = 0; v < 5; ++v)
        {
            double b[2][3];
            for (int m = 0; m < 2; ++m)
            {
                rig.set("view", v);
                rig.set("ss.camera", v);
                rig.set("ss.model", m);
                rig.set("src", srcs[k]);
                rig.set("sig", 4);
                rig.reset();
                rig.clear();
                rig.play(0.3, false);
                rig.play(1.4);
                bands(rig.left(), rig.right(), 0, b[m]);
            }
            std::printf("   %+5.1f %+5.1f %+5.1f  ", b[1][0] - b[0][0], b[1][1] - b[0][1], b[1][2] - b[0][2]);
        }
        std::printf("\n");
    }
}
