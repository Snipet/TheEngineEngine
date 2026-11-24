// ui/app/AppFrame.cpp: see AppFrame.h.
#include "app/AppFrame.h"

#include "app/Look.h"
#include "app/Placeholders.h"

#include <funkgui/canvas/Canvas.h>
#include <funkgui/core/Theme.h>

#include <algorithm>
#include <cmath>
#include <set>

namespace tee::ui
{
    namespace
    {
        constexpr double kPi = 3.14159265358979323846;
        constexpr double kToRpm = 30.0 / kPi;
        constexpr int kMaxTabs = 8;
        const std::string kNone;

        std::string firstLine(const std::string& s) { return s.substr(0, s.find('\n')); }

        // A quantity's unit as a stepped parameter: its units' index (the units page's steppers).
        class UnitPort final : public funkgui::ParamPort
        {
        public:
            UnitPort(Units& units, Quantity q) : units_(units), q_(q), id_(std::string("units.") + nameOf(q)) {}
            float value01() const override { return static_cast<float>(units_.chosen(q_)) / static_cast<float>(steps()); }
            float default01() const override { return 0.0f; }
            int numSteps() const override { return steps(); }
            void beginGesture() override {}
            void setValue01(float v) override
            {
                units_.choose(q_, static_cast<int>(std::lround(std::clamp(v, 0.0f, 1.0f) * static_cast<float>(steps()))));
            }
            void endGesture() override {}
            const char* id() const override { return id_.c_str(); }
            void* native() const override { return nullptr; }
            std::string text(float v) const
            {
                const auto i = static_cast<std::size_t>(std::lround(v * static_cast<float>(steps())));
                return unitsOf(q_)[std::min(i, unitsOf(q_).size() - 1)].name;
            }

        private:
            int steps() const { return std::max(1, static_cast<int>(unitsOf(q_).size()) - 1); }
            Units& units_;
            Quantity q_;
            std::string id_;
        };

        // Every field whose path is `prefix` or inside it, not hidden, in the document's order.
        std::vector<int> fieldsUnder(const FieldDoc& doc, const std::string& prefix)
        {
            std::vector<int> out;
            for (int i = 0; i < doc.count(); ++i)
            {
                const FieldInfo& f = doc.info(i);
                if (f.hidden || f.path.size() < prefix.size() || f.path.compare(0, prefix.size(), prefix) != 0)
                    continue;
                if (f.path.size() == prefix.size() || f.path[prefix.size()] == '.' || f.path[prefix.size()] == '[')
                    out.push_back(i);
            }
            return out;
        }

        const char* const kModes[3] = { "BENCH", "DYNO", "DRIVE" };

        funkgui::Col pedalThrottle(const look::Palette& p) { return p.value; }
        funkgui::Col pedalBrake(const look::Palette& p) { return p.hotter; }
        funkgui::Col pedalClutch(const look::Palette& p) { return p.text; }
    }

    std::vector<WorkspaceSlot> defaultSlots()
    {
        return {
            { "garage", "GARAGE", "GAR", {},
              "Builds as cards: each with its name, a sentence, peak torque and power, a small power curve. Two builds side by "
              "side: their curves overlaid and the fields that differ. The build's notes; load, save, duplicate; the engine, "
              "the car and the tune loadable apart.",
              "package U5", false },
            { "engine", "ENGINE", "ENG", { "engine", "fuel" },
              "The block from above, drawn from the layout: banks, cylinders numbered, the crank's pins; each cylinder lights "
              "as it fires, in its order, and the firing intervals as a ring. Beside it one cylinder in section: the piston "
              "on its rod and crank, both valves lifting by the real cam, the gas tinted by pressure. A pressure-volume loop.",
              "package U2", true },
            { "valves", "VALVES", "VLV", { "valve" },
              "The cam editor: both lifts against crank angle, the overlap shaded, handles to drag for centreline, duration "
              "and lift; the timing ring with each valve's open period round the cycle; the phasers' ranges as bands; the "
              "flow bench's curve.",
              "package U2", true },
            { "air", "AIR", "AIR", { "intake", "boost" },
              "The charge path as a row of parts, left to right, each with its live pressure and temperature: filter, "
              "compressor, intercooler, throttle, plenum, runners. The compressor's map with the operating point's trail, the "
              "surge line and the islands; the shaft's speed; the gate and the blow-off valve as they open.",
              "package U3", true },
            { "exhaust", "EXHAUST", "EXH", { "exhaust" },
              "The system as built: primaries into collectors into pipes, each element a shape; walls tinted by temperature; "
              "pulses travelling, live from the Scope; each radiating end marked. Click an element to select it.",
              "package U3", true },
            { "tune", "TUNE", "TUNE", { "ecu" },
              "The selected map as a heat map and a table, the live cell lit, the knock and lambda traces under it; slices "
              "through the map as plots.",
              "package U4", true },
            { "car", "CAR", "CAR", { "car" },
              "The car from the side: wheelbase, centre of mass, the load on each axle, live; the driveline for the layout "
              "chosen; the tyre's grip curve with each wheel's live point; the gearing chart.",
              "package U4", true },
            { "dyno", "DYNO", "DYNO", { "env" },
              "The big plot: torque and power (and anything else from telemetry) against speed; runs listed, overlaid, "
              "named, exported; an engine or chassis run started and watched as it draws.",
              "package U4", false },
            { "drive", "DRIVE", "DRV", { "env" },
              "Round tachometer and speedometer, boost, lambda, temperatures; the car on its road; the strip's tree and the "
              "time slip; the wheels' slip as bars.",
              "package U1", false },
            { "scope", "SCOPE", "SCP", { "sound" },
              "Cylinder pressure against crank angle and against volume; the ports' pressures; the output's wave; a "
              "spectrum, and a spectrum against engine speed where the firing orders stand as lines.",
              "package U5", true },
        };
    }

