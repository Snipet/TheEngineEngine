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
        // Smoothing: a path over the speeds that gives up stepCost of the best torque for each 10 degrees between
        // neighbouring rows (0: each row its own best). The turbo four's grid alone read 20, 20, 0, 30, 0, 20, 30, 20.
        double stepCost = 0.01;
        // Ties: cells within `floor` of a speed's best torque count as its best, and the path takes the one nearest its
        // neighbours' (a single predicted point's floor, about 0.5 %: card N1, measured; the lead's C, 25 Nov)
        double floor = 0.005;         // (where a cell has no standard error: a knock-limited point's window mean)
        // A cell's torque: the mean of its point's last `cycles` counted cycles, its standard error the row's floor
        // (twice it); the predictor's two-cycle mean carried some +-0.5 % (card N1, 25 Nov, measured)
        int cycles = 8;
        // No row gives up more than capLoss of its own best for the smoothing (the lead's ruling, 25 Nov)
        double capLoss = 0.01;
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

        // the top speed redline less 300 rpm: at the default 100 the top point sat on the rev limiter on three builds
        // (card C: economy four -36 N m, v6-35-na -81, flat-plane -155; at 300 all clean), its torque no measure
        CamBakeOptions()
        {
            predict.points = 8;
            predict.coarse = 8;
            predict.redlineMarginRpm = 300.0;
        }
    };

    struct CamBakeResult
    {
        std::vector<double> speeds;                  // rad/s
        std::vector<double> advances;                // degrees from parked, the cells swept
        std::vector<std::vector<double>> torque;     // [advance][speed], N m (NaN: no measure)
        std::vector<std::vector<double>> torqueSe;   // [advance][speed], its standard error (NaN: none)
        std::vector<std::vector<double>> knock;      // [advance][speed], degrees of spark given up: knock control's
                                                     // retard plus a baked knock-limit table's cell, when applied
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
    // The cam bake's own rules' version, in its key: a map baked by older rules reads stale (2: no knock gate, the
    // choice on knock-limited torque; ties inside a point's floor to the neighbours' choice. 3: a cell's torque the
    // mean of its last cycles, the floor its own standard error; the smoothing capped at 1 % of a row's best)
    constexpr int kCamBakeVersion = 3;
    std::int64_t intakeCamKey(const Build& build, const Sim& sim);
    // The same fields without the program (CalibrationBakeService's verified reuse: a map baked for these fields under
    // another program, a candidate to be checked, never current by itself)
    std::int64_t intakeCamFieldsKey(const Build& build);
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
        // full first. The lower ones run only where the one above still knocks: at a low speed a quarter pedal is near
        // full load (the V12 at 1000 rpm: 0.96 atm at 0.25, 0.70 at 0.08)
        std::vector<double> pedals = {1.0, 0.75, 0.5, 0.25, 0.15, 0.08};
        // A turbo's loads above the air's (card N2's): full pedal, ecu.maps.boostAuto on, spec.boost.target stepped from
        // the build's own level down to its gate's spring (below the spring the gate does not hold a target) in this
        // many steps; then the pedals under full, at the spring's target (the loads below). 0: pedals alone. A supercharger (no loop: its boost is its pulley's)
        // has pedals alone, said in the notes.
        int boostSteps = 4;
        int speeds = 8;               // over PredictOptions' from and to (0: 1000 rpm, the redline less 300)
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

    // How the knock-limit bake chooses its table, mixed into knockLimitKey and knockLimitBaseKey (so into intakeCamKey):
    // whoever changes how a table is chosen bumps it, with a line here saying what changed.
    //   1  (no number in the key) the first bake and its corrections
    //   2  a point on which the rev limiter acted (PredictPoint::limited) is missing, not a cell
    //   3  the top row 300 rpm under the redline, not 100 (PredictOptions::redlineMarginRpm's default)
    //   4  a settled point's torque the mean of its settled cycles, not its last two (the table itself does not move:
    //      the cache entries' check sets, read the old way, are kept out of the verified reuse)
    //   5  pedals 0.15 and 0.08 below 0.25, run where 0.25 still knocks: a low speed's row measured down to a load that
    //      does not knock, not copied flat from the lowest knocking cell (the V12's 1000 rpm row read 23.2 deg down to
    //      0.2 atm, its idle at -2 deg of spark: N1's find); and spec.fuel.octane in the knock and cam keys (when not 0)
    inline constexpr int kKnockBakeVersion = 5;

    // The fields that move the knock limit (card N2's list): the engine's compression, displacement, bore and stroke,
    // cylinders and layout; the fuel's kind, grade and model, and its octane number when not 0; the boost section;
    // the boost, lambda, cam and spark maps with their Auto switches; the cams' centres and phase ranges; the exhaust's and the intake's models; and the
    // program itself (Sim::sourceHash) and kKnockBakeVersion. Not the air's temperature (baked at the cell's 300 K)
    // nor ecu.timing (the person's offset, on top). A 31-bit number, as spec.ecu.maps.knockLimitKey holds it.
    std::int64_t knockLimitKey(const Build& build, const Sim& sim);
    // The same without the cam maps (ecu.maps.camIn, camInAuto, camEx, camExAuto): what the intake cam's bake key
    // carries of the spark (its intakeCamKey), so that the spark baked again after the cam (its key moved by the cam)
    // does not make the cam stale in turn: spark, cam, spark, and it ends. Hidden fields (the heatSettle counters)
    // are in neither.
    std::int64_t knockLimitBaseKey(const Build& build, const Sim& sim);
    // Both without the program (Sim::sourceHash): the verified reuse's keys (CalibrationBakeService), never "current"
    std::int64_t knockLimitFieldsKey(const Build& build);
    std::int64_t knockLimitBaseFieldsKey(const Build& build);
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
        Checking,     // stale: a table baked for these fields under another program, being checked on this one
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
        // A cache across launches (the lead's): each table baked is kept under its key (knockLimitKey, intakeCamKey:
        // the program's sources and the fields that move it) as spark-<key>.json or cam-<key>.json here, the field
        // values its apply wrote (some 6 KB a spark table, 3 KB a cam); a stage whose key is there is taken from it at
        // once, not measured. Empty: no cache. The folder keeps the newest cacheEntries (the oldest by their last use
        // removed past it): some 2 MB at 256.
        std::filesystem::path cacheDirectory;
        std::size_t cacheEntries = 256;
        // ---- the verified reuse (the lead's 06:28 ruling). The exact key (the program's sources with the fields) stays
        // the one meaning of "current". When it misses, the newest table in the cache baked for the same fields under
        // any program (knockLimitFieldsKey, intakeCamFieldsKey) is a candidate: applied to a copy and checked on this
        // program at a check set, and taken (re-keyed to this program, kept in the cache under its new key) only if
        // every point agrees within the floor; otherwise the stage is baked as before. The check set:
        //   - the spark: the full-load row at the table's speeds with the table applied, each point's torque within
        //     checkTorque of the one stored when it was baked (or last checked) and knock control's mean retard within
        //     checkRetard degrees; a point the limiter cut or that faulted then must be so now, and the reverse;
        //   - the cam: the same row with the map applied (torque only), and one warm idle (idleScenarios' p-idle-warm,
        //     8 to 14 s) inside the idle gate's bounds (checkIdle: the mean within 100 rpm of ecu.idle.speed, the
        //     deviation under 25 rpm), as the cam bake's gate protects it.
        // Some 8 to 16 points and an idle: seconds, where a bake is minutes. state() is Checking meanwhile, and a
        // result taken so says it (BakedFields::checked; lastWasChecked()).
        //
        // THE HOLE: the check measures full load only. A change under sim/ that moved the spark table's part-load cells
        // while leaving its full-load row within the floor would pass it. Thought small today: those cells are 0 on
        // every naturally aspirated archetype, and on a turbo they come from the same knock model and the same points
        // as the full-load row, one boost step down. Whoever first bakes part load for its own sake (a cell measured at
        // a pedal under 1 and not 0) grows the check set with it: a part-load row in it, or the reuse turned off for
        // such a table.
        bool verifiedReuse = true;           // with a cacheDirectory
        std::filesystem::path idleScenarios; // content/validation/scenarios: empty, no cam is reused (the idle unchecked)
        double checkTorque = 0.005;          // N1's floor: a share of the point's torque
        double checkRetard = 0.5;            // degrees
    };

    // The check set a cached table carries: the full-load row with it applied, as last measured
    struct BakeCheck
    {
        std::vector<double> speeds;   // rad/s
        std::vector<double> torque;   // N m
        std::vector<double> retard;   // degrees, knock control's mean (PredictPoint::retardMean)
        std::vector<bool> valid;      // neither faulted nor cut by the limiter
    };

    // A table as its apply wrote it: (field path, the field's values) for each field it wrote, with its key
    struct BakedFields
    {
        std::int64_t key = -1;
        std::vector<std::pair<std::string, std::vector<double>>> fields;
        std::vector<std::vector<bool>> stillKnocking;   // a spark table's: by table cell (KnockBakeResult's)
        bool cached = false;                            // taken from the cache, not measured
        bool checked = false;                           // ... baked under another program, checked on this one
        std::int64_t fieldsKey = -1;                    // knockLimitFieldsKey / intakeCamFieldsKey (-1: none)
        BakeCheck check;                                // its check set (empty: never reused)
    };

    struct CalibrationBakeResult
    {
        bool hasSpark = false;
        BakedFields spark;             // the last spark (after the cam, when the cam was baked)
        bool hasCam = false;
        BakedFields cam;
        KnockBakeResult sparkMeasured; // the measures, when the spark was baked here (not taken from the cache)
        CamBakeResult camMeasured;
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
        std::string stage() const;     // "spark map", "cam", "spark map again"; empty when not baking or checking
        // The stage running: fast-dyno points done of about `total` (an estimate made at the stage's start, from its
        // grid; corrections and refinements add to it as they come). {0, 0} when not baking.
        struct Progress
        {
            int done = 0, total = 0;
        };
        Progress progress() const;
        // While `predict` is busy (the ribbon's curve wanted), the bake waits between its points, so the curve a person
        // waits for has the workers. What is kept is the service's busy flag, shared (PredictService::busyFlag): it
        // stays valid when the PredictService goes (a reload that makes it again) and reads false then, so no order of
        // destruction is wrong. Call it again for a new PredictService; stopYielding() to yield to none.
        void yieldTo(const PredictService& predict);
        void stopYielding();
        // The spark table last applied by takeAndApply, kept for TUNE: its table cells still knocking after the
        // corrections (by [speed][load], as KnockBakeResult::stillKnocking), whether baked here or taken from the cache.
        bool lastSpark(BakedFields& out) const;
        int stillKnockingCells() const;   // table cells still knocking in it (0 with none applied)
        // The last takeAndApply applied only tables checked (or taken from the cache by their key), at least one
        // checked: what the status line says as "calibration checked", not "baked"
        bool lastWasChecked() const;
        std::string error() const;
        // Moves when the state or the stage changes.
        std::uint64_t revision() const noexcept { return revision_.load(); }

    private:
        void coordinator();
        void setState(KnockBakeState s);   // under mutex_
        bool camWanted(const Build& build) const;
        // A spark-ignition build's (a diesel has no spark and no knock limit: its spark table is never baked, and
        // reads as current, so the status line says nothing of it)
        static bool sparkWanted(const Build& build);

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
        std::shared_ptr<const std::atomic<bool>> yieldTo_;   // under mutex_
        bool haveLast_ = false;
        bool lastChecked_ = false;
        BakedFields last_;
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
