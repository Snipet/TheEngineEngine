// ui/views/valves/ValvesModel.h: what the VALVES stage knows of the valvetrain, read from the build by path.
//
// The fields are package E's lab (sim/valvetrain/lab-spec.fm, appended to spec.valve), each with the demo's name as
// its fallback where the demo has one; the reference's single CAM dial (spec.valve.cam) when the build has nothing
// else. Telemetry names E will add (tel.valve, its handoff's step d) are guesses kept in one place: absent, they read
// as NaN and draw a dash.
//
// The lift curves drawn for editing are made by the same generator the simulator uses (sim/valvetrain/cam.fm: a sine
// to a power p from the fatness, its ends eased by a ramp k solved so the 1 mm duration comes out as asked), so the
// curve under a handle is the curve the engine runs. The flow bench is bench.fm's: the head's effective area
// (area.fm) through an orifice at 28 inches of water. Angles are the cycle's: 0 the firing top, 360 the overlap's top.
#pragma once

#include "views/engine/EngineModel.h"

#include <array>
#include <string>
#include <vector>

namespace tee::ui::valves
{
    using engine::FieldPath;
    using engine::kNaN;
    using engine::kPi;

    enum class VF : int
    {
        model, cam, valves, dIn, dEx, portIn, portEx,
        inKind, inDurSeat, inDur1mm, inLift, inCentre, inFat,
        exKind, exDurSeat, exDur1mm, exLift, exCentre, exFat,
        inPhaseRange, exPhaseRange, phaseRate,
        stages, inDurSeatB, inDur1mmB, inLiftB, inCentreB, exDurSeatB, exDur1mmB, exLiftB, exCentreB, stageSpeed, stageHyst,
        liftControl, liftMin, liftThrottle,
        deacMask, deacLow, deacHigh, deacLoad,
        train, springRate, springPreload, valveMass,
        freeInOpen, freeInClose, freeExOpen, freeExClose, freeFlank, auxOn, auxOpen, auxDur, auxLift,
        count
    };
    inline constexpr FieldPath kValveFields[] = {
        { "valve.model", "" },
        { "valve.cam", "" },                         // the reference's dial (spec/valve.fm)
        { "valve.valves", "" },
        { "valve.dIn", "" },
        { "valve.dEx", "" },
        { "valve.portIn", "" },
        { "valve.portEx", "" },
        { "valve.inKind", "" },
        { "valve.inDurSeat", "valve.intakeDuration" },
        { "valve.inDur1mm", "" },
        { "valve.inLift", "valve.intakeLift" },
        { "valve.inCentre", "valve.intakeCentre" },
        { "valve.inFat", "" },
        { "valve.exKind", "" },
        { "valve.exDurSeat", "valve.exhaustDuration" },
        { "valve.exDur1mm", "" },
        { "valve.exLift", "valve.exhaustLift" },
        { "valve.exCentre", "valve.exhaustCentre" },
        { "valve.exFat", "" },
        { "valve.inPhaseRange", "" },
        { "valve.exPhaseRange", "" },
        { "valve.phaseRate", "" },
        { "valve.stages", "" },
        { "valve.inDurSeatB", "" },
        { "valve.inDur1mmB", "" },
        { "valve.inLiftB", "" },
        { "valve.inCentreB", "" },
        { "valve.exDurSeatB", "" },
        { "valve.exDur1mmB", "" },
        { "valve.exLiftB", "" },
        { "valve.exCentreB", "" },
        { "valve.stageSpeed", "" },
        { "valve.stageHyst", "" },
        { "valve.liftControl", "" },
        { "valve.liftMin", "" },
        { "valve.liftThrottle", "" },
        { "valve.deacMask", "" },
        { "valve.deacLow", "" },
        { "valve.deacHigh", "" },
        { "valve.deacLoad", "" },
        { "valve.train", "" },
        { "valve.springRate", "" },
        { "valve.springPreload", "" },
        { "valve.valveMass", "" },
        { "valve.freeInOpen", "" },                  // a free valve's events: the cycle's degrees
        { "valve.freeInClose", "" },
        { "valve.freeExOpen", "" },
        { "valve.freeExClose", "" },
        { "valve.freeFlank", "" },
        { "valve.auxOn", "" },                       // the second exhaust event (a compression brake's lobe)
        { "valve.auxOpen", "" },
        { "valve.auxDur", "" },
        { "valve.auxLift", "" },
    };
    static_assert(std::size(kValveFields) == static_cast<std::size_t>(VF::count));
    inline const FieldPath& path(VF f) { return kValveFields[static_cast<int>(f)]; }
    double fieldValue(const FieldDoc& doc, VF f);

