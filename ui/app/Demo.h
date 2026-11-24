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
#include <functional>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace tee::ui
{
    class DemoEngine;

    // ---- a workspace's own demo data ---------------------------------------------------------------------------
    // A package adds to the demo what its stage reads and the demo lacks (fields, telemetry, Scope arrays), from its
    // own directory: a function `void <fn>(DemoExtras&)` in namespace tee::ui, named on a second line of its
    // register.h:
    //     TEE_UI_DEMO(<id>, <fn>)
    // It is called twice for each demo: once with `fields` (append FieldInfo, before the document is made) and once
    // with `engine` (engine->addTelemetry / addScope / set). Every registered workspace's hook applies to every
    // DemoRig made with hooks on (the default): the frame's contact sheet shows each stage with its data.
    struct DemoExtras
    {
        std::vector<FieldInfo>* fields = nullptr;   // the first call's
        DemoEngine* engine = nullptr;               // the second call's
    };
    using DemoHook = void (*)(DemoExtras&);
    // The hooks this build found (generated from ui/views/*/register.h, as registeredWorkspaces()).
    const std::vector<DemoHook>& registeredDemos();
    const std::vector<const char*>& registeredDemoIds();   // each hook's workspace id, in the same order

    // Which hooks a demo applies: none, all, or those of the named workspaces (a stage's test: its own, so another
    // package's data never moves its goldens).
    struct DemoHooks
    {
        bool all = true;
        std::vector<std::string> only;   // when not all: these workspaces' hooks
        static DemoHooks none() { return { false, {} }; }
        static DemoHooks of(std::vector<std::string> ids) { return { false, std::move(ids) }; }
        DemoHooks(bool every = true) : all(every) {}   // true: all; false: none (as before)
        DemoHooks(bool every, std::vector<std::string> ids) : all(every), only(std::move(ids)) {}
        bool wants(const char* id) const;
    };

    // `hooks`: the registered workspaces' own fields (DemoHook) to add.
    std::vector<FieldInfo> demoFields(const DemoHooks& hooks = true);
    // Spec fields defined twice by the base demo and the registered hooks: a line each, naming both ("" : none).
    std::string demoFieldConflicts();

    class DemoEngine final : public TelemetrySource, public ScopeSource, public DynoSource
    {
    public:
        // `doc`: the document the dyno watches (nullptr: a fixed curve). `hooks`: with the registered workspaces' own
        // telemetry and Scope arrays (DemoHook).
        explicit DemoEngine(const FieldDoc* doc = nullptr, const DemoHooks& hooks = true);

        // Names defined twice (the base demo and a hook, or two hooks): one line each, naming both ("" : none). A
        // hook's names must be its own (ui.demo.hooks fails on any).
        const std::string& conflicts() const noexcept { return conflicts_; }
        // The base demo's names (a contract, frozen 2026-11-24: a new name comes through its workspace's hook).
        static std::vector<std::string> baseTelemetry();
        static std::vector<std::string> baseScope();
        // A package's own (DemoHook): a telemetry value that follows the script (read when asked), a Scope array (the
        // function writes up to max entries into out and answers how many).
        void addTelemetry(std::string name, std::function<double(const DemoEngine&)> value);
        void addScope(std::string name, std::function<int(const DemoEngine&, float* out, int max)> read);
        // The script's state, for those functions: rpm, the throttle (0..1), the crank's phase over the cycle (0..1).
        double rpm() const noexcept { return rpm_; }
        double throttle() const noexcept { return throttle_; }
        double phase() const noexcept { return phase_; }
        // A field of the document it watches (SI), or `fallback`.
        double field(std::string_view path, double fallback) const { return fieldOr(path, fallback); }

        // Time: the script's position (seconds, wraps at 8) and a step along it. tick(dt) (the DynoSource's, which the
        // frame calls each frame) steps the script, the Scope's generation and the dyno; while engine.state is held at
        // 0 the script stands still. advance(dt) steps the script alone.
        void advance(double dt);
        void tick(float dt) override;
        // Held, tick() leaves the script where it is (a picture of one moment; the dyno still answers edits).
        void hold(bool held) noexcept { frozen_ = held; }
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
        std::uint64_t requests() const override { return requests_; }
        const DynoTrace* runSoFar() const override { return runLeft_ > 0.0 ? &partial_ : nullptr; }

        static constexpr double kPeriod = 8.0;     // the script's length, seconds

    private:
        void update();
        static double interpolate(const DynoTrace& c, double speed);   // a curve's torque at a speed
        DynoTrace curveFor(double compression, double boost, const std::string& name) const;
        double fieldOr(std::string_view path, double fallback) const;

        const FieldDoc* doc_;
        std::string hookId_ = "the base demo", conflicts_;
        std::map<std::string, std::string, std::less<>> owner_;   // a name's definer
        std::vector<std::pair<std::string, std::function<double(const DemoEngine&)>>> extraTel_;
        std::vector<std::pair<std::string, std::function<int(const DemoEngine&, float*, int)>>> extraScope_;
        double t_ = 0.0, rpm_ = 850.0, throttle_ = 0.0, phase_ = 0.0;
        bool frozen_ = false;
        std::vector<std::string> names_;
        std::vector<double> values_;
        std::map<std::string, double, std::less<>> held_;
        std::set<std::string, std::less<>> forgotten_;
        std::uint64_t generation_ = 0;
        int reads_ = 0;
        // the dyno
        DynoTrace predicted_, before_, partial_;
        std::vector<DynoTrace> runs_;
        DynoRequest runPlan_;
        std::uint64_t seenRevision_ = 0, dynoRevision_ = 1;
        double quiet_ = 0.0, waiting_ = -1.0, runLeft_ = 0.0;
        std::uint64_t requests_ = 0;
    };

    // A whole context over the demo: a MemoryDoc of demoFields(), the units, the engine, a selection, services that
    // remember the controls a Panel asks for.
    struct DemoRig
    {
        MemoryDoc doc;
        Units units;
        DemoEngine engine;
        Selection selection;
        PanelControls asked{};
        int askedCount = 0;
        ControlsFrame sent{};
        AppServices app{ [this](const PanelControls& c) { asked = c; ++askedCount; }, [this] { return sent; },
                         [] { return std::string("demo engine   no audio"); } };
        WorkspaceContext ctx{ doc, engine, engine, engine, units, selection, app };

        // The script at `t` seconds, its dyno curve made, and the pedals as the application would send them.
        // `hooks`: with the registered workspaces' own demo data (off: the demo alone, for pictures that must not move
        // when another package adds to it).
        explicit DemoRig(double t = 4.6, const DemoHooks& hooks = true);
        void at(double t);
    };
}
