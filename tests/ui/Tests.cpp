// tests/ui/Tests.cpp: tee-ui-tests, the interface kit's headless tests (card J, item 5). Every test drives a Panel
// through FunkGui's HeadlessHost with scripted input and looks at what moved: a parameter of the made-up Player, a
// place in the layout, a readout, the frame's fingerprint. Nothing here opens a window or a device.
//
//   tee-ui-tests <name>            one test; 0 when it passes, 1 when it fails, 77 when it cannot run here
//   tee-ui-tests all               every test
//   tee-ui-tests --list            the names
//   ... --bless                    write the goldens the test compares with (tests/ui/golden/), instead of comparing
//
// A golden is a frame's fingerprint (funkgui/canvas/Fingerprint.h: hashes of the geometry and of the text, and counts)
// as a text file. Each golden test also writes its frame as a PNG under out/ui/, drawn by FunkGui's software
// rasteriser, which is what a person looks at before blessing. The rasteriser draws no shader: a shader element is its
// placeholder in every picture here.
#include "MockPlayer.h"
#include "Png.h"

#include "kit/gui/DisplayItem.h"
#include "kit/gui/GuiItems.h"
#include "kit/gui/GuiPanel.h"
#include "kit/gui/GuiSpec.h"
#include "kit/gui/Ports.h"
#include "kit/shaders/ShaderText.h"
#include "ref/RefPanel.h"

