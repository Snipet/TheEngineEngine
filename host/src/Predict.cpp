// host/src/Predict.cpp: the predicted curve (Predict.h).
#include "tee/Predict.h"

#include "tee/Denormals.h"
#include "tee/Instance.h"
#include "tee/Timeline.h"
#include "tee/Warm.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <functional>
#include <limits>

namespace tee
{
    namespace
    {
        constexpr double kFourPi = 12.566370614359172;   // a cycle: two turns of the crank, whatever the engine
        // the sections besides the exhaust whose spec.<section>.heatSettle the one settle raises, where the program has them
        constexpr const char* kOtherHeatSettles[] = {"intake", "boost"};
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
                hasFlags_ = find("tel.engine.flags", flagsH_);
                hasRetard_ = find("ecu.one.knock.retard", retardH_);
                // a part with hysteresis (a two-stage cam today), as the program states it in one place
                hasHysteresis_ = find("tel.engine.hysteresis", hysteresisH_) && find("tel.engine.latched", latchedH_);
                hasBoost_ = build.fieldOf("boost.kind") >= 0 && build.get("boost.kind") != 0.0
                            && find("tel.boost.pressure", boostH_);
                for (const std::string& name : options_.fields)
                {
                    const FieldInfo* f = nullptr;
                    std::size_t at = 0;
                    if (!telemetryElement(fields, name, f, at, error))
                        return false;
                    extra_.push_back({f, at});
                }
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
                    // the exhaust's own settle (G, 859bc7d): a counter whose increase puts its gas and walls where the
                    // present flow holds them, both models; else the host's secant on the reference's exhaust state
                    const int section = fields.sectionOf("exhaust");
                    heatSettle_ = find("spec.exhaust.heatSettle", heatSettleH_) && section >= 0;
                    if (heatSettle_)
                        settles_.push_back({heatSettleH_, fields.sections()[static_cast<std::size_t>(section)].revOffset});
                    else if (find("exh.tPort", h))
                        slow_.push_back({h, 0, 0, 1.0, 250.0, 1500.0, 0.0, 0.0, 0.0});
                    // the reference's turbo (boost.model 0): its shaft's speed, 0 to 1 (boost/boost.fm), spooling over
                    // seconds; the boost machine (model 1) keeps its own state
                    const bool machine = build.fieldOf("boost.model") >= 0 && build.get("boost.model") != 0.0;
                    if (!machine && build.fieldOf("boost.kind") >= 0 && build.get("boost.kind") != 0.0
                        && find("boost.spool", h))
                        slow_.push_back({h, 0, 0, 0.15 / 60.0, 0.0, 1.0, 0.0, 0.0, 0.0});
                    if (!heatSettle_ && find("exh.tWall", h))
                        for (std::size_t i = 0; i < h.count; ++i)
                            slow_.push_back({h, i, 1, 1.0, 250.0, 1500.0, 0.0, 0.0, 0.0});
                    // the other sections' settles where the program has them, raised with the exhaust's: the intake's
                    // charge heating (spec.intake.heatSettle), the boost's intercooler (spec.boost.heatSettle)
                    for (const char* other : kOtherHeatSettles)
                    {
                        const int sec = fields.sectionOf(other);
                        if (sec >= 0 && find(("spec." + std::string(other) + ".heatSettle").c_str(), h))
                        {
                            settles_.push_back({h, fields.sections()[static_cast<std::size_t>(sec)].revOffset});
                            heatSettle_ = true;
                        }
                    }
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

            // The key on for a quarter of a second before it is turned to start, as a person turns it (an ECU with a
            // key's state machine, a diesel's glow plugs, want on before start: from off straight to start the new
            // ECU never settled its boost and knock as it does after a proper start)
            void keyOn(double speed)
            {
                set(ignition_, 1.0);
                set(throttle_, 0.0);
                for (long done = 0; done < std::lround(0.25 * options_.sampleRate); done += options_.block)
                {
                    if (holdCrank_ && !hasTorque_)
                        set(omega_, 0.0);
                    run(options_.block);
                }
                (void)speed;
            }

