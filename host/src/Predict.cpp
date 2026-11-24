// host/src/Predict.cpp: the predicted curve (Predict.h).
#include "tee/Predict.h"

#include "tee/Denormals.h"
#include "tee/Instance.h"
#include "tee/Warm.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <functional>

namespace tee
{
    namespace
    {
        constexpr double kFourPi = 12.566370614359172;   // a cycle: two turns of the crank, whatever the engine
        constexpr int kModeDynoEngine = 2;

        using Clock = std::chrono::steady_clock;

        double seconds(Clock::time_point a, Clock::time_point b)
        {
            return std::chrono::duration<double>(b - a).count();
        }

        // One instance of the program, at a build, braked at a held speed.
        class Rig
        {
        public:
            Rig(const std::shared_ptr<Sim>& sim, const PredictOptions& options) : sim_(sim), options_(options) {}

            ~Rig()
            {
                if (begun_)
                    sim_->end()(instance_->data());
            }

            Rig(const Rig&) = delete;
            Rig& operator=(const Rig&) = delete;

            // Makes the instance at `build`, runs init. False with `error` when the program cannot be braked.
            bool make(const Build& build, std::string& error)
            {
                const Schema& schema = sim_->schema();
                const Fields& fields = sim_->fields();
                std::string e;
                const auto find = [&](const char* path, Schema::Handle& h) { return schema.find(path, h, e); };
                const bool programDyno = find("tel.engine.torque", torque_) && find("ctl.mode", mode_)
                                         && find("ctl.dynoSpeed", dynoSpeed_) && find("ctl.dynoRate", dynoRate_);
                if (!find("tel.engine.speed", speed_) || !find("ctl.throttle", throttle_)
                    || !find("ctl.ignition", ignition_))
                {
                    error = "the program has no tel.engine.speed, ctl.throttle or ctl.ignition: it cannot be put on a dyno";
                    return false;
                }
                // the crank's speed, held by the host while the engine starts (and throughout, with the host's brake)
                holdCrank_ = find("crank.omega", omega_);
                hasState_ = find("tel.engine.state", state_);
                const bool custom = build.fieldOf("engine.custom") >= 0 && build.get("engine.custom") != 0.0;
                const bool hostCan = holdCrank_ && find("mech.jBase", jBase_) && find("set.inertia", dialJ_)
                                     && find("set.load", dialLoad_) && !custom;
                if (options_.brake == "host" || (options_.brake == "auto" && hostCan))
                {
                    if (!hostCan)
                    {
                        error = custom ? "the host's brake does not know a custom engine's inertia (engine.custom is on)"
                                       : "the program lacks what the host brakes by (crank.omega, mech.jBase, set.inertia, "
                                         "set.load)";
                        return false;
                    }
                    hasTorque_ = false;
                }
                else if (programDyno)
                {
                    hasTorque_ = true;
                    // the absorber's torque now (core/dyno.fm), averaged here over whole cycles; else tel's mean
                    absorber_ = find("dyno.tqAbs", absorberH_);
                }
                else
                {
                    error = "the program has no dyno (tel.engine.torque, ctl.mode, ctl.dynoSpeed, ctl.dynoRate)"
                            + std::string(hostCan || options_.brake == "program" ? "" : ", nor what the host brakes by");
                    return false;
                }
                // the exhaust gas's lags (exhaust/exhaust.fm): the ports' temperature over 0.8 s, the pipes' over 0.3 s
                if (options_.settleExhaust)
                {
                    Schema::Handle h;
                    if (find("exh.tPort", h))
                        slow_.push_back({h, 0, 0, 1.0, 250.0, 1500.0, 0.0, 0.0, 0.0});
                    // the reference's turbo: its shaft's speed, 0 to 1 (boost/boost.fm), spooling over seconds
                    if (build.fieldOf("boost.kind") >= 0 && build.get("boost.kind") != 0.0 && find("boost.spool", h))
                        slow_.push_back({h, 0, 0, 0.15 / 60.0, 0.0, 1.0, 0.0, 0.0, 0.0});
                    if (find("exh.tWall", h))
                        for (std::size_t i = 0; i < h.count; ++i)
                            slow_.push_back({h, i, 1, 1.0, 250.0, 1500.0, 0.0, 0.0, 0.0});
                }
                redline_ = find("tel.engine.redline", redlineH_);
                idle_ = find("tel.engine.idle", idleH_);

                instance_ = std::make_unique<Instance>(sim_->module());
                unsigned char* data = static_cast<unsigned char*>(instance_->data());
                sim_->writeDefaults(data);
                const auto writeSpec = [&] {
                    for (std::size_t i = 0; i < fields.all().size(); ++i)
                    {
                        const FieldInfo& f = fields.all()[i];
                        if (f.block != "spec" || build.values(static_cast<int>(i)).empty())
                            continue;
                        const std::vector<unsigned char> b = build.bytes(static_cast<int>(i));
                        std::memcpy(data + f.offset, b.data(), std::min(b.size(), f.size));
                    }
                };
                writeSpec();
                // The host's brake replaces the bench's load: its torque and its added inertia are not the engine's.
                if (!hasTorque_)
                    if (const FieldInfo* load = fields.find("spec.engine.benchLoad"); load != nullptr && load->scalar == Scalar::Float)
                    {
                        const float zero = 0.0f;
                        std::memcpy(data + load->offset, &zero, sizeof zero);
                    }
                {
                    const ScopedFlushDenormals flush;
                    sim_->init()(data, static_cast<float>(options_.sampleRate));
                    begun_ = true;
                }
                // init may have set the Spec's memory from its own idea: the build's again, and every section's rev moved
                writeSpec();
                if (!hasTorque_)
                    if (const FieldInfo* load = fields.find("spec.engine.benchLoad"); load != nullptr && load->scalar == Scalar::Float)
                    {
                        const float zero = 0.0f;
                        std::memcpy(data + load->offset, &zero, sizeof zero);
                    }
                for (const Section& s : fields.sections())
                {
                    std::int32_t rev = 0;
                    std::memcpy(&rev, data + s.revOffset, 4);
                    ++rev;
                    std::memcpy(data + s.revOffset, &rev, 4);
                }
                if (options_.warm)
                    for (const HeldValue& h : warmEngine())
                    {
                        Schema::Handle handle;
                        if (schema.find(h.path, handle, e) && handle.scalar() != Scalar::None)
                            held_.emplace_back(handle, h.value);
                    }
                if (options_.warm)
                    warm();
                left_.assign(static_cast<std::size_t>(std::max(options_.block, 1)), 0.0f);
                right_ = left_;
                return true;
            }

