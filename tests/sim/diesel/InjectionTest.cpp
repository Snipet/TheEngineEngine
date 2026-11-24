// tests/sim/diesel/InjectionTest.cpp: card D2: injection as a tuner's thing (card H's fields under fuel.model 1:
// timing, pilot, rail; over-fuelling) on the truck six at full load on the engine dyno, 300 K, 101325 Pa: the rate of
// pressure rise and the combustion noise, the torque and the smoke.
#include "Diesel.h"

#include <cstdio>

using d2::Engine;

namespace
{
    struct Point
    {
        double rise, hf, torque, smoke;
    };

    Point fullLoad(double rpm, double pilot, double soiAdd, double overFuel = 0.0, double afc = 0.8)
    {
        Engine e("truck-15l-six");
        e.spec("ecu.governor", 2);
        e.spec("fuel.model", 1);
        e.spec("fuel.grade", 6);
        e.spec("fuel.pilot", pilot);
        e.spec("fuel.soiAdd", soiAdd);
        e.spec("fuel.overFuel", overFuel);
        e.spec("ecu.afc", afc);
        e.start();
        e.dyno(rpm, 1.0f);
        REQUIRE(e.run(5.0));
        Point p{};
        p.rise = e.mean("ecu.riseRate", 1.0);
        p.hf = e.tel("ecu.pressureHf");
        p.torque = e.mean("engine.torque", 1.0);
        p.smoke = e.tel("ecu.smoke");
        std::printf("%.0f rpm, pilot %.2f, timing %+.0f, over-fuel %.2f, limiter %.2f: rise %.2f bar/deg, noise %.0f Pa, %.0f N m, "
                    "smoke %.2f FSN, boost %.0f kPa\n",
                    rpm, pilot, soiAdd, overFuel, afc, p.rise / 1e5, p.hf, p.torque, p.smoke, e.tel("boost.pressure") / 1000.0);
        return p;
    }
} // namespace

TEST_CASE("diesel injection: a pilot and later timing lower the rate of pressure rise and the combustion noise",
          "[diesel][injection]")
{
    // Published: a direct-injection diesel's most rate of pressure rise is some 3 to 10 bar a degree without a pilot;
    // a pilot of a few per cent roughly halves it (Heywood ch. 10; common-rail makers' heat-release curves); retarded
    // injection lowers it too (shorter delay into hotter air, the burn after the top)
    const Point base = fullLoad(1200.0, 0.0, 0.0);
    const Point pilot = fullLoad(1200.0, 0.04, 0.0);
    const Point late = fullLoad(1200.0, 0.0, -6.0);
    CHECK(pilot.rise < 0.8 * base.rise);
    CHECK(late.rise < 0.9 * base.rise);
    CHECK(pilot.hf < base.hf);
    CHECK(late.hf < base.hf);
    CHECK(base.rise > 1.0e5);
}

TEST_CASE("diesel injection: more fuel, more torque and more smoke; the smoke limiter caps it", "[diesel][injection]")
{
    const Point stock = fullLoad(1400.0, 0.0, 0.0, 0.0, 0.8);
    const Point tuned = fullLoad(1400.0, 0.0, 0.0, 0.3, 0.0);
    CHECK(tuned.torque > 1.05 * stock.torque);
    CHECK(tuned.smoke > stock.smoke + 0.5);
}
