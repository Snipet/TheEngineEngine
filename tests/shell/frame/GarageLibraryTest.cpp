// tests/shell/frame/GarageLibraryTest.cpp: the GARAGE workspace's Library as the shell implements it
// (app/GarageLibrary.h) over the simulator's document (sim/engine.fm), the shipped builds (content/builds, its parts'
// folders too) and a user's folder of the test's own: list, parts, values, current, load whole and a part, save,
// save as, duplicate, delete, revert, unsaved, notes; and a shipped build never written or deleted.
#include "GarageLibrary.h"

#include "files/BuildFiles.h"
#include "player/SimAdapter.h"
#include "tee/SimHost.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <chrono>
#include <memory>
#include <thread>
#include <string>

using namespace tee;
namespace fs = std::filesystem;
using ui::garage::Part;

namespace
{
    const ui::garage::BuildSummary* byFile(const app::GarageLibrary& lib, const fs::path& file)
    {
        for (const auto& b : lib.builds())
            if (fs::path(b.id).filename() == file.filename() && fs::path(b.id).parent_path().filename() == file.parent_path().filename())
                return &b;
        return nullptr;
    }

    double valueOf(const ui::garage::BuildSummary& b, const std::string& path)
    {
        for (const auto& v : b.values)
            if (v.path == path && !v.v.empty())
                return v.v[0];
        return -1e30;
    }
}

