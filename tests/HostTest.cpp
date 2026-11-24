// tests/HostTest.cpp: tee::Program, Schema and Instance on small programs held in memory: the includes, the messages,
// the tree of fields and its paths, typed access, and the checks a development build has.
#include "tee/Instance.h"
#include "tee/Program.h"
#include "tee/Schema.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstdint>

namespace
{
    // A program of three files: an entry that includes a module, which includes another.
    tee::Program::Files smallProgram()
    {
        return {
            {"main.fm",
             "#include \"parts/tank.fm\"\n"
             "module Rig\n"
             "{\n"
             "    param float level [[fminus::range(-60, 0)]] [[fminus::default(-12)]] [[ui::unit(\"dB\")]];\n"
             "    param int mode [[fminus::range(0, 3)]] [[fminus::default(2)]];\n"
             "    Tank tank[3];\n"
             "    Valve single;\n"
             "    float grid[2][4];\n"
             "    double slow;\n"
             "    bool on;\n"
             "    float* buffer;\n"
             "    int count [[ui::label(\"Count\")]];\n"
             "    float sum(int n)\n"
             "    {\n"
             "        float s = 0.0f;\n"
             "        for (int i = 0; i < n; ++i)\n"
             "            s += grid[0][i];\n"
             "        return s + level[0];\n"
             "    }\n"
             "    int steps(int x)\n"              // a loop on the data, which no optimiser unrolls (Collatz's)
             "    {\n"
             "        int k = 0;\n"
             "        while (x != 1)\n"
             "        {\n"
             "            x = x % 2 == 0 ? x / 2 : 3 * x + 1;\n"
             "            k += 1;\n"
             "        }\n"
             "        return k;\n"
             "    }\n"
             "};\n"},
            {"parts/tank.fm",
             "#pragma once\n"
             "#include \"valve.fm\"\n"          // beside this file: parts/valve.fm
             "module Tank\n"
             "{\n"
             "    float p [[ui::unit(\"Pa\")]] [[fminus::range(0, 200000)]];\n"
             "    Valve in;\n"
             "    int cycles;\n"
             "};\n"},
            {"parts/valve.fm",
             "#pragma once\n"
             "module Valve\n"
             "{\n"
             "    float area;\n"
             "    bool open;\n"
             "};\n"},
        };
    }
} // namespace

TEST_CASE("a program compiles from its files, includes found beside the file that names them", "[host][program]")
{
    std::string errors;
    const auto program = tee::Program::fromFiles(smallProgram(), "main.fm", {}, errors);
    INFO(errors);
    REQUIRE(program != nullptr);
    CHECK(errors.empty());
    CHECK(program->warnings().empty());
    CHECK(program->compileSeconds() > 0.0);
    REQUIRE(program->filesRead().size() == 3);
    CHECK(program->filesRead()[0] == "main.fm");
    CHECK(program->filesRead()[1] == "parts/tank.fm");
    CHECK(program->filesRead()[2] == "parts/valve.fm");

    REQUIRE(program->link(errors));
    CHECK(program->codeBytes() > 0);
    auto* sum = program->function<float(void*, int)>("Rig::sum", errors);
    REQUIRE(sum != nullptr);
}

TEST_CASE("a program that does not compile gives FMinus's messages, in clang's words", "[host][program]")
{
    tee::Program::Files files = smallProgram();
    files["parts/valve.fm"] = "#pragma once\nmodule Valve\n{\n    float area;\n    bool open\n};\n";
    std::string errors;
    const auto program = tee::Program::fromFiles(files, "main.fm", {}, errors);
    REQUIRE(program == nullptr);
    INFO(errors);
    CHECK(errors.find("parts/valve.fm:") != std::string::npos);
    CHECK(errors.find("error:") != std::string::npos);

    files.erase("parts/valve.fm");
    const auto missing = tee::Program::fromFiles(files, "main.fm", {}, errors);
    REQUIRE(missing == nullptr);
    INFO(errors);
    CHECK(errors.find("'valve.fm' file not found") != std::string::npos);

    const auto noEntry = tee::Program::fromFiles(files, "nothing.fm", {}, errors);
    CHECK(noEntry == nullptr);
    const auto outside = tee::Program::fromFiles(files, "../main.fm", {}, errors);
    CHECK(outside == nullptr);
}

