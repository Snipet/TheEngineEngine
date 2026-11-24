// ui/app/AppFrame.cpp: see AppFrame.h.
#include "app/AppFrame.h"

#include <funkgui/canvas/Canvas.h>
#include <funkgui/core/Theme.h>

#include <algorithm>
#include <cmath>
#include <set>

namespace tee::ui
{
    namespace
    {
        constexpr int kMaxPageTabs = 12;
        const std::string kNone;

        std::string firstLine(const std::string& s) { return s.substr(0, s.find('\n')); }

        // A quantity's unit as a stepped parameter: its units' index.
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
    }

    std::vector<Screen> defaultScreens()
    {
        return {
            { "Build", { "engine", "intake", "fuel" }, {}, false },
            { "Valvetrain", { "valve" }, {}, false },
            { "Boost", { "boost" }, {}, false },
            { "Exhaust", { "exhaust" }, {}, false },
            { "Tune", { "ecu" }, {}, false },
            { "Dyno", {},
              "row gap=16 {\n"
              "  gauge engine.speed width=230 height=230\n"
              "  gauge boost.pressure width=170 height=170\n"
              "  column gap=10 width=200 { bar engine.throttle ; bar engine.manifold ; lamp engine.flags label=\"LIMITER\" }\n"
              "}\n"
              "plot engine.speed label=\"ENGINE SPEED\" height=220\n",
              false },
            { "Drive", {},
              "row gap=16 {\n"
              "  gauge engine.speed width=230 height=230\n"
              "  gauge car.speed width=230 height=230\n"
              "  column gap=10 width=200 { bar engine.coolant ; bar engine.oilPressure ; bar boost.pressure ;"
              " lamp engine.flags label=\"LIMITER\" }\n"
              "}\n"
              "auto \"car\"\n"
              "auto \"env\"\n",
              false },
            { "Scope", {}, "plot engine.speed label=\"ENGINE SPEED\" height=200\nauto \"sound\"\n", false },
            { "Garage", {}, {}, true },
        };
    }

    AppFrame::AppFrame(FieldDoc& doc, Units& units, TelemetrySource& telemetry, FrameServices services,
                       std::vector<Screen> screens, int width, int height)
        : Board(width, height, 900, 560), doc_(doc), units_(units), telemetry_(telemetry), services_(std::move(services)),
          screens_(std::move(screens))
    {
        stateHandle_ = telemetry_.findValue(telemetry::kState);
        gearHandle_ = telemetry_.findValue(telemetry::kGear);
        speedHandle_ = telemetry_.findValue(telemetry::kCarSpeed);
        rpmHandle_ = telemetry_.findValue(telemetry::kSpeed);

        // The rail.
        for (int s = 0; s < screenCount(); ++s)
        {
            Button& b = make<Button>(screens_[static_cast<std::size_t>(s)].name, [this, s] { show(s); });
            b.setLit([this, s] { return screen_ == s; });
            rail_.push_back(&b);
        }
        // The top bar.
        name_ = &make<Label>([this] { return services_.buildName ? services_.buildName() : std::string("Untitled"); },
                             funkgui::TextStyle{ 15.0f, 0.6f, 0.06f, false }, Label::Ink::value);
        status_ = &make<Label>([this] { return services_.app.status ? firstLine(services_.app.status()) : std::string(); },
                               kWidgetSmall, Label::Ink::label);
        state_ = &make<Label>([this] { return stateText(); }, kWidgetLabel, Label::Ink::text, funkgui::Align::right);
        level_ = &make<Bar>("Output", [this] {
            const float peak = services_.outputLevel ? services_.outputLevel() : telemetry_.outputPeak();
            return std::clamp(peak, 0.0f, 1.0f);
        });
        undo_ = &make<Button>("Undo", [this] { if (services_.undo) services_.undo(); });
        redo_ = &make<Button>("Redo", [this] { if (services_.redo) services_.redo(); });
        start_ = &make<Button>("Start");
        start_->setOnHold([this](bool held) {
            controls_.startHeld = held;
            if (services_.app.panelControls)
                services_.app.panelControls(controls_);
        });
        start_->setLit([this] { return stateHandle_ >= 0 && telemetry_.value(stateHandle_) >= 0.5 && telemetry_.value(stateHandle_) < 2.5; });
        // The page strip.
        for (int p = 0; p < kMaxPageTabs; ++p)
        {
            Button& b = make<Button>("", [this, p] { show(screen_, p); });
            b.setLit([this, p] { return page_ == p; });
            pageTabs_.push_back(&b);
        }
        // The bottom strip.
        const auto pedal = [this](float ControlsFrame::*m) {
            return [this, m] { return services_.app.controls ? services_.app.controls().*m : 0.0f; };
        };
        throttle_ = &make<Bar>("Throttle", pedal(&ControlsFrame::throttle));
        brake_ = &make<Bar>("Brake", pedal(&ControlsFrame::brake));
        clutch_ = &make<Bar>("Clutch", pedal(&ControlsFrame::clutch));
        gear_ = &make<Label>([this] {
            if (gearHandle_ < 0)
                return std::string("-");
            const long g = std::lround(telemetry_.value(gearHandle_));
            return g < 0 ? std::string("R") : g == 0 ? std::string("N") : std::to_string(g);
        }, kWidgetValue, Label::Ink::value, funkgui::Align::left, "Gear");
        speed_ = &make<Label>([this] { return speedHandle_ < 0 ? std::string("-") : units_.format(Quantity::speed, telemetry_.value(speedHandle_)); },
                              kWidgetValue, Label::Ink::value, funkgui::Align::left, "Speed");
        rpm_ = &make<Label>([this] { return rpmHandle_ < 0 ? std::string("-") : units_.format(Quantity::rotation, telemetry_.value(rpmHandle_)); },
                            kWidgetValue, Label::Ink::value, funkgui::Align::left, "Engine");
        makeGarage();
        makePages();
        unitsSeen_ = units_.revision();
        arrange();
    }

