// tests/ui/views/FrameTests.cpp: the application's frame (package U1, grown from J2's; its tests' intent kept): the ten
// workspaces on the rail, every field that is not hidden reachable from an inspector, selection, a value set through
// the inspector and undone, the cockpit (start, pedals, rpm bar), the garage's stand-in and the units, the dyno ribbon,
// sizes from 1100 x 700 up, frames drawn only when something changed, goldens of every workspace and the contact sheet
// out/ui/sheet.frame.png.
#include "ViewKit.h"

#include "app/AppFrame.h"
#include "kit/widgets/Tables.h"

#include <funkgui/text/FontService.h>
#include <funkgui/text/TextFit.h>

#include <algorithm>
#include <cctype>

namespace
{
    using namespace tee::test::views;
    using tee::ui::AppFrame;

    // The frame over the demo, with services that do what the shell's would.
    struct FrameRig
    {
        tee::ui::DemoRig demo;
        std::string name = "Demo V8";
        std::vector<std::string> saved = { "Hot cam", "Demo V8", "Twin turbo" };
        std::string loaded;
        float clutch = -1.0f;
        float peak = 0.31f;     // the simulator's peak share of a core (FrameServices::simLoadPeak)
        long dropouts = 0;
        std::string notes;      // the first launch's TRY FIRST card's text (FrameServices::tryFirst; empty: none)
        int tryFirstDone = 0;
        bool cold = false;      // FrameServices::coldStart (the settings' ENGINE START)
        std::string spark;      // FrameServices::sparkMap: the calibration bake's state ("" none)
        int mode = 0;
        // Card A's Session as the frame sees it (FrameServices::desktopAllowed ... removeKey), served when `assistant`:
        // the switch (a refusal's words in `refuseWith`), the status line, the key store (what was saved, never shown).
        bool desktop = false;
        std::string refuseWith, status, savedKey, refuseKey;
        int saves = 0;
        std::unique_ptr<AppFrame> frame;
        std::unique_ptr<funkgui::HeadlessHost> host;

        // `own`: the frame over U1's own slots: every workspace but DRIVE a placeholder under an id no package registers
        // ("own-air"), so these tests and goldens do not move when another package's stage lands or changes. Not own:
        // the real rail, whatever is registered (the contact sheet's pictures).
        bool own;
        std::string idOf(const std::string& id) const { return own && id != "drive" ? "own-" + id : id; }

        explicit FrameRig(int w = 1280, int h = 800, float dpi = 2.0f, double t = 4.6, bool ownSlots = true, bool assistant = false)
            : demo(t, !ownSlots), own(ownSlots)
        {
            demo.engine.hold(true);   // every picture at the same moment of the script
            tee::ui::FrameServices s;
            s.app = demo.app;
            s.buildName = [this] { return name; };
            s.rename = [this](const std::string& n) { name = n; return true; };
            s.undo = [this] { demo.doc.undo(); };
            s.redo = [this] { demo.doc.redo(); };
            s.builds = [this] { return saved; };
            s.load = [this](const std::string& n, std::string&) { loaded = n; name = n; return true; };
            s.save = [this](const std::string& n, std::string& error) {
                if (n.empty())
                    return error = "no name", false;
                if (std::find(saved.begin(), saved.end(), n) == saved.end())
                    saved.push_back(n);
                return true;
            };
            s.unsaved = [this] { return demo.doc.canUndo(); };
            s.simLoad = [] { return 0.23f; };
            s.simLoadPeak = [this] { return peak; };
            s.dropouts = [this] { return dropouts; };
            s.tryFirst = [this] { return notes; };
            s.coldStart = [this] { return cold; };
            s.sparkMap = [this] { return spark; };
            s.setColdStart = [this](bool c) { cold = c; };
            s.tryFirstDone = [this] { ++tryFirstDone; notes.clear(); };
            s.mode = [this] { return mode; };
            s.setMode = [this](int m) { mode = m; };
            s.clutch = [this](float v) { clutch = v; };
            if (assistant)
            {
                s.desktopAllowed = [this] { return desktop; };
                s.allowDesktop = [this](bool on, std::string& why) {
                    if (on && !refuseWith.empty())
                        return why = refuseWith, false;
                    desktop = on;
                    return true;
                };
                s.desktopStatus = [this] { return desktop ? status : std::string(); };
                s.saveKey = [this](std::string typed, std::string& why) {
                    ++saves;
                    if (!refuseKey.empty())
                        return why = refuseKey, false;
                    savedKey = std::move(typed);
                    return true;
                };
                s.keyMask = [this] {
                    return savedKey.empty() ? std::string("none saved") : "saved, ending ..." + savedKey.substr(savedKey.size() - 4);
                };
                s.removeKey = [this](std::string&) {
                    savedKey.clear();
                    return true;
                };
            }
            std::vector<tee::ui::WorkspaceSlot> slots = tee::ui::defaultSlots();
            for (tee::ui::WorkspaceSlot& sl : slots)
                sl.id = idOf(sl.id);
            frame = std::make_unique<AppFrame>(demo.doc, demo.units, demo.engine, std::move(s), &demo.engine, &demo.engine,
                                               std::move(slots), w, h);
            host = std::make_unique<funkgui::HeadlessHost>(*frame, 0, dpi);
            host->tick(60);
        }
        void showWorkspace(const std::string& id)
        {
            const int i = frame->find(idOf(id));
            if (i < 0)
                return failAt(__FILE__, __LINE__, "no workspace " + id);
            const funkgui::Rect r = frame->railRect(i);
            host->click(r.centreX(), r.centreY());
            host->tick(60);
        }
        double value(const char* path) const { return demo.doc.get(demo.doc.find(path)); }
        // A field's control in the inspector moved up by `amount` px of its own gesture: Rows' value box dragged right
        // (it scrubs), a FieldPage's knob dragged up. False: the field is not on the page in view.
        bool dragField(const char* path, float amount, int steps = 12)
        {
            const funkgui::Rect r = frame->inspectorValueRect(path);
            if (r.w <= 0.0f)
                return false;
            if (frame->inspectorRows() != nullptr)
                host->drag(r.centreX(), r.centreY(), r.centreX() + amount, r.centreY(), steps);
            else
                host->drag(r.centreX(), r.centreY(), r.centreX(), r.centreY() - amount, steps);
            return true;
        }
        std::string a11yValue(const std::string& title) const
        {
            for (const funkgui::A11yItem& it : host->accessibility())
                if (it.title == title)
                    return it.value;
            return "(none)";
        }
    };

    const char* const kIds[11] = { "garage", "engine", "valves", "air", "exhaust", "tune", "car", "dyno", "drive", "listen", "scope" };

    // The ten workspaces in the rail's order; the rail shows each; each inspector page reads without a note; every
    // field that is not hidden is on some page.
    VIEW_TEST(testFrameWorkspaces, "frame.workspaces")
    {
        // The real rail: the ten in order, whatever is registered; the pages of U1's (the placeholders', DRIVE's) read
        // without a note. A registered workspace's own page is its package's to check (ViewKit's inspectorNotes).
        {
            FrameRig real(1280, 800, 2.0f, 4.6, false);
            AppFrame& f = *real.frame;
            CHECK(f.workspaceCount() >= 11);
            for (int i = 0; i < 11 && i < f.workspaceCount(); ++i)
            {
                CHECK(f.slot(i).id == kIds[i] && std::string(f.workspace(i).id()) == kIds[i]);
                real.showWorkspace(kIds[i]);
                CHECK(f.shown() == i);
                CHECK_MSG(f.inspectorPageCount() > 0 || f.unitsShown(), kIds[i]);
                if (!f.placeholder(i) && std::string(kIds[i]) != "drive")
                    continue;
                for (int p = 0; p < f.inspectorPageCount(); ++p)
                {
                    f.showInspectorPage(p);
                    CHECK_MSG(f.inspector() != nullptr && f.inspector()->notes().empty(),
                              std::string(kIds[i]) + ": " + (f.inspector() != nullptr ? f.inspector()->notes() : "no page"));
                }
            }
            // Every field a person sets is on some page, over the real rail too.
            const std::vector<int> shown = f.fieldsShown();
            for (int i = 0; i < real.demo.doc.count(); ++i)
            {
                const bool there = std::find(shown.begin(), shown.end(), i) != shown.end();
                const tee::ui::FieldInfo& info = real.demo.doc.info(i);
                CHECK_MSG(there != info.hidden, info.path + (info.hidden ? " is hidden and shown" : " is on no page"));
            }
        }
        FrameRig rig;
        AppFrame& f = *rig.frame;
        // AIR's inspector: its two sections as tabs.
        rig.showWorkspace("air");
        CHECK(f.inspectorPageCount() == 3 && f.inspectorTitle(0) == "INTAKE" && f.inspectorTitle(1) == "BOOST" && f.inspectorTitle(2) == "DUTY");
        const funkgui::Rect tab = f.inspectorTabRect(1);
        rig.host->click(tab.centreX(), tab.centreY());
        CHECK(f.inspectorPageIndex() == 1 && f.inspector()->controlRect("boost.level").w > 0.0f);
        const funkgui::Rect r = f.inspector()->controlRect("boost.level");
        const funkgui::Rect room = f.inspectorRect();
        CHECK(r.x >= room.x && r.right() <= room.right() + 0.5f && r.y >= room.y);
        return 0;
    }

