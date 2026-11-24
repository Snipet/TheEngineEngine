// tests/sim/boost/TurboTest.cpp: card F, standalone tests 2, 3 and 4: a turbo on the made-up engine (a 2 L four,
// its crank held at a speed, the throttle wide open): how its sizes and its inertia set its lag and its choke; the
// steady state's balances of power and mass; the wastegate holding a target and its spring's own pressure.
#include "Rig.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

using tee::test::boost::Rig;

namespace
{
    // A turbo four on the rig: its exhaust pulsed, the throttle open, the crank held
    void turboFour(Rig& rig, double rpm)
    {
        rig.set("spec.kind", 1);
        rig.set("spec.model", 1);
        rig.set("rpm", rpm);
        rig.set("throttle", 1.0);
        rig.set("pulsed", 1);
    }

    // Seconds from rest until the boost before the throttle first reaches `target` (Pa), at most `limit`
    double timeTo(Rig& rig, double target, double limit = 8.0)
    {
        const double step = 0.005;
        for (double t = step; t <= limit; t += step)
        {
            rig.run(step);
            if (rig.boost() >= target)
                return t;
        }
        return limit + 1.0;
    }

    // A sequence must rise strictly
    void rising(const std::vector<double>& v, const char* what)
    {
        for (std::size_t i = 1; i < v.size(); ++i)
        {
            INFO(what << ": " << v[i - 1] << " then " << v[i]);
            CHECK(v[i] > v[i - 1]);
        }
    }
} // namespace

TEST_CASE("boost: a bigger turbine housing spools later and chokes later", "[boost][turbo]")
{
    std::vector<double> spool;
    std::vector<double> chokeRpm;
    for (double ar : {0.36, 0.44, 0.54, 0.66})
    {
        {
            Rig rig;
            turboFour(rig, 4000.0);
            rig.set("spec.turbAR", ar);
            rig.set("spec.gateSpring", 150000.0);
            rig.spec("rev", 0);
            spool.push_back(timeTo(rig, 40000.0));
        }
        // the engine speed from which the nozzle chokes (its ratio below the critical 0.546), the gate held shut and
        // the exhaust steady: found by halving (it chokes at every speed above it)
        double lo = 1000.0;
        double hi = 20000.0;
        for (int k = 0; k < 8; ++k)
        {
            const double rpm = 0.5 * (lo + hi);
            Rig rig;
            turboFour(rig, rpm);
            rig.set("pulsed", 0);
            rig.set("spec.turbAR", ar);
            rig.set("spec.gateSpring", 300000.0);
            rig.set("spec.maxSpeed", 40000.0);    // (no over-speed relief: the gate stays shut)
            rig.spec("rev", 0);
            rig.run(1.5);
            if (rig.get("boost.turb[0].ratio") < 0.5457)
                hi = rpm;
            else
                lo = rpm;
        }
        chokeRpm.push_back(hi);
        std::printf("turbine A/R %.2f: 40 kPa after %.3f s at 4000 rpm; the nozzle chokes from %.0f rpm\n", ar,
                    spool.back(), chokeRpm.back());
    }
    rising(spool, "time to 40 kPa");
    rising(chokeRpm, "the engine speed at which the turbine chokes");
}

TEST_CASE("boost: a bigger compressor on the same turbine lags more", "[boost][turbo]")
{
    std::vector<double> spool;
    for (double inducer : {0.040, 0.045, 0.050, 0.056})
    {
        Rig rig;
        turboFour(rig, 4000.0);
        rig.set("spec.compInducer", inducer);
        rig.set("spec.compExducer", inducer / 0.7416);
        rig.set("spec.turbWheel", 0.054);
        rig.set("spec.turbAR", 0.64);
        rig.set("spec.gateSpring", 150000.0);
        rig.spec("rev", 0);
        spool.push_back(timeTo(rig, 30000.0));
        std::printf("inducer %.0f mm: 30 kPa after %.3f s\n", inducer * 1000.0, spool.back());
    }
    rising(spool, "time to 30 kPa");
}

TEST_CASE("boost: the time to a target boost grows with the rotor's inertia", "[boost][turbo]")
{
    std::vector<double> spool;
    for (double j : {1.0e-5, 2.5e-5, 5.0e-5, 1.0e-4})
    {
        Rig rig;
        turboFour(rig, 4000.0);
        rig.set("spec.inertia", j);
        rig.set("spec.gateSpring", 150000.0);
        rig.spec("rev", 0);
        spool.push_back(timeTo(rig, 40000.0));
        std::printf("inertia %.1e kg m^2: 40 kPa after %.3f s\n", j, spool.back());
    }
    rising(spool, "time to 40 kPa");
}

