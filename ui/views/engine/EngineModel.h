// ui/views/engine/EngineModel.h: what the ENGINE and VALVES stages know of the engine, read from the build by path.
//
// Every field, telemetry name and Scope array the two stages read is named once, in the tables below (kEngineFields,
// kValveFields in ValvesModel.h), each with the path it has on the integration branch and the path the standalone
// spec or the demo gives it: when the physics packages' fields land under their final names, a rename is a one-line
// change here. A field that is in neither place reads as NaN, and the stage draws a dash: no value is invented.
//
// The layout: either one of the reference's seventeen (spec.engine.layout, sim/core/layouts.fm: the firing angles,
// copied here) or, with spec.engine.custom on, the cylinders as package D gives them (their banks, crank pins, firing
// revolutions and places; sim/core/layout.fm's derivation, done again here in the same order). The seventeen carry no
// geometry for a picture, so the bank angles drawn for them are the usual ones (a V8's 90 degrees, a flat engine's
// 180) and the crank pins follow from the bank angles and the firing angles (fire = bank angle - pin, plus 360 on the
// second revolution).
//
// The cylinder: bore, stroke and rod from package D's fields when they are set (above 0), else from the reference's
// dials (the displacement shared among the cylinders, and the bore over the stroke), and the rod 3.3 times the crank's
// radius as the reference's is. All SI.
#pragma once

#include "app/Workspace.h"

#include <array>
#include <cmath>
#include <limits>
#include <string>
#include <string_view>
#include <vector>
#include <vector>

namespace tee::ui::engine
{
    inline constexpr double kPi = 3.14159265358979323846;
    inline constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
    inline constexpr int kMaxCyl = 16;   // spec/limits.fm MAX_CYL
    inline constexpr int kMaxBanks = 4;  // core/mechspec.fm MECH_MAX_BANKS

    // ---- the bindings ------------------------------------------------------------------------------------------------
    // A field by its path on the branch, else the standalone spec's or the demo's ("" : none). %d in a path is an
    // array's index.
    struct FieldPath
    {
        const char* path;
        const char* alt;
    };
    enum class EF : int
    {
        layout, custom, cylinders, rotors, bankAngle, bank, pin, fireRev, place,
        displacement, boreStroke, bore, stroke, rod, compression,
        pistonMass, rodMass, rodRecip, flywheel, limiter, cycle,
        count
    };
    inline constexpr FieldPath kEngineFields[] = {
        { "engine.layout", "" },
        { "engine.custom", "" },            // package D (core/mechspec.fm), appended to spec.engine
        { "engine.cylinders", "" },
        { "engine.rotors", "" },
        { "engine.bankAngle[%d]", "" },
        { "engine.bank[%d]", "" },
        { "engine.pin[%d]", "" },
        { "engine.fireRev[%d]", "" },
        { "engine.place[%d]", "" },
        { "engine.displacement", "" },      // the reference's SIZE dial (m^3)
        { "engine.boreStroke", "" },
        { "engine.bore", "" },              // D's (0: from the dials); the demo's
        { "engine.stroke", "" },
        { "engine.rodLength", "engine.rod" },
        { "engine.compression", "" },
        { "engine.pistonMass", "" },
        { "engine.rodMass", "" },
        { "engine.rodRecip", "" },
        { "engine.flywheelInertia", "engine.flywheel" },   // D's kg m^2; else the reference's dial (and the demo's)
        { "ecu.limiter", "engine.redline" },
        { "engine.cycle", "" },             // card T: 1 a two-stroke (views/engine/TwoStroke.h reads its own fields)
    };
    static_assert(std::size(kEngineFields) == static_cast<std::size_t>(EF::count));

    // Telemetry (sim/spec/telemetry.fm) and the Scope's arrays (sim/spec/scope.fm) the stages read.
    inline constexpr const char* kTelCrank = "engine.crank";       // degrees, 0 to 2160
    inline constexpr const char* kTelSpeed = "engine.speed";       // rad/s
    inline constexpr const char* kTelRedline = "engine.redline";   // rad/s
    inline constexpr const char* kTelState = "engine.state";
    inline constexpr const char* kTelIvo = "valve.ivo";           // cylinder 1's valve events now (the cycle's degrees)
    inline constexpr const char* kTelEvo = "valve.evo";
    inline constexpr const char* kTelAdvance = "ecu.advance";     // degrees before top dead centre (the spark's)
    inline constexpr const char* kScopePressure = "pressure";
    inline constexpr const char* kScopeVolume = "volume";
    inline constexpr const char* kScopeLiftIn = "liftIntake";
    inline constexpr const char* kScopeLiftEx = "liftExhaust";
    inline constexpr const char* kScopeDegrees = "degrees";

    // The field's index in the document (-1: neither path is there).
    int findField(const FieldDoc& doc, const FieldPath& f, int index = -1);
    // Its value (SI), NaN when absent.
    double fieldValue(const FieldDoc& doc, const FieldPath& f, int index = -1);
    // The path that is there ("" : none).
    std::string fieldPath(const FieldDoc& doc, const FieldPath& f, int index = -1);
    inline double fieldValue(const FieldDoc& doc, EF f, int index = -1)
    {
        return fieldValue(doc, kEngineFields[static_cast<int>(f)], index);
    }
    inline std::string fieldPath(const FieldDoc& doc, EF f, int index = -1)
    {
        return fieldPath(doc, kEngineFields[static_cast<int>(f)], index);
    }

    // x mod m in [0, m)
    inline double wrap(double x, double m) { double r = std::fmod(x, m); return r < 0.0 ? r + m : r; }

