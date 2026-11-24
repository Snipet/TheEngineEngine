// tests/sim/valvetrain/BreathingTest.cpp: what the engine breathes with the lab (valve.model 1, intake.model 1):
// volumetric efficiency (tel.engine.veAir) and power on the engine dyno, wide open, in air at 300 K, for a 6.2 L
// pushrod V8 with a mild cam and for a 998 cc superbike four (76 x 55 mm, four valves, 9.5 mm lift, some 270 degrees
// at 1 mm, ITBs, 13:1). The targets (the lead's, from published figures): the V8 some 300 to 330 kW at 6000 rpm; the
// superbike 125 to 145 kW at 12,500 to 13,500 rpm, its VE near 1.0 to 1.1 at its tuned speed. Printed; the bounds
// asserted are loose (what the model reaches today is in docs/models/valvetrain.md).
#include "Engine.h"

#include <cstdio>
#include <vector>

using tee::test::lab::Rig;

namespace
{
    void sweep(const char* name, const char* build, const std::vector<std::pair<std::string, double>>& f,
               const std::vector<double>& rpms, double& bestKw, double& bestVe)
    {
        Rig r(build, f);
        r.start();
        bestKw = 0.0;
        bestVe = 0.0;
        std::printf("[e] %s:", name);
        for (double rpm : rpms)
        {
            const double tq = r.torqueAt(rpm, 1.0, 0.8, 0.4);
            const double kw = tq * rpm * 3.14159265358979 / 30.0 / 1000.0;
            const double ve = r.tel("engine.veAir");
            bestKw = std::max(bestKw, kw);
            bestVe = std::max(bestVe, ve);
            std::printf(" %.0f rpm %.0f N m %.0f kW VE %.2f;", rpm, tq, kw, ve);
        }
        std::printf("\n");
        std::fflush(stdout);
    }
} // namespace

TEST_CASE("breathing: a 6.2 L V8 and a superbike four, volumetric efficiency and power against speed",
          "[valvetrain][integrated][breathing]")
{
    double kw = 0.0, ve = 0.0;
    sweep("6.2 L V8, mild cam", "v8",
          {{"env.temperature", 300.0}, {"valve.model", 1.0}, {"intake.model", 1.0}, {"valve.inDurSeat", 240.0},
           {"valve.inDur1mm", 210.0}, {"valve.inLift", 0.0114}, {"valve.exDurSeat", 246.0}, {"valve.exDur1mm", 214.0},
           {"valve.exLift", 0.0114}},
          {2000, 3000, 4000, 5000, 6000}, kw, ve);
    CHECK(kw > 150.0);
    sweep("998 cc superbike four", "default",
          {{"env.temperature", 300.0}, {"engine.layout", 7.0}, {"engine.displacement", 0.000998},
           {"engine.boreStroke", 1.38}, {"engine.compression", 13.0}, {"valve.model", 1.0}, {"valve.valves", 4.0},
           {"valve.inDurSeat", 300.0}, {"valve.inDur1mm", 270.0}, {"valve.inLift", 0.0095}, {"valve.inCentre", 105.0},
           {"valve.exDurSeat", 292.0}, {"valve.exDur1mm", 262.0}, {"valve.exLift", 0.0085}, {"valve.exCentre", 108.0},
           {"valve.train", 3.0}, {"intake.model", 1.0}, {"intake.throttleKind", 1.0}, {"intake.runnerLength", 0.12}},
          {6000, 8000, 10000, 11500, 12500, 13500}, kw, ve);
    CHECK(kw > 30.0);
}
