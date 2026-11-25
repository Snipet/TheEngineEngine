// EngineTest.cpp: package L's listener in the whole simulator (sim/engine.fm through tee::EnginePlayer): the level
// computed across the reference's layouts, a drive with the listener moving, the cabin.
#include "sim/core/Engine.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

using tee::test::core::Bench;

namespace
{
    void model1(Bench& b)
    {
        b.write("spec.sound.model", 1);
        b.write("spec.sound.rev", b.field("spec.sound.rev") + 1);
    }

    void sound(Bench& b, const char* field, double v)
    {
        b.write(std::string("spec.sound.") + field, v);
        b.write("spec.sound.rev", b.field("spec.sound.rev") + 1);
    }
}

namespace
{
    // Each build held at 0.75 of its redline at full throttle on the dyno: the exhaust's radiation through model 1's
    // computed level (tel.sound.exhaustLevel + loudness) and through the reference's tables (exTrim x gainNorm); the
    // spread of each across the builds
    void loudnessSpread(const std::vector<std::string>& builds, double spread[2])
    {
        double lo[2] = {1e9, 1e9};
        double hi[2] = {-1e9, -1e9};
        for (const auto& name : builds)
        {
            Bench b(name);
            model1(b);
            b.start(1.0);
            b.ctl().mode = 2;
            b.ctl().dynoSpeed = float(0.75 * b.tel("engine.redline"));
            b.ctl().throttle = 1.0f;
            b.play(1.5);
            const double raw = b.tel("sound.exhaustLevel");
            const double computed = raw + b.tel("sound.loudness");
            const double table = raw + 20.0 * std::log10(b.field("exTrim") * b.field("gainNorm") / (8.169 * 0.6645));
            std::printf("%-24s exhaust at full load: %5.1f dB computed, %5.1f dB by the reference's tables\n", name.c_str(),
                        computed, table);
            lo[0] = std::min(lo[0], computed);
            hi[0] = std::max(hi[0], computed);
            lo[1] = std::min(lo[1], table);
            hi[1] = std::max(hi[1], table);
        }
        spread[0] = hi[0] - lo[0];
        spread[1] = hi[1] - lo[1];
        std::printf("spread: computed %.1f dB, the reference's tables %.1f dB\n", spread[0], spread[1]);
    }
}

TEST_CASE("loudness: the exhaust at full load is one level across the reference's layouts, computed", "[listen][engine][nightly]")
{
    // the sixteen piston layouts of the reference at their own default sizes
    double spread[2];
    loudnessSpread({"engines/ref-00-single", "engines/ref-01-twin-360", "engines/ref-02-twin-270", "engines/ref-03-vtwin-45",
                    "engines/ref-04-inline-3", "engines/ref-05-inline-4", "engines/ref-06-flat-4", "engines/ref-07-xplane-i4",
                    "engines/ref-08-inline-5", "engines/ref-09-v6-60", "engines/ref-10-inline-6", "engines/ref-11-flat-6",
                    "engines/ref-12-v8-cross", "engines/ref-13-v8-flat", "engines/ref-14-v10-72", "engines/ref-15-v12-60"},
                   spread);
    CHECK(spread[0] < 12.5);
    CHECK(spread[0] < spread[1]);
}

TEST_CASE("loudness, quick: the inline three and the flat-plane V8, which the tables leave farthest apart", "[listen][engine]")
{
    // the nightly test's corner: through the reference's tables these two are the extremes of the sixteen (17 dB)
    double spread[2];
    loudnessSpread({"engines/ref-04-inline-3", "engines/ref-13-v8-flat"}, spread);
    CHECK(spread[0] < 6.0);
    CHECK(spread[0] < spread[1]);
}