    // A path selected (as a stage's click does) shows its fields alone; ALL goes back to the workspace's pages; showing
    // another workspace clears it.
    VIEW_TEST(testFrameSelect, "frame.select")
    {
        FrameRig rig;
        AppFrame& f = *rig.frame;
        rig.showWorkspace("air");
        f.select("boost");
        rig.host->tick(2);
        // its fields on one page (Rows), each of its tables on a page of its own after it
        CHECK(f.inspectorPageCount() >= 1 && f.inspector() != nullptr && f.inspectorRows() != nullptr);
        CHECK(f.inspector()->params().findParam("boost.level") >= 0);
        CHECK(f.inspector()->params().findParam("intake.plenum") < 0);
        f.select("exhaust.crossover");               // a single field
        rig.host->tick(2);
        CHECK(f.inspector()->params().count() == 1);
        f.select("nothing.here");                    // no fields: the workspace's pages
        rig.host->tick(2);
        CHECK(f.inspectorPageCount() == 3);
        f.select("boost");
        rig.host->tick(2);
        // ALL, in the inspector's header.
        bool clicked = false;
        for (const funkgui::A11yItem& it : rig.host->accessibility())
            if (it.title == "All" && !clicked)
            {
                rig.host->click(it.bounds.centreX(), it.bounds.centreY());
                clicked = true;
            }
        rig.host->tick(2);
        CHECK(clicked && f.selection().empty() && f.inspectorPageCount() == 3);
        f.select("boost");
        rig.showWorkspace("exhaust");
        CHECK(f.selection().empty());
        return 0;
    }

    // A knob in the inspector turns its field; Cmd-Z takes the drag back as one step; a menu writes its choice.
    VIEW_TEST(testFrameSet, "frame.set")
    {
        FrameRig rig;
        AppFrame& f = *rig.frame;
        rig.showWorkspace("air");
        f.showInspectorPage(1);
        rig.host->tick(2);
        CHECK(rig.dragField("boost.level", 80.0f));
        CHECK(rig.value("boost.level") > 80e3);
        typeKeys(*rig.host, "cmd+z");
        CHECK_NEAR(rig.value("boost.level"), 80e3, 1e-6);
        typeKeys(*rig.host, "cmd+shift+z");
        CHECK(rig.value("boost.level") > 80e3);
        const funkgui::Rect m = f.inspectorValueRect("boost.kind");
        rig.host->click(m.centreX(), m.centreY());
        CHECK(rig.host->chooseMenuItem("TWIN TURBO") || rig.host->chooseMenuItem("Twin turbo"));
        rig.host->tick(2);
        CHECK(rig.value("boost.kind") == 2.0);
        // The build reads unsaved in the top bar (its name with a star).
        rig.host->tick(2);
        bool star = false;
        for (const funkgui::A11yItem& it : rig.host->accessibility())
            star = star || it.value == "Demo V8 *" || it.title == "Demo V8 *";
        CHECK(star);
        return 0;
    }

    // START held is a PanelControls with startHeld; the pedals drag and spring back; the clutch goes through its own
    // service; the mode cycles; the readouts say the telemetry, a dash where a name is absent.
    VIEW_TEST(testFrameCockpit, "frame.cockpit")
    {
        FrameRig rig;
        AppFrame& f = *rig.frame;
        const funkgui::Rect s = f.startRect();
        rig.host->press(s.centreX(), s.centreY());
        CHECK(rig.demo.asked.startHeld);
        rig.host->release();
        CHECK(!rig.demo.asked.startHeld);
        const funkgui::Rect t = f.pedal(0).track();
        rig.host->press(t.centreX(), t.y + t.h * 0.25f);
        CHECK_NEAR(rig.demo.asked.throttle, 0.75, 0.02);
        rig.host->moveTo(t.centreX(), t.bottom() - 1.0f, 4);
        CHECK(rig.demo.asked.throttle < 0.05f && rig.demo.asked.throttle >= 0.0f);
        rig.host->release();
        CHECK(rig.demo.asked.throttle < 0.0f);
        const funkgui::Rect b = f.pedal(1).track();
        rig.host->drag(b.centreX(), b.bottom() - 2.0f, b.centreX(), b.y + 2.0f);
        CHECK(rig.demo.asked.brake < 0.0f);          // let go: back to the keys'
        const funkgui::Rect c = f.pedal(2).track();
        rig.host->press(c.centreX(), c.centreY());
        CHECK_NEAR(rig.clutch, 0.5, 0.03);
        rig.host->release();
        CHECK(rig.clutch < 0.0f);
        // START disabled while the engine is not ready (its program compiling): no press reaches the shell.
        {
            bool ready = false;
            FrameRig late;
            // A fresh rig with engineReady false: its START takes no press.
            tee::ui::FrameServices sv;
            sv.app = late.demo.app;
            sv.engineReady = [&ready] { return ready; };
            AppFrame gated(late.demo.doc, late.demo.units, late.demo.engine, sv, &late.demo.engine, &late.demo.engine);
            funkgui::HeadlessHost h(gated, 0, 2.0f);
            h.tick(5);
            const int before = late.demo.askedCount;
            bool compiling = false, modeOff = false;
            for (const funkgui::A11yItem& it : h.accessibility())
            {
                compiling = compiling || (it.title == "Compiling" && !it.enabled);
                modeOff = modeOff || (it.title == "BENCH" && !it.enabled);
            }
            CHECK(compiling && modeOff);
            const funkgui::Rect st = gated.startRect();
            h.press(st.centreX(), st.centreY());
            h.release();
            CHECK(late.demo.askedCount == before);
            ready = true;
            h.tick(2);
            bool back = false;
            for (const funkgui::A11yItem& it : h.accessibility())
                back = back || (it.title == "Start" && it.enabled);
            CHECK(back);
            h.press(st.centreX(), st.centreY());
            CHECK(late.demo.asked.startHeld);
            h.release();
        }
        // The mode.
        bool cycled = false;
        for (const funkgui::A11yItem& it : rig.host->accessibility())
            if (it.title == "BENCH" && !cycled)
            {
                rig.host->click(it.bounds.centreX(), it.bounds.centreY());
                cycled = true;
            }
        CHECK(cycled && rig.mode == 1);
        // The readouts.
        rig.host->tick(5);
        CHECK(rig.a11yValue("Gear") == "3");
        CHECK(rig.a11yValue("Lambda") == "-");       // not in the program yet
        rig.demo.engine.set("ecu.lambda", 0.86);
        rig.host->tick(2);
        CHECK(rig.a11yValue("Lambda") == "0.86");
        // The rpm bar: the shift lights come on in turn; the limiter's word.
        rig.demo.engine.set("engine.speed", 6700.0 * 3.14159265358979 / 30.0);
        rig.host->tick(30);
        CHECK(f.rpmBar().lightsLit() == 5);
        rig.demo.engine.set("engine.speed", 5800.0 * 3.14159265358979 / 30.0);
        rig.host->tick(30);
        CHECK(f.rpmBar().lightsLit() == 2);
        CHECK(rig.demo.askedCount > 0);
        return 0;
    }

