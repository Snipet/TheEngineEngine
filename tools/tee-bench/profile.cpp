// tools/tee-bench/profile.cpp: tee-bench --profile: where the time goes, by module and by function. One render of the
// reference (a preset and a scenario) or, with --engine, of the simulator (a build and an engine scenario), while a
// sampler interrupts the rendering thread four thousand times a second (common/Sampler.h); each sample is looked up in
// the program's code ranges, which FMinus gives by function, with each function's module.
//
// What the table can and cannot say: a function the optimiser inlined has no code of its own, so its samples are
// its caller's, and its module's time shows under the caller's module. The "inlined into it" column lists what each
// function swallowed; a module that reads small here may be large inside another's row.
#include "common/Paths.h"
#include "common/Sampler.h"

#include "tee/Clock.h"
#include "tee/EnginePlayer.h"
#include "tee/Render.h"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <string>

namespace
{
    const char* const kProfileHelp =
        "tee-bench --profile: where the time goes, by module and by function (Linux and macOS)\n"
        "\n"
        "  tee-bench --profile [--preset v8] [--scenario free-revs]             the reference (sim/ref)\n"
        "  tee-bench --profile --engine [--build default] [--scenario free-revs]  the simulator (sim/engine.fm)\n"
        "\n"
        "  --seconds <s>      render this long instead of the scenario's length\n"
        "  --rate <hz>        the output's sample rate (48000)\n"
        "  --block <n>        the longest call of process (512)\n"
        "  --all              every function sampled, not only those above half a percent\n"
        "  --out <file>       write the tables there too (they are always printed)\n"
        "  --sim <dir>        the program's directory (sim/ref; with --engine, sim)\n";
} // namespace

int profileMain(int argc, char** argv);