namespace
{
    // `seconds` of a drive through the gears, something of the listener moved every second; the car must pass vWant
    void soak(int seconds, double vWant)
    {
    Bench b("v8");
    model1(b);
    b.start(1.0);
    b.ctl().mode = 1;
    b.ctl().lever = 1;
    b.ctl().throttle = 1.0f;
    double peak = 0.0;
    bool finite = true;
    double dMin = 10.0;
    double dMax = 0.0;
    double far = 0.0;
    double vMax = 0.0;
    int changes = 0;
    for (int s = 0; s < seconds; ++s)
    {
        // every second something moves: the camera, the place, the distance, the windows, the ground
        sound(b, "camera", (s * 3) % 7);
        sound(b, "place", (s / 2) % 5);
        sound(b, "distance", 0.1 * double(s % 9));
        sound(b, "windows", (s % 3) * 0.5);
        sound(b, "ground", (s / 5) % 3);
        if (s % 6 == 5)
            b.ctl().shiftUp += 1;
        if (s == 30)
            b.ctl().throttle = 0.0f;
        ++changes;
        b.play(1.0, [&] {
            for (std::size_t i = 0; i < b.left().size(); ++i)
            {
                const double l = b.left()[i];
                const double r = b.right()[i];
                finite = finite && std::isfinite(l) && std::isfinite(r);
                peak = std::max({peak, std::fabs(l), std::fabs(r)});
            }
            const double d = b.tel("sound.doppler");
            dMin = std::min(dMin, d);
            dMax = std::max(dMax, d);
            far = std::max(far, std::fabs(b.tel("sound.roadPosition")));
            vMax = std::max(vMax, b.tel("car.speed"));
        });
    }
    std::printf("soak: %d s, %d changes, the car up to %.1f m/s, peak %.3f, Doppler %.3f to %.3f, trackside up to %.0f m\n",
                seconds, changes, vMax, peak, dMin, dMax, far);
    CHECK(finite);
    CHECK(peak <= 1.0);
    CHECK(peak > 0.05);
    CHECK(dMin > 0.7);
    CHECK(dMax < 1.4);
    CHECK(vMax > vWant);
    }
}

TEST_CASE("soak: a drive through the gears with the listener moving round the car", "[listen][engine][nightly]")
{
    soak(40, 20.0);
}

TEST_CASE("soak, quick: ten seconds of the drive, every camera and place visited", "[listen][engine]")
{
    soak(10, 8.0);
}

TEST_CASE("the cabin: shut windows take the highs, open ones let them in", "[listen][engine]")
{
    Bench b("v8");
    model1(b);
    sound(b, "camera", 4);
    b.start(1.0);
    b.ctl().holdSpeed = 300.0f;
    b.play(1.0);
    // the highs' share: the output's first difference (a 6 dB-an-octave tilt) over the output
    auto highs = [&](double seconds) {
        double d = 0.0;
        double p = 0.0;
        b.play(seconds, [&] {
            for (std::size_t i = 1; i < b.left().size(); ++i)
            {
                const double x = b.left()[i];
                const double dx = x - b.left()[i - 1];
                d += dx * dx;
                p += x * x;
            }
        });
        return 10.0 * std::log10(d / std::max(p, 1e-30));
    };
    const double shut = highs(1.0);
    sound(b, "windows", 1.0);
    b.play(0.3);
    const double open = highs(1.0);
    std::printf("cabin at 2860 rpm: the highs' share %.1f dB with the windows shut, %.1f dB open\n", shut, open);
    CHECK(open > shut + 3.0);
    std::printf("cabin's ways in: floor %.1f, firewall %.1f, glass %.1f, mounts %.1f dB\n", b.tel("sound.pathFloor"),
                b.tel("sound.pathFirewall"), b.tel("sound.pathGlass"), b.tel("sound.pathStructure"));
    CHECK(b.tel("sound.pathFloor") > -150.0);
    CHECK(b.tel("sound.pathFirewall") > -150.0);
    CHECK(b.tel("sound.pathStructure") > -150.0);
}

