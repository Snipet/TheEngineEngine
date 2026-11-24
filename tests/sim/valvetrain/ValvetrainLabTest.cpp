// tests/sim/valvetrain/ValvetrainLabTest.cpp: card E's standalone tests of the valvetrain lab (sim/valvetrain/): the
// cam generator, lift to area, the phaser, the stage change and deactivation, valve float. Each compiles the lab alone
// (valvetrain/lab.fm) and calls its modules' methods by name: no engine.
#include "Lab.h"

#include <catch2/catch_approx.hpp>

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <cmath>
#include <cstdio>
#include <vector>

using tee::test::lab::Obj;

namespace
{
    tee::Program& labProgram() { return tee::test::lab::program("valvetrain/lab.fm"); }

    struct CamAsk
    {
        const char* name;
        float durSeat;
        float dur1mm;
        float lift;
        float centre;   // the cycle's degrees
        float fat;
    };

    // What a curve measures, from the generated curve itself (lift01, 0.01 degree steps), not from the solver
    struct CamMeasured
    {
        float durSeat = 0;
        float dur1mm = 0;
        float lift = 0;
        float centre = 0;
        float minLift = 0;
        float accJump = 0;      // the acceleration's largest change between 2 degree steps, over that at 4 degrees
    };

    CamMeasured measure(Obj& cam)
    {
        auto* lift01 = cam.fn<float(void*, float)>("lift01");
        const double span = cam.get("span");
        const double open = cam.get("open");
        const double lift = cam.get("lift");
        const double h = 0.01;
        CamMeasured m;
        int nSeat = 0;
        int n1 = 0;
        double best = -1.0;
        double bestAt = 0.0;
        double minL = 1.0;
        const int n = static_cast<int>(span / h) + 1;
        std::vector<double> l(static_cast<std::size_t>(n) + 2);
        for (int i = 0; i <= n + 1; ++i)
            l[static_cast<std::size_t>(i)] = lift * lift01(cam.data(), static_cast<float>((i - 1) * h));
        for (int i = 1; i <= n; ++i)
        {
            const double x = l[static_cast<std::size_t>(i)];
            nSeat += x >= 0.0001524 ? 1 : 0;
            n1 += x >= 0.001 ? 1 : 0;
            minL = std::min(minL, x);
            if (x > best)
            {
                best = x;
                bestAt = (i - 1) * h;
            }
        }
        m.durSeat = static_cast<float>(nSeat * h);
        m.dur1mm = static_cast<float>(n1 * h);
        m.lift = static_cast<float>(best);
        m.centre = static_cast<float>(std::fmod(open + bestAt, 720.0));
        m.minLift = static_cast<float>(minL);
        // the acceleration's continuity: the most it changes between two steps, at steps of 4 and of 2 degrees (shorter
        // ones measure the float's rounding). A continuous acceleration changes half as much over half the step; a
        // kink in it changes as much whatever the step
        auto jumpAt = [&](int step) {
            std::vector<double> acc;
            for (int i = step; i + step <= n; i += step)
                acc.push_back((l[static_cast<std::size_t>(i + step)] - 2.0 * l[static_cast<std::size_t>(i)] +
                               l[static_cast<std::size_t>(i - step)]) / (double(step) * step));
            double jump = 0.0;
            for (std::size_t i = 1; i < acc.size(); ++i)
                jump = std::max(jump, std::fabs(acc[i] - acc[i - 1]));
            return jump;
        };
        const double big = jumpAt(400);
        const double jump = jumpAt(200);
        m.accJump = static_cast<float>(jump / big);   // about 0.5 when continuous
        return m;
    }

    float wrapNear(float a, float ref)
    {
        float d = a - ref;
        while (d > 360.0f)
            d -= 720.0f;
        while (d < -360.0f)
            d += 720.0f;
        return ref + d;
    }
} // namespace

