// shell/audio/AudioDevice.h: the sound card. An SDL3 playback stream on the system's default device, stereo floats
// at the device's own rate, with a callback that asks the AudioSource for the samples the device wants and does
// nothing else that can wait. The seam between the application and the system's audio: if SDL's stream disappoints
// on some system (its latency on macOS is not measured), another implementation goes behind this header.
//
//   audio::Device device(player);
//   std::string why;
//   if (!device.open({}, why)) ...        // the source is prepared at the device's rate, the stream starts
//   ... each frame: device.handleEvent(e) for SDL's audio events, device.pump() ...
//   device.close();
//
// What it asks for and what it gets: a buffer near Request::bufferFrames (a hint SDL passes to the system; the device
// decides) and the device's own rate (so SDL resamples nothing). info() says what the device gave. SDL reports the
// device's buffer and nothing of the system's own output latency behind it, so info().bufferMs is the least the
// latency can be, not a measurement of it.
//
// The device that goes away. The stream is opened on the system's default device, which SDL follows by itself when the
// default changes (headphones plugged in): the stream keeps its rate and SDL converts. If the device is removed and
// nothing takes its place, pump() tries the default again once a second.
//
// With SDL_AUDIODRIVER=dummy (SDL's driver that plays to nowhere, in real time) all of this runs with no sound card:
// that is how it is tested. If no driver opens a device at all, open() falls back to the dummy driver unless
// Request::dummyFallback is false, so the application still runs, silent, and says so.
//
// Threads: every call is the main thread's. The callback runs on SDL's audio thread: it sets flush-to-zero, calls
// AudioSource::process in blocks of at most maxBlock(), and hands the samples to SDL.
#pragma once

#include "tee/app/Sources.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

struct SDL_AudioStream;
union SDL_Event;

namespace tee::audio
{
    struct Request
    {
        std::string deviceName;         // the device by its name as devices() lists it; empty: the system's default.
                                        // A name that is not there now: the default, and info().note says so
        int bufferFrames = 256;         // asked of the device; it gives what it gives
        int rate = 0;                   // 0: the device's own
        bool dummyFallback = true;      // no device opens: run on SDL's dummy driver, silent
    };

    struct DeviceInfo
    {
        bool open = false;
        bool silent = false;            // SDL's dummy or disk driver: nothing is heard
        std::string driver;             // SDL's audio driver: "wasapi", "coreaudio", "pulseaudio", "dummy"
        std::string name;               // the device's name as the system gives it
        int rate = 0;                   // the stream's rate, which the source was prepared at
        int deviceRate = 0;             // the device's own (differs only after the default device changed)
        int deviceChannels = 0;
        int bufferFrames = 0;           // the device's buffer, in frames at deviceRate
        double bufferMs = 0.0;          // that buffer as time: the least the output latency can be
        bool isDefault = true;          // opened as the system's default (it follows the default when that changes)
        std::string note;               // not empty: the named device was not there, and the default was opened
        std::string line() const;       // all of it in a line, for a person
    };

    class Device
    {
    public:
        explicit Device(AudioSource&);
        ~Device();
        Device(const Device&) = delete;
        Device& operator=(const Device&) = delete;

        // SDL's audio subsystem, the default device, the stream; prepares the source and starts. False and why.
        bool open(const Request&, std::string& why);
        void close();
        bool isOpen() const noexcept { return stream_ != nullptr; }
        // The playback devices' names now (SDL's audio is started for it if it was not), for a person to pick from.
        std::vector<std::string> devices();
        const DeviceInfo& info() const noexcept { return info_; }

        // An SDL event: true when it was an audio device event this took (the device gone, its format changed).
        bool handleEvent(const SDL_Event&);
        // Now and then (once a frame is fine): reopens the default device after ours went away. True when info()
        // changed since the last call.
        bool pump();

        // The largest block the source is asked for.
        int maxBlock() const noexcept { return maxBlock_; }

        // What the callback has done, for tests and --info.
        std::uint64_t callbacks() const noexcept { return callbacks_.load(std::memory_order_relaxed); }
        std::uint64_t frames() const noexcept { return frames_.load(std::memory_order_relaxed); }
        int largestRequest() const noexcept { return largest_.load(std::memory_order_relaxed); }
        bool flushToZeroSeen() const noexcept { return ftz_.load(std::memory_order_relaxed); }

    private:
        bool openStream(std::string& why);
        void refreshInfo();
        static void callback(void* user, SDL_AudioStream* stream, int additional, int total);
        void render(SDL_AudioStream* stream, int frames) noexcept;

        AudioSource& source_;
        Request request_{};
        SDL_AudioStream* stream_ = nullptr;
        std::uint32_t deviceId_ = 0;
        DeviceInfo info_{};
        bool audioInit_ = false;
        bool lost_ = false;                 // the device went away: pump() tries again
        bool changed_ = false;
        std::uint64_t retryAt_ = 0;         // SDL's milliseconds
        int maxBlock_ = 1024;
        std::vector<float> left_, right_, both_;
        std::atomic<std::uint64_t> callbacks_{ 0 }, frames_{ 0 };
        std::atomic<int> largest_{ 0 };
        std::atomic<bool> ftz_{ false };
    };
}
