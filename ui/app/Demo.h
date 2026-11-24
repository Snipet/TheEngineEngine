// ui/app/Demo.h: a made-up engine turning over, for tests and pictures (docs/plan/ui-design.md section 5): every
// workspace has goldens and a contact sheet from the day it exists, before the physics that feeds it has landed.
//
//   demoFields()   a made-up Spec whose sections are the real one's (engine, intake, fuel, valve, boost, exhaust, ecu,
//                  car, env, sound): every field kind, a few of each section's fields, a curve and maps.
//   DemoEngine     a TelemetrySource, a ScopeSource and a DynoSource at once. tick(dt) moves it along a fixed script of
//                  eight seconds (idle, a pull through third gear to the limiter, a lift), so a frame at a given time
//                  is always the same. Its telemetry is sim/spec/telemetry.fm's names; set() adds or overrides one
//                  (a name a package will add later); forget() takes one away (to picture the dash). Its Scope is
//                  cylinder 1's pressure, volume and lifts over 720 degrees and 64 pipe points; its dyno a curve that
//                  follows engine.compression and boost.level of the document it watches, made 0.4 s after an edit.
//   DemoRig        all of a WorkspaceContext over those, in one object (tests, the gallery, the sheet).
//
// No clock, no randomness: everything follows tick(dt).
#pragma once

#include "app/Workspace.h"

#include "kit/bind/Fields.h"
#include "kit/bind/Units.h"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace tee::ui
{
    std::vector<FieldInfo> demoFields();

    class DemoEngine final : public TelemetrySource, public ScopeSource, public DynoSource
    {
    public:
        // `doc`: the document the dyno watches (nullptr: a fixed curve).
        explicit DemoEngine(const FieldDoc* doc = nullptr);

        // Time: the script's position (seconds, wraps at 8) and a step along it (advance). tick() is the DynoSource's
        // (the frame calls it each frame): it moves the dyno and the Scope's generation, not the script, so a picture at
        // setTime(t) stays at t.
        void advance(double dt);
        void tick(float dt) override;
        void setTime(double t);
        double time() const noexcept { return t_; }
        // A telemetry value added or held at `v` (SI), whatever the script says; forget() makes a name absent.
        void set(std::string_view name, double v);
        void forget(std::string_view name);

        // TelemetrySource
        int findValue(std::string_view name) const override;
        double value(int handle) const override;
        float outputPeak() const override;
        int outputTail(float* mono, int max) const override;

        // ScopeSource
        int find(std::string_view name) const override;
        int read(int handle, float* out, int max) override;
        std::uint64_t generation() const override { return generation_; }
        int scopeReads() const noexcept { return reads_; }

        // DynoSource
        const DynoTrace& predicted() const override { return predicted_; }
        const DynoTrace& before() const override { return before_; }
        bool busy() const override { return waiting_ >= 0.0; }
        double progress() const override;
        std::uint64_t revision() const override { return dynoRevision_; }
        int runCount() const override { return static_cast<int>(runs_.size()); }
        const DynoTrace& run(int i) const override { return runs_[static_cast<std::size_t>(i)]; }
        bool startRun(const DynoRequest& r) override;
        bool running() const override { return runLeft_ > 0.0; }
        void stopRun() override { runLeft_ = 0.0; }

        static constexpr double kPeriod = 8.0;     // the script's length, seconds

    private:
        void update();
        DynoTrace curveFor(double compression, double boost, const std::string& name) const;
        double fieldOr(std::string_view path, double fallback) const;

        const FieldDoc* doc_;
        double t_ = 0.0, rpm_ = 850.0, throttle_ = 0.0, phase_ = 0.0;
        std::vector<std::string> names_;
        std::vector<double> values_;
        std::map<std::string, double, std::less<>> held_;
        std::set<std::string, std::less<>> forgotten_;
        std::uint64_t generation_ = 0;
        int reads_ = 0;
        // the dyno
        DynoTrace predicted_, before_;
        std::vector<DynoTrace> runs_;
        DynoRequest runPlan_;
        std::uint64_t seenRevision_ = 0, dynoRevision_ = 1;
        double quiet_ = 0.0, waiting_ = -1.0, runLeft_ = 0.0;
    };

    // A whole context over the demo: a MemoryDoc of demoFields(), the units, the engine, a selection, services that
    // remember the controls a Panel asks for.
    struct DemoRig
    {
        MemoryDoc doc{ demoFields() };
        Units units;
        DemoEngine engine{ &doc };
        Selection selection;
        PanelControls asked{};
        int askedCount = 0;
        ControlsFrame sent{};
        AppServices app{ [this](const PanelControls& c) { asked = c; ++askedCount; }, [this] { return sent; },
                         [] { return std::string("demo engine   no audio"); } };
        WorkspaceContext ctx{ doc, engine, engine, engine, units, selection, app };

        // The script at `t` seconds, its dyno curve made, and the pedals as the application would send them.
        explicit DemoRig(double t = 4.6);
        void at(double t);
    };
}