    // ---- the layout --------------------------------------------------------------------------------------------------
    struct Layout
    {
        std::string name;                 // "V8 crossplane", "Custom", "" (none)
        int n = 0;                        // cylinders (a rotary's faces)
        int rotors = 0;                   // a rotary's rotors (0: pistons)
        double cyc = 720.0;               // degrees a cycle
        int banks = 1;
        std::array<double, kMaxBanks> bankAngle{};   // degrees from vertical, + the way the crank turns
        std::array<int, kMaxCyl> bank{};             // each cylinder's bank (a face's: its rotor's)
        std::array<double, kMaxCyl> pin{};           // degrees, its crank pin at crank angle 0
        std::array<double, kMaxCyl> fire{};          // its firing angle after cylinder 1's, 0 to cyc
        std::array<double, kMaxCyl> along{};         // its place along the crank, 0 the front, in pitches
        std::array<int, kMaxCyl> order{};            // the firing order: indexes from 0
        std::array<double, kMaxCyl> interval{};      // degrees from each firing (in order) to the next
        bool custom = false;
        bool twoStroke = false;           // fires every turn: cyc 360
        bool ok = true;                   // false: two cylinders fire together, or a bank has no angle

        bool empty() const noexcept { return n <= 0; }
        // Each cylinder's angle into its own cycle (0 its firing top dead centre) when cylinder 1's is `a1`.
        double cylinderAngle(int c, double a1) const { return wrap(a1 - fire[static_cast<std::size_t>(c)], cyc); }
        // The firing order as text: "1-8-4-3-6-5-7-2".
        std::string orderText() const;
        // The intervals in order as text: "even, 90" or "270-180-90-180".
        std::string intervalText() const;
        bool even() const;
    };

    // The reference's seventeen (sim/core/layouts.fm), by index, and their names (spec/engine.fm's choices).
    inline constexpr int kPresetCount = 17;
    const char* presetName(int i);
    Layout presetLayout(int i);
    // A preset's index by a choice's text: the spec's names, and the short ones a build may use ("V8", "I4").
    int presetByName(std::string_view name);

    // Whether the build runs as a two-stroke (package D's configure, X-025): engine.cycle 1 with a custom layout of
    // pistons; without custom the cycle is ignored and the dial's layout runs; a custom rotary runs as itself (and D
    // flags the refusal). A document without D's custom switch (the demo's) runs the cycle as asked.
    bool twoStrokeRuns(const FieldDoc& doc);
    // The layout the document describes (custom, a preset by its choice, or none). `cycle2` false: as though the
    // cycle were four-stroke (what runs when D refuses a two-stroke of a rotary).
    Layout layoutOf(const FieldDoc& doc, bool cycle2 = true);
    // The layout dial's preset alone (what the simulator plays when it refuses a custom layout), or none.
    Layout dialLayoutOf(const FieldDoc& doc);
    // D's flag (finding X-001): 1 while a custom layout is refused and the dial's runs in its place.
    inline constexpr const char* kTelLayoutRefused = "engine.layoutRefused";
    // D's reason while refused: 1 cylinders fire together, 2 rotors fire together, 3 a cylinder's bank is outside the
    // banks, 4 a two-stroke asked of a rotary (it then runs as itself, a four-stroke-equivalent rotary) (0 or absent:
    // none given; the stage then says why from the build when it can).
    inline constexpr const char* kTelLayoutReason = "engine.layoutReason";

    // The firing angles, order and intervals from banks, pins and firing revolutions (sim/core/layout.fm's derive()).
    void deriveFiring(Layout& l, const std::array<int, kMaxCyl>& fireRev);

    // ---- the cylinder ------------------------------------------------------------------------------------------------
    struct Cylinder
    {
        double bore = kNaN, stroke = kNaN, rod = kNaN, compression = kNaN;   // m, m, m, ratio
        bool rodDerived = false, boreDerived = false;                        // from the dials, not set outright
        bool known() const noexcept { return std::isfinite(bore) && std::isfinite(stroke) && bore > 0.0 && stroke > 0.0; }
        double radius() const noexcept { return stroke * 0.5; }
        double swept() const noexcept { return kPi / 4.0 * bore * bore * stroke; }
    };
    Cylinder cylinderOf(const FieldDoc& doc, const Layout& l);

    // The slider-crank: the piston's distance down from top dead centre at crank angle `deg` (0: top), m.
    double pistonDrop(double deg, double radius, double rod);

    // Derived numbers (all SI; NaN when what they come from is absent).
    struct Derived
    {
        double displacement = kNaN;   // m^3, all the cylinders
        double boreStroke = kNaN;     // bore over stroke
        double rodRatio = kNaN;       // rod length over stroke
        double pistonSpeed = kNaN;    // m/s, mean, at the limiter
        double limiter = kNaN;        // rad/s, what the piston speed is at
        double compression = kNaN;
    };
    Derived derive(const Layout& l, const Cylinder& c, double limiter);

    // Geometry no engine has (the rule the lead and package D settled: the engine survives it, the interface warns):
    // each warning in words, and the fields it concerns. Derived from the build, never a block on a value.
    //   a piston heavier than about 1500 kg/m^3 x bore^3 (about twice a real one's)
    //   stroke / bore outside 0.5 to 2 (no production engine is so far over- or under-square)
    //   a mean piston speed at the limiter over 30 m/s (past any race engine's)
    //   rod / stroke under 1.4 (shorter than any production engine's)
    struct Warning
    {
        std::string text;
        std::vector<EF> fields;
    };
    std::vector<Warning> geometryWarnings(const FieldDoc& doc, const Layout& l, const Cylinder& c, const Derived& d);
}
