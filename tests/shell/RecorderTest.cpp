// tests/shell/RecorderTest.cpp: the recorder (shell/audio/Recorder.h): what an audio thread plays through a
// RecordingSource is in the file, sample for sample, and a ring that is full drops and counts, never waits.
#include "audio/Recorder.h"
#include "player/TonePlayer.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

using namespace tee;

namespace
{
    std::filesystem::path outFile(const char* name)
    {
        const std::filesystem::path dir = std::filesystem::path(TEE_SOURCE_DIR) / "out" / "shell";
        std::filesystem::create_directories(dir);
        return dir / name;
    }

    std::vector<float> samplesOf(const std::filesystem::path& path)
    {
        std::ifstream in(path, std::ios::binary);
        const std::vector<char> bytes{ std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };
        std::vector<float> samples(bytes.size() > 58 ? (bytes.size() - 58) / 4 : 0);
        if (!samples.empty())
            std::memcpy(samples.data(), bytes.data() + 58, samples.size() * 4);
        return samples;
    }
}

TEST_CASE("shell: the recorder writes what was played, sample for sample", "[shell][audio]")
{
    player::TonePlayer tone;
    tone.prepare(48000.0, 256);
    ControlsFrame frame;
    frame.ignition = 2;
    frame.throttle = 0.6f;
    tone.setControls(frame);

    audio::Recorder recorder;
    audio::RecordingSource tap(tone, recorder);
    const std::filesystem::path path = outFile("test-recorded.wav");
    std::string why;

    // before it starts nothing is kept
    std::vector<float> l(256), r(256);
    tap.process(l.data(), r.data(), 256);
    CHECK_FALSE(recorder.recording());

    REQUIRE(recorder.start(path.string(), 48000, why));
    CHECK(recorder.recording());
    // an "audio thread": 400 blocks of uneven sizes, a little apart, as a device's callbacks are
    std::vector<float> played;
    std::thread audio([&] {
        for (int b = 0; b < 400; ++b)
        {
            const int n = 64 + (b * 37) % 193;
            tap.process(l.data(), r.data(), n);
            for (int i = 0; i < n; ++i)
            {
                played.push_back(l[static_cast<std::size_t>(i)]);
                played.push_back(r[static_cast<std::size_t>(i)]);
            }
            if (b % 16 == 0)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    });
    audio.join();
    CHECK(recorder.stop());
    CHECK_FALSE(recorder.recording());
    CHECK(recorder.dropped() == 0);
    CHECK(recorder.written() == played.size() / 2);
    const std::vector<float> file = samplesOf(path);
    REQUIRE(file.size() == played.size());
    CHECK(file == played);
    bool heard = false;
    for (const float s : file)
        heard = heard || s != 0.0f;
    CHECK(heard);

    // after it stops nothing more is kept, and a second take starts a new file
    tap.process(l.data(), r.data(), 256);
    CHECK(recorder.written() == played.size() / 2);
    REQUIRE(recorder.start(path.string(), 48000, why));
    tap.process(l.data(), r.data(), 100);
    CHECK(recorder.stop());
    CHECK(samplesOf(path).size() == 200);

    CHECK_FALSE(recorder.start((outFile("no-such-folder") / "x.wav").string(), 48000, why));
    CHECK_FALSE(recorder.recording());
}
