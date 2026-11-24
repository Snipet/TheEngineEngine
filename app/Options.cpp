// app/Options.cpp: Options.h.
#include "Options.h"

#include <cstdlib>
#include <string_view>

namespace tee::app
{
    namespace
    {
        bool number(const std::string& text, double& out)
        {
            if (text.empty())
                return false;
            char* end = nullptr;
            out = std::strtod(text.c_str(), &end);
            return end != nullptr && *end == '\0' && out == out;
        }
    }

    bool parseOptions(int argc, char** argv, Options& o, std::string& why)
    {
        for (int i = 1; i < argc; ++i)
        {
            const std::string_view arg = argv[i];
            // an option's value: the next argument
            const auto value = [&](std::string& out) {
                if (i + 1 >= argc)
                {
                    why = std::string(arg) + " wants a value";
                    return false;
                }
                out = argv[++i];
                return true;
            };
            const auto real = [&](double& out, double least, double most) {
                std::string text;
                if (!value(text))
                    return false;
                if (!number(text, out) || out < least || out > most)
                {
                    why = std::string(arg) + ": '" + text + "' is no value for it";
                    return false;
                }
                return true;
            };
            const auto whole = [&](int& out, int least, int most) {
                double v = 0.0;
                if (!real(v, least, most))
                    return false;
                out = static_cast<int>(v);
                return true;
            };

            if (arg == "--help" || arg == "-h")
                o.help = true;
            else if (arg == "--version")
                o.version = true;
            else if (arg == "--info")
                o.info = true;
            else if (arg == "--checks")
                o.checks = true;
            else if (arg == "--no-audio")
                o.noAudio = true;
            else if (arg == "--keep-going")
                o.keepGoing = true;
            else if (arg == "--engine")
            {
                if (!value(o.engine))
                    return false;
                if (o.engine != "ref" && o.engine != "sim" && o.engine != "tone" && o.engine != "auto")
                {
                    why = "--engine is sim, ref or tone";
                    return false;
                }
            }
            else if (arg == "--ref-panel")
                o.refPanel = true;
            else if (arg == "--workspace")
            {
                if (!value(o.workspace))
                    return false;
            }
            else if (arg == "--panel")
            {
                if (!value(o.panel))
                    return false;
                if (o.panel != "plain" && o.panel != "main")
                {
                    why = "--panel is plain or main";
                    return false;
                }
            }
            else if (arg == "--sim")
            {
                if (!value(o.sim))
                    return false;
            }
            else if (arg == "--preset")
            {
                if (!value(o.preset))
                    return false;
            }
            else if (arg == "--set")
            {
                std::string text;
                if (!value(text))
                    return false;
                const std::size_t eq = text.find('=');
                double v = 0.0;
                if (eq == std::string::npos || eq == 0 || !number(text.substr(eq + 1), v))
                {
                    why = "--set wants id=value, as --set size=6.2";
                    return false;
                }
                o.sets.emplace_back(text.substr(0, eq), v);
            }
            else if (arg == "--frames")
            {
                if (!whole(o.frames, 0, 10000000))
                    return false;
            }
            else if (arg == "--shot")
            {
                if (!value(o.shot))
                    return false;
            }
            else if (arg == "--seconds")
            {
                if (!real(o.seconds, 0.0, 86400.0))
                    return false;
            }
            else if (arg == "--wav")
            {
                if (!value(o.wav))
                    return false;
            }
            else if (arg == "--record")
            {
                if (!value(o.record))
                    return false;
            }
            else if (arg == "--script")
            {
                if (!value(o.script))
                    return false;
            }
            else if (arg == "--rate")
            {
                if (!whole(o.rate, 8000, 384000))
                    return false;
            }
            else if (arg == "--block")
            {
                if (!whole(o.block, 1, 8192))
                    return false;
            }
            else if (arg == "--buffer")
            {
                if (!whole(o.buffer, 16, 8192))
                    return false;
                o.bufferGiven = true;
            }
            else if (arg == "--theme")
            {
                if (!whole(o.theme, 0, 1))
                    return false;
                o.themeGiven = true;
            }
            else if (arg == "--dpi" || arg == "--scale")
            {
                if (!real(o.dpi, 0.5, 4.0))
                    return false;
            }
            else if (arg == "--size")
            {
                std::string text;
                if (!value(text))
                    return false;
                const std::size_t x = text.find_first_of("xX");
                double w = 0.0, h = 0.0;
                if (x == std::string::npos || !number(text.substr(0, x), w) || !number(text.substr(x + 1), h) || w < 320
                    || h < 200 || w > 8192 || h > 8192)
                {
                    why = "--size: '" + text + "' is no size (WxH in logical px, as 1280x800)";
                    return false;
                }
                o.width = static_cast<int>(w);
                o.height = static_cast<int>(h);
            }
            else if (arg == "--dyno-wait")
            {
                if (!real(o.dynoWait, 0.0, 3600.0))
                    return false;
            }
            else
            {
                why = "no option '" + std::string(arg) + "' (--help lists them)";
                return false;
            }
        }
        if (o.refPanel)
            o.engine = "ref";       // the reference's panel shows the reference
        return true;
    }

    std::string usage(const char* program)
    {
        std::string u = "usage: ";
        u += program;
        u += " [options]\n"
             "  --info                 print the versions, the renderer and the audio device, and exit\n"
             "  --engine sim|ref|tone  the simulator (sim/engine.fm: the default) in the application's frame, the\n"
             "                         reference (sim/ref) in its own panel, or a plain tone that needs no compiler\n"
             "  --ref-panel            the reference engine in its own panel (as --engine ref)\n"
             "  --workspace ID[:PAGE]  the frame opens on that workspace, by its name on the rail (and its page, from 1)\n"
             "  --sim DIR              the folder that holds engine.fm and ref/ (default: sim/ beside the executable)\n"
             "  --preset NAME|FILE     a preset of content/ref/presets (the simulator: a build of content/builds),\n"
             "                         by its file's name, or a file\n"
             "  --set ID=VALUE         a parameter, after the preset; may be given many times\n"
             "  --checks               compile with the loop budget and the bounds checks\n"
             "  --frames N             run N frames, then exit\n"
             "  --shot FILE.png        write the last frame's picture\n"
             "  --seconds S            render S seconds with no audio device, then exit\n"
             "  --wav FILE.wav         write what was rendered (with --seconds, or --frames in tee-headless)\n"
             "  --record FILE.wav      the window's output written to a file while it plays\n"
             "  --script FILE          inputs at times: keys, the pointer, parameters, pictures, checks\n"
             "  --rate N, --block N    the rate and block size of a render with no device (48000, 256)\n"
             "  --buffer N             the buffer asked of the audio device, in frames (the settings file's; 256)\n"
             "  --no-audio             open no audio device\n"
             "  --panel plain          the plain Panel (numbers and dials), whatever interface the build has\n"
             "  --theme 0|1            the theme (graphite, paper)\n"
             "  --scale S (--dpi S)    a headless picture's pixels a logical px (1, 1.5, 2)\n"
             "  --size WxH             the window's or the picture's logical size (the frame's smallest: 1100x700)\n"
             "  --dyno-wait S          tee-headless: wait up to S seconds for the dyno's first curve before the first\n"
             "                         frame, so a picture shows it\n"
             "  --keep-going           with --seconds or --frames: go on when the program does not compile\n"
             "  --version, --help\n";
        return u;
    }
}