            // The engine started (held at 1000 rpm until it runs, then 50 ms more: the simulator says its redline and
            // idle for the build only once it runs); then the redline and the idle as it says them.
            void probe(double& idle, double& redline)
            {
                const double w = 1000.0 * kFourPi / 120.0;
                set(ignition_, 2.0);
                long done = 0;
                for (; done < std::lround(3.0 * options_.sampleRate); done += options_.block)
                {
                    if (done > 0 && (!hasState_ || instance_->read(state_) == 2.0))
                        break;
                    if (holdCrank_)
                        set(omega_, w);
                    run(options_.block);
                }
                set(ignition_, 1.0);
                for (long more = 0; more < std::lround(0.05 * options_.sampleRate); more += options_.block)
                {
                    if (holdCrank_)
                        set(omega_, w);
                    run(options_.block);
                }
                idle = idle_ ? instance_->read(idleH_) : 0.0;
                redline = redline_ ? instance_->read(redlineH_) : 0.0;
            }

            // Starts the engine held at `speed`: the key in start for a moment, then on, and the pedal down.
            // (the crank held at `speed` by the host meanwhile, where it can be, so the engine fires at once: the
            // program's dyno takes it only once it runs), until it runs or three seconds have gone
            void start(double speed)
            {
                set(ignition_, 2.0);
                set(throttle_, 0.0);
                const long least = std::lround(options_.startSeconds * options_.sampleRate);
                const long most = std::lround(3.0 * options_.sampleRate);
                for (long done = 0; done < most; done += options_.block)
                {
                    if (done >= least && (!hasState_ || instance_->read(state_) == 2.0))
                        break;
                    if (holdCrank_)
                        set(omega_, speed);
                    step(speed, options_.block);
                }
                set(ignition_, 1.0);
                set(throttle_, options_.pedal);
                // then until it runs steadily: three cycles in a row that give torque and agree within 5 % (a
                // diesel held from cold misfires for many before its charge lights every time, and a turbo spools
                // over seconds), at most eight seconds
                const double perCycle = kFourPi / std::max(speed, 1.0) * options_.sampleRate;
                int steady = 0;
                double last = 0.0;
                for (double ran = 0.0; ran < 8.0 * options_.sampleRate && steady < 3; ran += perCycle)
                {
                    double sum = 0.0;
                    const long total = std::lround(perCycle);
                    for (long run = 0; run < total;)
                    {
                        const int k = static_cast<int>(std::min<long>(options_.block, total - run));
                        sum += step(speed, k) * k;
                        run += k;
                    }
                    const double t = sum / static_cast<double>(total);
                    steady = t > 0.0 && std::fabs(t - last) <= 0.05 * t ? steady + 1 : 0;
                    last = t;
                }
            }

