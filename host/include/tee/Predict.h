// host/include/tee/Predict.h: the predicted full-load torque and power curve of a build, made in the background a moment
// after each edit (the dyno ribbon of docs/plan/ui-design.md, section 2: the application's signature).
//
// What it does, and why it is fast enough. A ramp sweep of the real engine runs at a few times real time on one core
// (the V8 at 2 to 4 times), far too slow for "a moment after each edit". This makes steady points instead: the crank is
// held at each of a dozen or so speeds, the engine at full throttle settles for a few of its cycles (two turns of the
// crank, whatever the engine), and the torque is the mean over whole cycles once two cycles in a row agree. The speeds
// are shared out to several worker threads, each with an instance of its own (never the audio thread: the engine the
// person hears keeps playing), coarse speeds first (the ribbon draws those at once), the rest after. A newer request
// cancels the one running at its next block; curves are kept by the build's hash, so an undo answers at once.
//
// The brake. The host's, where it can: after every 32 samples the crank's speed (crank.omega) is put back where it is
// held, and the torque is the inertia (mech.jBase, the flywheel and bench-load dials, as Mechanics::inertia has it)
// times the speed the engine gained over the time: exactly what the bench's step did with that torque, the speed held
// exactly, nothing to settle after a change of speed. The program's own dyno otherwise (tel.engine.torque, ctl.mode
// MODE_DYNO_ENGINE: core/dyno.fm's absorber, its torque dyno.tqAbs averaged over whole cycles): a custom engine
// (engine.custom on) steps its crank with an inertia that varies round the turn, which the host does not model. The
// program's absorber is a 6 Hz speed loop: after a change of speed it needs some 5 to 10 cycles more to settle, and
// near the redline its overshoot meets the limiter. The curve says which brake made it (PredictCurve::brake).
//
// The engine is warm and held so (the coolant at its thermostat, the oil and the chamber's walls as a warm engine's:
// heat.*, when the program has them, put back each cycle as a dyno cell's conditioning holds them): a dyno's curve is
// a warm engine's; the simulator would take minutes to warm, and at full load for minutes it overheats.
//
//   tee::PredictService predict(sim);                    // once; its threads wait
//   predict.request(build);                              // a moment after each edit: the newest wins
//   tee::PredictCurve curve;
//   if (predict.take(curve)) ribbon.show(curve);         // each frame: coarse first (curve.refined false), then refined
#pragma once