    // The garage's stand-in: the builds listed, one loaded, the name changed and saved; the units page: pressure in psi,
    // and the inspector's knob says so.
    // A truck (spec.car.kind "Truck", card D2's brakes): the cockpit shows the engine brake's lamps, lit only while
    // the brake acts, its stage as segments (tel.ecu.brakeCylinders over the cylinders); a car shows none.
    VIEW_TEST(testFrameTruck, "frame.truck")
    {
        FrameRig rig(1100, 700);
        const auto brakeItem = [&]() -> const funkgui::A11yItem* {
            static funkgui::A11yItem found;
            for (const funkgui::A11yItem& it : rig.host->accessibility())
                if (it.title == "Engine brake" && it.visible)
                    return &(found = it);
            return nullptr;
        };
        CHECK(brakeItem() == nullptr);
        CHECK(!tee::ui::engineBrakeState(rig.demo.ctx).shown);
        rig.demo.doc.set(rig.demo.doc.find("car.kind"), 1.0);   // Truck
        rig.host->tick(4);
        const funkgui::A11yItem* lamp = brakeItem();
        CHECK(lamp != nullptr && lamp->value.rfind("off", 0) == 0);
        rig.demo.engine.set("ecu.brakeCylinders", 6.0);           // six of the demo's eight: two thirds, MEDIUM
        rig.demo.engine.set("ecu.backPressure", 180e3);
        rig.host->tick(4);
        const tee::ui::EngineBrakeState s = tee::ui::engineBrakeState(rig.demo.ctx);
        CHECK(s.shown && s.stage == 2 && s.exhaustOn);
        lamp = brakeItem();
        CHECK_MSG(lamp != nullptr && lamp->value == "on, MEDIUM; exhaust brake on", lamp ? lamp->value : std::string("none"));
        CHECK(rig.frame->rpmBar().bounds().w >= 240.0f);   // lambda gives its room at this width: the rpm bar stays readable
        picture("frame.truck", *rig.host);
        rig.demo.engine.set("ecu.brakeCylinders", 0.0);
        rig.demo.engine.forget("ecu.backPressure");
        rig.host->tick(4);
        CHECK(tee::ui::engineBrakeState(rig.demo.ctx).stage == 0 && !tee::ui::engineBrakeState(rig.demo.ctx).exhaustOn);
        rig.demo.doc.set(rig.demo.doc.find("car.kind"), 0.0);
        rig.host->tick(4);
        CHECK(brakeItem() == nullptr);
        return 0;
    }

    // The top bar's load: the peak share of a core with its word (ok, tight, over), the bar's ink by it; the sentence
    // and QUALITY while the build does not fit, gone by itself three seconds after the load falls; never a switch of
    // the quality. Goldens at 40, 75 and 95 % of a core, densities 2 and 1.
    VIEW_TEST(testFrameLoad, "frame.load")
    {
        const auto textOf = [](FrameRig& rig, const std::string& title) -> std::string {
            for (const funkgui::A11yItem& it : rig.host->accessibility())
                if (it.title == title && it.visible)
                    return it.value;
            return "(none)";
        };
        for (const float dpi : { 2.0f, 1.0f })
        {
            const char* sfx = dpi < 1.5f ? ".dpi1" : "";
            int qualitySet = -1;
            FrameRig rig(1280, 800, dpi);
            rig.showWorkspace("drive");
            struct Case { float peak; const char* name; const char* value; bool advice; };
            const Case cases[] = { { 0.40f, "40", "40 % of a core, ok", false },
                                   { 0.75f, "75", "75 % of a core, tight", true },
                                   { 0.95f, "95", "95 % of a core, over", true } };
            for (const Case& k : cases)
            {
                rig.peak = k.peak;
                rig.host->tick(30);
                CHECK_MSG(textOf(rig, "Load") == k.value, textOf(rig, "Load"));
                const std::string advice = textOf(rig, "Load warning");
                CHECK_MSG(k.advice == (advice == "This build is heavy for this machine at Full: Economy halves it"), advice);
                golden(std::string("frame.load.") + k.name + sfx, *rig.host);
            }
            // Dropouts, when the host counts any.
            rig.dropouts = 3;
            rig.host->tick(2);
            CHECK_MSG(textOf(rig, "Load") == "95 % of a core, over, dropouts: 3", textOf(rig, "Load"));
            if (dpi > 1.5f)
                picture("frame.load.dropouts", *rig.host);
            // QUALITY shows the settings; nothing switched the quality.
            CHECK(!rig.frame->settingsShown());
            for (const funkgui::A11yItem& it : rig.host->accessibility())
                if (it.title == "Quality" && it.visible)
                    rig.host->click(it.bounds.centreX(), it.bounds.centreY());
            CHECK(rig.frame->settingsShown());
            CHECK(qualitySet == -1);
            // The load falls: the sentence stays while the peak is held (3 s), then goes by itself.
            rig.peak = 0.30f;
            rig.dropouts = 0;
            rig.host->tick(60);
            CHECK(textOf(rig, "Load warning") != "(none)");
            rig.host->tick(150);
            CHECK_MSG(textOf(rig, "Load warning") == "(none)", textOf(rig, "Load warning"));
            CHECK(textOf(rig, "Load") == "30 % of a core, ok");
        }
        // The verdict from the peak held three seconds: 89 % is tight, 90 over; a spike holds it, then it lets go.
        {
            tee::ui::LoadWatch w;
            w.step(0.1f, 0.89f);
            CHECK(w.verdict() == 0);
            w.step(0.1f, 0.90f);
            CHECK(w.verdict() == -1);
            for (int i = 0; i < 25; ++i)
                w.step(0.1f, 0.2f);
            CHECK(w.verdict() == -1);   // 2.5 s on: still held
            for (int i = 0; i < 10; ++i)
                w.step(0.1f, 0.2f);
            CHECK(w.verdict() == 1 && std::fabs(w.held() - 0.2f) < 1e-6f);
        }
        return 0;
    }

    // A low redline (D's e0bcf8f: a governed diesel's stated limiter is its redline, about 2000 rpm): the rev bar's
    // scale follows it (to 2500, numerals in hundreds), the red zone a fifth of it, the shift lights' spacing shrunk
    // with it (none lit at idle); a petrol with a 4500 rpm limiter to 5000 in thousands. The 6800 rpm demo keeps 8000.
    VIEW_TEST(testFrameRedline, "frame.redline")
    {
        constexpr double kRpm = 3.14159265358979323846 / 30.0;
        CHECK(tee::ui::look::rpmScale(6800.0).top == 8000.0 && tee::ui::look::rpmScale(7000.0).top == 8000.0);
        CHECK(tee::ui::look::rpmScale(8500.0).top == 10000.0);
        FrameRig rig(1280, 800, 2.0f);
        CHECK(rig.frame->rpmBar().scaleTop() == 8000.0);
        struct Case { double red, idle, near; double top; const char* name; };
        const Case cases[] = { { 2014.0, 700.0, 1990.0, 2500.0, "diesel" }, { 4500.0, 850.0, 4450.0, 5000.0, "petrol4500" } };
        for (const Case& k : cases)
        {
            rig.demo.engine.set("engine.redline", k.red * kRpm);
            rig.demo.engine.set("engine.speed", k.idle * kRpm);
            rig.showWorkspace("drive");
            rig.host->tick(30);
            CHECK_MSG(rig.frame->rpmBar().scaleTop() == k.top, std::to_string(rig.frame->rpmBar().scaleTop()));
            CHECK_MSG(rig.frame->rpmBar().lightsLit() == 0, k.name);
            rig.demo.engine.set("engine.speed", k.near * kRpm);
            rig.host->tick(30);
            CHECK_MSG(rig.frame->rpmBar().lightsLit() >= 4, k.name);
            picture(std::string("frame.redline.") + k.name, *rig.host);
        }
        return 0;
    }