            // Full throttle at the middle of the range for warmupSeconds: the exhaust's gas and pipes warm.
            void warmup(const std::vector<double>& speeds)
            {
                const double w = 0.5 * (speeds.front() + speeds.back());
                const long n = std::lround(options_.warmupSeconds * options_.sampleRate);
                for (long done = 0; done < n; done += options_.block)
                {
                    if (options_.warm && done % (64 * options_.block) == 0)
                        warm();
                    step(w, options_.block);
                }
            }

            // One point: held at `speed`, cycle after cycle until two agree or the most are run.
            PredictPoint point(double speed, const std::atomic<bool>* cancel)
            {
                PredictPoint p;
                p.speed = speed;
                const double perCycle = kFourPi / std::max(speed, 1.0) * options_.sampleRate;   // samples
                double previous = 0.0, held = 0.0;
                const int settling = slow_.empty() ? 0 : fresh_ ? 8 : 6;
                const int most = options_.holdSeconds > 0.0 ? 1 << 30 : options_.maxCycles + settling;
                for (int c = 0; c < most; ++c)
                {
                    if (cancel != nullptr && cancel->load(std::memory_order_relaxed))
                        return p;
                    if (options_.warm)
                        warm();   // the cell holds the engine at its working temperatures
                    double sum = 0.0;
                    long run = 0;
                    const long total = std::lround(perCycle * (c + 1)) - std::lround(perCycle * c);
                    while (run < total)
                    {
                        const int k = static_cast<int>(std::min<long>(options_.block, total - run));
                        sum += step(speed, k) * k;
                        run += k;
                    }
                    const double t = sum / static_cast<double>(std::max(total, 1L));
                    held += total / options_.sampleRate;
                    // the settling cycles (above), after the first; their torque is not counted
                    if (c >= 1 && c <= settling)
                    {
                        settleStep(c - 1, total / options_.sampleRate);
                        p.cycles = c + 1;
                        p.history.push_back(t);
                        continue;
                    }
                    if (settling > 0 && c == 0)
                        settleBegin();
                    p.cycles = c + 1;
                    p.history.push_back(t);
                    // the cycles counted: the first after the speed (or the settling) moved is not
                    const int k = c == 0 ? 0 : c - settling;
                    if (k >= 1)
                    {
                        p.torque = 0.5 * (t + previous);
                        if (k + 1 >= options_.minCycles && held >= options_.holdSeconds
                            && std::fabs(t - previous) <= options_.tolerance * std::max(std::fabs(p.torque), 1.0))
                        {
                            p.settled = true;
                            break;
                        }
                    }
                    else
                        p.torque = t;
                    previous = t;
                }
                p.power = p.torque * speed;
                fresh_ = false;
                return p;
            }

