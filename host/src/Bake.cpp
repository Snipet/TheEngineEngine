// host/src/Bake.cpp: the intake cam baked by a sweep of the fast dyno (Bake.h).
#include "tee/Bake.h"

#include <nlohmann/json.hpp>
#include "tee/SimHost.h"
#include "tee/Timeline.h"
#include "tee/Warm.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <cstring>
#include <limits>

namespace tee
{
    namespace
    {
        // The fields that move the intake cam's best advance (Bake.h, intakeCamKey): the valvetrain whole (lobes,
        // centres, phaser ranges, the stages and their lobes), the intake, the exhaust, the boost, the engine's
        // geometry, the fuel. Hidden fields never (the heat-settle counters the dynos bump, the keys); the ECU's maps
        // never (the cam map is the result; the spark is carried by knockLimitBaseKey). Over-including costs a bake,
        // never a wrong map
        bool movesCam(const FieldInfo& f)
        {
            if (f.hidden || f.block != "spec")
                return false;
            for (const char* prefix : {"spec.valve.", "spec.intake.", "spec.exhaust.", "spec.boost.", "spec.engine."})
                if (f.path.rfind(prefix, 0) == 0)
                    return true;
            return f.path == "spec.fuel.kind" || f.path == "spec.fuel.grade";
        }
    } // namespace

    std::int64_t intakeCamKey(const Build& build, const Sim& sim)
    {
        std::uint64_t h = 1469598103934665603ull;
        const auto mixIn = [&h](const void* data, std::size_t n) {
            const unsigned char* b = static_cast<const unsigned char*>(data);
            for (std::size_t i = 0; i < n; ++i)
            {
                h ^= b[i];
                h *= 1099511628211ull;
            }
        };
        const std::uint64_t program = sim.sourceHash();
        mixIn(&program, sizeof program);
        const auto& all = build.fields().all();
        for (std::size_t i = 0; i < all.size(); ++i)
        {
            if (!movesCam(all[i]))
                continue;
            mixIn(all[i].path.data(), all[i].path.size());
            for (const double v : build.values(static_cast<int>(i)))
                mixIn(&v, sizeof v);
        }
        // the spark the cam was chosen on (C's: the knock key without the cam maps, so a re-spark after the cam leaves
        // the cam current and the two bakes end)
        const std::int64_t spark = knockLimitBaseKey(build, sim);
        mixIn(&spark, sizeof spark);
        return static_cast<std::int64_t>(h & 0x7fffffffull);
    }

    bool intakeCamCurrent(const Build& build, const Sim& sim)
    {
        if (build.fieldOf("ecu.maps.camIn") < 0 || build.fieldOf("ecu.maps.camInAuto") < 0
            || build.fieldOf("ecu.maps.camInKey") < 0)
            return false;
        return build.get("ecu.maps.camInAuto") == 0.0
               && static_cast<std::int64_t>(build.get("ecu.maps.camInKey")) == intakeCamKey(build, sim);
    }