    // ---- making it --------------------------------------------------------------------------------------------------
    AppFrame::AppFrame(FieldDoc& doc, Units& units, TelemetrySource& telemetry, FrameServices services, DynoSource* dyno,
                       ScopeSource* scope, std::vector<WorkspaceSlot> slots, int width, int height)
        : Board(width, height, kMinW, kMinH), doc_(doc), units_(units), telemetry_(telemetry), services_(std::move(services)),
          slots_(std::move(slots))
    {
        if (dyno == nullptr)
            ownDyno_ = std::make_unique<NoDyno>();
        if (scope == nullptr)
            ownScope_ = std::make_unique<NoScope>();
        dyno_ = dyno != nullptr ? dyno : ownDyno_.get();
        scope_ = scope != nullptr ? scope : ownScope_.get();
        ctx_ = std::make_unique<WorkspaceContext>(WorkspaceContext{ doc_, telemetry_, *scope_, *dyno_, units_, selection_,
                                                                    services_.app, &services_ });
        panes_ = std::make_unique<Panes>(this->width(), this->height());

        // The top bar.
        name_ = &make<Label>([this] {
            std::string n = services_.buildName ? services_.buildName() : std::string("Untitled");
            return services_.unsaved && services_.unsaved() ? n + " *" : n;
        }, funkgui::TextStyle{ 15.0f, 0.6f, 0.06f, false }, Label::Ink::value);
        sentence_ = &make<Label>([this] { return engineSentence(); }, kWidgetText, Label::Ink::text);
        status_ = &make<Label>([this] { return services_.app.status ? firstLine(services_.app.status()) : std::string(); },
                               kWidgetSmall, Label::Ink::label, funkgui::Align::right);
        load_ = &make<Bar>("Load", [this] { return services_.simLoad ? std::clamp(services_.simLoad(), 0.0f, 1.0f) : 0.0f; },
                           [this] {
                               return services_.simLoad ? std::to_string(static_cast<int>(std::lround(services_.simLoad() * 100.0f))) + " %"
                                                        : std::string("-");
                           });
        level_ = &make<Bar>("Output", [this] {
            return std::clamp(services_.outputLevel ? services_.outputLevel() : telemetry_.outputPeak(), 0.0f, 1.0f);
        });
        unitsButton_ = &make<Button>("Units", [this] { showUnits(!unitsPage_); });
        unitsButton_->setLit([this] { return unitsPage_; });
        undo_ = &make<Button>("Undo", [this] { if (services_.undo) services_.undo(); });
        redo_ = &make<Button>("Redo", [this] { if (services_.redo) services_.redo(); });

        makeWorkspaces();
        // The rail.
        for (int i = 0; i < workspaceCount(); ++i)
        {
            Button& b = make<Button>(slots_[static_cast<std::size_t>(i)].title, [this, i] { show(i); });
            b.setLit([this, i] { return shown_ == i; });
            rail_.push_back(&b);
        }
        // The inspector's header, tabs and units page; the ribbon.
        clear_ = &make<Button>("All", [this] { selection_.clear(); });
        for (int p = 0; p < kMaxTabs; ++p)
        {
            Button& b = make<Button>("", [this, p] { showInspectorPage(p); });
            b.setLit([this, p] { return page_ == p && !unitsPage_; });
            tabs_.push_back(&b);
        }
        for (int q = 1; q < kQuantities; ++q)
        {
            const auto qq = static_cast<Quantity>(q);
            if (unitsOf(qq).size() < 2)
                continue;
            auto port = std::make_unique<UnitPort>(units_, qq);
            UnitPort& u = *port;
            unitPorts_.push_back(std::move(port));
            unitSteppers_.push_back(&make<Stepper>(u, std::string(nameOf(qq)), [&u](float v) { return u.text(v); }));
        }
        ribbon_ = &make<DynoRibbon>(*dyno_, units_);
        fold_ = &make<Button>("Fold", [this] { foldRibbon(!folded_); });
        makeCockpit();

        unitsSeen_ = units_.revision();
        selectionSeen_ = selection_.revision();
        show(std::max(0, find("engine")));
    }

    AppFrame::~AppFrame()
    {
        // The pages and workspaces go before the context and the sources they read.
        pages_.clear();
        workspaces_.clear();
    }

