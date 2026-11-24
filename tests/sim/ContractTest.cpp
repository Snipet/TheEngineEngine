// tests/sim/ContractTest.cpp: the simulator's contract (docs/plan/02-contracts.md, sections 1 to 5; sim/README.md):
// Engine::process is nonblocking, every Spec field describes itself, the defaults are the reference's, a build
// round-trips, the tables read as they should, and the Telemetry says what the engine does.
#include "Twin.h"
#include "tee/Fields.h"
#include "tee/test/Kept.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace tee;

namespace
{
    EnginePlayer& shared48()
    {
        return test::kept([] { return test::enginePlayer(48000.0); });   // never destroyed (tee/test/Kept.h)
    }
} // namespace

TEST_CASE("sim: Engine compiles, and its process is nonblocking", "[sim][contract]")
{
    EnginePlayer& player = shared48();
    INFO(player.program().warnings());
    CHECK(player.program().fminus().nonBlocking("Engine::process"));
    CHECK_FALSE(player.program().fminus().nonBlocking("Engine::init"));
    CHECK(player.specFields().size() >= 30);
    CHECK(player.telemetryIndex("engine.speed") >= 0);
    CHECK(player.telemetryIndex("car.gear") >= 0);
    CHECK(player.telemetryIndex("boost.pressure") >= 0);
    std::string error;
    Schema::Handle scope;
    CHECK(player.schema().find("scope.pressure", scope, error));
}

TEST_CASE("sim: every Spec field has its range, default and label", "[sim][contract]")
{
    EnginePlayer& player = shared48();
    // A table's own fields (a Curve16's x, y, n; a Map16's x, y, z, nx, ny) carry nothing for the display: the table
    // field does (its label, ui::axes and its value's unit), and the lint holds it to that.
    std::string error;
    const std::unique_ptr<Fields> fields = Fields::of(player.schema(), error);
    REQUIRE(fields != nullptr);
    std::vector<std::string> tables;
    for (const FieldInfo& f : fields->all())
        if (f.block == "spec" && f.table())
            tables.push_back(f.path + ".");
    const std::vector<std::string> problems = lint(*fields);
    for (const std::string& p : problems)
        UNSCOPED_INFO(p);
    CHECK(problems.empty());   // the same rules as `tee-schema --lint` and scripts/lint.sh
    for (const EnginePlayer::SpecField& field : player.specFields())
    {
        const std::string path = "spec." + field.path;
        bool inTable = false;
        for (const std::string& t : tables)
            inTable = inTable || path.rfind(t, 0) == 0;
        if (inTable)
            continue;
        INFO(path);
        CHECK(field.node->attribute("ui::label") != nullptr);
        CHECK(field.node->attribute("fminus::default") != nullptr);
        if (!field.isBool())
        {
            CHECK(field.hasRange);
            CHECK(field.defaultValue >= field.minimum);
            CHECK(field.defaultValue <= field.maximum);
            const bool described = field.node->attribute("ui::quantity") != nullptr
                                   || field.node->attribute("ui::unit") != nullptr || !field.choices.empty();
            CHECK(described);
        }
        if (!field.choices.empty())
            CHECK(static_cast<double>(field.choices.size()) == field.maximum - field.minimum + 1.0);
    }
}

TEST_CASE("sim: the defaults are the reference's defaults", "[sim][contract]")
{
    EnginePlayer& player = shared48();
    player.reset();
    float none[1] = {0.0f};
    player.process(none, none, 0);   // applies the Spec to the reference's Set, renders nothing
    const std::unique_ptr<RefPlayer> ref = test::refPlayer(48000.0);
    for (const test::DialOfSet& d : test::kDialsOfSet)
    {
        if (std::string(d.dial) == "hold")
            continue;   // the reference's HOLD idled the engine out; the contract's engine never switches itself off
        const int k = ref->dialIndex(d.dial);
        REQUIRE(k >= 0);
        INFO("dial " << d.dial << ", Set's " << d.set);
        const double mine = test::field(player, std::string("set.") + d.set);
        CHECK(static_cast<float>(mine) == static_cast<float>(ref->dials()[static_cast<std::size_t>(k)].defaultValue));
    }
    CHECK(test::field(player, "set.drive") == 0.0);   // MODE_BENCH: the reference's DRIVE KEYS, keys off
    CHECK(test::field(player, "set.keys") == 0.0);
}