    // The first launch's TRY FIRST card: the build's name and its notes' opening paragraph wrapped to the card, over
    // the stage's top right, never over START or the load meter, read by the accessibility list; closed for good by
    // its Close button or once the engine has been started and stopped. At 1100 x 700 and 1600 x 1000, densities 2
    // and 1.
    VIEW_TEST(testFrameTryFirst, "frame.tryfirst")
    {
        const std::string notes =
            "Try first: turn the key, then floor it on the dyno and watch the turbo spool in AIR; then open EXHAUST and "
            "hear the headers change as you shorten them. The builds in GARAGE are yours to change and keep.\n\n"
            "About this build: a 2.0 L turbocharged four with every model on.";
        const auto overlap = [](const funkgui::Rect& a, const funkgui::Rect& b) {
            return a.x < b.right() && b.x < a.right() && a.y < b.bottom() && b.y < a.bottom();
        };
        const auto itemOf = [](FrameRig& rig, const std::string& title) -> funkgui::A11yItem {
            for (const funkgui::A11yItem& it : rig.host->accessibility())
                if (it.title == title && it.visible)
                    return it;
            return {};
        };
        struct Size { int w, h; const char* name; };
        for (const Size& z : { Size{ 1100, 700, "small" }, Size{ 1600, 1000, "large" } })
            for (const float dpi : { 2.0f, 1.0f })
            {
                FrameRig rig(z.w, z.h, dpi);
                rig.showWorkspace("drive");
                CHECK(itemOf(rig, "Try first").title.empty());   // no notes: no card
                rig.notes = notes;
                rig.host->tick(4);
                const funkgui::A11yItem card = itemOf(rig, "Try first");
                CHECK_MSG(card.value.rfind("Demo V8. Try first: turn the key", 0) == 0, card.value);
                CHECK(card.value.find("About this build") == std::string::npos);   // the opening paragraph only
                CHECK(!overlap(card.bounds, rig.frame->startRect()));
                CHECK(!overlap(card.bounds, itemOf(rig, "Load").bounds));
                CHECK(rig.frame->stageRect().contains({ card.bounds.x, card.bounds.y }));
                CHECK(card.bounds.bottom() <= rig.frame->stageRect().bottom());
                // It covers nothing live: the workspace is laid out under it, every control and gauge of the stage clear.
                const tee::ui::Workspace& ws = rig.frame->workspace(rig.frame->shown());
                CHECK(!overlap(card.bounds, ws.stage()) && ws.stage().y >= card.bounds.bottom());
                for (const funkgui::A11yItem& it : rig.host->accessibility())
                    if (it.visible && it.title != "Try first" && it.title != "Close" && rig.frame->stageRect().contains({ it.bounds.centreX(), it.bounds.centreY() }))
                        CHECK_MSG(!overlap(card.bounds, it.bounds), it.title);
                golden(std::string("frame.tryfirst.") + z.name + (dpi < 1.5f ? ".dpi1" : ""), *rig.host);
                // Close: gone for good, the shell told once.
                const funkgui::A11yItem close = itemOf(rig, "Close");
                rig.host->click(close.bounds.centreX(), close.bounds.centreY());
                rig.host->tick(2);
                CHECK(itemOf(rig, "Try first").title.empty() && rig.tryFirstDone == 1);
            }
        // Started and stopped: closed by itself.
        {
            FrameRig rig(1280, 800, 1.0f);
            rig.demo.engine.set("engine.state", 0.0);
            rig.notes = notes;
            rig.host->tick(4);
            CHECK(!itemOf(rig, "Try first").title.empty());
            rig.demo.engine.set("engine.state", 2.0);
            rig.host->tick(4);
            CHECK(!itemOf(rig, "Try first").title.empty());   // running: still there
            rig.demo.engine.set("engine.state", 0.0);
            rig.host->tick(4);
            CHECK(itemOf(rig, "Try first").title.empty() && rig.tryFirstDone == 1);
        }
        // A long opening is cut at six lines with an ellipsis.
        {
            std::string longer;
            for (int i = 0; i < 40; ++i)
                longer += "turn the key and listen ";
            const std::vector<std::string> lines = tee::ui::TryFirstCard::wrap(longer, 300.0f);
            CHECK(static_cast<int>(lines.size()) == tee::ui::TryFirstCard::kMaxLines);
            CHECK(lines.back().size() >= 3 && lines.back().compare(lines.back().size() - 3, 3, "...") == 0);
        }
        return 0;
    }

    // The calibration bake in the status line on every workspace (the lead's ruling, 01:33): its stage in words while it
    // works, at the smallest window too; nothing once baked.
    VIEW_TEST(testFrameBake, "frame.bake")
    {
        const auto said = [](FrameRig& rig, const std::string& words) {
            for (const funkgui::A11yItem& it : rig.host->accessibility())
                if (it.visible && (it.value.find(words) != std::string::npos || it.title.find(words) != std::string::npos))
                    return true;
            return false;
        };
        for (const float dpi : { 2.0f, 1.0f })
        {
            FrameRig rig(1100, 700, dpi);
            rig.showWorkspace("drive");
            rig.host->tick(2);
            CHECK(!said(rig, "Baking"));
            rig.spark = "baking cam";
            rig.host->tick(3);
            CHECK(said(rig, "Baking the cam"));
            rig.showWorkspace("dyno");
            CHECK(said(rig, "Baking the cam"));   // on every workspace
            rig.spark = "baking spark map again";
            rig.host->tick(3);
            CHECK(said(rig, "Baking the spark map again"));
            rig.spark = "baking cam 23 of 42";   // with card C's count
            rig.host->tick(3);
            CHECK(said(rig, "Baking the cam 23 of 42"));
            rig.spark = "checking spark map 3 of 9";   // card C's verified reuse: its own words, not "Baking"
            rig.host->tick(3);
            CHECK(said(rig, "Checking the calibration 3 of 9") && !said(rig, "Baking"));
            rig.spark = "checking calibration";       // no stage chosen yet
            rig.host->tick(3);
            CHECK(said(rig, "Checking the calibration"));
            rig.spark = "baking";
            rig.host->tick(3);
            CHECK(said(rig, "Baking the calibration"));
            rig.showWorkspace("drive");
            rig.spark = "baking spark map";
            rig.host->tick(3);
            golden(std::string("frame.bake") + (dpi < 1.5f ? ".dpi1" : ""), *rig.host);
            rig.spark = "baked";
            rig.host->tick(3);
            CHECK(!said(rig, "Baking"));
            rig.spark = "calibration checked";
            rig.host->tick(3);
            CHECK(!said(rig, "Baking") && !said(rig, "Checking"));
        }
        return 0;
    }

    // Hover help (the owner's ask, 25 Nov): a dial on an inspector page, after a short rest of the pointer, shows its
    // label, its field's help and its range and default, beside it (never over it) and inside the window; it goes when
    // the pointer leaves. The cockpit's START says what it does; a control the keyboard reaches by Tab shows its help
    // too. At the smallest window, densities 2 and 1.
    VIEW_TEST(testFrameHelp, "frame.help")
    {
        const auto overlap = [](const funkgui::Rect& a, const funkgui::Rect& b) {
            return a.x < b.right() && b.x < a.right() && a.y < b.bottom() && b.y < a.bottom();
        };
        const auto inside = [](const funkgui::Rect& r, float w, float h) {
            return r.x >= 0.0f && r.y >= 0.0f && r.right() <= w && r.bottom() <= h && r.w > 40.0f && r.h > 20.0f;
        };
        for (const float dpi : { 2.0f, 1.0f })
        {
            const char* sfx = dpi < 1.5f ? ".dpi1" : "";
            FrameRig rig(1100, 700, dpi);
            AppFrame& f = *rig.frame;
            rig.showWorkspace("engine");
            // The field in sight: on the page that holds it, Rows' filter brings it to the top.
            funkgui::Rect dial{};
            for (int p = 0; p < f.inspectorPageCount() && dial.isEmpty(); ++p)
            {
                f.showInspectorPage(p);
                rig.host->tick(2);
                dial = f.inspectorValueRect("engine.compression");
            }
            if (f.inspectorRows() != nullptr)
            {
                f.inspectorRows()->rows().setFilter("compression");
                rig.host->tick(5);
                dial = f.inspectorValueRect("engine.compression");
            }
            CHECK(!dial.isEmpty() && dial.bottom() <= f.inspectorRect().bottom());
            rig.host->moveTo(dial.centreX(), dial.centreY(), 2);
            rig.host->tick(10);
            const auto paneTip = [&f]() -> const tee::ui::Tooltip& {
                return f.inspectorRows() != nullptr ? f.inspectorRows()->tooltip() : f.inspector()->tooltip();
            };
            CHECK(!paneTip().shown());   // not before the rest
            rig.host->tick(30);
            const tee::ui::Tooltip& tip = paneTip();
            CHECK(tip.shown());
            const std::string text = tip.text();
            CHECK_MSG(text.find("Compression") != std::string::npos && text.find("volume at the bottom") != std::string::npos
                          && text.find("default 10") != std::string::npos && text.find("7") != std::string::npos,
                      text);
            CHECK(!overlap(tip.box(), dial) && inside(tip.box(), 1100.0f, 700.0f));
            golden(std::string("frame.help.dial") + sfx, *rig.host);
            rig.host->moveTo(f.stageRect().centreX(), f.stageRect().centreY(), 2);
            rig.host->tick(3);
            CHECK(!paneTip().shown());   // gone with the pointer
            // The cockpit's START.
            const funkgui::Rect st = f.startRect();
            rig.host->moveTo(st.centreX(), st.centreY(), 2);
            rig.host->tick(40);
            CHECK_MSG(f.tooltip().shown() && f.tooltip().text().find("starter") != std::string::npos, f.tooltip().text());
            CHECK(!overlap(f.tooltip().box(), st) && inside(f.tooltip().box(), 1100.0f, 700.0f));
            if (dpi > 1.5f)
                picture("frame.help.start", *rig.host);
        }
        // The keyboard: Tab onto the inspector's controls (Rows: Tab onto the rows, Down onto a row); the one focused
        // says its help after a shorter rest.
        {
            FrameRig rig(1280, 800, 1.0f);
            AppFrame& f = *rig.frame;
            rig.showWorkspace("engine");
            rig.host->moveTo(2.0f, 2.0f, 1);
            bool shown = false;
            for (int k = 0; k < 60 && !shown; ++k)
            {
                typeKeys(*rig.host, "tab");
                // Rows take the focus as one widget: Down picks its first row (J2's FieldInspector)
                if (f.inspectorRows() != nullptr && f.inspectorRows()->focus() >= 0 && f.inspectorRows()->rows().focusedField() < 0)
                    typeKeys(*rig.host, "down");
                rig.host->tick(30);
                shown = f.inspectorRows() != nullptr ? f.inspectorRows()->tooltip().shown()
                                                     : f.inspector() != nullptr && f.inspector()->tooltip().shown();
            }
            CHECK_MSG(shown, "no control reached by Tab showed its help");
        }
        return 0;
    }

