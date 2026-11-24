// tools/tee-render/host.cpp: tee-render --build ... --scenario ...: the simulator through the host (tee::SimHost), a
// build (docs/formats.md, "A build") and a scenario (docs/formats.md, "A scenario": content/scenarios/) in; a WAV (32-bit
// float, or 24-bit with --pcm24) and a CSV of the telemetry asked for out; the scenario's assertions checked.
#include "common/Paths.h"

#include "tee/Timeline.h"
#include "tee/Wav.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>

namespace
{
    const char* const kHostHelp =
        "tee-render --build <name|file> --scenario <name|file>: the simulator through the host (SimHost)\n"
        "\n"
        "  --build <b>         a build of content/builds by name (v8), or a .json file; 'defaults': every field's\n"
        "                      default (the scenario's own build, if it names one, is taken when this is not given)\n"
        "  --scenario <s>      a scenario of content/scenarios by name (idle), or a .json file (format tee-scenario)\n"
        "  --rate <hz>         the output's sample rate (the scenario's, else 48000)\n"
        "  --block <n>         the longest call of process, in samples (the scenario's, else 512)\n"
        "  --seconds <s>       render this long instead of the scenario's length\n"
        "  --out <file>        the WAV (out/<build>-<scenario>.wav); '-' writes none\n"
        "  --pcm24             the WAV as 24-bit integers (else 32-bit floats)\n"
        "  --csv <file>        the telemetry, a row a block (the WAV's name with .csv); '-' writes none\n"
        "  --tel <a,b,...>     the telemetry fields in the CSV (engine.speed,engine.manifold); all numbers if none\n"
        "  --checks            compile with the loop budget and the bounds checks\n"
        "  --sim <dir>         the simulator's directory (sim)\n"
        "  --content <dir>     where builds/ and scenarios/ are (content)\n"
        "\n"
        "  The exit status is 0 when the scenario's assertions hold, 3 when one fails, 1 on an error.\n";

    int failHost(const std::string& message)
    {
        std::fprintf(stderr, "tee-render: %s\n", message.c_str());
        return 1;
    }

    std::vector<std::string> split(const std::string& text)
    {
        std::vector<std::string> out;
        std::stringstream in(text);
        std::string item;
        while (std::getline(in, item, ','))
            if (!item.empty())
                out.push_back(item);
        return out;
    }

    std::filesystem::path named(const std::filesystem::path& directory, const std::string& name)
    {
        const std::filesystem::path p(name);
        if (p.has_extension() || p.has_parent_path())
            return p;
        return directory / (name + ".json");
    }
} // namespace

int hostMain(int argc, char** argv);

