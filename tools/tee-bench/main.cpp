// tools/tee-bench: what the reference costs. For each preset and scenario: nanoseconds a sample of the engine's own
// 96 kHz rate, and the share of real time at 48 kHz in blocks of 512, without the checks (a release build) and with
// them (a development build); and how long the program takes to compile at -O0, -O1 and -O2. It writes a Markdown
// table (docs/measurements/p0-bench.md is one, with the machine it ran on).
//
// A cost is the thread's own processor time (tee/Clock.h) over the calls of process in which the engine ran: what the
// code took, not counting the moments a busy machine gave the core to something else. A busy machine still slows the
// code a little (the cache is shared, and so may be the core), so each figure is the least of --repeat runs. The
// slowest call is wall time: it is what a real-time thread would have met.
#include "common/Paths.h"

#include "tee/Render.h"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>

#if defined(__APPLE__)
#include <sys/sysctl.h>
#endif

namespace
{
    const char* const kHelp =
        "tee-bench: what the reference engine costs on this machine\n"
        "\n"
        "  tee-bench [options]\n"
        "\n"
        "  --quick            the V8 and the V12 on free-revs alone, once each: a first look (half a minute)\n"
        "  --presets <a,b>    these presets (every one of content/ref/presets)\n"
        "  --scenarios <a,b>  these scenarios (every one of content/ref/scenarios)\n"
        "  --repeat <n>       runs of each, the least time kept (3; --quick 1)\n"
        "  --rate <hz>        the output's sample rate (48000)\n"
        "  --block <n>        the longest call of process (512)\n"
        "  --no-checks        leave out the runs with the loop budget and the bounds checks\n"
        "  --out <file>       write the table there too (it is always printed)\n"
        "  --sim <dir>, --content <dir>   as tee-render's\n";

    std::vector<std::string> split(const std::string& list)
    {
        std::vector<std::string> out;
        std::stringstream in(list);
        std::string item;
        while (std::getline(in, item, ','))
            if (!item.empty())
                out.push_back(item);
        return out;
    }

    // The processor's name, as the system gives it.
    std::string cpuName()
    {
#if defined(__linux__)
        std::ifstream in("/proc/cpuinfo");
        std::string line;
        while (std::getline(in, line))
        {
            if (line.rfind("model name", 0) == 0)
            {
                const std::size_t colon = line.find(':');
                if (colon != std::string::npos)
                    return line.substr(line.find_first_not_of(" \t", colon + 1));
            }
        }
#elif defined(__APPLE__)
        char name[256] = {};
        std::size_t size = sizeof name - 1;
        if (sysctlbyname("machdep.cpu.brand_string", name, &size, nullptr, 0) == 0)
            return name;
#elif defined(_WIN32)
        if (const char* name = std::getenv("PROCESSOR_IDENTIFIER"))
            return name;
#endif
        return "unknown";
    }

    const char* systemName()
    {
#if defined(__linux__)
        return "Linux";
#elif defined(__APPLE__)
        return "macOS";
#elif defined(_WIN32)
        return "Windows";
#else
        return "unknown";
#endif
    }

    const char* machineName()
    {
#if defined(__x86_64__) || defined(_M_X64)
        return "x86-64";
#elif defined(__aarch64__) || defined(_M_ARM64)
        return "arm64";
#else
        return "unknown";
#endif
    }

    struct Cost
    {
        double nsPerSample = 0.0;   // processor time a sample of the engine's 96 kHz rate
        double share = 0.0;         // processor time over the audio's duration
        double worst = 0.0;         // the slowest call's wall time over its samples' duration
        double wallOver = 0.0;      // wall time over processor time, for the run kept: 1 on an idle machine
        bool ok = false;
        std::string failed;
    };

    // The least of `repeat` renders.
    Cost measure(tee::RefPlayer& player, const tee::Preset& preset, const tee::Scenario& scenario, int block, int repeat)
    {
        Cost cost;
        for (int k = 0; k < repeat; ++k)
        {
            tee::RenderOptions options;
            options.blockSize = block;
            options.keepAudio = false;
            tee::RenderResult result;
            std::string error;
            if (!tee::render(player, preset, scenario, options, result, error))
            {
                cost.failed = error;
                return cost;
            }
            if (result.stopped)
            {
                cost.failed = result.stopReason;
                return cost;
            }
            if (result.soundingFrames == 0)
            {
                cost.failed = "the engine never ran";
                return cost;
            }
            const double audio = static_cast<double>(result.soundingFrames) / player.sampleRate();
            const double share = result.soundingCpuSeconds / audio;
            if (!cost.ok || share < cost.share)
            {
                cost.share = share;
                cost.nsPerSample = result.soundingCpuSeconds * 1.0e9 / (audio * 96000.0);
                cost.wallOver = result.soundingCpuSeconds > 0.0 ? result.soundingSeconds / result.soundingCpuSeconds : 0.0;
            }
            cost.worst = !cost.ok ? result.worstCallShare : std::min(cost.worst, result.worstCallShare);
            cost.ok = true;
        }
        return cost;
    }
} // namespace

