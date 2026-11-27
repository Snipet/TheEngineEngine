// The pull and the launch drive an automatic and a CVT (the lead's ruling, 27 Nov 11:37): those boxes take R, N, D from
// the lever alone (sim/vehicle/gearbox.fm), so a scenario that drives by the shift counters with the lever in N ran them in
// neutral, the engine free against its limiter. The scenarios put the lever in D as they pull away and back in N as the car
// stops, and move no other box's lever (a manual's is its H-pattern). This case holds both halves.

#include "Combo.h"

#include <catch2/catch_test_macros.hpp>

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
    CHECK_FALSE(leverDrives(corpusFile("tractor-15-inline-six")));  // Eighteen-speed: no driver yet
    CHECK_FALSE(leverDrives(buildPath("turbo-six")));               // the generator's (dual clutch)
    CHECK_FALSE(leverDrives(buildPath("large-diesel")));            // no box named: the default, a manual

    // the scenarios the generator plays (and the files under content/scenarios/combo) are the drive=false ones, unchanged;
    // only a box the lever drives gets the lever's two steps
    for (Kind k : {Kind::Pull, Kind::Launch})
    {
        const std::string n = scenarioJson(k, 0u, false), d = scenarioJson(k, 0u, true);
        CHECK(n == scenarioJson(k, 0u));
        CHECK(n != d);
        CHECK(n.find("\"lever\": 1 ") == std::string::npos);
        CHECK(d.find("\"lever\": 1 ") != std::string::npos);
    }
    CHECK(scenarioJson(Kind::Tour, 0u, false) == scenarioJson(Kind::Tour, 0u, true));   // the tour drives nowhere

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
