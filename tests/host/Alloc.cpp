// tests/host/Alloc.cpp: operator new counted on the threads that ask (Support.h's AllocationCounter): the proof that
// nothing on the audio thread allocates. Replaces the global operator new and delete for this test executable.
#include "Support.h"

#include <cstdlib>
#include <new>

namespace
{
    thread_local bool tCounting = false;
    std::atomic<long> gCount{0};

    void* allocate(std::size_t n, std::size_t alignment)
    {
        if (tCounting)
            gCount.fetch_add(1, std::memory_order_relaxed);
        if (n == 0)
            n = 1;
        void* p = nullptr;
        if (alignment > alignof(std::max_align_t))
        {
            n = (n + alignment - 1) / alignment * alignment;
            p = std::aligned_alloc(alignment, n);
        }
        else
            p = std::malloc(n);
        if (p == nullptr)
            throw std::bad_alloc();
        return p;
    }
} // namespace

namespace tee::test
{
    void AllocationCounter::countThisThread(bool on) noexcept
    {
        tCounting = on;
    }

    long AllocationCounter::count() noexcept
    {
        return gCount.load();
    }
} // namespace tee::test

void* operator new(std::size_t n)
{
    return allocate(n, 0);
}
void* operator new[](std::size_t n)
{
    return allocate(n, 0);
}
void* operator new(std::size_t n, std::align_val_t a)
{
    return allocate(n, static_cast<std::size_t>(a));
}
void* operator new[](std::size_t n, std::align_val_t a)
{
    return allocate(n, static_cast<std::size_t>(a));
}
void* operator new(std::size_t n, const std::nothrow_t&) noexcept
{
    try
    {
        return allocate(n, 0);
    }
    catch (...)
    {
        return nullptr;
    }
}
void* operator new[](std::size_t n, const std::nothrow_t&) noexcept
{
    try
    {
        return allocate(n, 0);
    }
    catch (...)
    {
        return nullptr;
    }
}
void operator delete(void* p) noexcept
{
    std::free(p);
}
void operator delete[](void* p) noexcept
{
    std::free(p);
}
void operator delete(void* p, std::size_t) noexcept
{
    std::free(p);
}
void operator delete[](void* p, std::size_t) noexcept
{
    std::free(p);
}
void operator delete(void* p, std::align_val_t) noexcept
{
    std::free(p);
}
void operator delete[](void* p, std::align_val_t) noexcept
{
    std::free(p);
}
void operator delete(void* p, std::size_t, std::align_val_t) noexcept
{
    std::free(p);
}
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept
{
    std::free(p);
}