    void AppFrame::makeWorkspaces()
    {
        const std::vector<WorkspaceEntry>& registered = registeredWorkspaces();
        for (const WorkspaceSlot& s : slots_)
        {
            const auto it = std::find_if(registered.begin(), registered.end(), [&](const WorkspaceEntry& e) { return s.id == e.id; });
            std::unique_ptr<Workspace> ws;
            if (it != registered.end())
                ws = it->make(*ctx_);
            placeholders_.push_back(ws == nullptr);
            if (ws == nullptr)
                ws = s.id == "garage" ? makePlaceholderGarage(*ctx_, s) : makePlaceholder(*ctx_, s);
            ws->resize(width(), height());
            workspaces_.push_back(std::move(ws));
        }
        // A workspace registered with an id no slot has goes at the end of the rail.
        for (const WorkspaceEntry& e : registered)
            if (std::none_of(slots_.begin(), slots_.end(), [&](const WorkspaceSlot& s) { return s.id == e.id; }))
            {
                std::unique_ptr<Workspace> ws = e.make(*ctx_);
                if (ws == nullptr)
                    continue;
                slots_.push_back({ e.id, ws->title(), ws->title(), ws->sections(), {}, {}, ws->ribbon() });
                placeholders_.push_back(false);
                ws->resize(width(), height());
                workspaces_.push_back(std::move(ws));
            }
        // The sections no workspace names (in its sections or by `auto` in its page) go to ENGINE's inspector.
        std::set<std::string> claimed;
        for (const auto& ws : workspaces_)
        {
            for (const std::string& s : ws->sections())
                claimed.insert(s);
            const std::string gui = ws->inspectorPage();
            for (std::size_t at = gui.find("auto"); at != std::string::npos; at = gui.find("auto", at + 4))
            {
                const std::size_t q0 = gui.find('"', at), q1 = q0 == std::string::npos ? q0 : gui.find('"', q0 + 1);
                if (q1 != std::string::npos)
                    claimed.insert(gui.substr(q0 + 1, q1 - q0 - 1));
            }
        }
        for (const std::string& sec : tee::ui::sectionsOf(doc_))
            if (!claimed.count(sec) && !sectionFields(doc_, sec).empty())
                rest_.push_back(sec);
    }

    void AppFrame::makeCockpit()
    {
        start_ = &make<Button>("Start");
        start_->setOnHold([this](bool held) {
            controls_.startHeld = held;
            report();
        });
        start_->setLit([this] {
            const double s = ctx_->live.get(telemetry::kState);
            return s >= 0.5 && s < 2.5;
        });
        Button& mode = make<Button>("Bench", [this] {
            const int m = ((services_.mode ? services_.mode() : localMode_) + 1) % 3;
            localMode_ = m;
            if (services_.setMode)
                services_.setMode(m);
        });
        modes_.push_back(&mode);
        rpm_ = &make<RpmBar>([this] { return ctx_->live.get(telemetry::kSpeed) * kToRpm; },
                             [this] { return ctx_->live.get(telemetry::kRedline) * kToRpm; },
                             [this] { return (static_cast<long>(ctx_->live.get(telemetry::kFlags)) & 1) != 0; });
        const auto add = [this](const char* label, std::function<std::string()> n, std::function<std::string()> u,
                                funkgui::TextStyle st = look::kNumberM) -> Readout& {
            Readout& r = make<Readout>(label, std::move(n), std::move(u), st);
            readouts_.push_back(&r);
            return r;
        };
        add("Gear", [this] {
            const double g = ctx_->live.get(telemetry::kGear);
            if (!std::isfinite(g))
                return std::string("-");
            const long k = std::lround(g);
            return k < 0 ? std::string("R") : k == 0 ? std::string("N") : std::to_string(k);
        }, {}, look::kNumberL);
        add("Speed", [this] {
            const double v = ctx_->live.get(telemetry::kCarSpeed);
            return std::isfinite(v) ? units_.number(Quantity::speed, v, 0) : std::string("-");
        }, [this] { return std::string(units_.unit(Quantity::speed).name); });
        Readout& boost = add("Boost", [this] {
            const double v = ctx_->live.get(telemetry::kBoost);
            return std::isfinite(v) ? units_.number(Quantity::pressure, v) : std::string("-");
        }, [this] { return std::string(units_.unit(Quantity::pressure).name); });
        (void) boost;
        add("Lambda", [this] { return look::num(ctx_->live.get("engine.lambda"), "%.2f"); }, {});
        Readout& coolant = add("Coolant", [this] {
            const double v = ctx_->live.get(telemetry::kCoolant);
            return std::isfinite(v) ? units_.number(Quantity::temperature, v, 0) : std::string("-");
        }, [this] { return std::string(units_.unit(Quantity::temperature).name); });
        coolant.setInk([this](const look::Palette& p) {
            const double v = ctx_->live.get(telemetry::kCoolant);
            return std::isfinite(v) && v > 383.15 ? p.warning : look::kClear;   // over 110 C, and the label says so
        });
        add("Oil", [this] {
            const double v = ctx_->live.get("engine.oilTemperature");
            return std::isfinite(v) ? units_.number(Quantity::temperature, v, 0) : std::string("-");
        }, [this] { return std::string(units_.unit(Quantity::temperature).name); });
        // The pedals: they show the frame the application sent and, held, ask for their own.
        const auto sent = [this](float ControlsFrame::*m) {
            return [this, m] { return services_.app.controls ? services_.app.controls().*m : 0.0f; };
        };
        pedals_.push_back(&make<PedalBar>("Thr", sent(&ControlsFrame::throttle), [this](bool held, float v) {
            controls_.throttle = held ? v : -1.0f;
            report();
        }, &pedalThrottle));
        pedals_.push_back(&make<PedalBar>("Brk", sent(&ControlsFrame::brake), [this](bool held, float v) {
            controls_.brake = held ? v : -1.0f;
            report();
        }, &pedalBrake));
        pedals_.push_back(&make<PedalBar>("Clu", sent(&ControlsFrame::clutch), [this](bool held, float v) {
            if (services_.clutch)
                services_.clutch(held ? v : -1.0f);
        }, &pedalClutch));
    }

