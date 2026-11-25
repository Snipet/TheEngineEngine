// tests/sim/valvetrain/IntegratedTest.cpp: card E's integrated tests (8 to 13; test 7, model 0, is the null test,
// tests/sim/NullTest.cpp): the whole engine through tee::EnginePlayer, held on its engine dyno, with the valvetrain
// lab (valve.model 1) and the induction (intake.model 1). Each prints what it measured.
//
// The fast tier's budget (01-rules.md §3; moved by the build steward at the lead's leave, no assertion or bound
// changed): 8's quick form runs the default build alone, its full form both builds [nightly]; 12 (three VE sweeps of
// 27 speeds, 48 s; its assertions read the whole sweep, so it has no smaller form) is [nightly] whole.
#include "Engine.h"

#include <initializer_list>
#include <catch2/catch_approx.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>
#include <vector>

using tee::test::lab::Rig;

namespace
{
    using Fields = std::vector<std::pair<std::string, double>>;

    const Fields kHot = {{"valve.model", 1.0},     {"valve.inDurSeat", 280.0}, {"valve.inDur1mm", 246.0},
                         {"valve.inLift", 0.012},  {"valve.exDurSeat", 280.0}, {"valve.exDur1mm", 246.0},
                         {"valve.exLift", 0.012}};

    Fields with(Fields f, const Fields& more)
    {
        f.insert(f.end(), more.begin(), more.end());
        return f;
    }

    std::vector<double> curve(const std::string& build, const Fields& f, const std::vector<double>& rpms,
                              double throttle = 1.0)
    {
        Rig r(build, f);
        r.start();
        std::vector<double> t;
        for (double rpm : rpms)
            t.push_back(r.torqueAt(rpm, throttle, 0.8, 0.4));
        return t;
    }

    // The peak of a curve by a parabola through its largest point and its neighbours
    double peakOf(const std::vector<double>& rpms, const std::vector<double>& t)
    {
        const auto i = std::size_t(std::max_element(t.begin(), t.end()) - t.begin());
        if (i == 0 || i + 1 >= t.size())
            return rpms[i];
        const double a = t[i - 1], b = t[i], c = t[i + 1];
        const double d = a - 2.0 * b + c;
        return rpms[i] + (d != 0.0 ? 0.5 * (a - c) / d : 0.0) * (rpms[i + 1] - rpms[i]);
    }

    void print(const char* what, const std::vector<double>& rpms, const std::vector<double>& t)
    {
        std::printf("[e] %s:", what);
        for (std::size_t i = 0; i < t.size(); ++i)
            std::printf(" %.0f@%.0f", t[i], rpms[i]);
        std::printf("\n");
        std::fflush(stdout);
    }

    // The power of x at f Hz (Goertzel), x at rate Hz
    double powerAt(const std::vector<float>& x, double f, double rate)
    {
        const double w = 2.0 * 3.14159265358979 * f / rate;
        const double k = 2.0 * std::cos(w);
        double s1 = 0.0, s2 = 0.0;
        for (float v : x)
        {
            const double s0 = v + k * s1 - s2;
            s2 = s1;
            s1 = s0;
        }
        return (s1 * s1 + s2 * s2 - k * s1 * s2) / double(x.size() * x.size());
    }
} // namespace

