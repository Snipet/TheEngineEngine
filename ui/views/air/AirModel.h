// ui/views/air/AirModel.h: the induction as built, resolved from the build's fields (card F's forced induction,
// sim/boost/machinespec.fm; card E's induction, sim/intake/lab-spec.fm), and the compressor's map made from its sizes
// with the simulator's own arithmetic (sim/boost/compressor.fm: size(), speed(), rise(), efficiency();
// sim/boost/machine.fm: the matched sizes and mapLines()), so the map drawn is the map the simulator runs on.
//
// Every field is read by path through one table (AirPaths) with the Spec's default as its fallback.
#pragma once

#include "kit/bind/Fields.h"

#include <array>
#include <string>
#include <vector>

namespace tee::ui::air
{
    struct AirPaths
    {
        static constexpr const char* layout = "engine.layout";
        static constexpr const char* displacement = "engine.displacement";   // m^3; absent: from bore, stroke, cylinders
        static constexpr const char* redlineTrim = "engine.redlineTrim";
        static constexpr const char* kind = "boost.kind";                    // natural, turbo, twin, quad, supercharger
        static constexpr const char* arrangement = "boost.layout";
        static constexpr const char* size = "boost.size";
        static constexpr const char* compInducer = "boost.compInducer";
        static constexpr const char* compExducer = "boost.compExducer";
        static constexpr const char* compAR = "boost.compAR";
        static constexpr const char* compEta = "boost.compEta";
        static constexpr const char* maxSpeed = "boost.maxSpeed";
        static constexpr const char* vanes = "boost.vanes";                  // fixed, variable
        static constexpr const char* gate = "boost.gate";                    // internal, external
        static constexpr const char* gateSpring = "boost.gateSpring";
        static constexpr const char* target = "boost.target";
        static constexpr const char* blowOffKind = "boost.blowOffKind";      // none, to the air, recirculating
        static constexpr const char* icEffect = "boost.icEffect";
        static constexpr const char* charger = "boost.charger";              // Roots, twin-screw, centrifugal
        static constexpr const char* scRatio = "boost.scRatio";
        static constexpr const char* scStepUp = "boost.scStepUp";
        static constexpr const char* scBypass = "boost.scBypass";
        static constexpr const char* antiLag = "boost.antiLag";
        static constexpr const char* nitrousShot = "boost.nitrousShot";
        static constexpr const char* nitrousBottle = "boost.nitrousBottle";
        static constexpr const char* throttleKind = "intake.throttleKind";    // a plenum, a body a runner
        static constexpr const char* runnerLength = "intake.runnerLength";    // m, 0: the reference's 0.30 (a bike's 0.16)
        static constexpr const char* runnerLength2 = "intake.runnerLength2";  // m, 0: one length
        static constexpr const char* runnerSwitch = "intake.runnerSwitch";    // rad/s: the second length from
        static constexpr const char* runnerBore = "intake.runnerBore";        // m, 0: from the cylinder's bore
        static constexpr const char* boreStroke = "engine.boreStroke";
        static constexpr const char* compression = "engine.compression";
        static constexpr const char* trumpets = "intake.trumpets";          // to the air box, to the air
        static constexpr const char* ambientP = "env.pressure";
        static constexpr const char* ambientT = "env.temperature";
    };

    enum class Charger : int { none = 0, turbo, supercharger };
    enum class Arrangement : int { none = 0, one, twin, sequential, compound, quad };
    enum class Blower : int { roots = 0, screw, centrifugal };

    // The compressor's map from its sizes (sim/boost/compressor.fm), at the standard inlet.
    struct CompressorMap
    {
        float rTip = 0.03f, aInd = 0.0015f, arWide = 1.0f, phiK = 0.5f, phiB = 0.725f, etaMax = 0.76f;
        // a speed's: the choke flow, the rises, the efficiency's Mach term
        struct Line
        {
            float omega = 0, mc = 0, tauK = 0, tau0 = 0, tauC = 0, etaM = 0;
        };
        void size(float dInd, float dEx, float ar, float etaPeak);
        Line at(float omega) const;                    // at 101325 Pa and 293.15 K
        static float rise(const Line& l, float phiK, float phi);
        float efficiency(const Line& l, float phi) const;
        static float ratio(float tau);                  // (1 + tau)^3.5
    };

