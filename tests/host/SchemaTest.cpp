// tests/host/SchemaTest.cpp: the Schema's FieldInfo, and the contract's lint and snapshot (card C, section 1). The lint
// and the snapshot are how 02-contracts.md, sections 2 and 3, are enforced on every simulator package: they run on
// sim/engine.fm once it exists (and on the stand-in always).
#include "Support.h"

#include "tee/Fields.h"

#include <catch2/catch_test_macros.hpp>

#include <fstream>

using namespace tee;
using namespace tee::test;

namespace
{
    std::string readText(const std::filesystem::path& file)
    {
        std::ifstream in(file, std::ios::binary);
        std::ostringstream text;
        text << in.rdbuf();
        return text.str();
    }

    std::vector<std::string> lintOf(Program::Files files)
    {
        std::string errors;
        auto sim = Sim::compileFiles(std::move(files), {}, errors);
        INFO(errors);
        REQUIRE(sim != nullptr);
        return lint(sim->fields());
    }

    bool mentions(const std::vector<std::string>& problems, const std::string& text)
    {
        for (const std::string& p : problems)
            if (p.find(text) != std::string::npos)
                return true;
        return false;
    }
} // namespace

TEST_CASE("the schema reads every attribute of the contract into FieldInfo", "[schema]")
{
    const auto sim = standinSim();
    const Fields& f = sim->fields();

    REQUIRE(f.sections().size() == 3);
    CHECK(f.sections()[0].path == "spec.engine");
    CHECK(f.sectionOf("ecu") == 1);

    const FieldInfo* layout = f.find("spec.engine.layout");
    REQUIRE(layout != nullptr);
    CHECK(layout->kind == FieldKind::Choice);
    CHECK(layout->choices.size() == 3);
    CHECK(*layout->minimum == 0);
    CHECK(*layout->defaultValue == 1);
    CHECK(layout->group == "Block");
    CHECK(layout->section == 0);

    const FieldInfo* displacement = f.find("spec.engine.displacement");
    REQUIRE(displacement != nullptr);
    CHECK(displacement->quantity == "volume");
    CHECK(displacement->taper == "log");
    CHECK(displacement->decimals == 1);
    CHECK(displacement->group == "Block");   // a group holds until the next one

    CHECK(f.find("spec.engine.flywheel")->help.find("inertia") != std::string::npos);
    CHECK(f.find("spec.engine.friction")->unit == "N m s");
    CHECK(f.find("spec.engine.idle")->group == "Speeds");
    CHECK(f.find("spec.engine.starter")->kind == FieldKind::Switch);
    CHECK(f.find("spec.engine.rev")->kind == FieldKind::Rev);
    CHECK(f.find("spec.sound.tap")->hidden);

    const FieldInfo* curve = f.find("spec.engine.torqueShape");
    REQUIRE(curve != nullptr);
    CHECK(curve->kind == FieldKind::Curve);
    CHECK(curve->axisX == "rotation");
    CHECK(curve->leaves.size() == 3);
    const FieldInfo* map = f.find("spec.ecu.spark");
    REQUIRE(map != nullptr);
    CHECK(map->kind == FieldKind::Map);
    CHECK(map->leaves.size() == 5);

    const FieldInfo* harmonics = f.find("spec.sound.harmonics");
    REQUIRE(harmonics != nullptr);
    CHECK(harmonics->kind == FieldKind::Array);
    CHECK(harmonics->count == 4);

    std::string error;
    const FieldRef element = f.ref("spec.sound.harmonics[2]", error);
    REQUIRE(element.valid());
    CHECK(element.offset == harmonics->offset + 8);
    CHECK(element.count == 1);
    CHECK(element.section == 2);
    CHECK_FALSE(f.ref("spec.sound.harmonics[4]", error).valid());
    CHECK_FALSE(f.ref("spec.nothing", error).valid());

    CHECK(f.find("tel.engine.speed")->quantity == "rotation");
    CHECK(f.find("ctl.throttle") != nullptr);
}

