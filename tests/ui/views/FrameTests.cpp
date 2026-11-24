// tests/ui/views/FrameTests.cpp: the application's frame (package U1, grown from J2's; its tests' intent kept): the ten
// workspaces on the rail, every field that is not hidden reachable from an inspector, selection, a value set through
// the inspector and undone, the cockpit (start, pedals, rpm bar), the garage's stand-in and the units, the dyno ribbon,
// sizes from 1100 x 700 up, frames drawn only when something changed, goldens of every workspace and the contact sheet
// out/ui/sheet.frame.png.
#include "ViewKit.h"

#include "app/AppFrame.h"
#include "kit/widgets/Tables.h"

#include <funkgui/text/FontService.h>

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
        int mode = 0;
        std::unique_ptr<AppFrame> frame;
        std::unique_ptr<funkgui::HeadlessHost> host;

        // `own`: the frame over U1's own slots: every workspace but DRIVE a placeholder under an id no package registers
        // ("own-air"), so these tests and goldens do not move when another package's stage lands or changes. Not own:
        // the real rail, whatever is registered (the contact sheet's pictures).
        bool own;
        std::string idOf(const std::string& id) const { return own && id != "drive" ? "own-" + id : id; }

        explicit FrameRig(int w = 1280, int h = 800, float dpi = 2.0f, double t = 4.6, bool ownSlots = true) : demo(t, !ownSlots), own(ownSlots)
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
            s.mode = [this] { return mode; };
            s.setMode = [this](int m) { mode = m; };
            s.clutch = [this](float v) { clutch = v; };
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
        std::string a11yValue(const std::string& title) const
        {
            for (const funkgui::A11yItem& it : host->accessibility())
                if (it.title == title)
                    return it.value;
            return "(none)";
        }
    };

    const char* const kIds[10] = { "garage", "engine", "valves", "air", "exhaust", "tune", "car", "dyno", "drive", "scope" };

    // The ten workspaces in the rail's order; the rail shows each; each inspector page reads without a note; every
    // field that is not hidden is on some page.
    VIEW_TEST(testFrameWorkspaces, "frame.workspaces")
    {
        // The real rail: the ten in order, whatever is registered; the pages of U1's (the placeholders', DRIVE's) read
        // without a note. A registered workspace's own page is its package's to check (ViewKit's inspectorNotes).
        {
            FrameRig real(1280, 800, 2.0f, 4.6, false);
            AppFrame& f = *real.frame;
            CHECK(f.workspaceCount() >= 10);
            for (int i = 0; i < 10 && i < f.workspaceCount(); ++i)
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
        CHECK(f.inspectorPageCount() == 1 && f.inspector() != nullptr);
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
        const funkgui::Rect r = f.inspector()->controlRect("boost.level");
        CHECK(r.w > 0.0f);
        rig.host->drag(r.centreX(), r.centreY(), r.centreX(), r.centreY() - 80.0f, 12);
        CHECK(rig.value("boost.level") > 80e3);
        typeKeys(*rig.host, "cmd+z");
        CHECK_NEAR(rig.value("boost.level"), 80e3, 1e-6);
        typeKeys(*rig.host, "cmd+shift+z");
        CHECK(rig.value("boost.level") > 80e3);
        const funkgui::Rect m = f.inspector()->controlRect("boost.kind");
        rig.host->click(m.centreX(), m.centreY());
        CHECK(rig.host->chooseMenuItem("TWIN TURBO"));
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
        rig.demo.engine.set("engine.lambda", 0.86);
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
        // The units page from the top bar, anywhere.
        const funkgui::Rect u = find("Units");
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
        const funkgui::Rect r = f.inspector()->controlRect("boost.level");
        rig.host->drag(r.centreX(), r.centreY(), r.centreX(), r.centreY() - 60.0f, 10);
        rig.host->tick(60);
        CHECK(!rig.demo.engine.before().empty() && rig.demo.engine.predicted().peakTorque().value > before);
        golden("frame.ribbon", *rig.host);
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
            CHECK(f.railRect(9).bottom() <= f.cockpitRect().y);
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
        constexpr int kTileW = 640, kTileH = 400, kGap = 14, kPerRow = 2, kCount = 10;
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

    VIEW_TEST(testFrameSheet, "frame.sheet")
    {
        for (const float dpi : { 2.0f, 1.0f })
        {
            // Goldens over U1's own slots (placeholders and DRIVE): they move only with U1's work.
            FrameRig rig(1280, 800, dpi);
            for (const char* id : kIds)
            {
                rig.showWorkspace(id);
                golden("frame." + lower(id) + (dpi < 1.5f ? ".dpi1" : ""), *rig.host);
            }
            // Pictures of the real rail, whatever is registered (no golden: other packages' stages move).
            FrameRig real(1280, 800, dpi, 4.6, false);
            for (const char* id : kIds)
            {
                real.showWorkspace(id);
                picture("frame.real." + lower(id) + (dpi < 1.5f ? ".dpi1" : ""), *real.host);
            }
        }
        // The paper theme, once: the same frame reads on a light ground.
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
