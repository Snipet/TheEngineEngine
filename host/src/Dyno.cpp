// host/src/Dyno.cpp: the dyno runner (Dyno.h).
#include "tee/Dyno.h"

#include "tee/Timeline.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace tee
{
    namespace
    {
        constexpr double kPi = 3.14159265358979323846;
        constexpr int kModeDynoEngine = 2;
        constexpr int kModeDynoChassis = 3;

        std::uint64_t mix(std::uint64_t h, double v)
        {
            std::uint64_t bits = 0;
            std::memcpy(&bits, &v, sizeof bits);
            h ^= bits + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
            return h;
        }

        std::uint64_t mix(std::uint64_t h, const std::string& s)
        {
            for (const char c : s)
                h = (h ^ static_cast<unsigned char>(c)) * 1099511628211ull;
            return h;
        }
    } // namespace

    std::uint64_t DynoPlan::hash() const
    {
        std::uint64_t h = 1469598103934665603ull;
        for (const double v : {chassis ? 1.0 : 0.0, from, to, rate, spacing, settle, measure, pedal, warmup, sampleRate,
                               static_cast<double>(block), warm ? 1.0 : 0.0, settleExhaust ? 1.0 : 0.0})
            h = mix(h, v);
        for (const double s : steps)
            h = mix(h, s);
        h = mix(h, torque);
        for (const std::string& f : fields)
            h = mix(h, f);
        return h;
    }

    double DynoPlan::warmupFor(const Build& build) const
    {
        if (warmup >= 0.0)
            return warmup;
        const bool turbo = build.fieldOf("boost.kind") >= 0 && build.get("boost.kind") != 0.0;
        const bool diesel = build.fieldOf("fuel.kind") >= 0 && build.get("fuel.kind") != 0.0;
        return turbo || diesel ? 8.0 : 2.0;
    }

    std::string DynoCurve::csv() const
    {
        std::string out = "speed_rad_s,rpm,torque_N_m,power_kW";
        for (const std::string& f : fields)
            out += "," + f;
        out += "\n";
        char buffer[64];
        for (const DynoPoint& p : points)
        {
            std::snprintf(buffer, sizeof buffer, "%.9g,%.9g,%.9g,%.9g", p.speed, p.speed * 60.0 / (2.0 * kPi), p.torque,
                          p.power / 1000.0);
            out += buffer;
            for (const double v : p.fields)
            {
                std::snprintf(buffer, sizeof buffer, ",%.9g", v);
                out += buffer;
            }
            out += "\n";
        }
        return out;
    }

    std::string DynoCurve::chart(int width, int height) const
    {
        if (points.empty() || width < 8 || height < 4)
            return "(no points)\n";
        double rpmLo = 1e300, rpmHi = -1e300, tMax = 1e-9, pMax = 1e-9;
        for (const DynoPoint& p : points)
        {
            const double rpm = p.speed * 60.0 / (2.0 * kPi);
            rpmLo = std::min(rpmLo, rpm);
            rpmHi = std::max(rpmHi, rpm);
            tMax = std::max(tMax, p.torque);
            pMax = std::max(pMax, p.power / 1000.0);
        }
        std::vector<std::string> rows(static_cast<std::size_t>(height), std::string(static_cast<std::size_t>(width), ' '));
        for (const DynoPoint& p : points)
        {
            const double rpm = p.speed * 60.0 / (2.0 * kPi);
            const int x = rpmHi > rpmLo ? static_cast<int>(std::lround((rpm - rpmLo) / (rpmHi - rpmLo) * (width - 1))) : 0;
            const auto put = [&](double v, double top, char mark) {
                const int y = static_cast<int>(std::lround(std::clamp(v / top, 0.0, 1.0) * (height - 1)));
                char& c = rows[static_cast<std::size_t>(height - 1 - y)][static_cast<std::size_t>(x)];
                c = (c == ' ' || c == mark) ? mark : '*';
            };
            put(p.torque, tMax, 'T');
            put(p.power / 1000.0, pMax, 'P');
        }
        char buffer[160];
        std::string out;
        std::snprintf(buffer, sizeof buffer, "T torque (top %.0f N m)   P power (top %.1f kW)\n", tMax, pMax);
        out += buffer;
        for (const std::string& r : rows)
            out += "|" + r + "\n";
        out += "+" + std::string(static_cast<std::size_t>(width), '-') + "\n";
        std::snprintf(buffer, sizeof buffer, " %.0f rpm%*s%.0f rpm\n", rpmLo, std::max(1, width - 16), "", rpmHi);
        out += buffer;
        return out;
    }

    bool runDyno(const std::shared_ptr<Sim>& sim, const Build& build, const DynoPlan& plan, DynoCurve& curve,
                 std::string& error, const std::function<void(double)>& progress, const std::atomic<bool>* cancel)
    {
        curve = DynoCurve{};
        curve.fields = plan.fields;
        curve.key = build.hash() ^ (plan.hash() * 31u);
        const Fields& fields = sim->fields();
        const FieldInfo* torque = nullptr;
        std::size_t torqueAt = 0;
        const FieldInfo* speed = fields.find("tel.engine.speed");
        std::string why;
        if (speed == nullptr || !telemetryElement(fields, plan.torque, torque, torqueAt, why))
        {
            error = "the dyno reads tel.engine.speed and tel." + plan.torque + ", and the program lacks "
                    + std::string(speed == nullptr ? "the first" : "the second (" + why + ")");
            return false;
        }
        // more telemetry at each point: a field or an element of one ("car.wheelSlip[2]")
        std::vector<const FieldInfo*> extra;
        std::vector<std::size_t> extraAt;
        for (const std::string& f : plan.fields)
        {
            const FieldInfo* info = nullptr;
            std::size_t at = 0;
            if (!telemetryElement(fields, f, info, at, why))
            {
                error = "the dyno is asked for tel." + f + ": " + why;
                return false;
            }
            extra.push_back(info);
            extraAt.push_back(at);
        }

        SimHost host;
        LoadOptions load;
        load.sim = sim->options();
        load.sampleRate = plan.sampleRate;
        load.maxBlock = std::max(plan.block, 16);
        load.worker = false;
        if (plan.warm)
            load.hold = warmEngine();
        if (!host.attach(sim, load, error))
            return false;
        Build copy(fields);
        {
            std::vector<std::string> warnings;
            if (!copy.load(build.save(), BuildPart::Whole, warnings, error))
                return false;
        }
        copy.attach(&host);

        // the timeline: start on the dyno at `from`, warm up, then sweep or step
        Timeline t;
        t.rate = plan.sampleRate;
        t.block = plan.block;
        const int mode = plan.chassis ? kModeDynoChassis : kModeDynoEngine;
        const auto at = [&](double time, const char* name, double value) {
            t.events.push_back({time, TimelineEvent::Kind::Control, name, value, {}, 0.0});
        };
        at(0.0, "mode", mode);
        at(0.0, "dynoSpeed", plan.from);
        at(0.0, "dynoRate", 0.0);
        // the key as a person turns it: on, then start a moment later (an ECU with a key's state machine, a diesel
        // with glow plugs, wants on before start: from off straight to start it never cranked), released once it
        // has had time to run
        const double warmup = plan.warmupFor(build);
        at(0.0, "ignition", 1);
        at(0.25, "ignition", 2);
        at(std::max(1.0, std::min(2.0, 0.5 * warmup)), "ignition", 1);
        at(0.5 * warmup, "throttle", plan.pedal);
        const double start = warmup + plan.settle;
        // the exhaust's heat settled (G's spec.exhaust.heatSettle, a counter: each increase puts the gas and the walls
        // where the present flow holds them), as the predictor settles it: a held step then reads the steady state,
        // the same quantity as a predicted point, not a step's first seconds with the walls still warming
        const bool settleHeat = plan.settleExhaust && build.fieldOf("exhaust.heatSettle") >= 0;
        double heatCount = settleHeat ? build.get("exhaust.heatSettle") : 0.0;
        const auto settleAt = [&](double time) {
            if (!settleHeat)
                return;
            heatCount += 1.0;
            t.events.push_back({time, TimelineEvent::Kind::Spec, "exhaust.heatSettle", heatCount, {}, 0.0});
        };
        struct Window
        {
            double from, to;
        };
        std::vector<Window> windows;
        double end = start;
        if (plan.steps.empty())
        {
            const double rate = std::max(1e-6, std::fabs(plan.rate)) * (plan.to >= plan.from ? 1.0 : -1.0);
            settleAt(start - 0.5 * plan.settle);   // at the sweep's start (a sweep's walls then follow the sweep)
            at(start, "dynoRate", rate);
            end = start + (plan.to - plan.from) / rate;
        }
        else
        {
            double time = warmup;
            for (const double s : plan.steps)
            {
                at(time, "dynoSpeed", s);
                // twice: once the speed is there, and again later in the hold, the flow then the speed's own
                settleAt(time + std::min(0.3, 0.3 * plan.settle));
                settleAt(time + 0.6 * plan.settle);
                windows.push_back({time + plan.settle, time + plan.settle + plan.measure});
                time += plan.settle + plan.measure;
            }
            end = time;
        }
        t.seconds = end;
        t.sort();

        // the points: a sweep's binned by speed, a step's averaged over its window
        struct Sum
        {
            double speed = 0.0, torque = 0.0;
            std::vector<double> fields;
            int n = 0;
        };
        std::map<long, Sum> bins;
        std::vector<Sum> stepSums(windows.size());
        RunOptions options;
        options.keepAudio = false;
        options.telemetry = {"engine.speed"};
        const FieldInfo* flags = fields.find("tel.engine.flags");
        bool faulted = false;
        options.cancelled = [cancel, &faulted] {
            return faulted || (cancel != nullptr && cancel->load(std::memory_order_relaxed));
        };
        double lastProgress = -1.0;
        options.afterBlock = [&](double time, const TelemetryFrame& frame) {
            const double w = frame.get(*speed);
            Sum* sum = nullptr;
            if (plan.steps.empty())
            {
                if (time >= start && time <= end)
                    sum = &bins[std::lround((w - plan.from) / plan.spacing)];
            }
            else
                for (std::size_t k = 0; k < windows.size(); ++k)
                    if (time > windows[k].from && time <= windows[k].to)
                        sum = &stepSums[k];
            if (flags != nullptr && (static_cast<long>(frame.get(*flags)) & 4) != 0)
            {
                // FLAG_FAULT: the engine was put at rest; the point being measured is no measure, and the run stops
                faulted = true;
                ++curve.faults;
                if (sum != nullptr)
                    sum->n = 0;
                return;
            }
            if (sum != nullptr)
            {
                sum->speed += w;
                sum->torque += frame.get(*torque, torqueAt);
                sum->fields.resize(extra.size(), 0.0);
                for (std::size_t k = 0; k < extra.size(); ++k)
                    sum->fields[k] += frame.get(*extra[k], extraAt[k]);
                ++sum->n;
            }
            if (progress && time - lastProgress > 0.25)
            {
                lastProgress = time;
                progress(std::clamp(time / end, 0.0, 1.0));
            }
        };
        RunResult result;
        if (!runTimeline(host, copy, t, options, result, error))
            return false;
        if (result.host.guardTrips > 0)
        {
            error = "the engine tripped the host's guard on the dyno: " + result.stopReason;
            return false;
        }
        const auto point = [&](const Sum& s) {
            DynoPoint p;
            p.speed = s.speed / s.n;
            p.torque = s.torque / s.n;
            p.power = p.torque * p.speed;
            for (const double f : s.fields)
                p.fields.push_back(f / s.n);
            curve.points.push_back(std::move(p));
        };
        if (plan.steps.empty())
        {
            for (const auto& [bin, s] : bins)
                if (s.n > 0)
                    point(s);
        }
        else
            for (const Sum& s : stepSums)
                if (s.n > 0)
                    point(s);
        curve.simulatedSeconds = result.simulatedSeconds;
        curve.wallSeconds = result.wallSeconds;
        curve.cancelled = result.cancelled && !faulted;
        curve.settled = plan.settleExhaust && build.fieldOf("exhaust.heatSettle") >= 0;
        if (progress)
            progress(1.0);
        return true;
    }

    // ---- the service ------------------------------------------------------------------------------------------------

    DynoService::DynoService(std::shared_ptr<Sim> sim) : sim_(std::move(sim))
    {
        thread_ = std::thread([this] { loop(); });
    }

    DynoService::~DynoService()
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

    bool DynoService::request(const Build& build, const DynoPlan& plan, DynoCurve& cached)
    {
        const std::uint64_t key = build.hash() ^ (plan.hash() * 31u);
        const std::lock_guard<std::mutex> lock(mutex_);
        if (auto it = cache_.find(key); it != cache_.end())
        {
            cached = it->second;
            return true;
        }
        buildJson_ = build.save();
        plan_ = plan;
        key_ = key;
        waiting_ = true;
        cancel_ = true;   // the one running is older: dropped
        wake_.notify_all();
        return false;
    }

    bool DynoService::take(DynoCurve& out)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (!ready_)
            return false;
        out = done_;
        ready_ = false;
        return true;
    }

    std::string DynoService::error() const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        return error_;
    }

    void DynoService::loop()
    {
        while (true)
        {
            std::string json;
            DynoPlan plan;
            std::uint64_t key = 0;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                wake_.wait(lock, [this] { return stop_ || waiting_; });
                if (stop_)
                    return;
                json = buildJson_;
                plan = plan_;
                key = key_;
                waiting_ = false;
                cancel_ = false;
            }
            busy_ = true;
            progress_ = 0.0;
            Build build(sim_->fields());
            std::vector<std::string> warnings;
            std::string error;
            DynoCurve curve;
            const bool ok = build.load(json, BuildPart::Whole, warnings, error)
                            && runDyno(sim_, build, plan, curve, error, [this](double p) { progress_ = p; }, &cancel_);
            busy_ = false;
            const std::lock_guard<std::mutex> lock(mutex_);
            if (!ok)
            {
                error_ = error;
                continue;
            }
            if (curve.cancelled)
                continue;   // a newer request replaced it
            curve.key = key;
            cache_[key] = curve;
            done_ = std::move(curve);
            ready_ = true;
            error_.clear();
        }
    }
} // namespace tee