namespace
{
    void ceiling(int cases)
    {
    // the reference's V8 (its loudest build) and the twin-turbo V8, LEVEL at its most (+12 dB), INTAKE and MECHANICAL
    // at theirs, DISTANCE at its least (0.5 m), at the tailpipes and in the engine bay: free revs against the limiter
    // and lifts (the turbo's blow-off valves), through the resampler (48 kHz)
    const char* builds[] = {"v8", "v8", "boost/twin-turbo-v8"};
    const int cams[] = {0, 3, 0};
    for (int k0 = 0; k0 < cases; ++k0)
    {
        const int cam = cams[k0];
        Bench b(builds[k0]);
        model1(b);
        sound(b, "camera", cam);
        sound(b, "level", 12.0);
        sound(b, "intake", 1.0);
        sound(b, "mechanical", 1.0);
        sound(b, "distance", 0.0);
        sound(b, "autoLevel", 0);       // true levels: the auto level would take the +12 dB back
        b.start(1.0);
        double peak = 0.0;
        double worstLim = 0.0;
        auto watch = [&] {
            for (std::size_t i = 0; i < b.left().size(); ++i)
                peak = std::max({peak, double(std::fabs(b.left()[i])), double(std::fabs(b.right()[i]))});
            worstLim = std::min(worstLim, b.tel("sound.limiter"));
        };
        for (int k = 0; k < 4; ++k)
        {
            b.ctl().throttle = 1.0f;
            b.play(2.0, watch);
            b.ctl().throttle = 0.0f;
            b.play(0.7, watch);
        }
        const double parts = b.tel("sound.ceilingParts");
        std::printf("ceiling, %s, camera %d at 0.5 m, +12 dB: peak %.3f, the limiter down to %.1f dB, the ceiling bent %.0f parts\n",
                    builds[k0], cam, peak, worstLim, parts);
        CHECK(peak < 0.99);
        if (k0 == 0)
            CHECK(worstLim < -3.0);     // it was asked to work
    }
    }
}

TEST_CASE("the ceiling: every source at its loudest and the camera at its closest stays inside full scale", "[listen][engine][nightly]")
{
    ceiling(3);
}

TEST_CASE("the ceiling, quick: the V8 at the tailpipes at 0.5 m, LEVEL +12 dB, on the limiter", "[listen][engine]")
{
    ceiling(1);
}

TEST_CASE("a motorcycle heard from the rider's seat, revved and launched", "[listen][engine]")
{
    Bench b("bike-superbike");
    model1(b);
    sound(b, "camera", 4);
    b.start(1.0);
    double peak = 0.0;
    bool finite = true;
    auto watch = [&] {
        for (std::size_t i = 0; i < b.left().size(); ++i)
        {
            finite = finite && std::isfinite(b.left()[i]) && std::isfinite(b.right()[i]);
            peak = std::max({peak, double(std::fabs(b.left()[i])), double(std::fabs(b.right()[i]))});
        }
    };
    b.ctl().throttle = 1.0f;
    b.play(2.0, watch);
    const double in = b.field("tel.sound.srcLevel[2]");
    const double ex = b.field("tel.sound.srcLevel[0]");
    b.ctl().throttle = 0.0f;
    b.play(1.0, watch);
    std::printf("superbike, rider's seat: peak %.3f; at full throttle the intake %.1f dB, the exhaust %.1f dB at the head\n",
                peak, in, ex);
    CHECK(finite);
    CHECK(peak < 0.99);
    CHECK(peak > 0.01);
}

namespace
{
    struct Rev
    {
        double idle = 0.0;      // the peak at idle; the rev's, as its 0.1 s windows' peaks' 90th percentile (a crack
        double rev = 0.0;       // on the rev limiter, faster than a 16-sample part, passes the auto level to meet the
        double revMax = 0.0;    // limiter: revMax has it); the whole run's
        double all = 0.0;
        bool finite = true;
    };

