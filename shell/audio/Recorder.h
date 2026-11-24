// shell/audio/Recorder.h: the output written to a WAV file while it plays. The audio thread hands each block to a
// ring and goes on (no lock, no allocation, no waiting: when the ring is full the frames are dropped and counted); a
// worker thread of the Recorder's own takes them out and writes the file.
//
//   audio::Recorder recorder;
//   audio::RecordingSource tap(player, recorder);     // what the device plays: the player, heard by the recorder
//   audio::Device device(tap);
//   ...
//   recorder.start("take.wav", device.info().rate, why);
//   ...
//   recorder.stop();                                   // everything in the ring is written, the file closed
//
// The ring holds about 2.7 seconds at 48 kHz, so a worker held up by a slow disk for that long loses nothing.
#pragma once

#include "audio/Wav.h"
#include "tee/app/Sources.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>

namespace tee::audio
{
    class Recorder
    {
    public:
        Recorder();
        ~Recorder();                                    // stop()
        Recorder(const Recorder&) = delete;
        Recorder& operator=(const Recorder&) = delete;

        // Starts a file (its folder must exist) and the worker. False, and why, when the file cannot be made. A
        // recording under way is stopped first. Not the audio thread.
        bool start(const std::string& path, int rate, std::string& why);
        // Writes what is still in the ring, closes the file, ends the worker. False when the file is not whole. Not
        // the audio thread.
        bool stop();

        bool recording() const noexcept { return active_.load(std::memory_order_acquire); }

        // The audio thread: n frames of both channels, while recording() (nothing otherwise).
        void push(const float* l, const float* r, int n) noexcept;

        // Frames written to the file so far, and frames the ring had no room for.
        std::uint64_t written() const noexcept { return written_.load(std::memory_order_relaxed); }
        std::uint64_t dropped() const noexcept { return dropped_.load(std::memory_order_relaxed); }

    private:
        static constexpr std::size_t kFrames = std::size_t{ 1 } << 17;      // a power of two
        void work();
        void drain();

        std::unique_ptr<float[]> ring_;                 // kFrames stereo frames, interleaved
        std::atomic<std::uint64_t> head_{ 0 };          // frames pushed (the audio thread's)
        std::atomic<std::uint64_t> tail_{ 0 };          // frames taken (the worker's)
        std::atomic<bool> active_{ false };             // the audio thread pushes
        std::atomic<bool> quit_{ false };
        std::atomic<std::uint64_t> written_{ 0 }, dropped_{ 0 };
        std::thread worker_;
        WavWriter wav_;
        bool whole_ = true;
    };

    // An AudioSource that plays another and lets a Recorder hear it.
    class RecordingSource final : public AudioSource
    {
    public:
        RecordingSource(AudioSource& inner, Recorder& recorder) : inner_(inner), recorder_(recorder) {}

        void prepare(double rate, int maxBlock) override { inner_.prepare(rate, maxBlock); }
        void process(float* l, float* r, int n) noexcept override
        {
            inner_.process(l, r, n);
            recorder_.push(l, r, n);
        }

    private:
        AudioSource& inner_;
        Recorder& recorder_;
    };
}
