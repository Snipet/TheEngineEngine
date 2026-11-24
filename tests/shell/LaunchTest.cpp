// tests/shell/LaunchTest.cpp: which build the window opens with (app/Launch.h): a --preset's (nothing here), none with
// --reference, the remembered one, else the first build (content/builds/first.json, or --first-build), else none. The
// window itself is shell.window.first and shell.window.last (tests/shell/CMakeLists.txt).
#include "Launch.h"

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

using namespace tee;
namespace fs = std::filesystem;

namespace
{
    fs::path freshContent()
    {
        const fs::path dir = fs::path(TEE_SOURCE_DIR) / "out" / "shell" / "launch" / "content";
        std::error_code ec;
        fs::remove_all(dir, ec);
        fs::create_directories(dir / "builds", ec);
        return dir;
    }
}

TEST_CASE("app: the build a launch opens: preset, reference, the remembered one, the first build, none", "[app][launch]")
{
    const fs::path content = freshContent();
    const fs::path first = content / "builds" / "first.json";
    const fs::path last = content / "builds" / "mine.json";
    std::ofstream(last) << "{}";
    app::Options o;

    // no first build shipped, nothing remembered: none (the simulator's defaults, as before)
    app::LaunchBuild l = app::chooseLaunchBuild(o, false, "", content);
    CHECK(l.file.empty());
    CHECK(std::string(l.what).find("none") == 0);

    // the first build, once content has it
    std::ofstream(first) << "{}";
    l = app::chooseLaunchBuild(o, false, "", content);
    CHECK(l.file == first);
    CHECK(std::string(l.what) == "the first build");

    // the remembered build comes first; a remembered file that is gone does not
    l = app::chooseLaunchBuild(o, false, last.string(), content);
    CHECK(l.file == last);
    CHECK(std::string(l.what) == "the last build");
    l = app::chooseLaunchBuild(o, false, (content / "gone.json").string(), content);
    CHECK(l.file == first);

    // a machine's run, its settings not chosen for it: not the person's build, the first build all the same
    const char* chosen = std::getenv("TEE_SETTINGS_DIR");
    if (chosen == nullptr || chosen[0] == '\0')
        CHECK(app::chooseLaunchBuild(o, true, last.string(), content).file == first);

    // --first-build in place of content's; one that is not there: none
    app::Options given = o;
    given.firstBuild = last.string();
    CHECK(app::chooseLaunchBuild(given, false, "", content).file == last);
    given.firstBuild = (content / "nowhere.json").string();
    CHECK(app::chooseLaunchBuild(given, false, "", content).file.empty());

    // --reference: none, whatever is remembered
    app::Options reference = o;
    reference.reference = true;
    CHECK(app::chooseLaunchBuild(reference, false, last.string(), content).file.empty());

    // --preset: none here (the preset is applied before the window)
    app::Options preset = o;
    preset.preset = "v8";
    CHECK(app::chooseLaunchBuild(preset, false, last.string(), content).file.empty());

    // no content at all: none
    CHECK(app::chooseLaunchBuild(o, false, "", fs::path()).file.empty());
}
