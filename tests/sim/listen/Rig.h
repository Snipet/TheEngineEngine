// tests/sim/listen/Rig.h: the listener's rig (rig.fm) as the tests reach it, and what they measure with it.
//
// The rig is compiled once per run and never destroyed (a static that owns a compiled program must not run its
// destructor after FMinus's and LLVM's own statics are gone, as on macOS); each test makes its own instance.
#pragma once

#include "tee/Denormals.h"
#include "tee/Instance.h"
#include "tee/Program.h"
#include "tee/Schema.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <memory>
#include <string>
#include <vector>

namespace tee::test::listen
{
    constexpr double kRate = 96000.0;
    constexpr double kPi = 3.14159265358979323846;

    // The rig's sources (LRig::src)
    enum Src
    {
        kTailL = 0,
        kTailR = 1,
        kIntake = 2,
        kTorque = 3,
        kMech = 4,
        kBlock = 5,
        kTurbo = 6,
        kStarter = 7,
        kComb = 8,
        kShell = 9,
        kRoad = 10,
        kTails = 11
    };

    inline std::string readFile(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::binary);
        std::ostringstream text;
        text << in.rdbuf();
        return text.str();
    }

    inline tee::Program& program()
    {
        static tee::Program* compiled = [] {
            // sim/'s lib, listen and spec, with the rig at the root (listen.fm names its includes from sim/)
            tee::Program::Files files;
            const std::filesystem::path sim = std::filesystem::path(TEE_SOURCE_DIR) / "sim";
            for (const char* dir : {"lib", "listen", "spec"})
                for (const auto& entry : std::filesystem::directory_iterator(sim / dir))
                    if (entry.path().extension() == ".fm")
                        files[std::string(dir) + "/" + entry.path().filename().string()] = readFile(entry.path());
            files["rig.fm"] = readFile(std::filesystem::path(TEE_SOURCE_DIR) / "tests" / "sim" / "listen" / "rig.fm");
            std::string errors;
            auto made = tee::Program::fromFiles(files, "rig.fm", {}, errors);
            INFO(errors);
            REQUIRE(made != nullptr);
            REQUIRE(made->link(errors));
            return made.release();
        }();
        return *compiled;
    }

    class Rig
    {
    public:
        Rig()
        {
            tee::Program& p = program();
            std::string error;
            schema_ = tee::Schema::of(p.fminus(), "LRig", error);
            INFO(error);
            REQUIRE(schema_ != nullptr);
            inst_ = std::make_unique<tee::Instance>(*p.fminus().module("LRig"));
            // the sound section's defaults, as the host writes them
            schema_->forEach([&](const std::string& path, const tee::Schema::Node& node, std::size_t) {
                if (path.rfind("ss.", 0) != 0)
                    return;
                if (const auto* d = node.attribute("fminus::default"); d != nullptr && !d->arguments.empty())
                    set(path, d->arguments[0].number);
            });
            init_ = p.function<void(void*, float)>("LRig::init", error);
            end_ = p.function<void(void*)>("LRig::end", error);
            run_ = p.function<void(void*, float*, float*, int)>("LRig::run", error);
            reset_ = p.function<void(void*)>("LRig::reset", error);
            INFO(error);
            REQUIRE(reset_ != nullptr);
            tee::ScopedFlushDenormals flush;
            init_(inst_->data(), float(kRate));
            // the balance is compared without model 1's headroom stage (its own test has it)
            set("ln.kHead", 1.0);
            // and with true levels: the auto level would hide the laws these tests measure
            set("ss.autoLevel", 0.0);
        }

        ~Rig()
        {
            tee::ScopedFlushDenormals flush;
            end_(inst_->data());
        }

        Rig(const Rig&) = delete;
        Rig& operator=(const Rig&) = delete;

        void set(const std::string& path, double value)
        {
            std::string error;
            const bool ok = inst_->write(*schema_, path, value, error);
            INFO(path << ": " << error);
            REQUIRE(ok);
        }

        double get(const std::string& path) const
        {
            std::string error;
            double v = 0.0;
            const bool ok = inst_->read(*schema_, path, v, error);
            INFO(path << ": " << error);
            REQUIRE(ok);
            return v;
        }

        void reset()
        {
            tee::ScopedFlushDenormals flush;
            reset_(inst_->data());
        }

        // Plays `seconds` (rounded to whole parts of 16), appended to left() and right() when kept
        void play(double seconds, bool keep = true)
        {
            const int n = std::max(16, int(seconds * kRate / 16.0 + 0.5) * 16);
            std::vector<float> l(std::size_t(n), 0.0f);
            std::vector<float> r(std::size_t(n), 0.0f);
            {
                tee::ScopedFlushDenormals flush;
                run_(inst_->data(), l.data(), r.data(), n);
            }
            if (keep)
            {
                left_.insert(left_.end(), l.begin(), l.end());
                right_.insert(right_.end(), r.begin(), r.end());
            }
        }

        void clear()
        {
            left_.clear();
            right_.clear();
        }

        const std::vector<float>& left() const { return left_; }
        const std::vector<float>& right() const { return right_; }

        // The kept output's mean square, both channels summed
        double power(std::size_t from = 0) const
        {
            double s = 0.0;
            for (std::size_t i = from; i < left_.size(); ++i)
                s += double(left_[i]) * left_[i] + double(right_[i]) * right_[i];
            return s / double(std::max<std::size_t>(1, left_.size() - from));
        }

    private:
        std::unique_ptr<tee::Schema> schema_;
        std::unique_ptr<tee::Instance> inst_;
        void (*init_)(void*, float) = nullptr;
        void (*end_)(void*) = nullptr;
        void (*run_)(void*, float*, float*, int) = nullptr;
        void (*reset_)(void*) = nullptr;
        std::vector<float> left_;
        std::vector<float> right_;
    };

    inline double dB(double powerRatio) { return 10.0 * std::log10(std::max(powerRatio, 1e-30)); }

    // The level (power, both channels) of source `src` heard with white noise at the rig's default amplitude, after
    // `settle` seconds, over `seconds`
    inline double heard(Rig& rig, int src, double settle = 0.3, double seconds = 1.0)
    {
        rig.set("src", src);
        rig.set("sig", 0);
        rig.reset();
        rig.clear();
        rig.play(settle, false);
        rig.play(seconds);
        return rig.power();
    }
} // namespace tee::test::listen
