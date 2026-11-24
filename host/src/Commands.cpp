// host/src/Commands.cpp: tee::CommandQueue (Commands.h).
#include "tee/Commands.h"

namespace tee
{
    CommandQueue::CommandQueue(std::size_t slots, std::size_t poolBytes)
        : slots_(slots < 2 ? 2 : slots), poolBytes_(poolBytes < 64 ? 64 : poolBytes),
          ring_(std::make_unique<Command[]>(slots_)), pool_(std::make_unique<unsigned char[]>(poolBytes_))
    {
    }

    bool CommandQueue::push(std::uint32_t offset, const void* data, std::uint32_t size, std::int16_t section,
                            std::uint32_t generation) noexcept
    {
        const std::uint64_t head = head_.load(std::memory_order_relaxed);
        if (head - tail_.load(std::memory_order_acquire) >= slots_)
            return false;
        Command& c = ring_[head % slots_];
        c.offset = offset;
        c.size = size;
        c.generation = generation;
        c.section = section;
        if (size <= kInline)
        {
            c.pooled = 0;
            std::memcpy(c.bytes, data, size);
        }
        else
        {
            if (size > poolBytes_)
                return false;
            // a contiguous run: past the pool's end, start again at its beginning (the bytes skipped are freed with
            // this command)
            std::uint64_t start = poolHead_;
            const std::size_t at = static_cast<std::size_t>(start % poolBytes_);
            if (at + size > poolBytes_)
                start += poolBytes_ - at;
            const std::uint64_t end = start + size;
            if (end - poolTail_.load(std::memory_order_acquire) > poolBytes_)
                return false;
            std::memcpy(pool_.get() + (start % poolBytes_), data, size);
            c.pooled = 1;
            c.poolStart = start;
            c.poolEnd = end;
            poolHead_ = end;
        }
        head_.store(head + 1, std::memory_order_release);
        return true;
    }
} // namespace tee
