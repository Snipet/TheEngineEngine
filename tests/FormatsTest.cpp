// tests/FormatsTest.cpp: presets, scenarios and WAV files as docs/formats.md describes them, and the measures the
// other tests use (tests/support/Audio.h).
#include "support/Audio.h"

#include "tee/Scenario.h"
#include "tee/Wav.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <filesystem>

using namespace tee;
using namespace tee::test;

TEST_CASE("a preset is the dials by name, in the file's order", "[formats][preset]")
{
    Preset preset;
    std::string error;
    REQUIRE(parsePreset(R"({ "format": "tee-ref-preset/1", "name": "Small",
                             "dials": { "size": 1.6, "engine": 5, "starter": true } })",
                        "small.json", preset, error));
    CHECK(preset.name == "Small");
    REQUIRE(preset.dials.size() == 3);
    CHECK(preset.dials[0] == std::pair<std::string, double>("size", 1.6));
    CHECK(preset.dials[1] == std::pair<std::string, double>("engine", 5.0));
    CHECK(preset.dials[2] == std::pair<std::string, double>("starter", 1.0));

    // what it writes, it reads
    Preset again;
    REQUIRE(parsePreset(presetJson(preset), "again.json", again, error));
    CHECK(again.dials == preset.dials);

    CHECK_FALSE(parsePreset("{", "bad.json", preset, error));
    CHECK(error.find("bad.json") != std::string::npos);
    CHECK_FALSE(parsePreset(R"({ "format": "something-else", "dials": {} })", "bad.json", preset, error));
    CHECK_FALSE(parsePreset(R"({ "format": "tee-ref-preset/1" })", "bad.json", preset, error));
    CHECK_FALSE(parsePreset(R"({ "format": "tee-ref-preset/1", "dials": { "size": "big" } })", "bad.json", preset, error));
}

TEST_CASE("a scenario is unrolled into flat events in time order", "[formats][scenario]")
{
    Scenario scenario;
    std::string error;
    const char* text = R"({
        "format": "tee-ref-scenario/1", "name": "t", "seconds": 3,
        "events": [
            // out of order in the file: sorted by time when read
            { "t": 2.0, "tap": "paddleUp", "hold": 0.2 },
            { "t": 0.0, "dials": { "drive": 1, "mass": 1500 }, "input": { "clutch": 1 } },
            { "t": 0.5, "key": 36, "velocity": 0.5 },
            { "t": 0.6, "keyUp": true },
            { "t": 1.0, "input": { "throttle": 1.0 }, "ramp": 0.02 },
            { "t": 1.5, "press": ["start", "paddleDown"] },
            { "t": 1.6, "release": "start" }
        ] })";
    REQUIRE(parseScenario(text, "t.json", scenario, error));
    CHECK(scenario.seconds == 3.0);
    for (std::size_t k = 1; k < scenario.events.size(); ++k)
        CHECK(scenario.events[k - 1].time <= scenario.events[k].time);

    // t = 0: the two dials, then the clutch
    REQUIRE(scenario.events.size() == 3 + 2 + 4 + 2 + 2);
    CHECK(scenario.events[0].kind == Event::Kind::Dial);
    CHECK(scenario.events[0].name == "drive");
    CHECK(scenario.events[1].name == "mass");
    CHECK(scenario.events[2].kind == Event::Kind::Input);
    CHECK(scenario.events[2].name == "clutch");
    // the key
    CHECK(scenario.events[3].kind == Event::Kind::KeyDown);
    CHECK(scenario.events[3].value == 36.0);
    CHECK(scenario.events[3].velocity == 0.5);
    CHECK(scenario.events[4].kind == Event::Kind::KeyUp);
    // the ramp: from the throttle's last value (0) to 1 over 20 ms, a step every 5 ms, ending on the target at its end
    for (int k = 0; k < 4; ++k)
    {
        const Event& step = scenario.events[static_cast<std::size_t>(5 + k)];
        CHECK(step.kind == Event::Kind::Input);
        CHECK(step.name == "throttle");
        CHECK(step.time == Catch::Approx(1.0 + 0.005 * (k + 1)));
        CHECK(step.value == Catch::Approx(0.25 * (k + 1)));
    }
    // the buttons, by their bits
    CHECK(scenario.events[9].kind == Event::Kind::Press);
    CHECK(scenario.events[9].bits == ((1 << 17) | (1 << 5)));
    CHECK(scenario.events[10].kind == Event::Kind::Release);
    CHECK(scenario.events[10].bits == (1 << 17));
    // the tap: down, and up 0.2 s later
    CHECK(scenario.events[11].kind == Event::Kind::Press);
    CHECK(scenario.events[11].bits == (1 << 4));
    CHECK(scenario.events[12].kind == Event::Kind::Release);
    CHECK(scenario.events[12].time == Catch::Approx(2.2));

    CHECK(buttonBit("paddleUp") == 16);
    CHECK(buttonBit("paddleDown") == 32);
    CHECK(buttonBit("nothing") == 0);

    // what a scenario cannot say
    const auto bad = [&](const char* events) {
        const std::string json = std::string(R"({ "format": "tee-ref-scenario/1", "seconds": 1, "events": [)") + events + "] }";
        return !parseScenario(json, "bad.json", scenario, error);
    };
    CHECK(bad(R"({ "dials": { "size": 2 } })"));                       // no time
    CHECK(bad(R"({ "t": 0, "input": { "wings": 1 } })"));              // no such input
    CHECK(bad(R"({ "t": 0, "press": ["horn"] })"));                    // no such button
    CHECK(bad(R"({ "t": 0, "ramp": 1 })"));                            // a ramp of nothing
    CHECK(bad(R"({ "t": 0, "thrtle": 1 })"));                          // a misspelt key
    CHECK(bad(R"({ "t": -1, "keyUp": true })"));
    CHECK_FALSE(parseScenario(R"({ "format": "tee-ref-scenario/1", "events": [] })", "bad.json", scenario, error));
}