int hostMain(int argc, char** argv)
{
    using namespace tee;
    tools::Arguments args(argc, argv);
    std::string buildName, scenarioName, outName, csvName, simFlag, contentFlag, telList, value;
    double rate = 0.0, seconds = 0.0, blockValue = 0.0;
    bool checks = false, pcm24 = false;
    while (!args.done())
    {
        bool ok = true;
        const auto number = [&](double& out) {
            if (!args.value(value))
                return false;
            char* end = nullptr;
            out = std::strtod(value.c_str(), &end);
            return end != value.c_str();
        };
        if (args.is("--help") || args.is("-h"))
        {
            std::fputs(kHostHelp, stdout);
            return 0;
        }
        else if (args.is("--build"))
            ok = args.value(buildName);
        else if (args.is("--scenario"))
            ok = args.value(scenarioName);
        else if (args.is("--rate"))
            ok = number(rate);
        else if (args.is("--block"))
            ok = number(blockValue);
        else if (args.is("--seconds"))
            ok = number(seconds);
        else if (args.is("--out"))
            ok = args.value(outName);
        else if (args.is("--csv"))
            ok = args.value(csvName);
        else if (args.is("--tel"))
            ok = args.value(telList);
        else if (args.is("--pcm24"))
            pcm24 = true;
        else if (args.is("--checks"))
            checks = true;
        else if (args.is("--sim"))
            ok = args.value(simFlag);
        else if (args.is("--content"))
            ok = args.value(contentFlag);
        else
            return failHost("unknown option " + args.peek() + " (--build --help lists them)");
        if (!ok)
            return failHost("an option wants a value (--build --help lists them)");
    }
    if (scenarioName.empty())
        return failHost("give --scenario (a name under content/scenarios, or a file)");

    const std::filesystem::path sim =
        !simFlag.empty() ? std::filesystem::path(simFlag)
                         : tools::findDirectory({}, "TEE_SIM_DIR", argv[0], "sim/ref", TEE_SOURCE_SIM_DIR).parent_path();
    const std::filesystem::path content =
        !contentFlag.empty()
            ? std::filesystem::path(contentFlag)
            : tools::findDirectory({}, "TEE_CONTENT_DIR", argv[0], "content/ref", TEE_SOURCE_CONTENT_DIR).parent_path();

    std::string error;
    Timeline timeline;
    if (!loadTimeline(named(content / "scenarios", scenarioName), timeline, error))
        return failHost(error);
    if (rate > 0.0)
        timeline.rate = rate;
    if (blockValue >= 1.0)
        timeline.block = static_cast<int>(blockValue);
    if (timeline.rate < 8000.0 || timeline.rate > 768000.0 || timeline.block < 1 || timeline.block > 1048576)
        return failHost("--rate is 8000 to 768000, --block 1 to 1048576");

    Sim::Options simOptions;
    simOptions.checks = checks;
    const std::shared_ptr<Sim> compiled = Sim::compile(sim, simOptions, error);
    if (!compiled)
    {
        std::fprintf(stderr, "%s", error.c_str());
        return failHost("the simulator in " + sim.generic_string() + " does not compile");
    }
    Build build(compiled->fields());
    if (buildName.empty())
        buildName = timeline.build;
    std::vector<std::string> warnings;
    if (!buildName.empty() && buildName != "defaults"
        && !build.loadFile(named(content / "builds", buildName), BuildPart::Whole, warnings, error))
        return failHost(error);
    for (const std::string& w : warnings)
        std::fprintf(stderr, "tee-render: warning: %s\n", w.c_str());

    SimHost host;
    LoadOptions load;
    load.sim = simOptions;
    load.sampleRate = timeline.rate;
    load.maxBlock = std::max(16, timeline.block);
    load.worker = false;
    if (timeline.warm)
        load.hold = warmEngine();
    if (!host.attach(compiled, load, error))
        return failHost(error);
    build.attach(&host);
    RunOptions options;
    options.seconds = seconds;
    options.telemetry = split(telList);
    if (!options.telemetry.empty())   // what the assertions measure is kept too
        for (const Assertion& a : timeline.assertions)
            if (a.what == "tel" && std::find(options.telemetry.begin(), options.telemetry.end(), a.field) == options.telemetry.end())
                options.telemetry.push_back(a.field);
    RunResult run;
    if (!runTimeline(host, build, timeline, options, run, error))
        return failHost(error);

    const std::string stem = (buildName.empty() ? std::string("defaults")
                                                 : std::filesystem::path(buildName).stem().generic_string())
                             + "-" + std::filesystem::path(scenarioName).stem().generic_string();
    if (outName.empty())
        outName = "out/" + stem + ".wav";
    if (csvName.empty())
        csvName = outName == "-" ? "-" : std::filesystem::path(outName).replace_extension(".csv").generic_string();
    if (outName != "-")
    {
        Audio audio;
        audio.sampleRate = run.rate;
        audio.left = run.left;
        audio.right = run.right;
        if (!(pcm24 ? writeWav24(outName, audio, error) : writeWav(outName, audio, error)))
            return failHost(error);
    }
    if (csvName != "-")
    {
        std::error_code ec;
        if (std::filesystem::path(csvName).has_parent_path())
            std::filesystem::create_directories(std::filesystem::path(csvName).parent_path(), ec);
        std::FILE* csv = std::fopen(csvName.c_str(), "wb");
        if (csv == nullptr)
            return failHost(csvName + ": cannot be written");
        std::fputs("time", csv);
        for (const std::string& column : run.columns)
            std::fprintf(csv, ",%s", column.c_str());
        std::fputc('\n', csv);
        for (std::size_t r = 0; r < run.rowCount(); ++r)
        {
            std::fprintf(csv, "%.6f", run.rowTime[r]);
            for (std::size_t c = 0; c < run.columns.size(); ++c)
                std::fprintf(csv, ",%.9g", static_cast<double>(run.at(r, c)));
            std::fputc('\n', csv);
        }
        std::fclose(csv);
    }

    std::printf("%s, %s: %.2f s at %.0f Hz in blocks of %d, in %.2f s (%.1f times real time)\n",
                build.name.empty() ? (buildName.empty() ? "defaults" : buildName.c_str()) : build.name.c_str(),
                timeline.name.c_str(), run.simulatedSeconds, run.rate, timeline.block, run.wallSeconds,
                run.wallSeconds > 0.0 ? run.simulatedSeconds / run.wallSeconds : 0.0);
    std::printf("  host: load mean %.1f %%, peak %.1f %%; guard trips %d, restarts %d; blocks clamped to full scale %u "
                "(the largest sample %.3g)\n",
                100.0 * run.host.loadMean, 100.0 * run.host.loadPeak, static_cast<int>(run.host.guardTrips),
                static_cast<int>(run.host.restarts), run.host.clippedBlocks, static_cast<double>(run.host.largestSample));
    if (outName != "-")
        std::printf("  %s%s\n", outName.c_str(), pcm24 ? " (24-bit)" : "");
    if (csvName != "-")
        std::printf("  %s\n", csvName.c_str());
    if (!run.stopReason.empty())
        std::fprintf(stderr, "tee-render: %s\n", run.stopReason.c_str());
    std::vector<std::string> lines;
    const bool held = checkAssertions(timeline, run, lines);
    for (const std::string& line : lines)
        std::printf("  %s\n", line.c_str());
    return held ? 0 : 3;
}
