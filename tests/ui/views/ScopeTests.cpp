// tests/ui/views/ScopeTests.cpp: SCOPE (ui/views/scope/): its stage over the demo and over nothing, the FFT, the
// speed map filled by a sweep, FREEZE, LOG P-V, at rest. Pictures by FunkGui's software rasteriser, at density 2 and 1.
#include "ViewKit.h"

#include "views/scope/ScopeDemo.h"
#include "views/scope/ScopeWorkspace.h"
#include "views/scope/Spectrum.h"

#include <funkgui/core/Theme.h>

#include <cmath>
#include <memory>
#include <string>

namespace
{
    using namespace tee::test::views;
    using tee::ui::ScopeWorkspace;
    using tee::ui::WorkspaceContext;
    namespace scope = tee::ui::scope;

    // SCOPE over the demo, with what the demo lacks added (ScopeDemo: the port pressures and the spark's advance).
    struct ScopeRig
    {
        tee::ui::DemoRig demo;
        scope::ScopeDemo extra{ demo.engine };
        WorkspaceContext ctx{ demo.doc, demo.engine, extra, demo.engine, demo.units, demo.selection, demo.app };
        std::unique_ptr<ScopeWorkspace> ws;
        std::unique_ptr<funkgui::HeadlessHost> host;

        explicit ScopeRig(int w = 1100, int h = 700, float dpi = 2.0f, double t = 4.6, int theme = 0) : demo(t)
        {
            scope::scopeDemoTelemetry(demo.engine);
            ws = std::make_unique<ScopeWorkspace>(ctx);
            host = std::make_unique<funkgui::HeadlessHost>(*ws, theme, dpi);
            host->resize(w, h);
            ws->place({ 0.0f, 0.0f, static_cast<float>(w), static_cast<float>(h) });
            settle();
        }
        // The demo moved to `t` and its Scope's newer copy taken (the engine's tick moves its generation).
        void at(double t, int frames = 4)
        {
            demo.at(t);
            for (int i = 0; i < frames; ++i)
            {
                demo.engine.tick(0.0f);
                host->tick(1);
            }
        }
        void settle() { host->tick(90); }
    };

    // The stage over the demo: the numbers it reads are the cycle's, the orders are where the speed puts them.
    VIEW_TEST(testScopeStage, "scope.stage")
    {
        ScopeRig rig;
        const ScopeWorkspace::Facts& f = rig.ws->facts();
        CHECK(f.degrees == 720);
        CHECK(f.cylinders == 8 && f.firingOrder == 4.0);
        CHECK(f.peakPa > 3e6 && f.peakDeg > 0.0 && f.peakDeg < 40.0);   // the peak a little after firing top centre
        CHECK(std::isfinite(f.evo) && std::isfinite(f.evc) && std::isfinite(f.ivo) && std::isfinite(f.ivc));
        CHECK(f.evo > 90.0 && f.evo < 180.0);                           // the exhaust opens before bottom centre
        CHECK(f.ivc > -180.0 && f.ivc < -60.0);                        // the intake closes after its bottom centre
        CHECK_NEAR(f.sparkDeg, 26.0, 1e-9);
        CHECK_NEAR(f.firingHz, f.rpm / 60.0 * 4.0, 1e-6);
        // The demo's output is a sawtooth at the firing frequency: the spectrum's peak is there.
        const double peak = rig.ws->spectrum().peakHz(20.0, 2000.0);
        CHECK_MSG(std::fabs(peak - f.firingHz) <= rig.ws->spectrum().binHz() * 1.5,
                  std::to_string(peak) + " Hz against " + std::to_string(f.firingHz));
        golden("scope.stage", *rig.host);
        ScopeRig one(1100, 700, 1.0f);
        golden("scope.stage.dpi1", *one.host);
        return 0;
    }

    // A large window, both densities, and the paper theme.
    VIEW_TEST(testScopeLarge, "scope.large")
    {
        ScopeRig big(1600, 1000, 2.0f);
        golden("scope.large", *big.host);
        ScopeRig one(1600, 1000, 1.0f);
        golden("scope.large.dpi1", *one.host);
        ScopeRig paper(1100, 700, 2.0f, 4.6, 1);
        golden("scope.paper", *paper.host);
        return 0;
    }

