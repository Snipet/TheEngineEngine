// tools/tee-render/engine.cpp: tee-render --engine: the simulator (sim/engine.fm) through its contract, a build and an
// engine scenario in (tee/EnginePlayer.h has both formats); a WAV of 32-bit floats and a CSV of the Telemetry out.
#include "common/Paths.h"

#include "tee/EnginePlayer.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace
{
    const char* const kEngineHelp =
        "tee-render --engine: the simulator (sim/engine.fm), rendered through its contract\n"
        "\n"
        "  tee-render --engine --build <name|file> --scenario <name|file> [options]\n"
        "\n"
        "  --build <b>         a build of content/builds by name (v8), or a .json file; none: the defaults\n"
        "  --scenario <s>      an engine scenario of tests/sim/scenarios by name (free-revs), or a .json file\n"
        "  --rate <hz>         the output's sample rate (48000); the engine runs at 96000 inside\n"
        "  --block <n>         the longest call of process, in samples (512)\n"
        "  --out <file>        the WAV (out/engine-<build>-<scenario>.wav); '-' writes none\n"
        "  --csv <file>        the Telemetry, a row a call (the WAV's name with .csv); '-' writes none\n"
        "  --seconds <s>       render this long instead of the scenario's length\n"
        "  --set <path>=<v>    a Spec field over the build's (engine.displacement=0.002; may be given again)\n"
        "  -O <0..3>           FMinus's optimisation level (2)\n"
        "  --checks            compile with the loop budget and the bounds checks\n"
        "  --sim <dir>         the simulator's directory (sim)\n"
        "\n"
        "  tee-render --engine --list               the builds and the scenarios there are\n"
        "  tee-render --engine --defaults           the Spec at its defaults, as a build\n"
        "  tee-render --engine --schema <module>    a module's fields: offset, size, type, name, attributes\n";

    int failEngine(const std::string& message)
    {
        std::fprintf(stderr, "tee-render: %s\n", message.c_str());
        return 1;
    }
} // namespace

int engineMain(int argc, char** argv);

