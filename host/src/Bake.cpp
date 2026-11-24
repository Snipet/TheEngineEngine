// host/src/Bake.cpp: the intake cam baked by a sweep of the fast dyno (Bake.h).
#include "tee/Bake.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <limits>

namespace tee
{
    bool bakeIntakeCam(const std::shared_ptr<Sim>& sim, const Build& build, const CamBakeOptions& options,
                       CamBakeResult& result, std::string& error, const std::atomic<bool>* cancel)
    {
        const auto t0 = std::chrono::steady_clock::now();
        result = CamBakeResult{};
        if (build.fieldOf("valve.inCentre") < 0 || build.fieldOf("valve.inPhaseRange") < 0)
        {
            error = "the program has no intake phaser to hold (spec.valve.inCentre, inPhaseRange)";
            return false;
        }
        const double range = build.get("valve.inPhaseRange");
        if (!(range > 0.0))
        {
            error = "the build's intake phaser has no range (spec.valve.inPhaseRange 0): nothing to sweep";
            return false;
        }
        result.parkedCentre = build.get("valve.inCentre");
        const double step = std::max(options.step, 1.0);
        for (double a = 0.0; a <= range + 1e-9; a += step)
            result.advances.push_back(a);

        for (const double advance : result.advances)
        {
            if (cancel != nullptr && cancel->load())
            {
                error = "cancelled";
                return false;
            }
            // the phaser held still at this advance: no range, the centreline moved earlier by it
            Build held(build.fields());   // (a copy by its file: never the original's link to a host)
            std::vector<std::string> warnings;
            if (!held.load(build.save(), BuildPart::Whole, warnings, error))
                return false;
            held.set("valve.inPhaseRange", 0.0);
            held.set("valve.inCentre", result.parkedCentre - advance);
            PredictCurve curve;
            if (!predictCurve(sim, held, options.predict, curve, error, cancel))
                return false;
            if (result.speeds.empty())
                for (const PredictPoint& p : curve.points)
                    result.speeds.push_back(p.speed);
            std::vector<double> torque, knock;
            for (const PredictPoint& p : curve.points)
            {
                torque.push_back(p.valid ? p.torque : std::numeric_limits<double>::quiet_NaN());
                knock.push_back(p.knockRetard);
            }
            result.torque.push_back(std::move(torque));
            result.knock.push_back(std::move(knock));
        }

        // at each speed the most torque among the cells that do not knock more than the parked one by the limit
        for (std::size_t s = 0; s < result.speeds.size(); ++s)
        {
            double bestT = -std::numeric_limits<double>::infinity(), bestA = 0.0;
            const double parkedKnock = result.knock.front()[s];
            for (std::size_t a = 0; a < result.advances.size(); ++a)
            {
                const double t = result.torque[a][s];
                if (!std::isfinite(t) || result.knock[a][s] > parkedKnock + options.knockLimit)
                    continue;
                if (t > bestT)
                {
                    bestT = t;
                    bestA = result.advances[a];
                }
            }
            result.best.push_back(bestA);
            result.bestTorque.push_back(std::isfinite(bestT) ? bestT : std::numeric_limits<double>::quiet_NaN());
        }
        result.wallSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        return true;
    }

    bool applyIntakeCam(Build& build, const CamBakeResult& result, std::string& error)
    {
        if (result.speeds.empty() || result.speeds.size() != result.best.size())
        {
            error = "no baked cam to apply";
            return false;
        }
        if (build.fieldOf("ecu.maps.camIn") < 0 || build.fieldOf("ecu.maps.camInAuto") < 0)
        {
            error = "the program has no ecu.maps.camIn to bake into";
            return false;
        }
        // speeds as the map's x (at most 16), the same full-load row at no load and at three times the air's pressure
        std::vector<double> x = result.speeds, best = result.best;
        if (x.size() > 16)
        {
            x.resize(16);
            best.resize(16);
        }
        const std::vector<double> y = {0.0, 3.0};
        std::vector<std::vector<double>> z(x.size(), std::vector<double>{0.0, 0.0});
        for (std::size_t i = 0; i < x.size(); ++i)
            z[i] = {best[i], best[i]};
        build.beginGesture();
        const bool ok = build.setMap("ecu.maps.camIn", x, y, z) && build.set("ecu.maps.camInAuto", 0.0);
        build.endGesture();
        if (!ok)
            error = "the baked cam does not fit ecu.maps.camIn";
        return ok;
    }
} // namespace tee
