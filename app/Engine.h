// app/Engine.h: whatever plays, found and made ready: the reference simulator compiled from sim/ref beside the
// executable, or the stand-in tone. When the program does not compile there is still a Player (a silent one), so the
// window stays up, and `error` holds the compiler's words.
#pragma once

#include "Options.h"

#include "tee/app/Sources.h"

#include <filesystem>
#include <memory>
#include <string>

namespace tee::player
{
    class RefAdapter;
    class SimAdapter;
}

namespace tee::app
{
    struct Engine
    {
        std::unique_ptr<Player> player;         // never null
        player::RefAdapter* ref = nullptr;      // the player, when it is the reference
        player::SimAdapter* sim = nullptr;      // the player, when it is the simulator (sim/engine.fm)
        std::string name;                       // "the reference engine (sim/ref)", for the status line
        std::string error;                      // not empty: it did not start; FMinus's messages as clang words them
        std::string warnings;                   // the program's, when it compiled with some
        std::string notes;                      // a preset or a --set that named nothing, a line each
        double compileSeconds = 0.0;
        std::filesystem::path simDir;           // where it was found (empty: nowhere)

        // Once a frame, on the UI thread, before the Panel ticks: the player's own housekeeping (the simulator's
        // document sends what it has waiting; its telemetry is the newest block's).
        void frame();
        // Not empty: a check stopped the program while it played (a build with checks); FMinus's words. UI thread.
        std::string stopped() const;
    };

    // The folder that holds the running executable.
    std::filesystem::path executableDir();

    // sim/ for this run: Options::sim, else the environment's TEE_SIM_DIR, else beside the executable (inside a macOS
    // bundle: Contents/Resources). Empty when none of them has ref/program.fm.
    std::filesystem::path findSim(const Options&);

    // content/ the same way (TEE_CONTENT_DIR): where the presets are. Empty when there is none.
    std::filesystem::path findContent();

    // Makes the engine the options ask for, at `rate`: the reference (sim/ref, the default), the simulator
    // (sim/engine.fm: --engine sim) or the tone; and sets its parameters: the application's own choice first (the
    // reference starts in DRIVE CAR: in a car, in neutral, so the pedal is the throttle and it idles until it is
    // switched off), then the preset (the reference's: content/ref/presets; the simulator's: a build of
    // content/builds), then each --set.
    Engine loadEngine(const Options&, double rate);
}
