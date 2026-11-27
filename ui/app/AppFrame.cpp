// ui/app/AppFrame.cpp: see AppFrame.h.
#include "app/AppFrame.h"

#include "app/Look.h"
#include "app/Placeholders.h"
#include "kit/inspect/FieldInspector.h"
#include "kit/inspect/Outline.h"
#include "kit/widgets/Tables.h"

#include <funkgui/canvas/Canvas.h>
#include <funkgui/core/Theme.h>
#include <funkgui/prefs/UiPreferences.h>
#include <funkgui/text/FontService.h>
#include <funkgui/text/TextFit.h>
#include <funkgui/widgets/Scroll.h>

#include <algorithm>
#include <cmath>
#include <set>

#if __has_include("views/garage/Library.h")
#include "views/garage/Library.h"
#define TEE_UI_HAS_LIBRARY 1
#endif

namespace tee::ui
{
    namespace
    {
        constexpr double kPi = 3.14159265358979323846;
        constexpr double kToRpm = 30.0 / kPi;
        constexpr int kMaxTabs = 16;   // TUNE has eleven pages (J2's report, 25 Nov: past eight they had no tab)
        const std::string kNone;

        std::string firstLine(const std::string& s) { return s.substr(0, s.find('\n')); }

        // A generated page with a workspace's notes on its section (Workspace::sectionNotes) at its top, amber, each in
        // lines of at most 46 characters (a label's most is 48) broken at spaces.
        std::string withNotes(std::string gui, const std::vector<std::string>& notes)
        {
            const std::size_t open = gui.find('{');
            if (notes.empty() || open == std::string::npos)
                return gui;
            std::string labels;
            for (const std::string& note : notes)
                for (std::size_t at = 0; at < note.size();)
                {
                    std::size_t end = std::min(note.size(), at + 46);
                    if (end < note.size())
                        if (const std::size_t sp = note.rfind(' ', end); sp != std::string::npos && sp > at)
                            end = sp;
                    labels += " label \"" + note.substr(at, end - at) + "\" size=small color=yellow ;";
                    at = end + (end < note.size() && note[end] == ' ' ? 1 : 0);
                }
            return gui.insert(open + 1, labels);
        }

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

        // The rail's pictograms, drawn over its buttons (an item of no input and no accessibility of its own: each
        // button says its workspace's name).
        class RailMarks final : public Widget
        {
        public:
            RailMarks(uint32_t id, std::function<void(funkgui::Canvas&, const look::Palette&)> paint, std::function<uint64_t()> key)
                : Widget(id), paint_(std::move(paint)), key_(std::move(key))
            {
            }
            void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool) const override { paint_(c, look::Palette::of(th)); }
            bool focusable() const override { return false; }
            bool contains(funkgui::Point) const override { return false; }
            void accessibility(std::vector<funkgui::A11yItem>&) const override {}
            bool a11yAction(uint32_t, funkgui::A11yAction, double, funkgui::GestureController&) override { return false; }
            uint64_t drawKey() const override { return key_(); }

        protected:
            const char* kind() const override { return "rail marks"; }

        private:
            std::function<void(funkgui::Canvas&, const look::Palette&)> paint_;
            std::function<uint64_t()> key_;
        };

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
            { "listen", "LISTEN", "LSN", { "sound" },
              "Where the car is heard from: the car from above with every source as a dot sized by its level, and a "
              "listener to drag; the cameras, the places, the cabin; the loudness and the limiter.",
              "package U5", true },
            { "scope", "SCOPE", "SCP", {},
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
        // The status line, cut at whole fields ("   " apart) then whole words to its room: never part of a word.
        status_ = &make<Label>([this] {
            // The calibration's bake first while it works (the lead's ruling: a person on any workspace knows something
            // is happening), then the application's line.
            std::string line = services_.app.status ? firstLine(services_.app.status()) : std::string();
            if (const std::string bake = bakeWords(); !bake.empty())
                line = line.empty() ? bake : bake + "   " + line;
            const float room = status_ != nullptr ? status_->bounds().w : 0.0f;
            const auto fits = [&](const std::string& t) {
                return funkgui::text::width(funkgui::FontService::get().atlas(), t.c_str(), kWidgetSmall) <= room;
            };
            while (!line.empty() && !fits(line))
            {
                std::size_t cut = line.rfind("   ");
                if (cut == std::string::npos)
                    cut = line.rfind(' ');
                line = cut == std::string::npos ? std::string() : line.substr(0, line.find_last_not_of(' ', cut) + 1);
            }
            return line;
        }, kWidgetSmall, Label::Ink::label, funkgui::Align::right);
        // Under it, while the bake runs with the engine on the tables from before an edit (card C's option 1): which,
        // in the label's ink (information, not a warning), cut at whole words to its room.
        staleLine_ = &make<Label>([this] {
            std::string line = bakeStaleLine(bakeStatus());
            const float room = staleLine_ != nullptr ? staleLine_->bounds().w : 0.0f;
            while (!line.empty() && funkgui::text::width(funkgui::FontService::get().atlas(), line.c_str(), kWidgetSmall) > room)
            {
                const std::size_t cut = line.rfind(' ');
                line = cut == std::string::npos ? std::string() : line.substr(0, cut);
            }
            return line;
        }, kWidgetSmall, Label::Ink::label, funkgui::Align::right);
        // The load: the peak share of a core, its verdict and the dropouts; while the build does not fit, the sentence
        // and QUALITY one click away (it never switches by itself: Economy changes the sound, the owner's choice).
        load_ = &make<LoadMeter>(LoadSources{ services_.simLoadPeak, services_.simLoad, services_.loadFits, services_.dropouts });
        advice_ = &make<LoadAdvice>([this] { return services_.quality && services_.quality() == 1; });
        adviceButton_ = &make<Button>("Quality", [this] { showSettings(true); });
        // The output's peak: the bar on a scale of decibels (-60 to 0 dBFS), its number in dB.
        const auto peak = [this] {
            return std::clamp(services_.outputLevel ? services_.outputLevel() : telemetry_.outputPeak(), 0.0f, 1.0f);
        };
        level_ = &make<Bar>("Output", [peak] {
            const float p = peak();
            return p <= 0.001f ? 0.0f : std::clamp(1.0f + 20.0f * std::log10(p) / 60.0f, 0.0f, 1.0f);
        }, [peak] {
            const float p = peak();
            return p <= 0.001f ? std::string("-60 dB") : look::num(20.0 * std::log10(static_cast<double>(p)), "%.0f") + " dB";
        });
        // The listener's limiter: its gain reduction (tel.sound.limiter, dB), a small bar under the output's while it is
        // not zero, with the number.
        limiter_ = &make<Bar>("Limiter", [this] {
            const double db = ctx_->live.get("sound.limiter");
            return std::isfinite(db) ? static_cast<float>(std::clamp(std::fabs(db) / 12.0, 0.0, 1.0)) : 0.0f;
        }, [this] { return look::num(-std::fabs(ctx_->live.get("sound.limiter")), "%.1f") + " dB"; });
        unitsButton_ = &make<Button>("Settings", [this] {
            if (chatOpen_)
            {
                chatOpen_ = false;
                if (unitsPage_)
                {
                    makeInspector();
                    arrange();
                    return;
                }
            }
            showUnits(!unitsPage_);
        });
        unitsButton_->setLit([this] { return unitsPage_ && !chatOpen_; });
        chatButton_ = &make<Button>("Chat", [this] { openChat(!chatOpen_); });   // shown once a chat is mounted
        chatButton_->setLit([this] { return chatOpen_; });
        chatButton_->setPlace({}, false);
        undo_ = &make<Button>("Undo", [this] { if (services_.undo) services_.undo(); });
        redo_ = &make<Button>("Redo", [this] { if (services_.redo) services_.redo(); });

        makeWorkspaces();
        // The rail.
        for (int i = 0; i < workspaceCount(); ++i)
        {
            Button& b = make<Button>(slots_[static_cast<std::size_t>(i)].title, [this, i] { show(i); });
            b.setLit([this, i] { return shown_ == i; });
            b.setLeftRoom(24.0f);
            rail_.push_back(&b);
        }
        railMarks_ = &make<RailMarks>([this](funkgui::Canvas& c, const look::Palette& p) {
            if (railWidth() < kRailW)
                return;
            for (std::size_t i = 0; i < rail_.size(); ++i)
            {
                const funkgui::Rect r = rail_[i]->bounds();
                const std::string& id = slots_[i].id;
                const std::string_view plain = std::string_view(id).substr(id.rfind('-') == std::string::npos ? 0 : id.rfind('-') + 1);
                look::pictogram(c, plain, r.x + 15.0f, r.centreY(), 12.0f, shown_ == static_cast<int>(i) ? p.value : p.label);
            }
        }, [this] { return DrawKey{}.mix(shown_).mix(railWidth()).h; });
        railMarks_->setShown(true);
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
        audioLine_ = &make<Label>([this] {
            if (services_.audioInfo)
                return services_.audioInfo();
            return services_.app.status ? firstLine(services_.app.status()) : std::string("No audio device.");
        }, kWidgetText, Label::Ink::text);
        audioMenu_ = &make<Button>("Audio device...", [this] { if (services_.audioMenu) services_.audioMenu(); });
        // The theme: FunkGui's preference, which the window follows (as FunkGui's own theme cells write it).
        graphite_ = &make<Button>("Graphite", [] { funkgui::UiPreferences::get().setTheme(0); });
        paper_ = &make<Button>("Paper", [] { funkgui::UiPreferences::get().setTheme(1); });
        graphite_->setLit([this] { return host() != nullptr && host()->themeIndex() == 0; });
        paper_->setLit([this] { return host() != nullptr && host()->themeIndex() == 1; });
        // The simulator's quality: FULL or ECONOMY, through the services (none: FULL, and the buttons do nothing).
        const auto quality = [this] { return services_.quality ? services_.quality() : 0; };
        full_ = &make<Button>("Full", [this] { if (services_.setQuality) services_.setQuality(0); });
        economy_ = &make<Button>("Economy", [this] { if (services_.setQuality) services_.setQuality(1); });
        full_->setLit([quality] { return quality() == 0; });
        full_->setEnabled([this] { return static_cast<bool>(services_.setQuality); });
        economy_->setEnabled([this] { return static_cast<bool>(services_.setQuality); });
        economy_->setLit([quality] { return quality() == 1; });
        warmStart_ = &make<Button>("Warm", [this] { if (services_.setColdStart) services_.setColdStart(false); });
        coldStart_ = &make<Button>("Cold", [this] { if (services_.setColdStart) services_.setColdStart(true); });
        warmStart_->setLit([this] { return services_.coldStart && !services_.coldStart(); });
        coldStart_->setLit([this] { return services_.coldStart && services_.coldStart(); });
        wheelOff_ = &make<Button>("Off", [this] { if (services_.setWheelOn) services_.setWheelOn(false); });
        wheelOn_ = &make<Button>("On", [this] { if (services_.setWheelOn) services_.setWheelOn(true); });
        wheelOff_->setLit([this] { return services_.wheelOn && !services_.wheelOn(); });
        wheelOn_->setLit([this] { return services_.wheelOn && services_.wheelOn(); });
        // CLUTCH, three ways (card B's input::ClutchMode): with an H-shifter its clutch is yours (the default), or the
        // gearbox works it, or the clutch is yours whatever moves the lever.
        static const char* const kClutch[3] = { "Shifter's", "Automatic", "Yours" };
        for (int m = 0; m < 3; ++m)
        {
            clutch_[static_cast<std::size_t>(m)] = &make<Button>(kClutch[m], [this, m] { if (services_.setClutchMode) services_.setClutchMode(m); });
            clutch_[static_cast<std::size_t>(m)]->setLit([this, m] { return clutchLit() == m; });
        }
        desktopOff_ = &make<Button>("Deny", [this] { setDesktop(false); });
        desktopOn_ = &make<Button>("Allow", [this] { setDesktop(true); });
        desktopOff_->setLit([this] { return services_.desktopAllowed && !services_.desktopAllowed(); });
        desktopOn_->setLit([this] { return services_.desktopAllowed && services_.desktopAllowed(); });
        ribbon_ = &make<DynoRibbon>(*dyno_, units_);
        fold_ = &make<Button>("Fold", [this] { foldRibbon(!folded_); });
        makeCockpit();
        // The first launch's card, made last: over the stage, and the first to take a press there.
        tryFirst_ = &make<TryFirstCard>();
        tryFirstClose_ = &make<Button>("Close", [this] { closeTryFirst(); });
        keyButton_ = &make<Button>("Key", [this] { openKeyDialog(); });   // the chat's header: its key dialog
        keyButton_->setPlace({}, false);
        // The card on the picture (over the stage), then the chat's place (mountChat fills it), then the key dialog
        // and the assistant's question over everything.
        cardItem_ = &make<FieldInspector>(doc_, units_, FieldInspector::Style::list);
        card_ = std::make_unique<PartCard>(*cardItem_);
        {
            auto slot = std::make_unique<ChatSlot>();
            chatSlot_ = slot.get();
            add(std::move(slot));
            KeyDialog::Hooks h;
            h.save = [this](std::string typed, std::string& why) {
                if (!services_.saveKey)
                {
                    why = "this window cannot keep a key";
                    return false;
                }
                return services_.saveKey(std::move(typed), why);
            };
            h.mask = [this] { return services_.keyMask ? services_.keyMask() : std::string(); };
            h.remove = [this](std::string& why) {
                if (!services_.removeKey)
                {
                    why = "this window cannot remove a key";
                    return false;
                }
                return services_.removeKey(why);
            };
            h.clipboard = [this] { return host() != nullptr ? host()->clipboardText() : std::string(); };
            h.closed = [this] {
                arrange();
                setFocus(-1);
            };
            auto dialog = std::make_unique<KeyDialog>(nextId(), std::move(h));
            keyDialog_ = dialog.get();
            keyDialogIndex_ = add(std::move(dialog));
            auto asking = std::make_unique<AskDialog>(nextId());
            askDialog_ = asking.get();
            askIndex_ = add(std::move(asking));
        }
        writeHelp();

        unitsSeen_ = units_.revision();
        selectionSeen_ = selection_.revision();
        show(std::max(0, find("engine")));
    }

