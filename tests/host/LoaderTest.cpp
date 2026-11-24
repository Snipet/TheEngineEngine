// LoaderTest.cpp: one reading of a build file. tee::loadBuild / parseBuild (the test rigs' EngineBuild) read a file as
// tee::Build does: nested objects (spec.ecu.knock.on, spec.ecu.idle.speed, spec.ecu.maps.boost) are fields, not
// unknowns; a field the program lacks fails the rigs' strict call and is kept by the lenient one.
#include "tee/Build.h"
#include "tee/EnginePlayer.h"
#include "tee/Sim.h"

#include "Support.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>

using namespace tee;

namespace
{
    const std::filesystem::path kRoot = TEE_SOURCE_DIR;

    std::unique_ptr<EnginePlayer> player()
    {
        EnginePlayer::Options po;
        po.directory = kRoot / "sim";
        std::string error;
        auto p = EnginePlayer::create(po, error);
        INFO(error);
        REQUIRE(p != nullptr);
        return p;
    }
} // namespace

TEST_CASE("the rigs' loader reads a build's nested sections as tee::Build does, and round-trips them",
          "[build][loader][sim]")
{
    if (!std::filesystem::exists(kRoot / "sim" / "engine.fm"))
        SKIP("sim/engine.fm is not there yet");
    const auto p = player();
    const std::filesystem::path file = kRoot / "content/builds/archetypes/turbo-four-20-gdi.json";
    EngineBuild build;
    std::string error, warnings;
    REQUIRE(loadBuild(*p, file, build, error, warnings));
    INFO(warnings);
    CHECK(build.unknown.empty());
    // the nested blocks count (the file: idle 78.54 rad/s, limiter 712.094, knock control on, a boost map)
    REQUIRE(build.values.count("ecu.idle.speed") == 1);
    CHECK(static_cast<float>(build.values.at("ecu.idle.speed")[0]) == 78.54f);
    CHECK(static_cast<float>(build.values.at("ecu.limiter.speed")[0]) == 712.094f);
    CHECK(build.values.at("ecu.knock.on")[0] == 1.0);
    REQUIRE(build.values.count("ecu.maps.boost.z") == 1);
    CHECK(build.values.at("ecu.maps.boost.z")[0] == 120000.0);

    // every Spec field as tee::Build has it, element by element (as floats)
    std::string e;
    const auto sim = test::compiled(kRoot / "sim", {}, e);
    REQUIRE(sim != nullptr);
    Build reference(sim->fields());
    std::vector<std::string> said;
    REQUIRE(reference.loadFile(file, BuildPart::Whole, said, e));
    int compared = 0;
    for (const EnginePlayer::SpecField& f : p->specFields())
    {
        if (f.node->scalar == Scalar::None)
            continue;
        const int index = reference.fieldOf(f.path);
        if (index < 0 || reference.fields().all()[static_cast<std::size_t>(index)].table())
            continue;   // a table's own fields ("ecu.maps.boost.z"): checked by the map above
        for (std::size_t k = 0; k < f.count; ++k)
        {
            INFO(f.path << "[" << k << "]");
            CHECK(static_cast<float>(build.values.at(f.path)[k]) == static_cast<float>(reference.get(f.path, k)));
            ++compared;
        }
    }
    CHECK(compared > 100);

    // an array of choices by name (an exhaust's resonator on each side): read, not left at its default
    EngineBuild exhaust;
    REQUIRE(loadBuild(*p, kRoot / "content/builds/exhausts/dual-x-resonated-valved.json", exhaust, error, warnings));
    CHECK(exhaust.values.at("exhaust.resonator")[0] == 1.0);   // "Quarter-wave"
    CHECK(exhaust.values.at("exhaust.resonator")[1] == 1.0);

    // written and read again: the nested blocks written nested, the same values back
    const std::string json = buildJson(*p, build);
    CHECK(json.find("\"knock\": {") != std::string::npos);
    EngineBuild again;
    REQUIRE(parseBuild(*p, json, "again", again, error, warnings));
    for (const auto& [path, values] : build.values)
    {
        INFO(path);
        CHECK(again.values.at(path) == values);
    }
}

TEST_CASE("the rigs' loader fails on a field the program lacks; the lenient one keeps it, nested, and writes it back",
          "[build][loader][sim]")
{
    if (!std::filesystem::exists(kRoot / "sim" / "engine.fm"))
        SKIP("sim/engine.fm is not there yet");
    const auto p = player();
    const char* const json = R"({"format": "tee-build", "version": 1, "name": "x", "notes": "",
        "spec": {"ecu": {"idle": {"speed": 90.0}, "knock": {"on": false, "fromTheFuture": 3}}}})";
    EngineBuild build;
    std::string error, warnings;
    CHECK_FALSE(parseBuild(*p, json, "x.json", build, error, warnings));
    CHECK(error.find("spec.ecu.knock.fromTheFuture") != std::string::npos);

    error.clear();
    REQUIRE(parseBuild(*p, json, "x.json", build, error, warnings, false));
    CHECK(static_cast<float>(build.values.at("ecu.idle.speed")[0]) == 90.0f);
    CHECK(build.values.at("ecu.knock.on")[0] == 0.0);
    CHECK(build.unknown.count("ecu.knock.fromTheFuture") == 1);
    CHECK(warnings.find("ecu.knock.fromTheFuture") != std::string::npos);
    const std::string out = buildJson(*p, build);
    EngineBuild again;
    REQUIRE(parseBuild(*p, out, "again", again, error, warnings, false));
    CHECK(again.unknown == build.unknown);
    CHECK(again.values == build.values);
}
