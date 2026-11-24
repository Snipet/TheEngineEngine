// tests/sim/calibration/boost/GateTest.cpp: card N2's: what was fixed in the boosted engines stays fixed.
// On the engine dyno at full load, warm (Bench.h). Each case a few seconds of the machine's time over 20 s of engine.
#include "Bench.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdio>

using tee::calib::Row;

namespace
{
    struct Sims
    {
        std::shared_ptr<tee::Sim> sim;
        Sims()
        {
            std::string errors;
            sim = tee::calib::compileSim(errors);
            INFO(errors);
            REQUIRE(sim != nullptr);
        }
    };
} // namespace

TEST_CASE("calibration boost: a matched wastegate holds its target spooled at the top speed (no runaway)",
          "[calibration][boost]")
{
    // Before (gate 0.85 of the nozzle's area): the 2 L four spooled at 5271 rpm ran away to 255 kPa against 120, its
    // gate wide open, knock retard at 15 degrees; the kei could not hold its 60 kPa spring above 5000 rpm.
    Sims s;
    struct Case
    {
        const char* build;
        double target;
        std::vector<double> rpm;
    };
    const Case cases[] = {
        {"content/builds/archetypes/turbo-four-20-gdi.json", 120000.0, {5271.0, 6528.0}},
        {"content/builds/archetypes/kei-660-triple-turbo.json", 60000.0, {6500.0, 7800.0}},
    };
    for (const Case& c : cases)
    {
        std::vector<Row> rows;
        std::string error;
        REQUIRE(tee::calib::dyno(s.sim, c.build, c.rpm, {{"boost.target", c.target}, {"ecu.maps.boostAuto", 1.0}},
                                 {"boost.pressure", "boost.gate[0]", "engine.torque"}, rows, error, 4.0, 1.0));
        INFO(error);
        for (const Row& r : rows)
        {
            const double boost = r.at("boost.pressure");
            std::printf("%s at %.0f rpm: boost %.1f kPa (target %.0f), gate %.2f, %.0f N m\n", c.build, r.at("rpm"),
                        boost / 1000.0, c.target / 1000.0, r.at("boost.gate[0]"), r.at("engine.torque"));
            CHECK(boost < 1.1 * c.target);
            CHECK(boost > 0.85 * c.target);
            CHECK(r.at("boost.gate[0]") < 0.98);   // holding, not saturated
        }
    }
}

TEST_CASE("calibration boost: back pressure against boost on a matched turbo", "[calibration][boost]")
{
    // A matched turbo at full load runs its turbine inlet at or above the boost (1.0 to 1.5; Watson and Janota,
    // Turbocharging the Internal Combustion Engine, ch. 7): the cylinder's exhaust port, averaged over whole cycles,
    // at or above its intake port's. Before card G's network fed the housings: 0.61 to 0.87.
    Sims s;
    std::vector<Row> rows;
    std::string error;
    REQUIRE(tee::calib::dyno(s.sim, "content/builds/archetypes/turbo-four-20-gdi.json", {3500.0, 5000.0, 6500.0}, {},
                             {"boost.pressure", "boost.turbineIn[0]"}, rows, error, 4.0, 1.0));
    INFO(error);
    for (const Row& r : rows)
    {
        const double ratio = r.at("portExhaust") / r.at("portIntake");
        std::printf("turbo four at %.0f rpm: exhaust port %.0f kPa, intake port %.0f kPa (%.2f), turbine in %.0f kPa\n",
                    r.at("rpm"), r.at("portExhaust") / 1000.0, r.at("portIntake") / 1000.0, ratio,
                    r.at("boost.turbineIn[0]") / 1000.0);
        CHECK(ratio > 1.0);
        CHECK(ratio < 1.7);
    }
}

TEST_CASE("calibration boost: one state: stepped up from 1500 rpm or spooled at the top, the same boost",
          "[calibration][boost]")
{
    // Before: the 2 L four stepped up from 1500 rpm (as the ledger and the stepped dyno run) read 348 kPa absolute
    // in its manifold at 5271 rpm, the runaway; spooled from 2000 it held its target. Within 10 % either way now.
    Sims s;
    const char* build = "content/builds/archetypes/turbo-four-20-gdi.json";
    std::vector<Row> stepped, spooled;
    std::string error;
    REQUIRE(tee::calib::dyno(s.sim, build, {1501.0, 2754.0, 4014.0, 5271.0}, {}, {"boost.pressure"}, stepped, error,
                             3.0, 1.0, 4.0));
    INFO(error);
    REQUIRE(tee::calib::dyno(s.sim, build, {5271.0}, {}, {"boost.pressure"}, spooled, error, 3.0, 1.0, 4.0));
    const double a = stepped.back().at("boost.pressure");
    const double b = spooled.back().at("boost.pressure");
    std::printf("turbo four at 5271 rpm: stepped from 1500 %.1f kPa, spooled there %.1f kPa\n", a / 1000.0, b / 1000.0);
    CHECK(a == Catch::Approx(b).epsilon(0.1));
    CHECK(a < 1.1 * 120000.0);
}

TEST_CASE("calibration boost: the ECU's over-boost cut is the net under a gate too small", "[calibration][boost]")
{
    // ecu.model 1 (every archetype): the 2 L four with the gate it had before (26.3 mm) runs away spooled at 5271 rpm
    // (255 kPa, 421 N m) with the cut off; with the cut at 150 kPa the fuel is cut each time the boost has stood over
    // it for a second, and the mean torque falls (the cut cycles, as a production ECU's does). With the matched gate the
    // boost never reaches the cut (the build's own target map, 105 kPa at 5300 rpm, P's taper).
    Sims s;
    const char* build = "content/builds/archetypes/turbo-four-20-gdi.json";
    std::vector<Row> off, cut, matched;
    std::string error;
    REQUIRE(tee::calib::dyno(s.sim, build, {5271.0}, {{"boost.gateDiameter", 0.0263}, {"ecu.overBoost", -1.0}},
                             {"boost.pressure", "engine.torque"}, off, error, 3.0, 3.0, 4.0));
    INFO(error);
    REQUIRE(tee::calib::dyno(s.sim, build, {5271.0}, {{"boost.gateDiameter", 0.0263}, {"ecu.overBoost", 150000.0}},
                             {"boost.pressure", "engine.torque"}, cut, error, 3.0, 3.0, 4.0));
    REQUIRE(tee::calib::dyno(s.sim, build, {5271.0}, {{"ecu.overBoost", 150000.0}}, {"boost.pressure", "engine.torque"},
                             matched, error, 3.0, 1.0, 4.0));
    std::printf("over-boost at 5271 rpm: cut off %.0f kPa %.0f N m; cut at 150 kPa %.0f kPa %.0f N m; matched gate "
                "%.0f kPa %.0f N m\n", off[0].at("boost.pressure") / 1000.0, off[0].at("engine.torque"),
                cut[0].at("boost.pressure") / 1000.0, cut[0].at("engine.torque"),
                matched[0].at("boost.pressure") / 1000.0, matched[0].at("engine.torque"));
    CHECK(off[0].at("boost.pressure") > 200000.0);
    CHECK(cut[0].at("engine.torque") < 0.9 * off[0].at("engine.torque"));
    CHECK(cut[0].at("boost.pressure") < 0.95 * off[0].at("boost.pressure"));
    CHECK(matched[0].at("boost.pressure") < 132000.0);
}