    VIEW_TEST(testFrameGarage, "frame.garage")
    {
        FrameRig rig;
        AppFrame& f = *rig.frame;
        rig.showWorkspace("garage");
        CHECK(f.unitsShown());                       // the garage has no Spec section: its inspector is the units
        const auto find = [&](const char* title) {
            for (const funkgui::A11yItem& it : rig.host->accessibility())
                if (it.title == title)
                    return it.bounds;
            failAt(__FILE__, __LINE__, std::string("nothing called ") + title);
            return funkgui::Rect{};
        };
        const funkgui::Rect list = find("Builds");
        rig.host->click(list.centreX(), list.y + 11.0f + tee::ui::List::kRowH * 2.0f);   // the third row
        const funkgui::Rect load = find("Load");
        rig.host->click(load.centreX(), load.centreY());
        CHECK(rig.loaded == "Twin turbo");
        const funkgui::Rect name = find("Name");
        rig.host->click(name.right() - 20.0f, name.centreY());
        typeKeys(*rig.host, "M y space b u i l d enter");
        CHECK(rig.name == "My build");
        const funkgui::Rect save = find("Save as");
        rig.host->click(save.centreX(), save.centreY());
        CHECK(rig.saved.size() == 4);
        rig.demo.units.choose(tee::ui::Quantity::pressure, "psi");
        rig.host->tick(2);
        rig.showWorkspace("air");
        f.showInspectorPage(1);
        const int k = f.inspector()->params().findParam("boost.level");
        CHECK(k >= 0 && f.inspector()->params().info(k).unit == "psi");
        // The settings page from the top bar, anywhere (pictured at the smallest window).
        {
            FrameRig small(1100, 700, 1.0f);
            small.showWorkspace("valves");
            small.frame->showSettings(true);
            small.host->tick(5);
            CHECK(small.frame->settingsShown() && small.frame->ribbonRect().isEmpty());
            // Every unit's name whole at the smallest window: its label fits before its stepper's buttons.
            for (const tee::ui::Stepper* st : small.frame->unitSteppers())
            {
                const float room = st->minusRect().x - st->bounds().x - 4.0f;
                const float need = funkgui::text::width(funkgui::FontService::get().atlas(), tee::ui::upper(st->title()).c_str(),
                                                        tee::ui::kWidgetLabel);
                CHECK_MSG(st->shown() && need <= room && st->bounds().bottom() <= small.frame->inspectorRect().bottom(),
                          st->title() + ": needs " + std::to_string(need) + " px, has " + std::to_string(room));
            }
            // ENGINE START: warm lit; Cold chosen, lit, and told to the shell (taken from the next launch).
            {
                const auto itemOf = [&](const std::string& title) {
                    for (const funkgui::A11yItem& it : small.host->accessibility())
                        if (it.title == title && it.visible)
                            return it;
                    return funkgui::A11yItem{};
                };
                const funkgui::A11yItem warm = itemOf("Warm"), cold = itemOf("Cold");
                CHECK(!warm.title.empty() && !cold.title.empty() && !small.cold);
                small.host->click(cold.bounds.centreX(), cold.bounds.centreY());
                small.host->tick(2);
                CHECK(small.cold);
                small.host->click(warm.bounds.centreX(), warm.bounds.centreY());
                small.host->tick(2);
                CHECK(!small.cold);
                small.host->moveTo(5.0f, 5.0f, 1);
                small.host->tick(40);   // the hovers' eases land
            }
            golden("frame.settings.dpi1", *small.host);
        }
        const funkgui::Rect u = find("Settings");
        rig.host->click(u.centreX(), u.centreY());
        CHECK(f.unitsShown() && f.inspector() == nullptr);
        rig.host->click(u.centreX(), u.centreY());
        CHECK(!f.unitsShown() && f.inspector() != nullptr);
        return 0;
    }

    // The ribbon: under the inspector but on DYNO and DRIVE; an edit brings a new curve with the old one ghosted; it
    // folds to its title row and the inspector takes the room.
    VIEW_TEST(testFrameRibbon, "frame.ribbon")
    {
        FrameRig rig;
        AppFrame& f = *rig.frame;
        rig.showWorkspace("valves");
        CHECK(!f.ribbonRect().isEmpty());
        rig.showWorkspace("dyno");
        CHECK(f.ribbonRect().isEmpty());
        rig.showWorkspace("drive");
        CHECK(f.ribbonRect().isEmpty());
        rig.showWorkspace("air");
        const double before = rig.demo.engine.predicted().peakTorque().value;
        CHECK(rig.demo.engine.before().empty());
        f.showInspectorPage(1);
        rig.host->tick(2);
        const std::uint64_t asked = rig.demo.engine.requests();
        CHECK(rig.dragField("boost.level", 60.0f, 10));
        rig.host->tick(60);
        CHECK(rig.demo.engine.requests() == asked + 1);   // one drag of many steps: one curve asked for, after its quiet
        CHECK(!rig.demo.engine.before().empty() && rig.demo.engine.predicted().peakTorque().value > before);
        golden("frame.ribbon", *rig.host);
        const auto plotText = [&] {
            for (const funkgui::A11yItem& it : rig.host->accessibility())
                if (it.title == "Predicted torque and power")
                    return it.value;
            return std::string("(none)");
        };
        CHECK(plotText().find("KNOCK") == std::string::npos && rig.demo.engine.predicted().knockCount() == 0);
        // Knock-limited points (card C's): the demo knocks past 160 kPa of boost from 4500 rpm; ringed on the plot, the
        // span and retard in the title row's words and in the plot's accessible text.
        {
            const int bf = rig.demo.doc.find("boost.level");
            rig.demo.doc.beginGesture();
            rig.demo.doc.set(bf, 190e3);
            rig.demo.doc.endGesture();
            rig.host->tick(60);
            const tee::ui::DynoTrace& d = rig.demo.engine.predicted();
            CHECK(d.knockCount() > 0 && !d.knockLimited(0));
            std::size_t last = d.speed.size() - 1;
            CHECK_MSG(d.knockText(last) == "knock-limited, retard 6 to 10 deg", d.knockText(last));
            CHECK_MSG(plotText().find("; KNOCK-LIMITED 4500-7000 RPM, RETARD 1 TO 10 DEG") != std::string::npos, plotText());
            picture("frame.ribbon.knock", *rig.host);
            // Unsettled points (card C's torqueSe, settledCycles; the demo's from 185 kPa: 6250 rpm unsure, 6500 not
            // settled): an open square each on the torque curve, the speeds in the plot's accessible text.
            CHECK(d.unsettledAt(d.speed.size() - 4) && d.unsettledAt(d.speed.size() - 3) && !d.unsettledAt(d.speed.size() - 5));
            CHECK_MSG(plotText().find("; unsettled at 6250 rpm; unsettled at 6500 rpm") != std::string::npos, plotText());
            golden("frame.ribbon.unsettled", *rig.host);
        }
        const float open = f.inspectorRect().h;
        f.foldRibbon(true);
        rig.host->tick(2);
        CHECK(f.ribbonRect().h == AppFrame::kRibbonFoldedH && f.inspectorRect().h > open);
        return 0;
    }

