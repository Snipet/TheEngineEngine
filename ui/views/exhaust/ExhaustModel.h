// ui/views/exhaust/ExhaustModel.h: the exhaust as built, resolved from the build's fields the way the simulator's
// builder resolves it (sim/exhaust/slots.fm, XBuild::resolve; the layouts of sim/core/layouts.fm), so the stage draws
// the system the simulator makes: which collector each primary joins (or an open stack), which collectors merge, how
// many sides, each side's chain of slots, the turbine housings (sim/boost/machinespec.fm's arrangement).
//
// Every field is read by path through one table (ExhaustPaths, below) with the Spec's own default as its fallback: the
// physics packages are being merged into the one Spec, and a rename there is a one-line change here. A field the build
// does not have yet reads as its default, which is what the simulator would use.
#pragma once

#include "kit/bind/Fields.h"

#include <array>
#include <string>
#include <string_view>

namespace tee::ui::exhaust
{
    inline constexpr int kPorts = 16, kCollectors = 8, kSides = 2;

    // The paths read, and the Spec's defaults (sim/exhaust/buildspec.fm XSpec, sim/spec/*.fm). An array's element i is
    // "<path>[i]" (BuildDoc's naming).
    struct ExhaustPaths
    {
        static constexpr const char* layout = "engine.layout";          // the real Spec's 17 layouts (default 12)
        static constexpr const char* boostKind = "boost.kind";          // natural, turbo, twin, quad, supercharger
        static constexpr const char* boostLayout = "boost.layout";      // the turbos' arrangement (card F)
        static constexpr const char* headersDial = "exhaust.headers";   // 0..1: 0.61 to 1.63 times the layout's
        static constexpr const char* pipeDial = "exhaust.pipe";         // 0..1: half to twice the layout's main pipe
        static constexpr const char* primaryTo = "exhaust.primaryTo";   // [16] -2 the layout's, -1 an open stack
        static constexpr const char* primLen = "exhaust.primLen";       // [16] m, 0 the layout's
        static constexpr const char* primSpread = "exhaust.primSpread";
        static constexpr const char* collectorTo = "exhaust.collectorTo"; // [8] -1 the layout's; 0, 1 a side; 2+ a merge
        static constexpr const char* colLen = "exhaust.colLen";         // [8]
        static constexpr const char* colVol = "exhaust.colVol";         // [8] > 0: a log manifold
        static constexpr const char* sides = "exhaust.sides";           // 0 the layout's
        static constexpr const char* mainLen = "exhaust.mainLen";       // [2]
        static constexpr const char* converter = "exhaust.converter";   // [2] none, standard, high-flow
        static constexpr const char* crossKind = "exhaust.crossKind";   // none, H, X (card G's)
        static constexpr const char* crossover = "exhaust.crossover";   // without crossKind: the reference's dial (an H
                                                                        // when open), or the demo's switch (an X)
        static constexpr const char* crossAt = "exhaust.crossAt";
        static constexpr const char* resonator = "exhaust.resonator";   // [2] none, quarter-wave, Helmholtz, bottle
        static constexpr const char* mufKind = "exhaust.mufKind";       // [2] none, straight, chambered, three-pass
        static constexpr const char* muffler = "exhaust.muffler";       // [2] its name before card G's rename
        static constexpr const char* mufLen = "exhaust.mufLen";         // [2]
        static constexpr const char* bypass = "exhaust.bypass";         // [2] none, shut, open, by speed, by the ECU
        static constexpr const char* tailLen = "exhaust.tailLen";       // [2]
        static constexpr const char* tips = "exhaust.tips";             // [2] 1 or 2
        static constexpr const char* exitAt = "exhaust.exitAt";         // [2] -1 the layout's, then XTAG_*
        static constexpr const char* cycle = "engine.cycle";            // 0 four-stroke, 1 two-stroke (card T)
    };

    // Where a radiating end is (sim/exhaust/ends.fm XTAG_*).
    enum class Tag : int { rearCentre = 0, rearLeft, rearRight, sideLeft, sideRight, front, up };
    const char* tagName(Tag t) noexcept;   // "REAR LEFT"

