// tests/sim/core/CostTest.cpp: what the mechanics add to a sample (card D's budget: 0.06 R for eight cylinders, the
// inertia torque and the work integral; R the reference V8's cost, tee-bench's). Reported, not asserted: a shared
// machine's timing is not a test. The engine's own share is measured again once the mechanics are in it.
#include "Presets.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdio>

using namespace tee::test::core;

TEST_CASE("the mechanics' cost a sample, eight and sixteen cylinders", "[core][cost]")
{
    for (const char* name : {"ref-12-v8-cross", "v16-45"})
    {
        Rig rig;
        applyLayout(rig, loadPreset(name));
        rig.build(0.1, 0.09, 0.15, 10.0, 0.2);
        auto run = rig.method<void(void*, int, float)>("costRun");
        run(rig.data(), 96000, float(1.0 / 96000.0));
        double best = 1e30;
        for (int k = 0; k < 5; ++k)
        {
            const auto t0 = std::chrono::steady_clock::now();
            run(rig.data(), 96000, float(1.0 / 96000.0));
            const double ns = std::chrono::duration<double, std::nano>(std::chrono::steady_clock::now() - t0).count() / 96000.0;
            best = std::min(best, ns);
        }
        std::printf("%s: the inertia torque, the crank's step and the work integral, %.1f ns a 96 kHz sample\n", name, best);
        CHECK(rig.get("omega") > 0.0);
    }
}