TEST_CASE("shell: the garage's library over the builds on disk", "[shell][files][garage]")
{
    const fs::path user = fs::path(TEE_SOURCE_DIR) / "out" / "shell" / "files" / "garage-user";
    std::error_code ec;
    fs::remove_all(user, ec);
    const fs::path shipped = fs::path(TEE_SOURCE_DIR) / "content" / "builds";

    player::SimAdapter::Options o;
    o.directory = fs::path(TEE_SOURCE_SIM_DIR);
    std::string errors;
    const std::unique_ptr<player::SimAdapter> sim = player::SimAdapter::create(o, errors);
    INFO(errors);
    REQUIRE(sim != nullptr);
    tee::Build& build = sim->build();
    files::BuildFile doc(build);
    {
        app::GarageLibrary lib(doc, nullptr, shipped, user);

        // the shipped builds and parts, each with what its file sets
        REQUIRE(lib.builds().size() >= 30);
        const auto* v8 = byFile(lib, shipped / "v8.json");
        REQUIRE(v8 != nullptr);
        CHECK(v8->part == Part::whole);
        CHECK_FALSE(v8->user);
        CHECK_FALSE(v8->current);
        CHECK_FALSE(v8->values.empty());
        const auto* cam = byFile(lib, shipped / "cams" / "race.json");
        REQUIRE(cam != nullptr);
        CHECK(cam->part == Part::engine);          // the valve section alone: the engine's part
        CHECK(cam->name == "Race cam");
        // (its fields are the lab valvetrain's, not in sim/engine.fm yet: it sets nothing this program knows)
        const auto* six = byFile(lib, shipped / "engines" / "ref-10-inline-6.json");
        REQUIRE(six != nullptr);
        CHECK(six->part == Part::engine);
        CHECK(valueOf(*six, "engine.cylinders") == 6.0);
        const auto* car = byFile(lib, shipped / "cars" / "rwd-v8-coupe.json");
        REQUIRE(car != nullptr);
        CHECK(car->part == Part::car);
        const std::string camId = cam->id, sixId = six->id, carId = car->id, v8Id = v8->id;

        // a whole build: the document's file, current, clean
        std::string error;
        REQUIRE(lib.load(v8Id, Part::whole, error));
        CHECK(doc.file() == fs::path(v8Id));
        CHECK_FALSE(lib.unsaved());
        CHECK(byFile(lib, shipped / "v8.json")->current);
        // a part's file is not a whole build
        CHECK_FALSE(lib.load(camId, Part::whole, error));

        // a straight six's layout into this build: the fields its file sets and nothing else (not the compression,
        // the boost or the car, which the file does not name); unsaved; still the v8 current
        const double compression = build.get("engine.compression");
        const double boost = build.get("boost.level");
        REQUIRE(build.get("engine.cylinders") != 6.0);
        REQUIRE(lib.load(sixId, Part::engine, error));
        CHECK(build.get("engine.cylinders") == 6.0);
        CHECK(build.get("engine.compression") == compression);
        CHECK(build.get("boost.level") == boost);
        CHECK(lib.unsaved());
        CHECK(byFile(lib, shipped / "v8.json")->current);
        // the current one's values are the document's
        CHECK(valueOf(*byFile(lib, shipped / "v8.json"), "engine.cylinders") == 6.0);
        // one undo step
        REQUIRE(build.undo());
        CHECK_FALSE(lib.unsaved());
        REQUIRE(build.redo());

        // a shipped build is not written over; save as makes the user's, current and clean
        CHECK_FALSE(lib.save(error));
        REQUIRE(lib.saveAs("V8 race cam", error));
        INFO(error);
        CHECK(doc.file() == user / "V8 race cam.json");
        CHECK_FALSE(lib.unsaved());
        const auto* mine = byFile(lib, user / "V8 race cam.json");
        REQUIRE(mine != nullptr);
        CHECK(mine->user);
        CHECK(mine->current);
        CHECK(mine->name == "V8 race cam");

        // notes, saved over its own file
        lib.setNotes("with the race cam");
        CHECK(lib.unsaved());
        CHECK(lib.notes() == "with the race cam");
        REQUIRE(lib.save(error));
        CHECK_FALSE(lib.unsaved());

        // revert: back to the file
        REQUIRE(build.set("engine.compression", compression + 2.0));
        CHECK(lib.unsaved());
        REQUIRE(lib.revert(error));
        CHECK(build.get("engine.compression") == compression);
        CHECK_FALSE(lib.unsaved());

        // the car into this build
        REQUIRE(lib.load(carId, Part::car, error));
        CHECK(lib.unsaved());

        // duplicate (named "<name> copy", numbered when taken) and delete; a shipped one is not deleted
        std::string copy, copy2;
        REQUIRE(lib.duplicate(v8Id, copy, error));
        REQUIRE(lib.duplicate(v8Id, copy2, error));
        CHECK(copy != copy2);
        CHECK(fs::path(copy2).stem().string().find("copy 2") != std::string::npos);
        const auto* c = byFile(lib, fs::path(copy));
        REQUIRE(c != nullptr);
        CHECK(c->user);
        REQUIRE(lib.remove(copy, error));
        CHECK(byFile(lib, fs::path(copy)) == nullptr);
        CHECK_FALSE(lib.remove(v8Id, error));
        CHECK(fs::is_regular_file(v8Id));

        // without a program there are no curves, and nothing is asked
        lib.tick(0.1f);
        CHECK(byFile(lib, shipped / "v8.json")->curve.empty());
    }

    // with the program: each whole build's curve is made in the background, one at a time (or the dyno says why not)
    {
        app::GarageLibrary lib(doc, sim->host().sim(), shipped, user);
        CHECK(byFile(lib, shipped / "default.json")->curve.empty());           // nothing blocks: empty until made
        const auto made = [&] {
            for (const auto& b : lib.builds())
                if (!b.curve.empty())
                    return true;
            return false;
        };
        for (int i = 0; i < 3000 && lib.curveError().empty() && !made(); ++i)
        {
            lib.tick(0.02f);
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        INFO("error: " << lib.curveError() << " making: " << lib.makingCurves());
        CHECK((!lib.curveError().empty() || made()));
        for (const auto& b : lib.builds())
            if (!b.curve.empty())
            {
                CHECK(b.part == Part::whole);
                CHECK(b.curve.peakTorque().value > 10.0);       // N m
                CHECK(b.curve.peakPower().value > 1000.0);      // W
            }
    }
}
