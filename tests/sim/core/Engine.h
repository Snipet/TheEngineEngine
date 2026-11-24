// tests/sim/core/Engine.h: the whole simulator (sim/engine.fm, through tee::EnginePlayer) for card D's engine-level
// tests: a build from content/builds, started, run on the bench or the dyno, its telemetry read.
#pragma once

#include "tee/EnginePlayer.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace tee::test::core
{
    class Bench
    {
    public:
        static constexpr double kRate = 48000.0;
        static constexpr int kBlock = 512;

        // One compiled player for the whole process (some 6.6 CPU-s a compile), never destroyed (01-rules.md section 4:
        // a static that owns a compiled program must not be destroyed at exit), reset for each Bench: every Spec and
        // Controls field at its default and Engine::init run again, as a new player's (EngineTest's "a kept player
        // reset" proves it to the bit against a fresh one). One Bench at a time.
        static EnginePlayer* shared(bool fresh = false)
        {
            static EnginePlayer* kept = nullptr;
            if (kept != nullptr && !fresh)
            {
                kept->reset(kRate);
                return kept;
            }
            EnginePlayer::Options options;
            options.directory = std::filesystem::path(TEE_SOURCE_DIR) / "sim";
            options.sampleRate = kRate;
            std::string errors;
            EnginePlayer* p = EnginePlayer::create(options, errors).release();
            INFO(errors);
            REQUIRE(p != nullptr);
            if (!fresh)
                kept = p;
            return p;
        }

        explicit Bench(const std::string& build, bool fresh = false)
        {
            std::string errors;
            player_ = shared(fresh);
            EngineBuild b;
            std::string warnings;
            const auto file = std::filesystem::path(TEE_SOURCE_DIR) / "content" / "builds" / (build + ".json");
            const bool ok = loadBuild(*player_, file, b, errors, warnings);
            INFO(errors);
            REQUIRE(ok);
            applyBuild(*player_, b);
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

        // Plays `seconds`; calls `each` after every block
        template <typename F>
        void play(double seconds, F each)
        {
            const int blocks = int(seconds * kRate / kBlock + 0.5);
            for (int i = 0; i < blocks; ++i)
            {
                const auto t0 = std::chrono::steady_clock::now();
                player_->process(left_.data(), right_.data(), kBlock);
                busy_ += std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
                played_ += double(kBlock) / kRate;
                each();
            }
        }
        void play(double seconds) { play(seconds, [] {}); }

        // Started with the key, let idle
        void start(double idle = 1.5)
        {
            ctl().ignition = EnginePlayer::kIgnitionStart;
            play(1.0);
            ctl().ignition = EnginePlayer::kIgnitionOn;
            play(idle);
        }

        void write(const std::string& path, double value)
        {
            std::string error;
            const bool ok = player_->instance().write(player_->schema(), path, value, error);
            INFO(error);
            REQUIRE(ok);
        }

        // A warm engine: the coolant at 90 C, the oil at 100 C, the chamber's walls at 450 K (as minutes of running leave
        // them), so its oil's thickness is a warm engine's
        void warm()
        {
            write("heat.tCool", 363.0);
            write("heat.tOil", 373.0);
            write("heat.tSurf", 450.0);
        }

        const std::vector<float>& left() const { return left_; }
        const std::vector<float>& right() const { return right_; }

        // Wall time over audio time so far: the share of one core it took
        double load() const { return busy_ / played_; }

    private:
        EnginePlayer* player_;      // (the kept one; a fresh one is leaked on purpose, as the kept one is)
        std::vector<float> left_ = std::vector<float>(kBlock), right_ = std::vector<float>(kBlock);
        double busy_ = 0.0, played_ = 0.0;
    };
} // namespace tee::test::core
