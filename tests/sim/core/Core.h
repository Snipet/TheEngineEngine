// tests/sim/core/Core.h: the mechanics' standalone rig (rig.fm with sim/core/*.fm), compiled once, and an instance of
// it per test, its fields reached by path and its methods called by name.
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

namespace tee::test::core
{
    inline std::string readFile(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::binary);
        std::ostringstream text;
        text << in.rdbuf();
        return text.str();
    }

    // The rig, compiled once for the whole run (it is read-only once compiled)
    inline Program& program()
    {
        // (never destroyed: a static that owns a compiled program must not run its destructor after FMinus's and LLVM's own
        // statics are gone, as happens on macOS)
        static Program* compiled = [] {
            Program::Files files;
            for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(TEE_SOURCE_DIR) / "sim" / "core"))
                if (entry.path().extension() == ".fm")
                    files["core/" + entry.path().filename().string()] = readFile(entry.path());
            for (const auto& entry : std::filesystem::directory_iterator(std::filesystem::path(TEE_SOURCE_DIR) / "sim" / "spec"))
                if (entry.path().extension() == ".fm")
                    files["spec/" + entry.path().filename().string()] = readFile(entry.path());
            files["rig.fm"] = readFile(std::filesystem::path(TEE_SOURCE_DIR) / "tests" / "sim" / "core" / "rig.fm");
            std::string errors;
            auto p = Program::fromFiles(files, "rig.fm", {}, errors);
            INFO(errors);
            REQUIRE(p != nullptr);
            REQUIRE(p->link(errors));
            return p.release();
        }();
        return *compiled;
    }

    class Rig
    {
    public:
        Rig()
        {
            std::string error;
            schema_ = Schema::of(program().fminus(), "Rig", error);
            INFO(error);
            REQUIRE(schema_ != nullptr);
            instance_ = std::make_unique<Instance>(*program().fminus().module("Rig"));
            // the spec's defaults, as the host sets them (FMinus runs no constructor)
            schema_->forEach([&](const std::string& path, const Schema::Node& node, std::size_t) {
                if (path.rfind("spec.", 0) != 0)
                    return;
                if (const auto* d = node.attribute("fminus::default"); d != nullptr && !d->arguments.empty())
                    for (std::size_t i = 0; i < node.count(); ++i)
                        set(node.count() > 1 ? path + "[" + std::to_string(i) + "]" : path, d->arguments[0].number);
            });
        }

        double get(const std::string& path) const
        {
            double v = 0.0;
            std::string error;
            const bool ok = instance_->read(*schema_, path, v, error);
            INFO(error);
            REQUIRE(ok);
            return v;
        }

        void set(const std::string& path, double value)
        {
            std::string error;
            const bool ok = instance_->write(*schema_, path, value, error);
            INFO(error);
            REQUIRE(ok);
        }

        template <typename Signature>
        Signature* method(const std::string& name)
        {
            std::string errors;
            auto* f = program().function<Signature>("Rig::" + name, errors);
            INFO(errors);
            REQUIRE(f != nullptr);
            return f;
        }

        void* data() { return instance_->data(); }

        void build(double bore, double stroke, double rod, double compression, double flywheel)
        {
            method<void(void*, float, float, float, float, float)>("build")(data(), float(bore), float(stroke), float(rod),
                                                                          float(compression), float(flywheel));
        }

    private:
        std::unique_ptr<Schema> schema_;
        std::unique_ptr<Instance> instance_;
    };
} // namespace tee::test::core
