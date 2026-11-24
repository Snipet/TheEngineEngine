// tests/sim/core/ShakeTest.cpp: card D's test 5, shaking forces and rocking moments by order; balance shafts.
#include "Presets.h"

static constexpr double kPi = 3.14159265358979323846;

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdio>
#include <string>

using namespace tee::test::core;

namespace
{
    struct Shake
    {
        double f1, f2, m1, m2, f1raw, f2raw;
    };

    Shake shake(const std::string& preset, bool b1 = false, bool b2 = false)
    {
        Rig rig;
        applyLayout(rig, loadPreset(preset));
        rig.set("spec.pistonMass", 0.5);
        rig.set("spec.rodMass", 0.5);
        rig.set("spec.balance1", b1 ? 1 : 0);
        rig.set("spec.balance2", b2 ? 1 : 0);
        rig.build(0.09, 0.09, 0.15, 10.0, 0.15);
        rig.method<void(void*, float, float)>("shakeAt")(rig.data(), 0.3f, float(6000.0 * kPi / 30.0));
        Shake s{rig.get("sh.force1"), rig.get("sh.force2"), rig.get("sh.moment1"), rig.get("sh.moment2"),
                rig.get("sh.force1Raw"), rig.get("sh.force2Raw")};
        std::printf("%-18s at 6000 rpm: force 1st %8.1f N, 2nd %8.1f N; moment 1st %7.1f N m, 2nd %7.1f N m%s%s\n",
                    preset.c_str(), s.f1, s.f2, s.m1, s.m2, b1 ? " (1st shafts)" : "", b2 ? " (2nd shafts)" : "");
        return s;
    }
} // namespace

TEST_CASE("shaking by order: the known engines", "[core][shake]")
{
    // m r w^2 for these masses (0.5 + 0.5 / 3 kg, r 45 mm, 6000 rpm): some 11.8 kN; "none" is under a millionth of it
    const double unit = (0.5 + 0.5 * 0.333) * 0.045 * std::pow(6000.0 * kPi / 30.0, 2);
    const double none = 1e-5 * unit;

    const Shake i4 = shake("ref-05-inline-4");
    CHECK(i4.f1 < none);
    CHECK(i4.f2 > 0.5 * unit);         // four cylinders' second order: 4 / lambda (3.33) of the unit
    CHECK(std::abs(i4.f2 - 4.0 / 3.333 * unit) < 0.01 * unit);

    const Shake i6 = shake("ref-10-inline-6");
    CHECK(i6.f1 < none);
    CHECK(i6.f2 < none);
    CHECK(i6.m1 < none);
    CHECK(i6.m2 < none);

    const Shake boxer = shake("ref-06-flat-4");
    CHECK(boxer.f1 < none);
    CHECK(boxer.f2 < none);
    CHECK(boxer.m1 + boxer.m2 > 0.01 * unit * 0.1);

    const Shake xp = shake("ref-07-xplane-i4");
    CHECK(xp.f2 < none);
    CHECK(xp.m1 > 0.01 * unit * 0.1);

    // and a few more as checks of the sums: a crossplane V8 has no force of either order; a 60-degree V6 a first-order
    // rocking moment
    const Shake v8 = shake("ref-12-v8-cross");
    CHECK(v8.f1 < none);
    CHECK(v8.f2 < none);
    const Shake flat8 = shake("ref-13-v8-flat");
    CHECK(flat8.f1 < none);
    CHECK(flat8.f2 > 0.5 * unit);      // two flat-plane fours at 90 degrees: their second orders add sideways
}

TEST_CASE("balance shafts cancel the order they claim, and only that", "[core][shake]")
{
    const Shake plain = shake("ref-05-inline-4");
    const Shake shafts = shake("ref-05-inline-4", false, true);
    CHECK(shafts.f2 == 0.0);
    CHECK(shafts.f2raw == plain.f2);
    CHECK(shafts.m1 == plain.m1);
    const Shake twin = shake("twin-180");
    const Shake twinShaft = shake("twin-180", true, false);
    CHECK(twin.f1 < 1e-3);             // a 180-degree twin: no first-order force, a first-order moment
    CHECK(twin.m1 > 1.0);
    CHECK(twinShaft.m1 == twin.m1);    // a force-balancing shaft leaves the moment
    const Shake single = shake("ref-00-single");
    const Shake singleShaft = shake("ref-00-single", true, false);
    CHECK(single.f1 > 1000.0);
    CHECK(singleShaft.f1 == 0.0);
    CHECK(singleShaft.f2 == single.f2);
}
