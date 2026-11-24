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
        std::string engine = "ref";     // --engine ref|tone: the reference simulator, or the stand-in tone
        std::string sim;                // --sim DIR: the folder that holds ref/program.fm (default: beside the executable)
        std::string preset;             // --preset NAME|FILE
        std::vector<std::pair<std::string, double>> sets;   // --set id=value, in order, after the preset
        bool checks = false;            // --checks: the loop budget and the bounds checks
        int frames = -1;                // --frames N: run N frames and exit
        std::string shot;               // --shot FILE.png: the last frame's picture
        double seconds = -1.0;          // --seconds S: render S seconds with no device and exit
        std::string wav;                // --wav FILE.wav: what was rendered
        std::string script;             // --script FILE: inputs at times (app/Script.h)
        int rate = 48000;               // --rate N: the rate of a render with no device
        int block = 256;                // --block N: the block size of a render with no device
        int buffer = 256;               // --buffer N: the buffer asked of the audio device, in frames
        int theme = 0;                  // --theme 0|1
        bool themeGiven = false;        // the window follows the saved preference without it
        double dpi = 1.0;               // --dpi X: a headless picture's pixels a logical px
        std::string panel;              // --panel plain: the shell's own plain Panel, not the interface package's
        bool noAudio = false;           // --no-audio: the window with no device (nothing plays)
        bool keepGoing = false;         // --keep-going: a program that does not compile is no reason to stop a render
    };

    // Reads argv. False, and why, for an option it does not know or a value that is none.
    bool parseOptions(int argc, char** argv, Options& out, std::string& why);

    // The options, a line each, for --help.
    std::string usage(const char* program);
}