TEST_CASE("the schema is the module as a tree, with paths, offsets, extents and attributes", "[host][schema]")
{
    std::string errors;
    const auto program = tee::Program::fromFiles(smallProgram(), "main.fm", {}, errors);
    REQUIRE(program != nullptr);
    const auto schema = tee::Schema::of(program->fminus(), "Rig", errors);
    REQUIRE(schema != nullptr);
    CHECK(tee::Schema::of(program->fminus(), "Nothing", errors) == nullptr);

    const fminus::Program::Module* rig = program->fminus().module("Rig");
    const fminus::Program::Module* tank = program->fminus().module("Tank");
    const fminus::Program::Module* valve = program->fminus().module("Valve");
    REQUIRE(rig != nullptr);
    REQUIRE(tank != nullptr);
    REQUIRE(valve != nullptr);
    CHECK(schema->size() == rig->size);
    CHECK(schema->module() == "Rig");

    // the offsets FMinus gives for each module, added up along the path
    const auto offsetIn = [](const fminus::Program::Module& module, const char* name) {
        for (const auto& field : module.fields)
            if (field.name == name)
                return field.offset;
        FAIL("no field " << name);
        return std::size_t{0};
    };
    tee::Schema::Handle handle;
    REQUIRE(schema->find("tank[2].in.area", handle, errors));
    CHECK(handle.offset == offsetIn(*rig, "tank") + 2 * tank->size + offsetIn(*tank, "in") + offsetIn(*valve, "area"));
    CHECK(handle.scalar() == tee::Scalar::Float);
    CHECK(handle.count == 1);

    REQUIRE(schema->find("tank[1].p", handle, errors));
    CHECK(handle.offset == offsetIn(*rig, "tank") + tank->size + offsetIn(*tank, "p"));
    REQUIRE(handle.node->attribute("ui::unit") != nullptr);
    CHECK(handle.node->attribute("ui::unit")->arguments.at(0).text == "Pa");
    REQUIRE(handle.node->attribute("fminus::range") != nullptr);
    CHECK(handle.node->attribute("fminus::range")->arguments.at(1).number == 200000.0);

    REQUIRE(schema->find("grid[1][2]", handle, errors));
    CHECK(handle.offset == offsetIn(*rig, "grid") + (1 * 4 + 2) * sizeof(float));
    REQUIRE(schema->find("grid[1]", handle, errors));     // a row: what is left of the array
    CHECK(handle.count == 4);
    REQUIRE(schema->find("grid", handle, errors));
    CHECK(handle.count == 8);
    CHECK(handle.node->extents == std::vector<std::size_t>{2, 4});

    REQUIRE(schema->find("single.open", handle, errors));
    CHECK(handle.scalar() == tee::Scalar::Bool);
    REQUIRE(schema->find("slow", handle, errors));
    CHECK(handle.scalar() == tee::Scalar::Double);
    REQUIRE(schema->find("buffer", handle, errors));
    CHECK(handle.scalar() == tee::Scalar::Pointer);
    REQUIRE(schema->find("level", handle, errors));
    CHECK(handle.node->parameter);
    CHECK(handle.scalar() == tee::Scalar::Float);
    REQUIRE(handle.node->param != nullptr);
    CHECK(handle.offset == handle.node->param->valueOffset);

    // what a path cannot name, and why
    CHECK_FALSE(schema->find("tank[3].p", handle, errors));
    CHECK(errors.find("index 3") != std::string::npos);
    CHECK_FALSE(schema->find("tank.p", handle, errors));
    CHECK_FALSE(schema->find("nothing", handle, errors));
    CHECK(errors.find("no field 'nothing'") != std::string::npos);
    CHECK_FALSE(schema->find("buffer.x", handle, errors));
    CHECK_FALSE(schema->find("count[0]", handle, errors));
    CHECK_FALSE(schema->find("tank[1].", handle, errors));
    CHECK_FALSE(schema->find("", handle, errors));

    // typed handles check the type
    CHECK(schema->field<float>("tank[0].p", errors).valid());
    CHECK_FALSE(schema->field<std::int32_t>("tank[0].p", errors).valid());
    CHECK(schema->field<std::int32_t>("tank[0].cycles", errors).valid());
    CHECK(schema->field<bool>("on", errors).valid());
    CHECK(schema->field<double>("slow", errors).valid());
    CHECK_FALSE(schema->field<float>("tank[0]", errors).valid());

    // every leaf once, arrays of modules unrolled
    std::vector<std::string> paths;
    schema->forEach([&](const std::string& path, const tee::Schema::Node&, std::size_t) { paths.push_back(path); });
    const auto has = [&](const char* path) { return std::find(paths.begin(), paths.end(), path) != paths.end(); };
    CHECK(has("level"));
    CHECK(has("tank[0].p"));
    CHECK(has("tank[2].in.open"));
    CHECK(has("single.area"));
    CHECK(has("grid"));
    CHECK(has("count"));
    CHECK_FALSE(has("tank"));
    CHECK(paths.size() == 2 + 3 * 4 + 2 + 5);

    CHECK(schema->describe().find("Tank[3] tank") != std::string::npos);
}

