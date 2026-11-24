// tests/sim/boost/ExploreTest.cpp: hidden runs that print the machine's state over time, for reading by eye
// (tee-boost-tests "[.explore]" -s). They assert nothing.
#include "Rig.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdio>

using tee::test::boost::Rig;

TEST_CASE("boost: explore a turbo four at full throttle", "[.explore]")
{
    Rig rig;
    rig.set("spec.kind", 1);
    rig.spec("model", 1);
    rig.set("rpm", 4500);
    rig.set("throttle", 1.0);
    rig.set("pulsed", 1);
    for (int i = 0; i < 40; ++i)
    {
        rig.clear();
        rig.run(0.1);
        std::printf("t %.1f  shaft %7.0f rpm  boost %6.0f Pa  pr %.2f  flow %.3f  eta %.2f  gate %.2f  pT %7.0f  tT %5.0f  "
                    "drive %.2f  air %.3f  torque %6.1f  icOut %.0f  flags %d\n",
                    0.1 * (i + 1), rig.shaftRpm(), rig.boost(), rig.get("boost.telPr[0]"), rig.get("boost.telFlow[0]"),
                    rig.get("boost.telEta[0]"), rig.get("boost.telGate[0]"), rig.get("boost.telPTurb[0]"),
                    rig.get("boost.telTTurb[0]"), rig.get("boost.telDrive[0]"), rig.mean("sAir"), rig.mean("sTorque"),
                    rig.get("boost.telIcOut"), (int)rig.get("boost.flags"));
        std::printf("   qSrc %.5f mOutPipe %.4f aNoz %.6f turbos %d active %d dpMass %.4f house.vol %.6f m %.6f\n", rig.get("boost.qSrc[0]"),
                    rig.get("boost.house[0].mOutPipe"), rig.get("boost.turb[0].aNoz"), (int)rig.get("boost.turbos"),
                    (int)rig.get("boost.active"), rig.get("boost.dpMass[0]"), rig.get("boost.house[0].vol"), rig.get("boost.house[0].m"));
    }
}
