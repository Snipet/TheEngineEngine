// The pull and the launch drive an automatic and a CVT (the lead's ruling, 27 Nov 11:37): those boxes take R, N, D from
// the lever alone (sim/vehicle/gearbox.fm), so a scenario that drives by the shift counters with the lever in N ran them in
// neutral, the engine free against its limiter. The scenarios put the lever in D as they pull away and back in N as the car
// stops, and move no other box's lever (a manual's is its H-pattern). This case holds both halves.

#include "Combo.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string>

using namespace combo;

namespace
{
    std::string corpusFile(const char* stem)
    {
        return (tee::test::contentDirectory() / "builds" / "archetypes" / (std::string(stem) + ".json")).string();
    }
}

TEST_CASE("the pull drives an automatic and a CVT in D, and moves no other box's lever", "[fast][fast-3][combo]")
{
    // the boxes, as their builds say
    CHECK(leverDrives(corpusFile("twin-turbo-hot-vee-v8-40")));    // Automatic
    CHECK(leverDrives(corpusFile("economy-four-15")));             // CVT
    CHECK_FALSE(leverDrives(corpusFile("three-cylinder-10-turbo")));  // Manual
    CHECK_FALSE(leverDrives(corpusFile("flat-plane-v8-39-twin-turbo")));  // Dual clutch
    CHECK_FALSE(leverDrives(corpusFile("tractor-15-inline-six")));  // Eighteen-speed: its own driver
    CHECK(driverFor(corpusFile("tractor-15-inline-six")) == Driver::Truck);
    CHECK(driverFor(corpusFile("economy-four-15")) == Driver::Lever);
    CHECK(driverFor(corpusFile("three-cylinder-10-turbo")) == Driver::Presses);
    CHECK_FALSE(leverDrives(buildPath("turbo-six")));               // the generator's (dual clutch)
    CHECK_FALSE(leverDrives(buildPath("large-diesel")));            // no box named: the default, a manual

    // the scenarios the generator plays (and the files under content/scenarios/combo) are the drive=false ones, unchanged;
    // only a box the lever drives gets the lever's two steps
    for (Kind k : {Kind::Pull, Kind::Launch})
    {
        const std::string n = scenarioJson(k, 0u, Driver::Presses), d = scenarioJson(k, 0u, Driver::Lever);
        CHECK(n == scenarioJson(k, 0u));
        CHECK(n != d);
        CHECK(n.find("\"lever\": 1 ") == std::string::npos);
        CHECK(d.find("\"lever\": 1 ") != std::string::npos);
    }
    CHECK(scenarioJson(Kind::Tour, 0u, Driver::Presses) == scenarioJson(Kind::Tour, 0u, Driver::Lever));   // the tour drives nowhere
    // the truck's pull-away: the L slot and the clutch, on the pull only
    {
        const std::string t = scenarioJson(Kind::Pull, 0u, Driver::Truck);
        CHECK(t.find("\"lever\": 5 ") != std::string::npos);
        CHECK(t.find("\"lever\": 1 ") == std::string::npos);
        CHECK(scenarioJson(Kind::Launch, 0u, Driver::Truck) == scenarioJson(Kind::Launch, 0u, Driver::Presses));
    }

    // and the car moves: in gear and under way mid-pull, where in N it stood still at gear 0
    for (const char* stem : {"economy-four-15", "twin-turbo-hot-vee-v8-40"})
    {
        Times tm;
        const tee::test::Run r = scenario(Kind::Pull, corpusFile(stem), 0u, tm).run(9.0);
        INFO(stem);
        CHECK(r.tel("car.gear").at(8.0) >= 1.0);
        CHECK(r.tel("car.speed").at(8.0) > 5.0);
        if (std::string(stem) == "twin-turbo-hot-vee-v8-40")
            CHECK(r.tel("car.gear").at(8.0) >= 2.0);   // the automatic's own upshifts (1-2 near 6.0 s)
    }
}

// The reference driven its own way (the lead's ruling, 27 Nov 12:07): a build's reference (every switch off: car.model 0,
// the reference's own car, which reads no `gearbox`) is the manual the presses drive with the lever in N. Under the
// lever's 1 it was an H-pattern dropped into first with no clutch, and it stalled: the automatics' wide bounds (up to
// 17.02) were a stalled engine's. So a reference never moves the lever, and a reference that stalls while driven throws.
TEST_CASE("a lever-driven build's reference drives on its presses, and a stalled reference throws", "[fast][fast-3][combo]")
{
    const std::string cvt = corpusFile("economy-four-15");
    auto everyOff = [](tee::test::Scenario sc) {
        sc.at(0.0);
        for (const Switch& w : switches())
            sc.spec(w.path, 0);
        return sc;
    };
    {
        Times tm;
        const tee::test::Run r = everyOff(referenceScenario(Kind::Pull, cvt, tm)).run(tm.driveTo);
        CHECK(r.tel("car.gear").at(8.0) >= 2.0);    // its own upshift on the press at 6.5 s: a manual on its presses
        CHECK(r.tel("car.speed").at(8.0) > 5.0);
        CHECK_NOTHROW(requireReferenceRuns(r, tm, "economy-four-15 pull"));
    }
    {
        // the guard's bite: the same reference under the case's scenario (the lever to D, its first slammed in) stalls
        Times tm;
        const tee::test::Run r = everyOff(scenario(Kind::Pull, cvt, 0u, tm)).run(tm.driveTo);
        CHECK(r.tel("engine.speed").at(5.0) < 1.0);
        CHECK_THROWS_WITH(requireReferenceRuns(r, tm, "economy-four-15 pull"),
                          Catch::Matchers::ContainsSubstring("stalled while driven"));
    }
}
