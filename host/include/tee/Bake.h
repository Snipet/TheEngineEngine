// host/include/tee/Bake.h: maps the host calibrates by sweeping the fast dyno (Predict.h), as a maker calibrates them.
//
// The intake cam (calibration's ask, N1's method, docs/measurements/calibration/na/cam-maps.md): for each fixed
// intake advance from parked (0 to the phaser's range, 10 degrees apart), the phaser held still at it (its range 0,
// its centreline the parked one less the advance), a predicted full-load curve; at each speed the advance that gives
// the most brake torque, a cell whose knock retard rises more than knockLimit degrees over the parked one's set aside
// (a phase that wins only by knocking is wrong on another fuel). The result goes into ecu.maps.camIn (degrees of
// advance by engine speed and load) with ecu.maps.camInAuto off.
//
// What it leaves out: part load. A maker trades torque there for fuel (BSFC at a held torque) and internal EGR; not
// measured yet, so the full-load row stands for every load (the map's two rows are the same). The exhaust phaser is
// left as the build has it.
#pragma once

#include "tee/Predict.h"

#include <atomic>
#include <string>
#include <vector>

namespace tee
{
    struct CamBakeOptions
    {
        double step = 10.0;           // degrees of advance between cells, from 0 to spec.valve.inPhaseRange
        double knockLimit = 2.0;      // degrees of knock retard over the parked cell's that set a cell aside
        PredictOptions predict;       // the curves' (their range, speeds: 8 by default here)

        CamBakeOptions() { predict.points = 8; predict.coarse = 8; }
    };

    struct CamBakeResult
    {
        std::vector<double> speeds;                  // rad/s
        std::vector<double> advances;                // degrees from parked, the cells swept
        std::vector<std::vector<double>> torque;     // [advance][speed], N m (NaN: no measure)
        std::vector<std::vector<double>> knock;      // [advance][speed], degrees of retard
        std::vector<double> best;                    // degrees of advance chosen at each speed
        std::vector<double> bestTorque;              // N m there
        double parkedCentre = 0.0;                   // the build's intake centreline (degrees after top), parked
        double wallSeconds = 0.0;
    };

    // Sweeps `build`'s intake cam (above). False, with `error`, when the program or the build has no intake phaser to
    // hold (spec.valve.inCentre, inPhaseRange; a build whose phaser range is 0 has nothing to sweep) or a curve fails.
    bool bakeIntakeCam(const std::shared_ptr<Sim>& sim, const Build& build, const CamBakeOptions& options,
                       CamBakeResult& result, std::string& error, const std::atomic<bool>* cancel = nullptr);

    // Writes the result into `build`: ecu.maps.camIn over the swept speeds, the same row at no load and at three times
    // the air's pressure, and ecu.maps.camInAuto off. One undo step.
    bool applyIntakeCam(Build& build, const CamBakeResult& result, std::string& error);

    // ---- the knock limit ------------------------------------------------------------------------------------------
    //
    // The auto spark map asks best-torque timing at every load; where that knocks, knock control retards it cycle
    // after cycle (a sawtooth: a step a knock, given back at some 2 degrees a second). A maker calibrates the limit
    // instead: ecu.maps.knockLimit (card N2's), degrees taken from the charge's best-torque spark by speed (x, rad/s)
    // and load (y, the manifold's pressure in atmospheres), knock control then trimming only what the margin misses.
    // Baked once for a build (the lead's ruling: a table that learned as it ran would make the engine's output depend
    // on where it had been): at each speed and pedal the fast dyno's settled point (Predict.h; a knock-limited one is
    // the mean over whole teeth of the sawtooth), the cell its mean retard plus `margin` degrees, or 0 where the mean is
    // under knockLimit (a point that does not knock at best torque keeps best torque). Pedals from full down; at a speed
    // whose cell at a pedal is 0 the lower pedals are not run (below a speed's lowest knocking load the cells are 0).
    // Each speed's cells, at the loads its pedals gave, are put on the table's loads by straight lines, flat past the
    // last measured (those cells said: `extrapolated`). Keyed (knockLimitKey) by the fields that move the knock limit
    // and the program, so a build knows when its bake is stale.
    struct KnockBakeOptions
    {
        std::vector<double> pedals = {1.0, 0.75, 0.5, 0.25};   // full first
        int speeds = 8;               // over PredictOptions' from and to (0: 1000 rpm, the redline less 100)
        int loads = 8;                // the table's loads: from 0.2 atm to the most a pedal gave, evenly
        double margin = 2.0;          // degrees over the mean retard
        double knockLimit = 0.5;      // degrees: a mean retard under it is no knock
        PredictOptions predict;       // the points' (fields: engine.manifold is added)
    };

    struct KnockBakeCell
    {
        double pedal = 0.0;
        double load = 0.0;            // atm, the manifold's pressure the point gave
        double retardMean = 0.0;      // degrees
        double retardTooth = 0.0;     // the most less the mean: the sawtooth's size
        double torque = 0.0;          // N m
        bool knockLimited = false;
        bool valid = true;
    };

    struct KnockBakeResult
    {
        std::vector<double> speeds;                       // rad/s: the table's x
        std::vector<double> loads;                        // atm: the table's y
        std::vector<std::vector<KnockBakeCell>> measured; // [speed]: the pedals run there, full first
        std::vector<std::vector<double>> limit;           // [speed][load]: degrees from best torque
        std::vector<std::vector<bool>> extrapolated;      // [speed][load]: flat past the measured loads
        std::int64_t key = 0;                             // knockLimitKey(build, sim) when it was baked
        int points = 0;                                   // fast-dyno points run
        double simulatedSeconds = 0.0;
        double wallSeconds = 0.0;
    };

    // The fields that move the knock limit (card N2's list): the engine's compression, displacement, bore and stroke,
    // cylinders and layout; the fuel's kind, grade and model; the boost section; the boost, lambda, cam and spark maps
    // with their Auto switches; the cams' centres and phase ranges; the exhaust's and the intake's models; and the
    // program itself (Sim::sourceHash). Not the air's temperature (baked at the cell's 300 K) nor ecu.timing (the
    // person's offset, on top). A 31-bit number, as spec.ecu.maps.knockLimitKey holds it.
    std::int64_t knockLimitKey(const Build& build, const Sim& sim);
    // Whether the build's bake is in use and current: knockLimitAuto off and its key this build's (false when the
    // program has no knock-limit table).
    bool knockLimitCurrent(const Build& build, const Sim& sim);

    // Bakes `build`'s knock limit (above). False, with `error`, when a sweep fails or is cancelled.
    bool bakeKnockLimit(const std::shared_ptr<Sim>& sim, const Build& build, const KnockBakeOptions& options,
                        KnockBakeResult& result, std::string& error, const std::atomic<bool>* cancel = nullptr);
    // Writes it: ecu.maps.knockLimit, knockLimitAuto off, knockLimitKey its key. One undo step. False when the program
    // has no knock-limit table.
    bool applyKnockLimit(Build& build, const KnockBakeResult& result, std::string& error);
} // namespace tee
