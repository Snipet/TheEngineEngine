// tools/tee-render: plays the reference engine to a file. A preset (the dials), a scenario (what the player does, over
// time), a sample rate and a block size in; a WAV of 32-bit floats and a CSV of what the panel showed out.
// docs/formats.md describes the files; `tee-render --help` the options.
#include "common/Paths.h"

#include "tee/Render.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <string>

namespace
{
    const char* const kHelp =
        "tee-render: the reference engine, rendered to a WAV file\n"
        "\n"
        "  tee-render --preset <name|file> --scenario <name|file> [options]\n"
        "\n"
        "  --preset <p>      a preset of content/ref/presets by name (v8), or a .json file\n"
        "  --scenario <s>    a scenario of content/ref/scenarios by name (free-revs), or a .json file\n"
        "  --rate <hz>       the output's sample rate (48000); the engine runs at 96000 inside\n"
        "  --block <n>       the longest call of process, in samples (512)\n"
        "  --out <file>      the WAV (out/<preset>-<scenario>.wav); '-' writes none\n"
        "  --csv <file>      the panel's show* fields, a row a call (the WAV's name with .csv); '-' writes none\n"
        "  --seconds <s>     render this long instead of the scenario's length\n"
        "  --set <dial>=<v>  a dial, over the preset's (may be given again)\n"
        "  -O <0..3>         FMinus's optimisation level (2)\n"
        "  --checks          compile with the loop budget and the bounds checks\n"
        "  --sim <dir>       the program's directory (sim/ref)\n"
        "  --content <dir>   where presets/ and scenarios/ are (content/ref)\n"
        "\n"
        "  tee-render --list             the presets and the scenarios there are\n"
        "  tee-render --defaults         the program's dials at their defaults, as a preset\n"
        "  tee-render --schema <module>  a module's fields: offset, size, type, name, attributes\n"
        "  tee-render --engine ...       the simulator (sim/engine.fm) through its contract: --engine --help\n"
        "  tee-render --build B --scenario S   the simulator through the host, a build and a scenario of\n"
        "                                content/: --build x --help\n";

    int fail(const std::string& message)
    {
        std::fprintf(stderr, "tee-render: %s\n", message.c_str());
        return 1;
    }
} // namespace

