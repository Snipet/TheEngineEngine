// tests/sim/twostroke/Bench.h: the whole simulator (sim/engine.fm through tee::EnginePlayer) with a two-stroke build
// of content/builds/engines: started, run on the bench or the engine dyno, its telemetry, Scope and sound read.
#pragma once

#include "tee/EnginePlayer.h"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace tee::test::twostroke
{
    class Bench
    {
    public:
        static constexpr double kRate = 48000.0;
        static constexpr int kBlock = 512;

        explicit Bench(const std::string& build)
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
            const auto file = std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / "engines" / (build + ".json");
            const bool ok = loadBuild(*player_, file, b, errors, warnings);
            INFO(errors);
            REQUIRE(ok);
            applyBuild(*player_, b);
        }

        // Whether sim/engine.fm runs the two-stroke (card D's hook: Engine::two); the engine tests skip until it does
        bool hooked() const
        {
            double v = 0.0;
            std::string error;
            return player_->read("cycleUse", v, error);
        }

        EnginePlayer& player() { return *player_; }
        EnginePlayer::Controls& ctl() { return player_->controls(); }

        double tel(const std::string& name) const
        {
            const int i = player_->telemetryIndex(name);
            REQUIRE(i >= 0);
            return player_->telemetry(std::size_t(i));
        }

        double field(const std::string& path) const
        {
            double v = 0.0;
            std::string error;
            const bool ok = player_->read(path, v, error);
            INFO(error);
            REQUIRE(ok);
            return v;
        }

        // Plays `seconds`, keeping the sound (left + right) when `keep`; calls `each` after every block
        template <typename F>
        void play(double seconds, F each, bool keep = false)
        {
            const int blocks = int(seconds * kRate / kBlock + 0.5);
            for (int i = 0; i < blocks; ++i)
            {
                const auto t0 = std::chrono::steady_clock::now();
                player_->process(left_.data(), right_.data(), kBlock);
                busy_ += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                played_ += double(kBlock) / kRate;
                for (int k = 0; k < kBlock; ++k)
                {
                    peak_ = std::max(peak_, double(std::max(std::fabs(left_[k]), std::fabs(right_[k]))));
                    if (!std::isfinite(left_[k]) || !std::isfinite(right_[k]))
                        bad_ += 1;
                    if (keep)
                        kept_.push_back(0.5f * (left_[k] + right_[k]));
                }
                each();
            }
        }
        void play(double seconds) { play(seconds, [] {}); }

        void start(double idle = 2.0)
        {
            ctl().ignition = EnginePlayer::kIgnitionStart;
            play(1.0);
            ctl().ignition = EnginePlayer::kIgnitionOn;
            play(idle);
        }

        double rpm() const { return tel("engine.speed") * 30.0 / 3.14159265358979; }
        double load() const { return busy_ / played_; }
        double peak() const { return peak_; }
        int bad() const { return bad_; }
        std::vector<float>& kept() { return kept_; }

    private:
        std::unique_ptr<EnginePlayer> player_;
        std::vector<float> left_ = std::vector<float>(kBlock), right_ = std::vector<float>(kBlock);
        std::vector<float> kept_;
        double busy_ = 0.0, played_ = 0.0, peak_ = 0.0;
        int bad_ = 0;
    };
} // namespace tee::test::twostroke