    void AppFrame::report()
    {
        if (services_.app.panelControls)
            services_.app.panelControls(controls_);
        dirty_ = true;
    }

    void AppFrame::attached()
    {
        panes_->attach(*host());
        for (auto& ws : workspaces_)
            ws->attach(*host());
        for (Page& p : pages_)
            p.panel->attach(*host());
    }

    int AppFrame::find(std::string_view id) const
    {
        for (int i = 0; i < workspaceCount(); ++i)
            if (slots_[static_cast<std::size_t>(i)].id == id)
                return i;
        return -1;
    }

    bool AppFrame::placeholder(int i) const { return placeholders_[static_cast<std::size_t>(i)]; }

    void AppFrame::show(int i)
    {
        if (i < 0 || i >= workspaceCount())
            return;
        const bool changed = i != shown_ || pages_.empty();
        if (i != shown_)
        {
            workspace(shown_).shownChanged(false);
            selection_.clear();
        }
        shown_ = i;
        if (changed)
        {
            workspace(i).shownChanged(true);
            unitsPage_ = false;
            makeInspector();
        }
        arrange();
    }

    std::vector<std::string> AppFrame::sectionsOf(int ws) const
    {
        std::vector<std::string> out = workspace(ws).sections();
        const int home = find("engine") >= 0 ? find("engine") : 0;
        if (ws == home)
            out.insert(out.end(), rest_.begin(), rest_.end());
        return out;
    }

    // ---- the inspector ----------------------------------------------------------------------------------------------
    void AppFrame::makeInspector()
    {
        for (Page& p : pages_)
            if (p.panel)
            {
                p.panel->pointerExit();
                p.panel->closeGestures();
            }
        // The panes stop pointing at the pages before they go.
        panes_->set({ { &current(), stageRect() } });
        pages_.clear();
        page_ = 0;
        const Workspace& ws = current();
        const std::string& sel = selection_.path();
        inspectorHead_ = sel.empty() ? std::string(ws.title()) : upper(sel);
        for (char& ch : inspectorHead_)
            if (ch == '.')
                ch = ' ';
        if (!unitsPage_)
        {
            if (const std::string gui = ws.inspectorPage(); !gui.empty())
                pages_.push_back({ inspectorHead_, gui, nullptr });
            else
            {
                std::vector<int> fields = sel.empty() ? std::vector<int>{} : fieldsUnder(doc_, sel);
                if (!fields.empty())
                {
                    const auto split = splitPages(doc_, fields, 60);
                    for (std::size_t k = 0; k < split.size(); ++k)
                        pages_.push_back({ split.size() > 1 ? std::to_string(k + 1) : inspectorHead_,
                                           pageGui(doc_, split[k], {}), nullptr });
                }
                else
                {
                    if (!sel.empty())
                        inspectorHead_ = ws.title();
                    // A section's numbers, switches and menus on a page (or several), and each of its tables (a map,
                    // a curve) on a page of its own: the inspector is narrow, and a table wants all of it.
                    for (const std::string& sec : sectionsOf(shown_))
                    {
                        std::vector<int> scalars, tables;
                        for (const int f : sectionFields(doc_, sec))
                            (doc_.info(f).table() ? tables : scalars).push_back(f);
                        const auto split = scalars.empty() ? std::vector<std::vector<int>>{} : splitPages(doc_, scalars, 60);
                        for (std::size_t k = 0; k < split.size(); ++k)
                            pages_.push_back({ upper(sec) + (split.size() > 1 ? " " + std::to_string(k + 1) : std::string()),
                                               pageGui(doc_, split[k], {}), nullptr });
                        for (const int f : tables)
                        {
                            const FieldInfo& info = doc_.info(f);
                            pages_.push_back({ upper(info.label.empty() ? info.path : info.label), pageGui(doc_, { f }, {}), nullptr });
                        }
                    }
                }
            }
            if (pages_.empty())
                unitsPage_ = true;   // nothing of the Spec here (the garage): the units
        }
        if (unitsPage_)
        {
            pages_.clear();
            inspectorHead_ = "UNITS";
        }
        const funkgui::Rect room = inspectorRect();
        for (Page& page : pages_)
        {
            GuiPanel::Options o;
            o.width = width();
            o.height = height();
            o.minWidth = kMinW;
            o.minHeight = kMinH;
            o.name = "app";
            o.insetLeft = static_cast<int>(room.x) + 4;
            o.insetTop = static_cast<int>(room.y) + 2;
            o.insetRight = std::max(0, width() - static_cast<int>(room.right()) + 4);
            o.insetBottom = std::max(0, height() - static_cast<int>(room.bottom()));
            page.panel = std::make_unique<FieldPage>(doc_, units_, telemetry_, Sources{ { std::string(ws.id()) + ".gui", page.gui } },
                                                     std::move(o));
            if (host() != nullptr)
                page.panel->attach(*host());
        }
        selectionSeen_ = selection_.revision();
        pagesFor_ = selection_.path();
        setPanes();
        bump();
        dirty_ = true;
    }

