// tests/sim/combo/SoakTest.cpp: package X's long soak of the full combination: every switch on, the typical build
// (turbo V8, dual exhaust), random driving for TEE_SOAK_SECONDS (600 by default: ten minutes) under ctest's nightly
// label (ctest --preset nightly); an hour by hand: TEE_SOAK_SECONDS=3600 build-dev/bin/tee-tests-combo "[soak]". Judged as a
// fuzz run is (finite, no guard, no blast, not silent while running), and the engine must still run at the end.
#include "Combo.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <random>

using namespace combo;

TEST_CASE("nightly soak: every switch on and the turbo V8 driven at random for ten minutes", "[nightly][soak]")
{
    double seconds = 600.0;
    if (const char* s = std::getenv("TEE_SOAK_SECONDS"))
        seconds = std::atof(s);
    std::mt19937_64 g(20261124);
    auto u = [&] { return std::uniform_real_distribution<double>(0.0, 1.0)(g); };
    Times tm;
    tm.idleFrom = -1.0;
    tm.end = seconds;
    tee::test::Scenario sc(buildPath("turbo-v8-dual"));
    sc.at(0.0).spec("env.temperature", 293.15).control("mode", 1).lever(0).clutch(0).ignition(tee::On);
    for (const Switch& w : switches())
        sc.spec(w.path, w.on);
    sc.at(tm.start).ignition(tee::Start);
    sc.at(tm.started).ignition(tee::On);
    double t = 3.0;
    while (t < seconds - 2.0)
    {
        // a stretch of driving: pull away, up through some gears, lift, brake to a stop, back to neutral
        sc.at(t).shiftUp();
        sc.at(t + 0.3).throttle(0.3 + 0.7 * u()).ramp(0.2);
        double s = t + 1.5;
        const int gears = 1 + static_cast<int>(u() * 4.0);
        for (int k = 0; k < gears && s < seconds - 4.0; ++k)
        {
            s += 1.0 + 3.0 * u();
            sc.at(s).shiftUp();
            sc.at(s + 0.4).throttle(u()).ramp(0.1);
        }
        sc.at(s + 1.0).throttle(0.0).ramp(0.05);
        sc.at(s + 1.2).brake(0.3 + 0.5 * u()).ramp(0.2);
        for (int k = 0; k < gears; ++k)
            sc.at(s + 1.5 + 0.6 * k).shiftDown();
        s += 2.0 + 0.6 * gears;
        sc.at(s).brake(0.0);
        sc.at(s).lever(0);
        // a free rev or two in neutral now and then
        if (u() < 0.5)
            sc.at(s + 0.5).throttle(1.0).ramp(0.1).at(s + 1.5).throttle(0.0).ramp(0.05);
        t = s + 3.0;
    }
    const tee::test::Run r = sc.run(tm.end);
    Verdict v = judgeFuzz(r, tm);
    if (r.tel("engine.state").last() != 2.0)
        v.failures.push_back({"stall", "not running at the end of the soak"});
    std::printf("soak: %.0f s simulated in %.0f s of wall time; %zu failures\n", r.result().simulatedSeconds,
                r.result().wallSeconds, v.failures.size());
    report(v, "all", "turbo-v8-dual", "soak");
}
