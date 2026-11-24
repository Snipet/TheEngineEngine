// app/Launch.h: which build the window opens with at launch (TheEngineEngine; tee-headless opens only a --preset).
//
//   --preset NAME|FILE     that build (app/Engine.cpp applies it, before the window): nothing here
//   --reference            none: the simulator's defaults, every model switch at Reference (for comparisons)
//   the remembered build   the settings' lastBuild, where that file is (the build open when the window last closed)
//   the first build        content/builds/first.json, a whole engine with every model on (--first-build FILE: a
//                          test's), where it is; else none, as --reference
//
// A machine's run (--frames, --shot) opens none of these by default, as --reference: what CI's window runs draw and
// time does not change with a content file. It opens the first build only when named (--first-build FILE, or --preset
// first), and the remembered build only when its settings were chosen for it (TEE_SETTINGS_DIR).
#pragma once

#include "Options.h"

#include <filesystem>
#include <string>

namespace tee::files
{
    class BuildFile;
}

namespace tee::player
{
    class SimAdapter;
}

namespace tee::app
{
    struct LaunchBuild
    {
        std::filesystem::path file;     // empty: no build is opened
        const char* what = "";          // what it is, for the console: "the last build", "the first build", or why none
        bool first = false;             // the first build (a first launch: its notes' first line is shown)
    };

    // `content` is where content/ is (findContent()); empty: there is none.
    LaunchBuild chooseLaunchBuild(const Options& o, bool machine, const std::string& lastBuild,
                                  const std::filesystem::path& content);
    LaunchBuild chooseLaunchBuild(const Options& o, bool machine, const std::string& lastBuild);

    // One line of what a window ended with, for a test to read: the build's name as the top bar shows it, its file, and
    // each model switch, as the document holds it and as the engine was sent it ("sent" once the engine is ready):
    //   build: "Economy four" file=.../economy-four-15.json unsaved=no models: valve=1/1 intake=1/1 ...
    std::string launchReadBack(files::BuildFile& document, player::SimAdapter* sim);

    // A Spec field ("valve.stages", "spec." optional) as the host was sent it; false while the engine is not ready or
    // for a path the program does not have.
    bool readSent(player::SimAdapter& sim, const std::string& path, double& out);
}
