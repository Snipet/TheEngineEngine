// app/Headless.cpp: tee-headless, the application with no window and no audio device (app/Offline.h). This is what
// runs in a cloud container and in ctest:
//
//   tee-headless --script tests/shell/scripts/start-rev-stop.txt --wav out/ref.wav --shot out/ref.png
//
// With no script it runs the start key, a rev and a switch-off over --seconds (10 when not given).
#include "Offline.h"
#include "Options.h"

#include <csignal>
#include <cstdio>
#include <string>

int main(int argc, char** argv)
{
#if !defined(_WIN32)
    std::signal(SIGPIPE, SIG_IGN);   // a peer gone (Claude Desktop's bridge) is an error, never the app's end
#endif
    tee::app::Options options;
    std::string why;
    if (!tee::app::parseOptions(argc, argv, options, why))
    {
        std::fprintf(stderr, "tee-headless: %s\n", why.c_str());
        return 2;
    }
    if (options.help)
    {
        std::printf("%s", tee::app::usage("tee-headless").c_str());
        return 0;
    }
    if (options.version || options.info)
    {
        tee::app::printVersions();
        if (options.info)
            std::printf("renderer: none (FunkGui's software rasteriser)\naudio: none (rendering offline)\n");
        return 0;
    }
    return tee::app::runOffline(options);
}
