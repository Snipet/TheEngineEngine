// tests/sim/diesel/TurboTest.cpp: card D2: turbos in truck sizes (card F's machine, boost.model 1) on the 15 L six: a big
// single's lag counted, compound turbos, variable vanes; the pedal floored at 1300 rpm on the engine dyno (300 K).
#include "Diesel.h"

#include <cstdio>
#include <vector>

using d2::Engine;

namespace
{
    struct Spool
    {
        double t90;      // seconds to 90 % of the boost at 8 s
        double boost;    // Pa, at 8 s
        double torque;   // N m at 8 s
    };

    Spool spool(int layout, int vanes, double rpm = 1300.0)
    {
        Engine e("truck-15l-six");
        e.spec("boost.model", 1);
        e.spec("boost.kind", 1);
        e.spec("boost.layout", layout);
        e.spec("boost.vanes", vanes);
        e.spec("boost.gateSpring", 220000.0);
        e.spec("boost.icFlow", 0.8);
        e.spec("boost.chargeVolume", 0.02);
        e.start();
        e.dyno(rpm, 0.0f);
        REQUIRE(e.run(4.0));
        e.ctl().throttle = 1.0f;
        std::vector<double> b;
        for (int i = 0; i < 160; ++i)
        {
            REQUIRE(e.run(0.05));
            b.push_back(e.tel("boost.pressure"));
        }
        Spool s{};
        s.boost = b.back();
        s.torque = e.mean("engine.torque", 0.3);
        s.t90 = -1.0;
        for (size_t i = 0; i < b.size(); ++i)
            if (b[i] >= 0.9 * s.boost)
            {
                s.t90 = 0.05 * static_cast<double>(i + 1);
                break;
            }
        std::printf("%s%s at %.0f rpm: 90 %% of %.0f kPa in %.2f s, %.0f N m\n", layout == 1 ? "big single" : (layout == 4 ? "compound" : "?"),
                    vanes ? " with variable vanes" : "", rpm, s.boost / 1000.0, s.t90, s.torque);
        return s;
    }
} // namespace

TEST_CASE("diesel turbo: a big single's lag you can count, variable vanes quicker", "[diesel][turbo]")
{
    const Spool single = spool(1, 0);
    const Spool vgt = spool(1, 1);
    CHECK(single.t90 > 0.3);
    CHECK(vgt.t90 < single.t90);
    // (the boost each reaches moved with card F's controller: since N2's back-pressure limit, spec.boost.backPressure 1.4,
    // the vanes open at 1300 rpm and the VGT holds less than the big single: printed, not asserted)
}

TEST_CASE("diesel turbo, full: a big single's lag you can count, compound and variable vanes", "[diesel][turbo][nightly]")
{
    const Spool single = spool(1, 0);
    const Spool vgt = spool(1, 1);
    const Spool compound = spool(4, 0);
    CHECK(single.t90 > 0.3);
    CHECK(single.boost > 30000.0);
    // (measured: the big single 59 kPa after 2.05 s at 1300 rpm, its turbine too big to drive there; the vanes shut
    // 192 kPa in 0.65 s; compound 202 kPa in 1.9 s. The turbines' back pressure does not reach the cylinders yet: card
    // F's exhaust stand-in until card G's network feeds the housings)
    CHECK(vgt.t90 < single.t90);
    CHECK(compound.boost > 30000.0);
}
