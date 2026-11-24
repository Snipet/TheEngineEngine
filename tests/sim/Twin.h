// tests/sim/Twin.h: the null test's harness (namespace tee::test): the simulator (sim/engine.fm, through EnginePlayer)
// and the reference (sim/ref, through RefPlayer) played side by side in one process from the same inputs, written once
// in the new contract's terms (a build of content/builds and an engine scenario of tests/sim/scenarios), by
// tee::playTwin (EnginePlayer.h says how the reference is given them); and the files the tests read.
#pragma once

#include "support/Audio.h"
#include "support/Ref.h"

#include "tee/EnginePlayer.h"
#include "tee/RefPlayer.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace tee::test
{
    inline std::string simDir() { return std::string(TEE_SOURCE_DIR) + "/sim"; }
    inline std::string buildFile(const std::string& name) { return std::string(TEE_SOURCE_DIR) + "/content/builds/" + name + ".json"; }
    inline std::string engineScenarioFile(const std::string& name)
    {
        return std::string(TEE_SOURCE_DIR) + "/tests/sim/scenarios/" + name + ".json";
    }

    inline std::unique_ptr<EnginePlayer> enginePlayer(double rate = 48000.0, bool checks = false, int optimisation = 2)
    {
        EnginePlayer::Options options;
        options.directory = simDir();
        options.sampleRate = rate;
        options.checks = checks;
        options.optimisation = optimisation;
        std::string errors;
        auto player = EnginePlayer::create(options, errors);
        if (!player)
            throw std::runtime_error("sim/engine.fm does not compile:\n" + errors);
        return player;
    }

    inline EngineBuild engineBuild(const EnginePlayer& player, const std::string& name)
    {
        EngineBuild build;
        std::string error, warnings;
        if (!loadBuild(player, buildFile(name), build, error, warnings))
            throw std::runtime_error(error);
        return build;
    }

    inline EngineScenario engineScenario(const std::string& name)
    {
        EngineScenario scenario;
        std::string error;
        if (!loadEngineScenario(engineScenarioFile(name), scenario, error))
            throw std::runtime_error(error);
        return scenario;
    }

    // The reference's dials and what of Shared's Set each is read into (program.fm's Voice::readDials)
    struct DialOfSet
    {
        const char* dial;
        const char* set;
    };
    inline constexpr DialOfSet kDialsOfSet[] = {
        {"engine", "layout"}, {"size", "size"}, {"idle", "idle"}, {"redline", "redline"}, {"cam", "cam"},
        {"inertia", "inertia"}, {"load", "load"}, {"turbo", "turbo"}, {"bov", "bov"}, {"headers", "headers"},
        {"pipe", "pipe"}, {"xpipe", "xpipe"}, {"muffler", "muffler"}, {"pops", "pops"}, {"rough", "rough"},
        {"view", "view"}, {"distance", "distance"}, {"intake", "intake"}, {"mech", "mech"}, {"room", "room"},
        {"level", "level"}, {"timing", "timing"}, {"turbosize", "tsize"}, {"starter", "starter"}, {"hold", "hold"},
        {"mass", "mass"}, {"aspiration", "asp"}, {"fuel", "fuel"}, {"compression", "comp"}, {"borestroke", "bs"},
        {"ambient", "ambF"}};

    inline double field(const EnginePlayer& player, const std::string& path)
    {
        double v = 0.0;
        std::string error;
        if (!player.read(path, v, error))
            throw std::runtime_error(error);
        return v;
    }

    struct Null
    {
        TwinResult twin;
        double depthLeft = 0.0, depthRight = 0.0;   // nullDepth: the reference over the difference, dB
        double worst = 0.0;                         // the largest difference of a sample
        double peak = 0.0;
    };

    // Plays `scenario` from `build` through both (tee::playTwin), in blocks of `block`, and measures the difference.
    inline Null twin(EnginePlayer& engine, RefPlayer& ref, const EngineBuild& build, const EngineScenario& scenario, int block,
                     double seconds = 0.0)
    {
        Null out;
        TwinOptions options;
        options.blockSize = block;
        options.seconds = seconds;
        std::string error;
        if (!playTwin(engine, ref, build, scenario, options, out.twin, error))
            throw std::runtime_error(error);
        const Audio& a = out.twin.reference;
        const Audio& b = out.twin.engine;
        out.depthLeft = nullDepth(a.left, b.left);
        out.depthRight = nullDepth(a.right, b.right);
        out.worst = std::max(maxDifference(a.left, b.left), maxDifference(a.right, b.right));
        out.peak = std::max(peak(a.left), peak(a.right));
        return out;
    }
} // namespace tee::test