            // The engine started (held at 1000 rpm until it runs, then 50 ms more: the simulator says its redline and
            // idle for the build only once it runs); then the redline and the idle as it says them.
            void probe(double& idle, double& redline)
            {
                const double w = 1000.0 * kFourPi / 120.0;
                keyOn(w);
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
                keyOn(speed);
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

            // A part with hysteresis (tel.engine.hysteresis: a two-stage cam's switch today; a power valve, a runner flap
            // tomorrow) holds its state across a band of speed, so a point in the band would read the state of the way
            // the rig came: from a point above it, the state left latched; stepped up from below, the absorber's loop
            // carrying the speed over the switch (a critically damped loop overshoots a step in its set speed by some
            // 13 %: 430 to 601 rad/s peaks near 624, over the two-stage's 612.6). So each point is reached as an upward
            // sweep reaches it (a published full-load curve's way): while the program says a state is latched that an
            // upward sweep would not hold here (tel.engine.latched), the speed is let down in steps until it is not;
            // then any rise into the point is a ramp of 100 rad/s^2 (some 950 rpm a second, a dyno's sweep: judgement),
            // which the loop follows within a rad/s. Card N1's find and cure (25 Nov: P's 13 N m notch at 5740 rpm on
            // the two-stage), generalised. A build with no such part (or a program without the fields) is untouched,
            // bit for bit. False when a latched state would not let go (the point says so: approachForced).
            bool approach(double speed)
            {
                if (!hasHysteresis_ || instance_->read(hysteresisH_) < 0.5)
                    return true;
                double from = instance_->read(speed_);
                bool released = true;
                if (from > speed)
                {
                    // from above: down to the point first (a step down crosses no up switch), so that what the program
                    // says is latched is said at the point's speed (the two-stage's latch is "stage B under its up
                    // switch": over the switch, where the rig was, it reads 0)
                    for (long done = 0; done < std::lround(0.1 * options_.sampleRate); done += options_.block)
                        step(speed, options_.block);
                    from = speed;
                }
                if (instance_->read(latchedH_) >= 0.5)
                {
                    // down in steps of 5 % under the point, a quarter second each, to a floor of half its speed
                    released = false;
                    for (double at = std::min(from, speed) * 0.95; at >= 0.5 * speed && !released; at *= 0.95)
                    {
                        from = at;
                        for (long done = 0; done < std::lround(0.25 * options_.sampleRate) && !released;
                             done += options_.block)
                        {
                            step(at, options_.block);
                            released = instance_->read(latchedH_) < 0.5;
                        }
                    }
                }
                if (from < speed)
                {
                    const double dt = static_cast<double>(options_.block) / options_.sampleRate;
                    const long n = std::max(1L, std::lround((speed - from) / (100.0 * dt)));
                    for (long k = 1; k <= n; ++k)
                    {
                        if (options_.warm && k % 64 == 1)
                            warm();   // held as the warm-up and the points hold it
                        step(from + (speed - from) * static_cast<double>(k) / static_cast<double>(n), options_.block);
                    }
                }
                return released;
            }

            // One point: held at `speed`, cycle after cycle until two agree or the most are run.
            PredictPoint point(double speed, const std::atomic<bool>* cancel)
            {
                const bool released = approach(speed);
                PredictPoint p;
                p.speed = speed;
                p.approachForced = !released;
                const double perCycle = kFourPi / std::max(speed, 1.0) * options_.sampleRate;   // samples
                double previous = 0.0, held = 0.0;
                faulted_ = false;
                // the exhaust's settle (after the first cycle at this speed) wants some 0.1 s before a measure
                const int afterSettle = heatSettle_ ? static_cast<int>(std::ceil(0.1 * speed / kFourPi)) : 0;
                const int settling = std::max(slow_.empty() ? 0 : fresh_ ? 8 : 6, afterSettle);
                // a build whose knock control acts, or whose turbo spools, gets as many cycles as its retard and its
                // boost need, to some three seconds
                const int knockMost = hasRetard_ || hasBoost_
                                          ? std::max(options_.maxCycles, static_cast<int>(3.0 * speed / kFourPi)) : 0;
                const int holdMost = static_cast<int>((options_.minHoldSeconds + 1.0) * speed / kFourPi);
                const int window = std::max(2, static_cast<int>(std::lround(options_.knockWindowSeconds * speed / kFourPi)));
                const int knockLimitedMost = static_cast<int>(options_.knockMostSeconds * speed / kFourPi) + settling;
                int most = options_.holdSeconds > 0.0 ? 1 << 30
                                                      : std::max({options_.maxCycles, knockMost, holdMost}) + settling;
                std::vector<double> retards;   // the counted cycles' retards
                std::vector<double> boosts;    // and boosts
                double lastRetard = knockRetard();
                double lastBoost = boostNow();
                std::vector<double> counted;   // the counted cycles' torques
                std::vector<char> limitedCycles;   // and whether the limiter acted in each
                bool resettled = false;
                for (int c = 0; c < most; ++c)
                {
                    if (cancel != nullptr && cancel->load(std::memory_order_relaxed))
                        return p;
                    if (options_.warm)
                        warm();   // the cell holds the engine at its working temperatures
                    double sum = 0.0;
                    long run = 0;
                    const long total = std::lround(perCycle * (c + 1)) - std::lround(perCycle * c);
                    limiting_ = false;
                    while (run < total)
                    {
                        const int k = static_cast<int>(std::min<long>(options_.block, total - run));
                        sum += step(speed, k) * k;
                        run += k;
                    }
                    const double t = sum / static_cast<double>(std::max(total, 1L));
                    held += total / options_.sampleRate;
                    if (faulted_)
                    {
                        // the engine diverged and is at rest: the point is not a measure
                        p.valid = false;
                        p.settled = false;
                        p.torque = std::numeric_limits<double>::quiet_NaN();
                        p.power = p.torque;
                        return p;
                    }
                    // the settling cycles (above), after the first; their torque is not counted
                    if (c >= 1 && c <= settling)
                    {
                        if (!slow_.empty() && c - 1 < (fresh_ ? 8 : 6))
                            settleStep(c - 1, total / options_.sampleRate);
                        p.cycles = c + 1;
                        p.history.push_back(t);
                        continue;
                    }
                    if (settling > 0 && c == 0)
                    {
                        if (!slow_.empty())
                            settleBegin();
                        if (heatSettle_)
                            askHeatSettle();
                    }
                    p.cycles = c + 1;
                    p.history.push_back(t);
                    // the cycles counted: the first after the speed (or the settling) moved is not
                    const int k = c == 0 ? 0 : c - settling;
                    if (k >= 1)
                    {
                        p.torque = 0.5 * (t + previous);
                        // knock control's retard (ecu/knockctl.fm: a step a knock, 2 degrees a second given back)
                        // settles over seconds: a point is not settled while it still moves
                        const double retard = knockRetard();
                        const bool retardSteady = !hasRetard_ || std::fabs(retard - lastRetard) <= 0.25;
                        lastRetard = retard;
                        // a turbo's boost after a tip-in or a change of speed overshoots, the gate pulls it back over
                        // some 0.5 s and it settles over some 2 (N2's measures): steady within 1 % a cycle
                        const double boost = boostNow();
                        const bool boostSteady =
                            !hasBoost_ || std::fabs(boost - lastBoost) <= 0.01 * std::max(std::fabs(boost), 20000.0);
                        lastBoost = boost;
                        // the torque itself has stopped moving: the last two cycles agree, and the mean of the last three
                        // agrees with the three before (no trend: near a runner's resonance the intake's standing wave builds or
                        // decays over many cycles, and two cycles of a slow slide can agree by chance)
                        counted.push_back(t);
                        limitedCycles.push_back(limiting_ ? 1 : 0);
                        retards.push_back(retard);
                        boosts.push_back(boost);
                        if (hasRetard_ && retard >= options_.knockLimit && !p.knockLimited)
                        {
                            p.knockLimited = true;   // from now on the mean over whole windows (Predict.h)
                            if (options_.holdSeconds <= 0.0)
                                most = std::max(most, knockLimitedMost);
                        }
                        // a boost still moving after a second and a half here hunts (its gate's loop a limit cycle, as
                        // knock control's retard is): the mean over whole windows too
                        if (hasBoost_ && !boostSteady && held >= options_.boostHuntSeconds && !p.boostHunting)
                        {
                            p.boostHunting = true;
                            if (options_.holdSeconds <= 0.0)
                                most = std::max(most, knockLimitedMost);
                        }
                        if (p.knockLimited || p.boostHunting)
                        {
                            const std::size_t m = counted.size(), w = static_cast<std::size_t>(window);
                            if (m >= 2 * w)
                            {
                                double last = 0.0, before = 0.0;
                                for (std::size_t i = 0; i < w; ++i)
                                {
                                    last += counted[m - 1 - i];
                                    before += counted[m - 1 - w - i];
                                }
                                last /= static_cast<double>(w);
                                before /= static_cast<double>(w);
                                p.torque = 0.5 * (last + before);
                                p.retardMin = *std::min_element(retards.end() - static_cast<long>(2 * w), retards.end());
                                p.retardMax = *std::max_element(retards.end() - static_cast<long>(2 * w), retards.end());
                                double r = 0.0;
                                for (std::size_t i = m - 2 * w; i < m; ++i)
                                    r += retards[i];
                                p.retardMean = r / static_cast<double>(2 * w);
                                bool agree = std::fabs(last - before)
                                             <= options_.tolerance * std::max(std::fabs(p.torque), 1.0);
                                if (hasBoost_)
                                {
                                    // the boost's windows agree too (a hunting boost's mean, not each cycle's)
                                    double bl = 0.0, bb = 0.0;
                                    for (std::size_t i = 0; i < w; ++i)
                                    {
                                        bl += boosts[m - 1 - i];
                                        bb += boosts[m - 1 - w - i];
                                    }
                                    agree = agree && std::fabs(bl - bb) / static_cast<double>(w)
                                                         <= 0.01 * std::max(std::fabs(bl / static_cast<double>(w)), 20000.0);
                                }
                                if (agree && (boostSteady || p.boostHunting) && held >= options_.holdSeconds
                                    && held >= options_.minHoldSeconds)
                                {
                                    if (heatSettle_ && !resettled)
                                    {
                                        askHeatSettle();
                                        resettled = true;
                                        counted.clear();
                                        limitedCycles.clear();
                                        retards.clear();
                                        boosts.clear();
                                        continue;
                                    }
                                    p.settled = true;
                                    p.torqueSe = 0.5 * std::fabs(last - before);
                                    p.settledCycles = static_cast<int>(2 * w);
                                    break;
                                }
                            }
                            else
                            {
                                double all = 0.0;
                                for (const double v : counted)
                                    all += v;
                                p.torque = all / static_cast<double>(m);
                            }
                            previous = t;
                            continue;
                        }
                        const double scale = options_.tolerance * std::max(std::fabs(p.torque), 1.0);
                        const std::size_t m = counted.size();
                        const bool noTrend =
                            m >= 6
                            && std::fabs((counted[m - 1] + counted[m - 2] + counted[m - 3])
                                         - (counted[m - 4] + counted[m - 5] + counted[m - 6]))
                                       / 3.0
                                   <= scale;
                        const bool steady = k + 1 >= options_.minCycles && held >= options_.holdSeconds
                                            && held >= options_.minHoldSeconds && retardSteady
                                            && boostSteady && noTrend && std::fabs(t - previous) <= scale;
                        if (steady && heatSettle_ && !resettled)
                        {
                            // the exhaust settled again, now that the flow is this speed's (the first settle, a cycle
                            // after the change of speed, saw the gas still moving), and the torque watched again
                            askHeatSettle();
                            resettled = true;
                            counted.clear();
                            limitedCycles.clear();
                            retards.clear();
                            boosts.clear();
                            continue;
                        }
                        if (steady)
                        {
                            p.settled = true;
                            settledMean(counted, p);
                            break;
                        }
                    }
                    else
                    {
                        p.torque = t;
                        counted.push_back(t);
                        limitedCycles.push_back(limiting_ ? 1 : 0);
                        retards.push_back(knockRetard());
                        boosts.push_back(boostNow());
                    }
                    previous = t;
                }
                // a point that did not settle in its time: the mean over its last windows' worth of cycles (two cycles
                // of a limit cycle are no measure of it), said by settled false
                if (!p.settled && p.valid && counted.size() >= 4)
                {
                    const std::size_t n = std::min(counted.size(), static_cast<std::size_t>(2 * window));
                    double all = 0.0;
                    for (std::size_t i = counted.size() - n; i < counted.size(); ++i)
                        all += counted[i];
                    p.torque = all / static_cast<double>(n);
                }
                // the limiter acted in a counted cycle: the torque is the cut's, no measure of the engine's
                p.limited = std::find(limitedCycles.begin(), limitedCycles.end(), 1) != limitedCycles.end();
                p.power = p.torque * speed;
                p.knockRetard = knockRetard();
                if (!p.knockLimited)
                    p.retardMean = p.knockRetard;
                for (const auto& [f, at] : extra_)
                    p.fields.push_back(readScalar(static_cast<const unsigned char*>(instance_->data()) + f->offset
                                                      + at * scalarBytes(f->scalar),
                                                  f->scalar));
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
            // Engine::rate, the engine's own rate (0: the program has no such field)
            double innerRate() const
            {
                Schema::Handle h;
                std::string e;
                return sim_->schema().find("rate", h, e) ? instance_->read(h) : 0.0;
            }

        private:
            void set(const Schema::Handle& h, double v) noexcept { instance_->write(h, v); }

            // n samples held at `speed`; the mean torque the brake took over them (N m)
            // each heatSettle the program has (the exhaust's, the intake's, the boost's) raised by one, and its section's
            // rev, so the simulator takes it
            void askHeatSettle()
            {
                unsigned char* data = static_cast<unsigned char*>(instance_->data());
                for (const auto& [handle, revOffset] : settles_)
                {
                    instance_->write(handle, instance_->read(handle) + 1.0);
                    std::int32_t rev = 0;
                    std::memcpy(&rev, data + revOffset, 4);
                    ++rev;
                    std::memcpy(data + revOffset, &rev, 4);
                }
            }

            double boostNow() const { return hasBoost_ ? instance_->read(boostH_) : 0.0; }

            // The knock retard's mean over the retarded cylinders, degrees (0 without knock control's state)
            double knockRetard() const
            {
                if (!hasRetard_)
                    return 0.0;
                double sum = 0.0;
                int n = 0;
                for (std::size_t i = 0; i < retardH_.count; ++i)
                    if (const double r = instance_->read(retardH_, i); r > 0.0)
                    {
                        sum += r;
                        ++n;
                    }
                return n > 0 ? sum / n : 0.0;   // over the cylinders that are retarded (16 slots, as many in use)
            }

            // FLAG_FAULT (4) in tel.engine.flags: the simulator found its engine diverged and put it at rest this call
            void noteFault()
            {
                if (!hasFlags_)
                    return;
                const long flags = static_cast<long>(instance_->read(flagsH_));
                if ((flags & 4) != 0)
                    faulted_ = true;
                if ((flags & 1) != 0)   // FLAG_LIMITER: the rev limiter (a diesel's governor) holds it
                    limiting_ = true;
            }

        public:
            // Whether a fault was seen since the last call (and forget it).
            bool takeFault() noexcept
            {
                const bool f = faulted_;
                faulted_ = false;
                return f;
            }

        private:
            // A settled point's torque: the mean of its settled cycles, with its standard error. The settled cycles are
            // the counted ones since counting last began (the speed's settling, or the heat's resettle), less the
            // leading ones while the rest's first and second halves differ by more than the tolerance (the stop's own
            // no-trend test, over the whole run), at least six. The last two alone carried the engine's cycle-to-cycle
            // scatter (0.5 to 0.8 % a cycle on a four: N1's measure), so neighbouring points differed by 1.5 % at random.
            void settledMean(const std::vector<double>& cycles, PredictPoint& p) const
            {
                const std::size_t n = cycles.size();
                if (n < 6)
                    return;   // (the stop needs six: not reached)
                const auto mean = [&](std::size_t from, std::size_t to) {
                    double sum = 0.0;
                    for (std::size_t i = from; i < to; ++i)
                        sum += cycles[i];
                    return sum / static_cast<double>(to - from);
                };
                std::size_t first = 0;
                while (n - first > 6)
                {
                    const std::size_t mid = first + (n - first) / 2;
                    const double a = mean(first, mid), b = mean(mid, n);
                    if (std::fabs(a - b) <= options_.tolerance * std::max(std::fabs(0.5 * (a + b)), 1.0))
                        break;
                    ++first;
                }
                const double m = mean(first, n);
                double squares = 0.0;
                for (std::size_t i = first; i < n; ++i)
                    squares += (cycles[i] - m) * (cycles[i] - m);
                const std::size_t k = n - first;
                p.torque = m;
                p.torqueSe = std::sqrt(squares / static_cast<double>(k - 1)) / std::sqrt(static_cast<double>(k));
                p.settledCycles = static_cast<int>(k);
            }

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
                    noteFault();
                    return instance_->read(absorber_ ? absorberH_ : torque_);
                }
                set(omega_, speed);
                sim_->process()(data, left_.data(), right_.data(), n);
                samples_ += n;
                noteFault();
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
                 absorber_ = false, hasFlags_ = false, faulted_ = false, hasRetard_ = false, limiting_ = false;
            Schema::Handle flagsH_, retardH_, heatSettleH_;
            bool hasHysteresis_ = false;
            Schema::Handle hysteresisH_, latchedH_;
            std::vector<std::pair<const FieldInfo*, std::size_t>> extra_;   // PredictOptions::fields
            bool heatSettle_ = false, hasBoost_ = false;
            Schema::Handle boostH_;
            std::vector<std::pair<Schema::Handle, std::size_t>> settles_;   // each heatSettle and its section's rev
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
            if (!options.speeds.empty())
            {
                speeds = options.speeds;
                return true;
            }
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
                    to = redline > from ? redline - options.redlineMarginRpm * kFourPi / 120.0 : from * 6.0;
            }
            const int n = std::max(options.points, 2);
            speeds.resize(static_cast<std::size_t>(n));
            for (int i = 0; i < n; ++i)
                speeds[static_cast<std::size_t>(i)] = from + (to - from) * i / (n - 1);
            return true;
        }

