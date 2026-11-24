// tests/shell/BuildFilesTest.cpp: builds on disk (shell/files/BuildFiles.h): the dirty flag, open and save, a part
// apart from the rest, the recent files, and the library the GARAGE workspace lists (shipped and the user's: list,
// summary, load, save, duplicate, delete). On the simulator's own document (sim/engine.fm), in a folder of the test's.
#include "files/BuildFiles.h"
#include "player/SimAdapter.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

using namespace tee;
namespace fs = std::filesystem;

namespace
{
    fs::path freshFolder(const char* name)
    {
        const fs::path dir = fs::path(TEE_SOURCE_DIR) / "out" / "shell" / "files" / name;
        std::error_code ec;
        fs::remove_all(dir, ec);
        fs::create_directories(dir, ec);
        return dir;
    }

    std::unique_ptr<player::SimAdapter> simulator()
    {
        player::SimAdapter::Options o;
        o.directory = fs::path(TEE_SOURCE_SIM_DIR);
        std::string errors;
        std::unique_ptr<player::SimAdapter> sim = player::SimAdapter::create(o, errors);
        INFO(errors);
        REQUIRE(sim != nullptr);
        return sim;
    }
}

TEST_CASE("shell: a build saved, changed and opened again; the dirty flag; a part apart", "[shell][files]")
{
    const fs::path dir = freshFolder("document");
    const auto sim = simulator();
    tee::Build& build = sim->build();
    files::BuildFile doc(build);

    CHECK_FALSE(doc.dirty());
    CHECK(doc.file().empty());
    CHECK(doc.title() == "Untitled");
    std::string error;
    CHECK_FALSE(doc.save(error));               // untitled: the caller asks for a file
    CHECK_FALSE(error.empty());

    const double was = build.get("engine.compression");
    REQUIRE(build.set("engine.compression", was + 1.0));
    CHECK(doc.dirty());
    CHECK(doc.title() == "Untitled *");

    build.name = "Mine";
    REQUIRE(doc.saveAs(dir / "mine", BuildPart::Whole, error));      // ".json" added
    INFO(error);
    CHECK(doc.file() == dir / "mine.json");
    CHECK(fs::is_regular_file(dir / "mine.json"));
    CHECK_FALSE(doc.dirty());
    CHECK(doc.title() == "Mine");

    // an undo makes it dirty, a redo clean again (the flag is the values', not a count of edits)
    REQUIRE(build.undo());
    CHECK(doc.dirty());
    REQUIRE(build.redo());
    CHECK_FALSE(doc.dirty());

    // the notes count too
    build.notes = "a note";
    CHECK(doc.dirty());
    REQUIRE(doc.save(error));
    CHECK_FALSE(doc.dirty());

    // a new build: defaults, no file, clean
    doc.makeNew();
    CHECK(doc.file().empty());
    CHECK_FALSE(doc.dirty());
    CHECK(build.get("engine.compression") == was);

    // opened again: the value is back, the file is it, clean, and it is the newest recent file
    std::vector<std::string> warnings;
    REQUIRE(doc.open(dir / "mine.json", BuildPart::Whole, warnings, error));
    CHECK(build.get("engine.compression") == was + 1.0);
    CHECK(build.name == "Mine");
    CHECK(build.notes == "a note");
    CHECK_FALSE(doc.dirty());
    REQUIRE_FALSE(doc.recent().list().empty());
    CHECK(doc.recent().list().front() == dir / "mine.json");

    // the car alone: written apart; loading it touches only the car, and makes the build dirty, not the file another
    const double mass = build.get("car.mass");
    REQUIRE(doc.saveAs(dir / "car-part.json", BuildPart::Car, error));
    CHECK(doc.file() == dir / "mine.json");
    REQUIRE(build.set("car.mass", mass + 100.0));
    REQUIRE(build.set("engine.compression", was));
    REQUIRE(doc.open(dir / "car-part.json", BuildPart::Car, warnings, error));
    CHECK(build.get("car.mass") == mass);
    CHECK(build.get("engine.compression") == was);          // not the car's: left as it was
    CHECK(doc.file() == dir / "mine.json");
    CHECK(doc.dirty());
    // a part's file is refused as another part, and as a whole build
    CHECK_FALSE(doc.open(dir / "car-part.json", BuildPart::Engine, warnings, error));
    CHECK_FALSE(error.empty());
    // a file that is no build
    {
        std::ofstream(dir / "junk.json") << "{ not json";
    }
    CHECK_FALSE(doc.open(dir / "junk.json", BuildPart::Whole, warnings, error));
}

