// tests/sim/bike/PropertyTest.cpp: the motorcycle never runs the crank away (card I's passive.engine, finding X-007,
// for the bike): custom singles at the corners of the Spec's ranges and the 600's 16,000 rpm four, on the bike's
// sequential box, in neutral and in gear, under random throttle, clutch and brakes, with the checks on. (A bike's rev
// limiter caps what the defect could do here: stepped with configure's inertia, X-007's corner reached 943 rad/s where
// it now reaches 496, under the ceiling either way. BikeTest.cpp's bike.passive.crank is the test that sees it.)
#include "tee/test/Scenario.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <cstdlib>
#include <string>

namespace
{
    void ride(tee::test::Scenario& s, int geared, unsigned seed)
    {
        s.at(0.0).spec("car.model", "Vehicle").spec("car.kind", "Motorcycle");
        s.at(0.0).spec("env.temperature", 300.0).spec("env.pressure", 101325.0);
        s.at(0.0).mode(1).lever(0).clutch(0.0).brake(0.0).throttle(0.0).ignition(tee::On);
        s.at(0.25).ignition(tee::Start).until(1.5).ignition(tee::On);
        if (geared != 0)
            s.at(1.6).shiftUp();
        auto rnd = [&seed] {
            seed = seed * 1664525u + 1013904223u;
            return static_cast<double>(seed >> 8) / 16777216.0;
        };
        for (double t = 1.8; t < 5.0; t += 0.2)
        {
            s.at(t).throttle(rnd() < 0.2 ? 0.0 : rnd()).brake(rnd() < 0.8 ? 0.0 : rnd()).clutch(rnd() < 0.15 ? 1.0 : 0.0);
            if (geared != 0 && rnd() < 0.15)
                s.at(t).shiftUp();
            else if (geared != 0 && rnd() < 0.1)
                s.at(t).shiftDown();
        }
    }

    void check(const tee::test::Run& r, const std::string& what, double limit)
    {
        INFO(what);
        REQUIRE(r.ok());
        const double top = r.tel("engine.speed").max();
        if (std::getenv("TEE_BIKE_VERBOSE") != nullptr || r.host().stops != 0 || r.host().guardTrips != 0 || top > limit)
            std::printf("bike.passive.engine: %s: stops %u, guard trips %u, crank up to %.0f rad/s (%s)\n", what.c_str(),
                        r.host().stops, r.host().guardTrips, top, r.stopReason().c_str());
        CHECK(r.host().stops == 0);
        CHECK(r.host().guardTrips == 0);
        CHECK(top < limit);
        CHECK(r.audio().finite());
    }
} // namespace

TEST_CASE("bike.passive.engine: custom singles at their Spec's corners and a 16,000 rpm four, on the bike, never run "
          "away", "[bike][property]")
{
    int runs = 0;
    for (int corner = 0; corner < 5; ++corner)
        for (int geared = 0; geared < 2; ++geared)
        {
            // X-007's heavy piston on a long stroke and a light flywheel, a light one on a short stroke and a heavy
            // flywheel, the two between, and a big road single (102 x 90 mm, 650 cc) on a light flywheel
            const double bore[5] = {0.037, 0.12, 0.037, 0.12, 0.102};
            const double stroke[5] = {0.153, 0.05, 0.05, 0.153, 0.09};
            const double piston[5] = {18.4, 0.3, 20.0, 5.0, 0.6};
            const double flywheel[5] = {0.24, 1.0, 0.0, 0.5, 0.1};
            tee::test::Scenario s;
            s.checks();
            s.at(0.0).spec("engine.custom", 1.0).spec("engine.cylinders", 1.0).spec("engine.layout", "Single");
            s.at(0.0).spec("engine.bore", bore[corner]).spec("engine.stroke", stroke[corner]);
            s.at(0.0).spec("engine.pistonMass", piston[corner]).spec("engine.flywheel", flywheel[corner]);
            ride(s, geared, 4242u + static_cast<unsigned>(corner * 7 + geared));
            check(s.run(5.0), "single, corner " + std::to_string(corner) + ", in gear " + std::to_string(geared), 3000.0);
            runs += 1;
        }
    for (int geared = 0; geared < 2; ++geared)
    {
        tee::test::Scenario s("bike-supersport-600");
        s.checks();
        ride(s, geared, 777u + static_cast<unsigned>(geared));
        check(s.run(5.0), "the 600's four, in gear " + std::to_string(geared), 3000.0);
        runs += 1;
    }
    std::printf("bike.passive.engine: %d runs of 5 s with the checks on\n", runs);
}
