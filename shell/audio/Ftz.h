// shell/audio/Ftz.h: flush-to-zero for the thread that renders. The simulator is compiled for it (FMinus's
// Denormals::Flush): run in that mode already, its entry point writes no control register, and a subnormal never
// costs a slow path. Set once at the top of each audio callback (a read and, the first time on a thread, a write);
// the scope restores what it found, for a caller that renders on a thread of its own.
#pragma once

#include <cstdint>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
 #include <xmmintrin.h>
 #define TEE_FTZ_X86 1
#elif defined(__aarch64__) || defined(_M_ARM64)
 #define TEE_FTZ_ARM64 1
#endif

namespace tee::audio
{
    class ScopedFtz
    {
    public:
        ScopedFtz() noexcept
        {
           #if defined(TEE_FTZ_X86)
            old_ = _mm_getcsr();
            const unsigned want = old_ | 0x8040u;               // FTZ (bit 15) and DAZ (bit 6)
            if (want != old_)
                _mm_setcsr(want);
           #elif defined(TEE_FTZ_ARM64) && (defined(__clang__) || defined(__GNUC__))
            std::uint64_t fpcr = 0;
            __asm__ __volatile__("mrs %0, fpcr" : "=r"(fpcr));
            old_ = fpcr;
            const std::uint64_t want = fpcr | (std::uint64_t{ 1 } << 24);   // FZ
            if (want != fpcr)
                __asm__ __volatile__("msr fpcr, %0" : : "r"(want));
           #endif
        }

        ~ScopedFtz() noexcept
        {
           #if defined(TEE_FTZ_X86)
            if (_mm_getcsr() != old_)
                _mm_setcsr(old_);
           #elif defined(TEE_FTZ_ARM64) && (defined(__clang__) || defined(__GNUC__))
            std::uint64_t fpcr = 0;
            __asm__ __volatile__("mrs %0, fpcr" : "=r"(fpcr));
            if (fpcr != old_)
                __asm__ __volatile__("msr fpcr, %0" : : "r"(old_));
           #endif
        }

        ScopedFtz(const ScopedFtz&) = delete;
        ScopedFtz& operator=(const ScopedFtz&) = delete;

        // Whether this thread flushes subnormals to zero now.
        static bool on() noexcept
        {
           #if defined(TEE_FTZ_X86)
            return (_mm_getcsr() & 0x8040u) == 0x8040u;
           #elif defined(TEE_FTZ_ARM64) && (defined(__clang__) || defined(__GNUC__))
            std::uint64_t fpcr = 0;
            __asm__ __volatile__("mrs %0, fpcr" : "=r"(fpcr));
            return (fpcr & (std::uint64_t{ 1 } << 24)) != 0;
           #else
            return false;
           #endif
        }

    private:
       #if defined(TEE_FTZ_X86)
        unsigned old_ = 0;
       #else
        std::uint64_t old_ = 0;
       #endif
    };
}