    AppFrame::~AppFrame() = default;

    std::string AppFrame::stateText() const
    {
        if (stateHandle_ < 0)
            return {};
        switch (std::lround(telemetry_.value(stateHandle_)))
        {
            case 0: return "OFF";
            case 1: return "CRANKING";
            case 2: return "RUNNING";
            case 3: return "STOPPING";
            default: return {};
        }
    }

    // ---- the pages ---------------------------------------------------------------------------------------------------
    void AppFrame::makePages()
    {
        // The sections the screens name (in a list or by `auto`); what none names goes to the first screen of sections.
        std::set<std::string> claimed;
        for (const Screen& s : screens_)
        {
            claimed.insert(s.sections.begin(), s.sections.end());
            for (std::size_t at = s.gui.find("auto"); at != std::string::npos; at = s.gui.find("auto", at + 4))
            {
                const std::size_t q0 = s.gui.find('"', at), q1 = q0 == std::string::npos ? q0 : s.gui.find('"', q0 + 1);
                if (q1 != std::string::npos)
                    claimed.insert(s.gui.substr(q0 + 1, q1 - q0 - 1));
            }
        }
        std::vector<std::string> rest;
        for (const std::string& sec : sectionsOf(doc_))
            if (!claimed.count(sec) && !sectionFields(doc_, sec).empty())
                rest.push_back(sec);

        states_.clear();
        states_.resize(screens_.size());
        bool restPlaced = false;
        for (std::size_t s = 0; s < screens_.size(); ++s)
        {
            const Screen& def = screens_[s];
            if (def.garage)
                continue;
            ScreenState& st = states_[s];
            if (!def.gui.empty())
                st.pages.push_back({ upper(def.name), def.gui, nullptr });
            else
            {
                std::vector<std::string> sections = def.sections;
                if (!restPlaced)
                {
                    sections.insert(sections.end(), rest.begin(), rest.end());
                    restPlaced = true;
                }
                for (const std::string& sec : sections)
                {
                    const std::vector<int> fields = sectionFields(doc_, sec);
                    if (fields.empty())
                        continue;
                    const auto pages = splitPages(doc_, fields, 60);
                    for (std::size_t p = 0; p < pages.size(); ++p)
                        st.pages.push_back({ upper(sec) + (pages.size() > 1 ? " " + std::to_string(p + 1) : std::string()),
                                             pageGui(doc_, pages[p], sec), nullptr });
                }
                if (st.pages.empty())
                    st.pages.push_back({ upper(def.name), "label \"NOTHING TO SET HERE YET\" size=small", nullptr });
            }
            const bool strip = st.pages.size() > 1;
            for (Page& page : st.pages)
            {
                GuiPanel::Options o;
                o.width = width();
                o.height = height();
                o.minWidth = minWidth();
                o.minHeight = minHeight();
                o.name = "app";
                o.insetLeft = static_cast<int>(kRailW) + 12;
                o.insetTop = static_cast<int>(kTopH + (strip ? kPagesH : 0.0f)) + 10;
                o.insetBottom = static_cast<int>(kBottomH) + 8;
                o.insetRight = 12;
                page.panel = std::make_unique<FieldPage>(doc_, units_, telemetry_,
                                                         Sources{ { def.name + ".gui", page.gui } }, std::move(o));
                if (host() != nullptr)
                    page.panel->attach(*host());
            }
        }
        page_ = std::clamp(page_, 0, std::max(0, pageCount(screen_) - 1));
        bump();
    }