namespace
{
    // the body of "8: an advanced intake cam...": every case of `cases` (the quick form takes the first, the full form all)
    void caseOf_8__an_advanced_intake_ca(std::initializer_list<const char*> cases)
{
    const std::vector<double> rpms = {1500, 2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000};
    for (const char* build : cases)
    {
        const auto adv = curve(build, with(kHot, {{"valve.inCentre", 95.0}}), rpms);
        const auto ret = curve(build, with(kHot, {{"valve.inCentre", 125.0}}), rpms);
        std::printf("[e] %s, a 280 degree cam, intake centreline 95 and 125\n", build);
        print("  95 ", rpms, adv);
        print("  125", rpms, ret);
        const double low = adv[0] / ret[0];
        const double top = adv.back() / ret.back();
        std::printf("[e]   advanced over retarded: %+.1f %% at 1500 rpm, %+.1f %% at 6000; peaks %.0f and %.0f rpm\n",
                    100.0 * (low - 1.0), 100.0 * (top - 1.0), peakOf(rpms, adv), peakOf(rpms, ret));
        // low down: more, by a few percent to twenty
        CHECK(low > 1.02);
        CHECK(low < 1.25);
        // at the top the advantage is gone or reversed
        CHECK(top < low - 0.03);
        CHECK(top < 1.02);
        // a later intake closing moves the peak up (or keeps it)
        CHECK(peakOf(rpms, ret) >= peakOf(rpms, adv) - 100.0);
    }
}
} // namespace

// The quick form, in every run (01-rules.md §3, the build steward at the lead's leave): the default build alone.
TEST_CASE("8: an advanced intake cam fills in the torque low down and costs it at the top; later closing moves the peak up (quick: the default build alone)", "[valvetrain][integrated][part-1]")
{
    caseOf_8__an_advanced_intake_ca({"default"});
}

// The full form, nightly: every case.
TEST_CASE("8: an advanced intake cam fills in the torque low down and costs it at the top; later closing moves the peak up", "[valvetrain][integrated][nightly]")
{
    caseOf_8__an_advanced_intake_ca({"default", "engines/ref-05-inline-4"});
}

