// tests/sim/calibration/balance/Balance.h: card N3's air and energy balance of an engine on the engine dyno.
//
// An archetype is started as tee-dyno starts it (the key on, the starter at 0.25 s, released at 1 s, the throttle
// open), held at full throttle and a fixed speed with the warm engine's temperatures held (tee/Warm.h) and the
// exhaust's and the intake's heat settled twice in the hold (spec.<section>.heatSettle, as tee-dyno --steps settles),
// then measured over a window of whole seconds. Every cylinder's state is read each sample (one engine step a sample),
// so the books are the simulator's own to the sample:
//
//   air (kg over the window): through the throttle (Engine::airThrottle), in through the intake valves net of what
//     they pushed back (airValves), out through the exhaust valves net (gasExhaust), trapped at each intake closing
//     (the cylinder's mass, its burned part and the rest), and the air the injected fuel was metered for (each
//     firing's fuel x the fuel's stoichiometric ratio x the lambda asked, over the charge's oxygen share);
//   energy (J over the window): the injected fuel's lower heating value; the heat the mixture can release at its
//     lambda (Mixture::heat, before the burn's share); the heat released into the gas (the cylinder's burn, sample by
//     sample); the walls' heat by phase (hA (T - T_wall) dt: compression from intake closing to the spark, the burn
//     from the spark until the burn ends or the exhaust opens, expansion after it to the exhaust's opening, and the
//     gas exchange); the gas's work on the pistons, gross (the power loop) and pumping, as core/work.fm splits them;
//     the enthalpy through the exhaust and the intake valves (cp(T) T of each gas, the cylinder's own gamma(T)).
//
// The first law closes each cylinder's books over whole cycles: released - walls - work = H_out - H_in (+ the change of
// the gas's energy over the window, small over many cycles). What the probe calls the exhaust's sensible enthalpy is
// H_out less the same mass at the intake's temperature, Heywood's convention (Internal Combustion Engine Fundamentals,
// section 12.1: the exhaust's enthalpy above the inlet's).
#pragma once

#include "tee/EnginePlayer.h"
#include "tee/Warm.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace tee::balance
{
    struct Books
    {
        std::string slug;
        double rpm = 0.0, seconds = 0.0, cycles = 0.0;   // the window's mean speed, its length, and engine cycles in it
        int cylinders = 0;
        double vd = 0.0;                                   // swept volume of the engine, m^3
        double torque = 0.0;                               // brake, N m (the dyno's mean)
        // air, kg over the window
        double airThrottle = 0.0, airValves = 0.0, gasExhaust = 0.0;
        double trappedTotal = 0.0, trappedFresh = 0.0, trappedBurned = 0.0, fuelAir = 0.0;
        int closings = 0;
        // energy, J over the window
        double fuel = 0.0;          // kg injected
        double lhv = 0.0;           // J/kg
        double fuelEnergy = 0.0;    // fuel x LHV
        double mixHeat = 0.0;       // what the mixture can release at its lambda (Mixture::heat)
        double chargedHeat = 0.0;   // what the burns were charged with (the cylinders' qTot at the closing)
        double released = 0.0;      // released into the gas
        double wallCompression = 0.0, wallBurn = 0.0, wallExpansion = 0.0, wallExchange = 0.0;
        double workGross = 0.0, workPump = 0.0;   // p dV over the power loop, and over the pumping loop (negative: taken)
        double hOut = 0.0, hIn = 0.0;             // enthalpy through the exhaust valves and the intake valves, net out
        double hOutAtInlet = 0.0;                 // the exhaust's net mass at the intake's cp T
        double mOutExhaust = 0.0;
        double uStart = 0.0, uEnd = 0.0;          // the gas's energy, all cylinders, at the window's ends
        // means over the window
        double lambda = 0.0, tAmb = 0.0, tAirHeated = 0.0, pPlenum = 0.0, tIvc = 0.0;
        double veAirTel = 0.0, veManTel = 0.0, fmepTel = 0.0, imepTel = 0.0, pmepTel = 0.0, bmepTel = 0.0;
        double sparkTel = 0.0;
        std::map<std::string, double> reads;      // Options::read's paths at the window's end
        std::vector<std::vector<double>> traces;  // Options::trace's paths, each sample of the window
        std::map<std::string, double> means;      // Options::mean's paths, averaged over the window

        double walls() const { return wallCompression + wallBurn + wallExpansion + wallExchange; }
        double brakeWork() const { return torque * 2.0 * M_PI * rpm / 60.0 * seconds; }
        // the first law's residual over the window (J): released - walls - (gross + pumping work) - (H_out - H_in) - dU
        double firstLaw() const { return released - walls() - (workGross + workPump) - (hOut - hIn) - (uEnd - uStart); }
    };

    struct Options
    {
        double rpm = 3000.0;
        double settle = 2.0;    // s at the speed before the window (tee-dyno's default step settle)
        double measure = 1.0;   // s
        double sampleRate = 96000.0;   // the engine's own rate (engine.fm): one engine step a call, no resampler
        std::vector<std::pair<std::string, double>> set;   // Spec writes after the build ("fuel.model", 1)
        std::vector<std::pair<std::string, double>> raw;   // any instance path written after the build ("spec.ecu.maps.lambda.nx")
        std::vector<std::string> read;                     // instance paths to read at the window's end
        std::vector<std::string> trace;                    // instance paths recorded each sample of the window
        std::vector<std::string> mean;                     // instance paths averaged over the window
        std::filesystem::path sim;                         // another tree's sim/ (empty: this one's)
    };

    // Plays the archetype `slug` (a file name under content/builds/archetypes/, or a path) and returns its books;
    // false with `error` when it would not load or run.
    bool measure(const std::string& slug, const Options& options, Books& out, std::string& error);

    // Loads a build file whole (tee::Build, as tee-dyno) into the player: every Spec field, sections moved.
    bool loadWhole(EnginePlayer& player, const std::filesystem::path& file, std::string& error);

    // A player kept for several measurements (one compile a process): measure() makes one if it has none.
    void keepPlayer(bool keep);
} // namespace tee::balance
