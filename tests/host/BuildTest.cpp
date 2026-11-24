// tests/host/BuildTest.cpp: the document (card C, section 3): JSON round trips, unknown fields kept, a part loaded
// alone, undo and redo by gesture, the diff, the hash, and what reaches the simulator.
#include "Support.h"

#include <catch2/catch_test_macros.hpp>

using namespace tee;
using namespace tee::test;

TEST_CASE("a new build holds every Spec field at its default", "[build]")
{
    const auto sim = standinSim();
    Build b(sim->fields());
    CHECK(b.get("spec.engine.idle") == 80.0);
    CHECK(b.get("engine.idle") == 80.0);
    CHECK(b.get("engine.layout") == 1.0);
    CHECK(b.get("engine.starter") == 1.0);
    CHECK(b.get("sound.harmonics", 3) == 0.5);
    CHECK(b.fieldOf("engine.rev") < 0);          // the host's, not the document's
    CHECK(b.fieldOf("tel.engine.speed") < 0);
}

TEST_CASE("a build round-trips through JSON: numbers, choices, switches, arrays, curves, maps", "[build][json]")
{
    const auto sim = standinSim();
    Build a(sim->fields());
    a.name = "Test build";
    a.notes = "for the round trip";
    CHECK(a.set("engine.displacement", 0.0071));
    CHECK(a.setChoice("engine.layout", "V12"));
    CHECK(a.set("engine.starter", 0));
    CHECK(a.set("sound.harmonics", 0.25, 2));
    CHECK(a.setCurve("engine.torqueShape", {0, 300, 600}, {0.6, 1.0, 0.8}));
    CHECK(a.setMap("ecu.spark", {100, 400}, {0, 0.5, 1}, {{0, 0.01, 0.02}, {0.03, 0.04, 0.05}}));

    const std::string text = a.save();
    CHECK(text.find("\"layout\": \"V12\"") != std::string::npos);
    CHECK(text.find("\"starter\": false") != std::string::npos);
    CHECK(text.find("\"rev\"") == std::string::npos);
    CHECK(text.find("\"tap\"") == std::string::npos);   // hidden: not saved

    Build b(sim->fields());
    std::vector<std::string> warnings;
    std::string error;
    REQUIRE(b.load(text, BuildPart::Whole, warnings, error));
    CHECK(warnings.empty());
    CHECK(Build::diff(a, b).empty());
    CHECK(a.hash() == b.hash());
    CHECK(b.name == "Test build");
    CHECK(b.save() == text);
    CHECK(b.get("engine.torqueShape", 16 + 1) == 1.0);   // y[1]
}

TEST_CASE("values are clamped to their ranges, with one warning for a file's", "[build]")
{
    const auto sim = standinSim();
    Build b(sim->fields());
    CHECK(b.set("engine.redline", 99999));
    CHECK(b.get("engine.redline") == 1200);
    CHECK(b.set("engine.layout", 1.7));
    CHECK(b.get("engine.layout") == 1);
    std::vector<std::string> warnings;
    std::string error;
    REQUIRE(b.load(R"({"format": "tee-build", "version": 1, "spec": {"engine": {"idle": 5}}})", BuildPart::Whole, warnings,
                   error));
    CHECK(b.get("engine.idle") == 40);
    REQUIRE(warnings.size() == 1);
    CHECK(warnings[0].find("spec.engine.idle") != std::string::npos);
    CHECK(b.get("engine.redline") == 680);   // a field the file lacks takes its default
}

TEST_CASE("a field the program does not know survives a load and a save, with one warning", "[build][json]")
{
    const auto sim = standinSim();
    Build b(sim->fields());
    std::vector<std::string> warnings;
    std::string error;
    const char* newer = R"({"format": "tee-build", "version": 1, "name": "from a newer app",
        "spec": {"engine": {"idle": 90, "variableLift": {"stages": 2, "switchAt": 550}},
                 "boost": {"kind": "Twin", "turbo": [{"size": 0.5}, {"size": 0.6}]}}})";
    REQUIRE(b.load(newer, BuildPart::Whole, warnings, error));
    REQUIRE(warnings.size() == 1);
    CHECK(warnings[0].find("5 fields") != std::string::npos);
    CHECK(b.unknown().size() == 5);
    const std::string text = b.save();
    Build c(sim->fields());
    std::vector<std::string> again;
    REQUIRE(c.load(text, BuildPart::Whole, again, error));
    CHECK(c.unknown() == b.unknown());
    CHECK(text.find("\"switchAt\": 550") != std::string::npos);
    CHECK(text.find("\"kind\": \"Twin\"") != std::string::npos);
    CHECK(c.get("engine.idle") == 90);
}