int engineMain(int argc, char** argv)
{
    using namespace tee;
    tools::Arguments args(argc, argv);
    std::string buildName, scenarioName, outName, csvName, simFlag, schemaModule, value;
    std::vector<std::pair<std::string, double>> sets;
    double rate = 48000.0, seconds = 0.0;
    int block = 512, level = 2;
    bool checks = false, list = false, defaults = false;
    while (!args.done())
    {
        const auto need = [&](const char* option) {
            if (args.value(value))
                return true;
            std::fprintf(stderr, "tee-render: %s wants a value\n", option);
            return false;
        };
        if (args.is("--engine"))
            continue;
        if (args.is("--help") || args.is("-h"))
        {
            std::fputs(kEngineHelp, stdout);
            return 0;
        }
        else if (args.is("--list"))
            list = true;
        else if (args.is("--defaults"))
            defaults = true;
        else if (args.is("--checks"))
            checks = true;
        else if (args.is("--build") || args.is("--preset"))
        {
            if (!need("--build"))
                return 2;
            buildName = value;
        }
        else if (args.is("--scenario"))
        {
            if (!need("--scenario"))
                return 2;
            scenarioName = value;
        }
        else if (args.is("--out"))
        {
            if (!need("--out"))
                return 2;
            outName = value;
        }
        else if (args.is("--csv"))
        {
            if (!need("--csv"))
                return 2;
            csvName = value;
        }
        else if (args.is("--sim"))
        {
            if (!need("--sim"))
                return 2;
            simFlag = value;
        }
        else if (args.is("--schema"))
        {
            if (!need("--schema"))
                return 2;
            schemaModule = value;
        }
        else if (args.is("--rate"))
        {
            if (!need("--rate"))
                return 2;
            rate = std::atof(value.c_str());
        }
        else if (args.is("--seconds"))
        {
            if (!need("--seconds"))
                return 2;
            seconds = std::atof(value.c_str());
        }
        else if (args.is("--block"))
        {
            if (!need("--block"))
                return 2;
            block = std::atoi(value.c_str());
        }
        else if (args.is("-O"))
        {
            if (!need("-O"))
                return 2;
            level = std::atoi(value.c_str());
        }
        else if (args.is("--set"))
        {
            if (!need("--set"))
                return 2;
            const std::size_t equals = value.find('=');
            if (equals == std::string::npos || equals == 0)
                return failEngine("--set wants <path>=<value>");
            sets.emplace_back(value.substr(0, equals), std::atof(value.c_str() + equals + 1));
        }
        else
        {
            std::fprintf(stderr, "tee-render: unknown argument '%s' (--engine --help lists them)\n", args.peek().c_str());
            return 2;
        }
    }
    if (rate < 8000.0 || rate > 768000.0 || block < 1 || block > 1 << 20 || level < 0 || level > 3)
        return failEngine("--rate is 8000 to 768000, --block 1 to 1048576, -O 0 to 3");

    // sim/ref's parent is the simulator's directory; content/ref's parent holds builds/; the scenarios are the tests'
    const std::filesystem::path refSim =
        tools::findDirectory({}, "TEE_SIM_DIR", argv[0], "sim/ref", TEE_SOURCE_SIM_DIR);
    const std::filesystem::path sim = !simFlag.empty() ? std::filesystem::path(simFlag) : refSim.parent_path();
    const std::filesystem::path content =
        tools::findDirectory({}, "TEE_CONTENT_DIR", argv[0], "content/ref", TEE_SOURCE_CONTENT_DIR).parent_path();
    const std::filesystem::path builds = content / "builds";
    const std::filesystem::path scenarios = content.parent_path() / "tests" / "sim" / "scenarios";

    if (list)
    {
        std::printf("builds (%s):\n", builds.generic_string().c_str());
        for (const std::string& name : tools::namesIn(builds))
            std::printf("  %s\n", name.c_str());
        std::printf("scenarios (%s):\n", scenarios.generic_string().c_str());
        for (const std::string& name : tools::namesIn(scenarios))
        {
            EngineScenario scenario;
            std::string error;
            if (loadEngineScenario(scenarios / (name + ".json"), scenario, error))
                std::printf("  %-22s %6.1f s  %s\n", name.c_str(), scenario.seconds, scenario.description.c_str());
            else
                std::printf("  %-22s %s\n", name.c_str(), error.c_str());
        }
        return 0;
    }

    EnginePlayer::Options options;
    options.directory = sim;
    options.sampleRate = rate;
    options.optimisation = level;
    options.checks = checks;
    std::string errors, warnings;
    const std::unique_ptr<EnginePlayer> player = EnginePlayer::create(options, errors);
    if (!player)
    {
        std::fprintf(stderr, "%s", errors.c_str());
        return failEngine("the simulator in " + sim.generic_string() + " does not compile");
    }
    if (defaults)
    {
        std::fputs(buildJson(*player, currentBuild(*player, "Defaults", "Every Spec field at its fminus::default."))
                       .c_str(),
                   stdout);
        return 0;
    }
    if (!schemaModule.empty())
    {
        const std::unique_ptr<Schema> schema = Schema::of(player->program().fminus(), schemaModule, errors);
        if (!schema)
            return failEngine(errors);
        std::fputs(schema->describe().c_str(), stdout);
        return 0;
    }
    if (scenarioName.empty())
        return failEngine("give --scenario (--engine --list shows what there is, --engine --help the rest)");

    EngineScenario scenario;
    if (!loadEngineScenario(tools::namedFile(scenarios, scenarioName), scenario, errors))
        return failEngine(errors);
    if (buildName.empty())
        buildName = scenario.build;
    Build build;
    if (!buildName.empty() && !loadBuild(*player, tools::namedFile(builds, buildName), build, errors, warnings))
        return failEngine(errors);
    if (!warnings.empty())
        std::fputs(warnings.c_str(), stderr);
    for (const auto& [path, v] : sets)
    {
        if (player->specIndex(path) < 0)
            return failEngine("--set " + path + ": the Spec has no such field");
        build.values[path] = {v};
    }

    RenderOptions renderOptions;
    renderOptions.blockSize = block;
    renderOptions.seconds = seconds;
    RenderResult result;
    const auto began = std::chrono::steady_clock::now();
    if (!renderEngine(*player, build, scenario, renderOptions, result, errors))
        return failEngine(errors);
    const double took = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();

    const std::string stem = "engine-" + (buildName.empty() ? std::string("defaults")
                                                             : std::filesystem::path(buildName).stem().generic_string())
                             + "-" + std::filesystem::path(scenarioName).stem().generic_string();
    if (outName.empty())
        outName = "out/" + stem + ".wav";
    if (csvName.empty())
        csvName = outName == "-" ? "-" : std::filesystem::path(outName).replace_extension(".csv").generic_string();
    if (outName != "-" && !writeWav(outName, result.audio, errors))
        return failEngine(errors);
    if (csvName != "-")
    {
        std::error_code ec;
        if (std::filesystem::path(csvName).has_parent_path())
            std::filesystem::create_directories(std::filesystem::path(csvName).parent_path(), ec);
        std::FILE* csv = std::fopen(csvName.c_str(), "wb");
        if (csv == nullptr)
            return failEngine(csvName + ": cannot be written");
        std::fputs("time", csv);
        for (const std::string& column : result.columns)
            std::fprintf(csv, ",%s", column.c_str());
        std::fputc('\n', csv);
        for (std::size_t r = 0; r < result.rowCount(); ++r)
        {
            std::fprintf(csv, "%.6f", result.rowTime[r]);
            for (std::size_t c = 0; c < result.columns.size(); ++c)
                std::fprintf(csv, ",%.9g", static_cast<double>(result.at(r, c)));
            std::fputc('\n', csv);
        }
        std::fclose(csv);
    }

    const double audioSeconds = static_cast<double>(result.frames) / rate;
    std::printf("%s, %s: %.2f s at %.0f Hz in blocks of %d\n", build.name.empty() ? "defaults" : build.name.c_str(),
                scenario.name.c_str(), audioSeconds, rate, block);
    std::printf("  compile %.2f s, machine code %.2f s (%zu bytes), -O%d%s\n", player->program().compileSeconds(),
                player->program().linkSeconds(), player->program().codeBytes(), level, checks ? ", checks on" : "");
    std::printf("  render %.2f s: %.1f %% of real time while the engine ran, the slowest call %.1f %%\n", took,
                result.soundingFrames > 0 ? 100.0 * result.soundingSeconds * rate / static_cast<double>(result.soundingFrames) : 0.0,
                100.0 * result.worstCallShare);
    std::printf("  peak %.4f%s\n", result.peak, result.peak > 1.0 ? " (over full scale)" : "");
    if (outName != "-")
        std::printf("  %s\n", outName.c_str());
    if (csvName != "-")
        std::printf("  %s\n", csvName.c_str());
    if (!player->program().warnings().empty())
        std::fprintf(stderr, "%s", player->program().warnings().c_str());
    if (result.stopped)
        return failEngine("the program was stopped: " + result.stopReason);
    return 0;
}
