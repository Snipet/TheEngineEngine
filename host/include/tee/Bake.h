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
// measured yet, so the map is parked at light load and idle and takes the full-load row from 0.95 of the air's
// pressure up (applyIntakeCam). Before a map is written to a file, checkIdle runs P's idle gate on it. The exhaust
// phaser is left as the build has it.
#pragma once

#include "tee/Predict.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace tee
{
    struct CamBakeOptions
    {
        double step = 10.0;           // degrees of advance between cells, from 0 to spec.valve.inPhaseRange
        double knockLimit = 2.0;      // degrees of knock retard over the parked cell's that set a cell aside
        // Smoothing: a path over the speeds that gives up stepCost of the best torque for each 10 degrees between
        // neighbouring rows (0: each row its own best). The turbo four's grid alone read 20, 20, 0, 30, 0, 20, 30, 20.
        double stepCost = 0.01;
        // Between each two speeds the map interpolates its advance; where at an interval's quarter points the
        // interpolated advance makes more than midLoss less torque than the best cell there, that point is added as a
        // row of its own (the worst first, to the map's 16). A V12's exhaust wave returning into a big low-speed
        // overlap at 2049 rpm made a 30 % hole the grid's 1500 and 2500 rpm rows straddled (N3's finding). Off by
        // default: refined, the V12's hole at 2000 rpm filled +16 % but 1500-1750 fell 4 % (card N1).
        bool refine = false;
        double midLoss = 0.03;
        // The half steps either side of each speed's best cell measured too (the optimum can be narrower than the grid:
        // N3's turbo four at 1800 rpm, 28 degrees 302 N m, 35 degrees 319, 50 degrees 220), at the speeds beside them
        bool halfSteps = true;
        // A two-stage cam (spec.valve.stages): a row either side of the change, so the map steps there
        bool stageRows = true;
        // Acceptance: the map against the auto schedule at full load, a row falling back to the auto's phase where it
        // makes more than acceptLoss less.
        bool accept = true;
        double acceptLoss = 0.02;
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
        int added = 0;                               // midpoints added as rows of their own (refine)
        int fellBack = 0;                            // rows given the auto schedule's phase by the acceptance
        int halfSteps = 0;                           // points measured at the half steps beside the best cells
        int stageRows = 0;                           // rows added either side of a two-stage cam's change
        std::int64_t key = 0;                        // intakeCamKey(build, sim) of the build it measured
    };

    // Sweeps `build`'s intake cam (above). False, with `error`, when the program or the build has no intake phaser to
    // hold (spec.valve.inCentre, inPhaseRange; a build whose phaser range is 0 has nothing to sweep) or a curve fails.
    bool bakeIntakeCam(const std::shared_ptr<Sim>& sim, const Build& build, const CamBakeOptions& options,
                       CamBakeResult& result, std::string& error, const std::atomic<bool>* cancel = nullptr);

    // Writes the result into `build`: ecu.maps.camIn over the swept speeds, parked (0) at loads 0 and 0.75 of the
    // air's pressure and the baked advance at 0.95 and 3, ecu.maps.camInAuto off and ecu.maps.camInKey the result's
    // key. One undo step.
    bool applyIntakeCam(Build& build, const CamBakeResult& result, std::string& error);

    // The intake cam map's key: a hash of the program and the fields that move the cam's best (the valvetrain whole,
    // its stages included, the intake, the exhaust, the boost, the engine, the fuel; never a hidden field nor an ECU
    // map) with the spark it was chosen on (knockLimitBaseKey: the knock key without the cam maps, so that the
    // calibration's order, spark then cam then spark again, ends). A 31-bit number, as spec.ecu.maps.camInKey holds it.
    std::int64_t intakeCamKey(const Build& build, const Sim& sim);
    // Whether the build's cam map is baked and current: camInAuto off and camInKey this build's key
    bool intakeCamCurrent(const Build& build, const Sim& sim);

    // The idle gate (P's, content/builds/archetypes/generate.py idle_check, the same windows and limits, so that the
    // two never disagree: the lead, 22:55). On the scenarios p-idle-warm (8 to 14 s: the mean within 100 rpm of
    // ecu.idle.speed, the deviation under 25 rpm) and p-cold-start (8 to 11.5 s: running, engine.state 2 throughout,
    // the deviation under 60 rpm), found in `scenarios`.
    struct IdleCheck
    {
        double asked = 0.0;                     // rpm
        double warmMean = 0.0, warmSd = 0.0;    // rpm
        double coldMean = 0.0, coldSd = 0.0;    // rpm
        bool coldRunning = false;
        bool pass = false;
        std::vector<std::string> failures;      // a line each
    };
    bool checkIdle(const std::shared_ptr<Sim>& sim, const Build& build, const std::filesystem::path& scenarios,
                   IdleCheck& result, std::string& error);

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
        // A turbo's loads above the air's (card N2's): full pedal, ecu.maps.boostAuto on, spec.boost.target stepped from
        // the build's own level down to its gate's spring (below the spring the gate does not hold a target) in this
        // many steps; then the pedals under full, at the spring's target (the loads below). 0: pedals alone. A supercharger (no loop: its boost is its pulley's)
        // has pedals alone, said in the notes.
        int boostSteps = 4;
        int speeds = 8;               // over PredictOptions' from and to (0: 1000 rpm, the redline less 100)
        int loads = 8;                // the table's loads: from 0.2 atm to the most a pedal gave, evenly
        double margin = 2.0;          // degrees over the mean retard
        // Where a cell's mean retard was over correctAbove degrees (the retard at or near knock control's ceiling, its
        // mean then understating the need: N2's proof, the rotary at 5200, the six at 2450, the hot vee at 2380), the
        // table is applied and the cell measured again, its residual retard added to it; at most `corrections` times.
        // A cell that still knocks after them is said (stillKnocking): TUNE shows it knock-limited there, not finished.
        double correctAbove = 12.0;
        int corrections = 2;
        double knockLimit = 0.5;      // degrees: a mean retard under it is no knock
        PredictOptions predict;       // the points' (fields: engine.manifold is added)
    };

    struct KnockBakeCell
    {
        double pedal = 0.0;
        double boostTarget = 0.0;     // Pa over the air: the step's spec.boost.target (0: a pedal step, as built)
        double load = 0.0;            // atm, the manifold's pressure the point gave
        double retardMean = 0.0;      // degrees
        double retardTooth = 0.0;     // the most less the mean: the sawtooth's size
        double torque = 0.0;          // N m
        bool knockLimited = false;
        bool valid = true;
        double limit = 0.0;           // the cell: retardMean + margin, or 0; with the corrections' residuals added
        int corrections = 0;          // times it was measured again with the table applied
        double residual = 0.0;        // the mean retard at its last measure with the table applied
        bool stillKnocking = false;   // its residual still at knockLimit or more after the corrections
        int step = 0;                 // (the bake's step it was measured at)
    };

    struct KnockBakeResult
    {
        std::vector<double> speeds;                       // rad/s: the table's x
        std::vector<double> loads;                        // atm: the table's y
        std::vector<std::vector<KnockBakeCell>> measured; // [speed]: the pedals run there, full first
        std::vector<std::vector<double>> limit;           // [speed][load]: degrees from best torque
        std::vector<std::vector<bool>> extrapolated;      // [speed][load]: flat past the measured loads
        std::vector<std::vector<bool>> stillKnocking;     // [speed][load]: made from a cell still knocking (above)
        std::int64_t key = 0;                             // knockLimitKey(build, sim) when it was baked
        std::string notes;                                // how its loads were made
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
    // The same without the cam maps (ecu.maps.camIn, camInAuto, camEx, camExAuto): what the intake cam's bake key
    // carries of the spark (its intakeCamKey), so that the spark baked again after the cam (its key moved by the cam)
    // does not make the cam stale in turn: spark, cam, spark, and it ends. Hidden fields (the heatSettle counters)
    // are in neither.
    std::int64_t knockLimitBaseKey(const Build& build, const Sim& sim);
    // Whether the build's bake is in use and current: knockLimitAuto off and its key this build's (false when the
    // program has no knock-limit table).
    bool knockLimitCurrent(const Build& build, const Sim& sim);

    // Bakes `build`'s knock limit (above). False, with `error`, when a sweep fails or is cancelled.
    bool bakeKnockLimit(const std::shared_ptr<Sim>& sim, const Build& build, const KnockBakeOptions& options,
                        KnockBakeResult& result, std::string& error, const std::atomic<bool>* cancel = nullptr);
    // Writes it: ecu.maps.knockLimit, knockLimitAuto off, knockLimitKey its key. One undo step. False when the program
    // has no knock-limit table.
    bool applyKnockLimit(Build& build, const KnockBakeResult& result, std::string& error);

    // The calibration baked in the background, for the application (the lead's rulings: nobody waits on a bake, and
    // its state is shown; the spark table first, the intake cam on that spark, the spark again on that cam). After
    // each edit the UI thread calls consider(build): a build whose tables are current is left as it is; one whose key
    // moved (an edit of what moves them) has each stale table set aside at once (knockLimitAuto, camInAuto on: the
    // engine on its knock control and its auto cam, as before any bake) and a bake asked, debounced (a knob's drag
    // asks once, when it rests), a newer one cancelling the one running. The bake, on a copy of the build: the spark
    // table if stale, applied to the copy; the intake cam if stale (on that spark), applied; the spark again if the cam
    // made it stale (its key carries the cam maps; the cam's carries knockLimitBaseKey, so it ends). Controls never
    // move a key, so they never bake. takeAndApply(build) applies what is done, the cam first and then the spark, each
    // only while the build's key is still its key. TUNE shows stage() and state(): "spark map: baking", "cam: baking",
    // "baked".
    //
    //   tee::CalibrationBakeService bakes(sim);            // once
    //   bakes.consider(build);                             // after each edit, and when a build is loaded
    //   bakes.takeAndApply(build, error);                  // each frame
    enum class KnockBakeState
    {
        NoTable,      // the program has no knock-limit table
        Current,      // the build's tables are baked and their keys are the build's
        Waiting,      // stale: a bake is asked, waiting for the edits to rest
        Baking,       // stale: being baked (stage() says which)
        Ready,        // baked: take() or takeAndApply() has it
        Failed,       // the bake failed (error())
    };

    const char* knockBakeStateName(KnockBakeState state) noexcept;   // "no table", "baked", "baking" ...

    struct CalibrationBakeOptions
    {
        KnockBakeOptions spark;
        CamBakeOptions cam;
        bool bakeCam = true;           // the intake cam too (where the program and the build have a phaser to bake)
        double debounceSeconds = 1.5;
    };

    struct CalibrationBakeResult
    {
        bool hasSpark = false;
        KnockBakeResult spark;         // the last spark baked (after the cam, when the cam was baked)
        bool hasCam = false;
        CamBakeResult cam;
    };

    class CalibrationBakeService
    {
    public:
        explicit CalibrationBakeService(std::shared_ptr<Sim> sim, CalibrationBakeOptions options = {});
        virtual ~CalibrationBakeService();
        CalibrationBakeService(const CalibrationBakeService&) = delete;
        CalibrationBakeService& operator=(const CalibrationBakeService&) = delete;

        // ---- the UI thread ---------------------------------------------------------------------------------------
        KnockBakeState consider(Build& build);
        // What is done, applied to `build` (the cam, then the spark, each while its key is the build's). True when
        // something was applied; false when nothing was ready, or (with `error`) a table did not fit.
        bool takeAndApply(Build& build, std::string& error);
        bool take(CalibrationBakeResult& out);
        // The spark alone (KnockBakeService's): false when nothing is ready or the bake held a cam (use takeAndApply).
        bool take(KnockBakeResult& out);
        KnockBakeState state() const;
        std::string stage() const;     // "spark map", "cam", "spark map again"; empty when not baking
        // The stage running: fast-dyno points done of about `total` (an estimate made at the stage's start, from its
        // grid; corrections and refinements add to it as they come). {0, 0} when not baking.
        struct Progress
        {
            int done = 0, total = 0;
        };
        Progress progress() const;
        // While `predict` is busy (the ribbon's curve wanted), the bake waits between its points, so the curve a person
        // waits for has the workers (nullptr: never). The service must outlive this one, or be set to nullptr first.
        void yieldTo(const PredictService* predict) noexcept { yieldTo_.store(predict); }
        // The spark table last applied by takeAndApply, kept for TUNE: its cells still knocking after the corrections
        // (KnockBakeResult::stillKnocking, measured: KnockBakeCell::stillKnocking).
        bool lastSpark(KnockBakeResult& out) const;
        int stillKnockingCells() const;   // measured cells still knocking in it (0 with none applied)
        std::string error() const;
        // Moves when the state or the stage changes.
        std::uint64_t revision() const noexcept { return revision_.load(); }

    private:
        void coordinator();
        void setState(KnockBakeState s);   // under mutex_
        bool camWanted(const Build& build) const;

        std::shared_ptr<Sim> sim_;
        CalibrationBakeOptions options_;
        std::thread thread_;
        mutable std::mutex mutex_;
        std::condition_variable wake_;
        bool stop_ = false;
        bool asked_ = false;               // a bake of json_ waits to start
        std::string json_;                 // the build asked, saved
        std::int64_t sparkKey_ = -1;       // its keys
        std::int64_t camKey_ = -1;
        std::chrono::steady_clock::time_point due_{};
        std::atomic<bool> cancel_{false};
        std::atomic<std::uint64_t> revision_{0};
        KnockBakeState state_ = KnockBakeState::NoTable;
        std::string stage_;
        std::atomic<int> done_points_{0}, total_points_{0};
        std::atomic<const PredictService*> yieldTo_{nullptr};
        bool haveLast_ = false;
        KnockBakeResult last_;
        bool ready_ = false;
        CalibrationBakeResult done_;
        std::string error_;
    };

    // The spark table alone (U1's ServiceBake, take(KnockBakeResult&)): a CalibrationBakeService without the cam.
    class KnockBakeService : public CalibrationBakeService
    {
    public:
        explicit KnockBakeService(std::shared_ptr<Sim> sim, KnockBakeOptions options = {}, double debounceSeconds = 1.5);
    };
} // namespace tee