TEST_CASE("a WAV of floats is written and read back sample for sample", "[formats][wav]")
{
    Audio audio;
    audio.sampleRate = 44100.0;
    for (int i = 0; i < 1000; ++i)
    {
        audio.left.push_back(std::sin(0.01f * static_cast<float>(i)));
        audio.right.push_back(-0.5f * std::cos(0.02f * static_cast<float>(i)));
    }
    const std::filesystem::path file = std::filesystem::path(TEE_SOURCE_DIR) / "out" / "tests" / "roundtrip.wav";
    std::string error;
    REQUIRE(writeWav(file, audio, error));
    CHECK(std::filesystem::file_size(file) == 12 + 24 + 12 + 8 + 8000);
    Audio back;
    REQUIRE(readWav(file, back, error));
    CHECK(back.sampleRate == 44100.0);
    CHECK(back.left == audio.left);
    CHECK(back.right == audio.right);
    CHECK_FALSE(readWav(file.parent_path() / "missing.wav", back, error));
}

TEST_CASE("nullDepth is the difference in dB under the signal", "[formats][measures]")
{
    std::vector<float> a(48000), b(48000), c(48000);
    for (std::size_t i = 0; i < a.size(); ++i)
    {
        a[i] = 0.5f * static_cast<float>(std::sin(2.0 * kPi * 440.0 * static_cast<double>(i) / 48000.0));
        b[i] = a[i] * 1.001f;                       // a thousandth off: 60 dB under
        c[i] = -a[i];                               // inverted: the difference is twice the signal, 6 dB over
    }
    CHECK(nullDepth(a, a) == std::numeric_limits<double>::infinity());
    CHECK(nullDepth(a, b) == Catch::Approx(60.0).margin(0.1));
    CHECK(nullDepth(a, c) == Catch::Approx(-6.02).margin(0.01));
    CHECK(nullDepth(std::vector<float>(100, 0.0f), a) == -std::numeric_limits<double>::infinity());
    CHECK(maxDifference(a, b) == Catch::Approx(0.0005).epsilon(0.01));
    CHECK(levelDb(a, 0, a.size()) == Catch::Approx(-9.03).margin(0.01));      // a sine of 0.5: -6 dB peak, -9 dB RMS
    CHECK(peak(a) == Catch::Approx(0.5).epsilon(0.001));

    // the spectrum finds the line, and resampling keeps it and its level
    const Line line = strongestLine(powerSpectrum(a, 0, 32768), 48000.0, 20.0, 2000.0);
    CHECK(line.hz == Catch::Approx(440.0).margin(0.2));
    const std::vector<float> down = resample(a, 48000.0, 44100.0);
    CHECK(down.size() == 44100);
    CHECK(strongestLine(powerSpectrum(down, 0, 32768), 44100.0, 20.0, 2000.0).hz == Catch::Approx(440.0).margin(0.2));
    CHECK(levelDb(down, 4410, 39690) == Catch::Approx(-9.03).margin(0.02));
    const std::vector<float> back = resample(down, 44100.0, 48000.0);
    const std::vector<float> middleA(a.begin() + 4800, a.begin() + 43200);
    const std::vector<float> middleBack(back.begin() + 4800, back.begin() + 43200);
    CHECK(nullDepth(middleA, middleBack) > 80.0);
}
