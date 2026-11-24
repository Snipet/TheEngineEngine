// tests/sim/ecu/Sim.h: card H's integrated tests' way into the whole simulator (sim/engine.fm through
// tee::EnginePlayer): a build, Spec fields by path (an array's element too), the Controls, telemetry by name, and
// time played in blocks of 512 at 48 kHz (namespace tee::test::h).
#pragma once

#include "sim/Twin.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdlib>
#include <memory>
#include <string>
#include <vector>

namespace tee::test::h
{
    class Sim
    {
    public:
        // One compile a process (docs/plan/01-rules.md section 3): the first Sim compiles sim/ and keeps that player for
        // good (never deleted: a static owning a compiled program is destroyed after the JIT at exit on macOS); each
        // later Sim resets it (every Spec and Controls field at its default, Engine::init again: core's "a kept player
        // reset is a fresh player, to the bit" proves it). A Sim made while another is alive gets a fresh player of its
        // own. TEE_SIM_FRESH=1 gives every Sim a fresh one (to tell a reset's fault from the model's).
        explicit Sim(const std::string& build = "v8")
        {
            static EnginePlayer* kept = nullptr;
            static bool fresh = std::getenv("TEE_SIM_FRESH") != nullptr;
            if (!fresh && !keptInUse())
            {
                if (kept == nullptr)
                    kept = enginePlayer(48000.0).release();
                else
                    kept->reset(48000.0);
                player = kept;
                keptInUse() = true;
                usesKept_ = true;
            }
            else
            {
                owned_ = enginePlayer(48000.0);
                player = owned_.get();
            }
            applyBuild(*player, engineBuild(*player, build));
        }
        Sim(Sim&& other) noexcept
            : player(other.player), left(std::move(other.left)), right(std::move(other.right)), peak(other.peak),
              finite(other.finite), owned_(std::move(other.owned_)), usesKept_(other.usesKept_)
        {
            other.player = nullptr;
            other.usesKept_ = false;
        }
        Sim(const Sim&) = delete;
        Sim& operator=(const Sim&) = delete;
        Sim& operator=(Sim&&) = delete;
        ~Sim()
        {
            if (usesKept_)
                keptInUse() = false;
        }

        // Another build's fields over these (a car's part over an engine's: "cars/rwd-v8-coupe")
        void over(const std::string& build) { applyBuild(*player, engineBuild(*player, build)); }

        void set(const std::string& path, double value, std::size_t element = 0)
        {
            const int i = player->specIndex(path);
            INFO(path);
            REQUIRE(i >= 0);
            player->setSpec(static_cast<std::size_t>(i), value, element);
        }

        double tel(const std::string& name) const
        {
            const int i = player->telemetryIndex(name);
            INFO(name);
            REQUIRE(i >= 0);
            return player->telemetry(static_cast<std::size_t>(i));
        }

        double read(const std::string& path) const
        {
            double v = 0.0;
            std::string error;
            INFO(path << ": " << error);
            REQUIRE(player->read(path, v, error));
            return v;
        }

        EnginePlayer::Controls& ctl() { return player->controls(); }

        // `seconds` played; peak is the largest sample, finite whether every sample was
        void run(double seconds)
        {
            const int blocks = static_cast<int>(seconds * 48000.0 / 512.0 + 0.5);
            for (int b = 0; b < blocks; ++b)
            {
                player->process(left.data(), right.data(), 512);
                for (int i = 0; i < 512; ++i)
                {
                    finite = finite && std::isfinite(left[i]) && std::isfinite(right[i]);
                    peak = std::fmax(peak, std::fmax(std::fabs(left[i]), std::fabs(right[i])));
                }
            }
        }

        // The mean of a telemetry value over `seconds`, a reading a block
        double mean(const std::string& name, double seconds)
        {
            const int blocks = static_cast<int>(seconds * 48000.0 / 512.0 + 0.5);
            double sum = 0.0;
            for (int b = 0; b < blocks; ++b)
            {
                run(512.0 / 48000.0);
                sum += tel(name);
            }
            return sum / blocks;
        }

        // On the bench, started and running
        void start()
        {
            ctl().mode = EnginePlayer::kModeBench;
            ctl().ignition = EnginePlayer::kIgnitionStart;
            run(1.5);
            ctl().ignition = EnginePlayer::kIgnitionOn;
            run(0.5);
        }

        // A map made flat at `value` (its formula off)
        void flatMap(const std::string& map, double value)
        {
            set("ecu.maps." + map + ".nx", 1);
            set("ecu.maps." + map + ".ny", 1);
            set("ecu.maps." + map + ".z", value, 0);
            set("ecu.maps." + map + "Auto", 0);
        }

        EnginePlayer* player = nullptr;   // the kept player or owned_
        std::vector<float> left = std::vector<float>(512), right = std::vector<float>(512);
        double peak = 0.0;
        bool finite = true;

    private:
        static bool& keptInUse()
        {
            static bool inUse = false;
            return inUse;
        }
        std::unique_ptr<EnginePlayer> owned_;
        bool usesKept_ = false;
    };
} // namespace tee::test::h