TEST_CASE("9: two stages: the curves cross, and a change set at the crossing shows no dip", "[valvetrain][integrated][part-1]")
{
    const Fields stages = {{"valve.model", 1.0},       {"valve.stages", 1.0},       {"valve.valves", 4.0},
                           {"valve.inDurSeat", 230.0}, {"valve.inDur1mm", 205.0},   {"valve.inLift", 0.0085},
                           {"valve.exDurSeat", 232.0}, {"valve.exDur1mm", 206.0},   {"valve.exLift", 0.0080},
                           {"valve.inDurSeatB", 290.0}, {"valve.inDur1mmB", 255.0}, {"valve.inLiftB", 0.0115},
                           {"valve.exDurSeatB", 286.0}, {"valve.exDur1mmB", 250.0}, {"valve.exLiftB", 0.0105}};
    const char* build = "engines/ref-05-inline-4";
    const std::vector<double> rpms = {2000, 2500, 3000, 3500, 4000, 4500, 5000, 5500, 6000, 6500};
    const auto a = curve(build, with(stages, {{"valve.stageSpeed", 1100.0}}), rpms);
    const auto b = curve(build, with(stages, {{"valve.stageSpeed", 100.0}}), rpms);
    print("stage A", rpms, a);
    print("stage B", rpms, b);
    // they cross: A above at the bottom, B above at the top
    REQUIRE(a.front() > b.front());
    // (at the top speed that still makes torque: the last of the speeds is on the limiter's cut, both curves some -200
    // N m there and their order the cut's, not the cams')
    std::size_t top = rpms.size() - 1;
    while (top > 0 && (a[top] <= 0.0 || b[top] <= 0.0))
        --top;
    REQUIRE(b[top] > a[top]);
    double cross = 0.0;
    for (std::size_t i = 1; i < rpms.size(); ++i)
        if (a[i - 1] >= b[i - 1] && a[i] < b[i])
        {
            const double d0 = a[i - 1] - b[i - 1], d1 = a[i] - b[i];
            cross = rpms[i - 1] + d0 / (d0 - d1) * (rpms[i] - rpms[i - 1]);
        }
    REQUIRE(cross > 0.0);
    const double crossTq = [&] {
        for (std::size_t i = 1; i < rpms.size(); ++i)
            if (rpms[i] >= cross)
                return a[i - 1] + (a[i] - a[i - 1]) * (cross - rpms[i - 1]) / (rpms[i] - rpms[i - 1]);
        return 0.0;
    }();
    // a sweep through the crossing with the change set there: the torque each block, and the output's envelope
    Rig r(build, with(stages, {{"valve.stageSpeed", cross * 3.14159265358979 / 30.0}, {"valve.stageHyst", 5.0}}));
    r.start();
    r.torqueAt(cross - 800.0, 1.0, 1.0, 0.1);
    r.ctl().dynoRate = float(400.0 * 3.14159265358979 / 30.0);       // 400 rpm a second
    r.keep(true);
    std::vector<double> speed, tq, stage;
    r.play(4.0, [&] {
        speed.push_back(r.tel("engine.speed") * 30.0 / 3.14159265358979);
        tq.push_back(r.tel("engine.gasTorque"));
        stage.push_back(r.tel("valve.stage"));
    });
    std::size_t at = 0;
    for (std::size_t i = 1; i < stage.size(); ++i)
        if (stage[i] != stage[i - 1])
            at = i;
    REQUIRE(at > 0);
    // no dip: within the 0.25 s after the change the torque stays above the crossing's less 4 %
    double least = 1e9;
    for (std::size_t i = at; i < std::min(stage.size(), at + 24); ++i)
        least = std::min(least, tq[i]);
    std::printf("[e] stages cross at %.0f rpm (%.0f N m); changed at %.0f rpm; least gas torque after it %.0f N m\n", cross,
                crossTq, speed[at], least);
    CHECK(least > 0.96 * crossTq);
    // the output: its level a firing at a time (the four's firing: half a turn); the step at the change no larger
    // than the largest between two firings elsewhere in the sweep
    const auto& out = r.output();
    std::vector<double> env;
    std::vector<std::size_t> envAt;
    std::size_t i = 0;
    while (i < out.size())
    {
        const std::size_t block = std::min(out.size() - 1, i / std::size_t(Rig::kBlock));
        const double rpm = speed[std::min(block, speed.size() - 1)];
        const std::size_t n = std::size_t(Rig::kRate * 30.0 / std::max(rpm, 500.0));
        double s = 0.0;
        for (std::size_t j = i; j < std::min(out.size(), i + n); ++j)
            s += double(out[j]) * out[j];
        env.push_back(std::sqrt(s / double(n)));
        envAt.push_back(block);
        i += n;
    }
    double stepAt = 0.0, stepElse = 0.0;
    for (std::size_t k = 2; k < env.size(); ++k)
    {
        const double step = std::fabs(std::log(env[k] / env[k - 1]));
        if (envAt[k] + 2 >= at && envAt[k] <= at + 6)
            stepAt = std::max(stepAt, step);
        else
            stepElse = std::max(stepElse, step);
    }
    std::printf("[e] the output's level, firing to firing: the largest step at the change %.2f dB, elsewhere %.2f dB\n",
                8.686 * stepAt, 8.686 * stepElse);
    CHECK(stepAt <= stepElse);
}