TEST_CASE("a part saved alone and loaded alone touches only its sections", "[build][json]")
{
    const auto sim = standinSim();
    Build a(sim->fields());
    a.set("engine.idle", 100);
    a.set("ecu.limiterCut", 0.1);
    a.set("sound.level", 0.9);
    const std::string engine = a.save(BuildPart::Engine);
    const std::string tune = a.save(BuildPart::Tune);
    CHECK(engine.find("\"part\": \"engine\"") != std::string::npos);
    CHECK(engine.find("limiterCut") == std::string::npos);
    CHECK(tune.find("\"idle\"") == std::string::npos);

    Build b(sim->fields());
    b.set("sound.level", 0.5);
    b.set("ecu.limiterCut", 0.15);
    std::vector<std::string> warnings;
    std::string error;
    REQUIRE(b.load(engine, BuildPart::Engine, warnings, error));
    CHECK(b.get("engine.idle") == 100);
    CHECK(b.get("ecu.limiterCut") == 0.15);   // the tune untouched
    CHECK(b.get("sound.level") == 0.5);
    CHECK_FALSE(b.load(engine, BuildPart::Tune, warnings, error));   // the engine's file is not the tune's
    REQUIRE(b.load(tune, BuildPart::Tune, warnings, error));
    CHECK(b.get("ecu.limiterCut") == 0.1);
    CHECK(b.get("engine.idle") == 100);
}

TEST_CASE("undo and redo go by gesture", "[build][undo]")
{
    const auto sim = standinSim();
    Build b(sim->fields());
    b.beginGesture();
    for (int i = 0; i < 20; ++i)
        b.set("engine.idle", 80 + i);   // a knob dragged
    b.endGesture();
    b.set("engine.redline", 700);
    b.set("engine.redline", 720);
    CHECK(b.get("engine.idle") == 99);
    REQUIRE(b.undo());
    CHECK(b.get("engine.redline") == 700);
    REQUIRE(b.undo());
    CHECK(b.get("engine.redline") == 680);
    REQUIRE(b.undo());
    CHECK(b.get("engine.idle") == 80);
    CHECK_FALSE(b.canUndo());
    REQUIRE(b.redo());
    CHECK(b.get("engine.idle") == 99);
    b.set("engine.friction", 0.2);   // a new change drops what could be redone
    CHECK_FALSE(b.canRedo());
    std::vector<std::string> warnings;
    std::string error;
    REQUIRE(b.load(R"({"format": "tee-build", "version": 1, "spec": {}})", BuildPart::Whole, warnings, error));
    CHECK(b.get("engine.idle") == 80);
    REQUIRE(b.undo());   // a load is one step
    CHECK(b.get("engine.idle") == 99);
    CHECK(b.get("engine.friction") == 0.2);
}

TEST_CASE("the diff names what differs; the hash moves with any value and not with the name", "[build]")
{
    const auto sim = standinSim();
    Build a(sim->fields()), b(sim->fields());
    CHECK(a.hash() == b.hash());
    b.name = "another name";
    CHECK(a.hash() == b.hash());
    b.set("sound.harmonics", 0.1, 1);
    b.set("engine.idle", 81);
    const auto d = Build::diff(a, b);
    REQUIRE(d.size() == 2);
    CHECK(d[0].path == "spec.engine.idle");
    CHECK(d[1].path == "spec.sound.harmonics");
    CHECK(a.hash() != b.hash());
}

TEST_CASE("an attached build sends each change, and an undo, to the simulator", "[build][simhost]")
{
    const auto sim = standinSim();
    SimHost host;
    std::string errors;
    REQUIRE(host.attach(sim, offline(), errors));
    Build b(host.fields());
    b.set("engine.idle", 110);   // before attaching: sent with everything when it attaches
    b.attach(&host);
    std::vector<float> l(256), r(256);
    TelemetryFrame t;
    host.process(l.data(), r.data(), 256);
    REQUIRE(host.telemetry(t));
    CHECK(tel(host, t, "tel.engine.idle") == 110);
    b.setCurve("engine.torqueShape", {0, 100}, {0.5, 0.5});
    b.set("engine.idle", 120);
    host.process(l.data(), r.data(), 256);
    REQUIRE(host.telemetry(t));
    CHECK(tel(host, t, "tel.engine.idle") == 120);
    CHECK(tel(host, t, "tel.probe.curveN") == 2);
    b.undo();
    b.undo();
    host.process(l.data(), r.data(), 256);
    REQUIRE(host.telemetry(t));
    CHECK(tel(host, t, "tel.engine.idle") == 110);
    CHECK(tel(host, t, "tel.probe.curveN") == 0);
}

TEST_CASE("the presets list the shipped builds and the user's", "[build][presets]")
{
    const std::filesystem::path root = std::filesystem::path(TEE_SOURCE_DIR) / "out" / "tests" / "presets";
    std::filesystem::remove_all(root);
    std::filesystem::create_directories(root / "shipped");
    std::filesystem::create_directories(root / "user");
    const auto sim = standinSim();
    Build b(sim->fields());
    std::string error;
    REQUIRE(b.saveFile(root / "shipped" / "b.json", BuildPart::Whole, error));
    REQUIRE(b.saveFile(root / "shipped" / "a.json", BuildPart::Whole, error));
    REQUIRE(b.saveFile(root / "user" / "mine.json", BuildPart::Whole, error));
    const auto list = Presets::list(root / "shipped", root / "user");
    REQUIRE(list.size() == 3);
    CHECK(list[0].name == "a");
    CHECK(list[1].name == "b");
    CHECK(list[2].name == "mine");
    CHECK(list[2].user);
}