    const std::string& AppFrame::inspectorTitle(int p) const
    {
        return p >= 0 && p < inspectorPageCount() ? pages_[static_cast<std::size_t>(p)].title : kNone;
    }

    FieldPage* AppFrame::inspector() const
    {
        return page_ >= 0 && page_ < inspectorPageCount() ? pages_[static_cast<std::size_t>(page_)].panel.get() : nullptr;
    }

    void AppFrame::showInspectorPage(int p)
    {
        if (p < 0 || p >= inspectorPageCount())
            return;
        if (FieldPage* was = inspector(); was != nullptr && p != page_)
        {
            was->pointerExit();
            was->closeGestures();
        }
        page_ = p;
        arrange();
    }

    void AppFrame::showUnits(bool on)
    {
        if (on == unitsPage_)
            return;
        unitsPage_ = on;
        makeInspector();
        arrange();
    }

    void AppFrame::foldRibbon(bool folded)
    {
        folded_ = folded;
        arrange();
    }

    void AppFrame::setPanes()
    {
        std::vector<Panes::Pane> panes{ { &current(), stageRect() } };
        if (FieldPage* page = inspector())
            panes.push_back({ page, inspectorRect() });
        panes_->set(std::move(panes));
    }

    // ---- where things are -------------------------------------------------------------------------------------------
    float AppFrame::railWidth() const noexcept { return width() >= 1280 ? kRailW : kRailNarrowW; }

    funkgui::Rect AppFrame::railRect(int i) const
    {
        const float w = railWidth();
        return { 8.0f, kTopH + 12.0f + 38.0f * static_cast<float>(i), w - 16.0f, 30.0f };
    }

    funkgui::Rect AppFrame::cockpitRect() const
    {
        return { 0.0f, static_cast<float>(height()) - kCockpitH, static_cast<float>(width()), kCockpitH };
    }

    funkgui::Rect AppFrame::stageRect() const
    {
        const float x = railWidth(), inspector = width() >= 1400 ? kInspectorW + 20.0f : kInspectorW;
        return { x, kTopH, std::max(100.0f, static_cast<float>(width()) - x - inspector),
                 static_cast<float>(height()) - kTopH - kCockpitH };
    }

    funkgui::Rect AppFrame::ribbonRect() const
    {
        if (!current().ribbon())
            return {};
        const funkgui::Rect s = stageRect();
        const float h = folded_ ? kRibbonFoldedH : kRibbonH;
        return { s.right(), s.bottom() - h, static_cast<float>(width()) - s.right(), h };
    }

    funkgui::Rect AppFrame::inspectorRect() const
    {
        const funkgui::Rect s = stageRect();
        const float top = s.y + kHeadH + (inspectorPageCount() > 1 && !unitsPage_ ? 30.0f : 0.0f);
        const funkgui::Rect rib = ribbonRect();
        const float bottom = rib.isEmpty() ? s.bottom() : rib.y;
        return { s.right(), top, static_cast<float>(width()) - s.right(), std::max(40.0f, bottom - top) };
    }

    funkgui::Rect AppFrame::inspectorTabRect(int p) const
    {
        const funkgui::Rect s = stageRect();
        const int n = std::min(inspectorPageCount(), kMaxTabs);
        const float x0 = s.right() + 10.0f, w = (static_cast<float>(width()) - x0 - 10.0f) / static_cast<float>(std::max(1, n));
        return { x0 + w * static_cast<float>(p), s.y + kHeadH, w - 4.0f, 24.0f };
    }

    funkgui::Rect AppFrame::startRect() const
    {
        const funkgui::Rect c = cockpitRect();
        return { 12.0f, c.y + 20.0f, 76.0f, 40.0f };
    }

    funkgui::Rect AppFrame::pedalRect(int pedal) const
    {
        const funkgui::Rect c = cockpitRect();
        return { static_cast<float>(width()) - 12.0f - 34.0f * static_cast<float>(3 - pedal), c.y + 6.0f, 32.0f, kCockpitH - 10.0f };
    }