TEST_CASE("10: deactivation on a V8: half the firing lines, and the manifold's pressure up for the same torque",
          "[valvetrain][integrated][part-2]")
{
    const double rpm = 2000.0;
    const double want = 90.0;      // N m: light load
    auto run = [&](int mask, double& throttle, double& map, std::vector<float>& out) {
        Rig r("default", {{"valve.model", 1.0}, {"valve.deacMask", double(mask)}});
        r.start();
        // the throttle for the torque wanted: a bisection, each step settled on the dyno
        double lo = 0.02, hi = 0.34;
        for (int it = 0; it < 7; ++it)
        {
            const double mid = 0.5 * (lo + hi);
            if (r.torqueAt(rpm, mid, 0.7, 0.3) < want)
                lo = mid;
            else
                hi = mid;
        }
        throttle = 0.5 * (lo + hi);
        r.torqueAt(rpm, throttle, 0.8, 0.2);
        map = r.tel("engine.manifold");
        CHECK(r.tel("valve.active") == (mask == 0 ? 255.0 : double(255 & ~mask)));
        r.keep(true);
        r.play(1.0);
        out = r.output();
    };
    double thrAll, mapAll, thrHalf, mapHalf;
    std::vector<float> outAll, outHalf;
    run(0, thrAll, mapAll, outAll);
    run(105, thrHalf, mapHalf, outHalf);
    const double fire = rpm / 60.0 * 4.0;      // eight cylinders: four firings a turn
    auto lines = [&](const std::vector<float>& x) {
        return 10.0 * std::log10(powerAt(x, 0.5 * fire, Rig::kRate) / powerAt(x, fire, Rig::kRate));
    };
    std::printf("[e] V8 at %.0f rpm, %.0f N m: all eight, throttle %.3f, manifold %.1f kPa, the half-firing line %+.1f dB "
                "against the firing's; four resting, throttle %.3f, manifold %.1f kPa, %+.1f dB\n",
                rpm, want, thrAll, mapAll / 1000.0, lines(outAll), thrHalf, mapHalf / 1000.0, lines(outHalf));
    CHECK(mapHalf > mapAll + 5000.0);
    CHECK(lines(outHalf) > lines(outAll) + 6.0);
}

TEST_CASE("11: throttles a runner: the trumpets louder at full throttle than the plenum's box, the idle's ripple larger",
          "[valvetrain][integrated][part-2]")
{
    auto measure = [](int kind, double& radiated, double& ripple) {
        Rig r("default", {{"intake.model", 1.0}, {"intake.throttleKind", double(kind)}, {"intake.trumpets", 1.0}});
        r.start(2.0);
        r.ctl().throttle = 0.0f;
        r.play(1.0);
        ripple = r.tel("intake.ripple");
        r.torqueAt(4000.0, 1.0, 1.0, 0.1);
        double s = 0.0;
        int n = 0;
        r.play(0.5, [&] {
            s += r.field("intk.soundMs");
            n += 1;
        });
        radiated = std::sqrt(s / n);
    };
    double rPlenum, ripPlenum, rItb, ripItb;
    measure(0, rPlenum, ripPlenum);
    measure(1, rItb, ripItb);
    const double db = 20.0 * std::log10(rItb / rPlenum);
    std::printf("[e] at 4000 rpm, wide open: the trumpets radiate %+.1f dB against the plenum's snorkel; the idle's "
                "manifold ripple %.1f kPa with ITBs, %.1f kPa with the plenum\n", db, ripItb / 1000.0, ripPlenum / 1000.0);
    CHECK(db > 10.0);
    CHECK(ripItb > 1.5 * ripPlenum);
}

