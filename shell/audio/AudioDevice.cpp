// shell/audio/AudioDevice.cpp: AudioDevice.h, on SDL3's audio streams.
#include "audio/AudioDevice.h"

#include "audio/Ftz.h"

#include <SDL3/SDL.h>

#include <algorithm>
#include <cstdio>
#include <string>

namespace tee::audio
{
    namespace
    {
        constexpr int kChannels = 2;
        constexpr int kFrameBytes = kChannels * static_cast<int>(sizeof(float));

        std::string sdlError()
        {
            const char* e = SDL_GetError();
            return e != nullptr && *e != '\0' ? std::string(e) : std::string("no reason given");
        }

        bool silentDriver(const std::string& driver)
        {
            return driver == "dummy" || driver == "disk";
        }
    }

    std::string DeviceInfo::line() const
    {
        if (!open)
            return "no audio device";
        char text[320];
        std::snprintf(text, sizeof text, "%s%s: %s, %d Hz, buffer %d frames (%.1f ms)", driver.c_str(),
                      silent ? " (silent)" : "", name.empty() ? "default device" : name.c_str(), rate, bufferFrames,
                      bufferMs);
        return text;
    }

    Device::Device(AudioSource& source) : source_(source) {}

    Device::~Device()
    {
        close();
        if (audioInit_)
            SDL_QuitSubSystem(SDL_INIT_AUDIO);
    }

