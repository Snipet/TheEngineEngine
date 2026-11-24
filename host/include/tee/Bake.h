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
} // namespace tee