    // A free rev as content/validation/scenarios/p-free-rev.json has it: the key, idle, the throttle floored 4 s, let go
    Rev freeRev(Bench& b)
    {
        Rev r;
        double* into = &r.all;
        std::vector<double> windows;
        double win = 0.0;
        int blocks = 0;
        bool rev = false;
        auto watch = [&] {
            for (std::size_t i = 0; i < b.left().size(); ++i)
            {
                const double l = b.left()[i];
                const double rr = b.right()[i];
                r.finite = r.finite && std::isfinite(l) && std::isfinite(rr);
                const double a = std::max(std::fabs(l), std::fabs(rr));
                r.all = std::max(r.all, a);
                *into = std::max(*into, a);
                win = std::max(win, a);
            }
            if (rev && ++blocks % 9 == 0)       // 9 blocks of 512 at 48 kHz: 0.096 s
            {
                windows.push_back(win);
                win = 0.0;
            }
        };
        b.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
        b.play(1.75, watch);
        b.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
        b.play(1.25, watch);
        into = &r.idle;
        b.play(2.0, watch);
        into = &r.revMax;
        win = 0.0;
        rev = true;
        b.ctl().throttle = 1.0f;
        b.play(4.0, watch);
        rev = false;
        into = &r.all;
        b.ctl().throttle = 0.0f;
        b.play(3.0, watch);
        std::sort(windows.begin(), windows.end());
        r.rev = windows.empty() ? 0.0 : windows[windows.size() * 9 / 10];
        return r;
    }
}

namespace
{
    void atDefaults(const std::vector<const char*>& builds)
    {
    for (const char* name : builds)
    {
        Bench b(name);
        const Rev r = freeRev(b);
        std::printf("%-34s idle peak %.2f, free rev peak %.2f (its largest %.2f), whole run %.2f, auto level %+.1f dB\n", name,
                    r.idle, r.rev, r.revMax, r.all, b.tel("sound.autoGain"));
        CHECK(r.finite);
        // the rev's largest in the lead's band (0.3 to 0.6, a crack on the rev limiter to the limiter's 0.7); its
        // typical peaks under 0.6 and over 0.15 (the tractor's governed rev is only some 3 dB over its idle: 0.18)
        CHECK(r.revMax > 0.28);
        CHECK(r.revMax < 0.7);
        CHECK(r.rev > 0.15);
        CHECK(r.rev < 0.6);
        CHECK(r.idle > 0.05);           // within some 20 dB of the rev
        CHECK(r.all < 1.0);
    }
    }
}

TEST_CASE("the level at the defaults: a free rev peaks at 0.3 to 0.6, the idle plainly heard, on builds far apart", "[listen][engine][nightly]")
{
    // first.json and three archetypes whose true levels span some 25 dB (the kei triple, the V12, the tractor), each
    // at its own defaults (model 1, camera Rear, place Field, asphalt, the auto level on)
    atDefaults({"first", "archetypes/kei-660-triple-turbo", "archetypes/v12-65", "archetypes/tractor-15-inline-six"});
}

TEST_CASE("the level at the defaults, quick: first.json and the tractor (a quiet idle, a loud let-off)", "[listen][engine]")
{
    atDefaults({"first", "archetypes/tractor-15-inline-six"});
}

TEST_CASE("the auto level: cameras near and far brought together; off, each its true level", "[listen][engine]")
{
    double rev[2][2];
    const int cams[] = {1, 4};          // Rear, Driver (the cabin, some 3 to 7 dB down in truth)
    for (int on = 0; on < 2; ++on)
        for (int c = 0; c < 2; ++c)
        {
            Bench b("first");
            sound(b, "camera", cams[c]);
            sound(b, "autoLevel", on);
            const Rev r = freeRev(b);
            rev[on][c] = 20.0 * std::log10(r.rev);
            CHECK(r.finite);
            CHECK(r.all < 1.0);
            if (on == 0)
                CHECK(std::fabs(b.tel("sound.autoGain")) < 0.01);
        }
    std::printf("free rev, camera Rear and Driver: true levels %.1f and %.1f dB, auto %.1f and %.1f dB\n", rev[0][0],
                rev[0][1], rev[1][0], rev[1][1]);
    CHECK(rev[0][1] < rev[0][0] - 1.5);  // the cabin under the Rear (3.2 to 3.4 dB by the rev's typical peaks)
    CHECK(std::fabs(rev[1][1] - rev[1][0]) < 2.0);
}