    void AppFrame::arrange()
    {
        const float w = static_cast<float>(width());
        const bool narrow = railWidth() < kRailW;
        panes_->resize(width(), height());
        // The rail.
        for (std::size_t i = 0; i < rail_.size(); ++i)
        {
            const WorkspaceSlot& s = slots_[i];
            rail_[i]->setTitle(narrow ? s.shortTitle : s.title);
            rail_[i]->setPlace(railRect(static_cast<int>(i)), true);
        }
        // The top bar, from the right: redo, undo, units, the output's level, the load, the status line.
        float x = w - 12.0f;
        const auto right = [&](Item* it, float iw, float y, float h) {
            x -= iw;
            it->setPlace({ x, y, iw, h }, true);
            x -= 8.0f;
        };
        right(redo_, 62.0f, 9.0f, 30.0f);
        right(undo_, 62.0f, 9.0f, 30.0f);
        right(unitsButton_, 68.0f, 9.0f, 30.0f);
        x -= 8.0f;
        right(level_, 110.0f, 6.0f, 36.0f);
        right(load_, 100.0f, 6.0f, 36.0f);
        const float left = railWidth() + 16.0f;
        const float statusW = std::clamp(x - left - 420.0f, 0.0f, 300.0f);
        status_->setPlace({ x - statusW, 18.0f, statusW, 14.0f }, statusW > 60.0f);
        const float room = std::max(80.0f, x - statusW - 12.0f - left);
        name_->setPlace({ left, 6.0f, room, 22.0f }, true);
        sentence_->setPlace({ left, 28.0f, room, 16.0f }, true);

        // The workspace and its stage.
        const funkgui::Rect stage = stageRect();
        for (auto& ws : workspaces_)
            ws->resize(width(), height());
        current().place(stage);
        // The inspector: its header's ALL, the tabs, the page or the units, the ribbon.
        const funkgui::Rect insp{ stage.right(), stage.y, w - stage.right(), stage.h };
        clear_->setPlace({ insp.right() - 58.0f, insp.y + 6.0f, 48.0f, 22.0f }, !selection_.empty() && !unitsPage_);
        const int n = unitsPage_ ? 0 : inspectorPageCount();
        for (int p = 0; p < kMaxTabs; ++p)
        {
            const bool on = n > 1 && p < n;
            if (on)
                tabs_[static_cast<std::size_t>(p)]->setTitle(pages_[static_cast<std::size_t>(p)].title);
            tabs_[static_cast<std::size_t>(p)]->setPlace(inspectorTabRect(p), on);
        }
        placeInspector();
        const funkgui::Rect rib = ribbonRect();
        fold_->setTitle(folded_ ? "Open" : "Fold");
        fold_->setPlace({ rib.right() - 58.0f, rib.y + 4.0f, 48.0f, 22.0f }, !rib.isEmpty());
        ribbon_->setPlace({ rib.x + 12.0f, rib.y + 34.0f, rib.w - 24.0f, rib.h - 40.0f }, !rib.isEmpty() && !folded_);

        // The cockpit, left to right: START, the mode, the rpm bar; from the right: the pedals, the readouts.
        const funkgui::Rect c = cockpitRect();
        start_->setPlace(startRect(), true);
        modes_[0]->setPlace({ 96.0f, c.y + 20.0f, 76.0f, 40.0f }, true);
        for (int p = 0; p < 3; ++p)
            pedals_[static_cast<std::size_t>(p)]->setPlace(pedalRect(p), true);
        static constexpr float kWidths[6] = { 40.0f, 84.0f, 92.0f, 62.0f, 84.0f, 84.0f };   // gear speed boost lambda coolant oil
        float rx = pedalRect(0).x - 16.0f;
        const bool roomy = w >= 1240.0f;
        for (int k = 5; k >= 0; --k)
        {
            const bool on = k != 5 || roomy;
            Readout* r = readouts_[static_cast<std::size_t>(k)];
            if (on)
            {
                rx -= kWidths[k];
                r->setPlace({ rx, c.y + 14.0f, kWidths[k], 50.0f }, true);
                rx -= 14.0f;
            }
            else
                r->setPlace({}, false);
        }
        rpm_->setPlace({ 188.0f, c.y + 12.0f, std::max(160.0f, rx - 188.0f - 8.0f), 54.0f }, true);
        setPanes();
        bump();
        dirty_ = true;
    }

    void AppFrame::placeInspector()
    {
        const funkgui::Rect room = inspectorRect();
        for (Page& p : pages_)
        {
            p.panel->resize(width(), height());
            p.panel->setInsets(static_cast<int>(room.x) + 4, static_cast<int>(room.y) + 2,
                               std::max(0, width() - static_cast<int>(room.right()) + 4),
                               std::max(0, height() - static_cast<int>(room.bottom())));
        }
        // The units page: one column of steppers where it fits, else two.
        const bool one = 10.0f + 32.0f * static_cast<float>(unitSteppers_.size()) + 30.0f <= room.h;
        const float cw = one ? room.w - 24.0f : (room.w - 36.0f) / 2.0f;
        for (std::size_t i = 0; i < unitSteppers_.size(); ++i)
        {
            const float col = one ? 0.0f : static_cast<float>(i % 2), row = one ? static_cast<float>(i) : static_cast<float>(i / 2);
            unitSteppers_[i]->setPlace({ room.x + 12.0f + col * (cw + 12.0f), room.y + 8.0f + row * 32.0f, cw, 28.0f }, unitsPage_);
        }
    }

    // ---- drawing ----------------------------------------------------------------------------------------------------
    void AppFrame::drawGround(funkgui::Canvas& c, const funkgui::Theme& th) const
    {
        c.rrect(0.0f, 0.0f, static_cast<float>(width()), static_cast<float>(height()), 0.0f, th.ground);
    }