TEST_CASE("cam generator: what was asked is what the curve measures", "[valvetrain][cam]")
{
    // a mild stock cam, a hot street cam, a race cam (the cycle's degrees: 470 is 110 after the overlap's top)
    const CamAsk asks[] = {
        {"mild", 240.0f, 210.0f, 0.009f, 468.0f, 0.5f},
        {"hot", 280.0f, 245.0f, 0.012f, 466.0f, 0.5f},
        {"race", 310.0f, 270.0f, 0.015f, 464.0f, 0.65f},
        {"reference-like", 231.0f, 212.0f, 0.010f, 470.0f, 0.5f},
    };
    for (const CamAsk& a : asks)
    {
        Obj cam(labProgram(), "CamShape");
        cam.call<void>("request", a.durSeat, a.dur1mm, a.lift, a.centre, a.fat);
        cam.call<void>("build");
        const CamMeasured m = measure(cam);
        INFO(a.name << ": seat " << m.durSeat << " 1mm " << m.dur1mm << " lift " << m.lift << " centre " << m.centre
                    << " p " << cam.get("p") << " k " << cam.get("k") << " span " << cam.get("span"));
        CHECK(cam.get("reached") == 1.0);
        CHECK(std::fabs(m.durSeat - a.durSeat) <= 1.0f);
        CHECK(std::fabs(m.dur1mm - a.dur1mm) <= 1.0f);
        CHECK(std::fabs(m.lift - a.lift) <= 0.01f * a.lift);
        CHECK(std::fabs(wrapNear(m.centre, a.centre) - a.centre) <= 1.0f);
        CHECK(m.minLift >= 0.0f);
        // the acceleration is continuous (a kink would keep its jump whatever the step)
        CHECK(m.accJump < 0.7f);
        // the 129-point table is the curve at its points
        const double span = cam.get("span");
        auto* lift01 = cam.fn<float(void*, float)>("lift01");
        for (int i = 0; i <= 128; i += 8)
            CHECK(cam.get("shape[" + std::to_string(i) + "]") ==
                  Catch::Approx(lift01(cam.data(), static_cast<float>(span * i / 128.0))).margin(1e-6));
    }
}

TEST_CASE("cam generator: a free valve opens and closes where it is told", "[valvetrain][cam]")
{
    Obj cam(labProgram(), "CamShape");
    cam.call<void>("requestFree", 350.0f, 560.0f, 0.009f, 40.0f);     // an early intake closing (Miller)
    cam.call<void>("build");
    CHECK(cam.get("open") == Catch::Approx(350.0).margin(0.01));
    CHECK(cam.get("span") == Catch::Approx(210.0).margin(0.01));
    const CamMeasured m = measure(cam);
    CHECK(m.lift == Catch::Approx(0.009).epsilon(0.001));
    CHECK(m.minLift >= 0.0f);
    CHECK(m.accJump < 0.7f);
}

TEST_CASE("area against lift: the curtain low down, flat past a quarter of the diameter; four valves beat two",
          "[valvetrain][area]")
{
    const float bore = 0.1016f;     // a small-block's 4.00 in
    Obj head(labProgram(), "Head");
    head.call<void>("setup", 2, bore, 0.0f, 0.0f, 1.0f, 1.0f);
    const double d = head.get("dIn");
    CHECK(d == Catch::Approx(0.48 * bore).epsilon(1e-6));
    auto* areaIn = head.fn<float(void*, float)>("areaIn");
    // low down: the curtain, pi d L, times a discharge coefficient near 0.73
    const float l0 = static_cast<float>(0.01 * d);
    const double curtain = 3.14159265 * d * l0;
    CHECK(areaIn(head.data(), l0) / curtain == Catch::Approx(0.727).epsilon(0.01));
    // rises with the lift, never falls
    float last = 0.0f;
    for (int i = 1; i <= 100; ++i)
    {
        const float a = areaIn(head.data(), static_cast<float>(0.005 * i * d));
        CHECK(a > last);
        last = a;
    }
    // flattens: past a quarter of the diameter the slope is under a fifth of the curtain's
    const float q = static_cast<float>(0.25 * d);
    const float dl = static_cast<float>(0.01 * d);
    const double slope0 = (areaIn(head.data(), 2 * dl) - areaIn(head.data(), dl)) / dl;
    const double slopeQ = (areaIn(head.data(), q + dl) - areaIn(head.data(), q)) / dl;
    CHECK(slopeQ < 0.2 * slope0);
    // at a quarter: 0.4 of the valve's area
    CHECK(areaIn(head.data(), q) == Catch::Approx(0.4 * 0.785398 * d * d).epsilon(0.001));
    // four valves (two intakes) beat two at the same bore and lift, on the bench at every lift
    Obj four(labProgram(), "Head");
    four.call<void>("setup", 4, bore, 0.0f, 0.0f, 1.0f, 1.0f);
    auto* areaIn4 = four.fn<float(void*, float)>("areaIn");
    auto* areaEx4 = four.fn<float(void*, float)>("areaEx");
    auto* areaEx2 = head.fn<float(void*, float)>("areaEx");
    for (int i = 1; i <= 16; ++i)
    {
        const float l = 0.00127f * static_cast<float>(i);
        CHECK(areaIn4(four.data(), l) > areaIn(head.data(), l));
        CHECK(areaEx4(four.data(), l) > areaEx2(head.data(), l));
    }
    // the flow bench: a small-block's two-valve head at 0.500 in, in CFM (reported)
    Obj bench(labProgram(), "FlowBench");
    bench.call<void>("measure", head.data());
    const double cfm500 = bench.get("flowIn[9]") * 2118.88;
    Obj bench4(labProgram(), "FlowBench");
    bench4.call<void>("measure", four.data());
    std::printf("[valvetrain] flow bench at 28 in H2O, 0.500 in lift, 4.00 in bore: two valves %.0f CFM intake, "
                "four valves %.0f CFM\n", cfm500, bench4.get("flowIn[9]") * 2118.88);
    CHECK(cfm500 > 120.0);
    CHECK(cfm500 < 300.0);
    // the lab at its defaults (ports 1.35 and 1.5, 10 mm of lift) gives the reference V8's areas: 0.118 and 0.091 of
    // the piston's (sim/ref/engine.fm, valves)
    Obj spec(labProgram(), "ValveSpec");
    spec.defaults();
    Obj lab(labProgram(), "ValveLab");
    lab.call<void>("setup", bore, 8, 720.0f);
    lab.call<void>("reset");
    lab.call<void>("apply", spec.data(), 1);
    const double piston = 0.785398 * bore * bore;
    CHECK(lab.get("tab[0].peak") == Catch::Approx(0.118 * piston).epsilon(0.005));
    CHECK(lab.get("tab[2].peak") == Catch::Approx(0.091 * piston).epsilon(0.005));
}