            // Swept from speeds' first to its last at `rate`; the torque at each speed the mean over the two cycles about
            // the moment the sweep passed it.
            std::vector<PredictPoint> ramp(const std::vector<double>& speeds, double rate)
            {
                struct Block
                {
                    double time, speed, torque;
                };
                std::vector<Block> blocks;
                const double dt = options_.block / options_.sampleRate;
                double w = speeds.front(), time = 0.0;
                // settle at the first speed for some cycles, as a point would
                for (int c = 0; c < options_.maxCycles; ++c)
                    for (double a = 0.0; a < kFourPi; a += w * dt)
                        step(w, options_.block);
                while (w <= speeds.back() + 1e-9)
                {
                    if (options_.warm && blocks.size() % 64 == 0)
                        warm();
                    const double t = step(w, options_.block);
                    blocks.push_back({time + 0.5 * dt, w + 0.5 * rate * dt, t});
                    time += dt;
                    w += rate * dt;
                }
                std::vector<PredictPoint> out;
                for (const double s : speeds)
                {
                    const double at = (s - speeds.front()) / rate, half = kFourPi / s;   // a cycle each side
                    double sum = 0.0;
                    int k = 0;
                    for (const Block& b : blocks)
                        if (b.time >= at - half && b.time <= at + half)
                        {
                            sum += b.torque;
                            ++k;
                        }
                    PredictPoint p;
                    p.speed = s;
                    p.torque = k > 0 ? sum / k : 0.0;
                    p.power = p.torque * s;
                    p.settled = k > 0;
                    out.push_back(p);
                }
                return out;
            }

            double simulated() const noexcept { return samples_ / options_.sampleRate; }
            bool programBrake() const noexcept { return hasTorque_; }

        private:
            void set(const Schema::Handle& h, double v) noexcept { instance_->write(h, v); }

            // n samples held at `speed`; the mean torque the brake took over them (N m)
            double step(double speed, int n)
            {
                void* data = instance_->data();
                const ScopedFlushDenormals flush;
                if (hasTorque_)
                {
                    set(mode_, kModeDynoEngine);
                    set(dynoSpeed_, speed);
                    set(dynoRate_, 0.0);
                    sim_->process()(data, left_.data(), right_.data(), n);
                    samples_ += n;
                    return instance_->read(absorber_ ? absorberH_ : torque_);
                }
                set(omega_, speed);
                sim_->process()(data, left_.data(), right_.data(), n);
                samples_ += n;
                const double gained = instance_->read(omega_) - static_cast<double>(static_cast<float>(speed));
                // Mechanics::inertia: the base, the flywheel dial's factor (5x over its travel), the bench load's
                const double j = instance_->read(jBase_) * std::exp(1.6094379 * (instance_->read(dialJ_) - 0.5))
                                 * (1.0 + 5.0 * instance_->read(dialLoad_));
                return j * gained * options_.sampleRate / n;
            }

            // A warm engine, held so as a dyno cell's conditioning holds it: the coolant at its thermostat, the oil 10 K
            // and the chamber's walls 87 K over it, as Heat has a running engine's (core/heat.fm); the rest (the oil's
            // thickness, the friction) follows at the next control step. Left alone at full load the coolant climbs
            // past the thermostat over minutes, the oil thins and the friction falls: a curve that drifts with time.
            void warm()
            {
                for (const auto& [handle, value] : held_)
                    instance_->write(handle, value);
            }