// tee-render --engine: the simulator through its contract (engine.cpp)
int engineMain(int argc, char** argv);
// tee-render --build ... --scenario ...: the simulator through the host (host.cpp)
int hostMain(int argc, char** argv);

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "--engine")
            return engineMain(argc, argv);
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "--build")
            return hostMain(argc, argv);
    using namespace tee;
    tools::Arguments args(argc, argv);
    std::string presetName, scenarioName, outName, csvName, simFlag, contentFlag, schemaModule, value;
    std::vector<std::pair<std::string, double>> sets;
    double rate = 48000.0;
    double seconds = 0.0;
    int block = 512;
    int level = 2;
    bool checks = false, list = false, defaults = false;

    while (!args.done())
    {
        const auto need = [&](const char* option) {
            if (args.value(value))
                return true;
            std::fprintf(stderr, "tee-render: %s wants a value\n", option);
            return false;
        };
        if (args.is("--help") || args.is("-h"))
        {
            std::fputs(kHelp, stdout);
            return 0;
        }
        else if (args.is("--list"))
            list = true;
        else if (args.is("--defaults"))
            defaults = true;
        else if (args.is("--checks"))
            checks = true;
        else if (args.is("--preset"))
        {
            if (!need("--preset"))
                return 2;
            presetName = value;
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
        else if (args.is("--content"))
        {
            if (!need("--content"))
                return 2;
            contentFlag = value;
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
                return fail("--set wants <dial>=<value>");
            sets.emplace_back(value.substr(0, equals), std::atof(value.c_str() + equals + 1));
        }
        else
        {
            std::fprintf(stderr, "tee-render: unknown argument '%s' (--help lists them)\n", args.peek().c_str());
            return 2;
        }
    }
    if (rate < 8000.0 || rate > 768000.0 || block < 1 || block > 1 << 20 || level < 0 || level > 3)
        return fail("--rate is 8000 to 768000, --block 1 to 1048576, -O 0 to 3");

    const std::filesystem::path sim = tools::findDirectory(simFlag, "TEE_SIM_DIR", argv[0], "sim/ref", TEE_SOURCE_SIM_DIR);
    const std::filesystem::path content =
        tools::findDirectory(contentFlag, "TEE_CONTENT_DIR", argv[0], "content/ref", TEE_SOURCE_CONTENT_DIR);

    if (list)
    {
        std::printf("presets (%s):\n", (content / "presets").generic_string().c_str());
        for (const std::string& name : tools::namesIn(content / "presets"))
            std::printf("  %s\n", name.c_str());
        std::printf("scenarios (%s):\n", (content / "scenarios").generic_string().c_str());
        for (const std::string& name : tools::namesIn(content / "scenarios"))
        {
            Scenario scenario;
            std::string error;
            if (loadScenario(content / "scenarios" / (name + ".json"), scenario, error))
                std::printf("  %-22s %6.1f s  %s\n", name.c_str(), scenario.seconds, scenario.description.c_str());
            else
                std::printf("  %-22s %s\n", name.c_str(), error.c_str());
        }
        return 0;
    }

    RefPlayer::Options options;
    options.directory = sim;
    options.sampleRate = rate;
    options.optimisation = level;
    options.checks = checks;
    std::string errors;
    const std::unique_ptr<RefPlayer> player = RefPlayer::create(options, errors);
    if (!player)
    {
        std::fprintf(stderr, "%s", errors.c_str());
        return fail("the program in " + sim.generic_string() + " does not compile");
    }

    if (defaults)
    {
        Preset preset;
        preset.name = "Defaults";
        preset.description = "Every dial at its fminus::default in program.fm.";
        for (const RefPlayer::Dial& dial : player->dials())
            preset.dials.emplace_back(dial.name, dial.defaultValue);
        std::fputs(presetJson(preset).c_str(), stdout);
        return 0;
    }
    if (!schemaModule.empty())
    {
        const std::unique_ptr<Schema> schema = Schema::of(player->program().fminus(), schemaModule, errors);
        if (!schema)
            return fail(errors);
        std::fputs(schema->describe().c_str(), stdout);
        return 0;
    }
    if (presetName.empty() || scenarioName.empty())
        return fail("give --preset and --scenario (--list shows what there is, --help the rest)");

    Preset preset;
    Scenario scenario;
    if (!loadPreset(tools::namedFile(content / "presets", presetName), preset, errors))
        return fail(errors);
    if (!loadScenario(tools::namedFile(content / "scenarios", scenarioName), scenario, errors))
        return fail(errors);
    for (const auto& set : sets)
        preset.dials.push_back(set);

    RenderOptions renderOptions;
    renderOptions.blockSize = block;
    renderOptions.seconds = seconds;
    RenderResult result;
    const auto began = std::chrono::steady_clock::now();
    if (!render(*player, preset, scenario, renderOptions, result, errors))
        return fail(errors);
    const double took = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();

    const std::string stem = std::filesystem::path(presetName).stem().generic_string() + "-"
                             + std::filesystem::path(scenarioName).stem().generic_string();
    if (outName.empty())
        outName = "out/" + stem + ".wav";
    if (csvName.empty())
        csvName = outName == "-" ? "-" : std::filesystem::path(outName).replace_extension(".csv").generic_string();
    if (outName != "-" && !writeWav(outName, result.audio, errors))
        return fail(errors);
    if (csvName != "-")
    {
        std::error_code ec;
        if (std::filesystem::path(csvName).has_parent_path())
            std::filesystem::create_directories(std::filesystem::path(csvName).parent_path(), ec);
        std::FILE* csv = std::fopen(csvName.c_str(), "wb");
        if (csv == nullptr)
            return fail(csvName + ": cannot be written");
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
    std::printf("%s, %s: %.2f s at %.0f Hz in blocks of %d\n", preset.name.c_str(), scenario.name.c_str(), audioSeconds,
                rate, block);
    std::printf("  compile %.2f s, machine code %.2f s (%zu bytes), -O%d%s\n", player->program().compileSeconds(),
                player->program().linkSeconds(), player->program().codeBytes(), level, checks ? ", checks on" : "");
    std::printf("  render %.2f s: %.1f %% of real time while the engine ran (processor time; %.1f %% by the clock), the "
                "slowest call %.1f %%\n",
                took,
                result.soundingFrames > 0 ? 100.0 * result.soundingCpuSeconds * rate / static_cast<double>(result.soundingFrames) : 0.0,
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
        return fail("the program was stopped: " + result.stopReason);
    return 0;
}