TEST_CASE("sim: a build round-trips", "[sim][contract][build]")
{
    EnginePlayer& player = shared48();
    for (const char* name : {"default", "v8", "v10", "large-diesel"})
    {
        INFO(name);
        const EngineBuild loaded = test::engineBuild(player, name);
        CHECK_FALSE(loaded.values.empty());
        // as text and back
        EngineBuild again;
        std::string error, warnings;
        REQUIRE(parseBuild(player, buildJson(player, loaded), "again", again, error, warnings));
        CHECK(warnings.empty());
        CHECK(again.name == loaded.name);
        for (const auto& [path, values] : loaded.values)
        {
            INFO(path);
            REQUIRE(again.values.count(path) == 1);
            CHECK(again.values.at(path) == values);
        }
        // into the simulator and out again: what it holds is what the file says (as floats)
        applyBuild(player, loaded);
        const EngineBuild held = currentBuild(player, loaded.name, loaded.notes);
        for (const auto& [path, values] : loaded.values)
        {
            INFO(path);
            for (std::size_t e = 0; e < values.size(); ++e)
                CHECK(static_cast<float>(held.values.at(path)[e]) == static_cast<float>(values[e]));
        }
        CHECK(buildJson(player, held) == buildJson(player, currentBuild(player, loaded.name, loaded.notes)));
    }
    // the defaults' build is every field at its default
    applyBuild(player, test::engineBuild(player, "default"));
    for (std::size_t k = 0; k < player.specFields().size(); ++k)
        CHECK(static_cast<float>(player.spec(k)) == static_cast<float>(player.specFields()[k].defaultValue));
}

TEST_CASE("sim: a build keeps what the Spec lacks, and names a choice", "[sim][contract][build]")
{
    EnginePlayer& player = shared48();
    const char* const json = R"({"format": "tee-build", "version": 1, "name": "x", "notes": "",
        "spec": {"engine": {"layout": "Inline 4", "displacement": 0.0016, "fromTheFuture": [1, 2]},
                 "warp": {"drive": 9}}})";
    EngineBuild build;
    std::string error, warnings;
    REQUIRE(parseBuild(player, json, "x.json", build, error, warnings));
    CHECK(build.values.at("engine.layout") == std::vector<double>{5.0});
    CHECK(build.unknown.size() == 2);
    CHECK(warnings.find("engine.fromTheFuture") != std::string::npos);
    const std::string out = buildJson(player, build);
    CHECK(out.find("\"Inline 4\"") != std::string::npos);
    CHECK(out.find("fromTheFuture") != std::string::npos);
    CHECK(out.find("\"warp\"") != std::string::npos);
    EngineBuild bad;
    CHECK_FALSE(parseBuild(player, R"({"format": "tee-build", "version": 1, "spec": {"engine": {"layout": "W16"}}})",
                           "bad.json", bad, error, warnings));
    // a value outside its range is clamped where it is written
    applyBuild(player, build);
    REQUIRE(player.setSpec("engine.compression", 99.0, error));
    CHECK(player.spec(static_cast<std::size_t>(player.specIndex("engine.compression"))) == 25.0);
}

TEST_CASE("sim: a section's rev moves when one of its fields is written", "[sim][contract]")
{
    EnginePlayer& player = shared48();
    player.reset();
    const double before = test::field(player, "spec.exhaust.rev");
    const double other = test::field(player, "spec.engine.rev");
    std::string error;
    REQUIRE(player.setSpec("exhaust.muffler", 0.8, error));
    CHECK(test::field(player, "spec.exhaust.rev") == before + 1.0);
    CHECK(test::field(player, "spec.engine.rev") == other);
    // and the simulator takes it only then
    float none[1] = {0.0f};
    player.process(none, none, 0);
    CHECK(static_cast<float>(test::field(player, "set.muffler")) == 0.8f);
    std::string e;
    double revs = 0.0;
    REQUIRE(player.read("spec.exhaust.rev", revs, e));
    player.instance().write(player.schema(), "spec.exhaust.muffler", 0.1, e);   // written without its rev moving
    player.process(none, none, 0);
    CHECK(static_cast<float>(test::field(player, "set.muffler")) == 0.8f);
}