            // The exhaust's slow temperatures (exhaust/exhaust.fm: the gas at the ports over 0.8 s, the pipes' walls
            // over tens of seconds to minutes) decide the waves' speed and so where the pipes help and hinder: a point
            // held for cycles is another curve than one held for a minute, and a sweep's curve depends on how long it
            // took. So each point brings them to where they head, as a steady-state dyno's would after minutes: each
            // is a lag toward a value x* that the speed sets, x' = (x* - x) / tau, tau unknown. Its drift over a cycle
            // at x, then over a cycle from x + delta, gives x* (the secant: where the drift is 0); it is put there. Two
            // rounds, the second with a quarter of the step; the walls in a chain converge as the second round sees
            // the first's gas. The cycles that measure the drifts are not counted in the torque.
            struct Slow
            {
                Schema::Handle handle;
                std::size_t index = 0;
                int group = 0;           // 0 the ports' gas and the turbo's shaft, 1 the walls (settled after the gas)
                double scale = 1.0;      // its nudge over a temperature's (K)
                double low = 250.0, high = 1500.0;
                double x0 = 0.0, s0 = 0.0, m0 = 0.0;
            };

            // Called at each cycle's end with the cycle's length; `phase` 0, 1, 2, 3 for the four settling cycles.
            void settleBegin()
            {
                for (Slow& v : slow_)
                    v.x0 = instance_->read(v.handle, v.index);
            }

            // After each settling cycle (phase 0 up): the gas's rounds, then the walls' two.
            void settleStep(int phase, double cycle)
            {
                // the gas's rounds come first (two after a start: the gas is still rising), then the walls' two
                const int gasRounds = fresh_ ? 2 : 1;
                const int group = phase < 2 * gasRounds ? 0 : 1;
                const double nudge = group == 0 ? 60.0 : phase < 2 * gasRounds + 2 ? 150.0 : 40.0;
                const double gas = slow_.front().group == 0 ? instance_->read(slow_.front().handle, 0) : 1500.0;
                for (Slow& v : slow_)
                {
                    const double x = instance_->read(v.handle, v.index);
                    if (v.group != group)
                    {
                        v.x0 = x;
                        continue;
                    }
                    const double drift = (x - v.x0) / cycle;
                    const double mid = 0.5 * (x + v.x0);
                    if (phase % 2 == 0)
                    {
                        // the first drift: kept, then the nudge toward where it heads
                        v.s0 = drift;
                        v.m0 = mid;
                        const double nudged = std::clamp(x + (drift >= 0.0 ? 1.0 : -1.0) * nudge * v.scale, v.low, v.high);
                        instance_->write(v.handle, nudged, v.index);
                        v.x0 = nudged;
                    }
                    else
                    {
                        // the second: where the drift is 0, if the two say a lag toward a value
                        const double ds = v.s0 - drift, dm = mid - v.m0;
                        if (std::fabs(dm) > 1e-6 && ds * dm > 0.0)
                        {
                            const double target = v.m0 + v.s0 * dm / ds;
                            // a wall ends between the air and the gas that reaches it
                            const double top = v.group == 1 ? std::max(gas, 300.0) : v.high;
                            if (std::isfinite(target))
                                instance_->write(v.handle, std::clamp(target, v.low, top), v.index);
                        }
                        v.x0 = instance_->read(v.handle, v.index);
                    }
                }
            }

            void run(int n)
            {
                const ScopedFlushDenormals flush;
                sim_->process()(instance_->data(), left_.data(), right_.data(), n);
                samples_ += n;
            }

