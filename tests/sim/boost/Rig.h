// tests/sim/boost/Rig.h: card F's made-up engine (rig.fm) through the JIT host: the forced induction's files
// (sim/boost/*.fm, as "boost/<name>") and the rig compiled together, an instance of a rig module, its methods by name
// and its fields by path.
#pragma once

#include "tee/Denormals.h"
#include "tee/Instance.h"
#include "tee/Program.h"
#include "tee/Schema.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

namespace tee::test::boost
{
    inline std::string readFile(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::binary);
        std::ostringstream out;
        out << in.rdbuf();
        return out.str();
    }

    // The forced induction's files and the rig's, compiled once for the whole test run
    inline tee::Program& program()
    {
        // (never destroyed: a static that owns a compiled program would be destroyed after the JIT's own statics on
        // macOS, and crash at exit)
        static tee::Program* compiled = [] {
            tee::Program::Files files;
            const std::filesystem::path root = TEE_SOURCE_DIR;
            for (const auto& entry : std::filesystem::directory_iterator(root / "sim" / "boost"))
                if (entry.path().extension() == ".fm")
                    files["boost/" + entry.path().filename().string()] = readFile(entry.path());
            files["spec/boost.fm"] = readFile(root / "sim" / "spec" / "boost.fm");
            files["rig.fm"] = readFile(root / "tests" / "sim" / "boost" / "rig.fm");
            std::string errors;
            auto p = tee::Program::fromFiles(files, "rig.fm", {}, errors);
            INFO(errors);
            REQUIRE(p != nullptr);
            REQUIRE(p->link(errors));
            return p.release();
        }();
        return *compiled;
    }

    // An instance of a module of the rig's program, with typed access by path
    class Module
    {
    public:
        explicit Module(const char* name)
        {
            std::string error;
            schema_ = tee::Schema::of(program().fminus(), name, error);
            INFO(error);
            REQUIRE(schema_ != nullptr);
            instance_ = std::make_unique<tee::Instance>(*program().fminus().module(name));
            name_ = name;
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
            std::string error;
            auto* f = program().function<Signature>(name_ + "::" + name, error);
            INFO(error);
            REQUIRE(f != nullptr);
            return f;
        }

        void* data() { return instance_->data(); }

        // Every field under `prefix` (a module held by value, "spec") set to its fminus::default, as the host does
        void defaults(const std::string& prefix)
        {
            schema_->forEach([&](const std::string& path, const tee::Schema::Node& node, std::size_t) {
                if (path.rfind(prefix + ".", 0) != 0)
                    return;
                const auto* d = node.attribute("fminus::default");
                if (d == nullptr || d->arguments.empty())
                    return;
                for (std::size_t i = 0; i < node.count(); ++i)
                    set(node.count() > 1 ? path + "[" + std::to_string(i) + "]" : path, d->arguments[0].number);
            });
        }

    private:
        std::unique_ptr<tee::Schema> schema_;
        std::unique_ptr<tee::Instance> instance_;
        std::string name_;
    };

    // The made-up engine: displacement (m^3), cylinders, redline (rpm)
    class Rig : public Module
    {
    public:
        Rig(double vd = 0.002, int cylinders = 4, double redline = 7000.0) : Module("BoostRig")
        {
            run_ = method<void(void*, int)>("run");
            clear_ = method<void(void*)>("clear");
            defaults("spec");
            const tee::ScopedFlushDenormals flush;
            method<void(void*, float, int, float)>("init")(data(), static_cast<float>(vd), cylinders,
                                                           static_cast<float>(redline));
        }

        // a spec field, applied at the next run (its rev moved)
        void spec(const std::string& field, double value)
        {
            set("spec." + field, value);
            set("spec.rev", get("spec.rev") + 1.0);
        }

        void run(double seconds)
        {
            const tee::ScopedFlushDenormals flush;
            int left = static_cast<int>(seconds * 96000.0 + 0.5);
            while (left > 0)
            {
                const int n = left < 4800 ? left : 4800;
                run_(data(), n);
                left -= n;
            }
        }

        void clear() { clear_(data()); }

        // a mean since clear()
        double mean(const std::string& sum) const { return get(sum) / get("n"); }

        double boost() const { return get("boost.v2.p") - get("boost.pOut"); }
        double shaftRpm(int t = 0) const { return get("boost.shaft[" + std::to_string(t) + "].omega") * 60.0 / 6.283185307; }

    private:
        void (*run_)(void*, int) = nullptr;
        void (*clear_)(void*) = nullptr;
    };
} // namespace tee::test::boost