    // Hover help for the frame's own items (the owner's ask: help on hover everywhere); each line says what the thing
    // does or shows, nothing it does not.
    void AppFrame::writeHelp()
    {
        load_->setHelp("What the simulator takes of one CPU core, from its busiest moment in the last second. Past 70 % it "
                       "is tight and past 90 % the sound may drop out; Economy quality about halves it.");
        level_->setHelp("The output's loudest sample in the last moment, in decibels below full scale (0 dB).");
        limiter_->setHelp("How far the output limiter is turning the sound down, in decibels.");
        unitsButton_->setHelp("The settings: the audio device, the theme, the quality, how the engine starts, and the "
                              "units every number is shown in.");
        undo_->setHelp("Undoes the last change to the build (Cmd or Ctrl Z).");
        redo_->setHelp("Does again the change last undone (Cmd or Ctrl Y, or Shift Z).");
        adviceButton_->setHelp("Opens the settings, where QUALITY can be set to Economy.");
        clear_->setHelp("Clears the selection: the inspector shows the whole workspace's settings again.");
        audioMenu_->setHelp("Chooses the audio output device.");
        graphite_->setHelp("The dark theme.");
        paper_->setHelp("The light theme.");
        full_->setHelp("The engine computed at its full inner rate.");
        economy_->setHelp("The engine computed at half its inner rate: about half the CPU load, the highest octave lost.");
        warmStart_->setHelp("From the next launch the engine starts at its running temperature.");
        coldStart_->setHelp("From the next launch the engine starts cold, as after a night parked.");
        desktopOff_->setHelp("Claude Desktop cannot reach this app: it listens on nothing.");
        desktopOn_->setHelp("Lets Claude Desktop on this computer reach the build open here through a private link, and "
                            "change it. The status line says when it is connected and what it does.");
        clutch_[0]->setHelp("With an H-shifter, the clutch is yours: its pedal, or Left Shift. With the keys or paddles alone, "
                            "the gearbox works it.");
        clutch_[1]->setHelp("The gearbox works the clutch, whatever shifts: no clutch to work.");
        clutch_[2]->setHelp("The clutch is always yours, whatever moves the lever.");
        wheelOff_->setHelp("The steering wheel is not read.");
        wheelOn_->setHelp("Reads a connected Logitech G29: its wheel, its pedals and its paddles. The line under it says "
                          "what it is doing.");
        chatButton_->setHelp("Opens the chat with Claude in the inspector's column; again to close it.");
        keyButton_->setHelp("The Anthropic API key the chat uses: save, replace or remove it.");
        ribbon_->setHelp("The build's predicted torque and power, worked out again after each edit. The faint curves are "
                         "the build before the last edit.");
        fold_->setHelp("Folds the dyno strip to its title row and gives the inspector its room.");
        tryFirstClose_->setHelp("Closes this card for good.");
        for (std::size_t i = 0; i < rail_.size() && i < slots_.size(); ++i)
            rail_[i]->setHelp("Opens " + slots_[i].title + ".");
        static const char* const kPedalHelp[] = {
            "The throttle pedal as the keys or a controller press it. Drag it to hold it there with the mouse.",
            "The brake pedal as the keys or a controller press it. Drag it to hold it there with the mouse.",
            "The clutch pedal as the keys or a controller press it. Drag it to hold it there with the mouse.",
        };
        for (std::size_t i = 0; i < pedals_.size() && i < std::size(kPedalHelp); ++i)
            pedals_[i]->setHelp(kPedalHelp[i]);
    }