            std::shared_ptr<Sim> sim_;
            PredictOptions options_;
            std::unique_ptr<Instance> instance_;
            bool begun_ = false;
            bool hasTorque_ = false, redline_ = false, idle_ = false, holdCrank_ = false, hasState_ = false,
                 absorber_ = false;
            Schema::Handle state_, absorberH_;
            std::vector<Slow> slow_;
            std::vector<std::pair<Schema::Handle, double>> held_;   // Warm.h's warm engine
            bool fresh_ = true;   // no point made yet since the start
            Schema::Handle torque_, speed_, throttle_, ignition_, mode_, dynoSpeed_, dynoRate_, omega_, jBase_, dialJ_,
                dialLoad_, redlineH_, idleH_;
            std::vector<float> left_, right_;
            double samples_ = 0.0;
        };

        std::uint64_t mix(std::uint64_t h, double v)
        {
            std::uint64_t bits = 0;
            std::memcpy(&bits, &v, sizeof bits);
            h ^= bits + 0x9e3779b97f4a7c15ull + (h << 6) + (h >> 2);
            return h;
        }

        int workersFor(const PredictOptions& o)
        {
            if (o.threads > 0)
                return std::min(o.threads, 16);
            const int cores = static_cast<int>(std::thread::hardware_concurrency());
            return std::clamp(cores - 1, 1, 6);
        }

        // The speeds of a curve: the options' range, else the program's own idle and redline.
        bool speedsOf(const std::shared_ptr<Sim>& sim, const Build& build, const PredictOptions& options,
                      std::vector<double>& speeds, std::string& error)
        {
            double from = options.from, to = options.to;
            if (from <= 0.0 || to <= 0.0)
            {
                Rig probe(sim, options);
                if (!probe.make(build, error))
                    return false;
                double idle = 0.0, redline = 0.0;
                probe.probe(idle, redline);
                if (from <= 0.0)
                    from = std::max(1000.0 * kFourPi / 120.0, idle);   // 1000 rpm
                if (to <= 0.0)
                    to = redline > from ? redline - 100.0 * kFourPi / 120.0 : from * 6.0;   // 100 rpm under it
            }
            const int n = std::max(options.points, 2);
            speeds.resize(static_cast<std::size_t>(n));
            for (int i = 0; i < n; ++i)
                speeds[static_cast<std::size_t>(i)] = from + (to - from) * i / (n - 1);
            return true;
        }

