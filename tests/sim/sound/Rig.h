// tests/sim/sound/Rig.h: package M's voices (sim/sound/*.fm, sim/lib/*.fm, sim/spec/voices.fm) driven by rig.fm,
// compiled once, an instance per test; fields by path, a run into a buffer, and the measures the tests use (a line's
// level by Goertzel, the strongest line in a band, RMS, the largest step between samples).
#pragma once

#include "tee/Denormals.h"
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
#include <vector>

namespace tee::test::sound
{
    constexpr double kPi = 3.14159265358979323846;
    constexpr double kRate = 96000.0;

    enum Channel { Gearbox = 0, Axle, Front, Head, Tank, Gate, Starter, Rattle, Fan, Timing, Belt, Blower, Chain, Clutch, All = 99 };

    inline std::string readFile(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::binary);
        std::ostringstream text;
        text << in.rdbuf();
        return text.str();
    }

    inline Program& program()
    {
        // (never destroyed: see 01-rules.md, found on macOS)
        static Program* compiled = [] {
            Program::Files files;
            const std::filesystem::path root = TEE_SOURCE_DIR;
            for (const char* dir : {"sound", "lib", "spec"})
                for (const auto& entry : std::filesystem::directory_iterator(root / "sim" / dir))
                    if (entry.path().extension() == ".fm")
                        files[std::string(dir) + "/" + entry.path().filename().string()] = readFile(entry.path());
            files["rig.fm"] = readFile(root / "tests" / "sim" / "sound" / "rig.fm");
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
            schema_->forEach([&](const std::string& path, const Schema::Node& node, std::size_t) {
                if (path.rfind("spec.", 0) != 0)
                    return;
                if (const auto* d = node.attribute("fminus::default"); d != nullptr && !d->arguments.empty())
                    set(path, d->arguments[0].number);
            });
            set("spec.model", 1);
            call<void(void*, float)>("init")(data(), static_cast<float>(kRate));
            run_ = call<void(void*, int, float*, int)>("run");
            // a car idling in neutral, the clutch in, nothing else
            set("crankHz", 13.33);
            set("coolant", 300);
            set("peak", 400);
            set("fd", 3.6);
            set("cylinders", 8);
            set("displacement", 0.005);
            set("engaged", 1);
            set("ignition", 1);
            set("state", 2);
            set("primaryRatio", 1.7);
            set("frontTeeth", 16);
            set("lever", 1);
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
            INFO(path << ": " << error);
            REQUIRE(ok);
        }

        // In gear g of the reference car's six (ratios 3.15 ... 0.86, final drive 3.6) at the gearbox's input speed w
        // (rad/s), the clutch in, carrying `load` of the engine's most
        void inGear(int g, double w, double load)
        {
            static const double r[6] = {3.15, 2.18, 1.62, 1.28, 1.05, 0.86};
            const double rb = g > 0 ? r[g - 1] : (g < 0 ? -3.3 : 0.0);
            set("driven", 1);
            set("gear", g);
            set("wIn", w);
            set("crankHz", w / (2 * kPi));
            set("ratioTotal", rb * 3.6);
            set("wOut", g != 0 ? w / std::fabs(rb) : 0.0);
            set("torque", load * 400.0);
        }

        // s seconds of the channel
        std::vector<float> run(double seconds, int which)
        {
            const int n = static_cast<int>(seconds * kRate / 16.0 + 0.5) * 16;
            std::vector<float> out(static_cast<std::size_t>(n));
            ScopedFlushDenormals flush;
            int at = 0;
            while (at < n)
            {
                const int m = std::min(4096, n - at);
                run_(data(), m, out.data() + at, which);
                at += m;
            }
            return out;
        }

        void* data() { return instance_->data(); }

        template <typename Signature>
        Signature* call(const std::string& name)
        {
            std::string error;
            auto* f = program().function<Signature>("Rig::" + name, error);
            INFO(error);
            REQUIRE(f != nullptr);
            return f;
        }

    private:
        std::unique_ptr<Schema> schema_;
        std::unique_ptr<Instance> instance_;
        void (*run_)(void*, int, float*, int) = nullptr;
    };

    // The amplitude of the line at f (Hz) in x (a Hann window's, corrected: a sine of amplitude a reads a)
    inline double line(const std::vector<float>& x, double f)
    {
        const std::size_t n = x.size();
        double re = 0.0, im = 0.0, wsum = 0.0;
        for (std::size_t i = 0; i < n; ++i)
        {
            const double w = 0.5 - 0.5 * std::cos(2 * kPi * static_cast<double>(i) / static_cast<double>(n));
            const double ph = 2 * kPi * f * static_cast<double>(i) / kRate;
            re += w * x[i] * std::cos(ph);
            im += w * x[i] * std::sin(ph);
            wsum += w;
        }
        return 2.0 * std::sqrt(re * re + im * im) / wsum;
    }

    // The strongest line from lo to hi Hz, in steps of `step` Hz
    inline double strongest(const std::vector<float>& x, double lo, double hi, double step)
    {
        double best = lo, bestA = -1.0;
        for (double f = lo; f <= hi; f += step)
        {
            const double a = line(x, f);
            if (a > bestA)
            {
                bestA = a;
                best = f;
            }
        }
        return best;
    }

    inline double rms(const std::vector<float>& x, std::size_t from = 0, std::size_t to = 0)
    {
        if (to == 0 || to > x.size())
            to = x.size();
        double s = 0.0;
        for (std::size_t i = from; i < to; ++i)
            s += static_cast<double>(x[i]) * x[i];
        return to > from ? std::sqrt(s / static_cast<double>(to - from)) : 0.0;
    }

    inline double maxStep(const std::vector<float>& x, std::size_t from = 1, std::size_t to = 0)
    {
        if (to == 0 || to > x.size())
            to = x.size();
        double m = 0.0;
        for (std::size_t i = std::max<std::size_t>(from, 1); i < to; ++i)
            m = std::max(m, std::fabs(static_cast<double>(x[i]) - x[i - 1]));
        return m;
    }

    inline double db(double a, double b) { return 20.0 * std::log10(std::max(a, 1e-30) / std::max(b, 1e-30)); }
}