TEST_CASE("the stand-in passes the lint, and its snapshot holds", "[schema][lint]")
{
    const auto sim = standinSim();
    const std::vector<std::string> problems = lint(sim->fields());
    for (const std::string& p : problems)
        UNSCOPED_INFO(p);
    CHECK(problems.empty());
    const std::vector<std::string> broken =
        compareSnapshot(sim->fields(), readText(std::filesystem::path(TEE_HOST_TESTS_DIR) / "fixtures" / "standin.snapshot"));
    for (const std::string& p : broken)
        UNSCOPED_INFO(p);
    CHECK(broken.empty());
}

TEST_CASE("the lint finds what the contract forbids", "[schema][lint]")
{
    Program::Files files = standinFiles();
    std::string& spec = files["spec.fm"];
    spec = replaced(spec, "[[ui::label(\"Flywheel\")]]", "");
    spec = replaced(spec, "[[ui::quantity(\"torque\")]]", "[[ui::quantity(\"oomph\")]]");
    spec = replaced(spec, "[[ui::choices(\"Inline four\", \"V8\", \"V12\")]]", "[[ui::choices(\"Inline four\", \"V8\")]]");
    spec = replaced(spec, "float friction [[fminus::range(0, 0.5)]] [[fminus::default(0.08)]]", "float friction");
    spec = replaced(spec, "[[fminus::range(0.0005, 0.03)]]", "[[fminus::range(0, 0.03)]]");
    spec = replaced(spec, "    SoundSpec sound;\n", "    SoundSpec sound;\n    float loose [[ui::label(\"Loose\")]];\n");
    const std::vector<std::string> problems = lintOf(files);
    for (const std::string& p : problems)
        UNSCOPED_INFO(p);
    CHECK(mentions(problems, "spec.engine.flywheel: no ui::label"));
    CHECK(mentions(problems, "spec.engine.torque: ui::quantity(\"oomph\")"));
    CHECK(mentions(problems, "spec.engine.layout: 2 choices for a range of 3"));
    CHECK(mentions(problems, "spec.engine.friction: no fminus::default"));
    CHECK(mentions(problems, "spec.engine.friction: no fminus::range"));
    CHECK(mentions(problems, "spec.engine.displacement: a log taper"));
    CHECK(mentions(problems, "spec.loose: not a section"));
}

TEST_CASE("a snapshot catches a field that disappeared or changed its type", "[schema][snapshot]")
{
    const auto sim = standinSim();
    std::string snapshot = sim->fields().snapshot();
    snapshot += "spec.engine.gone\tfloat\tnumber\n";
    snapshot = replaced(snapshot, "spec.engine.torque\tfloat", "spec.engine.torque\tint");
    const std::vector<std::string> problems = compareSnapshot(sim->fields(), snapshot);
    REQUIRE(problems.size() == 2);
    CHECK(mentions(problems, "spec.engine.gone: disappeared"));
    CHECK(mentions(problems, "spec.engine.torque: was 'int'"));
}

TEST_CASE("the simulator passes the lint, and the committed snapshot holds", "[schema][lint][sim]")
{
    const std::filesystem::path sim = std::filesystem::path(TEE_SOURCE_DIR) / "sim";
    if (!std::filesystem::exists(sim / "engine.fm"))
        SKIP("sim/engine.fm is not there yet (part K3 writes it): the lint runs on the stand-in meanwhile");
    std::string errors;
    auto compiled = test::compiled(sim, {}, errors);   // the run's one compile (not one of its own: the fast tier's cap)
    INFO(errors);
    REQUIRE(compiled != nullptr);
    const std::vector<std::string> problems = lint(compiled->fields());
    for (const std::string& p : problems)
        UNSCOPED_INFO(p);
    CHECK(problems.empty());
    const std::filesystem::path snapshot = std::filesystem::path(TEE_HOST_TESTS_DIR) / "schema.snapshot";
    if (!std::filesystem::exists(snapshot))
        SKIP("tests/host/schema.snapshot is not committed yet: `tee-schema --snapshot --out tests/host/schema.snapshot`");
    const std::vector<std::string> broken = compareSnapshot(compiled->fields(), readText(snapshot));
    for (const std::string& p : broken)
        UNSCOPED_INFO(p);
    CHECK(broken.empty());
}