    // From 1100 x 700 up: nothing leaves the window, the rail narrows under 1280, the stage keeps its room.
    VIEW_TEST(testFrameSizes, "frame.sizes")
    {
        FrameRig rig;
        AppFrame& f = *rig.frame;
        for (const auto& [w, h] : { std::pair{ 1100, 700 }, std::pair{ 1279, 760 }, std::pair{ 1600, 1000 }, std::pair{ 2560, 1440 } })
        {
            CHECK(rig.host->resize(w, h));
            rig.host->tick(5);
            CHECK(f.width() == w && f.height() == h);
            CHECK((f.railWidth() < AppFrame::kRailW) == (w < 1280));
            CHECK(f.railRect(10).bottom() <= f.cockpitRect().y);
            CHECK(f.pedalRect(2).right() <= static_cast<float>(w));
            CHECK(f.stageRect().w >= 560.0f && f.stageRect().h >= 500.0f);
            CHECK(f.rpmBar().bounds().w >= 240.0f);
            CHECK(f.inspectorRect().right() <= static_cast<float>(w) + 0.5f);
        }
        CHECK(rig.host->resize(800, 500));          // held at the smallest
        rig.host->tick(2);
        CHECK(f.width() == AppFrame::kMinW && f.height() == AppFrame::kMinH);
        return 0;
    }

    // At rest (the engine off, nothing pressed), no frame is wanted; a field's change, a live value's or a press wants
    // one.
    VIEW_TEST(testFrameIdle, "frame.idle")
    {
        FrameRig rig;
        AppFrame& f = *rig.frame;
        rig.demo.engine.set("engine.state", 0.0);
        rig.host->move(5000.0f, 5000.0f);
        rig.host->settle();
        rig.host->draw();
        rig.host->tick(3);
        CHECK(!f.wantsFullRate());
        CHECK(!f.needsDraw());
        rig.demo.engine.set("engine.speed", 100.0);
        rig.host->tick(1);
        CHECK(f.needsDraw());
        rig.host->settle();
        rig.host->draw();
        rig.host->tick(1);
        CHECK(!f.needsDraw());
        const int fld = rig.demo.doc.find("exhaust.muffler");
        rig.demo.doc.beginGesture();
        rig.demo.doc.set(fld, 0.9);
        rig.demo.doc.endGesture();
        rig.host->tick(1);
        CHECK(f.needsDraw());
        rig.host->settle();
        rig.host->draw();
        rig.host->tick(1);
        CHECK(!f.needsDraw());
        rig.host->click(f.railRect(2).centreX(), f.railRect(2).centreY());
        CHECK(f.needsDraw());
        return 0;
    }

    // Keyboard and screen reader: every control of the frame is named, in every workspace (U1's own pages fail the
    // test; another package's stage is said, its own test's to fail); Tab reaches the rail and the inspector's controls
    // with the focus drawn, and Enter on a rail button shows its workspace.
    VIEW_TEST(testFrameKeys, "frame.keys")
    {
        {
            FrameRig rig;
            for (const char* id : kIds)
            {
                rig.showWorkspace(id);
                const std::string unnamed = unnamedControls(*rig.host);
                CHECK_MSG(unnamed.empty(), std::string(id) + ":\n" + unnamed);
            }
        }
        {
            FrameRig real(1280, 800, 2.0f, 4.6, false);
            for (const char* id : kIds)
            {
                real.showWorkspace(id);
                const std::string unnamed = unnamedControls(*real.host);
                if (!unnamed.empty())
                    std::printf("  note (%s's stage): %s", id, unnamed.c_str());
            }
        }
        FrameRig rig;
        AppFrame& f = *rig.frame;
        rig.showWorkspace("air");
        // Tab from nothing: the frame's items in turn (the rail first), then the inspector's controls.
        bool rail = false, inspector = false;
        for (int k = 0; k < 120 && !(rail && inspector); ++k)
        {
            typeKeys(*rig.host, "tab");
            rail = rail || f.railFocus() >= 0;
            inspector = inspector || (f.inspectorRows() != nullptr ? f.inspectorRows()->focus() >= 0
                                                                   : f.inspector() != nullptr && f.inspector()->focus() >= 0);
        }
        CHECK(rail && inspector);
        // Enter on a rail button shows its workspace.
        FrameRig again;
        for (int k = 0; k < 120 && again.frame->railFocus() != 3; ++k)
            typeKeys(*again.host, "tab");
        CHECK(again.frame->railFocus() == 3);
        typeKeys(*again.host, "enter");
        again.host->tick(5);
        CHECK(again.frame->shown() == 3);
        return 0;
    }

    std::string lower(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return s;
    }

    // Every workspace's golden at 1280 x 800 (density 2 and 1), and the contact sheets out/ui/sheet.frame.png (density 2)
    // and sheet.frame.dpi1.png: each workspace at 1100 x 700 and 1600 x 1000, two workspaces a row.
    void sheet(float dpi, const std::string& file)
    {
        const std::vector<std::pair<int, int>> sizes = { { 1100, 700 }, { 1600, 1000 } };
        constexpr int kTileW = 640, kTileH = 400, kGap = 14, kPerRow = 2, kCount = 11;
        const int sheetW = kPerRow * (2 * kTileW + kGap) + (kPerRow + 1) * kGap * 2;
        const int rows = (kCount + kPerRow - 1) / kPerRow;
        const int sheetH = rows * (kTileH + kGap * 2) + kGap * 2;
        funkgui::Image out;
        out.w = sheetW;
        out.h = sheetH;
        out.rgba.assign(static_cast<std::size_t>(sheetW) * static_cast<std::size_t>(sheetH) * 4, 0);
        for (std::size_t i = 0; i < out.rgba.size(); i += 4)
            out.rgba[i] = out.rgba[i + 1] = out.rgba[i + 2] = 8, out.rgba[i + 3] = 255;
        for (std::size_t z = 0; z < sizes.size(); ++z)
        {
            FrameRig rig(sizes[z].first, sizes[z].second, dpi, 4.6, false);   // the real rail
            for (int s = 0; s < kCount; ++s)
            {
                rig.showWorkspace(kIds[s]);
                const funkgui::Image img = funkgui::rasterise(rig.host->draw(), funkgui::FontService::get().atlas(), 1);
                const int col = s % kPerRow, row = s / kPerRow;
                const int ox = kGap * 2 + col * (2 * kTileW + kGap + kGap * 2) + static_cast<int>(z) * (kTileW + kGap);
                const int oy = kGap * 2 + row * (kTileH + kGap * 2);
                const double sx = static_cast<double>(img.w) / kTileW, sy = static_cast<double>(img.h) / kTileH;
                const double sc = std::max(sx, sy);       // keep the aspect: letterboxed in its tile
                for (int y = 0; y < kTileH; ++y)
                    for (int x = 0; x < kTileW; ++x)
                    {
                        const int x0 = static_cast<int>(x * sc), x1 = std::max(x0 + 1, static_cast<int>((x + 1) * sc));
                        const int y0 = static_cast<int>(y * sc), y1 = std::max(y0 + 1, static_cast<int>((y + 1) * sc));
                        if (x0 >= img.w || y0 >= img.h)
                            continue;
                        unsigned sum[3] = { 0, 0, 0 }, n = 0;
                        for (int yy = y0; yy < y1 && yy < img.h; ++yy)
                            for (int xx = x0; xx < x1 && xx < img.w; ++xx)
                            {
                                const std::size_t k = (static_cast<std::size_t>(yy) * static_cast<std::size_t>(img.w) + static_cast<std::size_t>(xx)) * 4;
                                for (int ch = 0; ch < 3; ++ch)
                                    sum[ch] += img.rgba[k + static_cast<std::size_t>(ch)];
                                ++n;
                            }
                        const std::size_t d = (static_cast<std::size_t>(oy + y) * static_cast<std::size_t>(sheetW) + static_cast<std::size_t>(ox + x)) * 4;
                        for (int ch = 0; ch < 3; ++ch)
                            out.rgba[d + static_cast<std::size_t>(ch)] = static_cast<uint8_t>(n > 0 ? sum[ch] / n : 0);
                        out.rgba[d + 3] = 255;
                    }
            }
        }
        const std::string path = (outDir() / file).string();
        CHECK_MSG(tee::test::writePng(out, path), path);
        std::printf("  contact sheet: %s (software rasteriser, density %g; each row: two workspaces, each at 1100 x 700 then "
                    "1600 x 1000)\n", path.c_str(), static_cast<double>(dpi));
    }