TEST_CASE("12: the runner's tuning: VE's bump moves with the runner's length as the acoustics say, above 1 where tuned, "
          "and two lengths give the envelope of both", "[valvetrain][integrated][nightly]")
{
    // A 2.0 L four (the inline four's layout at 2 L, a 280-degree cam, four valves, its redline raised 1500 rpm so the
    // limiter stays out of the sweep), 300 K air, wide open on the engine dyno. VE (tel.engine.veAir: the mass trapped
    // over what the swept volume holds at the air outside) against speed for runners of 0.40 and 0.60 m. Each curve
    // has two bumps, the runner's ring meeting the intake event every cycle (the main one) and every other (the lower,
    // at about half the speed). The acoustics: with the valve open the runner rings as a pipe loaded by the cylinder's
    // volume (x tan x = A L / V, f = x c / 2 pi L, L with its open end's 0.6 r; V the cylinder's mean volume over the
    // intake stroke), and the tuned speed goes with that frequency.
    const Fields engine = {{"engine.displacement", 0.002}, {"engine.redlineTrim", 1500.0}, {"env.temperature", 300.0},
                           {"intake.model", 1.0},         {"valve.model", 1.0},          {"valve.valves", 4.0},
                           {"valve.inDurSeat", 280.0},    {"valve.inDur1mm", 246.0},     {"valve.inLift", 0.010},
                           {"valve.exDurSeat", 276.0},    {"valve.exDur1mm", 242.0},     {"valve.exLift", 0.009}};
    const char* build = "engines/ref-05-inline-4";
    std::vector<double> rpms;
    for (double x = 1750.0; x <= 8250.0; x += 250.0)
        rpms.push_back(x);
    auto veCurve = [&](const Fields& f, double& area, double& vd, double& vc) {
        Rig r(build, f);
        r.start(0.5);
        area = r.field("intk.run[0].area");
        vd = r.field("cyl[0].vd");
        vc = r.field("cyl[0].vc");
        std::vector<double> ve;
        for (double rpm : rpms)
        {
            r.torqueAt(rpm, 1.0, 1.2, 0.05);
            double v = 0.0;
            int n = 0;
            r.play(0.6, [&] {
                v += r.tel("engine.veAir");
                n += 1;
            });
            ve.push_back(v / n);
        }
        return ve;
    };
    double area = 0.0, vd = 0.0, vc = 0.0;
    const double l1 = 0.40, l2 = 0.60;
    const auto v1 = veCurve(with(engine, {{"intake.runnerLength", l1}}), area, vd, vc);
    const auto v2 = veCurve(with(engine, {{"intake.runnerLength", l2}}), area, vd, vc);
    const auto vs = veCurve(with(engine, {{"intake.runnerLength", l2}, {"intake.runnerLength2", l1}, {"intake.runnerSwitch", 6300.0 * 3.14159265358979 / 30.0}}),
                            area, vd, vc);
    print("VE x 1000, runner 0.40 m", rpms, [&] { std::vector<double> x; for (double v : v1) x.push_back(1000 * v); return x; }());
    print("VE x 1000, runner 0.60 m", rpms, [&] { std::vector<double> x; for (double v : v2) x.push_back(1000 * v); return x; }());
    print("VE x 1000, 0.60 m then 0.40 from 6300 rpm", rpms, [&] { std::vector<double> x; for (double v : vs) x.push_back(1000 * v); return x; }());
    // the peaks: each curve's largest above 4000 rpm (the main bump) and below 4000 (the lower)
    auto peakIn = [&](const std::vector<double>& v, double lo, double hi) {
        std::vector<double> x, y;
        for (std::size_t i = 0; i < rpms.size(); ++i)
            if (rpms[i] >= lo && rpms[i] <= hi)
            {
                x.push_back(rpms[i]);
                y.push_back(v[i]);
            }
        return std::make_pair(peakOf(x, y), *std::max_element(y.begin(), y.end()));
    };
    const auto main1 = peakIn(v1, 4000.0, 8250.0), main2 = peakIn(v2, 4000.0, 8250.0);
    // (the lower bump's window ends at 4000 rpm or three quarters of the main bump's speed, the lower: card N1's loss
    // at the runner's end in the plenum (intake.fm, Idelchik's K) damps the lower bumps as a real manifold's are, and the
    // 0.60 m runner's main bump, rising from 4000 rpm, then outranked its own lower bump at 2250 at the window's edge)
    const auto low1 = peakIn(v1, 1750.0, std::fmin(4000.0, 0.75 * main1.first));
    const auto low2 = peakIn(v2, 1750.0, std::fmin(4000.0, 0.75 * main2.first));
    // the acoustics' ratio
    const double r = std::sqrt(area / 3.14159265358979);
    const double v = vc + 0.5 * vd;
    auto ring = [&](double l) {
        const double le = l + 0.6 * r;
        const double k = area * le / v;
        double lo = 0.0, hi = 1.5707;
        for (int it = 0; it < 60; ++it)
        {
            const double mid = 0.5 * (lo + hi);
            (mid * std::tan(mid) < k ? lo : hi) = mid;
        }
        return 0.5 * (lo + hi) * 350.0 / (2.0 * 3.14159265358979 * le);
    };
    const double predicted = ring(l2) / ring(l1);
    std::printf("[e] the runner loaded by the cylinder rings at %.0f Hz (0.40 m) and %.0f Hz (0.60 m): ratio %.3f\n", ring(l1),
                ring(l2), predicted);
    std::printf("[e] main bump %.0f rpm (VE %.3f) with 0.40 m, %.0f rpm (VE %.3f) with 0.60: ratio %.3f; lower bump %.0f and "
                "%.0f rpm: ratio %.3f\n", main1.first, main1.second, main2.first, main2.second, main2.first / main1.first,
                low1.first, low2.first, low2.first / low1.first);
    CHECK(main2.first < main1.first);
    CHECK(std::fabs(main2.first / main1.first - predicted) < 0.2 * predicted);
    CHECK(std::fabs(low2.first / low1.first - predicted) < 0.2 * predicted);
    // above 1 where tuned
    CHECK(main2.second > 1.0);
    // two lengths: from 4000 rpm up, the envelope of both main bumps (within 3 % of the better at each speed away from
    // the change at 6300). (Below 4000 one switch cannot follow both lower bumps: the short runner's at 3200 is lost.)
    double worst = 0.0;
    for (std::size_t i = 0; i < rpms.size(); ++i)
        if (rpms[i] >= 4000.0 && std::fabs(rpms[i] - 6300.0) > 300.0)
            worst = std::max(worst, std::max(v1[i], v2[i]) / vs[i] - 1.0);
    std::printf("[e] two lengths, from 4000 rpm: the most the envelope of both is above it, away from the change: %.1f %%\n", 100.0 * worst);
    CHECK(worst < 0.03);
}

