// tests/sim/combo/Combo.h: package X's harness: the whole program under test. Every package tests itself alone, with
// the others at their defaults; this drives them together.
//
// - The model switches are found in the program's own Schema (every `spec.<section>.model` with a range above 0, and
//   `spec.engine.custom`), so a package's switch is covered the moment it lands, with no edit here.
// - Builds are the archetypes of content/builds/combo/ (every switch at 0 there; a test switches them on).
// - Scenarios are made here in C++ (tour, pull, launch, cold), and the same ones are written as files under
//   content/scenarios/combo/ for tee-render (tee-fuzz --write-scenarios).
// - Each run is judged by `judge` (the measures every combination must pass) into a list of failures, each a short
//   name and its numbers; a failure that is in the known list (Known.h) is reported and does not fail the test.
//
// Nothing here owns a tee::Program in a static (01-rules.md section 4): the Sims are tee::test's never-destroyed cache.
#pragma once

#include "tee/test/Scenario.h"

#include <cstdint>
#include <filesystem>
#include <functional>
#include <random>
#include <span>
#include <string>
#include <vector>

namespace combo
{
    // A model switch: a Spec path (without "spec.") and the value that turns the package on.
    struct Switch
    {
        std::string path;
        int on = 1;
    };

    // The switches the program has now, in the Schema's order.
    const std::vector<Switch>& switches();

    // A set of switches on, as a bit mask over switches(); its name ("none", "car.model+boost.model", "all").
    std::string comboName(std::uint32_t mask);
    std::vector<std::uint32_t> subsetsOfSize(int k);

    // The archetypes (content/builds/combo/<name>.json).
    struct Archetype
    {
        const char* name;       // file stem
        double idleLow, idleHigh;   // a plausible idle, rad/s
        bool diesel;
    };
    const std::vector<Archetype>& archetypes();
    const Archetype* archetype(const std::string& name);
    std::string buildPath(const std::string& name);   // content/builds/combo/<name>.json

    // The scenarios, each with the times the judge reads.
    struct Times
    {
        double start = 0.25;     // the key to start
        double started = 2.0;    // the key back to on; it must be running by then
        double idleFrom = 3.0, idleTo = 5.0;   // a stretch of idle (-1: none)
        double revFrom = -1, revTo = -1;       // full throttle in neutral: it must near its redline
        double driveFrom = -1, driveTo = -1;   // in gear: the car must move
        double off = -1;                       // the key off: it must stop within 3 s
        double end = 0;
    };
    enum class Kind
    {
        Tour,     // start, idle, free revs to the limiter, a lift, idle, off
        Pull,     // start, pull away on the paddles, up through three gears at full throttle, a lift, brake, off
        Launch,   // start, first, the pedal held at 4000 rpm on the brake, released: a launch
        Cold,     // a start at -20 C, idle, off
    };
    const char* kindName(Kind k);
    std::vector<Kind> allKinds();
    // The scenario on a build with a set of switches on from the start. `times` gets its times.
    // shiftPhase: seconds added to every upshift after the pull-away (a shift cut's phase among the firings)
    tee::test::Scenario scenario(Kind kind, const std::string& build, std::uint32_t mask, Times& times,
                                 double shiftPhase = 0.0);
    // Its timeline as a tee-scenario file (for the tools and the listening pack).
    std::string scenarioJson(Kind kind, std::uint32_t mask);
    // Where those files are: content/scenarios/combo/ (tee-render takes them by path, not by name).
    std::filesystem::path scenarioDirectory();