        // The curve, with `partial` called (on a worker's thread, under no lock of ours) once the coarse speeds are in
        // and after each speed past them.
        bool makeCurve(const std::shared_ptr<Sim>& programSim, const Build& build, const PredictOptions& options,
                       PredictCurve& curve, std::string& error, const std::atomic<bool>* cancel,
                       const std::function<void(const PredictCurve&, double)>& partial,
                       const std::shared_ptr<Sim>& inner)
        {
            // the engine's own rate: the inner Sim's where it was given and the build allows it
            const bool machine = build.fieldOf("boost.model") >= 0 && build.get("boost.model") != 0.0;
            const std::shared_ptr<Sim>& sim = inner && !machine ? inner : programSim;
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
            std::atomic<double> simulated{0.0};
            std::string firstError;
            bool program = false;
            double rateInside = 0.0;

            const auto publish = [&](bool final) {
                // under `mutex`
                PredictCurve p;
                p.key = curve.key;
                for (int i = 0; i < n; ++i)
                    if (have[static_cast<std::size_t>(i)])
                        p.points.push_back(done[static_cast<std::size_t>(i)]);
                p.refined = final;
                p.brake = program ? "program" : "host";
                p.innerRate = rateInside;
                for (const PredictPoint& q : p.points)
                    p.faults += q.valid ? 0 : 1;
                p.wallSeconds = seconds(t0, Clock::now());
                p.simulatedSeconds = simulated.load();
                return p;
            };

            // Each worker takes a fixed share of the speeds, the order's k-th, (k + threads)-th, ...: what a worker
            // runs before a point is part of that point's history (a diesel's lighting, a turbo's spool carry from one
            // speed to the next), so a share by who comes first would make the same build's curve differ from run to
            // run (the large diesel's 1300 to 1650 rpm by up to 15 % of its peak).
            // The shares are fixed (PredictOptions::shares), not the workers: a share's points run in turn on an instance
            // of its own, and a worker runs whole shares, so what came before a point (its history) is the same on a
            // machine of two cores as of sixteen (with a share a worker, a 2-core runner and this one read one point of
            // the economy four 126 and 131 N m)
            const int shares = std::clamp(options.shares, 1, n);
            const int threads = std::min(workersFor(options), shares);
            const auto work = [&](int worker) {
                // this worker's shares, each its own instance; their points taken in the order's sequence (the coarse
                // speeds first, whatever the number of workers)
                std::vector<int> mine;
                for (int shareAt = worker; shareAt < shares; shareAt += threads)
                    mine.push_back(shareAt);
                std::vector<std::unique_ptr<Rig>> rigs;
                std::vector<bool> started(mine.size(), false);
                for (std::size_t r = 0; r < mine.size(); ++r)
                {
                    rigs.push_back(std::make_unique<Rig>(sim, options));
                    std::string e;
                    if (!rigs.back()->make(build, e))
                    {
                        const std::lock_guard<std::mutex> lock(mutex);
                        if (firstError.empty())
                            firstError = e;
                        return;
                    }
                }
                bool stop = false;
                for (int k0 = 0; k0 < n && !stop; k0 += shares)
                    for (std::size_t r = 0; r < mine.size() && !stop; ++r)
                    {
                        const int k = k0 + mine[r];
                        if (k >= n)
                            continue;
                        if (cancel != nullptr && cancel->load(std::memory_order_relaxed))
                        {
                            stop = true;
                            break;
                        }
                        Rig& rig = *rigs[r];
                        const int i = order[static_cast<std::size_t>(k)];
                        const double w = speeds[static_cast<std::size_t>(i)];
                        if (!started[r])
                        {
                            rig.start(speeds.front());   // the lowest: where an engine lights best
                            rig.warmup(speeds);
                            started[r] = true;
                        }
                        const PredictPoint p = rig.point(w, cancel);
                        if (options.onPoint)
                            options.onPoint();
                        if (!p.valid)
                            started[r] = false;   // at rest after the fault: the next point starts it again
                        if (cancel != nullptr && cancel->load(std::memory_order_relaxed))
                        {
                            stop = true;
                            break;
                        }
                        PredictCurve snapshot;
                        double share = 0.0;
                        bool tell = false;
                        {
                            const std::lock_guard<std::mutex> lock(mutex);
                            program = rig.programBrake();
                            rateInside = rig.innerRate();
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
                double total = 0.0;
                for (const auto& rig : rigs)
                    total += rig->simulated();
                double s = simulated.load();
                while (!simulated.compare_exchange_weak(s, s + total))
                {
                }
            };

            std::vector<std::thread> pool;
            for (int k = 1; k < threads; ++k)
                pool.emplace_back(work, k);
            work(0);
            for (std::thread& t : pool)
                t.join();

            if (haveAll < n && !firstError.empty())
            {
                error = firstError;
                return false;
            }
            // cancelled before every point was in: no curve. (It returned true with the points it had finished, and a
            // caller that reads a curve as whole, the cam bake's rows, indexed the missing ones: the owner's app on 25 and
            // 26 Nov, a read from address 0 in bakeIntakeCam whenever an edit landed while a cam map baked.) The
            // PredictService discards a cancelled curve by its request number already (request() and cancel() both
            // raise it), so it sees no change; its destructor's cancel leaves an error no one reads.
            if (haveAll < n && cancel != nullptr && cancel->load(std::memory_order_relaxed))
            {
                error = "cancelled";
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
                               o.sampleRate, static_cast<double>(o.block), o.startSeconds, o.warm ? 1.0 : 0.0, o.warmupSeconds, o.settleExhaust ? 1.0 : 0.0, o.holdSeconds, o.brake == "host" ? 1.0 : o.brake == "program" ? 2.0 : 0.0, o.innerRate, static_cast<double>(o.fields.size()), o.minHoldSeconds, o.knockLimit, o.knockWindowSeconds, o.knockMostSeconds, o.boostHuntSeconds, static_cast<double>(o.shares), o.redlineMarginRpm})
            h = mix(h, v);
        for (const double v : o.speeds)
            h = mix(h, v);
        return h;
    }

    bool predictCurve(const std::shared_ptr<Sim>& sim, const Build& build, const PredictOptions& options,
                      PredictCurve& curve, std::string& error, const std::atomic<bool>* cancel,
                      const std::shared_ptr<Sim>& inner)
    {
        return makeCurve(sim, build, options, curve, error, cancel, {}, inner);
    }

    std::shared_ptr<Sim> simAtInnerRate(const std::shared_ptr<Sim>& sim, double rate, std::string& error)
    {
        Sim::Options o = sim->options();
        // FMinus reads a macro only in #if and #ifdef: the program picks its rate by which is defined
        // (#ifdef TEE_ENGINE_RATE_48000 ... #else ... #endif)
        o.defines["TEE_ENGINE_RATE_" + std::to_string(static_cast<long>(std::lround(rate)))] = "1";
        std::shared_ptr<Sim> inner = Sim::compile(sim->directory(), o, error);
        if (!inner)
            return nullptr;
        // does it take the define? Engine::rate after init
        Schema::Handle h;
        std::string e;
        if (!inner->schema().find("rate", h, e))
        {
            error = "the program has no Engine::rate to check its inner rate by";
            return nullptr;
        }
        Instance instance(inner->module());
        inner->writeDefaults(instance.data());
        inner->init()(instance.data(), 48000.0f);
        const double seen = instance.read(h);
        inner->end()(instance.data());
        if (std::fabs(seen - rate) > 0.5)
        {
            error = "the program does not take TEE_ENGINE_RATE_" + std::to_string(static_cast<long>(std::lround(rate)))
                    + " (its engine runs at " + std::to_string(seen) + " Hz)";
            return nullptr;
        }
        return inner;
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
        busy_->store(false);   // gone: whoever still watches the flag waits no more
    }

    std::uint64_t PredictService::request(const Build& build)
    {
        const std::uint64_t key = predictKey(build, options_);
        const std::lock_guard<std::mutex> lock(mutex_);
        const std::uint64_t number = ++number_;
        made_ = 0;
        cancel_ = true;   // the one running is older
        for (auto it = cache_.begin(); it != cache_.end(); ++it)
            if (it->key == key)
            {
                done_ = *it;
                done_.request = number;
                done_.cached = true;
                done_.wallSeconds = 0.0;
                ready_ = true;
                made_ = 1;
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
            busy_->store(true);
            progress_ = 0.0;
            revision_.fetch_add(1);
            if (options_.innerRate > 0.0 && !innerTried_)
            {
                // once: the program again at the inner rate; a program that does not take it runs at its own
                innerTried_ = true;
                std::string why;
                inner_ = simAtInnerRate(sim_, options_.innerRate, why);
            }
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
                ++made_;
                revision_.fetch_add(1);
            };
            const bool ok = build.load(json, BuildPart::Whole, warnings, error)
                            && makeCurve(sim_, build, options_, curve, error, &cancel_, partial, inner_);
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
                        ++made_;
                        error_.clear();
                        progress_ = 1.0;
                    }
                }
                busy_->store(waiting_);
            }
            revision_.fetch_add(1);
        }
    }
} // namespace tee