TEST_CASE("phaser: a step at its slew rate, slower cold, parked colder, never out of its range", "[valvetrain][phaser]")
{
    const float d16 = 16.0f / 96000.0f;
    auto run = [&](float tOil, float pOil, float target, float seconds, std::vector<float>* trace) {
        Obj ph(labProgram(), "Phaser");
        ph.call<void>("reset");
        ph.call<void>("setup", 50.0f, 200.0f);
        auto* control = ph.fn<void(void*, float, float, float, float)>("control");
        const int ticks = static_cast<int>(seconds / d16);
        float lo = 1e9f;
        float hi = -1e9f;
        for (int i = 0; i < ticks; ++i)
        {
            control(ph.data(), d16, target, pOil, tOil);
            const float p = static_cast<float>(ph.get("phase"));
            lo = std::min(lo, p);
            hi = std::max(hi, p);
            if (trace != nullptr)
                trace->push_back(p);
        }
        CHECK(lo >= 0.0f);
        CHECK(hi <= 50.0f);
        return static_cast<float>(ph.get("phase"));
    };
    // warm, full pressure: 40 degrees at 200 a second is 0.2 s
    std::vector<float> warm;
    run(363.0f, 300000.0f, 40.0f, 0.4f, &warm);
    const int at01 = static_cast<int>(0.1f / d16);
    const int at025 = static_cast<int>(0.25f / d16);
    CHECK(warm[static_cast<std::size_t>(at01)] == Catch::Approx(20.0).margin(0.5));
    CHECK(warm[static_cast<std::size_t>(at025)] == Catch::Approx(40.0).margin(1e-3));
    // cold oil (50 C) moves it slower; colder than 40 C it stays parked
    std::vector<float> cool;
    run(323.0f, 300000.0f, 40.0f, 0.4f, &cool);
    CHECK(cool[static_cast<std::size_t>(at01)] < 0.6f * warm[static_cast<std::size_t>(at01)]);
    CHECK(cool[static_cast<std::size_t>(at01)] > 0.0f);
    CHECK(run(300.0f, 300000.0f, 40.0f, 0.4f, nullptr) == 0.0f);
    // no oil pressure: parked
    CHECK(run(363.0f, 30000.0f, 40.0f, 0.4f, nullptr) == 0.0f);
    // a target past its stops: held at them
    CHECK(run(363.0f, 300000.0f, 90.0f, 0.6f, nullptr) == 50.0f);
    CHECK(run(363.0f, 300000.0f, -20.0f, 0.4f, nullptr) == 0.0f);
}

namespace
{
    // A V8's lab, its Spec at the defaults (valve.model 1) and what a test changes
    struct V8Lab
    {
        Obj spec{labProgram(), "ValveSpec"};
        Obj lab{labProgram(), "ValveLab"};
        float theta = 0.0f;
        std::vector<float> ang = std::vector<float>(16, 0.0f);
        const float fire[8] = {0.0f, 90.0f, 180.0f, 270.0f, 360.0f, 450.0f, 540.0f, 630.0f};

        V8Lab()
        {
            spec.defaults();
            spec.set("model", 1);
        }

        void start()
        {
            lab.call<void>("setup", 0.1016f, 8, 720.0f);
            lab.call<void>("reset");
            lab.call<void>("apply", spec.data(), 1);
            angles();
        }

        void angles()
        {
            for (int c = 0; c < 8; ++c)
                ang[static_cast<std::size_t>(c)] = std::fmod(theta + fire[c], 720.0f);
        }
    };
} // namespace