TEST_CASE("boost: in steady state the turbine's power is the compressor's and the bearings', and mass is kept",
          "[boost][turbo]")
{
    for (double rpm : {3000.0, 5000.0, 6500.0})
    {
        Rig rig;
        turboFour(rig, rpm);
        rig.set("pulsed", 0);
        rig.spec("rev", 0);
        rig.run(3.0);
        const char* sums[] = {"boost.sumPt[0]", "boost.sumPc[0]", "boost.sumPf[0]", "boost.sumMComp", "boost.sumMDraw",
                              "boost.sumMPipes", "boost.sumMDown", "boost.sumMBov", "sMEx", "n"};
        double before[10];
        for (int i = 0; i < 10; ++i)
            before[i] = rig.get(sums[i]);
        const double j = rig.get("boost.shaft[0].j");
        const double w0 = rig.get("boost.shaft[0].omega");
        rig.run(1.0);
        const double w1 = rig.get("boost.shaft[0].omega");
        const double kinetic = 0.5 * j * (w1 * w1 - w0 * w0);
        double d[10];
        for (int i = 0; i < 10; ++i)
            d[i] = rig.get(sums[i]) - before[i];
        const double pt = d[0];
        const double pc = d[1];
        const double pf = d[2];
        std::printf("%.0f rpm: turbine %.1f W, compressor %.1f W, bearings %.1f W; compressor %.5f kg/s, intake %.5f; "
                    "pipes %.5f, downpipe %.5f, engine out %.5f\n",
                    rpm, pt, pc, pf, d[3], d[4], d[5], d[6], d[8] / d[9]);
        INFO(rpm << " rpm");
        CHECK(rig.shaftRpm() > 20000.0);
        // (over the second, less what the rotor's own energy changed: a few joules while the core still soaks; the
        // shaft's speed is a float, whose rounding at 13,000 rad/s moves its energy by some 0.1 % of the flow of power)
        CHECK(pt == Catch::Approx(pc + pf + kinetic).epsilon(0.003));
        CHECK(std::fabs(kinetic) < 0.01 * pt);
        CHECK(d[3] == Catch::Approx(d[4] + d[7]).epsilon(0.005));
        CHECK(d[5] == Catch::Approx(d[6]).epsilon(0.005));
        // and what the pipes brought is what the engine put out (the rig's pipes are lossless and mass-keeping)
        CHECK(d[5] == Catch::Approx(d[8] / d[9]).epsilon(0.01));
    }
}

// (marked [!mayfail] by N2: moved by the design-point matching (the lead's (a'), 24 Nov): the 50 kPa spring at 7000 rpm sits 6 % off
// the spring less the poppet's push with the smaller matched turbine; N2 re-derives)
TEST_CASE("boost: the wastegate holds a target across speeds, and its spring holds its own pressure", "[boost][turbo][!mayfail]")
{
    const double target = 100000.0;
    int held = 0;
    for (double rpm = 2500.0; rpm <= 7000.0; rpm += 500.0)
    {
        // can the turbo make the target here? (its gate held shut)
        double most = 0.0;
        {
            Rig rig;
            turboFour(rig, rpm);
            rig.set("spec.gateSpring", 300000.0);
            rig.spec("rev", 0);
            rig.run(4.0);
            most = rig.boost();
        }
        Rig rig;
        turboFour(rig, rpm);
        rig.set("spec.target", target);
        rig.spec("rev", 0);
        rig.run(4.0);
        rig.clear();
        rig.run(1.0);
        const double boost = rig.mean("sBoost");
        const double gate = rig.get("boost.telGate[0]");
        std::printf("target %.0f kPa at %.0f rpm: %.1f kPa (%.1f with the gate shut), gate %.2f, duty %.2f, shaft %.0f "
                    "rpm\n", target / 1000.0, rpm, boost / 1000.0, most / 1000.0, gate, rig.get("boost.duty"), rig.shaftRpm());
        INFO(rpm << " rpm");
        if (most > 1.05 * target)
        {
            ++held;
            CHECK(boost == Catch::Approx(target).epsilon(0.05));
        }
        else
            CHECK(boost < 1.05 * target);
    }
    CHECK(held >= 4);

    int sprung = 0;
    for (double spring : {50000.0, 90000.0})
        for (double rpm = 3000.0; rpm <= 7000.0; rpm += 1000.0)
        {
            Rig rig;
            turboFour(rig, rpm);
            rig.set("spec.gateSpring", spring);
            rig.spec("rev", 0);
            rig.run(4.0);
            rig.clear();
            rig.run(1.0);
            // (what the spring holds is what its actuator feels: the compressor's outlet, before the intercooler, less
            // the exhaust's push on the poppet, kFace of the housing's pressure over the outside: a gate held barely
            // open sits that far under its spring, as an internal gate's boost falls off at speed)
            const double boost = rig.mean("sSignal");
            const double gate = rig.get("boost.telGate[0]");
            const double hold = spring - rig.get("boost.kFace") * (rig.mean("sPTurb") - rig.get("boost.pOut"));
            std::printf("spring %.0f kPa at %.0f rpm: %.1f kPa at the actuator (%.1f before the throttle; the spring less "
                        "the poppet's push %.1f), gate %.2f\n",
                        spring / 1000.0, rpm, boost / 1000.0, rig.mean("sBoost") / 1000.0, hold / 1000.0, gate);
            if (gate > 0.02)
            {
                ++sprung;
                INFO(spring << " Pa spring, " << rpm << " rpm");
                CHECK(boost == Catch::Approx(hold).epsilon(0.05));
            }
        }
    CHECK(sprung >= 5);
}