    // The FFT: a sine's level and place; a band narrower than a bin; nothing in, the floor out.
    VIEW_TEST(testScopeFft, "scope.fft")
    {
        scope::Spectrum s;
        CHECK(s.empty() && std::isnan(s.peakHz(20.0, 2000.0)));
        std::vector<float> x(scope::Spectrum::kSize);
        const double f = 1000.0, rate = 48000.0;
        for (std::size_t i = 0; i < x.size(); ++i)
            x[i] = static_cast<float>(0.5 * std::sin(2.0 * scope::kPi * f * static_cast<double>(i) / rate));
        s.analyse(x.data(), static_cast<int>(x.size()), rate, 0.0f);
        CHECK(!s.empty());
        CHECK(std::fabs(s.peakHz(100.0, 10000.0) - f) <= s.binHz());
        // 0.5 full scale is -6 dB; Hann's scalloping takes at most 1.5 dB off between bins.
        CHECK(s.bandDb(f * 0.98, f * 1.02) <= -6.0 + 0.2 && s.bandDb(f * 0.98, f * 1.02) >= -6.0 - 1.6);
        CHECK(s.bandDb(5000.0, 6000.0) < -60.0);   // far from it, the window's leakage is small
        const float narrow = s.bandDb(1000.0, 1001.0);
        CHECK(narrow > -10.0f);
        // Fewer samples than the transform: zeros before them, no crash.
        s.clear();
        s.analyse(x.data(), 100, rate, 0.0f);
        CHECK(!s.empty());
        // Silence reads the floor.
        std::vector<float> zero(scope::Spectrum::kSize, 0.0f);
        s.clear();
        s.analyse(zero.data(), static_cast<int>(zero.size()), rate, 0.0f);
        CHECK(s.db(10) == scope::Spectrum::kFloorDb);
        // The speed map: a spectrum at a speed fills its column, nothing else.
        scope::Campbell m(10, 8, 0.0, 1000.0, 100.0, 10000.0);
        s.clear();
        s.analyse(x.data(), static_cast<int>(x.size()), rate, 0.0f);
        m.take(450.0, s);
        CHECK(m.filled() == 1 && m.columnOf(450.0) == 4);
        CHECK(std::isfinite(m.cell(4, 3)) && std::isnan(m.cell(3, 3)));
        m.take(2000.0, s);   // outside its speeds: not kept
        CHECK(m.filled() == 1);
        m.clear();
        CHECK(m.filled() == 0 && std::isnan(m.cell(4, 3)));
        return 0;
    }

    // The speed map filled in by the demo's pull through the gears; CLEAR MAP empties it.
    VIEW_TEST(testScopeCampbell, "scope.campbell")
    {
        ScopeRig rig;
        CHECK(rig.ws->campbell().filled() <= 1);
        for (double t = 2.0; t < 6.2; t += 0.04)
            rig.at(t, 3);
        rig.settle();
        CHECK_MSG(rig.ws->campbell().filled() > 30, std::to_string(rig.ws->campbell().filled()));
        golden("scope.campbell", *rig.host);
        // CLEAR MAP: the button at the bar's right.
        rig.host->click(1100.0f - 10.0f - 12.0f - 48.0f, 10.0f + 29.0f);
        rig.settle();
        CHECK(rig.ws->campbell().filled() == 0);
        return 0;
    }

