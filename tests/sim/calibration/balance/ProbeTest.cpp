// tests/sim/calibration/balance/ProbeTest.cpp: card N3's balance probe (hidden: run by hand, or by scripts/balance.py).
// Prints one archetype's air and energy books at each speed asked, as "key value" lines between "begin" and "end" (the
// script reads them) and as a short table for a person.
//
//   BAL_BUILD=air-cooled-flat-six-36 BAL_RPM=5199 build-dev/bin/tee-tests-calibration-balance "[.balance]"
//   (BAL_SET="engine.wallHeat=1;..." Spec writes; BAL_RAW="spec.ecu.maps.lambda.nx=1;..." any path; BAL_SIM=DIR another tree's sim/; BAL_SETTLE, BAL_MEASURE seconds)
#include "Balance.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace
{
    std::vector<std::string> split(const std::string& s, char c)
    {
        std::vector<std::string> out;
        std::stringstream in(s);
        std::string item;
        while (std::getline(in, item, c))
            if (!item.empty())
                out.push_back(item);
        return out;
    }
} // namespace

TEST_CASE("probe: an engine's air and energy books", "[.balance]")
{
    using namespace tee::balance;
    const char* b = std::getenv("BAL_BUILD");
    const char* r = std::getenv("BAL_RPM");
    Options o;
    if (const char* s = std::getenv("BAL_SET"))
        for (const std::string& kv : split(s, ';'))
        {
            const auto eq = kv.find('=');
            o.set.emplace_back(kv.substr(0, eq), std::atof(kv.c_str() + eq + 1));
        }
    if (const char* s = std::getenv("BAL_RAW"))
        for (const std::string& kv : split(s, ';'))
        {
            const auto eq = kv.find('=');
            o.raw.emplace_back(kv.substr(0, eq), std::atof(kv.c_str() + eq + 1));
        }
    if (const char* s = std::getenv("BAL_READ"))
        o.read = split(s, ',');
    if (const char* s = std::getenv("BAL_TRACE"))
        o.trace = split(s, ',');
    if (const char* s = std::getenv("BAL_SIM"))
        o.sim = s;
    if (const char* s = std::getenv("BAL_SETTLE"))
        o.settle = std::atof(s);
    if (const char* s = std::getenv("BAL_MEASURE"))
        o.measure = std::atof(s);
    keepPlayer(true);
    for (const std::string& slug : split(b ? b : "air-cooled-flat-six-36", ','))
        for (const std::string& rs : split(r ? r : "5199", ','))
        {
            o.rpm = std::atof(rs.c_str());
            Books k;
            std::string error;
            const bool ok = measure(slug, o, k, error);
            INFO(error);
            REQUIRE(ok);
            const double F = k.fuelEnergy > 0.0 ? k.fuelEnergy : 1.0;
            const double perL = 1e3 / (k.vd * 1e3) / std::max(k.cycles, 1e-9);   // g per cycle per litre, from kg
            std::printf("begin %s %.0f\n", slug.c_str(), o.rpm);
            const std::pair<const char*, double> lines[] = {
                {"rpm", k.rpm}, {"torque", k.torque}, {"cycles", k.cycles}, {"vd_l", k.vd * 1e3}, {"cylinders", double(k.cylinders)},
                {"air_throttle", k.airThrottle * perL}, {"air_valves", k.airValves * perL}, {"gas_exhaust", k.gasExhaust * perL},
                {"trapped_total", k.trappedTotal * perL}, {"trapped_fresh", k.trappedFresh * perL},
                {"trapped_burned", k.trappedBurned * perL}, {"fuel_air", k.fuelAir * perL}, {"fuel", k.fuel * perL},
                {"lambda", k.lambda}, {"lhv", k.lhv},
                {"mix_heat", k.mixHeat / F}, {"charged", k.chargedHeat / F}, {"released", k.released / F},
                {"wall_compression", k.wallCompression / F}, {"wall_burn", k.wallBurn / F}, {"wall_expansion", k.wallExpansion / F},
                {"wall_exchange", k.wallExchange / F}, {"walls", k.walls() / F},
                {"work_gross", k.workGross / F}, {"work_pump", -k.workPump / F}, {"brake", k.brakeWork() / F},
                {"friction", (k.workGross + k.workPump - k.brakeWork()) / F},
                {"exhaust_h", (k.hOut - k.hIn) / F}, {"exhaust_sensible", (k.hOut - k.hOutAtInlet) / F},
                {"first_law", k.firstLaw() / F}, {"du", (k.uEnd - k.uStart) / F},
                {"t_amb", k.tAmb}, {"t_air_heated", k.tAirHeated}, {"p_plenum", k.pPlenum}, {"t_ivc", k.tIvc},
                {"ve_air_tel", k.veAirTel}, {"ve_man_tel", k.veManTel}, {"imep_tel", k.imepTel}, {"pmep_tel", k.pmepTel},
                {"fmep_tel", k.fmepTel}, {"bmep_tel", k.bmepTel}, {"spark_tel", k.sparkTel},
                {"imep_gross", k.workGross / (k.vd * std::max(k.cycles, 1e-9))},
            };
            for (const auto& [key, value] : lines)
                std::printf("%s %.6g\n", key, value);
            // BAL_READ="vt.lab.stageWant,heat.tOil": instance paths read at the window's end
            if (const char* rdEnv = std::getenv("BAL_READ"))
                for (const std::string& path : split(rdEnv, ','))
                    std::printf("read %s %.6g\n", path.c_str(), k.reads.count(path) ? k.reads.at(path) : std::nan(""));
            // BAL_TRACE="a,b": each sample of the window, one line a sample ("trace" and the values)
            for (std::size_t n = 0; !k.traces.empty() && n < k.traces[0].size(); ++n)
            {
                std::printf("trace");
                for (const auto& tr : k.traces)
                    std::printf(" %.6g", tr[n]);
                std::printf("\n");
            }
            std::printf("end\n");
            std::fflush(stdout);
        }
    keepPlayer(false);
    SUCCEED();
}

