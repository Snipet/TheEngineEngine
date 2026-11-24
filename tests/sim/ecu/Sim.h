// tests/sim/ecu/Sim.h: card H's integrated tests' way into the whole simulator (sim/engine.fm through
// tee::EnginePlayer): a build, Spec fields by path (an array's element too), the Controls, telemetry by name, and
// time played in blocks of 512 at 48 kHz (namespace tee::test::h).
#pragma once

#include "sim/Twin.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <string>
#include <vector>

namespace tee::test::h
{
    class Sim
    {
    public:
        explicit Sim(const std::string& build = "v8")
        {
            player = enginePlayer(48000.0);
            applyBuild(*player, engineBuild(*player, build));
        }

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

        std::unique_ptr<EnginePlayer> player;
        std::vector<float> left = std::vector<float>(512), right = std::vector<float>(512);
        double peak = 0.0;
        bool finite = true;
    };
} // namespace tee::test::h
