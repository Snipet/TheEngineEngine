// tests/host/Alloc.cpp: operator new counted on the threads that ask (Support.h's AllocationCounter): the proof that
// nothing on the audio thread allocates. Replaces the global operator new and delete for this test executable, on
// Linux and macOS only: not on Windows (replaced operators in an executable pair badly with the C runtime's and with
// what LLVM allocates in its own code: tee-host-tests died of heap corruption, 0xc0000374, before its first test), and
// not under ThreadSanitizer, which replaces them itself.
#include "Support.h"

#if !defined(__has_feature)
#define TEE_HAS_FEATURE(x) 0
#else
#define TEE_HAS_FEATURE(x) __has_feature(x)
#endif
#if defined(_WIN32) || TEE_HAS_FEATURE(thread_sanitizer) || defined(__SANITIZE_THREAD__)
#define TEE_COUNT_ALLOCATIONS 0
#else
#define TEE_COUNT_ALLOCATIONS 1
#endif

#include <cstdlib>
#include <new>

#if TEE_COUNT_ALLOCATIONS
namespace
{
    thread_local bool tCounting = false;
    std::atomic<long> gCount{0};

    void freeAligned(void* p) noexcept
    {
#if defined(_WIN32)
        _aligned_free(p);
#else
        std::free(p);
#endif
    }

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
#if defined(_WIN32)
            p = _aligned_malloc(n, alignment);   // the Windows C runtime has no aligned_alloc
#else
            p = std::aligned_alloc(alignment, n);
#endif
        }
        else
            p = std::malloc(n);
        if (p == nullptr)
            throw std::bad_alloc();
        return p;
    }
} // namespace

#endif

namespace tee::test
{
#if TEE_COUNT_ALLOCATIONS
    void AllocationCounter::countThisThread(bool on) noexcept
    {
        tCounting = on;
    }

    long AllocationCounter::count() noexcept
    {
        return gCount.load();
    }

    bool AllocationCounter::available() noexcept
    {
        return true;
    }
#else
    void AllocationCounter::countThisThread(bool) noexcept {}

    long AllocationCounter::count() noexcept
    {
        return 0;
    }

    bool AllocationCounter::available() noexcept
    {
        return false;
    }
#endif
} // namespace tee::test

#if TEE_COUNT_ALLOCATIONS
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
    freeAligned(p);
}
void operator delete[](void* p, std::align_val_t) noexcept
{
    freeAligned(p);
}
void operator delete(void* p, std::size_t, std::align_val_t) noexcept
{
    freeAligned(p);
}
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept
{
    freeAligned(p);
}
#endif
