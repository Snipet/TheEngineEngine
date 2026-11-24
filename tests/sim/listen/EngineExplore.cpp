// EngineExplore.cpp: the whole simulator with model 1, for the level's fit (hidden: "[.explore]").
#include "sim/core/Engine.h"

#include <cmath>
#include <cstdio>

using tee::test::core::Bench;

TEST_CASE("explore: each build's raw exhaust at full throttle on the dyno", "[.explore][engine]")
{
    const char* builds[] = {"engines/ref-00-single", "engines/ref-01-twin-360", "engines/ref-02-twin-270",
                            "engines/ref-03-vtwin-45", "engines/ref-04-inline-3", "engines/ref-05-inline-4",
                            "engines/ref-06-flat-4", "engines/ref-07-xplane-i4", "engines/ref-08-inline-5",
                            "engines/ref-09-v6-60", "engines/ref-10-inline-6", "engines/ref-11-flat-6",
                            "engines/ref-12-v8-cross", "engines/ref-13-v8-flat", "engines/ref-14-v10-72",
                            "engines/ref-15-v12-60", "engines/ref-16-rotary-2", "v8", "v10", "large-diesel", "default"};
    for (const char* b : builds)
    {
        Bench bench(b);
        bench.write("spec.sound.model", 1);
        bench.write("spec.sound.rev", bench.field("spec.sound.rev") + 1);
        bench.start(2.0);
        const double red = bench.tel("engine.redline");
        const double idle = bench.tel("engine.idle");
        double lev[3];
        double mix[3];
        const double fr[3] = {0.0, 0.4, 0.75};
        for (int k = 0; k < 3; ++k)
        {
            if (k == 0)
            {
                bench.ctl().mode = 0;
                bench.ctl().throttle = 0.0f;
            }
            else
            {
                bench.ctl().mode = 2;
                bench.ctl().dynoSpeed = float(fr[k] * red);
                bench.ctl().throttle = 1.0f;
            }
            bench.play(1.5);
            lev[k] = bench.tel("sound.exhaustLevel");
            mix[k] = bench.tel("sound.mixLevel");
        }
        std::printf("neff %2.0f loud %6.1f dB ", bench.field("listen.nEff"), bench.tel("sound.loudness"));
        std::printf("%-26s n %2.0f vd %.5f idle %5.0f red %5.0f rpm exTrim %.3f norm %.3f | ex idle %6.1f 0.4 %6.1f 0.75 %6.1f | mix %6.1f %6.1f %6.1f\n",
                    b, bench.tel("engine.cylinders"), bench.field("vTotal"), idle * 9.5493, red * 9.5493,
                    bench.field("exTrim"), bench.field("gainNorm"), lev[0], lev[1], lev[2], mix[0], mix[1], mix[2]);
    }
}
