// tests/sim/valvetrain/Engine.h: the whole simulator (sim/engine.fm through tee::EnginePlayer) for card E's integrated
// tests: a build, Spec fields set by path, started, held on the engine dyno at a speed with the throttle wide open,
// the brake torque and the telemetry read; the output kept for spectra.
#pragma once

#include "tee/EnginePlayer.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace tee::test::lab
{
    class Rig
    {
    public:
        static constexpr double kRate = 48000.0;
        static constexpr int kBlock = 512;

        explicit Rig(const std::string& build = "v8", const std::vector<std::pair<std::string, double>>& spec = {})
        {
            EnginePlayer::Options options;
            options.directory = std::filesystem::path(TEE_SOURCE_DIR) / "sim";
            options.sampleRate = kRate;
            std::string errors;
            player_ = EnginePlayer::create(options, errors);
            INFO(errors);
            REQUIRE(player_ != nullptr);
            EngineBuild b;
            std::string warnings;
            const auto file = std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / (build + ".json");
            const bool ok = loadBuild(*player_, file, b, errors, warnings);
            INFO(errors);
            REQUIRE(ok);
            applyBuild(*player_, b);
            for (const auto& [path, value] : spec)
                set(path, value);
        }

        EnginePlayer& player() { return *player_; }
        EnginePlayer::Controls& ctl() { return player_->controls(); }

        void set(const std::string& path, double value)
        {
            std::string error;
            const bool ok = player_->setSpec(path, value, error);
            INFO(path << ": " << error);
            REQUIRE(ok);
        }

        double tel(const std::string& name) const
        {
            const int i = player_->telemetryIndex(name);
            INFO(name);
            REQUIRE(i >= 0);
            return player_->telemetry(std::size_t(i));
        }

        double field(const std::string& path) const
        {
            double v = 0.0;
            std::string error;
            const bool ok = player_->read(path, v, error);
            INFO(path << ": " << error);
            REQUIRE(ok);
            return v;
        }

        void write(const std::string& path, double value)
        {
            std::string error;
            const bool ok = player_->instance().write(player_->schema(), path, value, error);
            INFO(error);
            REQUIRE(ok);
        }

        template <typename F>
        void play(double seconds, F each)
        {
            const int blocks = int(seconds * kRate / kBlock + 0.5);
            for (int i = 0; i < blocks; ++i)
            {
                player_->process(left_.data(), right_.data(), kBlock);
                if (keep_)
                    out_.insert(out_.end(), left_.begin(), left_.end());
                each();
            }
        }
        void play(double seconds) { play(seconds, [] {}); }

        // Started with the key, warmed (as minutes of running leave it), let idle
        void start(double idle = 1.0)
        {
            ctl().ignition = EnginePlayer::kIgnitionStart;
            play(1.0);
            ctl().ignition = EnginePlayer::kIgnitionOn;
            write("heat.tCool", 363.0);
            write("heat.tOil", 373.0);
            write("heat.tSurf", 450.0);
            play(idle);
        }

        // The engine dyno holding `rpm` with the throttle at `throttle`: settled, then the brake torque's mean (N m)
        double torqueAt(double rpm, double throttle = 1.0, double settle = 1.2, double measure = 0.6)
        {
            ctl().mode = 2;
            ctl().dynoSpeed = float(rpm * 3.14159265358979 / 30.0);
            ctl().dynoRate = 0.0f;
            ctl().throttle = float(throttle);
            play(settle);
            double sum = 0.0;
            int n = 0;
            play(measure, [&] {
                sum += tel("engine.torque");
                n += 1;
            });
            return sum / n;
        }

        void keep(bool on)
        {
            keep_ = on;
            out_.clear();
        }
        const std::vector<float>& output() const { return out_; }

    private:
        std::unique_ptr<EnginePlayer> player_;
        std::vector<float> left_ = std::vector<float>(kBlock), right_ = std::vector<float>(kBlock);
        std::vector<float> out_;
        bool keep_ = false;
    };
} // namespace tee::test::lab