TEST_CASE("shell: the recent files: newest first, once each, at most ten, kept as text", "[shell][files]")
{
    const fs::path dir = freshFolder("recent");
    files::RecentFiles r;
    for (int i = 0; i < 12; ++i)
    {
        const fs::path f = dir / ("b" + std::to_string(i) + ".json");
        std::ofstream(f) << "{}";
        r.add(f);
    }
    REQUIRE(r.list().size() == files::RecentFiles::kMost);
    CHECK(r.list().front() == dir / "b11.json");
    r.add(dir / "b5.json");
    CHECK(r.list().front() == dir / "b5.json");
    CHECK(r.list().size() == files::RecentFiles::kMost);

    files::RecentFiles back;
    back.load(r.save());
    CHECK(back.list() == r.list());

    fs::remove(dir / "b11.json");
    back.prune();
    CHECK(back.list().size() == files::RecentFiles::kMost - 1);
    for (const fs::path& f : back.list())
        CHECK(f != dir / "b11.json");
}

TEST_CASE("shell: the library: shipped and the user's builds, listed, summed up, loaded, saved, duplicated, deleted",
          "[shell][files][library]")
{
    const fs::path user = freshFolder("library-user");
    const fs::path shipped = fs::path(TEE_SOURCE_DIR) / "content" / "builds";
    const files::BuildLibrary lib(shipped, user);
    const auto sim = simulator();
    files::BuildFile doc(sim->build());

    // the shipped builds are listed with what their files say; the parts' folders are not builds
    std::vector<files::BuildSummary> all = lib.list();
    REQUIRE(all.size() >= 3);
    files::BuildSummary v8;
    REQUIRE(lib.summary("v8", v8));
    CHECK_FALSE(v8.user);
    CHECK_FALSE(v8.title.empty());
    CHECK(v8.part.empty());
    files::BuildSummary d;
    REQUIRE(lib.summary("default", d));
    CHECK(d.layout == "V8 crossplane");
    CHECK(d.displacement > 0.005);
    for (const files::BuildSummary& s : all)
        CHECK(s.part.empty());
    CHECK_FALSE(lib.summary("no such build", d));

    // loaded by name: the document is that file, clean
    std::vector<std::string> warnings;
    std::string error;
    REQUIRE(lib.load(doc, "large-diesel", warnings, error));
    CHECK(doc.file().filename() == "large-diesel.json");
    CHECK_FALSE(doc.dirty());

    // saved as the user's: listed as the user's, named in the file
    REQUIRE(doc.build().set("engine.compression", 15.0));
    REQUIRE(lib.save(doc, "My Diesel", error));
    INFO(error);
    CHECK(doc.file() == user / "My Diesel.json");
    CHECK_FALSE(doc.dirty());
    files::BuildSummary mine;
    REQUIRE(lib.summary("My Diesel", mine));
    CHECK(mine.user);
    CHECK(mine.title == "My Diesel");

    // a user's build of a shipped one's name is listed once, as the user's
    REQUIRE(lib.save(doc, "v8", error));
    int v8s = 0;
    for (const files::BuildSummary& s : lib.list())
        if (s.name == "v8")
        {
            ++v8s;
            CHECK(s.user);
        }
    CHECK(v8s == 1);

    // duplicated, not over another; deleted, the user's only
    REQUIRE(lib.duplicate("My Diesel", "Copy", error));
    REQUIRE(lib.summary("Copy", mine));
    CHECK(mine.title == "Copy");
    CHECK_FALSE(lib.duplicate("My Diesel", "Copy", error));
    CHECK_FALSE(lib.remove("default", error));
    CHECK(fs::is_regular_file(shipped / "default.json"));
    REQUIRE(lib.remove("Copy", error));
    CHECK_FALSE(lib.summary("Copy", mine));
    REQUIRE(lib.remove("v8", error));            // the user's v8: the shipped one is listed again
    REQUIRE(lib.summary("v8", v8));
    CHECK_FALSE(v8.user);

    // names a file system refuses
    for (const char* bad : { "", "a/b", "a\\b", "c:d", " lead", "trail.", "q?" })
    {
        std::string why;
        CHECK_FALSE(files::BuildLibrary::validName(bad, why));
        CHECK_FALSE(why.empty());
    }
    std::string why;
    CHECK(files::BuildLibrary::validName("Twin Turbo 2.0 (E85)", why));
}