    // Telemetry (E's tel.valve, sim/spec/telemetry.fm) and the Scope's lifts.
    inline constexpr const char* kTelInPhase = "valve.inPhase";       // crank degrees from parked, the intake phaser's now
    inline constexpr const char* kTelExPhase = "valve.exPhase";
    inline constexpr const char* kTelInTarget = "valve.inPhaseTarget";   // and where it is asked to go
    inline constexpr const char* kTelExTarget = "valve.exPhaseTarget";
    inline constexpr const char* kTelStage = "valve.stage";           // 0 stage A, 1 stage B
    // Why the cams are not where asked (N1's tel.valve.waiting, a bit each), and the coolant the stage waits for
    // (tel.valve.waitTemp, K; 0 when not waiting): the simulator's own gates, read, not mirrored here.
    inline constexpr const char* kTelWaiting = "valve.waiting";
    inline constexpr const char* kTelWaitTemp = "valve.waitTemp";
    inline constexpr const char* kTelCoolant = "engine.coolant";          // K
    inline constexpr const char* kTelOilTemp = "engine.oilTemperature";   // K
    enum WaitBits : int
    {
        kWaitInParked = 1,      // the intake phaser parked: no oil pressure
        kWaitExParked = 2,      // the exhaust phaser parked
        kWaitSlowed = 4,        // the phasers slowed by cold oil
        kWaitCoolant = 8,       // the cam stage waiting for the coolant
        kWaitOilPressure = 16,  // the cam stage waiting for oil pressure
    };
    // The words VALVES and ENGINE both say from them ("" : nothing waits), temperatures in the person's units: a
    // phaser's "PARKED: NO OIL PRESSURE" or "SLOWED: COLD OIL, 15 C"; the stage's "B WAITS: COOLANT 5 C, NEEDS 10 C" or
    // "B WAITS: OIL PRESSURE LOW". `brief`: the phaser's short form for the editor's band ("SLOWED: COLD OIL").
    std::string phaserWaitWords(const Live& live, const Units& units, bool intake, bool brief = false);
    std::string stageWaitWords(const Live& live, const Units& units);
    inline constexpr const char* kTelLift = "valve.lift";             // the continuous lift's share now, 0..1
    inline constexpr const char* kTelFloat = "valve.floatSpeed";      // rad/s: the speed the valves float at
    inline constexpr const char* kTelFloating = "valve.floating";     // 1 while they float
    inline constexpr const char* kTelActive = "valve.active";         // a bit a cylinder: those firing now
    inline constexpr const char* kTelEvents[5] = { "valve.ivo", "valve.ivc", "valve.evo", "valve.evc", "valve.overlap" };
    // the flow bench as the simulator computes it, an element a name ("valve.benchLift[3]"): m, and m^3/s at 28 in
    // of water. (The shell does not yet offer array telemetry by element; until it does the bench is the port below.)
    inline constexpr const char* kTelBenchLift = "valve.benchLift[%d]";
    inline constexpr const char* kTelBenchIn = "valve.benchIntake[%d]";
    inline constexpr const char* kTelBenchEx = "valve.benchExhaust[%d]";

    // One cam lobe as the generator makes it (cam.fm).
    struct Lobe
    {
        double durSeat = kNaN, dur1mm = kNaN, lift = kNaN, centre = kNaN, fat = 0.5;   // centre: the cycle's angle
        bool free = false;           // a free valve (not drawn by the generator's lobe: a flat-topped event)
        double flank = 40.0;         // a free valve's flanks, degrees each
        // made
        double p = 1.25, k = 4.0, kNorm = 1.0, span = kNaN, open = kNaN;
        bool known() const noexcept { return std::isfinite(durSeat) && std::isfinite(centre) && durSeat > 1.0; }
        void build();
        // The lift as a share of `lift` at the cycle's angle a (0 off the event).
        double lift01(double a) const;
        // The lift (m) at a; a share when `lift` is not known.
        double liftAt(double a) const;
        // The seat events: where it leaves the seat and lands on it (the cycle's angles, 0..720).
        double seatOpen() const;
        double seatClose() const;
    };

    // The valvetrain as the build has it.
    struct Train
    {
        Lobe in, ex;                 // stage A (or the only one)
        Lobe inB, exB;               // stage B, when `stages`
        Lobe aux;                    // the second exhaust event, when on (known() otherwise false)
        bool stages = false, reference = false, labOff = false;   // the reference's dial; the lab's model is off
        double inPhaseRange = kNaN, exPhaseRange = kNaN;           // degrees
        double stageSpeed = kNaN;                                   // rad/s
        bool editable = false;       // the handles have fields to write
    };
    Train trainOf(const FieldDoc& doc);

    // The events as a cam card quotes them (degrees): IVO before top, IVC after bottom, EVO before bottom, EVC after top;
    // the overlap at the seat; the lobe separation.
    struct Events
    {
        double ivo = kNaN, ivc = kNaN, evo = kNaN, evc = kNaN, overlap = kNaN, lsa = kNaN;
    };
    Events eventsOf(const Lobe& in, const Lobe& ex);

    // The flow bench (bench.fm): flow (m^3/s of air) at lifts 0.050 in to 0.800 in, for each valve's head.
    struct Bench
    {
        std::array<double, 16> lift{}, flowIn{}, flowEx{};
        int nIn = 1, nEx = 1;
        double dIn = kNaN, dEx = kNaN;
        bool known = false;
        bool simulated = false;      // the simulator's own (telemetry), not this port
    };
    Bench benchOf(const FieldDoc& doc, double bore);
    // The simulator's bench when its telemetry has all of it, else benchOf().
    Bench benchOf(const FieldDoc& doc, double bore, const Live& live);
    // One head's flow at lift l (m): bench.fm's orifice through area.fm's effective area.
    double benchFlow(int n, double d, double q, double l);
}