TEST_CASE("an instance is zeroed with its parameters at their defaults, and reads and writes by handle and by path",
          "[host][instance]")
{
    std::string errors;
    const auto program = tee::Program::fromFiles(smallProgram(), "main.fm", {}, errors);
    REQUIRE(program != nullptr);
    const auto schema = tee::Schema::of(program->fminus(), "Rig", errors);
    REQUIRE(schema != nullptr);
    tee::Instance rig(*program->fminus().module("Rig"));
    CHECK(reinterpret_cast<std::uintptr_t>(rig.data()) % alignof(std::max_align_t) == 0);

    double value = 99.0;
    REQUIRE(rig.read(*schema, "tank[2].in.area", value, errors));
    CHECK(value == 0.0);
    REQUIRE(rig.read(*schema, "level", value, errors));
    CHECK(value == -12.0);                                     // its fminus::default
    REQUIRE(rig.read(*schema, "mode", value, errors));
    CHECK(value == 2.0);

    REQUIRE(rig.write(*schema, "tank[2].in.area", 0.25, errors));
    REQUIRE(rig.write(*schema, "tank[1].cycles", 7.9, errors));     // an int: truncated toward 0
    REQUIRE(rig.write(*schema, "single.open", 3.0, errors));        // a bool: true unless 0
    REQUIRE(rig.write(*schema, "slow", 1.0e-300, errors));
    REQUIRE(rig.write(*schema, "level", -30.0, errors));            // a parameter: through FMinus's setValue
    CHECK_FALSE(rig.write(*schema, "buffer", 1.0, errors));
    CHECK_FALSE(rig.write(*schema, "single", 1.0, errors));

    const auto area = schema->field<float>("tank[2].in.area", errors);
    const auto cycles = schema->field<std::int32_t>("tank[1].cycles", errors);
    const auto open = schema->field<bool>("single.open", errors);
    const auto slow = schema->field<double>("slow", errors);
    const auto grid = schema->field<float>("grid", errors);
    REQUIRE((area && cycles && open && slow && grid));
    CHECK(rig.get(area) == 0.25f);
    CHECK(rig.get(cycles) == 7);
    CHECK(rig.get(open) == true);
    CHECK(rig.get(slow) == 1.0e-300);
    const fminus::Program::Parameter* level = program->fminus().parameter("Rig::level");
    REQUIRE(level != nullptr);
    CHECK(fminus::Program::value(rig.data(), *level) == -30.0);

    // the program sees what the host wrote, where the schema said it was
    for (std::size_t i = 0; i < 4; ++i)
        rig.set(grid, static_cast<float>(i + 1), i);
    auto* sum = program->function<float(void*, int)>("Rig::sum", errors);
    REQUIRE(sum != nullptr);
    CHECK(sum(rig.data(), 4) == Catch::Approx(1.0 + 2.0 + 3.0 + 4.0 - 30.0));

    rig.reset();
    CHECK(rig.get(area) == 0.0f);
    CHECK(fminus::Program::value(rig.data(), *level) == -12.0);
}

TEST_CASE("with the checks, an index outside its array stops the call and is reported; a loop past its budget too",
          "[host][program][checks]")
{
    tee::Program::Options options;
    options.checks = true;
    std::string errors;
    const auto program = tee::Program::fromFiles(smallProgram(), "main.fm", options, errors);
    INFO(errors);
    REQUIRE(program != nullptr);
    tee::Instance rig(*program->fminus().module("Rig"));
    auto* sum = program->function<float(void*, int)>("Rig::sum", errors);
    REQUIRE(sum != nullptr);
    fminus::Program& fm = program->fminus();

    fm.clearBoundsViolation();
    fm.setLoopBudget(1000);
    (void)sum(rig.data(), 4);
    CHECK_FALSE(fm.boundsViolation().has_value());
    CHECK_FALSE(fm.loopBudgetExceeded());

    (void)sum(rig.data(), 9);                 // grid[0][8]: past the row of 4
    const auto failed = fm.boundsViolation();
    REQUIRE(failed.has_value());
    CHECK(failed->file == "main.fm");
    CHECK(failed->index >= 4);
    INFO(failed->message());
    CHECK(failed->message().find("out of bounds") != std::string::npos);

    // a loop within its budget runs whole; past it, the call returns early and says so
    auto* steps = program->function<int(void*, int)>("Rig::steps", errors);
    REQUIRE(steps != nullptr);
    fm.clearBoundsViolation();
    fm.setLoopBudget(1000);
    CHECK(steps(rig.data(), 27) == 111);
    CHECK_FALSE(fm.loopBudgetExceeded());
    fm.setLoopBudget(20);
    (void)steps(rig.data(), 27);
    CHECK(fm.loopBudgetExceeded());
}