    int AppFrame::pageCount(int s) const
    {
        return s >= 0 && s < screenCount() ? static_cast<int>(states_[static_cast<std::size_t>(s)].pages.size()) : 0;
    }

    FieldPage* AppFrame::pageOf(int s, int p) const
    {
        if (p < 0 || p >= pageCount(s))
            return nullptr;
        return states_[static_cast<std::size_t>(s)].pages[static_cast<std::size_t>(p)].panel.get();
    }

    const std::string& AppFrame::pageTitle(int s, int p) const
    {
        if (p < 0 || p >= pageCount(s))
            return kNone;
        return states_[static_cast<std::size_t>(s)].pages[static_cast<std::size_t>(p)].title;
    }

    void AppFrame::attached()
    {
        for (ScreenState& st : states_)
            for (Page& p : st.pages)
                p.panel->attach(*host());
    }

    funkgui::Panel* AppFrame::inner() const { return shownPage(); }

    void AppFrame::show(int s, int p)
    {
        if (s < 0 || s >= screenCount())
            return;
        if (funkgui::Panel* now = inner(); now != nullptr && (s != screen_ || p != page_))
        {
            now->pointerExit();
            now->closeGestures();
        }
        screen_ = s;
        page_ = std::clamp(p, 0, std::max(0, pageCount(s) - 1));
        if (FieldPage* in = shownPage())
            in->resize(width(), height());
        if (screens_[static_cast<std::size_t>(s)].garage && services_.builds)
            garageList_->setRows(builds_ = services_.builds());
        arrange();
    }

    // ---- the garage --------------------------------------------------------------------------------------------------
    void AppFrame::makeGarage()
    {
        garageList_ = &make<List>("Builds", std::vector<std::string>{});
        garageLoad_ = &make<Button>("Load", [this] {
            const int row = garageList_->selected();
            if (row < 0 || row >= static_cast<int>(builds_.size()) || !services_.load)
            {
                message_ = "Choose a build to load.";
                return;
            }
            std::string error;
            message_ = services_.load(builds_[static_cast<std::size_t>(row)], error)
                           ? "Loaded " + builds_[static_cast<std::size_t>(row)] + "."
                           : "Not loaded: " + firstLine(error);
        });
        garageName_ = &make<TextField>("Name", [this] { return services_.buildName ? services_.buildName() : std::string(); },
                                       [this](const std::string& s) {
                                           if (s.empty())
                                               return false;
                                           return services_.rename ? services_.rename(s) : false;
                                       });
        garageSave_ = &make<Button>("Save as", [this] {
            const std::string name = services_.buildName ? services_.buildName() : std::string();
            std::string error;
            if (!services_.save)
                message_ = "Saving is not here yet.";
            else if (services_.save(name, error))
            {
                message_ = "Saved " + name + ".";
                if (services_.builds)
                    garageList_->setRows(builds_ = services_.builds());
            }
            else
                message_ = "Not saved: " + firstLine(error);
        });
        garageNote_ = &make<Label>([this] { return message_; }, kWidgetText, Label::Ink::text);
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
    }