TEST_CASE("two stages: every cylinder changes with its valve shut, no lift jumps, once round the engine",
          "[valvetrain][stages]")
{
    V8Lab v;
    v.spec.set("stages", 1);
    v.spec.set("inPhaseRange", 40.0);     // and the intake's phaser moving meanwhile
    v.start();
    auto* areaIn = v.lab.fn<float(void*, int, float)>("areaIn");
    auto* areaEx = v.lab.fn<float(void*, int, float)>("areaEx");
    auto* control = v.lab.fn<void(void*, float, float, float, float, float, float, float*, float, float, float)>("control");
    // the most a table's area moves a degree: the step a sample may take at this speed
    auto slope = [&](const std::string& t) {
        double s = 0.0;
        const double k = v.lab.get(t + ".k");
        for (int i = 0; i < 128; ++i)
            s = std::max(s, std::fabs(v.lab.get(t + ".a[" + std::to_string(i + 1) + "]") -
                                      v.lab.get(t + ".a[" + std::to_string(i) + "]")) * k);
        return s;
    };
    const double slopeIn = std::max(slope("tab[0]"), slope("tab[1]"));
    const double slopeEx = std::max(slope("tab[2]"), slope("tab[3]"));
    const float dt = 1.0f / 96000.0f;
    const float redline = 680.0f;
    // up through the stage speed (576 rad/s) at 1500 rad/s^2, held, then down through it
    float omega = 450.0f;
    std::vector<float> lastIn(8, -1.0f);
    std::vector<float> lastEx(8, -1.0f);
    double worstIn = 0.0;
    double worstEx = 0.0;
    int switchedUpAt = -1;
    int allBAt = -1;
    float thetaAtWant = 0.0f;
    float turned = 0.0f;
    const int samples = static_cast<int>(0.5f / dt);
    for (int n = 0; n < samples; ++n)
    {
        const float t = static_cast<float>(n) * dt;
        omega = t < 0.25f ? 450.0f + 1500.0f * t : 825.0f - 1500.0f * (t - 0.25f);
        const float dDeg = omega * dt * 57.2957795f;
        v.theta = std::fmod(v.theta + dDeg, 720.0f);
        turned += dDeg;
        v.angles();
        if (n % 16 == 0)
        {
            control(v.lab.data(), 16.0f * dt, omega, redline, 0.9f, 400000.0f, 363.0f, v.ang.data(), -1.0f, -1.0f, -1.0f);
            if (switchedUpAt < 0 && v.lab.get("stageWant") == 1.0)
            {
                switchedUpAt = n;
                thetaAtWant = turned;
            }
            if (switchedUpAt >= 0 && allBAt < 0)
            {
                bool all = true;
                for (int c = 0; c < 8; ++c)
                    all = all && static_cast<int>(v.lab.get("useIn[" + std::to_string(c) + "]")) % 10 == 1 &&
                          static_cast<int>(v.lab.get("useEx[" + std::to_string(c) + "]")) % 10 == 3;
                if (all)
                    allBAt = n;
            }
        }
        // a phaser moves the events by at most its rate a sample: 200 deg/s
        const double phaseStep = 200.0 * 16.0 * dt;
        for (int c = 0; c < 8; ++c)
        {
            const float a = v.ang[static_cast<std::size_t>(c)];
            const float ai = areaIn(v.lab.data(), c, a);
            const float ae = areaEx(v.lab.data(), c, a);
            if (lastIn[static_cast<std::size_t>(c)] >= 0.0f)
            {
                worstIn = std::max(worstIn, std::fabs(ai - lastIn[static_cast<std::size_t>(c)]) /
                                                (slopeIn * (dDeg + phaseStep)));
                worstEx = std::max(worstEx, std::fabs(ae - lastEx[static_cast<std::size_t>(c)]) / (slopeEx * dDeg));
            }
            lastIn[static_cast<std::size_t>(c)] = ai;
            lastEx[static_cast<std::size_t>(c)] = ae;
        }
    }
    INFO("worst step over the most a table moves: intake " << worstIn << ", exhaust " << worstEx);
    REQUIRE(switchedUpAt >= 0);
    REQUIRE(allBAt >= 0);
    // every step a sample took is one the tables' own slopes allow: no jump at a change
    CHECK(worstIn <= 1.01);
    CHECK(worstEx <= 1.01);
    // the change went round the engine within a cycle and a tick
    float turnedThen = 0.0f;
    {
        // degrees turned between the want and the last cylinder's change, from the speed profile
        double deg = 0.0;
        for (int n = switchedUpAt; n < allBAt; ++n)
        {
            const float t = static_cast<float>(n) * dt;
            const float w = t < 0.25f ? 450.0f + 1500.0f * t : 825.0f - 1500.0f * (t - 0.25f);
            deg += w * dt * 57.2957795;
        }
        turnedThen = static_cast<float>(deg);
    }
    INFO("degrees from the want to the last cylinder's change: " << turnedThen << " (want at " << thetaAtWant << ")");
    CHECK(turnedThen <= 720.0f + 16.0f * 0.6f);
    // and back down: stage A again everywhere
    CHECK(v.lab.get("stageWant") == 0.0);
    for (int c = 0; c < 8; ++c)
        CHECK(static_cast<int>(v.lab.get("useIn[" + std::to_string(c) + "]")) % 10 == 0);
}

