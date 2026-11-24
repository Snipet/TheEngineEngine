// host/src/Bake.cpp: the intake cam baked by a sweep of the fast dyno (Bake.h).
#include "tee/Bake.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
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

    std::int64_t knockLimitKey(const Build& build, const Sim& sim)
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
            if (all[i].block != "spec" || !movesKnock(all[i]))
                continue;
            mixIn(all[i].path.data(), all[i].path.size());
            for (const double v : build.values(static_cast<int>(i)))
                mixIn(&v, sizeof v);
        }
        return static_cast<std::int64_t>(h & 0x7fffffffull);
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
            {
                std::vector<std::string> said;
                if (!stepBuild.load(build.save(), BuildPart::Whole, said, error))
                    return false;
            }
            if (steps[k].boost >= 0.0)
            {
                stepBuild.set("ecu.maps.boostAuto", 1.0);
                stepBuild.set("boost.target", steps[k].boost);
            }
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
                std::size_t at = 0;
                while (at + 1 < result.speeds.size() && std::fabs(result.speeds[at] - p.speed) > 1e-6)
                    ++at;
                KnockBakeCell cell;
                cell.pedal = predict.pedal;
                cell.boostTarget = std::max(steps[k].boost, 0.0);
                cell.load = p.fields.empty() ? 0.0 : p.fields[0] / 101325.0;
                cell.retardMean = p.retardMean;
                cell.retardTooth = std::max(p.retardMax - p.retardMean, 0.0);
                cell.torque = p.torque;
                cell.knockLimited = p.knockLimited;
                cell.valid = p.valid;
                result.measured[at].push_back(cell);
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
        for (const auto& column : result.measured)
        {
            std::vector<std::pair<double, double>> at;   // (load, cell)
            for (const KnockBakeCell& c : column)
                if (c.valid)
                    at.emplace_back(c.load, c.retardMean >= options.knockLimit ? c.retardMean + options.margin : 0.0);
            std::sort(at.begin(), at.end());
            std::vector<double> row;
            std::vector<bool> flat;
            for (const double y : result.loads)
            {
                if (at.empty())
                {
                    row.push_back(0.0);
                    flat.push_back(true);
                }
                else if (y <= at.front().first)
                {
                    row.push_back(at.front().second);
                    flat.push_back(at.front().second != 0.0 && y < at.front().first);   // under a 0: 0 by the rule
                }
                else if (y >= at.back().first)
                {
                    row.push_back(at.back().second);
                    flat.push_back(y > at.back().first);
                }
                else
                {
                    std::size_t i = 1;
                    while (at[i].first < y)
                        ++i;
                    const double u = (y - at[i - 1].first) / std::max(at[i].first - at[i - 1].first, 1e-9);
                    row.push_back(at[i - 1].second + u * (at[i].second - at[i - 1].second));
                    flat.push_back(false);
                }
            }
            result.limit.push_back(std::move(row));
            result.extrapolated.push_back(std::move(flat));
        }
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

    KnockBakeService::KnockBakeService(std::shared_ptr<Sim> sim, KnockBakeOptions options, double debounceSeconds)
        : sim_(std::move(sim)), options_(std::move(options)), debounce_(std::max(debounceSeconds, 0.0))
    {
        thread_ = std::thread([this] { coordinator(); });
    }

    KnockBakeService::~KnockBakeService()
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

    void KnockBakeService::setState(KnockBakeState s)
    {
        if (state_ != s)
        {
            state_ = s;
            ++revision_;
        }
    }

    KnockBakeState KnockBakeService::consider(Build& build)
    {
        if (build.fieldOf("ecu.maps.knockLimit") < 0 || build.fieldOf("ecu.maps.knockLimitAuto") < 0
            || build.fieldOf("ecu.maps.knockLimitKey") < 0)
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            setState(KnockBakeState::NoTable);
            return state_;
        }
        const std::int64_t key = knockLimitKey(build, *sim_);
        if (knockLimitCurrent(build, *sim_))
        {
            const std::lock_guard<std::mutex> lock(mutex_);
            if (key_ != key)
                cancel_ = true;   // what runs is for another build: not wanted
            asked_ = false;
            ready_ = false;
            key_ = key;
            setState(KnockBakeState::Current);
            return state_;
        }
        // stale: the table set aside now (the engine on its knock control alone), a bake asked
        if (build.get("ecu.maps.knockLimitAuto") == 0.0)
            build.set("ecu.maps.knockLimitAuto", 1.0);
        const std::lock_guard<std::mutex> lock(mutex_);
        if (key == key_ && state_ != KnockBakeState::Failed && state_ != KnockBakeState::Current)
            return state_;   // asked already (waiting, baking, or ready to take)
        key_ = key;
        json_ = build.save();
        asked_ = true;
        ready_ = false;
        due_ = std::chrono::steady_clock::now() + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                                                     std::chrono::duration<double>(debounce_));
        cancel_ = true;   // a bake running is of an older build
        setState(KnockBakeState::Waiting);
        wake_.notify_all();
        return state_;
    }

    bool KnockBakeService::take(KnockBakeResult& out)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (!ready_)
            return false;
        out = done_;
        ready_ = false;
        return true;
    }

    KnockBakeState KnockBakeService::state() const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        return state_;
    }

    std::string KnockBakeService::error() const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        return error_;
    }

    void KnockBakeService::coordinator()
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
            const std::int64_t key = key_;
            cancel_ = false;
            setState(KnockBakeState::Baking);
            lock.unlock();

            KnockBakeResult result;
            std::string error;
            Build build(sim_->fields());
            std::vector<std::string> warnings;
            bool ok = build.load(json, BuildPart::Whole, warnings, error);
            if (ok)
                ok = bakeKnockLimit(sim_, build, options_, result, error, &cancel_);

            lock.lock();
            if (stop_ || asked_ || key != key_ || cancel_)
                continue;   // a newer build was asked meanwhile, or this one was set aside: dropped
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
        }
    }
} // namespace tee
