// ui/app/Workspace.h: what a workspace is, and what it is given (docs/plan/ui-design.md section 5).
//
// The window is a top bar, a rail of workspaces, a stage and an inspector (with the dyno ribbon under it), and the
// cockpit (AppFrame.h). A workspace owns its stage: a picture of its part of the machine, drawn from the build and
// moved by telemetry, where clicking a part selects it. The frame owns the inspector: it shows the generated pages
// (J2's) for what is selected, or the workspace's own `.gui` page.
//
// A Workspace is a Board (kit/widgets/Board.h) the size of the window whose items and drawing keep to its stage rect:
// make widgets on it (make<Gauge>(...), make<Plot>(...)) and place them in layout(); draw anything else with the Canvas
// in drawStage() (ui/app/Look.h has the colours and helpers); take presses on drawn parts by overriding the Panel's
// pointer calls (call Board's for what is not yours). As every Panel: time only from tick(dt) (override step()), draw
// changes nothing, no clock and no environment read.
//
// A workspace reads fields by path (ctx.doc) and telemetry by name (Live, below), so it compiles and runs before the
// physics that feeds it has landed: a name that is not there yet reads as NaN and draws a dash (look::num).
//
// Registering one: a directory ui/views/<id>/ with its sources and a file `register.h` of one line,
//     TEE_UI_WORKSPACE(<id>, <factory>)
// where <factory> is `std::unique_ptr<Workspace> <factory>(WorkspaceContext&)` in namespace tee::ui. The build finds
// it (ui/CMakeLists.txt) and the frame puts it on the rail in place of that id's placeholder. docs/handoffs/u1-interface.md
// has an example (ui/views/_example/) and the way to test and picture one.
#pragma once

#include "kit/bind/Fields.h"
#include "kit/bind/Units.h"
#include "kit/widgets/Board.h"

#include <tee/app/Sources.h>

#include <funkgui/core/Geometry.h>

