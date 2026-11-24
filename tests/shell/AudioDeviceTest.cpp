// tests/shell/AudioDeviceTest.cpp: the audio device (shell/audio/AudioDevice.h) on SDL's dummy driver, which plays to
// nowhere in real time: the only driver a machine with no sound card has, and the same code path as a real one from
// the stream up. What it cannot say is anything about a real device's buffer or latency.
#include "audio/AudioDevice.h"
#include "player/TonePlayer.h"

#include <SDL3/SDL.h>

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <string>

using namespace tee;

TEST_CASE("shell: the audio device runs its callback on SDL's dummy driver", "[shell][audio]")
{
    SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "dummy", SDL_HINT_OVERRIDE);
    player::TonePlayer tone;
    ControlsFrame frame;
    frame.ignition = 2;
    frame.throttle = 0.5f;
    tone.setControls(frame);

    audio::Device device(tone);
    std::string why;
    audio::Request request;
    request.bufferFrames = 256;
    REQUIRE(device.open(request, why));
    INFO(why);
    const audio::DeviceInfo info = device.info();
    CHECK(info.open);
    CHECK(info.driver == "dummy");
    CHECK(info.silent);
    CHECK(info.rate > 0);
    CHECK(info.bufferFrames > 0);
    CHECK(info.bufferMs > 0.0);
    std::printf("audio device under SDL's dummy driver: %s (no real device: this says nothing about one)\n",
                info.line().c_str());

    // real time: half a second of callbacks
    SDL_Delay(500);
    SDL_Event e;
    while (SDL_PollEvent(&e))
        device.handleEvent(e);
    device.pump();
    CHECK(device.callbacks() > 2);
    CHECK(device.frames() > static_cast<std::uint64_t>(info.rate) / 10);        // at least a tenth of a second's
    CHECK(device.frames() < static_cast<std::uint64_t>(info.rate) * 2);
    CHECK(device.flushToZeroSeen());
    CHECK(tone.blocks() > 2);
    CHECK(tone.largestBlock() <= device.maxBlock());
    CHECK(tone.outputPeak() > 0.0f);                        // the source was asked, and played

    // closed, the callback stops
    device.close();
    CHECK_FALSE(device.isOpen());
    const long long blocks = tone.blocks();
    SDL_Delay(100);
    CHECK(tone.blocks() == blocks);

    // and it opens again
    REQUIRE(device.open(request, why));
    SDL_Delay(100);
    CHECK(tone.blocks() > blocks);
    device.close();
}