    bool bakeIntakeCam(const std::shared_ptr<Sim>& sim, const Build& build, const CamBakeOptions& options,
                       CamBakeResult& result, std::string& error, const std::atomic<bool>* cancel)
    {
        const auto t0 = std::chrono::steady_clock::now();
        result = CamBakeResult{};
        result.key = intakeCamKey(build, *sim);
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

        // a curve with the phaser held at `advance`, at `speeds` (empty: the options' own)
        auto heldCurve = [&](double advance, const std::vector<double>& speeds, PredictCurve& curve) {
            Build held(build.fields());
            std::vector<std::string> warnings;
            if (!held.load(build.save(), BuildPart::Whole, warnings, error))
                return false;
            held.set("valve.inPhaseRange", 0.0);
            held.set("valve.inCentre", result.parkedCentre - advance);
            PredictOptions po = options.predict;
            if (!speeds.empty())
                po.speeds = speeds;
            return predictCurve(sim, held, po, curve, error, cancel);
        };
        const double nan = std::numeric_limits<double>::quiet_NaN();
        // a row at `speed` measured at every advance, inserted in order (the stage's rows)
        auto insertRows = [&](const std::vector<double>& at) {
            std::vector<std::vector<double>> tAt(result.advances.size()), kAt(result.advances.size());
            for (std::size_t a = 0; a < result.advances.size(); ++a)
            {
                PredictCurve curve;
                if (!heldCurve(result.advances[a], at, curve))
                    return false;
                for (const PredictPoint& p : curve.points)
                {
                    tAt[a].push_back(p.valid ? p.torque : nan);
                    kAt[a].push_back(p.knockRetard);
                }
            }
            for (std::size_t k = 0; k < at.size() && k < tAt.front().size(); ++k)
            {
                std::size_t pos = 0;
                while (pos < result.speeds.size() && result.speeds[pos] < at[k])
                    ++pos;
                result.speeds.insert(result.speeds.begin() + static_cast<std::ptrdiff_t>(pos), at[k]);
                for (std::size_t a = 0; a < result.advances.size(); ++a)
                {
                    result.torque[a].insert(result.torque[a].begin() + static_cast<std::ptrdiff_t>(pos), tAt[a][k]);
                    result.knock[a].insert(result.knock[a].begin() + static_cast<std::ptrdiff_t>(pos), kAt[a][k]);
                }
            }
            return true;
        };
        // A build with two-stage cams (spec.valve.stages): each steady point already runs on the lobe the engine runs
        // at that speed (the stage switches itself in the predictor as on the road), but two rows either side of the
        // change would let the map blend the two lobes' advances across it: a row just below the change back down
        // (stageSpeed less its hysteresis) and one just above the change up, so the map steps there (N3, causes.md
        // B-3: the turbo four's advance on its short lobe and on its long one differ; baked on one, run on the other,
        // -6 %)
        if (options.stageRows && build.fieldOf("valve.stages") >= 0 && build.get("valve.stages") > 0.5
            && build.fieldOf("valve.stageSpeed") >= 0 && result.speeds.size() >= 2 && result.speeds.size() <= 14)
        {
            const double up = build.get("valve.stageSpeed");
            const double hyst = build.fieldOf("valve.stageHyst") >= 0 ? build.get("valve.stageHyst") : 0.0;
            std::vector<double> at;
            for (const double w : {(up - hyst) * 0.99, up * 1.01})
                if (w > result.speeds.front() && w < result.speeds.back())
                    at.push_back(w);
            if (!at.empty())
            {
                if (!insertRows(at))
                    return false;
                result.stageRows = static_cast<int>(at.size());
            }
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
        // between the rows: the map's interpolated advance against the better of its two neighbours' (above), at each
        // interval's quarter points (a tooth narrower than the grid's spacing falls anywhere between two rows: the V12's
        // at 2000 to 2100 rpm lay between its 1165 and 2256 rpm rows, the midpoint 1710 clear of it). Every cell is swept
        // there once (one curve a cell, as the grid's); the interpolated advance's torque is read between the two cells
        // about it. Where it loses more than midLoss, the point becomes a row with its own best cell, the worst first,
        // to the map's 16 rows
        for (int pass = 0; options.refine && pass < 3 && result.speeds.size() >= 2 && result.speeds.size() < 16; ++pass)
        {
            const int addedBefore = result.added;
            struct Probe
            {
                double speed, frac;
                std::size_t lo;
            };
            std::vector<Probe> probes;
            std::vector<double> at;
            for (std::size_t s = 0; s + 1 < result.speeds.size(); ++s)
                for (const double f : {0.25, 0.5, 0.75})
                {
                    probes.push_back({result.speeds[s] + f * (result.speeds[s + 1] - result.speeds[s]), f, s});
                    at.push_back(probes.back().speed);
                }
            std::vector<std::vector<double>> tAt(result.advances.size()), kAt(result.advances.size());
            for (std::size_t a = 0; a < result.advances.size(); ++a)
            {
                if (cancel != nullptr && cancel->load())
                {
                    error = "cancelled";
                    return false;
                }
                Build held(build.fields());
                std::vector<std::string> warnings;
                if (!held.load(build.save(), BuildPart::Whole, warnings, error))
                    return false;
                held.set("valve.inPhaseRange", 0.0);
                held.set("valve.inCentre", result.parkedCentre - result.advances[a]);
                PredictOptions po = options.predict;
                po.speeds = at;
                PredictCurve curve;
                if (!predictCurve(sim, held, po, curve, error, cancel))
                    return false;
                for (const PredictPoint& p : curve.points)
                {
                    tAt[a].push_back(p.valid ? p.torque : std::numeric_limits<double>::quiet_NaN());
                    kAt[a].push_back(p.knockRetard);
                }
            }
            // the torque at advance adv at probe k: between the two cells about it
            auto torqueAt = [&](double adv, std::size_t k) {
                std::size_t i = 0;
                while (i + 1 < result.advances.size() && result.advances[i + 1] < adv)
                    ++i;
                if (i + 1 >= result.advances.size())
                    return tAt[i][k];
                const double f = (adv - result.advances[i]) / (result.advances[i + 1] - result.advances[i]);
                return tAt[i][k] + std::clamp(f, 0.0, 1.0) * (tAt[i + 1][k] - tAt[i][k]);
            };
            struct Add
            {
                double speed, loss, best, bestT;
                std::size_t k;
            };
            std::vector<Add> adds;
            const std::vector<double> gridBest = result.best;
            for (std::size_t k = 0; k < probes.size() && k < tAt.front().size(); ++k)
            {
                const Probe& pr = probes[k];
                const double aI = gridBest[pr.lo] + pr.frac * (gridBest[pr.lo + 1] - gridBest[pr.lo]);
                const double tI = torqueAt(aI, k);
                const double better = std::fmax(torqueAt(gridBest[pr.lo], k), torqueAt(gridBest[pr.lo + 1], k));
                double bestT = -std::numeric_limits<double>::infinity(), bestA = 0.0;
                for (std::size_t a = 0; a < result.advances.size(); ++a)
                {
                    const double t = tAt[a][k];
                    if (!std::isfinite(t) || kAt[a][k] > kAt.front()[k] + options.knockLimit)
                        continue;
                    if (t > bestT)
                        bestT = t, bestA = result.advances[a];
                }
                const double ref = std::fmax(better, bestT);
                if (std::isfinite(tI) && std::isfinite(ref) && tI < (1.0 - options.midLoss) * ref && std::isfinite(bestT))
                    adds.push_back({pr.speed, 1.0 - tI / ref, bestA, bestT, k});
            }
            std::sort(adds.begin(), adds.end(), [](const Add& x, const Add& y) { return x.loss > y.loss; });
            for (const Add& ad : adds)
            {
                if (result.speeds.size() >= 16)
                    break;
                std::size_t pos = 0;
                while (pos < result.speeds.size() && result.speeds[pos] < ad.speed)
                    ++pos;
                result.speeds.insert(result.speeds.begin() + static_cast<std::ptrdiff_t>(pos), ad.speed);
                for (std::size_t a = 0; a < result.advances.size(); ++a)
                {
                    result.torque[a].insert(result.torque[a].begin() + static_cast<std::ptrdiff_t>(pos), tAt[a][ad.k]);
                    result.knock[a].insert(result.knock[a].begin() + static_cast<std::ptrdiff_t>(pos), kAt[a][ad.k]);
                }
                result.best.insert(result.best.begin() + static_cast<std::ptrdiff_t>(pos), ad.best);
                result.bestTorque.insert(result.bestTorque.begin() + static_cast<std::ptrdiff_t>(pos), ad.bestT);
                result.added += 1;
            }
            if (result.added == addedBefore)
                break;   // (again over the intervals the new rows made, to three passes: a row can spoil its neighbours')
        }
        // The optimum can be narrower than the grid (N3's turbo four at 1800 rpm on its short exhaust lobe: 28 degrees
        // 302 N m, 35 degrees 319, 50 degrees 220): at each speed the half steps either side of its best cell measured
        // too, a curve for each such advance over just the speeds whose best is beside it, and the best taken again
        // over all
        if (options.halfSteps && result.advances.size() >= 2)
        {
            const double h = 0.5 * step;
            std::vector<double> extra;
            for (std::size_t sp = 0; sp < result.speeds.size(); ++sp)
                for (const double c : {result.best[sp] - h, result.best[sp] + h})
                    if (c > 0.0 && c < range && std::find(extra.begin(), extra.end(), c) == extra.end())
                        extra.push_back(c);
            std::sort(extra.begin(), extra.end());
            for (const double c : extra)
            {
                if (cancel != nullptr && cancel->load())
                {
                    error = "cancelled";
                    return false;
                }
                std::vector<double> at;
                std::vector<std::size_t> where;
                for (std::size_t sp = 0; sp < result.speeds.size(); ++sp)
                    if (std::fabs(result.best[sp] - c) < h + 1e-9)
                    {
                        at.push_back(result.speeds[sp]);
                        where.push_back(sp);
                    }
                PredictCurve curve;
                if (!heldCurve(c, at, curve))
                    return false;
                std::vector<double> torque(result.speeds.size(), nan), knock(result.speeds.size(), nan);
                for (std::size_t k = 0; k < curve.points.size() && k < where.size(); ++k)
                {
                    torque[where[k]] = curve.points[k].valid ? curve.points[k].torque : nan;
                    knock[where[k]] = curve.points[k].knockRetard;
                }
                std::size_t pos = 0;
                while (pos < result.advances.size() && result.advances[pos] < c)
                    ++pos;
                result.advances.insert(result.advances.begin() + static_cast<std::ptrdiff_t>(pos), c);
                result.torque.insert(result.torque.begin() + static_cast<std::ptrdiff_t>(pos), std::move(torque));
                result.knock.insert(result.knock.begin() + static_cast<std::ptrdiff_t>(pos), std::move(knock));
                result.halfSteps += static_cast<int>(at.size());
            }
            for (std::size_t sp = 0; sp < result.speeds.size(); ++sp)
            {
                const double parkedKnock = result.knock.front()[sp];
                for (std::size_t a = 0; a < result.advances.size(); ++a)
                {
                    const double t = result.torque[a][sp];
                    if (!std::isfinite(t) || !(result.knock[a][sp] <= parkedKnock + options.knockLimit))
                        continue;
                    if (t > result.bestTorque[sp] || !std::isfinite(result.bestTorque[sp]))
                    {
                        result.bestTorque[sp] = t;
                        result.best[sp] = result.advances[a];
                    }
                }
            }
        }

        // smooth across speed: the advances chosen together, the most torque less a cost for each step between
        // neighbouring rows (stepCost of the best torque per 10 degrees: judgement), so the map does not swing back and
        // forth between cells chasing the waves' teeth (a turbo four's grid bake read 20, 20, 0, 30, 0, 20, 30, 20; the
        // lead's 22:35). Cells set aside for knock stay set aside
        if (options.stepCost > 0.0 && result.speeds.size() >= 2)
        {
            const std::size_t ns = result.speeds.size(), na = result.advances.size();
            double tMax = 0.0;
            for (std::size_t s = 0; s < ns; ++s)
                if (std::isfinite(result.bestTorque[s]))
                    tMax = std::fmax(tMax, result.bestTorque[s]);
            const double cost = options.stepCost * tMax / 10.0;
            const double bad = -std::numeric_limits<double>::infinity();
            auto ok = [&](std::size_t a, std::size_t s) {
                return std::isfinite(result.torque[a][s]) && result.knock[a][s] <= result.knock.front()[s] + options.knockLimit;
            };
            std::vector<std::vector<double>> score(ns, std::vector<double>(na, bad));
            std::vector<std::vector<std::size_t>> from(ns, std::vector<std::size_t>(na, 0));
            for (std::size_t a = 0; a < na; ++a)
                if (ok(a, 0))
                    score[0][a] = result.torque[a][0];
            for (std::size_t s = 1; s < ns; ++s)
                for (std::size_t a = 0; a < na; ++a)
                {
                    if (!ok(a, s))
                        continue;
                    for (std::size_t b = 0; b < na; ++b)
                    {
                        if (score[s - 1][b] == bad)
                            continue;
                        const double v = score[s - 1][b] + result.torque[a][s]
                                         - cost * std::fabs(result.advances[a] - result.advances[b]);
                        if (v > score[s][a])
                            score[s][a] = v, from[s][a] = b;
                    }
                }
            std::size_t last = 0;
            for (std::size_t a = 1; a < na; ++a)
                if (score[ns - 1][a] > score[ns - 1][last])
                    last = a;
            if (score[ns - 1][last] != bad)
                for (std::size_t s = ns; s-- > 0;)
                {
                    result.best[s] = result.advances[last];
                    result.bestTorque[s] = result.torque[last][s];
                    last = from[s][last];
                }
        }
        // acceptance: the map applied against the auto schedule at full load, at the map's own speeds; where it makes more
        // than acceptLoss less (the knock control's state moves a settled point by up to some 3 %: card N1), that row
        // takes the auto schedule's own phase there. The idle and light load are parked by the map's load axis
        // (applyIntakeCam), as the auto schedule is at idle (the lead's 22:35: the bake's own acceptance before it writes)
        if (options.accept && !result.speeds.empty() && build.fieldOf("ecu.maps.camInAuto") >= 0)
        {
            Build autoB(build.fields());
            std::vector<std::string> warnings;
            if (!autoB.load(build.save(), BuildPart::Whole, warnings, error))
                return false;
            autoB.set("ecu.maps.camInAuto", 1.0);
            Build mapB(build.fields());
            if (!mapB.load(build.save(), BuildPart::Whole, warnings, error) || !applyIntakeCam(mapB, result, error))
                return false;
            PredictOptions po = options.predict;
            po.speeds = result.speeds;
            po.fields = {"valve.inPhaseTarget"};
            PredictCurve autoC, mapC;
            if (!predictCurve(sim, autoB, po, autoC, error, cancel) || !predictCurve(sim, mapB, po, mapC, error, cancel))
                return false;
            for (std::size_t s = 0; s < result.speeds.size() && s < autoC.points.size() && s < mapC.points.size(); ++s)
            {
                const PredictPoint& pa = autoC.points[s];
                const PredictPoint& pm = mapC.points[s];
                if (!pa.valid || !pm.valid || pa.torque <= 0.0)
                    continue;
                if (pm.torque < (1.0 - options.acceptLoss) * pa.torque && !pa.fields.empty())
                {
                    result.best[s] = pa.fields[0];
                    result.bestTorque[s] = pa.torque;
                    result.fellBack += 1;
                }
            }
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
        // speeds as the map's x (at most 16)
        std::vector<double> x = result.speeds, best = result.best;
        if (x.size() > 16)
        {
            x.resize(16);
            best.resize(16);
        }
        // the load axis (the manifold over the air's): the bake measures full load only, so the map asks its advance at
        // full load and none (parked: the least overlap, as the auto schedule and a maker's map at idle and light load;
        // idle is the most throttled condition and the most residual, the burn's least stable: SAE 2004-01-1356 counts it
        // stable under 3 % CoV of IMEP) up to 0.75, the advance coming in by 0.95. Before: one row for every load asked
        // the full-load phase at idle, and a flat-plane V8 idled at 95 kPa, sd 108 rpm (the lead's 22:35)
        const std::vector<double> y = {0.0, 0.75, 0.95, 3.0};
        std::vector<std::vector<double>> z(x.size(), std::vector<double>{0.0, 0.0, 0.0, 0.0});
        for (std::size_t i = 0; i < x.size(); ++i)
            z[i] = {0.0, 0.0, best[i], best[i]};
        build.beginGesture();
        const bool ok = build.setMap("ecu.maps.camIn", x, y, z) && build.set("ecu.maps.camInAuto", 0.0)
                        && (build.fieldOf("ecu.maps.camInKey") < 0
                            || build.set("ecu.maps.camInKey", static_cast<double>(result.key)));
        build.endGesture();
        if (!ok)
            error = "the baked cam does not fit ecu.maps.camIn";
        return ok;
    }

    bool checkIdle(const std::shared_ptr<Sim>& sim, const Build& build, const std::filesystem::path& scenarios,
                   IdleCheck& result, std::string& error)
    {
        constexpr double kRpm = 60.0 / (2.0 * 3.14159265358979323846);
        result = IdleCheck{};
        if (build.fieldOf("ecu.idle.speed") < 0)
        {
            error = "the program has no ecu.idle.speed";
            return false;
        }
        result.asked = build.get("ecu.idle.speed") * kRpm;
        struct Case
        {
            const char* name;
            double from, to;
        };
        for (const Case& c : {Case{"p-idle-warm", 8.0, 14.0}, Case{"p-cold-start", 8.0, 11.5}})
        {
            Timeline timeline;
            if (!loadTimeline(scenarios / (std::string(c.name) + ".json"), timeline, error))
                return false;
            SimHost host;
            LoadOptions load;
            load.sim = sim->options();
            load.sampleRate = timeline.rate;
            load.maxBlock = std::max(16, timeline.block);
            load.worker = false;
            if (timeline.warm)
                load.hold = warmEngine();
            if (!host.attach(sim, load, error))
                return false;
            Build copy(sim->fields());
            std::vector<std::string> warnings;
            if (!copy.load(build.save(), BuildPart::Whole, warnings, error))
                return false;
            copy.attach(&host);
            RunOptions options;
            options.keepAudio = false;
            options.telemetry = {"engine.speed", "engine.state"};
            RunResult run;
            if (!runTimeline(host, copy, timeline, options, run, error))
                return false;
            const int speed = run.column("engine.speed"), state = run.column("engine.state");
            if (speed < 0 || state < 0)
            {
                error = "the idle check cannot read engine.speed and engine.state";
                return false;
            }
            double sum = 0.0, squares = 0.0, lowest = 1e9;
            std::size_t n = 0;
            for (std::size_t r = 0; r < run.rowCount(); ++r)
                if (run.rowTime[r] >= c.from && run.rowTime[r] <= c.to)
                {
                    const double rpm = run.at(r, static_cast<std::size_t>(speed)) * kRpm;
                    sum += rpm;
                    squares += rpm * rpm;
                    lowest = std::min(lowest, static_cast<double>(run.at(r, static_cast<std::size_t>(state))));
                    ++n;
                }
            const double mean = n > 0 ? sum / static_cast<double>(n) : 0.0;
            const double sd = n > 0 ? std::sqrt(std::max(0.0, squares / static_cast<double>(n) - mean * mean)) : 0.0;
            char line[160];
            if (c.from == 8.0 && c.to == 14.0)
            {
                result.warmMean = mean;
                result.warmSd = sd;
                if (n == 0 || std::fabs(mean - result.asked) > 100.0 || sd > 25.0)
                {
                    std::snprintf(line, sizeof line, "warm idle %.0f rpm (asked %.0f), sd %.0f", mean, result.asked, sd);
                    result.failures.push_back(line);
                }
            }
            else
            {
                result.coldMean = mean;
                result.coldSd = sd;
                result.coldRunning = n > 0 && lowest >= 2.0;
                if (!result.coldRunning || sd > 60.0)
                {
                    std::snprintf(line, sizeof line, "cold idle %.0f rpm, sd %.0f, %s", mean, sd,
                                  result.coldRunning ? "running" : "NOT running");
                    result.failures.push_back(line);
                }
            }
        }
        result.pass = result.failures.empty();
        return true;
    }

    // ---- the knock limit ----------------------------------------------------------------------------------------------

    namespace
    {
        // The fields that move the knock limit (Bake.h): whole paths, and the boost's section whole
        const char* const kKnockFields[] = {
            "spec.engine.compression", "spec.engine.displacement", "spec.engine.boreStroke", "spec.engine.cylinders",
            "spec.engine.layout",      "spec.fuel.kind",           "spec.fuel.grade",       "spec.fuel.model",
            "spec.exhaust.model",      "spec.intake.model",        "spec.valve.inCentre",   "spec.valve.exCentre",
            "spec.valve.inPhaseRange", "spec.valve.exPhaseRange",  "spec.ecu.maps.boost",   "spec.ecu.maps.boostAuto",
            "spec.ecu.maps.lambda",    "spec.ecu.maps.lambdaAuto", "spec.ecu.maps.camIn",   "spec.ecu.maps.camInAuto",
            "spec.ecu.maps.camEx",     "spec.ecu.maps.camExAuto",  "spec.ecu.maps.spark",   "spec.ecu.maps.sparkAuto"};

        // The intake and exhaust cam maps: in the knock limit's key, not in its base key (Bake.h)
        bool camMap(const std::string& path)
        {
            return path == "spec.ecu.maps.camIn" || path == "spec.ecu.maps.camInAuto" || path == "spec.ecu.maps.camEx"
                   || path == "spec.ecu.maps.camExAuto";
        }

        bool movesKnock(const FieldInfo& f)
        {
            if (f.hidden)
                return false;
            if (f.path.rfind("spec.boost.", 0) == 0)
                return true;
            for (const char* k : kKnockFields)
                if (f.path == k)
                    return true;
            return false;
        }
    } // namespace

    namespace
    {
        std::int64_t knockKeyOf(const Build& build, const Sim& sim, bool withCams)
        {
            std::uint64_t h = 1469598103934665603ull;
            const auto mixIn = [&h](const void* data, std::size_t n) {
                const unsigned char* b = static_cast<const unsigned char*>(data);
                for (std::size_t i = 0; i < n; ++i)
                {
                    h ^= b[i];
                    h *= 1099511628211ull;
                }
            };
            const std::uint64_t program = sim.sourceHash();
            mixIn(&program, sizeof program);
            const int version = kKnockBakeVersion;
            mixIn(&version, sizeof version);
            const auto& all = build.fields().all();
            for (std::size_t i = 0; i < all.size(); ++i)
            {
                if (all[i].block != "spec" || !movesKnock(all[i]) || (!withCams && camMap(all[i].path)))
                    continue;
                mixIn(all[i].path.data(), all[i].path.size());
                for (const double v : build.values(static_cast<int>(i)))
                    mixIn(&v, sizeof v);
            }
            return static_cast<std::int64_t>(h & 0x7fffffffull);
        }
    } // namespace

    std::int64_t knockLimitKey(const Build& build, const Sim& sim)
    {
        return knockKeyOf(build, sim, true);
    }

    std::int64_t knockLimitBaseKey(const Build& build, const Sim& sim)
    {
        return knockKeyOf(build, sim, false);
    }

    bool knockLimitCurrent(const Build& build, const Sim& sim)
    {
        if (build.fieldOf("ecu.maps.knockLimit") < 0 || build.fieldOf("ecu.maps.knockLimitAuto") < 0
            || build.fieldOf("ecu.maps.knockLimitKey") < 0)
            return false;
        return build.get("ecu.maps.knockLimitAuto") == 0.0
               && static_cast<std::int64_t>(build.get("ecu.maps.knockLimitKey")) == knockLimitKey(build, sim);
    }

    bool bakeKnockLimit(const std::shared_ptr<Sim>& sim, const Build& build, const KnockBakeOptions& options,
                        KnockBakeResult& result, std::string& error, const std::atomic<bool>* cancel)
    {
        const auto t0 = std::chrono::steady_clock::now();
        result = KnockBakeResult{};
        if (options.pedals.empty() || options.speeds < 2)
        {
            error = "a knock-limit bake wants a pedal and two speeds at least";
            return false;
        }
        PredictOptions predict = options.predict;
        predict.fields.insert(predict.fields.begin(), "engine.manifold");   // the load
        predict.points = predict.coarse = std::min(options.speeds, 16);

        // the steps, from the most load down: a turbo's boost targets at full pedal, then the pedals under full
        struct Step
        {
            double pedal, boost;   // boost: spec.boost.target, Pa over the air; < 0: as built
        };
        std::vector<Step> steps;
        const double kind = build.fieldOf("boost.kind") >= 0 ? build.get("boost.kind") : 0.0;
        const bool turbo = kind >= 1.0 && kind <= 3.0 && options.boostSteps > 0 && build.fieldOf("boost.target") >= 0
                           && build.fieldOf("boost.gateSpring") >= 0 && build.fieldOf("ecu.maps.boostAuto") >= 0;
        if (turbo)
        {
            const double spring = build.get("boost.gateSpring");
            double top = std::max(build.get("boost.target"), spring);
            // a build with its own boost map (boostAuto off): its map's most, which the bake's targets must reach
            if (build.get("ecu.maps.boostAuto") == 0.0)
                if (const int m = build.fieldOf("ecu.maps.boost"); m >= 0)
                {
                    const FieldInfo& f = build.fields().all()[static_cast<std::size_t>(m)];
                    const std::vector<double>& v = build.values(m);
                    std::size_t z = 0, at = 0, nx = 0, ny = 0;
                    for (const FieldInfo::Leaf& leaf : f.leaves)
                    {
                        if (leaf.name == "z")
                            z = at;
                        else if (leaf.name == "nx" && at < v.size())
                            nx = static_cast<std::size_t>(v[at]);
                        else if (leaf.name == "ny" && at < v.size())
                            ny = static_cast<std::size_t>(v[at]);
                        at += leaf.count;
                    }
                    for (std::size_t i = 0; i < nx && i < 16; ++i)
                        for (std::size_t j = 0; j < ny && j < 16; ++j)
                            if (z + i * 16 + j < v.size())
                                top = std::max(top, v[z + i * 16 + j]);
                }
            const int n = std::max(options.boostSteps, 1);
            for (int i = 0; i < n; ++i)
                steps.push_back({1.0, n == 1 ? top : top - (top - spring) * i / (n - 1)});
            for (const double p : options.pedals)
                if (p < 1.0)
                    steps.push_back({p, spring});   // under the spring's boost: the loads below the targets'
            char text[160];
            std::snprintf(text, sizeof text,
                          "a turbo: boost targets %.0f to %.0f kPa over the air at full pedal (boostAuto on), then pedals at the spring's",
                          top / 1000.0, spring / 1000.0);
            result.notes = text;
        }
        else
        {
            for (const double p : options.pedals)
                steps.push_back({p, -1.0});
            result.notes = kind >= 4.0 ? "a supercharger: pedals alone (its boost is its pulley's, no loop to command)"
                                       : "pedals";
        }

        // a build at step k: its pedal's predict options aside, the boost target, and a table when one is given
        const auto stepBuildOf = [&](std::size_t k, const KnockBakeResult* table, Build& out) {
            std::vector<std::string> said;
            if (!out.load(build.save(), BuildPart::Whole, said, error))
                return false;
            if (steps[k].boost >= 0.0)
            {
                out.set("ecu.maps.boostAuto", 1.0);
                out.set("boost.target", steps[k].boost);
            }
            return table == nullptr || applyKnockLimit(out, *table, error);
        };
        const auto speedIndex = [&](double w) {
            std::size_t at = 0;
            while (at + 1 < result.speeds.size() && std::fabs(result.speeds[at] - w) > 1e-6)
                ++at;
            return at;
        };

        std::vector<double> active;   // the speeds still knocking at the step above
        for (std::size_t k = 0; k < steps.size(); ++k)
        {
            if (cancel != nullptr && cancel->load())
            {
                error = "cancelled";
                return false;
            }
            predict.pedal = steps[k].pedal;
            Build stepBuild(build.fields());
            if (!stepBuildOf(k, nullptr, stepBuild))
                return false;
            if (k > 0)
            {
                if (active.empty())
                    break;
                predict.speeds = active;
                predict.points = predict.coarse = static_cast<int>(active.size());
            }
            PredictCurve curve;
            if (!predictCurve(sim, stepBuild, predict, curve, error, cancel))
                return false;
            result.points += static_cast<int>(curve.points.size());
            result.simulatedSeconds += curve.simulatedSeconds;
            if (k == 0)
            {
                for (const PredictPoint& p : curve.points)
                    result.speeds.push_back(p.speed);
                result.measured.assign(result.speeds.size(), {});
            }
            std::vector<double> next;
            for (const PredictPoint& p : curve.points)
            {
                KnockBakeCell cell;
                cell.pedal = predict.pedal;
                cell.boostTarget = std::max(steps[k].boost, 0.0);
                cell.load = p.fields.empty() ? 0.0 : p.fields[0] / 101325.0;
                cell.retardMean = p.retardMean;
                cell.retardTooth = std::max(p.retardMax - p.retardMean, 0.0);
                cell.torque = p.torque;
                cell.knockLimited = p.knockLimited;
                cell.valid = p.valid && !p.limited;   // the limiter acted: no measure, as missing
                cell.limit = p.retardMean >= options.knockLimit ? p.retardMean + options.margin : 0.0;
                cell.step = static_cast<int>(k);
                result.measured[speedIndex(p.speed)].push_back(cell);
                if (p.valid && p.retardMean >= options.knockLimit)
                    next.push_back(p.speed);   // still knocking: the next pedal down is run here
            }
            active = next;
        }

        // the table's loads, and each speed's cells put on them
        double most = 0.3;
        for (const auto& column : result.measured)
            for (const KnockBakeCell& c : column)
                if (c.valid)
                    most = std::max(most, c.load);
        const int ny = std::clamp(options.loads, 2, 16);
        for (int j = 0; j < ny; ++j)
            result.loads.push_back(0.2 + (most - 0.2) * j / (ny - 1));
        const auto tabulate = [&] {
            result.limit.clear();
            result.extrapolated.clear();
            result.stillKnocking.clear();
            for (const auto& column : result.measured)
            {
                struct At
                {
                    double load, limit;
                    bool knocking;
                };
                std::vector<At> at;
                for (const KnockBakeCell& c : column)
                    if (c.valid)
                        at.push_back({c.load, c.limit, c.stillKnocking});
                std::sort(at.begin(), at.end(), [](const At& a, const At& b) { return a.load < b.load; });
                std::vector<double> row;
                std::vector<bool> flat, knocking;
                for (const double y : result.loads)
                {
                    if (at.empty())
                    {
                        row.push_back(0.0);
                        flat.push_back(true);
                        knocking.push_back(false);
                    }
                    else if (y <= at.front().load)
                    {
                        row.push_back(at.front().limit);
                        flat.push_back(at.front().limit != 0.0 && y < at.front().load);   // under a 0: 0 by the rule
                        knocking.push_back(at.front().knocking);
                    }
                    else if (y >= at.back().load)
                    {
                        row.push_back(at.back().limit);
                        flat.push_back(y > at.back().load);
                        knocking.push_back(at.back().knocking);
                    }
                    else
                    {
                        std::size_t i = 1;
                        while (at[i].load < y)
                            ++i;
                        const double u = (y - at[i - 1].load) / std::max(at[i].load - at[i - 1].load, 1e-9);
                        row.push_back(at[i - 1].limit + u * (at[i].limit - at[i - 1].limit));
                        flat.push_back(false);
                        knocking.push_back(at[i - 1].knocking || at[i].knocking);
                    }
                }
                result.limit.push_back(std::move(row));
                result.extrapolated.push_back(std::move(flat));
                result.stillKnocking.push_back(std::move(knocking));
            }
        };
        tabulate();

        // the corrections: a cell whose mean was over correctAbove measured again with the table applied, its residual
        // added, at most `corrections` times (a program without the table: none)
        const bool canApply = build.fieldOf("ecu.maps.knockLimit") >= 0 && build.fieldOf("ecu.maps.knockLimitAuto") >= 0
                              && build.fieldOf("ecu.maps.knockLimitKey") >= 0;
        for (int pass = 0; canApply && pass < options.corrections; ++pass)
        {
            // the cells to measure again, by step: their speeds
            std::vector<std::vector<double>> again(steps.size());
            for (std::size_t s2 = 0; s2 < result.measured.size(); ++s2)
                for (const KnockBakeCell& c : result.measured[s2])
                    if (c.valid && c.retardMean > options.correctAbove
                        && (c.corrections == 0 || c.residual >= options.knockLimit))
                        again[static_cast<std::size_t>(c.step)].push_back(result.speeds[s2]);
            bool any = false;
            for (std::size_t k = 0; k < steps.size(); ++k)
            {
                if (again[k].empty())
                    continue;
                if (cancel != nullptr && cancel->load())
                {
                    error = "cancelled";
                    return false;
                }
                any = true;
                Build stepBuild(build.fields());
                if (!stepBuildOf(k, &result, stepBuild))
                    return false;
                predict.pedal = steps[k].pedal;
                predict.speeds = again[k];
                predict.points = predict.coarse = static_cast<int>(again[k].size());
                PredictCurve curve;
                if (!predictCurve(sim, stepBuild, predict, curve, error, cancel))
                    return false;
                result.points += static_cast<int>(curve.points.size());
                result.simulatedSeconds += curve.simulatedSeconds;
                for (const PredictPoint& p : curve.points)
                    for (KnockBakeCell& c : result.measured[speedIndex(p.speed)])
                        if (c.step == static_cast<int>(k))
                        {
                            ++c.corrections;
                            c.residual = p.valid && !p.limited ? p.retardMean : 0.0;
                            if (c.residual >= options.knockLimit)
                                c.limit += c.residual;
                        }
            }
            if (!any)
                break;
            tabulate();
        }
        for (auto& column : result.measured)
            for (KnockBakeCell& c : column)
                c.stillKnocking = c.corrections > 0 ? c.residual >= options.knockLimit
                                                    : (canApply ? false : c.retardMean > options.correctAbove);
        // (a cell measured again whose last residual still knocks; a program without the table could not correct)
        tabulate();

        result.key = knockLimitKey(build, *sim);
        result.wallSeconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
        return true;
    }

    bool applyKnockLimit(Build& build, const KnockBakeResult& result, std::string& error)
    {
        if (build.fieldOf("ecu.maps.knockLimit") < 0 || build.fieldOf("ecu.maps.knockLimitAuto") < 0
            || build.fieldOf("ecu.maps.knockLimitKey") < 0)
        {
            error = "the program has no knock-limit table (spec.ecu.maps.knockLimit, knockLimitAuto, knockLimitKey)";
            return false;
        }
        if (result.speeds.empty() || result.limit.size() != result.speeds.size())
        {
            error = "no baked knock limit to apply";
            return false;
        }
        std::vector<double> x = result.speeds, y = result.loads;
        std::vector<std::vector<double>> z = result.limit;
        if (x.size() > 16)
        {
            x.resize(16);
            z.resize(16);
        }
        build.beginGesture();
        const bool ok = build.setMap("ecu.maps.knockLimit", x, y, z) && build.set("ecu.maps.knockLimitAuto", 0.0)
                        && build.set("ecu.maps.knockLimitKey", static_cast<double>(result.key));
        build.endGesture();
        if (!ok)
            error = "the baked knock limit does not fit ecu.maps.knockLimit";
        return ok;
    }

    // ---- the background bake ------------------------------------------------------------------------------------------

    const char* knockBakeStateName(KnockBakeState state) noexcept
    {
        switch (state)
        {
            case KnockBakeState::NoTable: return "no table";
            case KnockBakeState::Current: return "baked";
            case KnockBakeState::Waiting: return "waiting to bake";
            case KnockBakeState::Baking: return "baking";
            case KnockBakeState::Ready: return "baked, to apply";
            case KnockBakeState::Failed: return "bake failed";
        }
        return "no table";
    }

    namespace
    {
        const char* const kSparkFields[] = {"ecu.maps.knockLimit", "ecu.maps.knockLimitAuto", "ecu.maps.knockLimitKey"};
        const char* const kCamFields[] = {"ecu.maps.camIn", "ecu.maps.camInAuto", "ecu.maps.camInKey"};

        // The fields an apply wrote, as the build now holds them
        template <std::size_t N>
        BakedFields capture(const Build& build, const char* const (&paths)[N], std::int64_t key)
        {
            BakedFields out;
            out.key = key;
            for (const char* path : paths)
                if (const int f = build.fieldOf(path); f >= 0)
                    out.fields.emplace_back(path, build.values(f));
            return out;
        }

        bool applyFields(Build& build, const BakedFields& baked, std::string& error)
        {
            build.beginGesture();
            bool ok = true;
            for (const auto& [path, values] : baked.fields)
            {
                const int f = build.fieldOf(path);
                ok = ok && f >= 0 && build.setValues(f, values);
            }
            build.endGesture();
            if (!ok)
                error = "a baked table does not fit this program's fields";
            return ok;
        }

        // ---- the cache: <dir>/<kind>-<key>.json, the fields and (a spark's) the cells still knocking
        std::filesystem::path cachePath(const std::filesystem::path& dir, const char* kind, std::int64_t key)
        {
            return dir / (std::string(kind) + "-" + std::to_string(key) + ".json");
        }

        bool cacheRead(const std::filesystem::path& dir, const char* kind, std::int64_t key, BakedFields& out)
        {
            if (dir.empty())
                return false;
            const std::filesystem::path file = cachePath(dir, kind, key);
            std::ifstream in(file, std::ios::binary);
            if (!in)
                return false;
            const nlohmann::json j = nlohmann::json::parse(in, nullptr, false);
            if (j.is_discarded() || !j.is_object() || j.value("key", std::int64_t{-2}) != key || !j.contains("fields"))
                return false;
            out = BakedFields{};
            out.key = key;
            out.cached = true;
            for (const auto& f : j["fields"])
                if (f.is_array() && f.size() == 2 && f[0].is_string() && f[1].is_array())
                    out.fields.emplace_back(f[0].get<std::string>(), f[1].get<std::vector<double>>());
            if (j.contains("stillKnocking") && j["stillKnocking"].is_array())
                for (const auto& row : j["stillKnocking"])
                    out.stillKnocking.push_back(row.get<std::vector<bool>>());
            std::error_code ec;
            std::filesystem::last_write_time(file, std::filesystem::file_time_type::clock::now(), ec);   // used: kept
            return !out.fields.empty();
        }

        void cacheWrite(const std::filesystem::path& dir, std::size_t most, const char* kind, const BakedFields& baked)
        {
            if (dir.empty())
                return;
            std::error_code ec;
            std::filesystem::create_directories(dir, ec);
            nlohmann::json j;
            j["format"] = "tee-bake-cache";
            j["key"] = baked.key;
            j["fields"] = nlohmann::json::array();
            for (const auto& [path, values] : baked.fields)
                j["fields"].push_back(nlohmann::json::array({path, values}));
            j["stillKnocking"] = baked.stillKnocking;
            const std::filesystem::path file = cachePath(dir, kind, baked.key);
            const std::filesystem::path temp = file.string() + ".part";
            {
                std::ofstream out(temp, std::ios::binary);
                out << j.dump();
                if (!out)
                    return;
            }
            std::filesystem::rename(temp, file, ec);   // whole or not at all
            // the newest `most` kept
            std::vector<std::pair<std::filesystem::file_time_type, std::filesystem::path>> entries;
            for (std::filesystem::directory_iterator it(dir, ec), end; !ec && it != end; it.increment(ec))
                if (it->path().extension() == ".json")
                    entries.emplace_back(std::filesystem::last_write_time(it->path(), ec), it->path());
            if (entries.size() > most)
            {
                std::sort(entries.begin(), entries.end());
                for (std::size_t i = 0; i + most < entries.size(); ++i)
                    std::filesystem::remove(entries[i].second, ec);
            }
        }

        bool hasTable(const Build& build)
        {
            return build.fieldOf("ecu.maps.knockLimit") >= 0 && build.fieldOf("ecu.maps.knockLimitAuto") >= 0
                   && build.fieldOf("ecu.maps.knockLimitKey") >= 0;
        }

        CalibrationBakeOptions sparkOnly(KnockBakeOptions spark, double debounce)
        {
            CalibrationBakeOptions o;
            o.spark = std::move(spark);
            o.bakeCam = false;
            o.debounceSeconds = debounce;
            return o;
        }
    } // namespace

    CalibrationBakeService::CalibrationBakeService(std::shared_ptr<Sim> sim, CalibrationBakeOptions options)
        : sim_(std::move(sim)), options_(std::move(options))
    {
        options_.debounceSeconds = std::max(options_.debounceSeconds, 0.0);
        thread_ = std::thread([this] { coordinator(); });
    }

    KnockBakeService::KnockBakeService(std::shared_ptr<Sim> sim, KnockBakeOptions options, double debounceSeconds)
        : CalibrationBakeService(std::move(sim), sparkOnly(std::move(options), debounceSeconds))
    {
    }

    CalibrationBakeService::~CalibrationBakeService()
    {
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            stop_ = true;
        }
        cancel_ = true;
        wake_.notify_all();
        if (thread_.joinable())
            thread_.join();
    }

    void CalibrationBakeService::setState(KnockBakeState s)
    {
        if (state_ != s)
        {
            state_ = s;
            ++revision_;
        }
    }

    bool CalibrationBakeService::sparkWanted(const Build& build)
    {
        return hasTable(build) && !(build.fieldOf("fuel.kind") >= 0 && build.get("fuel.kind") == 1.0);   // 1: Diesel
    }

    bool CalibrationBakeService::camWanted(const Build& build) const
    {
        return options_.bakeCam && build.fieldOf("ecu.maps.camInKey") >= 0 && build.fieldOf("ecu.maps.camIn") >= 0
               && build.fieldOf("valve.inPhaseRange") >= 0 && build.get("valve.inPhaseRange") > 0.0;
    }

    KnockBakeState CalibrationBakeService::consider(Build& build)
    {
        const bool wantSpark = sparkWanted(build);
        const bool wantCam = camWanted(build);
        if (!hasTable(build))
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            setState(KnockBakeState::NoTable);
            return state_;
        }
        const bool camCurrent = !wantCam || intakeCamCurrent(build, *sim_);
        const bool sparkCurrent = !wantSpark || knockLimitCurrent(build, *sim_);
        if (sparkCurrent && camCurrent)
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            if (asked_ || state_ == KnockBakeState::Waiting || state_ == KnockBakeState::Baking)
                cancel_ = true;   // what runs is for another build: not wanted
            asked_ = false;
            ready_ = false;
            setState(KnockBakeState::Current);
            return state_;
        }
        // stale: the stale tables set aside now (the engine on its knock control and its auto cam), a bake asked
        if (wantCam && !camCurrent && build.get("ecu.maps.camInAuto") == 0.0)
            build.set("ecu.maps.camInAuto", 1.0);
        if (!sparkCurrent && build.get("ecu.maps.knockLimitAuto") == 0.0)
            build.set("ecu.maps.knockLimitAuto", 1.0);
        const std::int64_t sparkKey = wantSpark ? knockLimitKey(build, *sim_) : 0;
        const std::int64_t camKey = wantCam ? intakeCamKey(build, *sim_) : 0;
        const std::lock_guard<std::mutex> lock(mutex_);
        if (sparkKey == sparkKey_ && camKey == camKey_ && state_ != KnockBakeState::Failed
            && state_ != KnockBakeState::Current)
            return state_;   // asked already (waiting, baking, or ready to take)
        sparkKey_ = sparkKey;
        camKey_ = camKey;
        json_ = build.save();
        asked_ = true;
        ready_ = false;
        due_ = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                                     std::chrono::duration<double>(options_.debounceSeconds));
        cancel_ = true;   // a bake running is of an older build
        setState(KnockBakeState::Waiting);
        wake_.notify_all();
        return state_;
    }

    bool CalibrationBakeService::take(CalibrationBakeResult& out)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (!ready_)
            return false;
        out = done_;
        ready_ = false;
        return true;
    }

    bool CalibrationBakeService::take(KnockBakeResult& out)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (!ready_ || done_.hasCam || !done_.hasSpark || done_.spark.cached)
            return false;
        out = done_.sparkMeasured;
        ready_ = false;
        return true;
    }

    bool CalibrationBakeService::takeAndApply(Build& build, std::string& error)
    {
        CalibrationBakeResult r;
        if (!take(r))
            return false;
        bool applied = false;
        if (r.hasCam && intakeCamKey(build, *sim_) == r.cam.key)
        {
            if (!applyFields(build, r.cam, error))
                return false;
            applied = true;
        }
        if (r.hasSpark && knockLimitKey(build, *sim_) == r.spark.key)
        {
            if (!applyFields(build, r.spark, error))
                return false;
            applied = true;
            const std::lock_guard<std::mutex> lock(mutex_);
            last_ = r.spark;
            haveLast_ = true;
        }
        // both current now: so it says (consider() need not be called for it)
        if (applied && (!sparkWanted(build) || knockLimitCurrent(build, *sim_))
            && (!camWanted(build) || intakeCamCurrent(build, *sim_)))
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            if (!asked_)
                setState(KnockBakeState::Current);
        }
        return applied;
    }

    KnockBakeState CalibrationBakeService::state() const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    std::string CalibrationBakeService::stage() const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        return state_ == KnockBakeState::Baking ? stage_ : std::string();
    }

    CalibrationBakeService::Progress CalibrationBakeService::progress() const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (state_ != KnockBakeState::Baking)
            return {};
        const int done = done_points_.load();
        return {done, std::max(done, total_points_.load())};
    }

    void CalibrationBakeService::yieldTo(const PredictService& predict)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        yieldTo_ = predict.busyFlag();
    }

    void CalibrationBakeService::stopYielding()
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        yieldTo_.reset();
    }

    bool CalibrationBakeService::lastSpark(BakedFields& out) const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (!haveLast_)
            return false;
        out = last_;
        return true;
    }

    int CalibrationBakeService::stillKnockingCells() const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        int n = 0;
        if (haveLast_)
            for (const auto& row : last_.stillKnocking)
                for (const bool k : row)
                    n += k ? 1 : 0;
        return n;
    }

    std::string CalibrationBakeService::error() const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        return error_;
    }

    void CalibrationBakeService::coordinator()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!stop_)
        {
            if (!asked_)
            {
                wake_.wait(lock);
                continue;
            }
            if (std::chrono::steady_clock::now() < due_)
            {
                wake_.wait_until(lock, due_);   // an edit within the debounce moves due_ and wakes it
                continue;
            }
            asked_ = false;
            const std::string json = json_;
            const std::int64_t sparkKey = sparkKey_, camKey = camKey_;
            cancel_ = false;
            stage_.clear();
            setState(KnockBakeState::Baking);
            lock.unlock();

            const auto at = [&](const char* stage, int total) {
                const std::lock_guard<std::mutex> guard(mutex_);
                stage_ = stage;
                done_points_ = 0;
                total_points_ = total;
                ++revision_;
            };
            // each point counted, and the bake waiting between points while the ribbon's curve is wanted
            const auto onPoint = [this] {
                ++done_points_;
                ++revision_;
                std::shared_ptr<const std::atomic<bool>> busy;
                {
                    const std::lock_guard<std::mutex> guard(mutex_);
                    busy = yieldTo_;
                }
                while (busy && !cancel_.load() && busy->load())
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
            };
            KnockBakeOptions spark = options_.spark;
            spark.predict.onPoint = onPoint;
            CamBakeOptions cam = options_.cam;
            cam.predict.onPoint = onPoint;
            const auto sparkPoints = [&](const Build& b) {
                const bool turbo = b.fieldOf("boost.kind") >= 0 && b.get("boost.kind") >= 1.0 && b.get("boost.kind") <= 3.0;
                return spark.speeds * (static_cast<int>(spark.pedals.size()) + (turbo ? spark.boostSteps : 0));
            };
            const auto camPoints = [&](const Build& b) {
                const double range = b.fieldOf("valve.inPhaseRange") >= 0 ? b.get("valve.inPhaseRange") : 0.0;
                return (static_cast<int>(range / std::max(cam.step, 1.0)) + 1) * std::max(cam.predict.points, 2);
            };
            CalibrationBakeResult result;
            std::string error;
            Build build(sim_->fields());
            std::vector<std::string> warnings;
            bool ok = build.load(json, BuildPart::Whole, warnings, error);
            // a stage: taken from the cache by its key, else baked, applied to the copy, and kept
            const std::filesystem::path& cache = options_.cacheDirectory;
            const auto sparkStage = [&](const char* name) {
                const std::int64_t key = knockLimitKey(build, *sim_);
                if (cacheRead(cache, "spark", key, result.spark))
                    return applyFields(build, result.spark, error);
                at(name, sparkPoints(build));
                if (!bakeKnockLimit(sim_, build, spark, result.sparkMeasured, error, &cancel_)
                    || !applyKnockLimit(build, result.sparkMeasured, error))
                    return false;
                result.spark = capture(build, kSparkFields, result.sparkMeasured.key);
                result.spark.stillKnocking = result.sparkMeasured.stillKnocking;
                if (!cancel_.load())
                    cacheWrite(cache, options_.cacheEntries, "spark", result.spark);
                return true;
            };
            const auto camStage = [&] {
                const std::int64_t key = intakeCamKey(build, *sim_);
                if (cacheRead(cache, "cam", key, result.cam))
                    return applyFields(build, result.cam, error);
                at("cam", camPoints(build));
                if (!bakeIntakeCam(sim_, build, cam, result.camMeasured, error, &cancel_)
                    || !applyIntakeCam(build, result.camMeasured, error))
                    return false;
                result.cam = capture(build, kCamFields, result.camMeasured.key);
                if (!cancel_.load())
                    cacheWrite(cache, options_.cacheEntries, "cam", result.cam);
                return true;
            };
            // the spark, if wanted and stale, applied to the copy
            if (ok && sparkWanted(build) && !knockLimitCurrent(build, *sim_))
            {
                ok = sparkStage("spark map");
                result.hasSpark = ok;
            }
            // the cam on that spark, if wanted and stale, applied to the copy; the spark again if that made it stale
            if (ok && camWanted(build) && !intakeCamCurrent(build, *sim_))
            {
                ok = camStage();
                result.hasCam = ok;
                if (ok && sparkWanted(build) && !knockLimitCurrent(build, *sim_))
                {
                    ok = sparkStage("spark map again");
                    result.hasSpark = ok;
                }
            }

            lock.lock();
            if (stop_ || asked_ || sparkKey != sparkKey_ || camKey != camKey_ || cancel_)
                continue;   // a newer build was asked meanwhile, or this one was set aside: dropped
            stage_.clear();
            if (ok)
            {
                done_ = std::move(result);
                ready_ = true;
                error_.clear();
                setState(KnockBakeState::Ready);
            }
            else
            {
                error_ = error;
                setState(KnockBakeState::Failed);
            }
            ++revision_;
        }
    }
} // namespace tee