#include <funkgui/canvas/Fingerprint.h>
#include <funkgui/panel/HeadlessGuiScope.h>
#include <funkgui/panel/HeadlessHost.h>
#include <funkgui/text/FontService.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iterator>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace
{
    using tee::test::MockPlayer;
    using namespace tee::ui;

    int g_fails = 0;
    bool g_bless = false;
    constexpr int kSkip = 77;

    void failAt(const char* file, int line, const std::string& what)
    {
        ++g_fails;
        std::printf("  FAIL %s:%d: %s\n", file, line, what.c_str());
    }
#define CHECK(cond) do { if (!(cond)) failAt(__FILE__, __LINE__, #cond); } while (false)
#define CHECK_MSG(cond, msg) do { if (!(cond)) failAt(__FILE__, __LINE__, std::string(#cond) + ": " + (msg)); } while (false)
#define CHECK_NEAR(a, b, tol)                                                                                          \
    do                                                                                                                 \
    {                                                                                                                  \
        const double va = static_cast<double>(a), vb = static_cast<double>(b);                                        \
        if (!(std::fabs(va - vb) <= (tol)))                                                                            \
            failAt(__FILE__, __LINE__, std::string(#a) + " = " + std::to_string(va) + ", wanted " + std::to_string(vb)); \
    } while (false)

    std::string readText(const std::filesystem::path& p)
    {
        std::ifstream in(p, std::ios::binary);
        return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }

    const std::filesystem::path kTestDir = TEE_UI_TEST_DIR;
    const std::filesystem::path kRepoDir = TEE_UI_REPO_DIR;
    const std::filesystem::path kOutDir = TEE_UI_OUT_DIR;

    Sources panelFiles(std::initializer_list<const char*> names)
    {
        Sources out;
        for (const char* n : names)
            out.push_back({ n, readText(kTestDir / "panels" / n) });
        return out;
    }
    Sources kindsFiles() { return panelFiles({ "kinds.gui", "glow.sc" }); }

    // A panel of kinds.gui over a made-up Player, in a headless host.
    struct Rig
    {
        MockPlayer player;
        GuiPanel panel;
        funkgui::HeadlessHost host;

        explicit Rig(int w = 0, int h = 0, Sources files = kindsFiles())
            : panel(std::move(files), player, player, [&] {
                  GuiPanel::Options o;
                  o.width = w;
                  o.height = h;
                  o.name = "kinds";
                  return o;
              }()),
              host(panel, 0, 2.0f)
        {
            host.tick(2);
        }

        funkgui::Point centreOf(const char* id) const
        {
            const funkgui::Rect r = panel.controlRect(id);
            return { r.centreX(), r.centreY() };
        }
        GuiControlItem& control(const char* id)
        {
            const int item = panel.view().controlItem(panel.slotOf(id));
            return static_cast<GuiControlItem&>(*panel.items()[static_cast<std::size_t>(item)]);
        }
        bool page(const char* title)
        {
            const funkgui::Rect r = panel.tabRect(title);
            if (r.w <= 0.0f)
                return false;
            host.click(r.centreX(), r.centreY());
            host.tick(2);
            return true;
        }
    };

    // The frame's fingerprint as the golden's text.
    std::string fingerprintText(const funkgui::PrimList& frame)
    {
        const funkgui::Fingerprint f = funkgui::fingerprint(frame);
        char buf[512];
        std::snprintf(buf, sizeof buf,
                      "geometry %016llx\ntext %016llx\nstatics %d\nlive %d\ntexts %d\nrrects %d\nsegments %d\nareas %d\n"
                      "shaders %d\nimages %d\nsize %d x %d\n",
                      static_cast<unsigned long long>(f.geometry), static_cast<unsigned long long>(f.text), f.statics, f.live,
                      f.texts, f.rrects, f.segments, f.areas, f.shaders, f.images, frame.info.logicalW, frame.info.logicalH);
        return buf;
    }

    // The host's frame against tests/ui/golden/<name>.txt (written instead with --bless), its picture to out/ui/.
    void golden(const std::string& name, funkgui::HeadlessHost& host)
    {
        const funkgui::PrimList& frame = host.draw();
        const std::string now = fingerprintText(frame);
        std::error_code ec;
        std::filesystem::create_directories(kOutDir, ec);
        const std::string png = (kOutDir / (name + ".png")).string();
        if (!tee::test::writePng(funkgui::rasterise(frame, funkgui::FontService::get().atlas(), 2), png))
            std::printf("  (could not write %s)\n", png.c_str());
        const std::filesystem::path path = kTestDir / "golden" / (name + ".txt");
        if (g_bless)
        {
            std::ofstream(path, std::ios::binary | std::ios::trunc) << now;
            std::printf("  blessed %s (look at %s)\n", path.string().c_str(), png.c_str());
            return;
        }
        const std::string had = readText(path);
        if (had.empty())
            failAt(__FILE__, __LINE__, "no golden " + path.string() + " (run with --bless after looking at " + png + ")");
        else if (had != now)
            failAt(__FILE__, __LINE__, "the frame differs from " + path.string() + ":\n--- golden\n" + had + "--- now\n" + now
                                           + "(look at " + png + ")");
    }

    // ---- the tests ---------------------------------------------------------------------------------------------------
    using Test = std::function<int()>;
    std::vector<std::pair<std::string, Test>>& tests()
    {
        static std::vector<std::pair<std::string, Test>> all;
        return all;
    }
    struct Add
    {
        Add(const char* name, Test t) { tests().emplace_back(name, std::move(t)); }
    };
#define TEST(ident, name) int ident(); const Add add_##ident(name, ident); int ident()

    // CMake's list of tests is this program's.
    TEST(testNames, "names")
    {
        std::set<std::string> mine, theirs;
        for (const auto& t : tests())
            mine.insert(t.first);
        std::stringstream in(TEE_UI_TEST_NAMES);
        for (std::string n; std::getline(in, n, ',');)
            theirs.insert(n);
        for (const std::string& n : mine)
            CHECK_MSG(theirs.count(n) == 1, n + " is not in tests/ui/CMakeLists.txt");
        for (const std::string& n : theirs)
            CHECK_MSG(mine.count(n) == 1, n + " is in tests/ui/CMakeLists.txt and is no test");
        return 0;
    }

    // Every file taken from TestingPlayground says so on its first line and has its row in ui/kit/SEED.tsv, and
    // every row is a file.
    TEST(testSeed, "seed")
    {
        const std::filesystem::path kit = kRepoDir / "ui" / "kit";
        std::map<std::string, std::vector<std::string>> rows;
        std::stringstream in(readText(kit / "SEED.tsv"));
        for (std::string line; std::getline(in, line);)
        {
            if (line.empty() || line[0] == '#')
                continue;
            std::vector<std::string> cols;
            std::stringstream ls(line);
            for (std::string c; std::getline(ls, c, '\t');)
                cols.push_back(c);
            CHECK_MSG(cols.size() == 4, "a row of 4 columns: " + line);
            if (cols.size() == 4)
                rows[cols[0]] = cols;
        }
        CHECK(rows.size() >= 30);
        for (const auto& [ours, cols] : rows)
        {
            const std::string text = readText(kRepoDir / ours);
            CHECK_MSG(!text.empty(), ours + " is in SEED.tsv and is not a file");
            const std::string first = text.substr(0, text.find('\n'));
            CHECK_MSG(first.find("from TestingPlayground, " + cols[1] + " at " + cols[2]) != std::string::npos,
                      ours + ": its first line does not name " + cols[1] + " at " + cols[2]);
            CHECK_MSG(cols[2].size() == 40, ours + ": the commit is not a full hash");
            CHECK_MSG(cols[3].size() == 64 && cols[3].find_first_not_of("0123456789abcdef") == std::string::npos,
                      ours + ": the sha256 is not 64 hex digits");
        }
        for (const char* dir : { "gui", "shaders" })
            for (const auto& entry : std::filesystem::directory_iterator(kit / dir))
            {
                const std::string text = readText(entry.path());
                const std::string first = text.substr(0, text.find('\n'));
                const std::string ours = std::string("ui/kit/") + dir + "/" + entry.path().filename().string();
                if (first.find("from TestingPlayground, Source/") != std::string::npos)
                    CHECK_MSG(rows.count(ours) == 1, ours + " says it is from TestingPlayground and has no row in SEED.tsv");
            }
        return 0;
    }

    // kinds.gui is taken whole, and holds every kind of element.
    TEST(testGuiParse, "gui.parse")
    {
        Rig rig;
        CHECK_MSG(rig.panel.notes().empty(), rig.panel.notes());
        std::set<GuiKind> kinds;
        for (const GuiNode& n : rig.panel.spec().gui.nodes)
            kinds.insert(n.kind);
        for (const GuiKind k : { GuiKind::row, GuiKind::column, GuiKind::grid, GuiKind::section, GuiKind::box, GuiKind::tabs,
                                 GuiKind::page, GuiKind::spacer, GuiKind::divider, GuiKind::label, GuiKind::knob,
                                 GuiKind::slider, GuiKind::toggle, GuiKind::buttons, GuiKind::menu, GuiKind::xy,
                                 GuiKind::envelope, GuiKind::display, GuiKind::keyboard, GuiKind::shader, GuiKind::area })
            CHECK_MSG(kinds.count(k) == 1, "kinds.gui lacks kind " + std::to_string(static_cast<int>(k)));
        std::set<DisplayKind> displays;
        for (const PanelDisplay& d : rig.panel.spec().displays)
            displays.insert(d.kind);
        CHECK(displays.size() == 6);                     // meter, vu, led, number, scope, spectrum
        std::set<std::string> looks;
        for (const GuiNode& n : rig.panel.spec().gui.nodes)
            if (n.kind == GuiKind::knob)
                looks.insert(n.style.look.empty() ? "modern" : n.style.look);
        for (const char* look : { "modern", "vintage", "flat", "minimal", "ring", "led", "chicken", "soft", "dot" })
            CHECK_MSG(looks.count(look) == 1, std::string("no knob of look ") + look);
        // What the file gives a parameter reaches its control: a log taper, a unit, a scale.
        const Control& cutoff = rig.panel.controls()[static_cast<std::size_t>(rig.panel.slotOf("cutoff"))];
        CHECK(cutoff.taper == ControlTaper::log);
        CHECK(std::string(cutoff.unit) == "Hz");
        CHECK(rig.panel.spec().shaders.size() == 1);
        CHECK(rig.panel.naturalWidth() > 300 && rig.panel.naturalHeight() > 200);
        return 0;
    }

    // What the reader cannot use is a note with its place, and the rest stands.
    TEST(testGuiNotes, "gui.notes")
    {
        Sources files{ { "bad.gui", "row {\n  knob cutoff\n  knob nosuchparam\n  wobble 3\n  meter nosuchfield\n  knob level\n}\n" } };
        Rig rig(0, 0, files);
        const std::string& notes = rig.panel.notes();
        CHECK_MSG(notes.find("bad.gui:3:") != std::string::npos, notes);
        CHECK_MSG(notes.find("bad.gui:4:") != std::string::npos, notes);
        CHECK_MSG(notes.find("bad.gui:5:") != std::string::npos, notes);
        CHECK_MSG(notes.find("note:") != std::string::npos, notes);
        CHECK(rig.panel.controlRect("cutoff").w > 0.0f);
        CHECK(rig.panel.controlRect("level").w > 0.0f);
        // No .gui file at all: a note, an empty panel, no crash.
        Rig none(0, 0, Sources{});
        CHECK(none.panel.notes().find("no .gui file") != std::string::npos);
        none.host.draw();
        return 0;
    }

    // A port maps 0..1 onto the parameter's own units and back.
    TEST(testPorts, "ports")
    {
        MockPlayer player;
        SourcePort level(player, player.find("level"));
        level.setValue01(0.25f);
        CHECK_NEAR(player.plain("level"), 0.25, 1e-6);
        CHECK_NEAR(level.value01(), 0.25, 1e-6);
        CHECK(level.numSteps() == 0);
        SourcePort wave(player, player.find("wave"));
        CHECK(wave.numSteps() == 3);
        wave.setValue01(1.0f);
        CHECK_NEAR(player.plain("wave"), 3.0, 0.0);
        wave.setValue01(0.4f);                           // the nearest of 0, 1, 2, 3
        CHECK_NEAR(player.plain("wave"), 1.0, 0.0);
        SourcePort sync(player, player.find("sync"));
        sync.setValue01(0.7f);
        CHECK_NEAR(player.plain("sync"), 1.0, 0.0);
        SourcePort steps(player, player.find("steps"));
        CHECK_NEAR(steps.default01(), 1.0 / 3.0, 1e-6);  // 2 in 1..4
        // A log taper (the .gui file's, or the parameter's own): the middle of the travel is the geometric mean.
        tee::ParamInfo info = player.info(player.find("cutoff"));
        info.logTaper = true;
        Control c = controlOf(info);
        CHECK_NEAR(plainOf(c, 0.5f), std::sqrt(20.0 * 20000.0), 0.5);
        CHECK_NEAR(normOf(c, 2000.0f), std::log(100.0) / std::log(1000.0), 1e-5);
        CHECK(optionsOf(player.info(player.find("mode"))).size() == 3);
        return 0;
    }

    // Every page of kinds.gui at its natural size.
    TEST(testKindsGolden, "kinds.golden")
    {
        Rig rig;
        for (const char* title : { "KNOBS", "SWITCHES", "DISPLAYS", "FRAMES" })
        {
            CHECK_MSG(rig.page(title), std::string("no tab ") + title);
            rig.host.tick(30);                           // the displays settle on the telemetry
            std::string name = std::string("kinds.") + title;
            std::transform(name.begin(), name.end(), name.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            golden(name, rig.host);
        }
        return 0;
    }

    // Smaller than its natural size it scrolls, and nothing is laid out narrower than it is.
    TEST(testKindsSmall, "kinds.small")
    {
        Rig rig(480, 300);
        const GuiLayout& gl = rig.panel.view().places();
        CHECK(rig.panel.width() == 480 && rig.panel.height() == 300);
        CHECK(gl.maxScrollX > 0.0f || gl.maxScrollY > 0.0f);
        CHECK(gl.contentW >= static_cast<float>(rig.panel.naturalWidth()) - 1.0f);
        golden("kinds.small", rig.host);
        return 0;
    }

    // Larger, it fills the window and scrolls nowhere.
    TEST(testKindsLarge, "kinds.large")
    {
        Rig rig(1600, 1000);
        const GuiLayout& gl = rig.panel.view().places();
        CHECK(gl.maxScrollX == 0.0f && gl.maxScrollY == 0.0f);
        CHECK_NEAR(gl.contentW, 1600.0, 1.0);
        const funkgui::Rect r = rig.panel.controlRect("cutoff");
        CHECK(r.w > 0.0f && r.right() <= 1600.0f && r.bottom() <= 1000.0f);
        golden("kinds.large", rig.host);
        return 0;
    }

    // A drag turns a knob and the parameter moves: up raises it, 200 px the whole range, one gesture.
    TEST(testKnobDrag, "knob.drag")
    {
        Rig rig;
        const funkgui::Point p = rig.centreOf("resonance");
        CHECK(p.x > 0.0f);
        const double before = rig.player.plain("resonance");
        rig.host.drag(p.x, p.y, p.x, p.y - 50.0f);
        rig.host.tick(2);
        CHECK_NEAR(rig.player.plain("resonance"), before + 0.25, 0.01);
        rig.host.drag(p.x, p.y, p.x, p.y + 300.0f);      // past the end: clamped
        CHECK_NEAR(rig.player.plain("resonance"), 0.0, 1e-6);
        SourcePort* port = rig.panel.port(rig.panel.slotOf("resonance"));
        CHECK(port != nullptr && port->gesturesBegun() == 2 && port->gesturesEnded() == 2);
        // A knob with a log taper (kinds.gui's taper=log): a fifth of the travel is the same ratio wherever it starts.
        const funkgui::Point c = rig.centreOf("cutoff");
        const double from = rig.player.plain("cutoff");
        rig.host.drag(c.x, c.y, c.x, c.y - 40.0f);
        CHECK_NEAR(rig.player.plain("cutoff") / from, std::pow(1000.0, 0.2), 0.05);
        return 0;
    }

    // The wheel steps a knob a hundredth a notch, in one gesture a burst.
    TEST(testKnobWheel, "knob.wheel")
    {
        Rig rig;
        const funkgui::Point p = rig.centreOf("level");
        const double before = rig.player.plain("level");
        rig.host.wheel(p.x, p.y, 1.0f);
        rig.host.wheel(p.x, p.y, 1.0f);
        CHECK(rig.player.plain("level") > before);
        CHECK_NEAR(rig.player.plain("level"), before + 0.02, 0.011);
        rig.host.tick(60);                               // the burst's gesture ends half a second after its last notch
        SourcePort* port = rig.panel.port(rig.panel.slotOf("level"));
        CHECK(port->gesturesBegun() == 1 && port->gesturesEnded() == 1);
        return 0;
    }

    // Tab reaches a control; the arrows step it, Home and End go to its ends.
    TEST(testKnobKeys, "knob.keys")
    {
        Rig rig;
        const funkgui::Point p = rig.centreOf("resonance");
        rig.host.click(p.x, p.y);                        // the pointer's focus
        CHECK(rig.panel.focus() == rig.panel.view().controlItem(rig.panel.slotOf("resonance")));
        const double before = rig.player.plain("resonance");
        rig.host.keys("right");
        CHECK_NEAR(rig.player.plain("resonance"), before + 0.01, 0.002);
        rig.host.keys("shift+right");
        CHECK_NEAR(rig.player.plain("resonance"), before + 0.11, 0.004);
        rig.host.keys("end");
        CHECK_NEAR(rig.player.plain("resonance"), 1.0, 1e-6);
        rig.host.keys("home");
        CHECK_NEAR(rig.player.plain("resonance"), 0.0, 1e-6);
        const int was = rig.panel.focus();
        rig.host.keys("tab");
        CHECK(rig.panel.focus() >= 0 && rig.panel.focus() != was);
        rig.host.keys("shift+tab");
        CHECK(rig.panel.focus() == was);
        return 0;
    }

    // A double-click puts a knob back to its parameter's default.
    TEST(testKnobReset, "knob.reset")
    {
        Rig rig;
        const funkgui::Point p = rig.centreOf("drive");
        rig.host.drag(p.x, p.y, p.x, p.y - 80.0f);
        CHECK(rig.player.plain("drive") > 5.0);
        rig.host.doubleClick(p.x, p.y);
        CHECK_NEAR(rig.player.plain("drive"), 2.0, 1e-4);
        return 0;
    }

    // A slider follows the pointer along its track.
    TEST(testSliderDrag, "slider.drag")
    {
        Rig rig;
        CHECK(rig.page("SWITCHES"));
        const funkgui::Rect t = rig.control("mix").track();          // vertical
        CHECK(t.h > 20.0f);
        const double before = rig.player.plain("mix");
        const float y0 = t.bottom() - static_cast<float>(before) * t.h;
        rig.host.drag(t.centreX(), y0, t.centreX(), y0 - t.h * 0.25f);
        CHECK_NEAR(rig.player.plain("mix"), before + 0.25, 0.03);
        const funkgui::Rect h = rig.control("spread").track();       // horizontal
        CHECK(h.w > 100.0f);
        rig.host.drag(h.x + h.w * 0.5f, h.centreY(), h.right() + 40.0f, h.centreY());
        CHECK_NEAR(rig.player.plain("spread"), 1.0, 1e-6);
        return 0;
    }

    TEST(testSwitchClick, "switch.click")
    {
        Rig rig;
        CHECK(rig.page("SWITCHES"));
        const funkgui::Point p = rig.centreOf("sync");
        CHECK_NEAR(rig.player.plain("sync"), 0.0, 0.0);
        rig.host.click(p.x, p.y);
        CHECK_NEAR(rig.player.plain("sync"), 1.0, 0.0);
        rig.host.click(p.x, p.y);
        CHECK_NEAR(rig.player.plain("sync"), 0.0, 0.0);
        rig.host.keys("space");                          // it has the focus from its click
        CHECK_NEAR(rig.player.plain("sync"), 1.0, 0.0);
        return 0;
    }

    // A click on a button chooses its value: a column of names, a row of numbers.
    TEST(testButtonsClick, "buttons.click")
    {
        Rig rig;
        CHECK(rig.page("SWITCHES"));
        const auto press = [&](const char* id, int k) {
            GuiControlItem& item = rig.control(id);
            const funkgui::Rect c = item.cell();
            for (float y = c.y + 1.0f; y < c.bottom(); y += 2.0f)
                for (float x = c.x + 1.0f; x < c.right(); x += 2.0f)
                    if (item.buttonAt({ x, y }) == k)
                    {
                        rig.host.click(x + 1.0f, y + 1.0f);
                        return true;
                    }
            return false;
        };
        CHECK(press("wave", 2));
        CHECK_NEAR(rig.player.plain("wave"), 2.0, 0.0);
        CHECK(press("wave", 0));
        CHECK_NEAR(rig.player.plain("wave"), 0.0, 0.0);
        CHECK(press("steps", 3));                        // 1..4: the fourth
        CHECK_NEAR(rig.player.plain("steps"), 4.0, 0.0);
        return 0;
    }

    // A menu control opens the host's menu with its values' names; the one chosen is the parameter's value.
    TEST(testMenuPick, "menu.pick")
    {
        Rig rig;
        CHECK(rig.page("SWITCHES"));
        const funkgui::Rect box = rig.control("mode").menuBox();
        CHECK(box.w > 0.0f);
        rig.host.click(box.centreX(), box.centreY());
        const funkgui::MenuRequest* menu = rig.host.pendingMenu();
        CHECK(menu != nullptr);
        if (menu == nullptr)
            return 0;
        CHECK(menu->items.size() == 3);
        CHECK(rig.host.chooseMenuItem("GAMMA"));
        CHECK_NEAR(rig.player.plain("mode"), 2.0, 0.0);
        rig.host.click(box.centreX(), box.centreY());
        CHECK(rig.host.cancelMenu());                    // dismissed: nothing written
        CHECK_NEAR(rig.player.plain("mode"), 2.0, 0.0);
        rig.host.keys("return");                         // the key opens it too
        CHECK(rig.host.pendingMenu() != nullptr);
        CHECK(rig.host.chooseMenuItem("ALPHA"));
        CHECK_NEAR(rig.player.plain("mode"), 0.0, 0.0);
        return 0;
    }

    // A pad moves two parameters at once: x to the right, y up.
    TEST(testXyDrag, "xy.drag")
    {
        Rig rig;
        CHECK(rig.page("SWITCHES"));
        CHECK(rig.panel.view().pads().size() >= 1);
        const auto& pad = static_cast<const GuiXyItem&>(*rig.panel.items()[static_cast<std::size_t>(rig.panel.view().pads()[0])]);
        CHECK(pad.shown());
        const funkgui::Rect r = pad.pad();
        rig.host.drag(r.centreX(), r.centreY(), r.x + r.w * 0.9f, r.y + r.h * 0.2f);
        CHECK_NEAR(rig.player.plain("padx"), 0.9, 0.05);
        CHECK_NEAR(rig.player.plain("pady"), 0.8, 0.05);
        return 0;
    }

    // An area writes what the pointer does over it; a box with pointer properties takes a press and lets go.
    TEST(testPointerArea, "pointer.area")
    {
        Rig rig;
        CHECK(rig.page("DISPLAYS"));
        const GuiNode* area = nullptr;
        const GuiNode* box = nullptr;
        const auto& nodes = rig.panel.spec().gui.nodes;
        std::size_t areaAt = 0, boxAt = 0;
        for (std::size_t i = 0; i < nodes.size(); ++i)
        {
            if (nodes[i].kind == GuiKind::area)
            {
                area = &nodes[i];
                areaAt = i;
            }
            if (nodes[i].kind == GuiKind::box && nodes[i].pointer.any())
            {
                box = &nodes[i];
                boxAt = i;
            }
        }
        CHECK(area != nullptr && box != nullptr);
        if (area == nullptr || box == nullptr)
            return 0;
        const funkgui::Rect a = rig.panel.view().places().rects[areaAt];
        rig.host.click(a.x + a.w * 0.25f, a.y + a.h * 0.25f);
        CHECK_NEAR(rig.player.plain("padx"), 0.25, 0.03);
        CHECK_NEAR(rig.player.plain("pady"), 0.75, 0.03);
        const funkgui::Rect b = rig.panel.view().places().rects[boxAt];
        rig.host.press(b.x + b.w * 0.5f, b.y + b.h * 0.5f);
        CHECK_NEAR(rig.player.plain("hold"), 1.0, 0.0);  // down= while pressed
        rig.host.release();
        CHECK_NEAR(rig.player.plain("hold"), 0.0, 0.0);
        return 0;
    }

    // Tabs switch pages: a page's controls are placed only while it shows.
    TEST(testTabsSwitch, "tabs.switch")
    {
        Rig rig;
        CHECK(rig.panel.controlRect("cutoff").w > 0.0f);
        CHECK(rig.panel.controlRect("mix").w == 0.0f);
        const uint32_t before = rig.panel.a11yRevision();
        CHECK(rig.page("SWITCHES"));
        CHECK(rig.panel.controlRect("cutoff").w == 0.0f);
        CHECK(rig.panel.controlRect("mix").w > 0.0f);
        CHECK(rig.panel.a11yRevision() != before);
        CHECK(rig.page("FRAMES"));                       // tabs inside a page, in three looks
        for (const char* inner : { "TWO", "P2", "S2" })
        {
            const funkgui::Rect r = rig.panel.tabRect(inner);
            CHECK_MSG(r.w > 0.0f, std::string("no tab ") + inner);
            rig.host.click(r.centreX(), r.centreY());
        }
        rig.host.tick(2);
        golden("tabs.inner", rig.host);
        CHECK(rig.page("KNOBS"));
        CHECK(rig.panel.controlRect("cutoff").w > 0.0f);
        return 0;
    }

    // A resize lays out again: the interface fills the new size, and scrolls when that is too small.
    TEST(testResizeLayout, "resize.layout")
    {
        Rig rig;
        const int w = rig.panel.naturalWidth(), h = rig.panel.naturalHeight();
        CHECK(rig.panel.width() == std::max(w, rig.panel.minWidth()));
        const funkgui::Rect before = rig.panel.controlRect("depth");
        CHECK(rig.host.resize(w + 300, h + 200));
        rig.host.tick(2);
        CHECK(rig.panel.width() == w + 300 && rig.panel.height() == h + 200);
        CHECK_NEAR(rig.panel.view().places().contentW, w + 300, 1.0);
        CHECK(rig.panel.view().places().maxScrollY == 0.0f);
        rig.host.resize(rig.panel.minWidth(), rig.panel.minHeight());
        rig.host.tick(2);
        CHECK(rig.panel.width() == rig.panel.minWidth());
        CHECK(rig.panel.view().places().maxScrollX > 0.0f || rig.panel.view().places().maxScrollY > 0.0f);
        rig.host.resize(w, h);
        rig.host.tick(2);
        const funkgui::Rect after = rig.panel.controlRect("depth");
        CHECK_NEAR(after.x, before.x, 0.01);
        CHECK_NEAR(after.y, before.y, 0.01);
        return 0;
    }

    // The wheel scrolls an interface larger than its window, but turns a control under the pointer instead.
    TEST(testScrollWheel, "scroll.wheel")
    {
        Rig rig(480, 300);
        CHECK(rig.panel.view().places().maxScrollY > 0.0f);
        const funkgui::Rect before = rig.panel.controlRect("cutoff");
        rig.host.wheel(470.0f, 150.0f, -1.0f);           // over no control, at the right edge
        const funkgui::Rect after = rig.panel.controlRect("cutoff");
        CHECK(rig.panel.view().scrollY() > 0.0f);
        CHECK_NEAR(after.y, before.y - rig.panel.view().scrollY(), 1.0);
        const double level = rig.player.plain("resonance");
        const float scrolled = rig.panel.view().scrollY();
        const funkgui::Point p = rig.centreOf("resonance");
        if (p.y > 0.0f && p.y < 300.0f && p.x < 470.0f)
        {
            rig.host.wheel(p.x, p.y, 1.0f);
            CHECK(rig.player.plain("resonance") > level);
            CHECK(rig.panel.view().scrollY() == scrolled);
        }
        return 0;
    }

    // A display shows the telemetry: a number's readout, a meter's level, an LED.
    TEST(testDisplayTelemetry, "display.telemetry")
    {
        Rig rig;
        CHECK(rig.page("DISPLAYS"));
        const auto display = [&](DisplayKind kind, const char* source) -> const DisplayItem* {
            for (const int i : rig.panel.view().displays())
            {
                const auto& d = static_cast<const DisplayItem&>(*rig.panel.items()[static_cast<std::size_t>(i)]);
                if (d.shown() && d.display().kind == kind && d.display().source == source)
                    return &d;
            }
            return nullptr;
        };
        const DisplayItem* rpm = display(DisplayKind::number, "shared->showRpm");
        const DisplayItem* boost = display(DisplayKind::meter, "boost");
        const DisplayItem* gate = display(DisplayKind::led, "gate");
        const DisplayItem* out = display(DisplayKind::meter, "out");
        CHECK(rpm != nullptr && boost != nullptr && gate != nullptr && out != nullptr);
        if (rpm == nullptr || boost == nullptr || gate == nullptr || out == nullptr)
            return 0;
        rig.host.tick(30);
        CHECK_MSG(rpm->readout().find("850") != std::string::npos, rpm->readout());
        rig.player.setTelemetry("showRpm", 4200.0);
        rig.player.setTelemetry("gate", 1.0);
        rig.player.setTelemetry("boost", 1.5);
        rig.player.peak = 1.0f;
        rig.host.tick(120);
        CHECK_MSG(rpm->readout().find("4200") != std::string::npos, rpm->readout());
        CHECK_MSG(rpm->readout().find("rpm") != std::string::npos, rpm->readout());
        CHECK_NEAR(boost->level(), 0.75, 0.05);          // 1.5 in 0..2
        CHECK(gate->level() > 0.9f);
        CHECK(out->level() > 0.8f);                      // 0 dBFS, near the top of -48..+6
        rig.player.peak = 0.0f;
        rig.host.tick(240);
        CHECK(out->level() < 0.1f);
        return 0;
    }

    // A keyboard keeps its room and is never drawn, hit or listed (there is no MIDI).
    TEST(testKeyboardHidden, "keyboard.hidden")
    {
        Rig rig;
        CHECK(rig.panel.keyboardRects().empty());        // on another page
        CHECK(rig.page("FRAMES"));
        const std::vector<funkgui::Rect> keys = rig.panel.keyboardRects();
        CHECK(keys.size() == 1);
        if (keys.empty())
            return 0;
        CHECK(keys[0].w > 100.0f && keys[0].h > 40.0f);
        for (const funkgui::A11yItem& it : rig.host.accessibility())
            CHECK_MSG(!(std::fabs(it.bounds.y - keys[0].y) < 0.5f && std::fabs(it.bounds.h - keys[0].h) < 0.5f
                        && it.bounds.x >= keys[0].x - 0.5f && it.bounds.right() <= keys[0].right() + 0.5f),
                      "a key is listed: " + it.title);
        const int sets = rig.player.sets, frames = rig.player.frames;
        rig.host.click(keys[0].centreX(), keys[0].centreY());
        rig.host.drag(keys[0].x + 10.0f, keys[0].centreY(), keys[0].right() - 10.0f, keys[0].centreY());
        CHECK(rig.player.sets == sets && rig.player.frames == frames);
        CHECK(rig.panel.focus() < 0);
        return 0;
    }

    // Accessibility: every control shown is listed once, with a title, inside the panel; an action moves it.
    TEST(testA11y, "a11y")
    {
        Rig rig;
        const std::vector<funkgui::A11yItem> items = rig.host.accessibility();
        std::set<uint32_t> ids;
        bool sawCutoff = false;
        for (const funkgui::A11yItem& it : items)
        {
            CHECK_MSG(ids.insert(it.id).second, "an id twice: " + it.title);
            CHECK_MSG(!it.title.empty(), "an item with no title");
            sawCutoff = sawCutoff || it.title.find("CUTOFF") != std::string::npos || it.title.find("cutoff") != std::string::npos;
        }
        CHECK(sawCutoff);
        CHECK(items.size() >= 15);
        for (const funkgui::A11yItem& it : items)
            if (it.role == funkgui::A11yRole::slider && (it.title.find("RESONANCE") != std::string::npos
                                                         || it.title.find("resonance") != std::string::npos))
            {
                const double before = rig.player.plain("resonance");
                rig.panel.a11yAction(it.id, funkgui::A11yAction::increment, 0.0);
                CHECK(rig.player.plain("resonance") > before);
            }
        return 0;
    }

    // What shaderc is handed for a shader: FunkGui's head, the panel's names, the text's strings as glyph codes, and
    // the files it includes beside it.
    TEST(testShaderText, "shader.text")
    {
        const Sources files = kindsFiles();
        const std::vector<PanelShader> shaders = readGuiShaders(files[0], true);
        CHECK(shaders.size() == 1);
        if (shaders.empty())
            return 0;
        CHECK(shaders[0].file == "glow.sc");
        CHECK(shaders[0].inputs.size() == 5);
        const std::string prelude = "$input v_uv, v_px\n#include <bgfx_shader.sh>\nuniform vec4 u_params[4];\n";
        const ShaderFiles made = shaderFiles("glow.sc", files[1].text, shaders[0].inputs, prelude);
        CHECK_MSG(made.error.empty(), made.error);
        const std::string& s = made.source;
        CHECK(s.rfind(prelude, 0) == 0);
        CHECK(s.find("#define u_time (u_viewSize.w)") != std::string::npos);
        CHECK(s.find("#define in0 (u_params[0].x)") != std::string::npos);
        CHECK(s.find("#define in15 (u_params[3].w)") != std::string::npos);
        CHECK(s.find("#define in_glow in0") != std::string::npos);
        CHECK(s.find("#define in_shared__showRpm in1") != std::string::npos);
        CHECK(s.find("#define in_out in2") != std::string::npos);
        CHECK(s.find("#define in_pointer_x in3") != std::string::npos);
        CHECK(s.find("#define in_pointer_hover in4") != std::string::npos);
        CHECK_MSG(s.find("\"RPM\"") == std::string::npos, "the string literal is still there");
        CHECK(s.find("vec4(") != std::string::npos);
        int line = 1;
        for (std::size_t i = 0; i < s.size() && line < made.bodyLine; ++i)
            line += s[i] == '\n' ? 1 : 0;
        CHECK(made.bodyLine > 4);
        std::set<std::string> included;
        for (const auto& f : made.includes)
            included.insert(f.first);
        CHECK(included.count("funk/text.sh") == 1 && included.count("funk/shapes.sh") == 1);
        CHECK(!shaderAnimates(files[1].text));
        CHECK(shaderAnimates("void main() { gl_FragColor = vec4(sin(u_time)); }"));
        CHECK(!shaderAnimates("// u_time\nvoid main() { gl_FragColor = vec4(1.0); }"));
        // A string the font cannot draw, and a file there is none of, are notes on their lines.
        const ShaderFiles bad = shaderFiles("bad.sc", "void main()\n{\n    float a = text(v_px, vec2(0.0), 12.0, 0.0, \"\x01\");\n}\n",
                                            {}, prelude);
        CHECK_MSG(bad.error.rfind("bad.sc:3:", 0) == 0, bad.error);
        const ShaderFiles missing = shaderFiles("inc.sc", "#include \"nosuch.sh\"\nvoid main() {}\n", {}, prelude);
        CHECK_MSG(missing.error.rfind("inc.sc:1:", 0) == 0, missing.error);
        // A header of the panel's own is written beside the shader, guarded.
        const Sources headers{ { "ink.sh", "vec3 ink() { return vec3(1.0); }\n" } };
        const ShaderFiles own = shaderFiles("own.sc", "#include \"ink.sh\"\nvoid main() { gl_FragColor = vec4(ink(), 1.0); }\n", {},
                                            prelude, headers);
        CHECK(own.error.empty() && own.includes.size() == 1 && own.includes[0].first == "ink.sh");
        CHECK(own.includes.empty() || own.includes[0].second.find("#ifndef TEE_INCLUDED_") == 0);
        return 0;
    }

    // With no GPU layer in the build a shader element is a placeholder that says why, and is still laid out.
    TEST(testShaderPlaceholder, "shader.placeholder")
    {
        Rig rig;
        CHECK(rig.page("DISPLAYS"));
        CHECK(rig.panel.view().shaders().size() >= 1);
        const auto& item = static_cast<const GuiShaderItem&>(*rig.panel.items()[static_cast<std::size_t>(rig.panel.view().shaders()[0])]);
        CHECK(item.shown());
        CHECK_NEAR(item.cell().w, 200.0, 0.5);
        CHECK_NEAR(item.cell().h, 120.0, 0.5);
#if TEE_UI_GPU
        std::printf("  (a GPU layer is in this build: the shader's program is %u)\n", item.program());
#else
        CHECK(item.program() == 0);
        CHECK_MSG(item.why().find("no GPU renderer") != std::string::npos, item.why());
#endif
        // Its inputs follow what they name: the parameter's turn, the field, the output's level.
        rig.player.set(rig.player.find("glow"), 0.75);
        rig.player.setTelemetry("showRpm", 3000.0);
        rig.player.peak = 0.5f;
        rig.host.tick(3);
        CHECK_NEAR(item.inputs()[0], 0.75, 1e-4);
        CHECK_NEAR(item.inputs()[1], 3000.0, 0.5);
        CHECK_NEAR(item.inputs()[2], 0.5, 1e-4);
        return 0;
    }

    // ---- RefPanel ----------------------------------------------------------------------------------------------------
    const char* kWithKeys = "column gap=8 pad=8 {\n  row {\n    knob level\n    knob drive\n"
                            "    number shared->showRpm label=\"RPM\"\n  }\n  keyboard from=36 keys=49\n}\n";
    const char* kNoKeys = "row pad=8 {\n  knob level\n  knob drive\n}\n";

    // The strip stands where the keyboard was and drives the Player's controls.
    TEST(testRefStrip, "ref.strip")
    {
        MockPlayer player;
        RefPanel panel(Sources{ { "panel.gui", kWithKeys } }, player, {});
        funkgui::HeadlessHost host(panel, 0, 2.0f);
        host.tick(2);
        CHECK_MSG(panel.notes().find("left out") == std::string::npos, panel.notes());
        const std::vector<funkgui::Rect> keys = panel.keyboardRects();
        CHECK(keys.size() == 1);
        const funkgui::Rect strip = panel.stripRect();
        CHECK(!keys.empty() && strip.x == keys[0].x && strip.y == keys[0].y && strip.w == keys[0].w);
        // START: held is the starter, let go leaves the ignition on. OFF switches it off.
        const funkgui::Rect start = panel.startRect();
        host.press(start.centreX(), start.centreY());
        CHECK(player.controls.ignition == 2);
        host.tick(30);
        CHECK(player.controls.ignition == 2);
        host.release();
        CHECK(player.controls.ignition == 1);
        const funkgui::Rect off = panel.offRect();
        host.click(off.centreX(), off.centreY());
        CHECK(player.controls.ignition == 0);
        // The throttle: the pedal follows the pointer and comes back up.
        const funkgui::Rect bar = panel.throttleRect();
        CHECK(bar.w > 100.0f);
        host.press(bar.x + bar.w * 0.5f, bar.centreY());
        CHECK_NEAR(player.controls.throttle, 0.5, 0.02);
        host.moveTo(bar.right() + 50.0f, bar.centreY(), 4);
        CHECK_NEAR(player.controls.throttle, 1.0, 1e-6);
        host.release();
        CHECK_NEAR(player.controls.throttle, 0.0, 1e-6);
        // The wheel sets where it rests; a drag still comes back to that; a double-click clears it.
        host.wheel(bar.centreX(), bar.centreY(), 1.0f);
        host.wheel(bar.centreX(), bar.centreY(), 1.0f);
        CHECK_NEAR(player.controls.throttle, 0.10, 1e-4);
        host.drag(bar.x + bar.w * 0.8f, bar.centreY(), bar.x + bar.w * 0.9f, bar.centreY());
        CHECK_NEAR(player.controls.throttle, 0.10, 1e-4);
        host.doubleClick(bar.centreX(), bar.centreY());
        CHECK_NEAR(player.controls.throttle, 0.0, 1e-6);
        // The shifts count presses; the gear is the telemetry's.
        const funkgui::Rect up = panel.shiftRect(true), down = panel.shiftRect(false);
        host.click(up.centreX(), up.centreY());
        host.click(up.centreX(), up.centreY());
        host.click(down.centreX(), down.centreY());
        CHECK(player.controls.shiftUp == 2 && player.controls.shiftDown == 1);
        host.tick(2);
        CHECK(panel.gearText() == "N");
        player.setTelemetry("showGear", 3.0);
        host.tick(2);
        CHECK(panel.gearText() == "3");
        player.setTelemetry("showGear", -1.0);
        host.tick(2);
        CHECK(panel.gearText() == "R");
        // The interface's own controls still work beside it.
        const funkgui::Rect knob = panel.controlRect("level");
        host.drag(knob.centreX(), knob.centreY(), knob.centreX(), knob.centreY() - 40.0f);
        CHECK(player.plain("level") > 0.6);
        golden("ref.strip", host);
        // An application that merges the strip with its own input takes the frames instead.
        MockPlayer other;
        int taken = 0;
        RefPanel::Options o;
        o.controls = [&](const tee::ControlsFrame& f) { taken += f.ignition; };
        RefPanel hooked(Sources{ { "panel.gui", kWithKeys } }, other, o);
        funkgui::HeadlessHost host2(hooked, 0, 2.0f);
        host2.tick(1);
        host2.click(hooked.startRect().centreX(), hooked.startRect().centreY());
        CHECK(taken == 3 && other.frames == 0);          // 2 at the press, 1 at the release
        return 0;
    }

    // A panel file with no keyboard gets the strip under it; no file at all says so and still runs.
    TEST(testRefFallback, "ref.fallback")
    {
        MockPlayer player;
        RefPanel panel(Sources{ { "panel.gui", kNoKeys } }, player, {});
        funkgui::HeadlessHost host(panel, 0, 2.0f);
        host.tick(2);
        const funkgui::Rect strip = panel.stripRect();
        CHECK(strip.w > 0.0f && strip.bottom() <= static_cast<float>(panel.height()));
        CHECK(strip.y >= panel.controlRect("level").bottom());
        host.click(panel.startRect().centreX(), panel.startRect().centreY());
        CHECK(player.controls.ignition == 1 && player.frames == 2);
        golden("ref.fallback", host);
        host.resize(900, 500);
        host.tick(2);
        CHECK(panel.stripRect().bottom() <= 500.0f && panel.stripRect().w > 800.0f);

        MockPlayer none;
        RefPanel empty(Sources{}, none, {});
        funkgui::HeadlessHost host3(empty, 0, 2.0f);
        host3.tick(2);
        host3.draw();
        CHECK(empty.notes().find("no .gui file") != std::string::npos);
        host3.click(empty.startRect().centreX(), empty.startRect().centreY());
        CHECK(none.controls.ignition == 1);              // the strip is there all the same
        return 0;
    }

    int refPanel();                                      // RefTests.inc: the reference's own panel
#include "RefTests.inc"
    const Add addRefPanel("ref.panel", refPanel);

    int run(const std::string& name)
    {
        for (const auto& t : tests())
            if (t.first == name)
            {
                std::printf("ui.%s\n", name.c_str());
                const int before = g_fails;
                const int status = t.second();
                if (status == kSkip && g_fails == before)
                {
                    std::printf("  skipped\n");
                    return kSkip;
                }
                std::printf("  %s\n", g_fails == before ? "ok" : "FAILED");
                return g_fails == before ? 0 : 1;
            }
        std::printf("no test named %s (--list)\n", name.c_str());
        return 2;
    }
}

int main(int argc, char** argv)
{
    const funkgui::HeadlessGuiScope gui;
    std::string name;
    for (int i = 1; i < argc; ++i)
    {
        const std::string a = argv[i];
        if (a == "--bless")
            g_bless = true;
        else if (a == "--list")
        {
            for (const auto& t : tests())
                std::printf("%s\n", t.first.c_str());
            return 0;
        }
        else
            name = a;
    }
    if (name.empty())
    {
        std::printf("usage: tee-ui-tests <name> | all | --list  [--bless]\n");
        return 2;
    }
    if (name != "all")
        return run(name);
    int failed = 0;
    for (const auto& t : tests())
        failed += run(t.first) == 1 ? 1 : 0;
    std::printf("%d of %d failed\n", failed, static_cast<int>(tests().size()));
    return failed == 0 ? 0 : 1;
}