int profileMain(int argc, char** argv)
{
    using namespace tee;
    tools::Arguments args(argc, argv);
    std::string simFlag, outName, presetName = "v8", buildName = "default", scenarioName = "free-revs", value;
    double rate = 48000.0, seconds = 0.0;
    int block = 512;
    bool engine = false, all = false;
    while (!args.done())
    {
        if (args.is("--profile"))
            continue;
        if (args.is("--engine"))
        {
            engine = true;
            continue;
        }
        if (args.is("--all"))
        {
            all = true;
            continue;
        }
        if (args.is("--help") || args.is("-h"))
        {
            std::fputs(kProfileHelp, stdout);
            return 0;
        }
        const std::string name = args.take();
        if (!args.value(value))
        {
            std::fprintf(stderr, "tee-bench: %s wants a value (--profile --help lists the options)\n", name.c_str());
            return 2;
        }
        if (name == "--preset")
            presetName = value;
        else if (name == "--build")
            buildName = value;
        else if (name == "--scenario")
            scenarioName = value;
        else if (name == "--seconds")
            seconds = std::atof(value.c_str());
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
            std::fprintf(stderr, "tee-bench: unknown argument '%s' (--profile --help lists them)\n", name.c_str());
            return 2;
        }
    }
    if (!tools::Sampler::available())
    {
        std::fprintf(stderr, "tee-bench: --profile samples with POSIX signals: Linux and macOS only\n");
        return 1;
    }

    const std::filesystem::path refSim = tools::findDirectory({}, "TEE_SIM_DIR", argv[0], "sim/ref", TEE_SOURCE_SIM_DIR);
    const std::filesystem::path refContent =
        tools::findDirectory({}, "TEE_CONTENT_DIR", argv[0], "content/ref", TEE_SOURCE_CONTENT_DIR);
    RenderOptions options;
    options.blockSize = block;
    options.seconds = seconds;
    options.keepAudio = false;
    RenderResult result;
    std::string errors, what;
    std::vector<fminus::Program::CodeRange> ranges;
    std::vector<std::uintptr_t> samples;
    tools::Sampler sampler;
    double cpuSeconds = 0.0;   // the rendering thread's processor time over the render, the sampler's handler included

    // The players outlive the look-up below: the code ranges are their programs'.
    std::unique_ptr<RefPlayer> ref;
    std::unique_ptr<EnginePlayer> sim;
    if (!engine)
    {
        RefPlayer::Options playerOptions;
        playerOptions.directory = simFlag.empty() ? refSim : std::filesystem::path(simFlag);
        playerOptions.sampleRate = rate;
        ref = RefPlayer::create(playerOptions, errors);
        Preset preset;
        Scenario scenario;
        if (!ref || !loadPreset(tools::namedFile(refContent / "presets", presetName), preset, errors)
            || !loadScenario(tools::namedFile(refContent / "scenarios", scenarioName), scenario, errors))
        {
            std::fprintf(stderr, "tee-bench: %s\n", errors.c_str());
            return 1;
        }
        ranges = ref->program().fminus().codeRanges(errors);
        sampler.start();
        const double began = threadCpuSeconds();
        const bool ok = render(*ref, preset, scenario, options, result, errors);
        cpuSeconds = threadCpuSeconds() - began;
        samples = sampler.stop();
        if (!ok)
        {
            std::fprintf(stderr, "tee-bench: %s\n", errors.c_str());
            return 1;
        }
        what = "the reference (sim/ref), preset " + presetName + ", scenario " + scenarioName;
    }
    else
    {
        const std::filesystem::path content = refContent.parent_path();
        EnginePlayer::Options playerOptions;
        playerOptions.directory = simFlag.empty() ? refSim.parent_path() : std::filesystem::path(simFlag);
        playerOptions.sampleRate = rate;
        sim = EnginePlayer::create(playerOptions, errors);
        EngineBuild build;
        EngineScenario scenario;
        std::string warnings;
        if (!sim || !loadBuild(*sim, tools::namedFile(content / "builds", buildName), build, errors, warnings)
            || !loadEngineScenario(tools::namedFile(content.parent_path() / "tests" / "sim" / "scenarios", scenarioName),
                                   scenario, errors))
        {
            std::fprintf(stderr, "tee-bench: %s\n", errors.c_str());
            return 1;
        }
        ranges = sim->program().fminus().codeRanges(errors);
        sampler.start();
        const double began = threadCpuSeconds();
        const bool ok = renderEngine(*sim, build, scenario, options, result, errors);
        cpuSeconds = threadCpuSeconds() - began;
        samples = sampler.stop();
        if (!ok)
        {
            std::fprintf(stderr, "tee-bench: %s\n", errors.c_str());
            return 1;
        }
        what = "the simulator (sim/engine.fm), build " + buildName + ", scenario " + scenarioName;
    }
    if (ranges.empty() || samples.empty() || result.soundingFrames == 0)
    {
        std::fprintf(stderr, "tee-bench: nothing to show: %zu code ranges, %zu samples, %lld samples of sound\n",
                     ranges.size(), samples.size(), static_cast<long long>(result.soundingFrames));
        return 1;
    }

    // Each sample to its range: by function, and by the function's module.
    struct Row
    {
        std::size_t samples = 0;
        std::string module;
        std::string inlined;
    };
    std::map<std::string, Row> byFunction;
    std::map<std::string, std::size_t> byModule;
    std::size_t inside = 0;
    for (const std::uintptr_t pc : samples)
    {
        const fminus::Program::CodeRange* range = fminus::Program::codeRangeAt(ranges, pc);
        if (range == nullptr)
            continue;
        ++inside;
        const std::string name = range->function.empty() ? range->symbol : range->function;
        Row& row = byFunction[name];
        if (row.samples == 0)
        {
            row.module = range->module;
            for (const std::string& callee : range->inlined)
                row.inlined += (row.inlined.empty() ? "" : ", ") + callee;
        }
        ++row.samples;
        ++byModule[range->module.empty() ? std::string("(no module: a function, or FMinus's own)") : range->module];
    }

    // The render's own cost, to turn a share into nanoseconds: the whole render's processor time (the calls in which
    // nothing sounded cost next to nothing) over the samples in which the engine ran. The sampler's interruptions are
    // in it, so it reads a little above tee-bench's plain figure.
    const double engineSamples = static_cast<double>(result.soundingFrames) * 96000.0 / rate;
    const double ns = 1.0e9 * cpuSeconds / engineSamples;
    const double total = static_cast<double>(samples.size());

    std::string out;
    char line[1024];
    std::snprintf(line, sizeof line,
                  "### Where the time goes: %s\n\n%.0f Hz, blocks of %d. %zu samples of the rendering thread, one every "
                  "250 us; %.1f %% fell in the program's own code (the rest: the host between calls, the runtime's math, "
                  "the system). The render cost %.0f ns a 96 kHz sample with the sampler running.\n\n",
                  what.c_str(), rate, block, samples.size(), 100.0 * static_cast<double>(inside) / total, ns);
    out += line;

    out += "| Module | of the thread's time | ns a 96 kHz sample |\n|---|---:|---:|\n";
    std::vector<std::pair<std::string, std::size_t>> modules(byModule.begin(), byModule.end());
    std::sort(modules.begin(), modules.end(), [](const auto& a, const auto& b) { return a.second > b.second; });
    for (const auto& [module, count] : modules)
    {
        std::snprintf(line, sizeof line, "| %s | %.1f %% | %.0f |\n", module.c_str(),
                      100.0 * static_cast<double>(count) / total, ns * static_cast<double>(count) / total);
        out += line;
    }

    out += "\n| Function | of the thread's time | ns a 96 kHz sample | inlined into it (their time is in this row) |\n"
           "|---|---:|---:|---|\n";
    std::vector<std::pair<std::string, Row>> functions(byFunction.begin(), byFunction.end());
    std::sort(functions.begin(), functions.end(),
              [](const auto& a, const auto& b) { return a.second.samples > b.second.samples; });
    std::size_t hidden = 0;
    for (const auto& [function, row] : functions)
    {
        const double share = static_cast<double>(row.samples) / total;
        if (!all && share < 0.005)
        {
            hidden += row.samples;
            continue;
        }
        std::string inlined = row.inlined;
        if (!all && inlined.size() > 240)
            inlined = inlined.substr(0, 240) + " ...";
        std::snprintf(line, sizeof line, "| %s | %.1f %% | %.0f | %s |\n", function.c_str(), 100.0 * share, ns * share,
                      inlined.c_str());
        out += line;
    }
    if (hidden > 0)
    {
        std::snprintf(line, sizeof line, "| (functions under 0.5 %% each; --all lists them) | %.1f %% | %.0f | |\n",
                      100.0 * static_cast<double>(hidden) / total, ns * static_cast<double>(hidden) / total);
        out += line;
    }

    std::fputs(out.c_str(), stdout);
    if (!outName.empty())
    {
        std::error_code ec;
        if (std::filesystem::path(outName).has_parent_path())
            std::filesystem::create_directories(std::filesystem::path(outName).parent_path(), ec);
        std::ofstream file(outName, std::ios::binary | std::ios::trunc);
        file << out;
    }
    return 0;
}