namespace
{
    // A spectrum's magnitudes (radix 2, a Hann window), of x's first 2^k samples
    std::vector<double> spectrum(const std::vector<float>& x, int k)
    {
        const std::size_t n = std::size_t(1) << k;
        std::vector<double> re(n), im(n, 0.0);
        for (std::size_t i = 0; i < n; ++i)
            re[i] = double(x[i]) * (0.5 - 0.5 * std::cos(2.0 * 3.14159265358979 * double(i) / double(n)));
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
                    const double c = std::cos(a * double(j));
                    const double s = std::sin(a * double(j));
                    const double ur = re[i + j], ui = im[i + j];
                    const double vr = re[i + j + len / 2] * c - im[i + j + len / 2] * s;
                    const double vi = re[i + j + len / 2] * s + im[i + j + len / 2] * c;
                    re[i + j] = ur + vr;
                    im[i + j] = ui + vi;
                    re[i + j + len / 2] = ur - vr;
                    im[i + j + len / 2] = ui - vi;
                }
        }
        std::vector<double> m(n / 2);
        for (std::size_t i = 0; i < n / 2; ++i)
            m[i] = (re[i] * re[i] + im[i] * im[i]) / double(n);
        return m;
    }

    // first.json warm at idle, the spectrum of 2^16 samples (1.37 s) of its mono mix after 4 s: through the Places
    // camera `cam`, or (cam < 0) the reference's view -1 - cam; voices on or off; free: on a free rev instead (from 1 s
    // after the floor)
    std::vector<double> heardAt(int cam, bool voices, bool free, int ground = -1)
    {
        Bench b("first");
        b.write("spec.voices.model", voices ? 1 : 0);
        if (ground >= 0)
            sound(b, "ground", ground);
        if (cam >= 0)
            sound(b, "camera", cam);
        else
        {
            sound(b, "model", 0);
            sound(b, "view", -1 - cam);
        }
        // held warm as tee-render's "warm" scenarios hold it (host/include/tee/Warm.h): the coolant at the thermostat,
        // so the fan runs as it does in p-idle-warm
        auto hold = [&] {
            b.write("heat.tCool", 368.0);
            b.write("heat.tOil", 378.0);
            b.write("heat.tSurf", 455.0);
            b.write("heat.warm", 1.0);
            b.write("heat.warmS", 1.0);
        };
        hold();
        b.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
        b.play(1.0, hold);
        b.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
        b.play(4.0, hold);
        if (free)
        {
            b.ctl().throttle = 1.0f;
            b.play(1.0, hold);
        }
        std::vector<float> x;
        b.play(1.5, [&] {
            hold();
            for (std::size_t i = 0; i < b.left().size(); ++i)
                x.push_back(0.5f * (b.left()[i] + b.right()[i]));
        });
        return spectrum(x, 16);
    }

    const double kHz = Bench::kRate / 65536.0;

    // The power above 250 Hz, dB
    double highs(const std::vector<double>& m)
    {
        double p = 0.0;
        for (std::size_t i = std::size_t(250.0 / kHz); i < m.size(); ++i)
            p += m[i];
        return 10.0 * std::log10(p + 1e-30);
    }

    // The strongest tone above 300 Hz that the voices make: a peak standing 15 dB over its neighbours (the median of
    // 0.8 to 1.25 of its frequency) and 6 dB over the same bins without the voices; dB of its bins, and where
    double voiceTone(const std::vector<double>& on, const std::vector<double>& off, double& hz)
    {
        double best = -300.0;
        hz = 0.0;
        std::vector<double> nb;
        for (std::size_t i = std::size_t(300.0 / kHz); i < std::size_t(16000.0 / kHz); ++i)
        {
            if (on[i] < on[i - 1] || on[i] < on[i + 1])
                continue;
            nb.clear();
            for (std::size_t j = std::size_t(double(i) * 0.8); j < std::size_t(double(i) * 1.25); ++j)
                if (j + 3 < i || j > i + 3)
                    nb.push_back(on[j]);
            std::nth_element(nb.begin(), nb.begin() + std::ptrdiff_t(nb.size() / 2), nb.end());
            const double p = on[i - 1] + on[i] + on[i + 1];
            const double q = off[i - 1] + off[i] + off[i + 1];
            const double db = 10.0 * std::log10(p + 1e-30);
            if (10.0 * std::log10(on[i] / (nb[nb.size() / 2] + 1e-30)) > 15.0 && p > 4.0 * q && db > best)
            {
                best = db;
                hz = double(i) * kHz;
            }
        }
        return best;
    }
}