    void AppFrame::drawOver(funkgui::Canvas& c, const funkgui::Theme& th) const
    {
        const look::Palette p = look::Palette::of(th);
        const float w = static_cast<float>(width()), h = static_cast<float>(height());
        const funkgui::Rect stage = stageRect(), cock = cockpitRect();
        const float rw = railWidth();
        const funkgui::Col bar = p.panel;
        // The bars: top, rail, cockpit; the inspector's column a little lighter than the stage.
        c.rrect(0.0f, 0.0f, w, kTopH, 0.0f, bar);
        c.rrect(0.0f, kTopH, rw, h - kTopH - kCockpitH, 0.0f, bar);
        c.rrect(0.0f, cock.y, w, kCockpitH, 0.0f, bar);
        c.rrect(stage.right(), stage.y, w - stage.right(), kHeadH, 0.0f, bar);
        c.hairlineH(0.0f, c.snapY(kTopH), w, p.edge);
        c.hairlineH(0.0f, c.snapY(cock.y), w, p.edge);
        c.hairlineV(c.snapX(rw), kTopH, h - kTopH - kCockpitH, p.edge);
        c.hairlineV(c.snapX(stage.right()), kTopH, stage.h, p.edge);
        c.hairlineH(stage.right(), c.snapY(stage.y + kHeadH), w - stage.right(), p.edge);
        // The wordmark over the rail.
        if (rw >= kRailW)
        {
            c.text("THE ENGINE", rw * 0.5f, 9.0f, kWidgetLabel, p.label, funkgui::Align::centre);
            c.text("ENGINE", rw * 0.5f, 22.0f, funkgui::type::kWordmark, p.value, funkgui::Align::centre);
        }
        else
            c.text("TEE", rw * 0.5f, 16.0f, funkgui::type::kWordmark, p.value, funkgui::Align::centre);
        c.rrect(rw * 0.5f - 12.0f, kTopH - 6.0f, 24.0f, 2.0f, 1.0f, p.selected);
        // The inspector's header: what is selected (or the workspace), in the accent while something is.
        const std::string head = fitText(c, inspectorHead_, look::kTitle, w - stage.right() - 90.0f);
        c.text(head.c_str(), stage.right() + 14.0f, stage.y + 11.0f, look::kTitle, selection_.empty() || unitsPage_ ? p.text : p.selected);
        if (unitsPage_ && !unitSteppers_.empty())
            c.text("Units for every page; saved with the settings.", stage.right() + 14.0f,
                   unitSteppers_.back()->bounds().bottom() + 12.0f, look::kBody, p.label);
        // The ribbon's title row.
        if (const funkgui::Rect rib = ribbonRect(); !rib.isEmpty())
        {
            c.rrect(rib.x, rib.y, rib.w, rib.h, 0.0f, bar);
            c.hairlineH(rib.x, c.snapY(rib.y), rib.w, p.edge);
            c.text("DYNO", rib.x + 14.0f, rib.y + 10.0f, look::kCaps, p.text);
            c.text("PREDICTED, THE LAST CURVE GHOSTED", rib.x + 56.0f, rib.y + 10.0f, look::kCaps, p.label);
            if (folded_)
                c.text(fitText(c, ribbon_->peaksText(), look::kBody, rib.w - 330.0f).c_str(), rib.right() - 70.0f, rib.y + 9.0f,
                       look::kBody, p.text, funkgui::Align::right);
        }
        // The cockpit's own words.
        c.text("MODE", 96.0f, cock.y + 6.0f, look::kCaps, p.label);
        c.text("IGNITION", 12.0f, cock.y + 6.0f, look::kCaps, p.label);
    }

    std::string AppFrame::engineSentence() const
    {
        // From the Spec: "V8, 5.7 L, turbo, petrol" (each part only when its field is there).
        std::vector<std::string> parts;
        const auto lower = [](std::string w) {
            for (char& ch : w)
                ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
            return w;
        };
        const auto choice = [&](const char* path) -> std::string {
            const int f = doc_.find(path);
            if (f < 0 || doc_.info(f).kind != FieldKind::choice)
                return {};
            const auto k = static_cast<std::size_t>(std::lround(doc_.get(f)));
            return k < doc_.info(f).choices.size() ? doc_.info(f).choices[k] : std::string();
        };
        const auto value = [&](const char* path) {
            const int f = doc_.find(path);
            return f < 0 ? std::nan("") : doc_.get(f);
        };
        if (std::string l = choice("engine.layout"); !l.empty())
            parts.push_back(l);
        else if (const double n = value("engine.cylinders"); std::isfinite(n))
            parts.push_back(std::to_string(static_cast<int>(n)) + " cylinders");
        double litres = value("engine.displacement");
        if (!std::isfinite(litres))
        {
            const double b = value("engine.bore"), s = value("engine.stroke"), n = value("engine.cylinders");
            litres = std::isfinite(b * s * n) ? kPi / 4.0 * b * b * s * n : std::nan("");
        }
        if (std::isfinite(litres))
            parts.push_back(look::num(litres * 1000.0, "%.1f") + " L");
        if (std::string a = choice("boost.kind"); !a.empty())
            parts.push_back(a == "Natural" || a == "NA" ? std::string("naturally aspirated") : lower(a));
        if (std::string fuel = choice("fuel.kind"); !fuel.empty())
            parts.push_back(fuel == "E85" ? fuel : lower(fuel));
        std::string out;
        for (std::size_t i = 0; i < parts.size(); ++i)
            out += (i == 0 ? "" : ", ") + parts[i];
        return out;
    }

