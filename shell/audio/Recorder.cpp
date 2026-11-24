// shell/audio/Recorder.cpp: Recorder.h. One producer (the audio thread) and one consumer (the worker): head_ is
// written only by the first and tail_ only by the second, so the ring needs no lock.
#include "audio/Recorder.h"

#include <algorithm>
#include <chrono>
#include <vector>

namespace tee::audio
{
    Recorder::Recorder() : ring_(std::make_unique<float[]>(kFrames * 2)) {}

    Recorder::~Recorder()
    {
        stop();
    }

    bool Recorder::start(const std::string& path, int rate, std::string& why)
    {
        stop();
        if (!wav_.open(path, rate, why))
            return false;
        whole_ = true;
        written_.store(0, std::memory_order_relaxed);
        dropped_.store(0, std::memory_order_relaxed);
        // an empty ring: nothing pushes while active_ is false
        tail_.store(head_.load(std::memory_order_acquire), std::memory_order_release);
        quit_.store(false, std::memory_order_release);
        worker_ = std::thread([this] { work(); });
        active_.store(true, std::memory_order_release);
        return true;
    }

    bool Recorder::stop()
    {
        if (!worker_.joinable())
            return whole_;
        active_.store(false, std::memory_order_release);    // a block the audio thread is pushing now still lands:
        quit_.store(true, std::memory_order_release);       // the worker drains once more after it sees quit
        worker_.join();
        whole_ = wav_.close() && whole_;
        return whole_;
    }

    void Recorder::push(const float* l, const float* r, int n) noexcept
    {
        if (n <= 0 || !active_.load(std::memory_order_acquire))
            return;
        const std::uint64_t head = head_.load(std::memory_order_relaxed);
        const std::uint64_t used = head - tail_.load(std::memory_order_acquire);
        const std::uint64_t room = kFrames - used;
        const auto take = static_cast<std::uint64_t>(std::min<std::uint64_t>(static_cast<std::uint64_t>(n), room));
        for (std::uint64_t i = 0; i < take; ++i)
        {
            const std::size_t at = static_cast<std::size_t>((head + i) & (kFrames - 1)) * 2;
            ring_[at] = l[i];
            ring_[at + 1] = r[i];
        }
        head_.store(head + take, std::memory_order_release);
        if (take < static_cast<std::uint64_t>(n))
            dropped_.fetch_add(static_cast<std::uint64_t>(n) - take, std::memory_order_relaxed);
    }

    void Recorder::drain()
    {
        float left[1024], right[1024];
        for (;;)
        {
            const std::uint64_t tail = tail_.load(std::memory_order_relaxed);
            const std::uint64_t ready = head_.load(std::memory_order_acquire) - tail;
            if (ready == 0)
                return;
            const int n = static_cast<int>(std::min<std::uint64_t>(ready, 1024));
            for (int i = 0; i < n; ++i)
            {
                const std::size_t at = static_cast<std::size_t>((tail + static_cast<std::uint64_t>(i)) & (kFrames - 1)) * 2;
                left[i] = ring_[at];
                right[i] = ring_[at + 1];
            }
            whole_ = wav_.write(left, right, n) && whole_;
            tail_.store(tail + static_cast<std::uint64_t>(n), std::memory_order_release);
            written_.fetch_add(static_cast<std::uint64_t>(n), std::memory_order_relaxed);
        }
    }

    void Recorder::work()
    {
        while (!quit_.load(std::memory_order_acquire))
        {
            drain();
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }
        // once more, after the last block the audio thread could still have been pushing when it was told to stop
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        drain();
    }
}