namespace
{
    // Package M's voices through Places against the same build through the reference's matching view (Side with
    // Side, Rear with Rear, Driver with Cabin; the reference's mix is what M staged the voices in), held warm as
    // p-idle-warm is (368 K: since f074ab47 the fan comes on at 373 K, so it is off here; the numbers below had it on). Their share of what is heard above 250 Hz (voices on over off) within a
    // camera's tolerance of the reference's; their strongest tone above 300 Hz (15 dB over its neighbours, 6 dB over
    // the same bins without them) against the engine's own highs (the mix above 250 Hz without the voices) no worse
    // than the reference's by more than it; and on a free rev none of theirs within 8 dB of the engine's highs.
    // Measured after the tank's wall (Places / the reference): Side +18.6 / +17.7 dB, the fan's 560 Hz 8.3 / 7.0 dB
    // over the engine's highs; Rear +8.6 / +4.4, 5.6 / 8.1 under; Driver +19.6 / +11.8, 9.4 / 1.4 over; on a rev, none.
    // Without the wall (a still idle, the fan off) Side had a share of +12.2 dB against +3.8 and the pump's 760 Hz
    // 9.5 dB over the engine's highs. The fan's own level is package M's
    void voicesHeld(int cams)
    {
        const int places[] = {2, 1, 4};
        const int refs[] = {2, 1, 4};
        const char* names[] = {"Side", "Rear", "Driver / Cabin"};
        // the tolerance by camera: Side 3 dB; Rear 6 (the fit's worst residual, the bay in REAR: 6.6 dB); the cabin 9,
        // open: its firewall passes the fan's 560 Hz some 8 dB more than the reference's cabin does (L's next)
        const double tol[] = {3.0, 6.0, 9.0};
        for (int k = 0; k < cams; ++k)
        {
            const auto pOn = heardAt(places[k], true, false), pOff = heardAt(places[k], false, false);
            const auto rOn = heardAt(-1 - refs[k], true, false), rOff = heardAt(-1 - refs[k], false, false);
            const double sP = highs(pOn) - highs(pOff);
            const double sR = highs(rOn) - highs(rOff);
            double hzP = 0.0, hzR = 0.0;
            const double under = highs(pOff) - voiceTone(pOn, pOff, hzP);
            const double underR = highs(rOff) - voiceTone(rOn, rOff, hzR);
            std::printf("%-15s idle: the voices' share above 250 Hz %+.1f dB through Places, %+.1f through the reference; "
                        "their strongest tone %.0f Hz, %.1f dB under the engine's highs (the reference's %.0f Hz, %.1f)\n",
                        names[k], sP, sR, hzP, under, hzR, underR);
            CHECK(std::fabs(sP - sR) < tol[k]);
            // the path adds no tone (a tone 12 dB under the engine's highs is under them either way); the voices' own
            // levels are M's (8 dB: M's test)
            CHECK(under > std::min(underR - tol[k], 12.0));
            const auto fOn = heardAt(places[k], true, true), fOff = heardAt(places[k], false, true);
            double hzF = 0.0;
            const double fUnder = highs(fOff) - voiceTone(fOn, fOff, hzF);
            if (hzF > 0.0)
                std::printf("%-15s free rev: the voices' share %+.1f dB, their strongest tone %.0f Hz, %.1f dB under the engine's highs\n",
                            names[k], highs(fOn) - highs(fOff), hzF, fUnder);
            else
                std::printf("%-15s free rev: the voices' share %+.1f dB, no tone of theirs stands out\n", names[k],
                            highs(fOn) - highs(fOff));
            CHECK(fUnder > 8.0);            // on a rev the engine's highs are 20 to 30 dB up: none of theirs stands out
        }
    }
}

TEST_CASE("the voices through Places: their share is the reference view's, no tone of theirs over the engine", "[listen][engine][nightly]")
{
    voicesHeld(3);
}