#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace tee::ui
{
    // ---- what is selected -------------------------------------------------------------------------------------------
    // A path prefix into the Spec: "" (nothing), a section ("exhaust"), or deeper ("exhaust.header", "valve.intake").
    // The stage sets it when a part is clicked; the inspector shows that prefix's fields. hovered() is the part under
    // the pointer (for a stage to light), set by the stage that owns it.
    class Selection
    {
    public:
        const std::string& path() const noexcept { return path_; }
        bool empty() const noexcept { return path_.empty(); }
        void select(std::string prefix)
        {
            if (prefix != path_)
            {
                path_ = std::move(prefix);
                ++revision_;
            }
        }
        void clear() { select({}); }
        // Whether `prefix` is what is selected, or inside it ("exhaust.header" is inside "exhaust").
        bool is(std::string_view prefix) const noexcept
        {
            return !path_.empty() && path_.size() >= prefix.size() && path_.compare(0, prefix.size(), prefix) == 0
                && (path_.size() == prefix.size() || path_[prefix.size()] == '.' || path_[prefix.size()] == '[');
        }
        const std::string& hovered() const noexcept { return hovered_; }
        void hover(std::string prefix)
        {
            if (prefix != hovered_)
            {
                hovered_ = std::move(prefix);
                ++revision_;
            }
        }
        // Moves at every change.
        std::uint32_t revision() const noexcept { return revision_; }

    private:
        std::string path_, hovered_;
        std::uint32_t revision_ = 1;
    };

    // ---- the Scope --------------------------------------------------------------------------------------------------
    // What is too big to copy every block (sim/spec/scope.fm): cylinder 1's pressure, volume and both lifts for each
    // degree of the last whole cycle, pressures along the pipes, and what packages add. The host copies it only while
    // someone reads it, so a workspace reads it only while it is shown. UI thread.
    class ScopeSource
    {
    public:
        virtual ~ScopeSource() = default;
        // A handle for an array (or a scalar) by its name in scope.fm: "pressure", "volume", "liftIntake",
        // "liftExhaust", "pipePressure", "degrees", "cycles". -1: none (not in this program yet).
        virtual int find(std::string_view name) const = 0;
        // The newest copy of it into out[0..max), in SI: how many entries were written. For the per-degree arrays
        // that is the cycle's length ("degrees": 720 or 1080), for pipePressure "pipePoints"; 0 while there is none
        // yet. Reading asks the host to keep copying for about half a second.
        virtual int read(int handle, float* out, int max) = 0;
        // Moves when a newer copy is in (a reader that keeps what it drew need not read again until it moves).
        virtual std::uint64_t generation() const = 0;
    };

    // ---- the dyno ---------------------------------------------------------------------------------------------------
    // One curve: torque and power against the crank's speed (SI: rad/s, N m, W), and more fields at each point.
    struct DynoTrace
    {
        std::string name;                        // "predicted", "before", a run's name
        std::vector<double> speed, torque, power;
        std::vector<std::string> fields;         // more telemetry at each point, by name
        std::vector<std::vector<double>> values; // values[f][point]
        bool empty() const noexcept { return speed.empty(); }
        // The peak of torque or power: its value and the speed at it (NaN, NaN when empty).
        struct Peak
        {
            double value = std::numeric_limits<double>::quiet_NaN(), speed = std::numeric_limits<double>::quiet_NaN();
        };
        Peak peakTorque() const noexcept { return peakOf(torque); }
        Peak peakPower() const noexcept { return peakOf(power); }

    private:
        Peak peakOf(const std::vector<double>& v) const noexcept
        {
            Peak p;
            for (std::size_t i = 0; i < v.size() && i < speed.size(); ++i)
                if (std::isfinite(v[i]) && !(v[i] <= p.value))
                    p = { v[i], speed[i] };
            return p;
        }
    };

    // A run a person starts on the DYNO workspace (card C's DynoPlan, the part a person sets).
    struct DynoRequest
    {
        bool chassis = false;     // the car's own speed on rollers rather than the crank held
        double from = 150.0;      // rad/s
        double to = 650.0;        // rad/s
        double rate = 60.0;       // rad/s^2 (a sweep)
        double pedal = 1.0;       // 0..1
        std::string name;         // the run's name ("" : numbered)
    };

    // The predicted curve of the build as it stands (card C's dyno runner, in the background, a moment after each
    // edit), the one before it (the ghost), and the runs a person made. tick() is the frame's, once a frame, on the UI
    // thread: it notices edits (the document's revision), asks for a new curve after a short quiet, and takes finished
    // ones. Everything else only reads.
    class DynoSource
    {
    public:
        virtual ~DynoSource() = default;
        virtual void tick(float dt) = 0;
        virtual const DynoTrace& predicted() const = 0;   // empty until the first is in
        virtual const DynoTrace& before() const = 0;      // the predicted curve before the newest; empty: none
        virtual bool busy() const = 0;                    // a curve is being made
        virtual double progress() const = 0;              // 0..1 of the one being made
        virtual std::string error() const { return {}; }  // why there is no curve ("" : no error)
        virtual std::uint64_t revision() const = 0;       // moves when any curve or the state above changes
        // Runs (the DYNO workspace's): listed, started, stopped. A source with no runner has none and starts none.
        virtual int runCount() const { return 0; }
        virtual const DynoTrace& run(int i) const;
        virtual bool startRun(const DynoRequest&) { return false; }
        virtual bool running() const { return false; }
        virtual void stopRun() {}
    };

    // ---- telemetry by name ------------------------------------------------------------------------------------------
    // A TelemetrySource read by name, its handles kept: get("engine.speed") is the value, NaN when the source has no
    // such name (yet). Names: docs/plan/02-contracts.md section 5 and sim/spec/telemetry.fm ("engine.speed",
    // "car.gear", "boost.pressure", "exhaust.portTemperature", ...), in SI.
    class Live
    {
    public:
        explicit Live(TelemetrySource& source) : source_(&source) {}
        double get(std::string_view name) const
        {
            // A name found is kept; one not found is asked again next time (a program loaded later may have it).
            auto it = handles_.find(name);
            if (it == handles_.end())
            {
                const int h = source_->findValue(name);
                if (h < 0)
                    return std::numeric_limits<double>::quiet_NaN();
                it = handles_.emplace(std::string(name), h).first;
            }
            return source_->value(it->second);
        }
        // Forget the handles (the source now plays another program).
        void reset() { handles_.clear(); }
        bool has(std::string_view name) const { return std::isfinite(get(name)); }
        TelemetrySource& source() const noexcept { return *source_; }

    private:
        TelemetrySource* source_;
        mutable std::map<std::string, int, std::less<>> handles_;
    };

    // ---- the context ------------------------------------------------------------------------------------------------
    // What every workspace is given; owned by the frame (or a test's rig), alive as long as the workspaces are.
    struct WorkspaceContext
    {
        FieldDoc& doc;                 // the build: fields by path (doc.find, get, set inside begin/endGesture), undo.
                                       // In the application a BuildDoc over card C's tee::Build (J2's).
        TelemetrySource& telemetry;    // the newest block's values (card B's interface); read through `live`
        ScopeSource& scope;
        DynoSource& dyno;
        Units& units;                  // the user's units: units.format(Quantity::pressure, pa)
        Selection& selection;
        const AppServices& app;        // the start button, the pedals as sent, the status line
        Live live{ telemetry };
    };

    // ---- the workspace ----------------------------------------------------------------------------------------------
    class Workspace : public Board
    {
    public:
        explicit Workspace(WorkspaceContext& context, int width = 1280, int height = 800);

        virtual const char* id() const = 0;        // "drive": its directory, and the rail's key
        virtual const char* title() const = 0;     // "DRIVE": on the rail
        // The Spec sections it edits, in order: its inspector's pages when nothing is selected (and the frame's check
        // that every field is on some page).
        virtual std::vector<std::string> sections() const = 0;
        // The .gui text of the inspector for what is selected (ctx.selection), which may use `auto "section"`, J2's
        // widgets and dotted field paths. "" (the default): the frame's generated pages for the selection's fields,
        // or for sections() when nothing is selected or the selection has no fields.
        virtual std::string inspectorPage() const { return {}; }
        // Whether the dyno ribbon is under the inspector (not on DYNO and DRIVE).
        virtual bool ribbon() const { return true; }
        // Wants full rate: something on the stage moves (live values, an ease). Default: whatever the Board says.
        virtual bool moving() const { return wantsFullRate(); }
        // A mix of what drawStage() reads that can change with no input (a live value, a field): the frame draws again
        // when it moves (Panel::needsDraw). Items' own drawKey()s are mixed by the frame; this is for drawStage's.
        virtual std::uint64_t drawKey() const { return 0; }

        // The frame gives the stage (window coordinates); the workspace lays out for it (layout()).
        void place(const funkgui::Rect& stage);
        const funkgui::Rect& stage() const noexcept { return stage_; }
        WorkspaceContext& context() const noexcept { return ctx_; }

        // funkgui::Panel
        void tick(float dt) override;

    protected:
        // Place the items for the stage.
        virtual void layout(const funkgui::Rect& stage) = 0;
        // Draw what is not an item, under the items (clipped to the stage by the frame).
        virtual void drawStage(funkgui::Canvas& /*c*/, const funkgui::Theme& /*th*/) const {}
        // Its own time (eases of drawn parts), before the items are ticked.
        virtual void step(float /*dt*/) {}

        // Board
        void arrange() override;
        void drawGround(funkgui::Canvas& c, const funkgui::Theme& th) const override;
        void drawOver(funkgui::Canvas& c, const funkgui::Theme& th) const override;

        WorkspaceContext& ctx_;
        funkgui::Rect stage_{};
    };

    // A workspace's factory (ui/views/<id>/register.h names one).
    using WorkspaceFactory = std::unique_ptr<Workspace> (*)(WorkspaceContext&);
    struct WorkspaceEntry
    {
        const char* id;
        WorkspaceFactory make;
    };
    // The workspaces under ui/views/ that this build found (generated by ui/CMakeLists.txt from their register.h).
    const std::vector<WorkspaceEntry>& registeredWorkspaces();
}