        // The curve, with `partial` called (on a worker's thread, under no lock of ours) once the coarse speeds are in
        // and after each speed past them.
        bool makeCurve(const std::shared_ptr<Sim>& sim, const Build& build, const PredictOptions& options,
                       PredictCurve& curve, std::string& error, const std::atomic<bool>* cancel,
                       const std::function<void(const PredictCurve&, double)>& partial)
        {
            const Clock::time_point t0 = Clock::now();
            curve = PredictCurve{};
            curve.key = predictKey(build, options);

            std::vector<double> speeds;
            if (!speedsOf(sim, build, options, speeds, error))
                return false;
            const int n = static_cast<int>(speeds.size());
            const int c = std::clamp(options.coarse, 2, n);
            // the coarse ones first, then the rest from the bottom
            std::vector<int> order;
            std::vector<bool> isCoarse(static_cast<std::size_t>(n), false);
            for (int k = 0; k < c; ++k)
            {
                const int i = static_cast<int>(std::lround(static_cast<double>(k) * (n - 1) / (c - 1)));
                if (!isCoarse[static_cast<std::size_t>(i)])
                {
                    isCoarse[static_cast<std::size_t>(i)] = true;
                    order.push_back(i);
                }
            }
            const int coarseCount = static_cast<int>(order.size());
            for (int i = 0; i < n; ++i)
                if (!isCoarse[static_cast<std::size_t>(i)])
                    order.push_back(i);

            std::mutex mutex;
            std::vector<PredictPoint> done(static_cast<std::size_t>(n));
            std::vector<bool> have(static_cast<std::size_t>(n), false);
            int haveCoarse = 0, haveAll = 0;
            std::atomic<int> next{0};
            std::atomic<double> simulated{0.0};
            std::string firstError;
            bool program = false;

            const auto publish = [&](bool final) {
                // under `mutex`
                PredictCurve p;
                p.key = curve.key;
                for (int i = 0; i < n; ++i)
                    if (have[static_cast<std::size_t>(i)])
                        p.points.push_back(done[static_cast<std::size_t>(i)]);
                p.refined = final;
                p.brake = program ? "program" : "host";
                p.wallSeconds = seconds(t0, Clock::now());
                p.simulatedSeconds = simulated.load();
                return p;
            };

            const auto work = [&] {
                Rig rig(sim, options);
                std::string e;
                if (!rig.make(build, e))
                {
                    const std::lock_guard<std::mutex> lock(mutex);
                    if (firstError.empty())
                        firstError = e;
                    return;
                }
                bool started = false;
                while (true)
                {
                    if (cancel != nullptr && cancel->load(std::memory_order_relaxed))
                        break;
                    const int k = next.fetch_add(1);
                    if (k >= n)
                        break;
                    const int i = order[static_cast<std::size_t>(k)];
                    const double w = speeds[static_cast<std::size_t>(i)];
                    if (!started)
                    {
                        rig.start(speeds.front());   // the lowest: where an engine lights best
                        rig.warmup(speeds);
                        started = true;
                    }
                    const PredictPoint p = rig.point(w, cancel);
                    if (cancel != nullptr && cancel->load(std::memory_order_relaxed))
                        break;
                    PredictCurve snapshot;
                    double share = 0.0;
                    bool tell = false;
                    {
                        const std::lock_guard<std::mutex> lock(mutex);
                        program = rig.programBrake();
                        done[static_cast<std::size_t>(i)] = p;
                        have[static_cast<std::size_t>(i)] = true;
                        ++haveAll;
                        if (isCoarse[static_cast<std::size_t>(i)])
                            ++haveCoarse;
                        if (haveCoarse == coarseCount && haveAll < n)
                        {
                            snapshot = publish(false);
                            share = static_cast<double>(haveAll) / n;
                            tell = true;
                        }
                    }
                    if (tell && partial)
                        partial(snapshot, share);
                }
                double s = simulated.load();
                while (!simulated.compare_exchange_weak(s, s + rig.simulated()))
                {
                }
            };

            const int threads = std::min(workersFor(options), n);
            std::vector<std::thread> pool;
            for (int k = 1; k < threads; ++k)
                pool.emplace_back(work);
            work();
            for (std::thread& t : pool)
                t.join();

            if (haveAll < n && !firstError.empty())
            {
                error = firstError;
                return false;
            }
            curve = publish(haveAll == n);
            return true;
        }
    } // namespace

    std::uint64_t predictKey(const Build& build, const PredictOptions& o)
    {
        std::uint64_t h = build.hash();
        for (const double v : {static_cast<double>(o.coarse), static_cast<double>(o.points), o.from, o.to, o.pedal,
                               o.tolerance, static_cast<double>(o.minCycles), static_cast<double>(o.maxCycles),
                               o.sampleRate, static_cast<double>(o.block), o.startSeconds, o.warm ? 1.0 : 0.0, o.warmupSeconds, o.settleExhaust ? 1.0 : 0.0, o.holdSeconds, o.brake == "host" ? 1.0 : o.brake == "program" ? 2.0 : 0.0})
            h = mix(h, v);
        return h;
    }

    bool predictCurve(const std::shared_ptr<Sim>& sim, const Build& build, const PredictOptions& options,
                      PredictCurve& curve, std::string& error, const std::atomic<bool>* cancel)
    {
        return makeCurve(sim, build, options, curve, error, cancel, {});
    }

