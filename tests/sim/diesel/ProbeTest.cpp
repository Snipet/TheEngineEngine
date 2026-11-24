// hidden probes: what the builds are (run with "[.probe]")
#include "Diesel.h"
#include <cstdio>
using d2::Engine;
TEST_CASE("probe: the truck six's full-load curve", "[.probe]")
{
    for (int gov : {0, 2}) {
    Engine e("truck-15l-six");
    e.spec("ecu.governor", gov);
    e.spec("engine.idleTrim", -400);
    e.spec("engine.redlineTrim", -500);
    e.start();
    std::printf("idle %.0f rpm, redline %.0f rpm\n", e.tel("engine.idle") / 0.10471976, e.tel("engine.redline") / 0.10471976);
    for (double rpm : {1000.0, 1200.0, 1400.0, 1600.0, 1800.0, 2000.0})
    {
        e.dyno(rpm, 1.0f);
        REQUIRE(e.run(5.0));
        const double tq = e.mean("engine.torque", 1.0);
        std::printf("%.0f rpm: %.0f N m, %.0f kW, boost %.0f kPa, rack %.2f phi %.2f smoke %.1f tPort %.0f flags %.0f\n", rpm, tq, tq * rpm * 0.10471976 / 1000.0,
                    e.tel("boost.pressure") / 1000.0, e.tel("ecu.rack"), e.get("comb.diesel.phiAsked"), e.tel("ecu.smoke"), e.get("exh.tPort"), e.tel("engine.flags"));
    }}
}
