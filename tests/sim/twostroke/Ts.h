// tests/sim/twostroke/Ts.h: the two-stroke's bench (rig2.fm with sim/core/twostroke/*.fm and the reference's pipes),
// compiled once; an instance per test, its spec at its defaults, its pipes' memory taken and given back.
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

namespace tee::test::twostroke
{
    inline std::string readFile(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::binary);
        std::ostringstream text;
        text << in.rdbuf();
        return text.str();
    }

    inline Program& program()
    {
        // (never destroyed: a static that owns a compiled program must not run its destructor after FMinus's and LLVM's own
        // statics are gone, as happens on macOS)
        static Program* compiled = [] {
            const std::filesystem::path root(TEE_SOURCE_DIR);
            Program::Files files;
            for (const char* dir : {"core", "core/twostroke"})
                for (const auto& entry : std::filesystem::directory_iterator(root / "sim" / dir))
                    if (entry.path().extension() == ".fm")
                        files[std::string(dir) + "/" + entry.path().filename().string()] = readFile(entry.path());
            files["ref/pipes.fm"] = readFile(root / "sim" / "ref" / "pipes.fm");
            files["rig2.fm"] = readFile(root / "tests" / "sim" / "twostroke" / "rig2.fm");
            std::string errors;
            auto p = Program::fromFiles(files, "rig2.fm", {}, errors);
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
        static constexpr double kRate = 96000.0;

        Rig()
        {
            std::string error;
            schema_ = Schema::of(program().fminus(), "TsRig", error);
            INFO(error);
            REQUIRE(schema_ != nullptr);
            instance_ = std::make_unique<Instance>(*program().fminus().module("TsRig"));
            schema_->forEach([&](const std::string& path, const Schema::Node& node, std::size_t) {
                if (path.rfind("spec.", 0) != 0)
                    return;
                if (const auto* d = node.attribute("fminus::default"); d != nullptr && !d->arguments.empty())
                    set(path, d->arguments[0].number);
            });
            method<void(void*, float)>("init")(data(), float(kRate));
        }

        ~Rig() { method<void(void*)>("end")(data()); }

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
            auto* f = program().function<Signature>("TsRig::" + name, errors);
            INFO(errors);
            REQUIRE(f != nullptr);
            return f;
        }

        void* data() { return instance_->data(); }

        void build(double tPipe = 800.0) { method<void(void*, float)>("build")(data(), float(tPipe)); }

        // a run at a speed: about `seconds` of it, the last `tail` cycles measured
        void run(double rpm, double throttle, double seconds = 0.6, int tail = 40)
        {
            const int n = int(seconds * kRate);
            method<void(void*, int, float, float, int)>("run")(data(), n, float(rpm), float(throttle), tail);
        }

        double meanTorque() const { return get("tqSum") / get("samples"); }
        double meanTrapped() const { return get("freshSum") / get("cyc"); }   // the fresh charge trapped, kg
        double meanDelivered() const { return get("deliveredSum") / get("cyc"); }

    private:
        std::unique_ptr<Schema> schema_;
        std::unique_ptr<Instance> instance_;
    };
} // namespace tee::test::twostroke
