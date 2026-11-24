// host/include/tee/Commands.h: the writes the host sends the audio thread: a single-producer single-consumer ring of
// commands, each an offset in the main module and its bytes (inline up to 16; past that a run of a preallocated pool,
// for an array, a curve or a map).
//
// The producer side is any thread but the audio thread; SimHost serialises producers with a lock of their own, which
// the audio thread never takes. The consumer (the audio thread) drains it with no allocation, no lock and no wait. A
// command is applied whole or not at all, and commands are applied in the order they were pushed: a table is never
// seen half written, since the simulator runs only between two drains. A full ring (or pool) refuses the push: the
// caller tries again later (Build does, on its next flush), and the refusal is counted.
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

namespace tee
{
    class CommandQueue
    {
    public:
        static constexpr std::size_t kInline = 16;

        struct Command
        {
            std::uint32_t offset = 0;       // in the main module
            std::uint32_t size = 0;         // bytes
            std::uint32_t generation = 0;   // the Fields the offset is of
            std::int16_t section = -1;      // whose rev to bump, or -1
            std::uint8_t pooled = 0;        // the bytes are in the pool, not inline
            std::uint8_t pad = 0;
            unsigned char bytes[kInline] = {};
            std::uint64_t poolStart = 0;    // a pooled command's bytes: [poolStart, poolStart + size) of the pool's
            std::uint64_t poolEnd = 0;      // counter; the pool is free up to poolEnd once it is applied
        };

        CommandQueue(std::size_t slots, std::size_t poolBytes);

        // The producer: false when the ring or the pool has no room.
        bool push(std::uint32_t offset, const void* data, std::uint32_t size, std::int16_t section,
                  std::uint32_t generation) noexcept;

        // The consumer: calls apply(command, bytes) for each command waiting, in order, and frees each after it.
        // apply returns false to leave that command (and every one after it) waiting for the next drain. Returns how
        // many it took. At most `limit` (so one drain never runs away).
        template <typename Apply>
        std::size_t drain(Apply&& apply, std::size_t limit = static_cast<std::size_t>(-1)) noexcept
        {
            std::size_t done = 0;
            std::uint64_t tail = tail_.load(std::memory_order_relaxed);
            const std::uint64_t head = head_.load(std::memory_order_acquire);
            std::uint64_t poolFree = poolTail_.load(std::memory_order_relaxed);
            while (tail != head && done < limit)
            {
                const Command& c = ring_[tail % slots_];
                const unsigned char* bytes = c.pooled != 0 ? pool_.get() + (c.poolStart % poolBytes_) : c.bytes;
                if (!apply(c, bytes))
                    break;
                if (c.pooled != 0)
                    poolFree = c.poolEnd;
                ++tail;
                ++done;
            }
            poolTail_.store(poolFree, std::memory_order_release);
            tail_.store(tail, std::memory_order_release);
            return done;
        }

        std::size_t waiting() const noexcept
        {
            return static_cast<std::size_t>(head_.load(std::memory_order_acquire) - tail_.load(std::memory_order_acquire));
        }
        std::size_t slots() const noexcept { return slots_; }
        std::size_t poolBytes() const noexcept { return poolBytes_; }

    private:
        std::size_t slots_;
        std::size_t poolBytes_;
        std::unique_ptr<Command[]> ring_;
        std::unique_ptr<unsigned char[]> pool_;
        std::atomic<std::uint64_t> head_{0};       // the producer's
        std::atomic<std::uint64_t> tail_{0};       // the consumer's
        std::uint64_t poolHead_ = 0;               // the producer's own
        std::atomic<std::uint64_t> poolTail_{0};   // the consumer's: freed up to here
    };
} // namespace tee