    bool Device::open(const Request& request, std::string& why)
    {
        close();
        request_ = request;
        if (!audioInit_)
        {
            if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
            {
                why = "SDL's audio did not start: " + sdlError();
                if (!request.dummyFallback)
                    return false;
                // no audio driver at all: SDL's dummy one, so the application runs, silent
                SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "dummy", SDL_HINT_OVERRIDE);
                if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
                {
                    why += "; nor its dummy driver: " + sdlError();
                    return false;
                }
            }
            audioInit_ = true;
        }
        if (openStream(why))
            return true;
        const char* driver = SDL_GetCurrentAudioDriver();
        if (!request.dummyFallback || (driver != nullptr && std::string(driver) == "dummy"))
            return false;
        // a driver with no device that opens (a machine with no sound card): the dummy driver
        const std::string first = why;
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        audioInit_ = false;
        SDL_SetHintWithPriority(SDL_HINT_AUDIO_DRIVER, "dummy", SDL_HINT_OVERRIDE);
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO))
        {
            why = first + "; nor SDL's dummy driver: " + sdlError();
            return false;
        }
        audioInit_ = true;
        if (!openStream(why))
        {
            why = first + "; nor on SDL's dummy driver: " + why;
            return false;
        }
        why = first;        // open, silent: the caller may say why there is no sound
        return true;
    }

    bool Device::openStream(std::string& why)
    {
        // the buffer asked for: a hint SDL reads when it opens the device
        const std::string frames = std::to_string(std::clamp(request_.bufferFrames, 16, 8192));
        SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES, frames.c_str());

        // the device's own rate, so that SDL resamples nothing
        SDL_AudioSpec device{};
        int deviceFrames = 0;
        int rate = request_.rate;
        if (rate <= 0)
            rate = SDL_GetAudioDeviceFormat(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &device, &deviceFrames) && device.freq > 0
                       ? device.freq : 48000;

        // the blocks the source is asked for: the device's buffer where it is small, never more than 1024 frames
        maxBlock_ = std::clamp(std::max(request_.bufferFrames, deviceFrames), 64, 1024);
        left_.assign(static_cast<std::size_t>(maxBlock_), 0.0f);
        right_.assign(static_cast<std::size_t>(maxBlock_), 0.0f);
        both_.assign(static_cast<std::size_t>(maxBlock_) * kChannels, 0.0f);
        source_.prepare(static_cast<double>(rate), maxBlock_);

        const SDL_AudioSpec spec{ SDL_AUDIO_F32, kChannels, rate };
        stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, &Device::callback, this);
        if (stream_ == nullptr)
        {
            why = "no audio device opened: " + sdlError();
            info_ = {};
            return false;
        }
        deviceId_ = SDL_GetAudioStreamDevice(stream_);
        info_.rate = rate;
        refreshInfo();
        if (!SDL_ResumeAudioStreamDevice(stream_))
        {
            why = "the audio device did not start: " + sdlError();
            close();
            return false;
        }
        lost_ = false;
        changed_ = true;
        return true;
    }

    void Device::refreshInfo()
    {
        info_.open = stream_ != nullptr;
        const char* driver = SDL_GetCurrentAudioDriver();
        info_.driver = driver != nullptr ? driver : "";
        info_.silent = silentDriver(info_.driver);
        const char* name = deviceId_ != 0 ? SDL_GetAudioDeviceName(deviceId_) : nullptr;
        info_.name = name != nullptr ? name : "";
        SDL_AudioSpec device{};
        int frames = 0;
        if (deviceId_ != 0 && SDL_GetAudioDeviceFormat(deviceId_, &device, &frames))
        {
            info_.deviceRate = device.freq;
            info_.deviceChannels = device.channels;
            info_.bufferFrames = frames;
            info_.bufferMs = device.freq > 0 ? 1000.0 * static_cast<double>(frames) / static_cast<double>(device.freq) : 0.0;
        }
    }

    void Device::close()
    {
        if (stream_ != nullptr)
        {
            SDL_DestroyAudioStream(stream_);        // closes the device it was opened with; the callback has returned
            stream_ = nullptr;
        }
        deviceId_ = 0;
        if (info_.open)
            changed_ = true;
        info_.open = false;
    }

    bool Device::handleEvent(const SDL_Event& e)
    {
        if (e.type == SDL_EVENT_AUDIO_DEVICE_REMOVED)
        {
            if (stream_ != nullptr && e.adevice.which == deviceId_)
            {
                // ours went, and SDL found no default to move the stream to: let it go, and look again (pump)
                close();
                lost_ = true;
                retryAt_ = SDL_GetTicks() + 1000;
            }
            return true;
        }
        if (e.type == SDL_EVENT_AUDIO_DEVICE_FORMAT_CHANGED)
        {
            if (stream_ != nullptr && e.adevice.which == deviceId_)
            {
                // the default device changed, or its format did: the stream keeps its rate and SDL converts
                refreshInfo();
                changed_ = true;
            }
            return true;
        }
        if (e.type == SDL_EVENT_AUDIO_DEVICE_ADDED)
        {
            if (lost_)
                retryAt_ = SDL_GetTicks();          // something to try at once
            return true;
        }
        return false;
    }

    bool Device::pump()
    {
        if (lost_ && stream_ == nullptr && SDL_GetTicks() >= retryAt_)
        {
            std::string why;
            if (!openStream(why))
                retryAt_ = SDL_GetTicks() + 1000;
        }
        const bool changed = changed_;
        changed_ = false;
        return changed;
    }

    void Device::callback(void* user, SDL_AudioStream* stream, int additional, int)
    {
        if (additional > 0)
            static_cast<Device*>(user)->render(stream, additional / kFrameBytes);
    }

    // The audio thread. Nothing here waits but SDL's own queue, which the stream's lock (held around this call)
    // already guards.
    void Device::render(SDL_AudioStream* stream, int frames) noexcept
    {
        const ScopedFtz ftz;
        if (!ftz_.load(std::memory_order_relaxed) && ScopedFtz::on())
            ftz_.store(true, std::memory_order_relaxed);
        callbacks_.fetch_add(1, std::memory_order_relaxed);
        if (frames > largest_.load(std::memory_order_relaxed))
            largest_.store(frames, std::memory_order_relaxed);
        int left = frames;
        while (left > 0)
        {
            const int n = std::min(left, maxBlock_);
            source_.process(left_.data(), right_.data(), n);
            for (int i = 0; i < n; ++i)
            {
                both_[static_cast<std::size_t>(2 * i)] = left_[static_cast<std::size_t>(i)];
                both_[static_cast<std::size_t>(2 * i + 1)] = right_[static_cast<std::size_t>(i)];
            }
            SDL_PutAudioStreamData(stream, both_.data(), n * kFrameBytes);
            left -= n;
        }
        frames_.fetch_add(static_cast<std::uint64_t>(frames), std::memory_order_relaxed);
    }
}