    // The goldens of U1's own slots (placeholders and DRIVE) at densities 2 and 1: they move only with U1's work.
    VIEW_TEST(testFrameSheet, "frame.sheet")
    {
        for (const float dpi : { 2.0f, 1.0f })
        {
            FrameRig rig(1280, 800, dpi);
            for (const char* id : kIds)
            {
                rig.showWorkspace(id);
                golden("frame." + lower(id) + (dpi < 1.5f ? ".dpi1" : ""), *rig.host);
            }
        }
        return 0;
    }

    // Pictures, no checks (labelled nightly by its name's ".pictures": the soak tier makes them at every push, the
    // default run leaves them out, 01-rules.md section 3): the real rail, whatever is registered, at 1280 x 800; the
    // paper theme once; the contact sheets (every workspace at 1100 x 700 and 1600 x 1000, densities 2 and 1).
    VIEW_TEST(testFramePictures, "frame.pictures")
    {
        FrameRig real(1280, 800, 2.0f, 4.6, false);
        for (const char* id : kIds)
        {
            real.showWorkspace(id);
            picture("frame.real." + lower(id), *real.host);
        }
        {
            FrameRig rig(1280, 800, 2.0f);
            funkgui::HeadlessHost paper(*rig.frame, 1, 2.0f);
            paper.tick(5);
            picture("frame.engine.paper", paper);
        }
        sheet(2.0f, "sheet.frame.png");
        sheet(1.0f, "sheet.frame.dpi1.png");
        return 0;
    }
}

namespace
{
    using namespace tee::test::views;
    using tee::ui::AppFrame;

    // The item of `role` titled `title`: its bounds (empty: none shown).
    funkgui::Rect boundsOf(funkgui::HeadlessHost& host, const std::string& title, funkgui::A11yRole role)
    {
        for (const funkgui::A11yItem& it : host.accessibility())
            if (it.title == title && it.role == role)
                return it.bounds;
        return {};
    }
    bool clickOn(funkgui::HeadlessHost& host, const std::string& title, funkgui::A11yRole role = funkgui::A11yRole::button)
    {
        const funkgui::Rect r = boundsOf(host, title, role);
        if (r.isEmpty())
            return false;
        host.click(r.centreX(), r.centreY());
        host.tick(20);
        return true;
    }
    std::string allText(funkgui::HeadlessHost& host)
    {
        std::string out;
        for (const funkgui::A11yItem& it : host.accessibility())
            out += it.title + "|" + it.value + "|" + it.description + "\n";
        return out;
    }

    // The settings' CLAUDE DESKTOP row (card A's switch; the lead's ask, 25 Nov): off by default ("the app listens on
    // nothing"), Allow turns it on and the line follows the status (Session::statusNote), a refusal leaves it off and
    // says why in words; with no assistant served, no row.
    VIEW_TEST(testFrameDesktop, "frame.desktop")
    {
        {
            FrameRig bare(1100, 700, 2.0f);
            bare.frame->showSettings(true);
            bare.host->tick(20);
            CHECK(bare.frame->desktopLine().empty() && boundsOf(*bare.host, "Allow", funkgui::A11yRole::button).isEmpty());
        }
        FrameRig rig(1100, 700, 2.0f, 4.6, true, true);
        rig.frame->showSettings(true);
        rig.host->tick(20);
        CHECK_MSG(rig.frame->desktopLine() == "Off: the app listens on nothing.", rig.frame->desktopLine());
        // Every unit's stepper still above the cockpit at the smallest window, the row added.
        for (const tee::ui::Stepper* st : rig.frame->unitSteppers())
            CHECK_MSG(st->bounds().bottom() <= rig.frame->inspectorRect().bottom(), std::to_string(st->bounds().bottom()));
        golden("frame.desktop.off", *rig.host);
        CHECK(clickOn(*rig.host, "Allow") && rig.desktop);
        CHECK_MSG(rig.frame->desktopLine() == "Allowed", rig.frame->desktopLine());
        rig.status = "Claude Desktop connected, 3 changes";
        rig.host->tick(5);
        CHECK_MSG(rig.frame->desktopLine() == "Connected, 3 changes", rig.frame->desktopLine());
        golden("frame.desktop.connected", *rig.host);
        CHECK(clickOn(*rig.host, "Off") && !rig.desktop);
        rig.refuseWith = "another app has Claude Desktop's link";
        CHECK(clickOn(*rig.host, "Allow") && !rig.desktop);
        CHECK_MSG(rig.frame->desktopLine() == "Not allowed: another app has Claude Desktop's link", rig.frame->desktopLine());
        golden("frame.desktop.refused", *rig.host);
        return 0;
    }