    void AppFrame::placeGarage(bool shown)
    {
        const funkgui::Rect c = contentRect();
        const float colW = std::min(340.0f, c.w * 0.4f);
        garageList_->setPlace({ c.x, c.y, colW, std::max(120.0f, c.h - 50.0f) }, shown);
        garageLoad_->setPlace({ c.x, c.bottom() - 34.0f, 110.0f, 32.0f }, shown);
        const float x = c.x + colW + 30.0f, w = std::min(420.0f, c.right() - x);
        garageName_->setPlace({ x, c.y + 16.0f, w, 30.0f }, shown);
        garageSave_->setPlace({ x, c.y + 56.0f, 110.0f, 32.0f }, shown);
        garageNote_->setPlace({ x + 124.0f, c.y + 56.0f, std::max(10.0f, w - 124.0f), 32.0f }, shown);
        const float uy = c.y + 120.0f, uw = std::min(300.0f, (c.right() - x - 20.0f) / 2.0f);
        for (std::size_t i = 0; i < unitSteppers_.size(); ++i)
        {
            const float col = static_cast<float>(i % 2), row = static_cast<float>(i / 2);
            unitSteppers_[i]->setPlace({ x + col * (uw + 20.0f), uy + 20.0f + row * 34.0f, uw, 28.0f }, shown);
        }
    }

    // ---- placing -----------------------------------------------------------------------------------------------------
    funkgui::Rect AppFrame::railRect(int s) const
    {
        return { 10.0f, kTopH + 12.0f + 40.0f * static_cast<float>(s), kRailW - 20.0f, 32.0f };
    }

    funkgui::Rect AppFrame::pageTabRect(int p) const
    {
        const float w = 120.0f;
        return { kRailW + 12.0f + (w + 6.0f) * static_cast<float>(p), kTopH + 4.0f, w, kPagesH - 8.0f };
    }

    funkgui::Rect AppFrame::startRect() const
    {
        return { static_cast<float>(width()) - 122.0f, 10.0f, 110.0f, kTopH - 20.0f };
    }

    funkgui::Rect AppFrame::contentRect() const
    {
        const bool strip = pageCount(screen_) > 1;
        const float top = kTopH + (strip ? kPagesH : 0.0f) + 10.0f;
        return { kRailW + 12.0f, top, static_cast<float>(width()) - kRailW - 24.0f,
                 static_cast<float>(height()) - top - kBottomH - 8.0f };
    }

    void AppFrame::arrange()
    {
        const float w = static_cast<float>(width()), h = static_cast<float>(height());
        for (std::size_t s = 0; s < rail_.size(); ++s)
            rail_[s]->setPlace(railRect(static_cast<int>(s)), true);
        // The top bar, from the right: START, redo, undo, the level, the state; the name and the status at the left.
        const funkgui::Rect st = startRect();
        start_->setPlace(st, true);
        redo_->setPlace({ st.x - 78.0f, st.y, 70.0f, st.h }, true);
        undo_->setPlace({ st.x - 154.0f, st.y, 70.0f, st.h }, true);
        level_->setPlace({ st.x - 324.0f, 8.0f, 150.0f, kTopH - 14.0f }, true);
        state_->setPlace({ st.x - 450.0f, 10.0f, 110.0f, kTopH - 20.0f }, true);
        const float left = kRailW + 16.0f, room = std::max(60.0f, st.x - 460.0f - left);
        name_->setPlace({ left, 6.0f, room, 24.0f }, true);
        status_->setPlace({ left, 30.0f, room, 14.0f }, true);
        // The page strip.
        const int pages = pageCount(screen_);
        for (int p = 0; p < kMaxPageTabs; ++p)
        {
            const bool shown = pages > 1 && p < pages;
            if (shown)
                pageTabs_[static_cast<std::size_t>(p)]->setTitle(pageTitle(screen_, p));
            pageTabs_[static_cast<std::size_t>(p)]->setPlace(pageTabRect(p), shown);
        }
        // The bottom strip.
        const float by = h - kBottomH + 10.0f;
        throttle_->setPlace({ left, by, 170.0f, 40.0f }, true);
        brake_->setPlace({ left + 186.0f, by, 170.0f, 40.0f }, true);
        clutch_->setPlace({ left + 372.0f, by, 170.0f, 40.0f }, true);
        gear_->setPlace({ left + 580.0f, by - 2.0f, 60.0f, 44.0f }, true);
        speed_->setPlace({ left + 660.0f, by - 2.0f, 150.0f, 44.0f }, w > left + 820.0f);
        rpm_->setPlace({ left + 830.0f, by - 2.0f, 150.0f, 44.0f }, w > left + 990.0f);
        placeGarage(screens_[static_cast<std::size_t>(screen_)].garage);
        if (FieldPage* in = shownPage())
            in->resize(width(), height());
        bump();
    }

