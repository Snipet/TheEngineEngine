// host/include/tee/Telemetry.h: what SimHost publishes after each block, and the seqlock it goes through.
//
// SeqBuffer is a seqlock over a fixed number of bytes: one writer (the audio thread, or whoever holds the writers'
// lock), any number of readers, nobody waits on anybody. The bytes are kept as relaxed atomic words, so a reader that
// races a write reads words of either version (and then sees the count move and tries again), never a torn word, and
// ThreadSanitizer has nothing to report. A writer never waits; a reader retries a few times and else says it found no
// whole frame (it keeps the one it had).
#pragma once

#include "tee/Fields.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace tee
{
    class SeqBuffer
    {
    public:
        explicit SeqBuffer(std::size_t bytes = 0) { resize(bytes); }

        // Not while anyone reads or writes.
        void resize(std::size_t bytes)
        {
            bytes_ = bytes;
            words_ = (bytes + 7) / 8;
            data_ = std::make_unique<std::atomic<std::uint64_t>[]>(words_ == 0 ? 1 : words_);
            for (std::size_t i = 0; i < words_; ++i)
                data_[i].store(0, std::memory_order_relaxed);
            sequence_.store(0, std::memory_order_relaxed);
        }

        std::size_t size() const noexcept { return bytes_; }

        // The writer: no allocation, no lock, no wait.
        void write(const void* source, std::size_t bytes) noexcept
        {
            const std::uint64_t s = sequence_.load(std::memory_order_relaxed);
            sequence_.store(s + 1, std::memory_order_relaxed);
            std::atomic_thread_fence(std::memory_order_release);
            const unsigned char* in = static_cast<const unsigned char*>(source);
            const std::size_t n = bytes < bytes_ ? bytes : bytes_;
            for (std::size_t w = 0; w * 8 < n; ++w)
            {
                std::uint64_t word = 0;
                std::memcpy(&word, in + w * 8, n - w * 8 < 8 ? n - w * 8 : 8);
                data_[w].store(word, std::memory_order_relaxed);
            }
            sequence_.store(s + 2, std::memory_order_release);
        }

        // A reader: false when no whole frame could be read in `tries` (the writer was busy each time), or nothing
        // has been written yet. `destination` takes size() bytes.
        bool read(void* destination, int tries = 8) const noexcept
        {
            unsigned char* out = static_cast<unsigned char*>(destination);
            for (int t = 0; t < tries; ++t)
            {
                const std::uint64_t before = sequence_.load(std::memory_order_acquire);
                if (before == 0)
                    return false;
                if ((before & 1u) != 0)
                    continue;
                for (std::size_t w = 0; w * 8 < bytes_; ++w)
                {
                    const std::uint64_t word = data_[w].load(std::memory_order_relaxed);
                    std::memcpy(out + w * 8, &word, bytes_ - w * 8 < 8 ? bytes_ - w * 8 : 8);
                }
                std::atomic_thread_fence(std::memory_order_acquire);
                if (sequence_.load(std::memory_order_relaxed) == before)
                    return true;
            }
            return false;
        }

        // How many frames have been written.
        std::uint64_t written() const noexcept { return sequence_.load(std::memory_order_acquire) / 2; }

    private:
        std::size_t bytes_ = 0;
        std::size_t words_ = 0;
        std::unique_ptr<std::atomic<std::uint64_t>[]> data_;
        std::atomic<std::uint64_t> sequence_{0};
    };

    // The host's own numbers about a block, published with the telemetry.
    struct HostStats
    {
        std::uint64_t block = 0;          // blocks processed since load
        std::int64_t samples = 0;         // samples processed since load (the session's clock)
        double load = 0.0;                // the newest callback's time over its buffer's duration
        double loadMean = 0.0;            // that, averaged over about a second
        double loadPeak = 0.0;            // the largest in the last second or so
        double hostMicros = 0.0;          // the host's own work in the newest block, besides Engine::process
        double hostMicrosMean = 0.0;
        double hostMicrosPeak = 0.0;      // the largest since load
        float outputPeak = 0.0f;          // the newest block's largest sample, either channel
        std::uint32_t commandsApplied = 0;    // since load
        std::uint32_t commandsRefused = 0;    // set() and setBytes() that found the queue full (the caller retries)
        std::uint32_t guardTrips = 0;     // blocks the output guard refused
        std::uint32_t stops = 0;          // calls a check stopped (loop budget, bounds)
        std::uint32_t restarts = 0;       // instances made afresh after a trip or a stop
        std::uint32_t reloads = 0;        // programs swapped in by a reload
        std::uint8_t faulted = 0;         // silent now, waiting for a fresh instance
        std::uint8_t restarting = 0;      // a fresh instance after a fault, being cranked back to running
        std::uint8_t pad[6] = {};
        std::uint32_t clippedBlocks = 0;  // blocks with a sample beyond full scale, clamped (no fault)
        float largestSample = 0.0f;       // the largest sample the simulator wrote since load, before the clamp
        // Calls in which the simulator found its engine diverged (a crank not finite or past five times its redline,
        // an output not finite) and put it at rest afresh: tel.engine.flags' FLAG_FAULT (4), counted since load. The
        // host then cranks it back as after a restart. Never in normal running: a failure of the program's invariant.
        std::uint32_t engineFaults = 0;
    };

    // The newest whole block's telemetry: the `tel` block's bytes and the host's numbers. Read field by field.
    struct TelemetryFrame
    {
        HostStats host;
        std::vector<unsigned char> tel;   // the tel block's bytes, as the simulator left them
        std::size_t telOffset = 0;        // the tel block's place in the main module
        std::uint32_t generation = 0;     // the Fields these bytes are laid out by

        // A field (by its FieldInfo or a ref of the same generation) as a double; 0 when it is not in the tel block.
        double get(const FieldInfo& field, std::size_t index = 0) const noexcept;
        double get(const FieldRef& ref, std::size_t index = 0) const noexcept;
    };

    // The newest Scope the simulator filled while it was asked for.
    struct ScopeFrame
    {
        std::uint64_t block = 0;          // the block it was copied after
        std::vector<unsigned char> bytes; // the scope block's bytes
        std::size_t scopeOffset = 0;
        std::uint32_t generation = 0;

        double get(const FieldInfo& field, std::size_t index = 0) const noexcept;
    };

    // A number of `scalar` at `at`, as a double.
    double readScalar(const unsigned char* at, Scalar scalar) noexcept;
    // `value` written at `at` as `scalar` (a float rounded, an int truncated, a bool true unless 0).
    void writeScalar(unsigned char* at, Scalar scalar, double value) noexcept;
    std::size_t scalarBytes(Scalar scalar) noexcept;
} // namespace tee