#include "tee/Build.h"
#include "tee/Sim.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace tee
{
    struct PredictOptions
    {
        int threads = 0;              // workers; 0: the machine's cores less one (for the audio and the UI), 1 to 6
        int coarse = 6;               // speeds in the first curve
        int points = 16;              // speeds in the refined curve (the coarse ones among them)
        double from = 0.0;            // rad/s; 0: 1000 rpm, or the idle's speed when that is higher
        double to = 0.0;              // rad/s; 0: just under the redline (tel.engine.redline)
        double pedal = 1.0;           // the throttle, 0 to 1
        double tolerance = 0.01;      // two cycles in a row within this share of each other: settled
        std::vector<std::string> fields;   // telemetry read at each point's end ("boost.pressure", "car.wheelSlip[2]")
        int minCycles = 3;            // cycles run at each speed at least (the first is never counted) ...
        int maxCycles = 14;           // ... and at most: a point that has not settled then says so
        // Which brake: "auto" (the host's where it knows the crank's inertia: the reference's crank, engine.custom off;
        // else the program's dyno), "host" or "program" (above).
        std::string brake = "auto";
        // The engine's own rate inside, for the curve only: 0, the program's (96 kHz); 48000 halves the work (the
        // performance session: the V8's and the default build's torque within 1.2 % at worst, 0.1 to 0.3 % typical,
        // only the top octave lost). It needs the program to take TEE_ENGINE_RATE_48000 (a define the service compiles
        // a second Sim with, once; FMinus reads macros only in #ifdef); a program that does not, or a build with boost.model 1 (its machine is built for
        // 96 kHz until it takes the rate), runs at the program's own rate. PredictCurve::innerRate says which.
        double innerRate = 0.0;
        double sampleRate = 96000.0;  // the instances' host rate: the simulator's own, so nothing is resampled
        int block = 32;               // samples a process call (the held brake's step)
        double startSeconds = 0.12;   // the key held at the first speed before full throttle
        bool warm = true;             // start warm (above)
        bool settleExhaust = true;    // bring the exhaust's slow temperatures to where they head (below)
        double holdSeconds = 0.0;     // engine time each point is held at least (a reference's: a minute)
        // Engine time each point runs at least, whatever its torque says: something about the engine (its charge's
        // and its walls' temperatures besides the exhaust's) settles over about a second after a change of speed, and
        // a point taken sooner reads some 3 % high near a runner's resonance (the V12 at 6500 to 6850 rpm against the
        // stepped dyno with its exhaust settled: +3.5 % at none, within 1.7 % at 1 s, 1 % at 2 s). The V8's curve
        // with it: first 0.78 s, refined 1.97 s on 3 workers.
        double minHoldSeconds = 1.0;
        double warmupSeconds = 0.0;   // engine time a fresh instance runs at full throttle mid-range before its first point
        std::size_t cacheSize = 64;   // curves kept by key
    };

    struct PredictPoint
    {
        double speed = 0.0;           // rad/s, where it was held
        double torque = 0.0;          // N m, the mean over the last two cycles
        double power = 0.0;           // W
        int cycles = 0;               // cycles run there
        bool settled = false;         // two cycles agreed within the tolerance
        // False when the simulator found its engine diverged during the point (FLAG_FAULT: put at rest afresh): its
        // torque and power are NaN, not to be plotted, and the curve counts it (PredictCurve::faults).
        bool valid = true;
        std::vector<double> fields;   // PredictOptions::fields' values at the point's end, in order
        double knockRetard = 0.0;     // degrees, knock control's retard at the point's end (mean over retarded cylinders)
        std::vector<double> history;  // each cycle's mean torque there, in order (the first after a change of speed)
    };

    struct PredictCurve
    {
        std::uint64_t key = 0;        // the build's hash with the options'
        std::uint64_t request = 0;    // the request it answers (PredictService::request's number)
        std::vector<PredictPoint> points;   // by speed
        bool refined = false;         // every speed is in (else: the coarse ones, and what is done of the rest)
        bool cached = false;          // answered from the cache
        std::string brake;            // "program": the simulator's dyno mode; "host": the crank held by the host
        double wallSeconds = 0.0;     // from the request to this curve
        double simulatedSeconds = 0.0;   // engine time run for it, over all workers
        double innerRate = 0.0;       // the engine's own rate it ran at (Engine::rate), 0 when the program has none
        int faults = 0;               // points the simulator's FLAG_FAULT made invalid: a failure of its invariant

        bool empty() const noexcept { return points.empty(); }
    };

    // The Sim `sim` compiled again with the engine's own rate at `rate` (the define TEE_ENGINE_RATE_<rate>, e.g. TEE_ENGINE_RATE_48000): nullptr, with
    // `error`, when it does not compile or does not take the define (its Engine::rate is not `rate`).
    std::shared_ptr<Sim> simAtInnerRate(const std::shared_ptr<Sim>& sim, double rate, std::string& error);

    // One curve, made now on this thread with `threads` workers (0 as PredictOptions says): what the service does, for
    // tools and tests. False with `error` when the program cannot be braked or played. `cancel` stops it early.
    // `inner`: the Sim at options.innerRate (simAtInnerRate), or nullptr (the program's own rate).
    bool predictCurve(const std::shared_ptr<Sim>& sim, const Build& build, const PredictOptions& options,
                      PredictCurve& curve, std::string& error, const std::atomic<bool>* cancel = nullptr,
                      const std::shared_ptr<Sim>& inner = nullptr);

    // The slow reference the steady points are measured against: one instance, the crank swept from the first speed
    // of `options` to the last at `rate` (rad/s^2) with the same brake, the torque at each of the curve's speeds the mean
    // over the two cycles about it. Slow by design: at 20 rad/s^2 a V8's curve is some 25 s of engine time.
    bool predictRamp(const std::shared_ptr<Sim>& sim, const Build& build, const PredictOptions& options, double rate,
                     PredictCurve& curve, std::string& error);

    class PredictService
    {
    public:
        explicit PredictService(std::shared_ptr<Sim> sim, PredictOptions options = {});
        ~PredictService();
        PredictService(const PredictService&) = delete;
        PredictService& operator=(const PredictService&) = delete;

        // ---- the UI thread ---------------------------------------------------------------------------------------
        // A curve for this build: the one running (if any) is cancelled at its next block and this one starts. Its
        // number, which the curves that answer it carry. From the cache when the build was seen: then take() has it at
        // once, refined.
        std::uint64_t request(const Build& build);
        // Stops what runs; nothing more is taken for it.
        void cancel();
        // The newest curve for the newest request since the last call: the coarse one, then each time more speeds are
        // in, then the refined one. False when there is nothing new.
        bool take(PredictCurve& out);
        bool busy() const noexcept { return busy_.load(); }
        double progress() const noexcept { return progress_.load(); }   // 0 to 1 of the speeds of the one running
        std::string error() const;    // why the newest request has no curve ("" : none)
        // Moves when a curve, the error or busy changes.
        std::uint64_t revision() const noexcept { return revision_.load(); }
        // The curves made for the newest request so far, coarse and partial ones with the refined: take() gives only
        // the newest of them, so a fast machine may hand out the refined one first; this counts them all.
        int curvesMade() const noexcept { return made_.load(); }

        int threads() const noexcept { return threads_; }
        const PredictOptions& options() const noexcept { return options_; }

    private:
        void coordinator();

        std::shared_ptr<Sim> sim_;
        std::shared_ptr<Sim> inner_;   // at options_.innerRate, compiled on the coordinator's first request
        bool innerTried_ = false;
        PredictOptions options_;
        int threads_ = 1;
        std::thread thread_;
        mutable std::mutex mutex_;
        std::condition_variable wake_;
        bool stop_ = false;
        bool waiting_ = false;
        std::string json_;
        std::uint64_t key_ = 0;
        std::uint64_t number_ = 0;
        std::atomic<bool> cancel_{false};
        std::atomic<bool> busy_{false};
        std::atomic<double> progress_{0.0};
        std::atomic<std::uint64_t> revision_{0};
        std::atomic<int> made_{0};
        std::list<PredictCurve> cache_;   // newest first
        bool ready_ = false;
        PredictCurve done_;
        std::string error_;
    };

    // The key of a build's curve under these options (what the cache holds it by).
    std::uint64_t predictKey(const Build& build, const PredictOptions& options);
} // namespace tee
