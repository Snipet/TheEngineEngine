// app/Options.h: the application's command line, the same for the window (TheEngineEngine) and for tee-headless.
#pragma once

#include <string>
#include <utility>
#include <vector>

namespace tee::app
{
    struct Options
    {
        bool help = false;
        bool version = false;
        bool info = false;              // --info: print the versions, the renderer and the audio device, and exit
        std::string engine = "auto";    // --engine sim|ref|tone: the simulator (sim/engine.fm), the reference (sim/ref)
                                        // or the stand-in tone; auto: the simulator where its source is, else the
                                        // reference
        std::string sim;                // --sim DIR: the folder that holds ref/program.fm (default: beside the executable)
        std::string preset;             // --preset NAME|FILE
        std::vector<std::pair<std::string, double>> sets;   // --set id=value, in order, after the preset
        bool checks = false;            // --checks: the loop budget and the bounds checks
        int frames = -1;                // --frames N: run N frames and exit
        std::string shot;               // --shot FILE.png: the last frame's picture
        double seconds = -1.0;          // --seconds S: render S seconds with no device and exit
        std::string wav;                // --wav FILE.wav: what was rendered
        std::string record;             // --record FILE.wav: the window's output, written while it plays
        std::string script;             // --script FILE: inputs at times (app/Script.h)
        int rate = 48000;               // --rate N: the rate of a render with no device
        int block = 256;                // --block N: the block size of a render with no device
        int buffer = 256;               // --buffer N: the buffer asked of the audio device, in frames
        bool bufferGiven = false;       // without it, the settings file's
        int theme = 0;                  // --theme 0|1
        bool themeGiven = false;        // the window follows the saved preference without it
        double dpi = 1.0;               // --scale S (or --dpi S): a headless picture's pixels a logical px
        int width = 0, height = 0;      // --size WxH: the logical size (0: the Panel's own)
        double dynoWait = 0.0;          // --dyno-wait S: tee-headless waits for the dyno's first curve
        std::string panel;              // --panel plain: the shell's own plain Panel, not the interface package's
        bool refPanel = false;          // --ref-panel: the reference engine in its own panel (as --engine ref)
        std::string workspace;          // --workspace ID[:PAGE]: the frame opens on that workspace (its name on the rail)
        bool noAudio = false;           // --no-audio: the window with no device (nothing plays)
        bool keepGoing = false;
        std::string quality;            // --quality full|economy: the simulator's inner rate (empty: the settings')
        bool expectFault = false;
        double holdCompile = 0.0;       // --hold-compile S (tee-headless, a test's): the engine's compile is held back for
                                        // S seconds of the run (its frames drawn meanwhile), then waited for
        bool background = false;        // not an option: the window compiles the engine in the background       // --expect-fault: a run where the engine diverges and is restarted is not a failure         // --keep-going: a program that does not compile is no reason to stop a render
        // not from the command line: the units the settings file remembers, for the frame ("quantity=unit" lines)
        std::string unitChoices;
    };

    // Reads argv. False, and why, for an option it does not know or a value that is none.
    bool parseOptions(int argc, char** argv, Options& out, std::string& why);

    // The options, a line each, for --help.
    std::string usage(const char* program);
}