TEST_CASE("the voices through Places, quick: at Side (first.json's camera)", "[listen][engine]")
{
    voicesHeld(1);
}

namespace
{
    // The octave bands 250 Hz to 8 kHz, each against the 125 Hz band (dB): a camera's tilt, whatever its level
    std::vector<double> tilt(const std::vector<double>& m)
    {
        auto band = [&](double c) {
            double p = 0.0;
            for (std::size_t i = std::size_t(c / 1.41421356 / kHz); i < std::size_t(c * 1.41421356 / kHz) && i < m.size(); ++i)
                p += m[i];
            return 10.0 * std::log10(p + 1e-30);
        };
        const double ref = band(125.0);
        std::vector<double> t;
        for (double c : {250.0, 500.0, 1000.0, 2000.0, 4000.0, 8000.0})
            t.push_back(band(c) - ref);
        return t;
    }

    // The engine alone (no voices) at idle through a Places camera against the reference's view it was fitted on:
    // each octave's tilt within `tol` dB of the reference's (`tol500` at 500 Hz, where the ground's first notch falls)
    void tiltHeld(const char* name, int cam, int view, int ground, double tol, double tol500, std::size_t bands = 6)
    {
        const auto p = tilt(heardAt(cam, false, false, ground));
        const auto r = tilt(heardAt(-1 - view, false, false));
        std::printf("%-30s the engine alone, Places minus the reference's view, dB (250 Hz to 8 kHz, each against 125 Hz):", name);
        for (std::size_t i = 0; i < p.size(); ++i)
            std::printf(" %+.1f", p[i] - r[i]);
        std::printf("\n");
        for (std::size_t i = 0; i < p.size() && i < bands; ++i)
            CHECK(std::fabs(p[i] - r[i]) < (i == 1 ? tol500 : tol));
    }
}

// The engine's tilt by camera, held warm at idle (measured: Tailpipe -1.4 -5.6 -3.6 -2.0 -2.1 -1.2; Side with no
// ground -0.2 -0.2 +1.1 +3.1 +5.2 +3.6, on asphalt -2.1 -8.1 +0.4 +2.3 +2.2 -1.4; Engine bay +0.9 -2.8 +0.9 +1.2 +1.3
// +1.7; Rear -1.0 -3.7 -7.4 -0.8 0.0 -2.9; Driver, since the firewall's mass law, +0.1 +0.6 +3.9 +1.0 -6.5 -12.4,
// before it +0.4 +0.5 +1.4 -6.5 -19.1 -32.3). Tolerances: 7 dB (some 2 dB over the worst reading, the voices and the
// engines still moving); 9 to 10 at 500 Hz where there is ground (its first notch falls near 600 Hz: the reference
// has no ground); Rear 9: not a finding but the camera (the bay shaded by the body and the ground's notch near 1 kHz,
// against a REAR whose bay no geometry gives). The Driver 9 to 4 kHz; at 8 kHz it is 12 dB under the reference's
// CABIN, which hears the bay unfiltered (0.06 of it) where Places brings it through the firewall's mass law
TEST_CASE("the engine's tilt through Places, quick: Tailpipe, and Side without the ground, against the reference's views", "[listen][engine]")
{
    tiltHeld("Tailpipe / Exhaust", 0, 0, -1, 7.0, 9.0);
    tiltHeld("Side (ground None) / Side", 2, 2, 2, 7.0, 7.0);
}

TEST_CASE("the engine's tilt through Places: every fitted camera against the reference's view", "[listen][engine][nightly]")
{
    tiltHeld("Tailpipe / Exhaust", 0, 0, -1, 7.0, 9.0);
    tiltHeld("Side (ground None) / Side", 2, 2, 2, 7.0, 7.0);
    tiltHeld("Side (asphalt) / Side", 2, 2, 0, 7.0, 10.0);
    tiltHeld("Engine bay / Engine", 3, 3, -1, 7.0, 9.0);
    tiltHeld("Rear / Rear (the camera: see listen.md)", 1, 1, -1, 9.0, 9.0);
    tiltHeld("Driver / Cabin (to 4 kHz)", 4, 4, -1, 9.0, 9.0, 5);
}