    // What a run is judged against. Every bound is stated where it is set (Combo.cpp, judge).
    struct Failure
    {
        std::string check;   // "finite", "peak", "dc", "click", "start", "idle", "revs", "moves", "stop", "tel", "guard"
        std::string detail;
    };
    struct Verdict
    {
        std::vector<Failure> failures;
        double stepMax = 0.0;       // the largest step over the local level while running
        double dcMax = 0.0;         // the running stretch's mean, as a magnitude
        double idle = 0.0;          // rad/s, mean over the idle stretch
        double revMax = 0.0;        // rad/s
        // The step's excess over its neighbouring firing periods: the worst step's own measure in one firing period
        // about it, over the median of the same measure in the periods t +- kT, k = 1 to 4. A pulse train at the
        // firing rate reads about 1; a click its neighbours lack reads well above. -1 where the engine does not fire
        // (no period). It decides a click where defined and the caller gives the reference's (judge; the lead's
        // ruling, 25 Nov 17:24). Its proof that it bites: X-015 on 62a6a7fd's tree reads 7.33 against 3.19 on the
        // pull and 4.07 against 3.64 on the tour (its weakest proof, a margin of 12 %); X-014 7.77 against 4.60.
        double stepExcess = -1.0;
        double stepAt = 0.0;        // seconds: where the worst step is
        double firingPeriod = 0.0;  // seconds, at stepAt
        double excessBound = -1.0;  // twice the reference's own excess (-1: none given)
        double costR = 0.0;         // wall time a simulated second over the reference's (costOfR)
        double coreShare = 0.0;     // wall seconds a simulated second (a core's share at the run's rate)
    };
    // refStep: the reference's largest step on the same build and scenario (referenceStep), for the click bound.
    // refExcess: the reference's own excess over its firing periods (referenceExcess); where it is given (>= 0) and
    // the run fires, the excess decides a click (the lead's ruling, 25 Nov), else the step does.
    Verdict judge(const tee::test::Run& run, const Times& times, const Archetype* archetype, bool expectRunning,
                  double refStep, double refExcess = -1.0);
    // The reference's (every switch off) largest step while running, on a build through a scenario: measured once a
    // process and kept.
    double referenceStep(const std::string& build, Kind kind);
    // The reference's own excess (Verdict::stepExcess) on a build through a scenario, measured once a process. In a
    // [nightly] case: the LOWEST over the reference's copies at kBoundNudges (the lead's ruling, 26 Nov 08:07; the
    // default tier keeps the single draw at 293.15 K as its tripwire). The lowest, not the median or the highest: the
    // references' highs are what spread across copies (the flat six's 1.41 to 3.90 over 17), and a bound drawn high is
    // a loose bound; the 293.15 K copy is among them, so the nightly's bound is never above the default tier's.
    double referenceExcess(const std::string& build, Kind kind);
    // The air temperatures' offsets (K, from 293.15) of the nightly bound's reference copies: fixed, so the bound is one
    // number on a given tree and anyone can recompute it (tests/sim/combo/spread.sh with SPREAD_DT set to these)
    inline constexpr double kBoundNudges[5] = {0.0, -0.07, -0.03, 0.03, 0.07};
    // The lowest of `excess` over a reference's copies (-1 where none had a period): the nightly bound's make-up
    double lowestExcess(const std::vector<double>& excess);
    // Whether the running Catch case is tagged [nightly] (ComboTest.cpp's listener sets it at each case's start; false in
    // a tool, tee-fuzz, that plays without Catch)
    bool nightlyCase();
    void setNightlyCase(bool nightly);
    // The excess of the worst step of [from, to] over its neighbouring firing periods (Verdict::stepExcess)
    double stepExcess(const tee::test::Run& run, double from, double to, double* at = nullptr, double* period = nullptr);
    // The bang rule's baseline (cuts that send fuel to the pipe, the new ECU's): the largest step of three reference
    // copies with ecu.pops at 0.3, 0.01 K apart in the air (one run of a chaotic engine is no bound: 11.9 to 20.5 measured).
    double referenceStepWithPops(const std::string& build, Kind kind);
    // Where tee::measure::largestStep finds its step: seconds from the span's start.
    double largestStepTime(std::span<const float> x, double rate);

    // Telemetry: every column finite; where the field declares a range, within it; by its quantity, within physical
    // bounds (combo.cpp: telBounds). The failures' names list the fields.
    void judgeTelemetry(const tee::test::Run& run, Verdict& v);

    // The reference's cost on this machine: the default build through the same host, wall seconds a simulated
    // second, measured once a process (the median of three short runs).
    double costOfR();
    // R if this process has measured it already, else 0 (no runs)
    double costOfRIfMeasured();

    // The known failures: reported, not failed (Known.h holds the list).
    struct Known
    {
        const char* id;        // "X-001"
        const char* date;      // found
        const char* owner;     // the package
        const char* combo;     // a comboName, "*", or "+a+b": every combination with a and b on
        const char* build;     // an archetype, or "*"
        const char* scenario;  // a kindName or a test's name, or "*"
        const char* check;     // a Failure's check, or "*"
        const char* what;      // one sentence
    };
    const Known* known(const std::string& comboName, const std::string& build, const std::string& scenario,
                       const std::string& check);

    // The lead's ruling (23:15): judge's "click-shift-cut" (a step within 42 at an upshift's cut) is the engine's own
    // event for a combination with ecu.model on and fuel.model off; report() accepts it there and nowhere else.
    bool holds(const std::string& comboName, const std::string& sw);   // "all", or one of its '+'-joined names
    bool shiftCutRuled(const std::string& comboName, const Failure& f);

    // Reports a verdict through Catch: each failure that is not known fails the test (FAIL_CHECK), a known one is a
    // WARN naming its finding. Returns how many failures were known.
    int report(const Verdict& v, const std::string& comboName, const std::string& build, const std::string& scenario);

    // ---- fuzzing (FuzzTest.cpp, tools/tee-fuzz) ----

    // A Spec value set by path (under spec), as a scenario's event writes it.
    struct Value
    {
        std::string path;
        double value = 0.0;
    };
    // Every Spec field a fuzzer may set: numbers, switches and choices with a declared range, not hidden, not
    // tables; arrays element by element (path "name[i]").
    struct Field
    {
        std::string path;
        double lo = 0.0, hi = 0.0, def = 0.0;
        bool integer = false;
        bool log = false;
        int choices = 0;       // a choice's options (0: a number); a switch counts as 2
    };
    const std::vector<Field>& fuzzFields();
    // Random values for every field (corners: each at its minimum or its maximum), from a seed.
    std::vector<Value> randomSpec(std::uint64_t seed, bool corners);
    // A run of random driving on a Spec: start, then `seconds` of random pedals, levers and shifts from the seed.
    tee::test::Scenario fuzzScenario(const std::vector<Value>& spec, std::uint64_t seed, double seconds, Times& times);
    // A fuzz run's own judge: finite, no guard, no blast, not silent while it says it runs, not far over budget.
    Verdict judgeFuzz(const tee::test::Run& run, const Times& times);
    // A build file holding these values (for a finding's reproduction).
    std::string buildJson(const std::vector<Value>& spec, const std::string& name, const std::string& notes);
} // namespace combo
