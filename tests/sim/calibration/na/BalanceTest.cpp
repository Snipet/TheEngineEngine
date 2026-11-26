// tests/sim/calibration/na/BalanceTest.cpp: card N1's hold on the air's account. In a steady state the fresh air the
// cylinders trap is the air the throttle passes: nothing else feeds them. Measured at a warm idle on the bench, where
// the overlap pushes the most gas back up the runners (docs/measurements/calibration/na/causes.md, D).
#include "Player.h"
#include "tee/EnginePlayer.h"
#include "tee/Warm.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <filesystem>
#include <string>

// (a section of the fast tier's one case, FastTest.cpp: "the fresh air trapped at a warm idle is the air through the
// throttle")
void n1::freshAirIsThrottleAir()
{
    // (finding D of causes.md: gas pushed back into the runner left at the cylinder's temperature and came back at the
    // air's: 1.58x on this build before card D's conserving cylinder, 1.05 after)
    struct
    {
        double sampleRate = 96000.0;
    } options;
    std::string errors, warnings;
    tee::EnginePlayer* p = &n1::player(options.sampleRate);
    tee::EngineBuild b;
    REQUIRE(tee::loadBuild(*p, std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "archetypes" / "economy-four-15.json", b,
                           errors, warnings));
    tee::applyBuild(*p, b);
    const auto get = [&](const std::string& path) {
        double v = 0.0;
        std::string e;
        p->read(path, v, e);
        return v;
    };
    float l = 0.0f, r = 0.0f;
    double thr = 0.0, trap = 0.0;
    const int cyls = 4;
    const long n = long(5.0 * options.sampleRate);
    auto& c = p->controls();
    c.ignition = 2;
    for (long i = 0; i < n; ++i)
    {
        if ((i & 63) == 0)
            for (const tee::HeldValue& h : tee::warmEngine())
                p->instance().write(p->schema(), h.path, h.value, errors);
        if (i == long(options.sampleRate))
            c.ignition = 1;
        p->process(&l, &r, 1);
        if (i < long(3.0 * options.sampleRate))
            continue;
        thr += get("intk.qThr") * get("intk.pBox") / (287.0 * get("intk.tAir")) / options.sampleRate;
        for (int k = 0; k < cyls; ++k)
            if (get("cyl[" + std::to_string(k) + "].ivcNow") != 0.0)
                trap += get("cyl[" + std::to_string(k) + "].m") - get("cyl[" + std::to_string(k) + "].mInert");
    }
    std::printf("economy four, warm idle: %.2f g/s through the throttle, %.2f g/s trapped fresh (%.3f)\n", thr / 2.0 * 1e3,
                trap / 2.0 * 1e3, trap / thr);
    CHECK(trap / thr < 1.15);
    CHECK(trap / thr > 0.85);
}