    // What the map shows: speed lines from knee to choke, the surge line, the choke line, efficiency islands.
    struct MapLines
    {
        static constexpr int kLines = 8, kPoints = 16, kLevels = 5;
        // the islands are traced over finer speed steps than the lines drawn (the simulator's 8 make them angular)
        static constexpr int kIslandSteps = 36;
        std::array<std::array<float, kPoints>, kLines> flow{}, pr{};
        std::array<float, kLines> speed{};             // rad/s of each line
        // each island: its left crossings up the speeds, then its right crossings down them; a flow < 0: none there.
        // A left crossing beyond the knee (in the surge side, where no line is drawn) is held at the surge line.
        std::array<std::array<float, 2 * kIslandSteps>, kLevels> islandFlow{}, islandPr{};
        std::array<float, kLevels> islandEta{};
        float maxFlow = 0, maxPr = 1;
    };
    MapLines mapLines(const CompressorMap& m);

    struct AirBuild
    {
        int layout = 12;
        int cylinders = 8;
        double displacement = 0.00576;
        double redRpm = 6500;
        Charger charger = Charger::none;
        Arrangement arrangement = Arrangement::none;
        int turbos = 0;                 // compressors drawn
        Blower blower = Blower::roots;
        bool vanes = false, externalGate = false, intercooler = false, antiLag = false, nitrous = false;
        bool scBypass = true;
        int blowOff = 1;                // none, to the air, recirculating
        double scRatio = 2.4, gateSpring = 70e3, target = 0, nitrousBottle = 4.5;
        bool itb = false, trumpetsToAir = false;
        double runner = 0.30, runner2 = 0.0, runnerSwitch = 0.0;   // m, m (0: one length), rad/s
        double runnerArea = 0.0;        // m^2, as the simulator sizes it (sim/intake/intake.fm runners())
        double cylinderMean = 0.0;      // m^3: a cylinder's mean volume over the intake stroke (clearance + half swept)
        double ambientP = 101325, ambientT = 293.15;
        // The document's env.pressure as set (U1 for U3, 26 Nov). The engine does not read it yet (only the car's drag
        // does: the lead's finding, sim/engine.fm's literal 101325), so ambientP stays at sea level until the next
        // window's item 15 makes the engine breathe it; this is what the owner set, for the warning that says so.
        double docAmbientP = 101325;
        CompressorMap map;              // turbo 1's (or a centrifugal blower's)
        double omegaMax = 0;            // the shaft's limit, rad/s
        bool hasMap() const noexcept { return turbos > 0 || (charger == Charger::supercharger && blower == Blower::centrifugal); }
        std::string sentence() const;
    };

    AirBuild resolveAir(const FieldDoc& doc);

    // Where a runner of `length` (m) is tuned, derived (card E's relation, tests/sim/valvetrain/IntegratedTest.cpp test
    // 12's ring(): with the intake valve open the runner rings as a pipe loaded by the cylinder's volume, x tan x =
    // A L / V with L its length plus 0.6 of its radius, at f = x c / (2 pi L), c = 350 m/s). The speeds the ring meets
    // the intake event at: the main bump (each cycle) and the lower (every other), from card E's measured bumps on a
    // 2.0 L four with a 280-degree cam (7640 and 3180 rpm at 161 Hz, 5250 and 2300 at 117 Hz): 46.2 and 19.7 rpm a hertz
    // (judgement: a cam of another duration moves them some per cent).
    struct RamTune
    {
        double hz = 0, mainRpm = 0, lowerRpm = 0;
    };
    RamTune ramTune(const AirBuild& b, double length);
}
