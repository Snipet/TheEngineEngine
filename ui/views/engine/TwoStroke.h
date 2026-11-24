// ui/views/engine/TwoStroke.h: a crankcase-scavenged two-stroke as the ENGINE and VALVES stages draw it (card T's
// fields, sim/spec/engine.fm's marked block; docs/models/twostroke.md).
//
// With spec.engine.cycle 1 the engine fires every turn: a 360-degree cycle, 0 the firing top dead centre. Its ports
// are windows in the cylinder wall the piston uncovers: each opens when the crown passes its top edge and is open for
// its duration about bottom dead centre (the exhaust's from 180 - dur/2 to 180 + dur/2, the transfers' likewise), so
// a port's top edge stands as far above bottom dead centre as the slider-crank puts the crown at its opening angle.
// The exhaust's share open at an angle follows from the same geometry: how far the crown has dropped below the
// port's top over the port's height. (Derived here from the fields; the Scope's shares, when it has the two-stroke's
// cycle, are the simulator's.) The power valve lowers the exhaust's top edge at low speed: its duration with the
// valve shut, opening between two speeds. The inlet is a reed (opened by the crankcase's pressure, no fixed timing),
// a rotary disc (open for its duration from its angle) or a piston port (open about top dead centre, as the skirt
// uncovers it).
#pragma once

#include "views/engine/EngineModel.h"

#include <string>

namespace tee::ui::engine
{
    enum class Inlet : int
    {
        reed,
        disc,
        pistonPort
    };

    struct TwoStroke
    {
        bool on = false;
        int cylinders = 1;
        bool together = false;      // all fire at once (else evenly, 360 / n apart)
        double exDur = kNaN, trDur = kNaN;              // degrees open, about bottom dead centre
        double exWidth = kNaN, trWidth = kNaN;          // over the bore
        double valveDur = kNaN, valveFrom = kNaN, valveTo = kNaN;   // the power valve: degrees shut; rad/s
        Inlet inlet = Inlet::reed;
        double discOpen = kNaN, discDur = kNaN, pistonDur = kNaN;   // degrees
        double inletArea = kNaN, carbArea = kNaN, reedFull = kNaN;  // m^2, m^2, Pa
        double crankcaseRatio = kNaN, spark = kNaN;                  // ratio; degrees before top

        // Where a port opens and closes (the cycle's degrees).
        double exOpens() const { return 180.0 - 0.5 * exDur; }
        double exCloses() const { return 180.0 + 0.5 * exDur; }
        double trOpens() const { return 180.0 - 0.5 * trDur; }
        double trCloses() const { return 180.0 + 0.5 * trDur; }
        bool hasValve() const { return std::isfinite(valveDur) && valveDur > 0.0; }
        // The exhaust's duration at a speed (rad/s): the power valve's, opening between its two speeds.
        double exDurAt(double speed) const;
        // A timed inlet's window (degrees from, for); false for a reed.
        bool inletWindow(double& from, double& dur) const;
        const char* inletName() const;
    };
    TwoStroke twoStrokeOf(const FieldDoc& doc);

    // Card T's telemetry (tel.engine; 0 for a four-stroke): the scavenging's measures, the power valve's place (0 shut,
    // 1 open), the crankcase's pressure (Pa absolute, cylinder 1's) and the reed's opening (0 shut, 1 on its stop).
    inline constexpr const char* kTelPurity = "engine.tsPurity";
    inline constexpr const char* kTelDelivery = "engine.tsDelivery";
    inline constexpr const char* kTelTrapping = "engine.tsTrapping";
    inline constexpr const char* kTelPowerValve = "engine.tsPowerValve";
    inline constexpr const char* kTelCrankcase = "engine.tsCrankcase";
    inline constexpr const char* kTelReed = "engine.tsReed";
    // The exhaust's duration now: from the power valve's place when telemetry has it, else from the speed by the fields.
    double exhaustDurationNow(const TwoStroke& t, const Live& live);

    // A port's height (m) above bottom dead centre for its duration, and its share open at angle `deg`.
    double portHeight(double dur, double radius, double rod);
    double portShare(double deg, double dur, double radius, double rod);

    // The two-stroke's phases by the cycle's angle: POWER, BLOWDOWN (exhaust open, transfers shut), SCAVENGE (both open),
    // TRAPPING (transfers shut, exhaust still open), COMPRESSION.
    const char* twoStrokePhase(double deg, const TwoStroke& t);
}