// The two loaders against each other (hidden): every Spec number after loadWhole and after the older loadBuild; the
// fields they differ in are printed (the older one drops nested sections: those should be all that differ).
TEST_CASE("probe: the whole loader against loadBuild", "[.balance-loader]")
{
    const char* b = std::getenv("BAL_BUILD");
    const std::string slug = b ? b : "four-cam-v8-50";
    const std::filesystem::path file = std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "archetypes" / (slug + ".json");
    tee::EnginePlayer::Options po;
    po.directory = std::filesystem::path(TEE_SOURCE_DIR) / "sim";
    po.sampleRate = 96000.0;
    std::string error, warnings;
    auto p = tee::EnginePlayer::create(po, error);
    REQUIRE(p);
    tee::EngineBuild eb;
    REQUIRE(tee::loadBuild(*p, file, eb, error, warnings));
    tee::applyBuild(*p, eb);
    std::vector<double> before;
    for (std::size_t i = 0; i < p->specFields().size(); ++i)
        for (std::size_t k = 0; k < p->specFields()[i].count; ++k)
            before.push_back(p->spec(i, k));
    p->reset(96000.0);
    REQUIRE(tee::balance::loadWhole(*p, file, error));
    std::size_t at = 0;
    int differ = 0;
    for (std::size_t i = 0; i < p->specFields().size(); ++i)
        for (std::size_t k = 0; k < p->specFields()[i].count; ++k, ++at)
            if (p->spec(i, k) != before[at])
            {
                if (++differ <= 40)
                    std::printf("%s[%zu]: loadBuild %g, whole %g\n", p->specFields()[i].path.c_str(), k, before[at], p->spec(i, k));
            }
    std::printf("%d numbers differ\n", differ);
    SUCCEED();
}
