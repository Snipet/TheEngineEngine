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
        double idle = 0.0;      // the peak at idle, the throttle floored, the whole run's
        double rev = 0.0;
        double all = 0.0;
        bool finite = true;
    };

    // A free rev as content/validation/scenarios/p-free-rev.json has it: the key, idle, the throttle floored 4 s, let go
    Rev freeRev(Bench& b)
    {
        Rev r;
        double* into = &r.all;
        auto watch = [&] {
            for (std::size_t i = 0; i < b.left().size(); ++i)
            {
                const double l = b.left()[i];
                const double rr = b.right()[i];
                r.finite = r.finite && std::isfinite(l) && std::isfinite(rr);
                const double a = std::max(std::fabs(l), std::fabs(rr));
                r.all = std::max(r.all, a);
                *into = std::max(*into, a);
            }
        };
        b.ctl().ignition = tee::EnginePlayer::kIgnitionStart;
        b.play(1.75, watch);
        b.ctl().ignition = tee::EnginePlayer::kIgnitionOn;
        b.play(1.25, watch);
        into = &r.idle;
        b.play(2.0, watch);
        into = &r.rev;
        b.ctl().throttle = 1.0f;
        b.play(4.0, watch);
        into = &r.all;
        b.ctl().throttle = 0.0f;
        b.play(3.0, watch);
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
        std::printf("%-34s idle peak %.2f, free rev peak %.2f, whole run %.2f, auto level %+.1f dB\n", name, r.idle, r.rev,
                    r.all, b.tel("sound.autoGain"));
        CHECK(r.finite);
        CHECK(r.rev > 0.3);
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
    const int cams[] = {1, 4};          // Rear, Driver (the cabin, some 6 dB down in truth)
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
    CHECK(rev[0][1] < rev[0][0] - 4.0);
    CHECK(std::fabs(rev[1][1] - rev[1][0]) < 2.0);
}
