// tests/sim/ecu/FmHarness.h: card H's standalone tests' way into an FMinus program: a test program (a .fm file of
// tests/sim/) compiled with every file of sim/ beside it, an instance of one of its modules, its methods by name and
// its fields by path (namespace tee::test::h).
//
//   auto fm = tee::test::h::Fm::load("combustion/trace.fm", "Trace");
//   fm.set("rpm", 2000.0);
//   fm.call<void>("Trace::run");
//   double at = fm.get("knockAt");
#pragma once

#include "tee/Instance.h"
#include "tee/Program.h"
#include "tee/Schema.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

namespace tee::test::h
{
    inline std::string readText(const std::filesystem::path& file)
    {
        std::ifstream in(file, std::ios::binary);
        std::ostringstream text;
        text << in.rdbuf();
        return text.str();
    }

    class Fm
    {
    public:
        // `entry` names a file under tests/sim/ ("combustion/trace.fm"); every .fm file under sim/ is there too, by its
        // path under sim/ ("combustion/knock.fm"), which is what the test's #include writes.
        static Fm load(const std::string& entry, const std::string& module, int optimisation = 2, bool checks = false)
        {
            const std::filesystem::path root = TEE_SOURCE_DIR;
            tee::Program::Files files;
            for (const auto& item : std::filesystem::recursive_directory_iterator(root / "sim"))
                if (item.is_regular_file() && item.path().extension() == ".fm")
                    files[std::filesystem::relative(item.path(), root / "sim").generic_string()] = readText(item.path());
            const std::string name = "test/" + entry;
            files[name] = readText(root / "tests" / "sim" / entry);
            REQUIRE_FALSE(files[name].empty());
            tee::Program::Options options;
            options.optimisation = optimisation;
            options.checks = checks;
            Fm fm;
            std::string errors;
            fm.program = tee::Program::fromFiles(std::move(files), name, options, errors);
            INFO(errors);
            REQUIRE(fm.program != nullptr);
            fm.schema = tee::Schema::of(fm.program->fminus(), module, errors);
            REQUIRE(fm.schema != nullptr);
            const fminus::Program::Module* m = fm.program->fminus().module(module);
            REQUIRE(m != nullptr);
            fm.instance = std::make_unique<tee::Instance>(*m);
            return fm;
        }

        // The method `name` ("Trace::run") called on the instance with `args` (their C types must be the
        // declaration's: float, int; a bool result is read as an int)
        template <typename R, typename... A>
        R call(const std::string& name, A... args)
        {
            std::string errors;
            auto* f = program->function<R(void*, A...)>(name, errors);
            INFO(errors);
            REQUIRE(f != nullptr);
            return f(instance->data(), args...);
        }

        double get(const std::string& path) const
        {
            double value = 0.0;
            std::string error;
            const bool ok = instance->read(*schema, path, value, error);
            INFO(error);
            REQUIRE(ok);
            return value;
        }

        void set(const std::string& path, double value)
        {
            std::string error;
            const bool ok = instance->write(*schema, path, value, error);
            INFO(error);
            REQUIRE(ok);
        }

        std::unique_ptr<tee::Program> program;
        std::unique_ptr<tee::Schema> schema;
        std::unique_ptr<tee::Instance> instance;
    };
} // namespace tee::test::h