    BakeStatus AppFrame::bakeStatus() const
    {
        if (services_.bakeStatus)
            return services_.bakeStatus();
        return bakeStatusOf(services_.sparkMap ? services_.sparkMap() : std::string());
    }

    std::string AppFrame::ribbonStaleCaption() const
    {
        const BakeStatus b = bakeStatus();
        return b.stale() && !b.setAside && b.working() ? std::string("ON THE OLD TABLES UNTIL BAKED") : std::string();
    }

    std::string AppFrame::bakeWords() const
    {
        // The bake's state as the status line says it while the bake works or failed (BakeStatus.h's words); empty
        // otherwise.
        return bakeLine(bakeStatus());
    }

    void AppFrame::closeTryFirst()
    {
        if (tryFirstClosed_)
            return;
        tryFirstClosed_ = true;
        tryFirstNotes_.clear();
        if (services_.tryFirstDone)
            services_.tryFirstDone();
        arrange();
    }

    AppFrame::~AppFrame()
    {
        // A question not answered: no (nothing lost), so its asker does not wait out its limit.
        closing_ = true;
        if (askOpen())
            askDialog_->answer(false);
        {
            const std::lock_guard<std::mutex> lock(askMutex_);
            for (Question& q : asks_)
                q.answer->set_value(false);
            asks_.clear();
        }
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
                ws = s.id.ends_with("garage") ? makePlaceholderGarage(*ctx_, s) : makePlaceholder(*ctx_, s);
            ws->resize(width(), height());
            workspaces_.push_back(std::move(ws));
        }
        // A workspace registered with an id that none of ui-design.md's ten has goes at the end of the rail (one of the
        // ten left out of `slots` on purpose, as a test's frame does, stays out).
        const std::vector<WorkspaceSlot> ten = defaultSlots();
        for (const WorkspaceEntry& e : registered)
            if (std::none_of(slots_.begin(), slots_.end(), [&](const WorkspaceSlot& s) { return s.id == e.id; })
                && std::none_of(ten.begin(), ten.end(), [&](const WorkspaceSlot& s) { return s.id == e.id; }))
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
        start_->setEnabled([this] { return !services_.engineReady || services_.engineReady(); });
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
        mode.setEnabled([this] { return !services_.engineReady || services_.engineReady(); });
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
        add("Lambda", [this] { return look::num(ctx_->live.lambda(), "%.2f"); }, {});
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
        brake_ = &make<BrakeLamp>(*ctx_);
        start_->setHelp("Hold to turn the starter; let go once the engine runs.");
        modes_[0]->setHelp("Where the engine runs: BENCH, free with no load; DYNO, held against a dynamometer; DRIVE, in "
                           "the car. Click to change.");
        rpm_->setHelp("Engine speed in rpm. The hatched zone is past the redline; the shift lights count down to it.");
        brake_->setHelp("A heavy diesel's brakes: lit while each acts; the segments are the engine brake's stage.");
        static const char* const kReadoutHelp[] = {
            "The gear engaged (N neutral, R reverse).",
            "Road speed.",
            "The manifold's pressure over the air outside: boost above zero, vacuum below.",
            "Lambda: the exhaust's air-fuel ratio over the ideal. Above 1.00 lean, below rich.",
            "Coolant temperature. Past 110 C it says HOT.",
            "Oil temperature.",
        };
        for (std::size_t i = 0; i < readouts_.size() && i < std::size(kReadoutHelp); ++i)
            readouts_[i]->setHelp(kReadoutHelp[i]);
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
            controls_.clutch = held ? v : -1.0f;   // PanelControls::clutch (card B, 920397d)
            report();
            if (services_.clutch)                  // the earlier route, while a shell still serves it
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
        {
            p.panel->attach(*host());
            if (p.rows)   // Rows made before the host (the launch workspace's) need it too (J2's finding)
                p.rows->attach(*host());
        }
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
            tableFrom_ = tableField_ = -1;   // another workspace's pages
            card_->close();
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
        {
            if (p.rows)
            {
                p.rows->pointerExit();
                p.rows->closeGestures();
            }
            if (p.panel)
            {
                p.panel->pointerExit();
                p.panel->closeGestures();
            }
        }
        // The panes stop pointing at the pages before they go.
        panes_->set({ { &current(), stageRect() } });
        pages_.clear();
        page_ = 0;
        const Workspace& ws = current();
        inspectorKeySeen_ = ws.inspectorKey();
        const std::string& sel = selection_.path();
        inspectorHead_ = sel.empty() ? std::string(ws.title()) : upper(sel);
        for (char& ch : inspectorHead_)
            if (ch == '.')
                ch = ' ';
        if (!unitsPage_)
        {
            if (const std::string gui = ws.inspectorPage(); !gui.empty())
                pages_.push_back({ inspectorHead_, gui, nullptr, nullptr });
            else
            {
                std::vector<int> fields = sel.empty() ? std::vector<int>{} : fieldsUnder(doc_, sel);
                if (!fields.empty())
                {
                    const auto split = splitPages(doc_, fields, 60);
                    for (std::size_t k = 0; k < split.size(); ++k)
                        pages_.push_back({ split.size() > 1 ? std::to_string(k + 1) : inspectorHead_,
                                           pageGui(doc_, split[k], {}), nullptr, nullptr });
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
                        const std::vector<std::string> omits = ws.sectionOmits(sec);
                        for (const int f : sectionFields(doc_, sec))
                            if (std::find(omits.begin(), omits.end(), doc_.info(f).path) == omits.end())
                                (doc_.info(f).table() ? tables : scalars).push_back(f);
                        const auto split = scalars.empty() ? std::vector<std::vector<int>>{} : splitPages(doc_, scalars, 60);
                        const std::vector<std::string> notes = ws.sectionNotes(sec);
                        for (std::size_t k = 0; k < split.size(); ++k)
                            pages_.push_back({ upper(sec) + (split.size() > 1 ? " " + std::to_string(k + 1) : std::string()),
                                               withNotes(pageGui(doc_, split[k], {}), k == 0 ? notes : std::vector<std::string>{}),
                                               nullptr, nullptr });
                        for (const int f : tables)
                        {
                            const FieldInfo& info = doc_.info(f);
                            pages_.push_back({ upper(info.label.empty() ? info.path : info.label), pageGui(doc_, { f }, {}), nullptr, nullptr });
                        }
                    }
                }
            }
            if (pages_.empty())
                unitsPage_ = true;   // nothing of the Spec here: the settings
        }
        if (unitsPage_)
        {
            pages_.clear();
            inspectorHead_ = "SETTINGS";
        }
        const funkgui::Rect room = inspectorRect();
        const auto makePanel = [&](Page& page) {
            GuiPanel::Options o;
            o.width = width();
            o.height = height();
            o.minWidth = kMinW;
            o.minHeight = kMinH;
            o.name = "app";
            o.dimAtDefault = false;   // a value is what a person reads, at its default or not
            o.tabWraps = false;       // Tab goes on from the page's last control to the frame's
            o.insetLeft = static_cast<int>(room.x) + 4;
            o.insetTop = static_cast<int>(room.y) + 2;
            o.insetRight = std::max(0, width() - static_cast<int>(room.right()) + 4);
            o.insetBottom = std::max(0, height() - static_cast<int>(room.bottom()));
            page.panel = std::make_unique<FieldPage>(doc_, units_, telemetry_, Sources{ { std::string(ws.id()) + ".gui", page.gui } },
                                                     std::move(o));
            if (host() != nullptr)
                page.panel->attach(*host());
        };
        for (Page& page : pages_)
            makePanel(page);
        // Rows (J2's RowsPage) over every page that is not a table alone; a table inside a workspace's own page gets a
        // page of its own (the generated pages already give each table one), and the filter reaches every page.
        for (Page& page : pages_)
            page.rows = RowsPage::over(*page.panel, doc_, units_);
        const auto heldAlone = [this](int field) {
            for (const Page& p : pages_)
                if (!p.rows && p.panel)
                {
                    const std::vector<int> fs = fieldsOf(outlineOf(*p.panel, doc_));
                    if (std::find(fs.begin(), fs.end(), field) != fs.end())
                        return true;
                }
            return false;
        };
        std::vector<RowsPage::Table> missing;
        for (const Page& page : pages_)
            if (page.rows)
                for (const RowsPage::Table& t : page.rows->tables())
                    if (!t.fields.empty() && !heldAlone(t.fields.front())
                        && std::none_of(missing.begin(), missing.end(), [&t](const RowsPage::Table& m) { return m.fields == t.fields; }))
                        missing.push_back(t);
        for (const RowsPage::Table& t : missing)
        {
            pages_.push_back({ upper(t.title), pageGui(doc_, t.fields, {}), nullptr, nullptr });
            makePanel(pages_.back());
        }
        std::vector<int> searchable;
        for (const Page& page : pages_)
            if (page.rows)
                searchable.insert(searchable.end(), page.rows->fields().begin(), page.rows->fields().end());
        for (Page& page : pages_)
            if (page.rows)
            {
                page.rows->setSearchable(searchable);
                page.rows->place(width(), height(), room);
                page.rows->setOnTable([this](const RowsPage::Table& t) { openTable(t); });
                if (host() != nullptr)
                    page.rows->attach(*host());
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

    RowsPage* AppFrame::inspectorRows() const
    {
        return page_ >= 0 && page_ < inspectorPageCount() ? pages_[static_cast<std::size_t>(page_)].rows.get() : nullptr;
    }

    funkgui::Rect AppFrame::inspectorValueRect(const std::string& path) const
    {
        if (const RowsPage* rows = inspectorRows())
        {
            funkgui::Rect row, value;
            const int f = doc_.find(path);
            return f >= 0 && rows->rows().fieldRect(f, row, value) ? value : funkgui::Rect{};
        }
        if (FieldPage* page = inspector())
            return page->controlRect(path);
        return {};
    }

    funkgui::Panel* AppFrame::inspectorPane() const
    {
        if (RowsPage* rows = inspectorRows())
            return rows;
        return inspector();
    }

    void AppFrame::openTable(const RowsPage::Table& t)
    {
        if (t.fields.empty())
            return;
        for (int p = 0; p < inspectorPageCount(); ++p)
        {
            const Page& page = pages_[static_cast<std::size_t>(p)];
            if (page.rows || !page.panel)
                continue;
            const std::vector<int> fs = fieldsOf(outlineOf(*page.panel, doc_));
            if (std::find(fs.begin(), fs.end(), t.fields.front()) != fs.end())
            {
                const int from = page_;
                showInspectorPage(p);
                tableFrom_ = from;
                tableField_ = t.fields.front();
                if (auto* table = dynamic_cast<MapEdit*>(tableItem()))
                    table->focusFirstCell();   // a controller's first press lands on a cell
                return;
            }
        }
    }

    void AppFrame::showInspectorPage(int p)
    {
        if (p < 0 || p >= inspectorPageCount())
            return;
        if (FieldPage* was = inspector(); was != nullptr && p != page_)
        {
            was->pointerExit();
            was->closeGestures();
            if (RowsPage* rows = inspectorRows())
            {
                rows->pointerExit();
                rows->closeGestures();
            }
        }
        page_ = p;
        arrange();
    }

    std::string AppFrame::desktopLine() const
    {
        if (!services_.desktopAllowed || !services_.allowDesktop)
            return {};
        if (!desktopWhy_.empty())
            return "Not allowed: " + desktopWhy_;
        if (!services_.desktopAllowed())
            return "Off: the app listens on nothing.";
        // The status (Session::statusNote) without the heading's words: "connected, 3 changes", "working: set_fields".
        std::string status = services_.desktopStatus ? services_.desktopStatus() : std::string();
        if (status.rfind("Claude Desktop ", 0) == 0)
            status = status.substr(15);
        if (status.empty())
            return "Allowed";
        status[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(status[0])));
        return status;
    }

    std::string AppFrame::clutchLine() const
    {
        if (!services_.clutchMode || !services_.setClutchMode || !services_.autoClutchSays)
            return {};
        return services_.autoClutchSays();
    }

    int AppFrame::clutchLit() const
    {
        return services_.clutchMode ? services_.clutchMode() : -1;
    }

    std::string AppFrame::wheelLine() const
    {
        if (!services_.wheelOn || !services_.setWheelOn || !services_.wheelStatus)
            return {};
        const std::string status = services_.wheelStatus();
        if (!status.empty())
            return status;
        return services_.wheelOn() ? std::string("On: no wheel connected.") : std::string("Off: the wheel is not read.");
    }

    void AppFrame::setDesktop(bool on)
    {
        if (!services_.allowDesktop)
            return;
        std::string why;
        if (services_.allowDesktop(on, why))
            desktopWhy_.clear();
        else
            desktopWhy_ = why.empty() ? std::string(on ? "it could not be allowed" : "it could not be turned off") : why;
        dirty_ = true;
    }

    Widget& AppFrame::mountChat(std::unique_ptr<Widget> chat)
    {
        Widget& w = *chat;
        chat_ = &w;
        chat_->setPlace({}, false);
        chatSlot_->set(std::move(chat));
        arrange();
        return w;
    }

    std::future<bool> AppFrame::ask(std::string question, std::string yes, std::string no,
                                    std::shared_ptr<const std::atomic<bool>> withdrawn)
    {
        auto answer = std::make_shared<std::promise<bool>>();
        std::future<bool> f = answer->get_future();
        {
            const std::lock_guard<std::mutex> lock(askMutex_);
            asks_.push_back({ std::move(question), std::move(yes), std::move(no), std::move(answer), std::move(withdrawn) });
        }
        return f;
    }

    void AppFrame::nextQuestion()
    {
        // The open question withdrawn by its asker: closed as a no would close it, laid out once below (by the next
        // question opening, or by this close when none follows).
        bool withdrew = false;
        if (askOpen() && openWithdrawn_ && openWithdrawn_->load(std::memory_order_acquire))
        {
            withdrawing_ = true;
            askDialog_->answer(false);
            withdrawing_ = false;
            withdrew = true;
        }
        const auto settle = [&] {
            if (!withdrew)
                return;
            arrange();
            setFocus(-1);
            dirty_ = true;
        };
        if (askOpen() || keyDialogOpen())
        {
            settle();
            return;
        }
        // The next question not withdrawn; one withdrawn before its turn is never shown, its future answered no.
        Question q;
        bool have = false;
        {
            const std::lock_guard<std::mutex> lock(askMutex_);
            while (!asks_.empty() && !have)
            {
                Question front = std::move(asks_.front());
                asks_.pop_front();
                if (front.isWithdrawn())
                    front.answer->set_value(false);
                else
                {
                    q = std::move(front);
                    have = true;
                }
            }
        }
        if (!have)
        {
            settle();
            return;
        }
        openWithdrawn_ = q.withdrawn;
        askDialog_->open(std::move(q.text), std::move(q.yes), std::move(q.no), [this, answer = q.answer](bool yes) {
            answer->set_value(yes);
            openWithdrawn_.reset();
            if (closing_ || withdrawing_)
                return;
            arrange();
            setFocus(-1);
            dirty_ = true;
        });
        arrange();
        setFocus(askIndex_);
        dirty_ = true;
    }

    void AppFrame::openChat(bool on)
    {
        if (chat_ == nullptr || on == chatOpen_)
            return;
        chatOpen_ = on;
        if (on)
            card_->close();
        if (FieldPage* page = inspector())
        {
            page->pointerExit();
            page->closeGestures();
        }
        if (RowsPage* rows = inspectorRows())
        {
            rows->pointerExit();
            rows->closeGestures();
        }
        setPanes();
        arrange();
        bump();
        dirty_ = true;
    }

    funkgui::Rect AppFrame::chatRect() const
    {
        if (!chatOpen_)
            return {};
        const funkgui::Rect s = stageRect();
        const float top = s.y + kHeadH + 2.0f;
        return { s.right() + 4.0f, top, static_cast<float>(width()) - s.right() - 8.0f, s.bottom() - top - 4.0f };
    }

    void AppFrame::openKeyDialog()
    {
        if (keyDialog_ == nullptr || chat_ == nullptr || askOpen())
            return;
        keyDialog_->open();
        arrange();
        setFocus(keyDialogIndex_);
        dirty_ = true;
    }

    void AppFrame::showUnits(bool on)
    {
        if (on)
        {
            settingsScroll_ = 0.0f;   // the settings open at their top
            chatOpen_ = false;
            card_->close();
        }
        if (on == unitsPage_)
        {
            arrange();
            return;
        }
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
        std::vector<Panes::Pane> panes{ { &current(), current().stage().isEmpty() ? stageRect() : current().stage() } };
        if (funkgui::Panel* pane = inspectorPane(); pane != nullptr && !chatOpen_)
            panes.push_back({ pane, inspectorRect() });
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

    funkgui::Rect AppFrame::stageFor(int w, int h)
    {
        w = std::max(w, kMinW);
        h = std::max(h, kMinH);
        const float x = w >= 1280 ? kRailW : kRailNarrowW, inspector = w >= 1400 ? kInspectorW + 20.0f : kInspectorW;
        return { x, kTopH, std::max(100.0f, static_cast<float>(w) - x - inspector), static_cast<float>(h) - kTopH - kCockpitH };
    }

    funkgui::Rect AppFrame::stageRect() const { return stageFor(width(), height()); }

    void AppFrame::setLibrary(garage::Library* library) { ctx_->library = library; }

    funkgui::Rect AppFrame::ribbonRect() const
    {
        if (!current().ribbon() || unitsPage_ || chatOpen_)   // the settings or the chat take the whole column
            return {};
        const funkgui::Rect s = stageRect();
        const float h = folded_ ? kRibbonFoldedH : kRibbonH;
        return { s.right(), s.bottom() - h, static_cast<float>(width()) - s.right(), h };
    }

    int AppFrame::railFocus() const
    {
        for (std::size_t i = 0; i < rail_.size(); ++i)
            if (focus() >= 0 && &const_cast<AppFrame*>(this)->item(focus()) == rail_[i])
                return static_cast<int>(i);
        return -1;
    }

    int AppFrame::tabRows() const noexcept
    {
        const int n = unitsPage_ || chatOpen_ ? 0 : std::min(inspectorPageCount(), kMaxTabs);
        // More than three: rows of four at most, two at least, so each title has room (eleven: three rows).
        return n <= 1 ? 0 : n <= 3 ? 1 : std::max(2, (n + 3) / 4);
    }

    funkgui::Rect AppFrame::inspectorRect() const
    {
        const funkgui::Rect s = stageRect();
        const float top = s.y + kHeadH + 28.0f * static_cast<float>(tabRows()) + (tabRows() > 0 ? 2.0f : 0.0f);
        const funkgui::Rect rib = ribbonRect();
        const float bottom = rib.isEmpty() ? s.bottom() : rib.y;
        return { s.right(), top, static_cast<float>(width()) - s.right(), std::max(40.0f, bottom - top) };
    }

    funkgui::Rect AppFrame::inspectorTabRect(int p) const
    {
        const funkgui::Rect s = stageRect();
        const int n = std::min(inspectorPageCount(), kMaxTabs), rows = std::max(1, tabRows());
        const int perRow = (n + rows - 1) / rows, row = p / std::max(1, perRow), col = p % std::max(1, perRow);
        const float x0 = s.right() + 10.0f, w = (static_cast<float>(width()) - x0 - 10.0f) / static_cast<float>(std::max(1, perRow));
        return { x0 + w * static_cast<float>(col), s.y + kHeadH + 2.0f + 28.0f * static_cast<float>(row), w - 4.0f, 24.0f };
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
            rail_[i]->setLeftRoom(narrow ? 0.0f : 24.0f);   // narrow: the short words alone; wide: a pictogram and the word
        }
        railMarks_->setPlace({ 0.0f, kTopH, railWidth(), static_cast<float>(height()) - kTopH - kCockpitH }, true);
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
        if (chat_ != nullptr)
            right(chatButton_, 52.0f, 9.0f, 30.0f);
        else
            chatButton_->setPlace({}, false);
        x -= 8.0f;
        {
            const double db = ctx_->live.get("sound.limiter");
            const bool limiting = std::isfinite(db) && std::fabs(db) > 0.05;
            right(level_, 120.0f, 6.0f, limiting ? 26.0f : 36.0f);
            limiter_->setPlace({ x + 8.0f, 33.0f, 120.0f, 13.0f }, limiting);
        }
        x -= 18.0f;                                       // a clear gap: each label with its own number and bar
        right(load_, 150.0f, 5.0f, 40.0f);
        const float left = railWidth() + 16.0f;
        float statusW = 0.0f;
        bakeShown_ = !bakeWords().empty();
        staleShown_ = !bakeStaleLine(bakeStatus()).empty();
        if (load_->verdict() < 1)
        {
            // The build does not fit (tight or over): the sentence and QUALITY in the status line's place, the name and
            // the engine's sentence keep at least 170 px.
            x -= 6.0f;
            right(adviceButton_, 72.0f, 10.0f, 28.0f);
            statusW = std::clamp(x - left - 170.0f, 120.0f, 330.0f);
            advice_->setPlace({ x - statusW, 6.0f, statusW, 34.0f }, true);
            status_->setPlace({}, false);
            staleLine_->setPlace({}, false);
        }
        else
        {
            adviceButton_->setPlace({}, false);
            advice_->setPlace({}, false);
            statusW = std::clamp(x - left - 420.0f, 0.0f, 300.0f);
            if (bakeShown_)   // the bake's words have room at the smallest window (the name keeps 170 px)
                statusW = std::max(statusW, std::min(200.0f, x - left - 182.0f));
            // across an edit (old tables in play, or a reason after the count: card C's options 1 and 2), up to 420 px, the
            // name keeping 200
            if (staleShown_ || bakeWords().find("   ") != std::string::npos)
                statusW = std::max(statusW, std::min(420.0f, x - left - 200.0f));
            // two lines while old tables play (the bake, then which tables), else the one line where it always was
            const bool two = staleShown_;
            status_->setPlace({ x - statusW, two ? 9.0f : 18.0f, statusW, 14.0f }, statusW > 60.0f);
            staleLine_->setPlace({ x - statusW, 27.0f, statusW, 14.0f }, two && statusW > 60.0f);
        }
        const float room = std::max(80.0f, x - statusW - 12.0f - left);
        name_->setPlace({ left, 6.0f, room, 22.0f }, true);
        sentence_->setPlace({ left, 28.0f, room, 16.0f }, true);

        // The workspace and its stage.
        const funkgui::Rect stage = stageRect();
        for (auto& ws : workspaces_)
            ws->resize(width(), height());
        // The first launch's card: a strip across the stage's top, the workspace laid out in the rest, so it covers
        // nothing live (the top bar, the cockpit and the inspector are untouched).
        funkgui::Rect wsStage = stage;
        if (!tryFirstNotes_.empty())
        {
            const float cw = stage.w - 24.0f;
            const float ch = std::min(tryFirst_->heightFor(cw), stage.h * 0.3f);
            const funkgui::Rect card{ stage.x + 12.0f, stage.y + 10.0f, cw, ch };
            tryFirst_->setPlace(card, true);
            tryFirstClose_->setPlace({ card.right() - 12.0f - 64.0f, card.y + 8.0f, 64.0f, 26.0f }, true);
            const float taken = card.bottom() - stage.y + 2.0f;
            wsStage = { stage.x, stage.y + taken, stage.w, stage.h - taken };
        }
        else
        {
            tryFirst_->setPlace({}, false);
            tryFirstClose_->setPlace({}, false);
        }
        current().place(wsStage);
        // The inspector: its header's ALL, the tabs, the page or the units, the ribbon.
        const funkgui::Rect insp{ stage.right(), stage.y, w - stage.right(), stage.h };
        clear_->setPlace({ insp.right() - 58.0f, insp.y + 6.0f, 48.0f, 22.0f }, !selection_.empty() && !unitsPage_ && !chatOpen_);
        const int n = unitsPage_ || chatOpen_ ? 0 : inspectorPageCount();
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
        card_->place(stage);
        // The chat in the column, KEY in its header; the key dialog over the whole window while it is open.
        if (chat_ != nullptr)
            chat_->setPlace(chatRect(), chatOpen_);
        keyButton_->setPlace({ insp.right() - 58.0f, insp.y + 6.0f, 48.0f, 22.0f }, chatOpen_);
        keyDialog_->setPlace({ 0.0f, 0.0f, w, static_cast<float>(height()) }, keyDialog_->isOpen());
        askDialog_->setPlace({ 0.0f, 0.0f, w, static_cast<float>(height()) }, askDialog_->isOpen());

        // The cockpit, left to right: START, the mode, the rpm bar; from the right: the pedals, the readouts.
        const funkgui::Rect c = cockpitRect();
        start_->setPlace(startRect(), true);
        modes_[0]->setPlace({ 96.0f, c.y + 20.0f, 76.0f, 40.0f }, true);
        for (int p = 0; p < 3; ++p)
            pedals_[static_cast<std::size_t>(p)]->setPlace(pedalRect(p), true);
        static constexpr float kWidths[6] = { 40.0f, 84.0f, 92.0f, 62.0f, 84.0f, 84.0f };   // gear speed boost lambda coolant oil
        float rx = pedalRect(0).x - 16.0f;
        const bool roomy = w >= 1240.0f;
        const bool truck = engineBrakeState(*ctx_).shown;
        for (int k = 5; k >= 0; --k)
        {
            // Narrow: no oil; a truck's brake lamps narrow take lambda's room too (a diesel runs lean at every load).
            const bool on = (k != 5 || roomy) && (k != 3 || !truck || roomy);
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
        // A truck's engine brake, left of the readouts (it takes its room from the rpm bar).
        if (truck)
        {
            rx -= 140.0f;
            brake_->setPlace({ rx, c.y + 16.0f, 140.0f, 46.0f }, true);
            rx -= 8.0f;
        }
        else
            brake_->setPlace({}, false);
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
            if (p.rows)
                p.rows->place(width(), height(), room);
            p.panel->setInsets(static_cast<int>(room.x) + 4, static_cast<int>(room.y) + 2,
                               std::max(0, width() - static_cast<int>(room.right()) + 4),
                               std::max(0, height() - static_cast<int>(room.bottom())));
        }
        // The settings page: laid out at its scroll, again once if the scroll had to come back within its most.
        if (!(unitsPage_ && !chatOpen_) || width() != settingsSize_.first || height() != settingsSize_.second)
            settingsScroll_ = 0.0f;   // reopened, or the window changed: at the top
        settingsSize_ = { width(), height() };
        placeSettings(room);
        if (settingsScroll_ > settingsMax_)
        {
            settingsScroll_ = settingsMax_;
            placeSettings(room);
        }
    }

    void AppFrame::placeSettings(const funkgui::Rect& room)
    {
        // The settings page, top down: AUDIO (the device in a line, its menu), THEME, UNITS (two columns of steppers),
        // KEYS (drawn: drawOver). Where its rows do not fit the room (the smallest window with every row served), it
        // scrolls (settingsScroll_, U1, 27 Nov, the lead's ruling): every row in its own place and size, the page moved
        // up; a row not wholly in the room is not shown, so nothing is drawn over the cockpit or the ribbon.
        const bool on = unitsPage_ && !chatOpen_;
        const float x = room.x + 12.0f, w = room.w - 24.0f;
        float y = room.y + 8.0f - settingsScroll_;
        settingsHeads_.clear();
        settingsHeads_.push_back({ "AUDIO", y });
        // The device's line, its menu's button at the line's right (one row).
        const float menuW = services_.audioMenu ? 112.0f : 0.0f;
        audioLine_->setPlace({ x, y + 16.0f, w - menuW - (menuW > 0.0f ? 8.0f : 0.0f), 18.0f }, on);
        audioMenu_->setPlace({ x + w - menuW, y + 12.0f, menuW, 26.0f }, on && static_cast<bool>(services_.audioMenu));
        y += 44.0f;
        // THEME and QUALITY side by side, the quality's sentence under them.
        const float half = (w - 12.0f) / 2.0f, bw = (half - 6.0f) / 2.0f;
        settingsHeads_.push_back({ "THEME", y });
        graphite_->setPlace({ x, y + 16.0f, bw, 26.0f }, on);
        paper_->setPlace({ x + bw + 6.0f, y + 16.0f, bw, 26.0f }, on);
        settingsHeads_.push_back({ "QUALITY", y });
        settingsHeadX_ = x + half + 12.0f;
        full_->setPlace({ settingsHeadX_, y + 16.0f, bw, 26.0f }, on);
        economy_->setPlace({ settingsHeadX_ + bw + 6.0f, y + 16.0f, bw, 26.0f }, on);
        qualityNoteY_ = y + 48.0f;
        y += 54.0f;   // (what ECONOMY does is its hover help; without the shell's quality the buttons are disabled)
        // ENGINE START: warm (the default) or cold, from the next launch; only where the shell serves it.
        const bool start = on && services_.coldStart && services_.setColdStart;
        // One row: its heading and "from the next launch" at the left, the buttons under QUALITY's at the right.
        warmStart_->setPlace({ settingsHeadX_, y + 2.0f, bw, 26.0f }, start);
        coldStart_->setPlace({ settingsHeadX_ + bw + 6.0f, y + 2.0f, bw, 26.0f }, start);
        startNoteY_ = -1.0f;
        if (start)
        {
            settingsHeads_.push_back({ "ENGINE START", y + 2.0f });
            startNoteY_ = y + 16.0f;
            y += 38.0f;
        }
        // CLAUDE DESKTOP: Off or Allow (card A's link; default off: the app listens on nothing), its line under them;
        // only where the shell serves it.
        const bool desk = on && services_.desktopAllowed && services_.allowDesktop;
        desktopOff_->setPlace({ settingsHeadX_, y + 2.0f, bw, 26.0f }, desk);
        desktopOn_->setPlace({ settingsHeadX_ + bw + 6.0f, y + 2.0f, bw, 26.0f }, desk);
        desktopNoteY_ = -1.0f;
        if (desk)
        {
            settingsHeads_.push_back({ "CLAUDE DESKTOP", y + 2.0f });
            desktopNoteY_ = y + 16.0f;   // its line, wrapped to two at most under the heading (left of the buttons)
            y += 42.0f;
        }
        // WHEEL: Off or On (card B's G29; only where the shell reads a wheel), its status under the heading.
        const bool wheel = on && services_.wheelOn && services_.setWheelOn && services_.wheelStatus;
        wheelOff_->setPlace({ settingsHeadX_, y + 2.0f, bw, 26.0f }, wheel);
        wheelOn_->setPlace({ settingsHeadX_ + bw + 6.0f, y + 2.0f, bw, 26.0f }, wheel);
        wheelNoteY_ = -1.0f;
        if (wheel)
        {
            settingsHeads_.push_back({ "WHEEL", y + 2.0f });
            wheelNoteY_ = y + 16.0f;
            y += 42.0f;
        }
        // CLUTCH: three ways (card B's), across the whole column under its heading (three do not fit beside it), what
        // it means on the gearbox loaded under them, two lines at most.
        const bool clutch = on && services_.clutchMode && services_.setClutchMode && services_.autoClutchSays;
        const float cw = (w - 12.0f) / 3.0f;
        clutchNoteY_ = -1.0f;
        if (clutch)
            y += 4.0f;   // air under the row above (WHEEL's line, wrapped to two, ends where this heading would begin)
        for (std::size_t m = 0; m < clutch_.size(); ++m)
            clutch_[m]->setPlace({ x + static_cast<float>(m) * (cw + 6.0f), y + 16.0f, cw, 26.0f }, clutch);
        if (clutch)
        {
            settingsHeads_.push_back({ "CLUTCH", y });
            clutchNoteY_ = y + 47.0f;
            y += 78.0f;
        }
        settingsHeads_.push_back({ "UNITS", y });
        // One column: each quantity's name in full at the left, its stepper at the right.
        // 25 px apart, closer (to 23, the stepper's own height) where the room is short.
        const float n = static_cast<float>(std::max<std::size_t>(1, unitSteppers_.size()));
        // Down to 21 (the steppers as tall) where even 23 would run into the cockpit; where even 21 would, the page scrolls
        // and the steppers keep their own pitch, 25 (the room reckoned at no scroll, so the pitch does not move with it).
        const float y0 = y + settingsScroll_;   // UNITS' heading at no scroll
        const bool overflows = y0 + 16.0f + 21.0f * n > room.bottom() + 0.5f;   // even at 21 the last runs past
        const float pitch = overflows ? 25.0f : std::clamp((room.bottom() - y0 - 16.0f - 4.0f) / n, 21.0f, 25.0f);
        for (std::size_t i = 0; i < unitSteppers_.size(); ++i)
            unitSteppers_[i]->setPlace({ x, y + 16.0f + static_cast<float>(i) * pitch, w, std::min(23.0f, pitch) }, on);
        y += 16.0f + pitch * static_cast<float>(unitSteppers_.size()) + 8.0f;
        keysY_ = y + 16.0f;
        if (keysY_ + 14.0f <= room.bottom())   // a heading only with a key under it
            settingsHeads_.push_back({ "KEYS", y });
        // How far it scrolls: the last stepper's foot past the room's, with a margin (0: it fits, and nothing here changes
        // the page).
        settingsMax_ = on && overflows ? std::max(0.0f, y0 + 16.0f + pitch * (n - 1.0f) + std::min(23.0f, pitch) + 6.0f - room.bottom())
                                       : 0.0f;
        // A widget not wholly in the room is not shown (kept with its place, for Tab: AppFrame::key).
        settingsClipped_.clear();
        if (on)
            for (Widget* wd : settingsWidgets())
                if (wd->shown() && (wd->bounds().y < room.y - 0.5f || wd->bounds().bottom() > room.bottom() + 0.5f))
                {
                    settingsClipped_.push_back({ wd, wd->bounds() });
                    wd->setPlace(wd->bounds(), false);
                }
    }

    std::vector<Widget*> AppFrame::settingsWidgets() const
    {
        std::vector<Widget*> out = { audioLine_, audioMenu_, graphite_, paper_, full_, economy_, warmStart_, coldStart_,
                                     desktopOff_, desktopOn_, wheelOff_, wheelOn_ };
        out.insert(out.end(), clutch_.begin(), clutch_.end());
        for (Stepper* st : unitSteppers_)
            out.push_back(st);
        return out;
    }

    bool AppFrame::bringSettingsIn(const Item& it)
    {
        const auto c = std::find_if(settingsClipped_.begin(), settingsClipped_.end(),
                                    [&](const auto& w) { return static_cast<const Item*>(w.first) == &it; });
        if (c == settingsClipped_.end())
            return false;
        const funkgui::Rect room = inspectorRect(), r = c->second;
        scrollSettings(r.y < room.y ? settingsScroll_ - (room.y - r.y) - 4.0f : settingsScroll_ + (r.bottom() - room.bottom()) + 4.0f);
        return true;
    }

    bool AppFrame::scrollSettings(float to)
    {
        const float before = settingsScroll_;
        settingsScroll_ = std::clamp(to, 0.0f, settingsMax_);
        if (settingsScroll_ == before)
            return false;
        arrange();
        dirty_ = true;
        return true;
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
        c.text(chatOpen_ ? "CHAT" : head.c_str(), stage.right() + 14.0f, stage.y + 11.0f, look::kTitle,
               selection_.empty() || unitsPage_ || chatOpen_ ? p.text : p.selected);
        if (unitsPage_ && !chatOpen_)
        {
            const funkgui::Rect room = inspectorRect();
            // what is drawn here is drawn only wholly in the room (the page may be scrolled: placeSettings)
            const auto inRoom = [&](float ty, float th2) { return ty >= room.y - 0.5f && ty + th2 <= room.bottom() + 0.5f; };
            for (const auto& [heading, hy] : settingsHeads_)
                if (inRoom(hy, 12.0f))
                    c.text(heading.c_str(), heading == "QUALITY" ? settingsHeadX_ : room.x + 12.0f, hy, look::kCaps, p.label);
            if (startNoteY_ >= 0.0f && inRoom(startNoteY_, 12.0f))
                c.text("from the next launch", room.x + 12.0f, startNoteY_, look::kUnit, p.label);
            // A row's line, two lines at most in the left column (the buttons at the right), the second cut to fit.
            const auto rowNote = [&](const std::string& line, float ny, funkgui::Col ink, float lw = -1.0f) {
                if (!inRoom(ny, 26.0f))
                    return;
                if (lw < 0.0f)
                    lw = settingsHeadX_ - room.x - 12.0f - 8.0f;
                std::string first;
                std::size_t at = 0;
                while (at < line.size())
                {
                    const std::size_t sp = std::min(line.find(' ', at), line.size());
                    const std::string tried = line.substr(0, sp);
                    if (!first.empty() && c.textWidth(tried.c_str(), look::kUnit) > lw)
                        break;
                    first = tried;
                    at = sp + 1;
                }
                const std::string rest = at < line.size() ? line.substr(at) : std::string();
                c.text(first.c_str(), room.x + 12.0f, ny, look::kUnit, ink);
                if (!rest.empty())
                    c.text(fitText(c, rest, look::kUnit, lw).c_str(), room.x + 12.0f, ny + 13.0f, look::kUnit, ink);
            };
            if (desktopNoteY_ >= 0.0f)
                rowNote(desktopLine(), desktopNoteY_, desktopWhy_.empty() ? p.text : Look::of(th).error);
            if (wheelNoteY_ >= 0.0f)
                rowNote(wheelLine(), wheelNoteY_, p.text);
            if (clutchNoteY_ >= 0.0f)   // the whole column's width: nothing beside it
                rowNote(clutchLine(), clutchNoteY_, p.text, room.w - 24.0f);
            // The keys of docs/controls.md (the shell reads them; they work in every workspace but a text field).
            static const char* const kKeys[][2] = {
                { "Up / W", "throttle" }, { "Down / S", "brake" }, { "Left Shift", "clutch" }, { "Return", "start (hold: stop)" },
                { "E / Page Up", "shift up" }, { "Q / Page Down", "shift down" }, { "N, 1 to 6", "neutral, a gear" },
                { "F9", "the audio device" }, { "Cmd/Ctrl Z", "undo (Shift: redo)" },
            };
            float ky = keysY_;
            for (const auto& k : kKeys)
            {
                if (ky + 14.0f > room.bottom())
                    break;
                if (ky < room.y)
                {
                    ky += 15.0f;
                    continue;
                }
                c.text(k[0], room.x + 12.0f, ky, look::kUnit, p.value);
                c.text(k[1], room.x + 130.0f, ky, look::kUnit, p.label);
                ky += 15.0f;
            }
            // While it scrolls: a thin bar at the column's right edge, its thumb where the page is.
            if (settingsMax_ > 0.0f)
            {
                const float tx = room.right() - 5.0f, ty0 = room.y + 4.0f, th0 = room.h - 8.0f;
                const float thumb = std::max(24.0f, th0 * room.h / (room.h + settingsMax_));
                c.rrect(tx, ty0, 3.0f, th0, 1.5f, p.edge);
                c.rrect(tx, ty0 + (th0 - thumb) * (settingsScroll_ / settingsMax_), 3.0f, thumb, 1.5f, p.label);
            }
        }
        // The ribbon's title row.
        if (const funkgui::Rect rib = ribbonRect(); !rib.isEmpty())
        {
            c.rrect(rib.x, rib.y, rib.w, rib.h, 0.0f, bar);
            c.hairlineH(rib.x, c.snapY(rib.y), rib.w, p.edge);
            c.text("DYNO", rib.x + 14.0f, rib.y + 10.0f, look::kCaps, p.text);
            // The caption, or the predicted curve's knock-limited span while it has one (its points ringed on the plot).
            const std::string knock = ribbon_->knockText(true);
            // While the engine plays on tables from before an edit the curve is the new build on the old calibration
            // (C: the predictor runs on the build as it stands) and moves when the bake lands: said in the caption's
            // place, quietly; the knock rings stay on the plot and in the cursor's words.
            if (const std::string old = ribbonStaleCaption(); !old.empty())
                c.text(fitText(c, old, look::kCaps, rib.w - 56.0f - 70.0f).c_str(), rib.x + 56.0f, rib.y + 10.0f, look::kCaps, p.label);
            else if (knock.empty())
                c.text("PREDICTED, THE LAST CURVE GHOSTED", rib.x + 56.0f, rib.y + 10.0f, look::kCaps, p.label);
            else
                c.text(fitText(c, knock, look::kCaps, folded_ ? 200.0f : rib.w - 56.0f - 70.0f).c_str(), rib.x + 56.0f, rib.y + 10.0f, look::kCaps, p.warning);
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
        nextQuestion();   // the assistant's next question, once nothing else is asking
        dyno_->tick(dt);
#ifdef TEE_UI_HAS_LIBRARY
        if (ctx_->library != nullptr)
            ctx_->library->tick(dt);
#endif
        // The units changed: the pages again, with the new units' texts.
        if (units_.revision() != unitsSeen_ && !(gestures() != nullptr && gestures()->dragging()))
        {
            unitsSeen_ = units_.revision();
            makeInspector();
            arrange();
        }
        // What decides the shown workspace's pages moved (Workspace::inspectorKey: VALVES' model): the pages again.
        if (current().inspectorKey() != inspectorKeySeen_ && !(gestures() != nullptr && gestures()->dragging()))
        {
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
        card_->update(selection_.path(), inspectorRows() != nullptr ? &inspectorRows()->rows().outline() : nullptr, stageRect());
        modes_[0]->setTitle(kModes[std::clamp(services_.mode ? services_.mode() : localMode_, 0, 2)]);
        // While the engine's program compiles START says so (drawn disabled: Button::setEnabled).
        start_->setTitle(!services_.engineReady || services_.engineReady() ? "Start" : "Compiling");
        if (const double db = ctx_->live.get("sound.limiter"); (std::isfinite(db) && std::fabs(db) > 0.05) != limiter_->shown())
            arrange();   // the limiter's bar comes or goes
        // The first launch's card: its text while the shell has one; closed for good once the engine has been started
        // and stopped (the owner has done what it says).
        if (!tryFirstClosed_)
        {
            const std::string notes = services_.tryFirst ? services_.tryFirst() : std::string();
            if (notes != tryFirstNotes_)
            {
                tryFirstNotes_ = notes;
                tryFirst_->setText(services_.buildName ? services_.buildName() : std::string(), notes);
                arrange();
            }
            const double st = ctx_->live.get(telemetry::kState);
            const bool running = std::isfinite(st) && st >= 0.5 && st < 2.5;
            if (running && !tryFirstNotes_.empty())
                sawRunning_ = true;   // a start counts while the card is up
            else if (sawRunning_ && std::isfinite(st) && !tryFirstNotes_.empty())
                closeTryFirst();
        }
        if (bakeWords().empty() == bakeShown_ || bakeStaleLine(bakeStatus()).empty() == staleShown_)
            arrange();   // a bake began or ended, or old tables came into play or left: the status line's room
        if ((load_->verdict() < 1) != advice_->shown())
            arrange();   // the build stopped fitting, or fits again: the sentence comes or goes by itself
        if (engineBrakeState(*ctx_).shown != brake_->shown())
            arrange();   // a truck loaded, or one put away: its brake's lamps
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
        k.mix(width()).mix(height()).mix(shown_).mix(page_).mix(unitsPage_ ? 1 : 0).mix(folded_ ? 1 : 0).mix(chatOpen_ ? 1 : 0);
        if (desktopNoteY_ >= 0.0f)
            k.mix(std::hash<std::string>{}(desktopLine()));
        if (wheelNoteY_ >= 0.0f)
            k.mix(std::hash<std::string>{}(wheelLine()));
        if (clutchNoteY_ >= 0.0f)
            k.mix(std::hash<std::string>{}(clutchLine())).mix(static_cast<std::uint64_t>(clutchLit() + 1));
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
            k.mix(page->tooltip().drawKey());   // hover help shown or gone
        if (const RowsPage* rows = inspectorRows())
            k.mix(rows->drawKey());             // Rows: values, typing, scroll, hover help
        k.mix(tooltip().drawKey()).mix(current().tooltip().drawKey());
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
        // A press on the stage (not on the card itself) may open or close the card on the picture.
        const funkgui::Point p{ e.x, e.y };
        const std::string before = selection_.path();
        const bool onStage = !askOpen() && !keyDialogOpen() && stageRect().contains(p)
                             && !(card_->isOpen() && cardItem_->bounds().contains(p));
        pointerHeld_ = true;
        Board::pointerDown(e);
        if (onStage)
            card_->afterPress(before, selection_.path(), selection_.hovered(), p);
    }

    void AppFrame::pointerDrag(const funkgui::PointerEvent& e)
    {
        dirty_ = true;
        Board::pointerDrag(e);
    }

    void AppFrame::pointerUp(const funkgui::PointerEvent& e)
    {
        dirty_ = true;
        pointerHeld_ = false;
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
        // The settings page while it scrolls: by the inspector's own rule (FieldInspector::wheel), a stepper's value box
        // takes the wheel (its unit) and the page does not move; anywhere else the page scrolls, as every list does
        // (funkgui::scroll::wheelPx with the Rows' row height: the trackpad's small deltas as mended on 26 Nov).
        if (unitsPage_ && !chatOpen_ && settingsMax_ > 0.0f && e.dy != 0.0f && inspectorRect().contains({ e.x, e.y }))
        {
            // While a press holds, the page does not move under it (the press ends as a click where it began).
            if (pointerHeld_)
                return true;
            for (const Stepper* st : unitSteppers_)
                if (st->shown() && st->bounds().contains({ e.x, e.y }) && e.x >= st->minusRect().x && e.x <= st->plusRect().right())
                    return Board::wheel(e);
            scrollSettings(settingsScroll_ - funkgui::scroll::wheelPx(e.dy, e.smooth, FieldInspector::kWheelRowH));
            return true;
        }
        return Board::wheel(e);
    }

    bool AppFrame::key(const funkgui::KeyEvent& e)
    {
        dirty_ = true;
        // The key dialog takes every key while it is open; an item that takes all keys (the chat's message box) has
        // them first, the frame's undo only what it leaves.
        if (askOpen())
            return askDialog_->type(e);
        if (keyDialogOpen())
            return keyDialog_->type(e);
        if (focus() >= 0 && item(focus()).takesAllKeys() && Board::key(e))
            return true;
        if (e.key == funkgui::Key::escape && card_->isOpen())
        {
            const bool taken = Board::key(e);
            if (!taken)
            {
                card_->close();
                return true;
            }
            return taken;
        }
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
        // On the settings page while it scrolls, a stop out of the room is brought in whole before a key reaches it: Tab's
        // next stop (so the board's own Tab finds it shown: a list shows its focused row), or the stop focused when the
        // page was wheeled away from it (it keeps the focus, unseen, and no unit is changed out of sight).
        if (unitsPage_ && !chatOpen_ && !settingsClipped_.empty())
        {
            if (e.key == funkgui::Key::tab)
            {
                const int step = e.mods.shift ? -1 : 1, n = count();
                for (int k = 1; k <= n; ++k)
                {
                    const int at = focus() < 0 ? (step > 0 ? k - 1 : n - k) : focus() + step * k;
                    if (at < 0 || at >= n)
                        break;
                    const Item& it = item(at);
                    if (it.shown() && it.focusable())
                        break;   // a stop in sight: the board's Tab takes it
                    if (bringSettingsIn(it))
                        break;
                }
            }
            else if (focus() >= 0)
                bringSettingsIn(item(focus()));
        }
        return Board::key(e);
    }

    Item* AppFrame::tableItem() const
    {
        if (inspectorRows() != nullptr || page_ < 0 || page_ >= inspectorPageCount())
            return nullptr;
        const FieldPage* page = pages_[static_cast<std::size_t>(page_)].panel.get();
        if (page == nullptr)
            return nullptr;
        for (const std::unique_ptr<Item>& it : page->items())
            if (it->shown() && dynamic_cast<MapEdit*>(it.get()) != nullptr)
                return it.get();
        return nullptr;
    }

    bool AppFrame::edit(EditCommand c, bool fine)
    {
        if (askOpen() || keyDialogOpen())
            return false;
        dirty_ = true;
        // The item in focus first: the frame's own (a button: none takes one today), then the inspector's.
        if (focus() >= 0 && item(focus()).edit(c, fine))
            return true;
        if (!unitsPage_ && !chatOpen_)
        {
            if (RowsPage* rows = inspectorRows())
            {
                if (rows->focus() < 0)
                    rows->focusFirst();   // the first press lands on a row
                if (rows->edit(c, fine))
                    return true;
            }
            else if (Item* table = tableItem(); table != nullptr && table->edit(c, fine))
                return true;
        }
        switch (c)
        {
            case EditCommand::workspacePrev:
            case EditCommand::workspaceNext:
            {
                const int n = workspaceCount();
                if (n <= 1)
                    return false;
                show((shown_ + (c == EditCommand::workspaceNext ? 1 : n - 1)) % n);   // the rail's order, wrapping
                if (RowsPage* rows = inspectorRows())
                    rows->focusFirst();
                return true;
            }
            case EditCommand::pagePrev:
            case EditCommand::pageNext:
            {
                if (unitsPage_ || chatOpen_)
                    return false;
                const int to = page_ + (c == EditCommand::pageNext ? 1 : -1);
                if (to < 0 || to >= inspectorPageCount())
                    return false;   // the ends: stopping, as the tabs do
                showInspectorPage(to);
                if (RowsPage* rows = inspectorRows())
                    rows->focusFirst();
                else if (auto* table = dynamic_cast<MapEdit*>(tableItem()))
                    table->focusFirstCell();
                return true;
            }
            case EditCommand::undo:
                if (!services_.undo)
                    return false;
                services_.undo();
                return true;
            case EditCommand::redo:
                if (!services_.redo)
                    return false;
                services_.redo();
                return true;
            case EditCommand::back:
                if (card_->isOpen())
                {
                    card_->close();
                    return true;
                }
                if (chatOpen_)
                {
                    openChat(false);
                    return true;
                }
                if (unitsPage_)
                {
                    showUnits(false);
                    return true;
                }
                if (inspectorRows() == nullptr && tableFrom_ >= 0 && tableFrom_ < inspectorPageCount()
                    && pages_[static_cast<std::size_t>(tableFrom_)].rows)
                {
                    // a table's page: back to the rows it was opened from, its row in focus
                    const int field = tableField_;
                    showInspectorPage(tableFrom_);
                    tableFrom_ = tableField_ = -1;
                    if (RowsPage* rows = inspectorRows(); rows != nullptr && !rows->focusTable(field))
                        rows->focusFirst();
                    return true;
                }
                return false;
            default: return false;
        }
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
                    if (page.items()[static_cast<std::size_t>(wa.item)]->shown())   // a table, or an array's elements
                        for (const int f : fieldsOfWidget(doc_, page.spec().gui.nodes[static_cast<std::size_t>(wa.node)]))
                            out.insert(f);
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