TEST_CASE("sim: the Telemetry follows a start, a run and off", "[sim][contract]")
{
    EnginePlayer& player = shared48();
    applyBuild(player, test::engineBuild(player, "default"));
    player.reset(0.0, true);
    const int state = player.telemetryIndex("engine.state");
    const int speed = player.telemetryIndex("engine.speed");
    std::vector<float> l(512), r(512);
    const auto run = [&](double seconds) {
        for (int k = 0; k < static_cast<int>(seconds * 48000.0 / 512.0); ++k)
            player.process(l.data(), r.data(), 512);
    };
    run(0.2);
    CHECK(player.telemetry(static_cast<std::size_t>(state)) == 0.0);   // nothing happens until the key
    player.controls().ignition = EnginePlayer::kIgnitionStart;
    run(0.1);
    CHECK(player.telemetry(static_cast<std::size_t>(state)) >= 1.0);
    run(2.0);
    player.controls().ignition = EnginePlayer::kIgnitionOn;
    run(3.0);
    CHECK(player.telemetry(static_cast<std::size_t>(state)) == 2.0);
    const double idle = player.telemetry(static_cast<std::size_t>(player.telemetryIndex("engine.idle")));
    CHECK(player.telemetry(static_cast<std::size_t>(speed)) == Catch::Approx(idle).epsilon(0.25));
    CHECK(player.telemetry(static_cast<std::size_t>(player.telemetryIndex("engine.manifold"))) < 101325.0);
    run(30.0);   // the reference's HOLD would have idled it out after 10 s: this one runs on
    CHECK(player.telemetry(static_cast<std::size_t>(state)) == 2.0);
    // MODE_BENCH, holdSpeed: the key driver holds 3000 rpm
    player.controls().holdSpeed = static_cast<float>(3000.0 * 0.10471976);
    run(4.0);
    CHECK(player.telemetry(static_cast<std::size_t>(speed)) * 9.5492966 == Catch::Approx(3000.0).epsilon(0.05));
    player.controls().holdSpeed = 0.0f;
    player.controls().ignition = EnginePlayer::kIgnitionOff;
    run(4.0);
    CHECK(player.telemetry(static_cast<std::size_t>(state)) == 0.0);
    CHECK(player.telemetry(static_cast<std::size_t>(speed)) < 1.0);   // (the gas left in the cylinders may nudge it)
}