    enum class Turbos : int { none = 0, one, perBank, sequential, compound, fourPerBank };

    // A two-stroke's pipe (engine.cycle 1: card T's ts* fields, sim/spec/engine.fm; sim/core/twostroke/chamber.fm):
    // a header, a diffuser cone, a belly, a baffle cone and a stinger (or one straight pipe), a silencer after it; one
    // a cylinder.
    struct TwoStrokePipe
    {
        bool on = false;
        int cylinders = 1;
        bool chamber = true;                      // false: a straight pipe of the header's bore, the whole length
        float header = 0.16f, diffuser = 0.33f, belly = 0.12f, baffle = 0.24f, stinger = 0.22f;   // m
        float dHeader = 0.038f, dBelly = 0.115f, dStinger = 0.022f;                                // m
        bool silencer = false;
        float silLen = 0.40f, silDiam = 0.026f;
        float exhaustDur = 192.0f;                // degrees the exhaust port is open (about bottom dead centre)
        float length() const noexcept { return header + diffuser + belly + baffle + stinger; }
        // chamber.fm's `tuned`: the length to the baffle cone's middle, where the plugging pulse comes back from
        float tunedLength() const noexcept { return chamber ? header + diffuser + belly + 0.5f * baffle : 0.0f; }
        // The speed the pipe is tuned for (rpm) at the pipe's gas temperature (K): card T's returning wave
        // (tests/sim/twostroke/PipeTest.cpp predicted(): c (exhaust duration - 25) / (12 L), c = sqrt(1.35 287 T) as
        // twostroke.fm soundSpeed()). 0 for a straight pipe.
        double tunedRpm(double gasK) const noexcept;
    };

    struct ExhaustBuild
    {
        int layout = 12;                    // sim/core/layouts.fm's index
        int ports = 8;
        std::array<int, kPorts> bank{};     // each port's bank, 0 or 1
        std::array<float, kPorts> fire{};   // where each port's cylinder fires, degrees after the first's (layouts.fm)
        float cycle = 720.0f;               // degrees a cycle (a rotary's face: 1080)
        std::array<float, kPorts> primLen{};   // m, as built (the spread included)
        std::array<int, kPorts> portTo{};   // its collector, -1 an open stack
        int sides = 2;
        std::array<bool, kCollectors> colOn{}, colFed{};
        std::array<int, kCollectors> colTo{};   // 0, 1 a side; 2 + j merges into collector j
        std::array<float, kCollectors> colLen{}, colVol{};
        int cross = 1;                      // 0 none, 1 H, 2 X
        int crossAt = 0;                    // 0 at the head, 1 after the main pipes
        std::array<bool, kSides> sideOn{};
        std::array<int, kSides> conv{}, res{}, muf{}, byp{}, tips{};
        std::array<Tag, kSides> tag{};
        std::array<float, kSides> mainLen{}, mufLen{}, tailLen{};
        Turbos turbos = Turbos::none;
        TwoStrokePipe ts;                   // engine.cycle 1: drawn instead of the four-stroke's system
        bool supercharger = false;

        bool openStacks() const noexcept;   // every port an open stack (zoomies, a single's open pipe)
        bool anyMerge() const noexcept;     // a second merge (4-2-1)
        // The engine and its exhaust in a few words: "V8 crossplane, 8 into 2, dual with an X, chambered".
        std::string sentence() const;
    };

    // The layouts' names (the real Spec's choices) and their tables.
    const char* layoutName(int layout) noexcept;
    // A layout's index from a choice's label: the real names, then the demo's short ones ("V8", "I4"); -1 none.
    int layoutFromLabel(std::string_view label) noexcept;

    ExhaustBuild resolveExhaust(const FieldDoc& doc);

    // A field by path (an element "<path>[i]" when i >= 0), its value, or `fallback` when the build has none.
    double fieldOr(const FieldDoc& doc, std::string_view path, int i, double fallback);
}