    bool predictRamp(const std::shared_ptr<Sim>& sim, const Build& build, const PredictOptions& options, double rate,
                     PredictCurve& curve, std::string& error)
    {
        const Clock::time_point t0 = Clock::now();
        curve = PredictCurve{};
        curve.key = predictKey(build, options);
        std::vector<double> speeds;
        if (!speedsOf(sim, build, options, speeds, error))
            return false;
        Rig rig(sim, options);
        if (!rig.make(build, error))
            return false;
        rig.start(speeds.front());
        rig.warmup(speeds);
        curve.points = rig.ramp(speeds, std::max(rate, 1e-3));
        curve.refined = true;
        curve.brake = rig.programBrake() ? "program" : "host";
        curve.simulatedSeconds = rig.simulated();
        curve.wallSeconds = seconds(t0, Clock::now());
        return true;
    }

    // ---- the service ------------------------------------------------------------------------------------------------

    PredictService::PredictService(std::shared_ptr<Sim> sim, PredictOptions options)
        : sim_(std::move(sim)), options_(options), threads_(workersFor(options))
    {
        thread_ = std::thread([this] { coordinator(); });
    }

    PredictService::~PredictService()
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

    std::uint64_t PredictService::request(const Build& build)
    {
        const std::uint64_t key = predictKey(build, options_);
        const std::lock_guard<std::mutex> lock(mutex_);
        const std::uint64_t number = ++number_;
        cancel_ = true;   // the one running is older
        for (auto it = cache_.begin(); it != cache_.end(); ++it)
            if (it->key == key)
            {
                done_ = *it;
                done_.request = number;
                done_.cached = true;
                done_.wallSeconds = 0.0;
                ready_ = true;
                error_.clear();
                cache_.splice(cache_.begin(), cache_, it);
                waiting_ = false;
                revision_.fetch_add(1);
                return number;
            }
        json_ = build.save();
        key_ = key;
        waiting_ = true;
        ready_ = false;
        wake_.notify_all();
        revision_.fetch_add(1);
        return number;
    }

    void PredictService::cancel()
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        ++number_;
        waiting_ = false;
        ready_ = false;
        cancel_ = true;
        revision_.fetch_add(1);
    }

    bool PredictService::take(PredictCurve& out)
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        if (!ready_)
            return false;
        out = done_;
        ready_ = false;
        return true;
    }

    std::string PredictService::error() const
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        return error_;
    }

    void PredictService::coordinator()
    {
        while (true)
        {
            std::string json;
            std::uint64_t number = 0, key = 0;
            {
                std::unique_lock<std::mutex> lock(mutex_);
                wake_.wait(lock, [this] { return stop_ || waiting_; });
                if (stop_)
                    return;
                json = json_;
                number = number_;
                key = key_;
                waiting_ = false;
                cancel_ = false;
            }
            busy_ = true;
            progress_ = 0.0;
            revision_.fetch_add(1);
            Build build(sim_->fields());
            std::vector<std::string> warnings;
            std::string error;
            PredictCurve curve;
            const auto partial = [&](const PredictCurve& p, double share) {
                const std::lock_guard<std::mutex> lock(mutex_);
                if (number != number_)
                    return;
                progress_ = share;
                done_ = p;
                done_.request = number;
                ready_ = true;
                revision_.fetch_add(1);
            };
            const bool ok = build.load(json, BuildPart::Whole, warnings, error)
                            && makeCurve(sim_, build, options_, curve, error, &cancel_, partial);
            {
                const std::lock_guard<std::mutex> lock(mutex_);
                if (number == number_)
                {
                    if (!ok)
                        error_ = error;
                    else if (curve.refined)
                    {
                        curve.key = key;
                        curve.request = number;
                        cache_.push_front(curve);
                        while (cache_.size() > std::max<std::size_t>(options_.cacheSize, 1))
                            cache_.pop_back();
                        done_ = curve;
                        ready_ = true;
                        error_.clear();
                        progress_ = 1.0;
                    }
                }
                busy_ = waiting_;
            }
            revision_.fetch_add(1);
        }
    }
} // namespace tee