TEST_CASE("a cam edited while the engine runs: each cylinder takes the new one with its valve shut", "[valvetrain][stages]")
{
    V8Lab v;
    v.start();
    auto* areaIn = v.lab.fn<float(void*, int, float)>("areaIn");
    auto* apply = v.lab.fn<void(void*, void*, int)>("apply");
    auto* control = v.lab.fn<void(void*, float, float, float, float, float, float, float*, float, float, float)>("control");
    auto slope = [&](const std::string& t) {
        double s = 0.0;
        const double k = v.lab.get(t + ".k");
        for (int i = 0; i < 128; ++i)
            s = std::max(s, std::fabs(v.lab.get(t + ".a[" + std::to_string(i + 1) + "]") -
                                      v.lab.get(t + ".a[" + std::to_string(i) + "]")) * k);
        return s;
    };
    const float dt = 1.0f / 96000.0f;
    const float omega = 500.0f;
    std::vector<float> last(8, -1.0f);
    double worst = 0.0;
    double slopeMost = slope("tab[0]");
    for (int n = 0; n < 48000; ++n)
    {
        if (n == 4000)
        {
            v.spec.set("inLift", 0.013);        // a bigger cam, asked mid-run
            v.spec.set("inDurSeat", 262.0);
            v.spec.set("inDur1mm", 235.0);
            apply(v.lab.data(), v.spec.data(), 0);
        }
        const float dDeg = omega * dt * 57.2957795f;
        v.theta = std::fmod(v.theta + dDeg, 720.0f);
        v.angles();
        if (n % 16 == 0)
        {
            control(v.lab.data(), 16.0f * dt, omega, 680.0f, 0.9f, 300000.0f, 363.0f, v.ang.data(), -1.0f, -1.0f, -1.0f);
            if (n == 4016)
                slopeMost = std::max(slopeMost, slope("tab[10]"));
        }
        for (int c = 0; c < 8; ++c)
        {
            const float a = areaIn(v.lab.data(), c, v.ang[static_cast<std::size_t>(c)]);
            if (last[static_cast<std::size_t>(c)] >= 0.0f)
                worst = std::max(worst, std::fabs(a - last[static_cast<std::size_t>(c)]) / (slopeMost * dDeg));
            last[static_cast<std::size_t>(c)] = a;
        }
    }
    INFO("worst step over the most a table moves: " << worst);
    CHECK(worst <= 1.01);
    for (int c = 0; c < 8; ++c)
        CHECK(v.lab.get("useIn[" + std::to_string(c) + "]") == 10.0);     // the new copy
    CHECK(v.lab.get("tab[10].peak") > v.lab.get("tab[0].peak"));
    // the events the cylinder detects are the new cam's: 262 at the seat about the same centreline
    auto* ivo = v.lab.fn<float(void*, int)>("ivo");
    auto* ivc = v.lab.fn<float(void*, int)>("ivc");
    const double span = v.lab.get("tab[10].span");
    CHECK(std::fmod(ivc(v.lab.data(), 3) - ivo(v.lab.data(), 3) + 720.0, 720.0) == Catch::Approx(span).margin(1e-3));
    CHECK(span > 262.0);
}