    void AppFrame::drawOver(funkgui::Canvas& c, const funkgui::Theme& th) const
    {
        const Look l = Look::of(th);
        const float w = static_cast<float>(width()), h = static_cast<float>(height());
        const funkgui::Col bar = l.over(l.edge, 0.45f);
        if (screens_[static_cast<std::size_t>(screen_)].garage)
            c.rrect(kRailW, kTopH, w - kRailW, h - kTopH - kBottomH, 0.0f, th.ground);   // no page under the garage
        c.rrect(0.0f, 0.0f, kRailW, h, 0.0f, bar);
        c.rrect(kRailW, 0.0f, w - kRailW, kTopH, 0.0f, bar);
        c.rrect(kRailW, h - kBottomH, w - kRailW, kBottomH, 0.0f, bar);
        c.hairlineV(c.snapX(kRailW), 0.0f, h, l.edge);
        c.hairlineH(kRailW, c.snapY(kTopH), w - kRailW, l.edge);
        c.hairlineH(kRailW, c.snapY(h - kBottomH), w - kRailW, l.edge);
        if (pageCount(screen_) > 1)
        {
            c.rrect(kRailW + 1.0f, kTopH + 1.0f, w - kRailW - 1.0f, kPagesH - 1.0f, 0.0f, l.over(l.edge, 0.2f));
            c.hairlineH(kRailW, c.snapY(kTopH + kPagesH), w - kRailW, l.edge);
        }
        // The wordmark, at the top of the rail, and the one accent's mark under it.
        c.text("THE ENGINE", kRailW * 0.5f, 12.0f, kWidgetLabel, l.label, funkgui::Align::centre);
        c.text("ENGINE", kRailW * 0.5f, 25.0f, funkgui::type::kWordmark, l.value, funkgui::Align::centre);
        c.rrect(kRailW * 0.5f - 12.0f, kTopH - 6.0f, 24.0f, 2.0f, 1.0f, l.accent);
    }

    // ---- input and time ----------------------------------------------------------------------------------------------
    void AppFrame::tick(float dt)
    {
        // The units changed (the garage's steppers, the settings): the pages again, with the new units' texts.
        if (units_.revision() != unitsSeen_ && !(gestures() != nullptr && gestures()->dragging()))
        {
            unitsSeen_ = units_.revision();
            makePages();
            arrange();
        }
        Board::tick(dt);
    }

    bool AppFrame::key(const funkgui::KeyEvent& e)
    {
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

    std::vector<int> AppFrame::fieldsShown() const
    {
        std::set<int> out;
        for (const ScreenState& st : states_)
            for (const Page& p : st.pages)
            {
                FieldPage& page = *p.panel;
                for (int i = 0; i < page.params().count(); ++i)
                    if (page.controlRect(page.params().info(i).id).w > 0.0f)
                        out.insert(page.params().field(i));
                for (const GuiPanel::WidgetAt& w : page.widgets())
                {
                    const int f = doc_.find(page.spec().gui.nodes[static_cast<std::size_t>(w.node)].file);
                    if (f >= 0 && page.items()[static_cast<std::size_t>(w.item)]->shown())
                        out.insert(f);
                }
            }
        return { out.begin(), out.end() };
    }

    std::unique_ptr<AppFrame> makeAppFrame(FieldDoc& doc, Units& units, TelemetrySource& telemetry, FrameServices services)
    {
        return std::make_unique<AppFrame>(doc, units, telemetry, std::move(services));
    }
}
