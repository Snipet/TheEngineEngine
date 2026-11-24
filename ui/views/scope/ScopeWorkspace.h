// ui/views/scope/ScopeWorkspace.h: SCOPE, the instrument bench (docs/plan/ui-design.md section 3; docs/ui/scope.md).
//
// Six plots over the newest copy of the Scope and the output, all read only while the workspace is shown (its step()
// is called only then), all held by one FREEZE:
//
//   CYLINDER PRESSURE   cylinder 1's pressure against crank degrees from its firing top centre, the valves' open
//                       periods as bands under it (IVO, IVC, EVO, EVC from the lifts), the spark (ecu.advance), the
//                       peak's pressure and angle as numbers in the bar over the plots.
//   P-V LOOP            the same cycle against the cylinder's volume, linear or log-log (LOG P-V), the valve events and
//                       the spark as points on it.
//   PORTS               the intake and exhaust port pressures over the cycle on one plot (Scope "portIntake",
//                       "portExhaust"), the valves' open periods under them: the tuned pulses arriving.
//   OUTPUT WAVE         the output's last whole cycles (TelemetrySource::outputTail), triggered on the crank's cycle
//                       (engine.crank) so it stands still at steady speed; on its own rising edge when there is none.
//   SPECTRUM            the output's spectrum (Spectrum.h), dB against log frequency, the firing frequency and its
//                       orders marked and numbered from the engine's speed and its cylinders.
//   SPEED MAP           the spectrum against engine speed (a Campbell diagram) filled in as the engine sweeps, the
//                       orders as slanted lines, a scale of dB; CLEAR empties it.
//
// Every plot has the kit's cursor (hover: a line and every trace's value at it, in the title line). A name a program
// does not have yet draws its plot empty, with a line saying what is missing; nothing is made up.
#pragma once

#include "app/Look.h"
#include "app/Workspace.h"
#include "kit/widgets/Basic.h"
#include "views/scope/ScopePlot.h"
#include "views/scope/Spectrum.h"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace tee::ui
{
    class ScopeWorkspace final : public Workspace
    {
    public:
        explicit ScopeWorkspace(WorkspaceContext& c);

        const char* id() const override { return "scope"; }
        const char* title() const override { return "SCOPE"; }
        // The inspector: the listener (sim/spec/sound.fm), until it has a workspace of its own.
        std::vector<std::string> sections() const override { return { "sound" }; }
        bool moving() const override;
        std::uint64_t drawKey() const override;

        // ---- for tests and the inspector ----------------------------------------------------------------------------
        bool frozen() const noexcept { return frozen_; }
        void setFrozen(bool f);
        bool logPv() const noexcept { return logPv_; }
        void setLogPv(bool on);
        void clearMap();
        // On a small stage: show plot i (0 pressure, 1 P-V, 2 ports, 3 wave, 4 spectrum, 5 speed map), or hide it when
        // shown (one always stays); at most three, the one chosen longest ago going.
        void choosePlot(int i);
        const std::vector<int>& chosenPlots() const noexcept { return chosen_; }
        bool compact() const noexcept { return compact_; }
        // The plots' panels as laid out (empty: not shown), and the bar's: for tests of what overlaps.
        const std::array<funkgui::Rect, 6>& panels() const noexcept { return cells_; }
        const funkgui::Rect& bar() const noexcept { return bar_; }
        const funkgui::Rect& tools() const noexcept { return tools_; }

        // What the newest data says (NaN when absent).
        struct Facts
        {
            double rpm = 0, peakPa = 0, peakDeg = 0, sparkDeg = 0, firingHz = 0;   // sparkDeg: before top centre
            double ivo = 0, ivc = 0, evo = 0, evc = 0;                             // degrees after firing top centre
            int degrees = 0, cylinders = 0;
            double firingOrder = 0;     // firing events per crank revolution
            double halfBurn = 0;        // degrees after firing top centre where half the charge has burned
            double knocks = 0;          // ecu.knock: knocks heard since the start
            std::vector<double> knockRetard;   // ecu.knockByCylinder[i], degrees (NaN: absent), one a cylinder
        };
        const Facts& facts() const noexcept { return f_; }
        const scope::Spectrum& spectrum() const noexcept { return spectrum_; }
        const scope::Campbell& campbell() const noexcept { return campbell_; }
        int scopeReads() const noexcept { return reads_; }

    protected:
        void layout(const funkgui::Rect& stage) override;
        void drawStage(funkgui::Canvas& c, const funkgui::Theme& th) const override;
        void step(float dt) override;

    private:
        void readScope();
        void readOutput(float dt);
        void refreshPlots();
        void cylinderFacts();
        void readCombustion();
        void drawKnock(funkgui::Canvas& c, const look::Palette& p) const;
        double toAtdc(int index) const noexcept;
        double rpmNow() const;
        void drawCampbell(funkgui::Canvas& c, const look::Palette& p, const ScopePlot& pl) const;
        void drawOrders(funkgui::Canvas& c, const look::Palette& p, const ScopePlot& pl) const;
        void drawValveBands(funkgui::Canvas& c, const look::Palette& p, const ScopePlot& pl, bool labels) const;

        // the plots, in the stage's order
        ScopePlot* pressure_ = nullptr;
        ScopePlot* pv_ = nullptr;
        ScopePlot* ports_ = nullptr;
        ScopePlot* wave_ = nullptr;
        ScopePlot* spec_ = nullptr;
        ScopePlot* map_ = nullptr;
        Button* freeze_ = nullptr;
        Button* log_ = nullptr;
        Button* clear_ = nullptr;
        std::array<funkgui::Rect, 6> cells_{};
        funkgui::Rect bar_{}, tools_{}, knock_{};
        std::array<Button*, 6> selectors_{};
        std::vector<int> chosen_{ 0, 4, 5 };   // a small stage's: pressure, spectrum, speed map
        bool compact_ = false;

        bool frozen_ = false, logPv_ = false;
        // the newest copy
        std::vector<float> p_, v_, li_, le_, pi_, pe_, tail_;
        int degrees_ = 0, tdc_ = 0, tailN_ = 0;
        std::uint64_t seenGeneration_ = ~0ull, tailSum_ = 0;
        int reads_ = 0;
        Facts f_;
        scope::Spectrum spectrum_;
        scope::Campbell campbell_;
        float sinceSpectrum_ = 1.0f;
        std::vector<double> cycleMarksMs_;
        std::uint64_t revision_ = 1;
        bool live_ = false;
    };
}