    // ---- time, input, unchanged frames ------------------------------------------------------------------------------
    void AppFrame::tick(float dt)
    {
        dyno_->tick(dt);
        // The units changed: the pages again, with the new units' texts.
        if (units_.revision() != unitsSeen_ && !(gestures() != nullptr && gestures()->dragging()))
        {
            unitsSeen_ = units_.revision();
            makeInspector();
            arrange();
        }
        // The selection moved (a part clicked on the stage, ALL): the inspector shows the new one.
        if (selection_.revision() != selectionSeen_)
        {
            selectionSeen_ = selection_.revision();
            if (selection_.path() != pagesFor_)
            {
                pagesFor_ = selection_.path();
                unitsPage_ = false;
                makeInspector();
                arrange();
            }
        }
        modes_[0]->setTitle(kModes[std::clamp(services_.mode ? services_.mode() : localMode_, 0, 2)]);
        Board::tick(dt);
        if (Board::wantsFullRate())
            dirty_ = true;
    }

    bool AppFrame::wantsFullRate() const
    {
        if (Board::wantsFullRate() || current().moving() || dyno_->busy())
            return true;
        const double s = ctx_->live.get(telemetry::kState);
        return std::isfinite(s) && s >= 0.5 && s < 2.5;   // cranking or running: the live values move
    }

    std::uint64_t AppFrame::frameKey() const
    {
        DrawKey k;
        k.mix(width()).mix(height()).mix(shown_).mix(page_).mix(unitsPage_ ? 1 : 0).mix(folded_ ? 1 : 0);
        k.mix(doc_.revision()).mix(selection_.revision()).mix(units_.revision()).mix(dyno_->revision());
        for (int i = 0; i < count(); ++i)
        {
            const Item& it = const_cast<AppFrame*>(this)->item(i);
            if (it.shown())
                k.mix(it.drawKey()).mix(it.settled() ? 1 : 0);
        }
        Workspace& ws = current();
        k.mix(ws.drawKey());
        for (int i = 0; i < ws.count(); ++i)
            if (ws.item(i).shown())
                k.mix(ws.item(i).drawKey());
        if (const FieldPage* page = inspector())
            for (const auto& it : page->items())
                if (it->shown())
                    k.mix(it->drawKey());
        return k.h;
    }

    bool AppFrame::needsDraw() const { return dirty_ || frameKey() != drawnKey_; }

    void AppFrame::draw(funkgui::Canvas& c, const funkgui::Theme& th)
    {
        Board::draw(c, th);
        drawnKey_ = frameKey();
        dirty_ = false;
        ++draws_;
    }

    void AppFrame::pointerDown(const funkgui::PointerEvent& e)
    {
        dirty_ = true;
        Board::pointerDown(e);
    }

    void AppFrame::pointerDrag(const funkgui::PointerEvent& e)
    {
        dirty_ = true;
        Board::pointerDrag(e);
    }

    void AppFrame::pointerUp(const funkgui::PointerEvent& e)
    {
        dirty_ = true;
        Board::pointerUp(e);
    }

    void AppFrame::pointerMove(const funkgui::PointerEvent& e)
    {
        dirty_ = true;
        Board::pointerMove(e);
    }

    bool AppFrame::wheel(const funkgui::WheelEvent& e)
    {
        dirty_ = true;
        return Board::wheel(e);
    }

    bool AppFrame::key(const funkgui::KeyEvent& e)
    {
        dirty_ = true;
        const bool command = e.mods.cmd || e.mods.ctrl;
        if (command && e.key == funkgui::Key::character && (e.ch == 'z' || e.ch == 'Z'))
        {
            if (e.mods.shift ? static_cast<bool>(services_.redo) : static_cast<bool>(services_.undo))
                (e.mods.shift ? services_.redo : services_.undo)();
            return true;
        }
        if (command && e.key == funkgui::Key::character && (e.ch == 'y' || e.ch == 'Y'))
        {
            if (services_.redo)
                services_.redo();
            return true;
        }
        return Board::key(e);
    }

    std::vector<int> AppFrame::fieldsShown()
    {
        const int wasShown = shown_;
        const std::string wasSelected = selection_.path();
        std::set<int> out;
        for (int w = 0; w < workspaceCount(); ++w)
        {
            show(w);
            selection_.clear();
            pagesFor_.clear();
            makeInspector();
            arrange();
            for (int p = 0; p < inspectorPageCount(); ++p)
            {
                showInspectorPage(p);
                FieldPage& page = *inspector();
                for (int i = 0; i < page.params().count(); ++i)   // a page scrolls: each of its controls is reachable
                    out.insert(page.params().field(i));
                for (const GuiPanel::WidgetAt& wa : page.widgets())
                {
                    const int f = doc_.find(page.spec().gui.nodes[static_cast<std::size_t>(wa.node)].file);
                    if (f >= 0 && page.items()[static_cast<std::size_t>(wa.item)]->shown())
                        out.insert(f);
                }
            }
        }
        show(wasShown);
        selection_.select(wasSelected);
        return { out.begin(), out.end() };
    }

    std::unique_ptr<AppFrame> makeAppFrame(FieldDoc& doc, Units& units, TelemetrySource& telemetry, FrameServices services,
                                           DynoSource* dyno, ScopeSource* scope)
    {
        return std::make_unique<AppFrame>(doc, units, telemetry, std::move(services), dyno, scope);
    }
}
