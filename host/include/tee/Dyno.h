// host/include/tee/Dyno.h: the dyno runner (card C, section 5). A copy of a build, a plan (an engine dyno's sweep
// from one speed to another at a rate, or steps each held for a settling time; the pedal), a warm-up, then the
// points: speed, torque, power, and every telemetry field asked for. It runs on an instance of its own, as fast as the
// machine allows (DynoCurve::realTime() says how many times real time that was).
//
// The simulator side (02-contracts.md, section 4): ctl.mode = MODE_DYNO_ENGINE (2) holds the crank at ctl.dynoSpeed
// and sweeps it at ctl.dynoRate (0 holds), the engine runs on its starter and ignition as on the road, and the
// absorbed torque is read from telemetry (DynoPlan::torque, "engine.torque" by default: the brake torque in N m). The
// chassis dyno (MODE_DYNO_CHASSIS) is the same with the car's own speed.
//
// DynoService is the interface's: it runs requests on a worker thread of its own, the newest only (a request made
// while one runs cancels it), and keeps the curves by the build's hash and the plan, so an edit undone costs nothing.
#pragma once

#include "tee/Build.h"
#include "tee/Sim.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace tee
{
    struct DynoPlan
    {
        bool chassis = false;          // MODE_DYNO_CHASSIS (3) rather than MODE_DYNO_ENGINE (2)
        double from = 150.0;           // rad/s
        double to = 650.0;             // rad/s
        double rate = 60.0;            // a sweep's rate, rad/s^2 (when there are no steps)
        double spacing = 10.0;         // a sweep's points, every this many rad/s (each the mean over its span)
        std::vector<double> steps;     // speeds held in turn (rad/s): steps instead of a sweep
        double settle = 0.6;           // seconds each step (and the sweep's start) is held before it is measured
        double measure = 0.25;         // seconds a step is averaged over
        double pedal = 1.0;            // the throttle, 0 to 1
        // Seconds at `from`, started and running, before anything is measured; below 0 (the default): 2 for a petrol
        // engine without a turbo, 8 for a turbo or a diesel. A turbo diesel swept after 2 s has not spooled and its
        // charge does not yet light every time: it misfires through the sweep (the large diesel at 60 rad/s^2 read
        // negative torque from 1400 rpm; after 8 s, none). Above a diesel's governed speed the governor takes the
        // fuel and the brake reads its friction: a negative torque that is right.
        double warmup = -1.0;
        bool warm = true;              // the engine held at its working temperatures (Warm.h), as a dyno cell holds it
        // The exhaust's heat settled (spec.exhaust.heatSettle) at each step, twice, and at a sweep's start: a held step
        // reads the steady state, as a predicted point does (Predict.h), the walls as a minute at that speed leaves
        // them. Off: a step's first seconds, its walls still warming (some 3 % low on the V12 near 6700 rpm: 769
        // against 779 after a minute). A run says which it is (DynoCurve::settled).
        bool settleExhaust = true;
        std::string torque = "engine.torque";   // the tel field the absorbed torque is read from
        std::vector<std::string> fields;        // more tel fields at each point: "boost.pressure", "engine.manifold"
        double sampleRate = 48000.0;
        int block = 512;

        // The warm-up this plan gives `build` (warmup, or its default above).
        double warmupFor(const Build& build) const;
        // A key for the plan, which with the build's hash keys the cache.
        std::uint64_t hash() const;
    };

    struct DynoPoint
    {
        double speed = 0.0;            // rad/s
        double torque = 0.0;           // N m
        double power = 0.0;            // W
        std::vector<double> fields;    // DynoPlan::fields', in order
    };

    struct DynoCurve
    {
        std::vector<std::string> fields;
        std::vector<DynoPoint> points;
        double simulatedSeconds = 0.0;
        double wallSeconds = 0.0;
        bool cancelled = false;
        // The simulator found its engine diverged (FLAG_FAULT in tel.engine.flags) and put it at rest: the run
        // stopped there, the point it was measuring dropped. A failure of the program's invariant, never normal.
        int faults = 0;
        bool settled = false;          // the exhaust's heat was settled at each step (DynoPlan::settleExhaust)
        std::uint64_t key = 0;         // the build's hash with the plan's

        double realTime() const noexcept { return wallSeconds > 0.0 ? simulatedSeconds / wallSeconds : 0.0; }
        // The curve as CSV: speed (rad/s), rpm, torque (N m), power (kW), then the fields.
        std::string csv() const;
        // A small text chart of torque and power against rpm, `width` columns by `height` rows.
        std::string chart(int width = 64, int height = 16) const;
    };

    // Runs `plan` on a fresh instance of `sim` with `build`'s values. False, with `error`, when the program cannot be
    // played or lacks the torque field. `progress` (0 to 1) is called now and then; `cancel` stops it early (the curve
    // then has what was measured, and `cancelled`).
    bool runDyno(const std::shared_ptr<Sim>& sim, const Build& build, const DynoPlan& plan, DynoCurve& curve,
                 std::string& error, const std::function<void(double)>& progress = {},
                 const std::atomic<bool>* cancel = nullptr);

    class DynoService
    {
    public:
        explicit DynoService(std::shared_ptr<Sim> sim);
        ~DynoService();
        DynoService(const DynoService&) = delete;
        DynoService& operator=(const DynoService&) = delete;

        // A curve for this build and plan: from the cache at once (true), else queued (false) in place of any request
        // still waiting, and the one running cancelled.
        bool request(const Build& build, const DynoPlan& plan, DynoCurve& cached);
        // The newest curve finished since the last call (true), and its key.
        bool take(DynoCurve& out);
        double progress() const noexcept { return progress_.load(); }
        bool busy() const noexcept { return busy_.load(); }
        std::string error() const;

    private:
        void loop();

        std::shared_ptr<Sim> sim_;
        std::thread thread_;
        mutable std::mutex mutex_;
        std::condition_variable wake_;
        bool stop_ = false;
        bool waiting_ = false;
        std::string buildJson_;
        DynoPlan plan_;
        std::uint64_t key_ = 0;
        std::atomic<bool> cancel_{false};
        std::atomic<double> progress_{0.0};
        std::atomic<bool> busy_{false};
        std::map<std::uint64_t, DynoCurve> cache_;
        bool ready_ = false;
        DynoCurve done_;
        std::string error_;
    };
} // namespace tee
