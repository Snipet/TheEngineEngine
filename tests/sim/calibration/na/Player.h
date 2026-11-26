// tests/sim/calibration/na/Player.h: card N1's fast tier on one compiled simulator. A process compiles the program
// once (some 6 s of a fast case's 8, the steward's budget: calibration-na's fast tier at 115 test-s against its 80 on
// CI's runner) and each case resets it (EnginePlayer::reset: the instance zeroed, the Spec and the Controls at their
// defaults, init run at the rate: as create() left it, without compiling again). The fast cases are sections of one
// case (FastTest.cpp), each under its old name, so one process holds them.
#pragma once

#include "tee/EnginePlayer.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>

namespace n1
{
    // The process's player, reset at `rate` (never destroyed: a compiled program destroyed after the JIT at exit
    // crashes on macOS)
    inline tee::EnginePlayer& player(double rate)
    {
        static tee::EnginePlayer* kept = nullptr;
        if (kept == nullptr)
        {
            tee::EnginePlayer::Options options;
            options.directory = std::filesystem::path(TEE_SOURCE_DIR) / "sim";
            options.sampleRate = rate;
            std::string errors;
            kept = tee::EnginePlayer::create(options, errors).release();
            INFO(errors);
            REQUIRE(kept != nullptr);
        }
        else
            kept->reset(rate);
        return *kept;
    }

    // The fast tier's cases, sections of FastTest.cpp's one case
    void freshAirIsThrottleAir();          // BalanceTest.cpp
    void itbManifoldAbove5kPa();           // ItbTest.cpp
    void dynoStartTractorAndTurbo();       // StartTest.cpp
    void dynoStartCommonRail();            // StartTest.cpp
    void tinyChargePathsHold();            // BoostInletTest.cpp
} // namespace n1
