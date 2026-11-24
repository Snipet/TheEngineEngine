// tests/sim/vehicle/Rig.h: the vehicle model driven by rig.fm's made-up engine, as the tests reach it: compiled once
// per process from sim/vehicle/ and this directory's rig.fm, an instance per test, fields by path.
//
//   tee::vehicle::Rig rig;                     // at rest, in neutral, the Spec's defaults, the engine idling
//   rig.set("car.spec.drive", VEH_RWD);  rig.settle();
//   rig.set("lever", 1);  rig.set("throttle", 1.0);
//   rig.seconds(5.0);
//   CHECK(rig.get("car.tel.speed") > 20.0);
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

namespace tee::vehicle
{
    enum Drive { FWD = 0, RWD = 1, AWD = 2 };
    enum Box { Manual = 0, Sequential = 1, DualClutch = 2, Automatic = 3, Cvt = 4 };
    enum Compound { Economy = 0, Street, Sport, SemiSlick, Slick, DragRadial, DragSlick, Winter, Truck, Motorcycle };
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
        files["rig.fm"] = readFile(root / "tests/sim/vehicle/rig.fm");
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
        void (*audit)(void*) = nullptr;
        double (*kinetic)(void*) = nullptr;
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
        c.schema = tee::Schema::of(c.program->fminus(), "Rig", errors);
        REQUIRE(c.schema != nullptr);
        c.init = c.program->function<void(void*, float)>("Rig::init", errors);
        c.settle = c.program->function<void(void*)>("Rig::settle", errors);
        c.run = c.program->function<void(void*, int)>("Rig::runNonblocking", errors);
        c.clear = c.program->function<void(void*)>("Rig::clearMeasures", errors);
        c.audit = c.program->function<void(void*)>("Rig::startAudit", errors);
        c.kinetic = c.program->function<double(void*)>("Rig::kinetic", errors);
        c.random = c.program->function<float(void*)>("Rig::random", errors);
        INFO(errors);
        CHECK(c.program->warnings().empty());
        CHECK(c.program->fminus().nonBlocking("Rig::runNonblocking"));
        REQUIRE(c.init != nullptr);
        REQUIRE(c.run != nullptr);
        REQUIRE(c.kinetic != nullptr);
        return c;
    }

    class Rig
    {
    public:
        explicit Rig(double rate = 96000.0, bool checks = false)
            : c_(compiled(checks)), instance_(*c_.program->fminus().module("Rig")), rate_(rate)
        {
            defaults();
            c_.init(instance_.data(), static_cast<float>(rate));
        }

        // Every field of car.spec and car.env at its fminus::default, as the host gives a new build (FMinus runs no
        // constructor)
        void defaults()
        {
            c_.schema->forEach([&](const std::string& path, const tee::Schema::Node& node, std::size_t offset) {
                if (path.rfind("car.spec.", 0) != 0 && path.rfind("car.env.", 0) != 0)
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
        void audit() { c_.audit(instance_.data()); }
        double kinetic() { return c_.kinetic(instance_.data()); }
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

        // A driver pulling away: in `gear` with the clutch down, the throttle at `throttle`; the pedal comes up while the
        // engine stays above `holdW` (rad/s) and goes back a little when it sags, until the clutch has locked. The
        // seconds it took.
        double pullAway(int gear, double throttle, double holdW = 180.0, double limit = 8.0)
        {
            set("clutch", 1.0);
            seconds(0.2);
            set("lever", gear);
            seconds(0.2);
            set("throttle", throttle);
            seconds(0.3);
            double pedal = 0.75;
            const double t0 = get("t");
            while (get("t") - t0 < limit)
            {
                pedal += get("w") > holdW ? -0.003 : 0.02;
                pedal = pedal < 0.0 ? 0.0 : (pedal > 1.0 ? 1.0 : pedal);
                set("clutch", pedal);
                seconds(0.005);
                if (pedal <= 0.0 && std::fabs(get("car.slipC")) < 0.5)
                    break;
            }
            set("clutch", 0.0);
            return get("t") - t0;
        }

        double rate() const { return rate_; }
        tee::Instance& instance() { return instance_; }

    private:
        Compiled& c_;
        tee::Instance instance_;
        double rate_;
    };
} // namespace tee::vehicle