    // FREEZE holds every plot: a newer Scope and output are not read; the stage is at rest; again, it reads.
    VIEW_TEST(testScopeFreeze, "scope.freeze")
    {
        ScopeRig rig;
        const double peak = rig.ws->facts().peakPa;
        const int reads = rig.ws->scopeReads();
        // FREEZE: the third button from the right.
        rig.host->click(1100.0f - 10.0f - 12.0f - 96.0f * 2.0f - 16.0f - 48.0f, 10.0f + 29.0f);
        rig.settle();
        CHECK(rig.ws->frozen());
        rig.at(2.5, 6);   // the engine elsewhere in its script
        CHECK(rig.ws->scopeReads() == reads && rig.ws->facts().peakPa == peak);
        CHECK(!rig.ws->moving());
        const std::uint64_t key = rig.ws->drawKey();
        rig.host->tick(30);
        CHECK(rig.ws->drawKey() == key);
        picture("scope.frozen", *rig.host);
        rig.ws->setFrozen(false);
        rig.host->tick(2);
        CHECK(rig.ws->scopeReads() > reads && rig.ws->facts().peakPa != peak);
        return 0;
    }

    // LOG P-V: both axes logarithmic, the loop's straight lines (the polytropes) as they should be.
    VIEW_TEST(testScopeLogPv, "scope.logpv")
    {
        ScopeRig rig;
        rig.host->click(1100.0f - 10.0f - 12.0f - 96.0f - 8.0f - 48.0f, 10.0f + 29.0f);
        rig.settle();
        CHECK(rig.ws->logPv());
        golden("scope.logpv", *rig.host);
        return 0;
    }

    // At rest: a still demo asks for no redraw, and its draw key does not move.
    VIEW_TEST(testScopeRest, "scope.rest")
    {
        ScopeRig rig;
        rig.host->tick(5);
        CHECK(!rig.ws->moving());
        const std::uint64_t key = rig.ws->drawKey();
        rig.host->tick(30);
        CHECK(rig.ws->drawKey() == key);
        // Reading the Scope only while shown: a hidden workspace is not ticked, so a newer copy is not read.
        const int reads = rig.ws->scopeReads();
        rig.demo.engine.tick(0.0f);
        CHECK(rig.ws->scopeReads() == reads);
        rig.host->tick(1);
        CHECK(rig.ws->scopeReads() == reads + 1);
        // It is registered.
        bool found = false;
        for (const auto& e : tee::ui::registeredWorkspaces())
            found = found || std::string(e.id) == "scope";
        CHECK(found);
        return 0;
    }

    // Nothing to read: no telemetry, no Scope, no output. Every plot empty with its words, every number a dash.
    struct Nothing final : tee::TelemetrySource, tee::ui::ScopeSource
    {
        int findValue(std::string_view) const override { return -1; }
        double value(int) const override { return 0.0; }
        float outputPeak() const override { return 0.0f; }
        int find(std::string_view) const override { return -1; }
        int read(int, float*, int) override { return 0; }
        std::uint64_t generation() const override { return 0; }
    };

    VIEW_TEST(testScopeAbsent, "scope.absent")
    {
        tee::ui::DemoRig demo;
        Nothing none;
        WorkspaceContext ctx{ demo.doc, none, none, demo.engine, demo.units, demo.selection, demo.app };
        ScopeWorkspace ws(ctx);
        funkgui::HeadlessHost host(ws, 0, 2.0f);
        host.resize(1100, 700);
        ws.place({ 0.0f, 0.0f, 1100.0f, 700.0f });
        host.tick(90);
        CHECK(ws.facts().degrees == 0 && std::isnan(ws.facts().peakPa) && std::isnan(ws.facts().rpm));
        CHECK(ws.spectrum().empty() && ws.campbell().filled() == 0);
        CHECK(!ws.moving());
        golden("scope.absent", host);
        // The demo without the ports: that plot alone says so.
        tee::ui::DemoRig plain;
        WorkspaceContext pctx{ plain.doc, plain.engine, plain.engine, plain.engine, plain.units, plain.selection, plain.app };
        ScopeWorkspace pws(pctx);
        funkgui::HeadlessHost phost(pws, 0, 1.0f);
        phost.resize(1100, 700);
        pws.place({ 0.0f, 0.0f, 1100.0f, 700.0f });
        phost.tick(90);
        CHECK(pws.facts().degrees == 720 && std::isnan(pws.facts().sparkDeg));
        picture("scope.noports.dpi1", phost);
        return 0;
    }
}
