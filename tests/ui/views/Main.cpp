// tests/ui/views/Main.cpp: tee-view-tests, the frame's and the workspaces' headless tests (package U1 and those after
// it). ViewKit.h says how a test is written.
//
//   tee-view-tests <name>          one test; 0 when it passes, 1 when it fails, 77 when it cannot run here
//   tee-view-tests all             every test
//   tee-view-tests --list          the names
//   ... --bless                    write the goldens instead of comparing (after looking at the PNGs)
#include "ViewKit.h"

#include "kit/bind/Binding.h"

#include <funkgui/panel/HeadlessGuiScope.h>
#include <funkgui/text/FontService.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <set>
#include <sstream>

namespace tee::test::views
{
    namespace
    {
        bool g_bless = false;
        constexpr int kSkip = 77;
        const std::filesystem::path kGoldenDir = TEE_VIEW_GOLDEN_DIR;
        const std::filesystem::path kOutDir = TEE_UI_OUT_DIR;

        std::vector<std::pair<std::string, Test>>& tests()
        {
            static std::vector<std::pair<std::string, Test>> all;
            return all;
        }

        std::string readText(const std::filesystem::path& p)
        {
            std::ifstream in(p, std::ios::binary);
            return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
        }

        std::string fingerprintText(const funkgui::PrimList& frame)
        {
            const funkgui::Fingerprint f = funkgui::fingerprint(frame);
            char buf[512];
            std::snprintf(buf, sizeof buf,
                          "geometry %016llx\ntext %016llx\nstatics %d\nlive %d\ntexts %d\nrrects %d\nsegments %d\nareas %d\n"
                          "shaders %d\nimages %d\nsize %d x %d\n",
                          static_cast<unsigned long long>(f.geometry), static_cast<unsigned long long>(f.text), f.statics,
                          f.live, f.texts, f.rrects, f.segments, f.areas, f.shaders, f.images, frame.info.logicalW,
                          frame.info.logicalH);
            return buf;
        }

        std::string writePicture(const std::string& name, const funkgui::PrimList& frame)
        {
            const std::string png = (outDir() / (name + ".png")).string();
            if (!tee::test::writePng(funkgui::rasterise(frame, funkgui::FontService::get().atlas(), 1), png))
                std::printf("  (could not write %s)\n", png.c_str());
            return png;
        }
    }

    int& fails()
    {
        static int n = 0;
        return n;
    }
    bool blessing() { return g_bless; }

    void failAt(const char* file, int line, const std::string& what)
    {
        ++fails();
        std::printf("  FAIL %s:%d: %s\n", file, line, what.c_str());
    }

    Add::Add(const char* name, Test t) { tests().emplace_back(name, std::move(t)); }

    std::filesystem::path outDir()
    {
        std::error_code ec;
        std::filesystem::create_directories(kOutDir, ec);
        return kOutDir;
    }

    void picture(const std::string& name, funkgui::HeadlessHost& host) { writePicture(name, host.draw()); }

    void golden(const std::string& name, funkgui::HeadlessHost& host)
    {
        const funkgui::PrimList& frame = host.draw();
        const std::string now = fingerprintText(frame);
        const std::string png = writePicture(name, frame);
        const std::filesystem::path path = kGoldenDir / (name + ".txt");
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

    std::string inspectorNotes(const tee::ui::Workspace& ws, tee::ui::DemoRig& demo)
    {
        const std::string gui = ws.inspectorPage();
        if (gui.empty())
            return {};
        tee::ui::GuiPanel::Options o;
        o.width = 340;
        o.height = 600;
        tee::ui::FieldPage page(demo.doc, demo.units, demo.engine, tee::ui::Sources{ { std::string(ws.id()) + ".gui", gui } }, o);
        return page.notes();
    }

    std::string unnamedControls(funkgui::HeadlessHost& host)
    {
        using R = funkgui::A11yRole;
        std::string out;
        for (const funkgui::A11yItem& it : host.accessibility())
        {
            const bool control = it.role == R::slider || it.role == R::toggleButton || it.role == R::button
                              || it.role == R::comboBox || it.role == R::radioButton || it.role == R::listItem
                              || it.role == funkgui::a11yrole::editableText;
            if (control && it.visible && it.title.empty())
            {
                char b[160];
                std::snprintf(b, sizeof b, "an unnamed control (role %d) at %.0f,%.0f %.0fx%.0f, value '%s'\n",
                              static_cast<int>(it.role), static_cast<double>(it.bounds.x), static_cast<double>(it.bounds.y),
                              static_cast<double>(it.bounds.w), static_cast<double>(it.bounds.h), it.value.c_str());
                out += b;
            }
        }
        return out;
    }

    void typeKeys(funkgui::HeadlessHost& host, std::string_view spec)
    {
        std::string out;
        std::size_t i = 0;
        while (i < spec.size())
        {
            const std::size_t sp = std::min(spec.find(' ', i), spec.size());
            std::string word(spec.substr(i, sp - i));
            if (word == "enter")
                word = "return";
            if (!word.empty())
                out += (out.empty() ? "" : ",") + word;
            i = sp + 1;
        }
        host.keys(out.c_str());
    }

    namespace
    {
        // CMake's list of tests is this program's.
        VIEW_TEST(testNames, "views.names")
        {
            std::set<std::string> mine, theirs;
            for (const auto& t : tests())
                mine.insert(t.first);
            std::stringstream in(TEE_VIEW_TEST_NAMES);
            for (std::string n; std::getline(in, n, ',');)
                theirs.insert(n);
            for (const std::string& n : mine)
                CHECK_MSG(theirs.count(n) == 1, n + " is not found by tests/ui/CMakeLists.txt (VIEW_TEST(ident, \"name\") on one line)");
            for (const std::string& n : theirs)
                CHECK_MSG(mine.count(n) == 1, n + " is in CMake's list but not in the program");
            return 0;
        }

        int run(const std::string& name)
        {
            for (const auto& t : tests())
                if (t.first == name)
                {
                    std::printf("ui.%s\n", name.c_str());
                    const int before = fails();
                    const int status = t.second();
                    if (status == kSkip && fails() == before)
                    {
                        std::printf("  skipped\n");
                        return kSkip;
                    }
                    std::printf("  %s\n", fails() == before ? "ok" : "FAILED");
                    return fails() == before ? 0 : 1;
                }
            std::printf("no test named %s (--list)\n", name.c_str());
            return 2;
        }
    }
}

int main(int argc, char** argv)
{
    using namespace tee::test::views;
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
        std::printf("usage: tee-view-tests <name> | all | --list  [--bless]\n");
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