TEST_CASE("deactivation: chosen cylinders rest in their power stroke, exhaust shut, and come back", "[valvetrain][deac]")
{
    V8Lab v;
    v.spec.set("deacMask", 0b10010110);     // four of eight
    v.start();
    auto* control = v.lab.fn<void(void*, float, float, float, float, float, float, float*, float, float, float)>("control");
    auto* areaIn = v.lab.fn<float(void*, int, float)>("areaIn");
    auto* areaEx = v.lab.fn<float(void*, int, float)>("areaEx");
    auto* active = v.lab.fn<bool(void*, int)>("active");
    const float dt = 1.0f / 96000.0f;
    const float omega = 200.0f;     // 1900 rpm, in the window
    std::vector<float> lastIn(8, -1.0f);
    std::vector<float> lastEx(8, -1.0f);
    bool jumped = false;
    auto runFor = [&](float seconds, float load) {
        const int samples = static_cast<int>(seconds / dt);
        for (int n = 0; n < samples; ++n)
        {
            v.theta = std::fmod(v.theta + omega * dt * 57.2957795f, 720.0f);
            v.angles();
            if (n % 16 == 0)
                control(v.lab.data(), 16.0f * dt, omega, 680.0f, load, 300000.0f, 363.0f, v.ang.data(), -1.0f, -1.0f, -1.0f);
            for (int c = 0; c < 8; ++c)
            {
                const float ai = areaIn(v.lab.data(), c, v.ang[static_cast<std::size_t>(c)]);
                const float ae = areaEx(v.lab.data(), c, v.ang[static_cast<std::size_t>(c)]);
                // a valve that rests or wakes does so shut: its area never jumps from or to an open one
                if (lastIn[static_cast<std::size_t>(c)] >= 0.0f && (ai == 0.0f) != (lastIn[static_cast<std::size_t>(c)] == 0.0f) &&
                    std::max(ai, lastIn[static_cast<std::size_t>(c)]) > 1.0e-5f)
                    jumped = true;
                if (lastEx[static_cast<std::size_t>(c)] >= 0.0f && (ae == 0.0f) != (lastEx[static_cast<std::size_t>(c)] == 0.0f) &&
                    std::max(ae, lastEx[static_cast<std::size_t>(c)]) > 1.0e-5f)
                    jumped = true;
                lastIn[static_cast<std::size_t>(c)] = ai;
                lastEx[static_cast<std::size_t>(c)] = ae;
            }
        }
    };
    runFor(0.1f, 0.2f);     // light load: in the window (a cycle at 1900 rpm is 63 ms)
    CHECK(v.lab.get("activeMask") == static_cast<double>(0xFF & ~0b10010110));
    for (int c = 0; c < 8; ++c)
        CHECK(active(v.lab.data(), c) == ((0b10010110 & (1 << c)) == 0));
    // resting: no valve of theirs opens for a whole cycle
    float most = 0.0f;
    {
        const int samples = static_cast<int>(0.07f / dt);
        for (int n = 0; n < samples; ++n)
        {
            v.theta = std::fmod(v.theta + omega * dt * 57.2957795f, 720.0f);
            v.angles();
            if (n % 16 == 0)
                control(v.lab.data(), 16.0f * dt, omega, 680.0f, 0.2f, 300000.0f, 363.0f, v.ang.data(), -1.0f, -1.0f, -1.0f);
            for (int c : {1, 2, 4, 7})
                most = std::max({most, areaIn(v.lab.data(), c, v.ang[static_cast<std::size_t>(c)]),
                                 areaEx(v.lab.data(), c, v.ang[static_cast<std::size_t>(c)])});
        }
    }
    CHECK(most == 0.0f);
    runFor(0.1f, 0.9f);     // the pedal down: all eight
    CHECK(v.lab.get("activeMask") == 255.0);
    CHECK_FALSE(jumped);
}