    // A stand-in for card A's ChatView (ui/ links nothing of the assistant; its own panel is drawn in the root build's
    // build.chat): a framed box with a line of words and an "Add a key" button that calls the frame's key dialog.
    class StandInChat final : public tee::ui::Widget
    {
    public:
        StandInChat(uint32_t id, std::function<void()> addKey) : Widget(id), addKey_(std::move(addKey)) { title_ = "Chat"; }
        funkgui::Rect addKeyRect() const { return { bounds_.x + 12.0f, bounds_.y + 40.0f, 110.0f, 28.0f }; }
        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool) const override
        {
            const tee::ui::look::Palette p = tee::ui::look::Palette::of(th);
            c.rrect(bounds_.x, bounds_.y, bounds_.w, bounds_.h, 6.0f, p.panel, 1.0f, p.edge);
            c.text("THE CHAT'S PANEL (CARD A'S)", bounds_.x + 12.0f, bounds_.y + 14.0f, tee::ui::look::kCaps, p.label);
            const funkgui::Rect b = addKeyRect();
            c.rrect(b.x, b.y, b.w, b.h, 5.0f, p.raised, 1.0f, p.edge);
            c.text("ADD A KEY", b.centreX(), b.y + 9.0f, tee::ui::look::kCaps, p.value, funkgui::Align::centre);
        }
        void pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&) override
        {
            if (addKeyRect().contains({ e.x, e.y }) && addKey_)
                addKey_();
        }
        void accessibility(std::vector<funkgui::A11yItem>& out) const override
        {
            funkgui::A11yItem it;
            it.id = id_;
            it.role = funkgui::A11yRole::staticText;
            it.bounds = bounds_;
            it.title = "Chat";
            out.push_back(it);
        }
        bool a11yAction(uint32_t, funkgui::A11yAction, double, funkgui::GestureController&) override { return false; }

    protected:
        const char* kind() const override { return "chat"; }

    private:
        std::function<void()> addKey_;
    };

    funkgui::KeyEvent ch(char c, bool command = false)
    {
        funkgui::KeyEvent e;
        e.key = funkgui::Key::character;
        e.ch = static_cast<char32_t>(c);
        e.mods.ctrl = command;
        return e;
    }
    funkgui::KeyEvent keyOf(funkgui::Key k)
    {
        funkgui::KeyEvent e;
        e.key = k;
        return e;
    }

    // The chat's place (the lead's ask, 25 Nov): no CHAT until a chat is mounted; CHAT opens it in the inspector's
    // column (the tabs and the ribbon give it room) and closes it; SETTINGS takes the column back. Its key dialog
    // (from the chat's "Add a key" and from KEY): the key typed and pasted shows as stars and a count, never itself
    // (not drawn, not in accessibility), Enter saves it through the shell and forgets it here, a refusal is said in
    // words, Escape closes; while it is open the frame behind takes no key.
    VIEW_TEST(testFrameChat, "frame.chat")
    {
        FrameRig rig(1100, 700, 2.0f, 4.6, true, true);
        AppFrame& f = *rig.frame;
        rig.showWorkspace("tune");
        CHECK(boundsOf(*rig.host, "Chat", funkgui::A11yRole::button).isEmpty() && !f.chatMounted());
        auto chat = std::make_unique<StandInChat>(f.nextId(), [&f] { f.openKeyDialog(); });
        StandInChat* const panel = chat.get();
        f.mountChat(std::move(chat));
        rig.host->tick(10);
        CHECK(!f.chatOpen() && !panel->shown());   // closed at first
        CHECK(clickOn(*rig.host, "Chat") && f.chatOpen() && panel->shown());
        CHECK(panel->bounds().x >= f.stageRect().right() && f.ribbonRect().isEmpty() && f.inspectorTabRect(0).bottom() > 0.0f);
        for (int p = 0; p < f.inspectorPageCount(); ++p)   // the tabs give their room
            CHECK(boundsOf(*rig.host, f.inspectorTitle(p), funkgui::A11yRole::button).isEmpty() || f.inspectorPageCount() < 2);
        golden("frame.chat.open", *rig.host);
        // SETTINGS takes the column; CHAT again gives it back to the chat; CHAT closes it, the inspector as it was.
        CHECK(clickOn(*rig.host, "Settings") && !f.chatOpen() && f.settingsShown());
        CHECK(clickOn(*rig.host, "Chat") && f.chatOpen());
        CHECK(clickOn(*rig.host, "Settings") && !f.chatOpen() && f.settingsShown());
        CHECK(clickOn(*rig.host, "Settings") && !f.settingsShown());
        CHECK(clickOn(*rig.host, "Chat") && clickOn(*rig.host, "Chat") && !f.chatOpen() && !f.ribbonRect().isEmpty());
        // The key dialog from the chat's "Add a key".
        CHECK(clickOn(*rig.host, "Chat"));
        const funkgui::Rect add = panel->addKeyRect();
        rig.host->click(add.centreX(), add.centreY());
        rig.host->tick(10);
        CHECK(f.keyDialogOpen());
        tee::ui::KeyDialog& d = *f.keyDialog();
        CHECK(d.mask() == "none saved");
        const std::string secret = "sk-ant-api03-TESTKEYabcdefghijklmnopqrstuvwxyz0123456789-wxyz";
        for (const char c : std::string("sk-ant-"))
            f.key(ch(c));
        rig.host->setClipboard(secret.substr(7) + "\n");
        f.key(ch('v', true));                          // pasted: its line break dropped
        rig.host->tick(10);
        CHECK_MSG(d.typed() == secret.size(), std::to_string(d.typed()));
        CHECK(f.key(ch('z', true)) && rig.demo.doc.canUndo() == false);   // Ctrl-Z is the dialog's (typed nothing), not the build's undo
        CHECK(d.typed() == secret.size());
        // Never the key: not in accessibility, not in the words drawn (the field says how many characters).
        const std::string said = allText(*rig.host);
        CHECK_MSG(said.find("TESTKEY") == std::string::npos && said.find(std::to_string(secret.size()) + " characters") != std::string::npos, said);
        golden("frame.chat.key", *rig.host);
        // A refusal in words; the typed key forgotten all the same.
        rig.refuseKey = "the key was refused: 401, not a valid key";
        f.key(keyOf(funkgui::Key::enter));
        rig.host->tick(5);
        CHECK(rig.saves == 1 && d.typed() == 0 && d.messageIsError() && d.message() == rig.refuseKey && f.keyDialogOpen());
        picture("frame.chat.key.refused", *rig.host);
        // Saved: the mask's last four, the key forgotten here.
        rig.refuseKey.clear();
        rig.host->setClipboard(secret);
        f.key(ch('v', true));
        f.key(keyOf(funkgui::Key::enter));
        rig.host->tick(5);
        CHECK(rig.savedKey == secret && d.typed() == 0 && !d.messageIsError());
        CHECK_MSG(d.mask() == "saved, ending ...wxyz", d.mask());
        // Escape closes it; KEY in the chat's header opens it again, REMOVE removes the key.
        f.key(keyOf(funkgui::Key::escape));
        rig.host->tick(5);
        CHECK(!f.keyDialogOpen() && f.chatOpen());
        CHECK(clickOn(*rig.host, "Key") && f.keyDialogOpen());
        CHECK(clickOn(*rig.host, "Remove") && rig.savedKey.empty() && d.mask() == "none saved");
        CHECK(clickOn(*rig.host, "Close") && !f.keyDialogOpen());
        return 0;
    }
}

#include <thread>

namespace
{
    using namespace tee::test::views;
    using tee::ui::AppFrame;

    bool ready(std::future<bool>& f) { return f.wait_for(std::chrono::seconds(0)) == std::future_status::ready; }

    // The assistant's question to the owner (card A's SessionParts::ask; the lead's ask, 25 Nov): asked from another
    // thread, shown on the frame's tick with the asker's words (LOAD, KEEP), the safe answer (KEEP) with the focus;
    // Escape and Enter on it answer no, a press on LOAD (or Tab then Enter) yes; one question at a time, the next once
    // the first is answered; every key the dialog's while it is open; a frame that goes answers what is left no.
    VIEW_TEST(testFrameAsk, "frame.ask")
    {
        const std::string load = "Claude asks to load \"Hot cam\". The open build, \"Demo V8\", has unsaved changes, which "
                                 "would be lost. Load it?";
        std::future<bool> first, second, third;
        {
            FrameRig rig(1100, 700, 2.0f, 4.6, true, true);
            AppFrame& f = *rig.frame;
            std::thread worker([&] {
                first = f.ask(load, "Load", "Keep");
                second = f.ask("Claude asks to save this build as \"Demo V8 (2)\". Save it?");
            });
            worker.join();
            CHECK(!f.askOpen());   // shown on the frame's tick, not from the asker's thread
            rig.host->tick(5);
            CHECK(f.askOpen() && f.askDialog()->question() == load && !f.askDialog()->focusOnYes());
            golden("frame.ask", *rig.host);
            // Every key is the dialog's: Ctrl-Z does not undo the build behind it.
            funkgui::KeyEvent z;
            z.key = funkgui::Key::character;
            z.ch = U'z';
            z.mods.ctrl = true;
            CHECK(f.key(z) && f.askOpen() && !ready(first));
            // Enter presses KEEP (the focus): no. Nothing answers yes by itself.
            funkgui::KeyEvent enter;
            enter.key = funkgui::Key::enter;
            f.key(enter);
            CHECK(ready(first) && first.get() == false);
            rig.host->tick(5);
            CHECK(f.askOpen() && f.askDialog()->question().find("save this build") != std::string::npos);
            CHECK(boundsOf(*rig.host, "Yes", funkgui::A11yRole::button).w > 0.0f);   // no words given: YES and NO
            CHECK(clickOn(*rig.host, "Yes") && ready(second) && second.get() == true && !f.askOpen());
            // Escape is no; Tab then Enter is yes.
            third = f.ask(load, "Load", "Keep");
            rig.host->tick(5);
            funkgui::KeyEvent esc;
            esc.key = funkgui::Key::escape;
            f.key(esc);
            CHECK(ready(third) && third.get() == false);
            std::future<bool> fourth = f.ask(load, "Load", "Keep");
            rig.host->tick(5);
            funkgui::KeyEvent tab;
            tab.key = funkgui::Key::tab;
            f.key(tab);
            CHECK(f.askDialog()->focusOnYes());
            f.key(enter);
            CHECK(ready(fourth) && fourth.get() == true);
            // Two left when the frame goes: both answered no.
            first = f.ask(load, "Load", "Keep");
            second = f.ask(load, "Load", "Keep");
            rig.host->tick(5);
            CHECK(f.askOpen() && !ready(first) && !ready(second));
        }
        CHECK(ready(first) && first.get() == false && ready(second) && second.get() == false);
        return 0;
    }
}
