// tests/host/RenderToolTest.cpp: tee-render's build-and-scenario path (--build, --scenario): the WAV it writes is the
// run the host library makes of the same build and scenario, sample for sample; 24-bit PCM is that, rounded; the
// scenario's assertions decide its exit status.
#include "Support.h"

#include "tee/Timeline.h"
#include "tee/Wav.h"

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <sstream>
#if !defined(_WIN32)
#include <sys/wait.h>
#endif

using namespace tee;
using namespace tee::test;

namespace
{
    int runTool(const std::string& arguments)
    {
        const std::string command = std::string("\"") + TEE_BIN_DIR + "/tee-render\" " + arguments;
#if defined(_WIN32)
        const std::string line = "\"" + command + "\"";   // cmd /c strips the outer pair of quotes
#else
        const std::string& line = command;
#endif
        const int status = std::system(line.c_str());
#if defined(_WIN32)
        return status;
#else
        return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
#endif
    }

    std::string quoted(const std::filesystem::path& p)
    {
        return "\"" + p.string() + "\"";
    }
} // namespace

TEST_CASE("tee-render --build --scenario writes the host's run, and says whether its assertions hold", "[tools][render]")
{
    const std::filesystem::path out = std::filesystem::path(TEE_SOURCE_DIR) / "out" / "tests" / "render";
    std::filesystem::create_directories(out);
    const auto sim = standinSim();
    Build build(sim->fields());
    build.set("engine.torque", 420);
    std::string error;
    REQUIRE(build.saveFile(out / "build.json", BuildPart::Whole, error));
    const char* scenario = R"({"format": "tee-scenario", "version": 1, "name": "start and rev", "seconds": 3,
        "rate": 32000, "block": 200,
        "events": [{"t": 0, "ctl": {"ignition": 2}}, {"t": 0.8, "ctl": {"ignition": 1}},
                   {"t": 1.5, "ctl": {"throttle": 1}, "ramp": 0.2}],
        "assertions": [{"tel": "engine.state", "measure": "last", ">=": 2, "source": "it runs"},
                       {"audio": "peak", "<=": 1}]})";
    std::ofstream(out / "scenario.json") << scenario;

    // the library's run
    Timeline timeline;
    REQUIRE(loadTimeline(out / "scenario.json", timeline, error));
    SimHost host;
    LoadOptions load = offline(timeline.rate, timeline.block);
    REQUIRE(host.attach(sim, load, error));
    Build copy(sim->fields());
    std::vector<std::string> warnings;
    REQUIRE(copy.loadFile(out / "build.json", BuildPart::Whole, warnings, error));
    copy.attach(&host);
    RunResult run;
    REQUIRE(runTimeline(host, copy, timeline, {}, run, error));

    const std::string common = "--sim " + quoted(standin()) + " --build " + quoted(out / "build.json") + " --scenario "
                               + quoted(out / "scenario.json");
    REQUIRE(runTool(common + " --out " + quoted(out / "f.wav") + " --tel engine.speed") == 0);
    Audio audio;
    REQUIRE(readWav(out / "f.wav", audio, error));
    CHECK(audio.sampleRate == 32000.0);
    REQUIRE(audio.frames() == run.left.size());
    CHECK(audio.left == run.left);
    CHECK(audio.right == run.right);
    {
        std::ifstream csv(out / "f.csv");
        std::string header;
        std::getline(csv, header);
        CHECK(header == "time,engine.speed,engine.state");   // asked for, then what the assertions read
    }

    // a define the program does not read changes nothing; a malformed one is refused
    REQUIRE(runTool(common + " --define TEE_UNREAD --define B=2 --out " + quoted(out / "d.wav") + " --csv -") == 0);
    Audio defined;
    REQUIRE(readWav(out / "d.wav", defined, error));
    CHECK(defined.left == run.left);
    CHECK(runTool(common + " --define =3 --out - --csv -") == 1);

    // 24-bit: the same samples, rounded to 2^-23
    REQUIRE(runTool(common + " --pcm24 --out " + quoted(out / "i.wav") + " --csv -") == 0);
    std::ifstream in(out / "i.wav", std::ios::binary);
    const std::string bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    REQUIRE(bytes.size() == 44 + run.left.size() * 6);
    CHECK(bytes[20] == 1);    // integer PCM
    CHECK(bytes[34] == 24);   // bits a sample
    double worst = 0.0;
    for (std::size_t i = 0; i < run.left.size(); ++i)
    {
        const auto* p = reinterpret_cast<const unsigned char*>(bytes.data() + 44 + i * 6);
        std::int32_t v = static_cast<std::int32_t>(p[0] | (p[1] << 8) | (p[2] << 16));
        if ((v & 0x800000) != 0)
            v -= 0x1000000;
        worst = std::max(worst, std::fabs(v / 8388607.0 - static_cast<double>(run.left[i])));
    }
    CHECK(worst <= 0.6 / 8388607.0);

    // an assertion that fails: exit status 3
    std::string failing = scenario;
    failing.replace(failing.find("\">=\": 2"), 7, "\">=\": 9");
    std::ofstream(out / "failing.json") << failing;
    CHECK(runTool("--sim " + quoted(standin()) + " --build " + quoted(out / "build.json") + " --scenario "
                  + quoted(out / "failing.json") + " --out - --csv -")
          == 3);
}

TEST_CASE("tee-render: an unknown --tel field is an error naming the nearest, and the last run's files are gone",
          "[tools][render]")
{
    const std::filesystem::path out = std::filesystem::path(TEE_SOURCE_DIR) / "out" / "tests" / "render-unknown";
    std::filesystem::create_directories(out);
    const auto sim = standinSim();
    Build build(sim->fields());
    std::string error;
    REQUIRE(build.saveFile(out / "build.json", BuildPart::Whole, error));
    std::ofstream(out / "scenario.json") << R"({"format": "tee-scenario", "version": 1, "name": "start", "seconds": 1,
        "rate": 32000, "block": 200, "events": [{"t": 0, "ctl": {"ignition": 2}}]})";
    // the last run's files, which a failed run must not leave to be read as its own
    std::ofstream(out / "u.wav") << "last run's";
    std::ofstream(out / "u.csv") << "last run's";
    const std::string common = "--sim " + quoted(standin()) + " --build " + quoted(out / "build.json") + " --scenario "
                               + quoted(out / "scenario.json") + " --out " + quoted(out / "u.wav");
    CHECK(runTool(common + " --tel engine.sped 2> " + quoted(out / "stderr.txt")) == 1);
    std::ifstream in(out / "stderr.txt");
    std::ostringstream said;
    said << in.rdbuf();
    INFO(said.str());
    CHECK(said.str().find("engine.sped") != std::string::npos);
    CHECK(said.str().find("nearest: engine.speed") != std::string::npos);
    CHECK_FALSE(std::filesystem::exists(out / "u.wav"));
    CHECK_FALSE(std::filesystem::exists(out / "u.csv"));
}