TEST_CASE("float: the speed rises with the spring, falls with the mass and with a sharper cam", "[valvetrain][float]")
{
    Obj flt(labProgram(), "ValveFloat");
    auto* speedFor = flt.fn<float(void*, void*, float, float, float, int)>("speedFor");
    auto cam = [&](float durSeat, float dur1mm, float lift) {
        auto c = std::make_unique<Obj>(labProgram(), "CamShape");
        c->call<void>("request", durSeat, dur1mm, lift, 470.0f, 0.5f);
        c->call<void>("build");
        return c;
    };
    // a small-block's street cam: 280 at the seat, 0.48 in lift; a spring of 300 lb/in on 120 lb at the seat; a
    // pushrod train with a 0.12 kg valve
    auto street = cam(280.0f, 245.0f, 0.0122f);
    const float rate = 52540.0f;
    const float preload = 534.0f;
    const float mass = static_cast<float>(flt.call<float>("massOf", 0.049f, 0.12f, 0));
    const float w = speedFor(flt.data(), street->data(), rate, preload, mass, 0);
    std::printf("[valvetrain] float: 280 deg cam, 0.48 in lift, 300 lb/in on 120 lb, pushrod (%.3f kg at the valve): "
                "%.0f rpm\n", mass, w * 9.5492966);
    CHECK(w > 0.0f);
    CHECK(speedFor(flt.data(), street->data(), 1.3f * rate, preload, mass, 0) > w);
    CHECK(speedFor(flt.data(), street->data(), rate, 1.3f * preload, mass, 0) > w);
    CHECK(speedFor(flt.data(), street->data(), rate, preload, 1.3f * mass, 0) < w);
    // the same lift over less duration (and shorter ramps) decelerates harder over the nose
    auto sharp = cam(250.0f, 222.0f, 0.0122f);
    CHECK(speedFor(flt.data(), sharp->data(), rate, preload, mass, 0) < w);
    // a lighter train (a bucket's) floats later; a pneumatic one never
    const float mBucket = flt.call<float>("massOf", 0.049f, 0.12f, 2);
    CHECK(speedFor(flt.data(), street->data(), rate, preload, mBucket, 2) > w);
    CHECK(speedFor(flt.data(), street->data(), rate, preload, mass, 4) == 0.0f);
    // the lab: past it the flag, a clatter, and a tail of area after the event
    V8Lab v;
    v.spec.set("springRate", rate);
    v.spec.set("springPreload", preload);
    v.spec.set("valveMass", 0.12);
    v.start();
    auto* control = v.lab.fn<void(void*, float, float, float, float, float, float, float*, float, float, float)>("control");
    for (int i = 0; i < 4; ++i)
        control(v.lab.data(), 16.0f / 96000.0f, 0.8f * v.lab.get("flt.omegaIn"), 900.0f, 1.0f, 400000.0f, 363.0f,
                v.ang.data(), -1.0f, -1.0f, -1.0f);
    const double wf = v.lab.get("flt.omegaIn");
    CHECK(wf > 0.0);
    CHECK(v.lab.get("floating") == 0.0);
    CHECK(v.lab.get("tab[0].tailDeg") == 0.0);
    control(v.lab.data(), 16.0f / 96000.0f, static_cast<float>(1.2 * wf), 900.0f, 1.0f, 400000.0f, 363.0f, v.ang.data(),
            -1.0f, -1.0f, -1.0f);
    CHECK(v.lab.get("floating") == 1.0);
    CHECK(v.lab.get("clatter") > 0.0);
    CHECK(v.lab.get("tab[0].tailDeg") > 10.0);
    // the tail: area after the event's end, and past the tail only the seat's leak (a few percent of the most)
    auto* at = v.lab.fn<float(void*, int, float)>("areaIn");
    const double end = v.lab.get("tab[0].openNow") + v.lab.get("tab[0].span");
    CHECK(at(v.lab.data(), 0, static_cast<float>(std::fmod(end + 5.0, 720.0))) > 0.0f);
    const double leak = v.lab.get("tab[0].leak");
    CHECK(leak > 0.0);
    CHECK(leak <= 0.03 * v.lab.get("tab[0].peak") + 1e-12);
    CHECK(at(v.lab.data(), 0, static_cast<float>(std::fmod(end + 105.0, 720.0))) == Catch::Approx(leak));
}

namespace
{
    // The lab's per-sample work for a V8 (each cylinder's two areas, the control every 16 samples), in a loop of the
    // program's own, beside the reference's (cylinder.fm, areaAt on its table): what the lab adds to a sample. Its
    // run is [[clang::nonblocking]], so FMinus proves the lab's per-sample and control path allocate nothing.
    const char* kBench = R"(
#include "core/cylinder.fm"
module LabBench
{
    ValveLab lab;
    Cylinder cy[8];
    float ang[16];

    float run(int n, float omega) [[clang::nonblocking]]
    {
        float theta = 0.0f;
        float acc = 0.0f;
        const float dDeg = omega / 96000.0f * 57.2957795f;
        for (int i = 0; i < n; ++i)
        {
            theta += dDeg;
            if (theta >= 720.0f)
                theta -= 720.0f;
            for (int c = 0; c < 8; ++c)
            {
                float a = theta + 90.0f * (float)c;
                if (a >= 720.0f)
                    a -= 720.0f;
                ang[c] = a;
                acc += lab.areaIn(c, a) + lab.areaEx(c, a);
            }
            if ((i & 15) == 0)
                lab.control(16.0f / 96000.0f, omega, 680.0f, 0.5f, 300000.0f, 363.0f, ang, -1.0f, -1.0f, -1.0f);
        }
        return acc;
    }

    float runRef(int n, float omega) [[clang::nonblocking]]
    {
        float theta = 0.0f;
        float acc = 0.0f;
        const float dDeg = omega / 96000.0f * 57.2957795f;
        for (int i = 0; i < n; ++i)
        {
            theta += dDeg;
            if (theta >= 720.0f)
                theta -= 720.0f;
            for (int c = 0; c < 8; ++c)
            {
                float a = theta + 90.0f * (float)c;
                if (a >= 720.0f)
                    a -= 720.0f;
                ang[c] = a;
                acc += cy[c].aI * cy[c].areaAt(a, cy[c].ivo, cy[c].idur) + cy[c].aE * cy[c].areaAt(a, cy[c].evo, cy[c].edur);
            }
        }
        return acc;
    }
};
)";
} // namespace

