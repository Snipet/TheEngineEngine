// tests/support/Ref.h: the reference, its presets and its scenarios, as the tests reach them (namespace tee::test).
#pragma once

#include "tee/Render.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <memory>
#include <string>

namespace tee::test
{
    inline std::unique_ptr<RefPlayer> refPlayer(double rate = 48000.0, bool checks = false, int optimisation = 2)
    {
        RefPlayer::Options options;
        options.directory = TEE_SOURCE_SIM_DIR;
        options.sampleRate = rate;
        options.checks = checks;
        options.optimisation = optimisation;
        std::string errors;
        std::unique_ptr<RefPlayer> player = RefPlayer::create(options, errors);
        INFO(errors);
        REQUIRE(player != nullptr);
        return player;
    }

    inline Preset preset(const std::string& name)
    {
        Preset out;
        std::string error;
        const bool ok = loadPreset(std::filesystem::path(TEE_SOURCE_CONTENT_DIR) / "presets" / (name + ".json"), out, error);
        INFO(error);
        REQUIRE(ok);
        return out;
    }

    inline Scenario scenario(const std::string& name)
    {
        Scenario out;
        std::string error;
        const bool ok =
            loadScenario(std::filesystem::path(TEE_SOURCE_CONTENT_DIR) / "scenarios" / (name + ".json"), out, error);
        INFO(error);
        REQUIRE(ok);
        return out;
    }

    // A render that must succeed and must not have been stopped by a check.
    inline RenderResult rendered(RefPlayer& player, const Preset& p, const Scenario& s, int block = 512, double seconds = 0.0)
    {
        RenderOptions options;
        options.blockSize = block;
        options.seconds = seconds;
        RenderResult result;
        std::string error;
        const bool ok = render(player, p, s, options, result, error);
        INFO(error);
        REQUIRE(ok);
        INFO(result.stopReason);
        REQUIRE_FALSE(result.stopped);
        return result;
    }

    // The engine's state in a row of the panel's lamps: 0 off, 1 cranking, 2 running, 3 running down.
    inline int stateOf(float lamps) { return static_cast<int>(lamps) & 3; }
    inline bool limiterOf(float lamps) { return (static_cast<int>(lamps) & 4) != 0; }
} // namespace tee::test