// tee-bench --engine: the simulator against the reference (engine.cpp)
int engineBenchMain(int argc, char** argv);

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
        if (std::string(argv[i]) == "--engine")
            return engineBenchMain(argc, argv);
    using namespace tee;
    tools::Arguments args(argc, argv);
    std::string simFlag, contentFlag, outName, value;
    std::vector<std::string> presetNames, scenarioNames;
    double rate = 48000.0;
    int block = 512;
    int repeat = 0;
    bool quick = false, withChecks = true;
    while (!args.done())
    {
        const auto need = [&](const char* option) {
            if (args.value(value))
                return true;
            std::fprintf(stderr, "tee-bench: %s wants a value\n", option);
            return false;
        };
        if (args.is("--help") || args.is("-h"))
        {
            std::fputs(kHelp, stdout);
            return 0;
        }
        else if (args.is("--quick"))
            quick = true;
        else if (args.is("--no-checks"))
            withChecks = false;
        else if (args.is("--presets"))
        {
            if (!need("--presets"))
                return 2;
            presetNames = split(value);
        }
        else if (args.is("--scenarios"))
        {
            if (!need("--scenarios"))
                return 2;
            scenarioNames = split(value);
        }
        else if (args.is("--repeat"))
        {
            if (!need("--repeat"))
                return 2;
            repeat = std::atoi(value.c_str());
        }
        else if (args.is("--rate"))
        {
            if (!need("--rate"))
                return 2;
            rate = std::atof(value.c_str());
        }
        else if (args.is("--block"))
        {
            if (!need("--block"))
                return 2;
            block = std::atoi(value.c_str());
        }
        else if (args.is("--out"))
        {
            if (!need("--out"))
                return 2;
            outName = value;
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
        else
        {
            std::fprintf(stderr, "tee-bench: unknown argument '%s' (--help lists them)\n", args.peek().c_str());
            return 2;
        }
    }
    if (rate < 8000.0 || rate > 768000.0 || block < 1 || block > 1 << 20)
    {
        std::fprintf(stderr, "tee-bench: --rate is 8000 to 768000, --block 1 to 1048576\n");
        return 2;
    }
    const std::filesystem::path sim = tools::findDirectory(simFlag, "TEE_SIM_DIR", argv[0], "sim/ref", TEE_SOURCE_SIM_DIR);
    const std::filesystem::path content =
        tools::findDirectory(contentFlag, "TEE_CONTENT_DIR", argv[0], "content/ref", TEE_SOURCE_CONTENT_DIR);
    if (quick)
    {
        if (presetNames.empty())
            presetNames = {"v8", "bench-v12"};
        if (scenarioNames.empty())
            scenarioNames = {"free-revs"};
    }
    if (presetNames.empty())
        presetNames = tools::namesIn(content / "presets");
    if (scenarioNames.empty())
        scenarioNames = tools::namesIn(content / "scenarios");
    if (repeat <= 0)
        repeat = quick ? 1 : 3;

    std::string errors;
    std::vector<Preset> presets(presetNames.size());
    std::vector<Scenario> scenarios(scenarioNames.size());
    for (std::size_t p = 0; p < presetNames.size(); ++p)
        if (!loadPreset(tools::namedFile(content / "presets", presetNames[p]), presets[p], errors))
            return std::fprintf(stderr, "tee-bench: %s\n", errors.c_str()), 1;
    for (std::size_t s = 0; s < scenarioNames.size(); ++s)
        if (!loadScenario(tools::namedFile(content / "scenarios", scenarioNames[s]), scenarios[s], errors))
            return std::fprintf(stderr, "tee-bench: %s\n", errors.c_str()), 1;

    std::string out;
    const auto add = [&](const std::string& text) {
        out += text;
        std::fputs(text.c_str(), stdout);
        std::fflush(stdout);
    };
    char line[512];

    std::snprintf(line, sizeof line,
                  "Machine: %s, %u logical cores, %s %s. FMinus %s on LLVM %s. tee-bench %s%s, the least of %d run%s.\n\n",
                  cpuName().c_str(), std::thread::hardware_concurrency(), systemName(), machineName(),
                  FMINUS_VERSION_STRING, FMINUS_LLVM_PACKAGE_VERSION, TEE_VERSION, quick ? " --quick" : "", repeat,
                  repeat == 1 ? "" : "s");
    add(line);

    // ---- the compile -----------------------------------------------------------------------------------------------
    add("### Compiling the reference\n\n");
    add("Seconds of the compiling thread's processor time for `program.fm` and the ten files it includes (5,173\n"
        "lines): FMinus's `compile()`, then the machine code (the optimiser and the code generator, which FMinus runs at\n"
        "the first look-up of a function; with the checks it runs them inside `compile()`).\n\n");
    add("| Level | compile() | machine code | together | code bytes | with checks, together | code bytes |\n");
    add("|---|---:|---:|---:|---:|---:|---:|\n");
    for (int level = 0; level <= 2; ++level)
    {
        double best[2][3] = {{0, 0, 0}, {0, 0, 0}};
        std::size_t bytes[2] = {0, 0};
        for (int checks = 0; checks < (withChecks ? 2 : 1); ++checks)
        {
            for (int k = 0; k < repeat; ++k)
            {
                RefPlayer::Options options;
                options.directory = sim;
                options.sampleRate = rate;
                options.optimisation = level;
                options.checks = checks != 0;
                const std::unique_ptr<RefPlayer> player = RefPlayer::create(options, errors);
                if (!player)
                {
                    std::fprintf(stderr, "%s\ntee-bench: the program in %s does not compile\n", errors.c_str(),
                                 sim.generic_string().c_str());
                    return 1;
                }
                const double c = player->program().compileCpuSeconds();
                const double l = player->program().linkCpuSeconds();
                if (k == 0 || c + l < best[checks][2])
                {
                    best[checks][0] = c;
                    best[checks][1] = l;
                    best[checks][2] = c + l;
                }
                bytes[checks] = player->program().codeBytes();
            }
        }
        if (withChecks)
            std::snprintf(line, sizeof line, "| -O%d | %.2f | %.2f | %.2f | %zu | %.2f | %zu |\n", level, best[0][0],
                          best[0][1], best[0][2], bytes[0], best[1][2], bytes[1]);
        else
            std::snprintf(line, sizeof line, "| -O%d | %.2f | %.2f | %.2f | %zu | | |\n", level, best[0][0], best[0][1],
                          best[0][2], bytes[0]);
        add(line);
    }

    // ---- playing ---------------------------------------------------------------------------------------------------
    std::snprintf(line, sizeof line, "\n### Playing it\n\n-O2, %.0f Hz, blocks of %d, over the calls in which the engine ran.\n", rate, block);
    add(line);
    add("\"ns\" is the thread's processor time for one sample of the engine's own 96 kHz rate; \"real time\" is that\n"
        "time over the audio's duration (100 % is one core, whole); \"slowest\" is the slowest single call's wall time\n"
        "over its own duration, which a busy machine lengthens. \"With checks\" is a development build: the loop budget\n"
        "and the bounds checks on.\n\n");
    add("| Preset | Scenario | ns | real time | slowest | with checks: ns | real time | slowest |\n");
    add("|---|---|---:|---:|---:|---:|---:|---:|\n");
    std::unique_ptr<RefPlayer> players[2];
    for (int checks = 0; checks < (withChecks ? 2 : 1); ++checks)
    {
        RefPlayer::Options options;
        options.directory = sim;
        options.sampleRate = rate;
        options.checks = checks != 0;
        players[checks] = RefPlayer::create(options, errors);
        if (!players[checks])
        {
            std::fprintf(stderr, "%s\n", errors.c_str());
            return 1;
        }
    }
    bool allOk = true;
    double wallOverSum = 0.0;
    int wallOverCount = 0;
    for (std::size_t p = 0; p < presets.size(); ++p)
    {
        for (std::size_t s = 0; s < scenarios.size(); ++s)
        {
            const Cost plain = measure(*players[0], presets[p], scenarios[s], block, repeat);
            Cost checked;
            if (withChecks)
                checked = measure(*players[1], presets[p], scenarios[s], block, repeat);
            std::string row = "| " + presetNames[p] + " | " + scenarioNames[s] + " | ";
            const auto cells = [&](const Cost& cost) {
                if (!cost.ok)
                {
                    allOk = false;
                    return "failed: " + cost.failed + " | | ";
                }
                std::snprintf(line, sizeof line, "%.0f | %.1f %% | %.0f %% | ", cost.nsPerSample, 100.0 * cost.share,
                              100.0 * cost.worst);
                return std::string(line);
            };
            row += cells(plain);
            row += withChecks ? cells(checked) : std::string(" | | | ");
            row.erase(row.find_last_not_of(' ') + 1);
            add(row + "\n");
            if (plain.ok)
            {
                wallOverSum += plain.wallOver;
                ++wallOverCount;
            }
        }
    }
    if (wallOverCount > 0)
    {
        std::snprintf(line, sizeof line,
                      "\nHow busy the machine was: the wall time of those calls ran %.0f %% over their processor time, on "
                      "average (0 %% on an idle machine).\n",
                      100.0 * (wallOverSum / wallOverCount - 1.0));
        add(line);
    }

    if (!outName.empty())
    {
        std::error_code ec;
        if (std::filesystem::path(outName).has_parent_path())
            std::filesystem::create_directories(std::filesystem::path(outName).parent_path(), ec);
        std::ofstream file(outName, std::ios::binary | std::ios::trunc);
        file << out;
        if (!file)
        {
            std::fprintf(stderr, "tee-bench: %s cannot be written\n", outName.c_str());
            return 1;
        }
    }
    return allOk ? 0 : 1;
}