TEST_CASE("cost: what the lab adds to a sample of a V8, against the reference's areas", "[valvetrain][cost]")
{
    tee::Program& prog = tee::test::lab::program("valvetrain/lab.fm", kBench);
    Obj spec(prog, "ValveSpec");
    spec.defaults();
    spec.set("inPhaseRange", 40.0);
    spec.set("exPhaseRange", 30.0);
    spec.set("stages", 1);
    Obj bench(prog, "LabBench");
    for (int c = 0; c < 8; ++c)
    {
        const std::string cy = "cy[" + std::to_string(c) + "].";
        bench.set(cy + "cyc", 720.0);
        bench.set(cy + "ivo", 359.0);
        bench.set(cy + "idur", 250.0);
        bench.set(cy + "evo", 109.0);
        bench.set(cy + "edur", 250.0);
        bench.set(cy + "aI", 0.001);
        bench.set(cy + "aE", 0.001);
    }
    std::string errors;
    auto* labSetup = prog.function<void(void*, float, int, float)>("ValveLab::setup", errors);
    auto* labReset = prog.function<void(void*)>("ValveLab::reset", errors);
    auto* labApply = prog.function<void(void*, void*, int)>("ValveLab::apply", errors);
    REQUIRE(labApply != nullptr);
    tee::Schema::Handle h;
    REQUIRE(bench.schema->find("lab", h, errors));
    void* lab = static_cast<unsigned char*>(bench.data()) + h.offset;
    labSetup(lab, 0.1016f, 8, 720.0f);
    labReset(lab);
    labApply(lab, spec.data(), 1);
    auto* run = bench.fn<float(void*, int, float)>("run");
    auto* runRef = bench.fn<float(void*, int, float)>("runRef");
    const int n = 96000;
    auto time = [&](auto* f) {
        double best = 1e30;
        for (int r = 0; r < 5; ++r)
        {
            const auto t0 = std::chrono::steady_clock::now();
            volatile float sink = f(bench.data(), n, 600.0f);
            (void)sink;
            const auto t1 = std::chrono::steady_clock::now();
            best = std::min(best, std::chrono::duration<double, std::nano>(t1 - t0).count() / n);
        }
        return best;
    };
    const double lab8 = time(run);
    const double ref8 = time(runRef);
    std::printf("[valvetrain] cost a 96 kHz sample, eight cylinders' areas: the lab %.1f ns, the reference's %.1f ns: "
                "%.1f ns more\n", lab8, ref8, lab8 - ref8);
    // card E's share is 0.05 R; R is some 3400 ns here (tee-bench, the V8): 170 ns. A loose bound, the machine shared
    CHECK(lab8 - ref8 < 170.0);
}

TEST_CASE("the cam presets: every field is the lab's, in its range, and every cam they ask for is made", "[valvetrain][content]")
{
    namespace fs = std::filesystem;
    const fs::path dir = fs::path(TEE_SOURCE_DIR) / "content" / "builds" / "cams";
    int files = 0;
    for (const auto& entry : fs::directory_iterator(dir))
    {
        if (entry.path().extension() != ".json")
            continue;
        files += 1;
        std::ifstream in(entry.path());
        const std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        const auto at = text.find("\"valve\"");
        REQUIRE(at != std::string::npos);
        const std::string body = text.substr(at, text.find('}', at) - at);
        V8Lab v;
        const std::regex pair("\"(\\w+)\"\\s*:\\s*(\"[^\"]*\"|[-0-9.eE]+)");
        for (auto it = std::sregex_iterator(body.begin(), body.end(), pair); it != std::sregex_iterator(); ++it)
        {
            const std::string key = (*it)[1];
            std::string value = (*it)[2];
            INFO(entry.path().filename().string() << ": " << key << " = " << value);
            tee::Schema::Handle h;
            std::string error;
            REQUIRE(v.spec.schema->find(key, h, error));
            double x = 0.0;
            if (value.front() == '"')
            {
                value = value.substr(1, value.size() - 2);
                const auto* choices = h.node->attribute("ui::choices");
                REQUIRE(choices != nullptr);
                const auto* range = h.node->attribute("fminus::range");
                int found = -1;
                for (std::size_t i = 0; i < choices->arguments.size(); ++i)
                    if (choices->arguments[i].text == value)
                        found = static_cast<int>(i);
                REQUIRE(found >= 0);
                x = range->arguments[0].number + found;
            }
            else
                x = std::stod(value);
            const auto* range = h.node->attribute("fminus::range");
            REQUIRE(range != nullptr);
            CHECK(x >= range->arguments[0].number);
            CHECK(x <= range->arguments[1].number);
            v.spec.set(key, x);
        }
        v.start();
        INFO(entry.path().filename().string());
        CHECK(v.lab.get("cam[0].reached") == 1.0);
        CHECK(v.lab.get("cam[2].reached") == 1.0);
        if (v.spec.get("stages") == 1.0)
        {
            CHECK(v.lab.get("cam[1].reached") == 1.0);
            CHECK(v.lab.get("cam[3].reached") == 1.0);
        }
    }
    CHECK(files >= 10);
}
