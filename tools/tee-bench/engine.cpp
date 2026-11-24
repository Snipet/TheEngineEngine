// tools/tee-bench/engine.cpp: tee-bench --engine: what the simulator (sim/engine.fm) costs against the reference
// (sim/ref), each build of content/builds on each engine scenario of tests/sim/scenarios, the two played side by side
// from the same inputs (tee::playTwin), each call of each timed. The figure is each side's wall time a sample of the
// engine's own 96 kHz rate over the calls in which the engine sounded, the least of --repeat runs, and their ratio.
#include "common/Paths.h"

#include "tee/EnginePlayer.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

namespace
{
    const char* const kEngineHelp =
        "tee-bench --engine: what the simulator costs against the reference, from the same inputs\n"
        "\n"
        "  tee-bench --engine [options]\n"
        "\n"
        "  --builds <a,b>     these builds (every one of content/builds)\n"
        "  --scenarios <a,b>  these engine scenarios (every one of tests/sim/scenarios)\n"
        "  --repeat <n>       runs of each, the least time of each side kept (3)\n"
        "  --rate <hz>        the output's sample rate (48000)\n"
        "  --block <n>        the longest call of process (512)\n"
        "  --out <file>       write the table there too (it is always printed)\n"
        "  --sim <dir>        the simulator's directory (sim)\n";

    std::vector<std::string> list(const std::string& text)
    {
        std::vector<std::string> out;
        std::stringstream in(text);
        std::string item;
        while (std::getline(in, item, ','))
            if (!item.empty())
                out.push_back(item);
        return out;
    }
} // namespace

int engineBenchMain(int argc, char** argv);

int engineBenchMain(int argc, char** argv)
{
    using namespace tee;
    tools::Arguments args(argc, argv);
    std::string simFlag, outName, value;
    std::vector<std::string> buildNames, scenarioNames;
    double rate = 48000.0;
    int block = 512, repeat = 3;
    while (!args.done())
    {
        if (args.is("--engine"))
            continue;
        if (args.is("--help") || args.is("-h"))
        {
            std::fputs(kEngineHelp, stdout);
            return 0;
        }
        const char* option = args.peek().c_str();
        std::string name = args.take();
        if (!args.value(value))
        {
            std::fprintf(stderr, "tee-bench: %s wants a value (--engine --help lists the options)\n", option);
            return 2;
        }
        if (name == "--builds")
            buildNames = list(value);
        else if (name == "--scenarios")
            scenarioNames = list(value);
        else if (name == "--repeat")
            repeat = std::max(1, std::atoi(value.c_str()));
        else if (name == "--rate")
            rate = std::atof(value.c_str());
        else if (name == "--block")
            block = std::max(1, std::atoi(value.c_str()));
        else if (name == "--out")
            outName = value;
        else if (name == "--sim")
            simFlag = value;
        else
        {
            std::fprintf(stderr, "tee-bench: unknown argument '%s' (--engine --help lists them)\n", name.c_str());
            return 2;
        }
    }
    const std::filesystem::path refSim = tools::findDirectory({}, "TEE_SIM_DIR", argv[0], "sim/ref", TEE_SOURCE_SIM_DIR);
    const std::filesystem::path sim = !simFlag.empty() ? std::filesystem::path(simFlag) : refSim.parent_path();
    const std::filesystem::path content =
        tools::findDirectory({}, "TEE_CONTENT_DIR", argv[0], "content/ref", TEE_SOURCE_CONTENT_DIR).parent_path();
    const std::filesystem::path builds = content / "builds";
    const std::filesystem::path scenarios = content.parent_path() / "tests" / "sim" / "scenarios";
    if (buildNames.empty())
        buildNames = tools::namesIn(builds);
    if (scenarioNames.empty())
        scenarioNames = tools::namesIn(scenarios);

    std::string errors;
    EnginePlayer::Options engineOptions;
    engineOptions.directory = sim;
    engineOptions.sampleRate = rate;
    const auto engine = EnginePlayer::create(engineOptions, errors);
    RefPlayer::Options refOptions;
    refOptions.directory = refSim;
    refOptions.sampleRate = rate;
    const auto ref = engine ? RefPlayer::create(refOptions, errors) : nullptr;
    if (!engine || !ref)
    {
        std::fprintf(stderr, "tee-bench: %s", errors.c_str());
        return 1;
    }

    std::ostringstream table;
    table << "| build | scenario | simulator, ns a 96 kHz sample | reference, ns | simulator / reference |\n"
          << "|---|---|---:|---:|---:|\n";
    double sumEngine = 0.0, sumRef = 0.0;
    for (const std::string& b : buildNames)
    {
        EngineBuild build;
        std::string warnings;
        if (!loadBuild(*engine, tools::namedFile(builds, b), build, errors, warnings))
        {
            std::fprintf(stderr, "tee-bench: %s\n", errors.c_str());
            return 1;
        }
        for (const std::string& s : scenarioNames)
        {
            EngineScenario scenario;
            if (!loadEngineScenario(tools::namedFile(scenarios, s), scenario, errors))
            {
                std::fprintf(stderr, "tee-bench: %s\n", errors.c_str());
                return 1;
            }
            double bestEngine = 0.0, bestRef = 0.0;
            for (int k = 0; k < repeat; ++k)
            {
                TwinOptions options;
                options.blockSize = block;
                options.keepAudio = false;
                TwinResult result;
                if (!playTwin(*engine, *ref, build, scenario, options, result, errors) || result.soundingFrames == 0)
                {
                    std::fprintf(stderr, "tee-bench: %s, %s: %s\n", b.c_str(), s.c_str(),
                                 errors.empty() ? "the engine never ran" : errors.c_str());
                    return 1;
                }
                const double engineSamples = static_cast<double>(result.soundingFrames) * 96000.0 / rate;
                const double e = 1.0e9 * result.engineSeconds / engineSamples;
                const double r = 1.0e9 * result.referenceSeconds / engineSamples;
                bestEngine = k == 0 ? e : std::min(bestEngine, e);
                bestRef = k == 0 ? r : std::min(bestRef, r);
            }
            sumEngine += bestEngine;
            sumRef += bestRef;
            char line[256];
            std::snprintf(line, sizeof line, "| %s | %s | %.1f | %.1f | %.3f |\n", b.c_str(), s.c_str(), bestEngine,
                          bestRef, bestEngine / bestRef);
            table << line;
            std::fputs(line, stdout);
            std::fflush(stdout);
        }
    }
    char total[160];
    std::snprintf(total, sizeof total, "| all | | %.1f | %.1f | %.3f |\n", sumEngine, sumRef, sumEngine / sumRef);
    table << total;
    std::fputs(total, stdout);
    if (!outName.empty())
    {
        std::ofstream out(outName);
        out << table.str();
    }
    return 0;
}
