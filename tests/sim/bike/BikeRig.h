// tests/sim/bike/BikeRig.h: the motorcycle (sim/vehicle/bike.fm) driven by rig.fm's made-up engine, as the tests reach it: compiled once
// per process from sim/vehicle/ and this directory's rig.fm, an instance per test, fields by path.
//
//   tee::bike::Rig rig;                        // at rest, in neutral, the Spec's defaults, the engine idling
//   rig.set("bike.spec.mass", 200);  rig.settle();
//   rig.set("up", 1);  rig.set("throttle", 1.0);
//   rig.seconds(5.0);
//   CHECK(rig.get("bike.tel.speed") > 20.0);
#pragma once

#include "tee/Instance.h"
#include "tee/Program.h"
#include "tee/Schema.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>

namespace tee::bike
{
    enum BikeTyre { Touring = 0, Sport, Hypersport, RaceSlick, DualSport, Cruiser };
    enum Surface { Dry = 0, Damp, Wet, Snow, Strip };
    enum Mode { Bench = 0, DriveMode = 1, DynoEngine = 2, DynoChassis = 3 };

    inline std::string readFile(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::binary);
        std::ostringstream s;
        s << in.rdbuf();
        return s.str();
    }

    // The rig's program: rig.fm and every file of sim/vehicle/ under "vehicle/"
    inline tee::Program::Files rigFiles()
    {
        tee::Program::Files files;
        const std::filesystem::path root = TEE_SOURCE_DIR;
        files["rig.fm"] = readFile(root / "tests/sim/bike/rig.fm");
        for (const auto& entry : std::filesystem::directory_iterator(root / "sim/vehicle"))
            if (entry.path().extension() == ".fm")
                files["vehicle/" + entry.path().filename().string()] = readFile(entry.path());
        return files;
    }

    struct Compiled
    {
        std::unique_ptr<tee::Program> program;
        std::unique_ptr<tee::Schema> schema;
        void (*init)(void*, float) = nullptr;
        void (*settle)(void*) = nullptr;
        void (*run)(void*, int) = nullptr;
        void (*clear)(void*) = nullptr;
        float (*random)(void*) = nullptr;
    };

    inline Compiled& compiled(bool checks = false)
    {
        static Compiled plain;
        static Compiled checked;
        Compiled& c = checks ? checked : plain;
        if (c.program)
            return c;
        std::string errors;
        tee::Program::Options options;
        options.checks = checks;
        c.program = tee::Program::fromFiles(rigFiles(), "rig.fm", options, errors);
        INFO(errors);
        REQUIRE(c.program != nullptr);
        c.schema = tee::Schema::of(c.program->fminus(), "BikeRig", errors);
        REQUIRE(c.schema != nullptr);
        c.init = c.program->function<void(void*, float)>("BikeRig::init", errors);
        c.settle = c.program->function<void(void*)>("BikeRig::settle", errors);
        c.run = c.program->function<void(void*, int)>("BikeRig::runNonblocking", errors);
        c.clear = c.program->function<void(void*)>("BikeRig::clearMeasures", errors);
        c.random = c.program->function<float(void*)>("BikeRig::random", errors);
        INFO(errors);
        CHECK(c.program->warnings().empty());
        CHECK(c.program->fminus().nonBlocking("BikeRig::runNonblocking"));
        REQUIRE(c.init != nullptr);
        REQUIRE(c.run != nullptr);
        return c;
    }

    class Rig
    {
    public:
        explicit Rig(double rate = 96000.0, bool checks = false)
            : c_(compiled(checks)), instance_(*c_.program->fminus().module("BikeRig")), rate_(rate)
        {
            defaults();
            c_.init(instance_.data(), static_cast<float>(rate));
        }

        // Every field of car.spec and car.env at its fminus::default, as the host gives a new build (FMinus runs no
        // constructor)
        void defaults()
        {
            c_.schema->forEach([&](const std::string& path, const tee::Schema::Node& node, std::size_t offset) {
                if (path.rfind("bike.spec.", 0) != 0 && path.rfind("bike.bs.", 0) != 0 && path.rfind("bike.env.", 0) != 0)
                    return;
                const auto* d = node.attribute("fminus::default");
                if (d == nullptr || d->arguments.empty())
                    return;
                const tee::Schema::Handle handle{&node, offset, node.count()};
                for (std::size_t i = 0; i < node.count(); ++i)
                    instance_.write(handle, d->arguments[0].number, i);
            });
        }

        double get(const std::string& path) const
        {
            double value = 0.0;
            std::string error;
            const bool ok = instance_.read(*c_.schema, path, value, error);
            INFO(error);
            REQUIRE(ok);
            return value;
        }

        void set(const std::string& path, double value)
        {
            std::string error;
            const bool ok = instance_.write(*c_.schema, path, value, error);
            INFO(error);
            REQUIRE(ok);
        }

        // After writing car.spec.*, car.env.* or the engine's fields
        void settle() { c_.settle(instance_.data()); }
        void clear() { c_.clear(instance_.data()); }
        float random() { return c_.random(instance_.data()); }

        void samples(int n) { c_.run(instance_.data(), n); }
        // Runs whole blocks of 16 samples for about `s` seconds
        void seconds(double s)
        {
            int n = static_cast<int>(s * rate_ / 16.0 + 0.5) * 16;
            while (n > 0)
            {
                const int k = n > 4096 ? 4096 : n;
                c_.run(instance_.data(), k);
                n -= k;
            }
        }

        // Runs until `done` is true (checked every 16 samples) or `limit` seconds pass; the seconds it took
        template <typename Done>
        double until(Done done, double limit)
        {
            const double t0 = get("t");
            while (get("t") - t0 < limit)
            {
                c_.run(instance_.data(), 16);
                if (done())
                    break;
            }
            return get("t") - t0;
        }

        // The address of a field (a nested module's, to call its methods)
        void* at(const std::string& path)
        {
            tee::Schema::Handle handle;
            std::string error;
            const bool ok = c_.schema->find(path, handle, error);
            INFO(error);
            REQUIRE(ok);
            return static_cast<unsigned char*>(instance_.data()) + handle.offset;
        }

        // A function of the program, by name ("VehTyreCurve::at")
        template <typename Signature>
        Signature* function(const std::string& name)
        {
            std::string errors;
            Signature* f = c_.program->function<Signature>(name, errors);
            INFO(errors);
            REQUIRE(f != nullptr);
            return f;
        }

        double rate() const { return rate_; }
        tee::Instance& instance() { return instance_; }

    private:
        Compiled& c_;
        tee::Instance instance_;
        double rate_;
    };
} // namespace tee::bike
