// tests/sim/calibration/boost/GateTest.cpp: card N2's: what was fixed in the boosted engines stays fixed.
// On the engine dyno at full load, warm (Bench.h). Each case a few seconds of the machine's time over 20 s of engine.
#include "Bench.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
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

TEST_CASE("calibration boost: twin turbos on one bank, each fed by its own collector, both spin", "[calibration][boost]")
{
    // A straight six's twin turbos: before, Boost::housingOf mapped twin turbos by bank, so on a single bank every
    // collector ended in turbo 0: turbo 1 stood (0 rad/s, its compressor flowing backwards) and turbo 0 choked and
    // over-sped (boost 61 kPa at 4300 rpm against 100). Now the front collector (1-3) feeds turbo 0, the rear (4-6)
    // turbo 1; here a dual exhaust, a downpipe each.
    Sims s;
    std::vector<std::pair<std::string, double>> sets = {{"exhaust.sides", 2}, {"exhaust.collectorTo[0]", 0},
                                                        {"exhaust.collectorTo[1]", 1}};
    for (int i = 0; i < 6; ++i)
        sets.emplace_back("exhaust.primaryTo[" + std::to_string(i) + "]", i < 3 ? 0 : 1);
    std::vector<Row> rows;
    std::string error;
    REQUIRE(tee::calib::dyno(s.sim, "content/builds/archetypes/straight-six-twin-turbo-30.json", {4297.0}, sets,
                             {"boost.pressure", "boost.shaftSpeed[0]", "boost.shaftSpeed[1]", "engine.torque"}, rows,
                             error, 4.0, 1.0));
    INFO(error);
    const Row& r = rows[0];
    std::printf("twin six at 4297 rpm: boost %.1f kPa, shafts %.0f and %.0f rad/s, %.0f N m\n",
                r.at("boost.pressure") / 1000.0, r.at("boost.shaftSpeed[0]"), r.at("boost.shaftSpeed[1]"),
                r.at("engine.torque"));
    CHECK(r.at("boost.shaftSpeed[1]") > 0.8 * r.at("boost.shaftSpeed[0]"));
    CHECK(r.at("boost.shaftSpeed[1]") < 1.25 * r.at("boost.shaftSpeed[0]"));
    CHECK(r.at("boost.pressure") > 85000.0);
}

TEST_CASE("calibration boost: variable vanes past the surge line open (one state, no worse than a fixed nozzle)",
          "[calibration][boost]")
{
    // Card D2's 15 L truck at 1300 rpm, full load, its target out of reach (the spring's 220 kPa): the vanes' loop
    // shut them to 0.47 and the engine stood in a second state, 98 kPa and 882 N m, its compressor surging, against
    // 209 kPa and 1945 N m with the vanes at 0.9 (a fixed nozzle 206 kPa). Now the surge guard opens them: the same
    // state whether 1300 rpm is reached from 800 or from 2000, and at least the fixed nozzle's boost.
    Sims s;
    const std::vector<std::pair<std::string, double>> base = {{"boost.model", 1},       {"boost.kind", 1},
                                                              {"boost.layout", 1},      {"boost.gateSpring", 220000},
                                                              {"boost.icFlow", 0.8},    {"boost.chargeVolume", 0.02}};
    const auto run = [&](double vanes, double from) {
        auto sets = base;
        sets.emplace_back("boost.vanes", vanes);
        std::vector<Row> rows;
        std::string error;
        REQUIRE(tee::calib::dyno(s.sim, "content/builds/truck-15l-six.json", {from, 1300.0}, sets,
                                 {"boost.pressure", "engine.torque"}, rows, error, 5.0, 1.0));
        return rows[1];
    };
    const Row fixed = run(0, 800.0);
    const Row up = run(1, 800.0);
    const Row down = run(1, 2000.0);
    std::printf("truck at 1300 rpm: fixed %.1f kPa %.0f N m; vanes from 800 %.1f kPa %.0f N m, from 2000 %.1f kPa %.0f N m\n",
                fixed.at("boost.pressure") / 1000.0, fixed.at("engine.torque"), up.at("boost.pressure") / 1000.0,
                up.at("engine.torque"), down.at("boost.pressure") / 1000.0, down.at("engine.torque"));
    CHECK(std::fabs(up.at("boost.pressure") - down.at("boost.pressure")) < 0.05 * down.at("boost.pressure"));
    CHECK(up.at("boost.pressure") > 0.97 * fixed.at("boost.pressure"));
    CHECK(up.at("engine.torque") > 0.95 * fixed.at("engine.torque"));
}

TEST_CASE("calibration boost: the telemetry says when the charge system is far from its engine", "[calibration][boost]")
{
    // (the lead's X-021 ruling) A 15 L diesel with F's defaults (a 0.25 kg/s cooler and 5 L of pipes, a 2 to 3 L
    // engine's) makes little boost, as it should; tel.boost.mismatch says why, with each pair (has, wants). The 2 L
    // four, its own sizes, says nothing.
    Sims s;
    const auto read = [&](const std::string& build, const std::vector<std::pair<std::string, double>>& sets) {
        std::vector<Row> rows;
        std::string error;
        REQUIRE(tee::calib::dyno(s.sim, build, {2000.0}, sets,
                                 {"boost.mismatch", "boost.coolerHas", "boost.coolerWants", "boost.volumeHas",
                                  "boost.volumeWants"},
                                 rows, error, 0.2, 0.1, 2.0));
        return rows[0];
    };
    const Row big = read("content/builds/combo/large-diesel.json",
                         {{"boost.model", 1}, {"boost.icFlow", 0.25}, {"boost.chargeVolume", 0.005}});
    const Row four = read("content/builds/archetypes/turbo-four-20-gdi.json", {});
    std::printf("mismatch: large diesel %d (cooler %.2f against %.2f kg/s, pipes %.1f against %.1f L); turbo four %d\n",
                static_cast<int>(big.at("boost.mismatch")), big.at("boost.coolerHas"), big.at("boost.coolerWants"),
                1000.0 * big.at("boost.volumeHas"), 1000.0 * big.at("boost.volumeWants"),
                static_cast<int>(four.at("boost.mismatch")));
    CHECK(static_cast<int>(big.at("boost.mismatch")) == 5);
    CHECK(big.at("boost.coolerWants") > 2.0 * big.at("boost.coolerHas"));
    CHECK(static_cast<int>(four.at("boost.mismatch")) == 0);
}
