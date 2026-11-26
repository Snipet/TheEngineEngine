// LoaderTest.cpp: one reading of a build file. tee::loadBuild / parseBuild (the test rigs' EngineBuild) read a file as
// tee::Build does: nested objects (spec.ecu.knock.on, spec.ecu.idle.speed, spec.ecu.maps.boost) are fields, not
// unknowns; a field the program lacks fails the rigs' strict call and is kept by the lenient one.
#include "tee/Build.h"
#include "tee/EnginePlayer.h"
#include "tee/Sim.h"

#include "Support.h"

#include <catch2/catch_approx.hpp>
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

TEST_CASE("the rigs' loader reads a build's nested sections as tee::Build does, round-trips them, and is strict",
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

    // (one player for both: an EnginePlayer is a compile of its own)
    // a field the program lacks: the rigs' strict call fails on it; the lenient one keeps it, nested, and writes it back
    const char* const lackingJson = R"({"format": "tee-build", "version": 1, "name": "x", "notes": "",
        "spec": {"ecu": {"idle": {"speed": 90.0}, "knock": {"on": false, "fromTheFuture": 3}}}})";
    EngineBuild lacking;
    CHECK_FALSE(parseBuild(*p, lackingJson, "x.json", lacking, error, warnings));
    CHECK(error.find("spec.ecu.knock.fromTheFuture") != std::string::npos);

    error.clear();
    REQUIRE(parseBuild(*p, lackingJson, "x.json", build, error, warnings, false));
    CHECK(static_cast<float>(build.values.at("ecu.idle.speed")[0]) == 90.0f);
    CHECK(build.values.at("ecu.knock.on")[0] == 0.0);
    CHECK(build.unknown.count("ecu.knock.fromTheFuture") == 1);
    CHECK(warnings.find("ecu.knock.fromTheFuture") != std::string::npos);
    const std::string out = buildJson(*p, build);
    EngineBuild back;
    REQUIRE(parseBuild(*p, out, "again", back, error, warnings, false));
    CHECK(back.unknown == build.unknown);
    CHECK(back.values == build.values);
}

TEST_CASE("a stated bore under its least is refused at a person's input: the inspector's accepts and the loader; 0 "
          "(derived) and the least itself are taken",
          "[build][loader][sim]")
{
    if (!std::filesystem::exists(kRoot / "sim" / "engine.fm"))
        SKIP("sim/engine.fm is not there yet");
    std::string e;
    const auto sim = test::compiled(kRoot / "sim", {}, e);
    REQUIRE(sim != nullptr);
    CHECK(lint(sim->fields()).empty());   // (the least's fields start their ranges at 0)
    Build build(sim->fields());
    if (build.fieldOf("exhaust.primD") < 0 || build.fieldOf("intake.runnerBore") < 0)
        SKIP("the program has no stated bores");
    CHECK(statedLeast("spec.exhaust.primD") == 0.010);
    CHECK(statedLeast("spec.intake.runnerBore") == 0.008);
    CHECK(statedLeast("spec.engine.bore") == 0.0);

    // the inspector's way: it asks before it sets
    std::string why;
    CHECK_FALSE(build.accepts("exhaust.primD", 0.005, &why));
    CHECK(why.find("10 mm") != std::string::npos);
    CHECK(build.accepts("exhaust.primD", 0.010));   // the least itself
    CHECK(build.accepts("exhaust.primD", 0.0));     // derived
    CHECK_FALSE(build.accepts("intake.runnerBore", 0.007));
    CHECK(build.accepts("intake.runnerBore", 0.009));
    CHECK_FALSE(build.accepts("exhaust.tailD", 0.001));
    CHECK(build.accepts("engine.bore", 0.001));      // no least: whatever its range says
    // a timeline's or a fuzzer's way (Build::set) takes any value: the program raises it (tel.exhaust.boreRaised)
    CHECK(build.set("exhaust.primD", 0.005));
    CHECK(build.get("exhaust.primD") == Catch::Approx(0.005));

    // a file's: refused, at its default (0, derived), and said
    const char* const json = R"({"format": "tee-build", "version": 1, "name": "x", "notes": "",
        "spec": {"exhaust": {"primD": [0.0005], "colD": [0.045]}, "intake": {"runnerBore": 0.004}}})";
    Build loaded(sim->fields());
    std::vector<std::string> warnings;
    REQUIRE(loaded.load(json, BuildPart::Whole, warnings, e));
    CHECK(loaded.get("exhaust.primD") == 0.0);
    CHECK(loaded.get("intake.runnerBore") == 0.0);
    CHECK(loaded.get("exhaust.colD") == Catch::Approx(0.045));   // a bore over its least: taken
    std::string all;
    for (const std::string& w : warnings)
        all += w + "\n";
    INFO(all);
    CHECK(all.find("spec.exhaust.primD") != std::string::npos);
    CHECK(all.find("spec.intake.runnerBore") != std::string::npos);
    CHECK(all.find("refused") != std::string::npos);
}