TEST_CASE("sim: the tables read as their header says", "[sim][tables]")
{
    Program::Options options;
    options.appended = "float tee_curve(Curve16* c, float v) [[clang::nonblocking]] { return c->at(v); }\n"
                       "float tee_map(Map16* m, float u, float v) [[clang::nonblocking]] { return m->at(u, v); }\n";
    std::string errors;
    const auto program = Program::fromDirectory(test::simDir() + "/spec", "tables.fm", options, errors);
    INFO(errors);
    REQUIRE(program != nullptr);
    auto* curve = program->function<float(void*, float)>("tee_curve", errors);
    auto* map = program->function<float(void*, float, float)>("tee_map", errors);
    REQUIRE(curve != nullptr);
    REQUIRE(map != nullptr);
    const auto curveSchema = Schema::of(program->fminus(), "Curve16", errors);
    const auto mapSchema = Schema::of(program->fminus(), "Map16", errors);
    REQUIRE(curveSchema != nullptr);
    REQUIRE(mapSchema != nullptr);

    Instance c(*program->fminus().module("Curve16"));
    std::string e;
    CHECK(curve(c.data(), 3.0f) == 0.0f);   // empty: 0
    const float xs[4] = {1.0f, 2.0f, 4.0f, 8.0f};
    const float ys[4] = {10.0f, 20.0f, 0.0f, -8.0f};
    for (int i = 0; i < 4; ++i)
    {
        c.write(*curveSchema, "x[" + std::to_string(i) + "]", xs[i], e);
        c.write(*curveSchema, "y[" + std::to_string(i) + "]", ys[i], e);
    }
    c.write(*curveSchema, "n", 1, e);
    CHECK(curve(c.data(), 100.0f) == 10.0f);   // one point: everywhere
    c.write(*curveSchema, "n", 4, e);
    CHECK(curve(c.data(), 0.0f) == 10.0f);     // below: the first
    CHECK(curve(c.data(), 1.5f) == 15.0f);
    CHECK(curve(c.data(), 2.0f) == 20.0f);
    CHECK(curve(c.data(), 3.0f) == 10.0f);
    CHECK(curve(c.data(), 6.0f) == -4.0f);
    CHECK(curve(c.data(), 9.0f) == -8.0f);     // above: the last

    Instance m(*program->fminus().module("Map16"));
    CHECK(map(m.data(), 1.0f, 1.0f) == 0.0f);
    // z = 10 x + y on x {0, 1, 3}, y {0, 2}
    const float mx[3] = {0.0f, 1.0f, 3.0f};
    const float my[2] = {0.0f, 2.0f};
    for (int i = 0; i < 3; ++i)
        m.write(*mapSchema, "x[" + std::to_string(i) + "]", mx[i], e);
    for (int j = 0; j < 2; ++j)
        m.write(*mapSchema, "y[" + std::to_string(j) + "]", my[j], e);
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 2; ++j)
            m.write(*mapSchema, "z[" + std::to_string(i) + "][" + std::to_string(j) + "]", 10.0f * mx[i] + my[j], e);
    m.write(*mapSchema, "nx", 3, e);
    m.write(*mapSchema, "ny", 2, e);
    CHECK(map(m.data(), 0.5f, 1.0f) == Catch::Approx(6.0));
    CHECK(map(m.data(), 2.0f, 0.5f) == Catch::Approx(20.5));
    CHECK(map(m.data(), 3.0f, 2.0f) == Catch::Approx(32.0));
    CHECK(map(m.data(), -1.0f, -1.0f) == Catch::Approx(0.0));    // outside: the edge
    CHECK(map(m.data(), 9.0f, 9.0f) == Catch::Approx(32.0));
}

TEST_CASE("sim: the Scope holds cylinder 1's last whole cycle while scopeOn is set", "[sim][contract][scope]")
{
    EnginePlayer& player = shared48();
    applyBuild(player, test::engineBuild(player, "default"));
    player.reset(0.0, true);
    std::vector<float> l(512), r(512);
    const auto run = [&](double seconds) {
        for (int k = 0; k < static_cast<int>(seconds * 48000.0 / 512.0); ++k)
            player.process(l.data(), r.data(), 512);
    };
    player.controls().ignition = EnginePlayer::kIgnitionStart;
    run(1.5);
    player.controls().ignition = EnginePlayer::kIgnitionOn;
    run(1.0);
    CHECK(test::field(player, "scope.cycles") == 0.0);   // off: nothing written
    player.controls().scopeOn = 1;
    run(1.0);
    CHECK(test::field(player, "scope.cycles") >= 2.0);
    CHECK(test::field(player, "scope.degrees") == 720.0);
    double pMax = 0.0, atMax = 0.0, vMin = 1.0, vMax = 0.0, eMax = 0.0, iMax = 0.0;
    for (int d = 0; d < 720; ++d)
    {
        const std::string i = "[" + std::to_string(d) + "]";
        const double p = test::field(player, "scope.pressure" + i);
        const double v = test::field(player, "scope.volume" + i);
        if (p > pMax)
        {
            pMax = p;
            atMax = d;
        }
        vMin = std::min(vMin, v);
        vMax = std::max(vMax, v);
        eMax = std::max(eMax, test::field(player, "scope.liftExhaust" + i));
        iMax = std::max(iMax, test::field(player, "scope.liftIntake" + i));
    }
    INFO("peak " << pMax << " Pa at " << atMax << " degrees; volume " << vMin << " to " << vMax);
    CHECK(pMax > 1.0e6);                                     // compression and the burn
    CHECK((atMax < 60.0 || atMax > 690.0));                  // near the top of compression
    CHECK(vMax / vMin > 8.0);                                // the compression ratio's 10, less the rod's geometry
    CHECK(eMax > 0.9);
    CHECK(iMax > 0.9);
    CHECK(test::field(player, "scope.pipePoints") == 8.0);
}