TEST_CASE("13: past the float speed the power falls; nothing goes non-finite at twice it", "[valvetrain][integrated][part-3]")
{
    const char* build = "engines/ref-05-inline-4";
    // springs that let the four's valves float near 5000 rpm
    const Fields springs = {{"valve.model", 1.0}, {"valve.train", 0.0}, {"valve.springRate", 30000.0},
                            {"valve.springPreload", 250.0}, {"valve.valveMass", 0.10}};
    // (the probe gone before the curves, so each Rig after it takes the one kept player: tests/sim/valvetrain/Engine.h)
    const double rf = [&] {
        Rig probe(build, springs);
        probe.start(0.3);
        return probe.tel("valve.floatSpeed") * 30.0 / 3.14159265358979;
    }();
    REQUIRE(rf > 3000.0);
    REQUIRE(rf < 7000.0);
    const std::vector<double> rpms = {0.85 * rf, 1.1 * rf, 1.25 * rf};
    const auto stiff = curve(build, {{"valve.model", 1.0}}, rpms);
    const auto soft = curve(build, springs, rpms);
    std::printf("[e] float at %.0f rpm: torque without float / with: %.0f/%.0f at 0.85, %.0f/%.0f at 1.1, %.0f/%.0f at 1.25 "
                "times it\n", rf, stiff[0], soft[0], stiff[1], soft[1], stiff[2], soft[2]);
    // below it the same; past it a cliff: a tenth or more lost 10 % past the speed, a third or more 25 % past it
    CHECK(soft[0] == Catch::Approx(stiff[0]).epsilon(0.02));
    CHECK(soft[1] < 0.9 * stiff[1]);
    CHECK(soft[2] / stiff[2] < soft[1] / stiff[1]);
    CHECK(soft[2] < 0.67 * stiff[2]);
    // twice it, held by the dyno: finite, within the output's bounds
    Rig r(build, springs);
    r.start(0.3);
    r.keep(true);
    r.torqueAt(2.0 * rf, 1.0, 1.0, 0.5);
    bool finite = true;
    float most = 0.0f;
    for (float v : r.output())
    {
        finite = finite && std::isfinite(v);
        most = std::max(most, std::fabs(v));
    }
    CHECK(finite);
    CHECK(most <= 1.0f);
    CHECK(std::isfinite(r.tel("engine.torque")));
    CHECK(r.tel("valve.floating") == 1.0);
}
