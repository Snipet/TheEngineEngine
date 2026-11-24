// host/include/tee/Denormals.h: flush-to-zero for a scope, the floating-point mode FMinus compiles the simulator for
// (fminus::Program::Denormals::Flush): a subnormal reads as 0 and a subnormal result is 0.
//
// A program's entry points (its [[clang::nonblocking]] functions that nothing in the program calls) set the mode
// themselves and hand the caller's back. Everything else a host calls, a module's init and end, runs in the caller's
// mode, and the code assumes this one: wrap those calls.
//
//   {
//       const tee::ScopedFlushDenormals flush;
//       init(shared, 48000.0f);
//   }
//
// An audio callback may hold one around its whole block too: an entry point that finds the mode set writes nothing.
// x86-64 (MXCSR's FTZ and DAZ) and arm64 (FPCR's FZ); on any other target it does nothing.
#pragma once

#include <cstdint>

#if defined(__x86_64__) || defined(_M_X64)
#include <xmmintrin.h>
#endif

namespace tee
{
    class ScopedFlushDenormals
    {
    public:
        ScopedFlushDenormals() noexcept
        {
#if defined(__x86_64__) || defined(_M_X64)
            saved_ = _mm_getcsr();
            _mm_setcsr(static_cast<unsigned>(saved_) | 0x8040u);   // FTZ (bit 15) and DAZ (bit 6)
#elif defined(__aarch64__)
            std::uint64_t fpcr = 0;
            __asm__ volatile("mrs %0, fpcr" : "=r"(fpcr));
            saved_ = fpcr;
            fpcr |= std::uint64_t{1} << 24;                        // FZ
            __asm__ volatile("msr fpcr, %0" : : "r"(fpcr));
#endif
        }

        ~ScopedFlushDenormals() noexcept
        {
#if defined(__x86_64__) || defined(_M_X64)
            _mm_setcsr(static_cast<unsigned>(saved_));
#elif defined(__aarch64__)
            const std::uint64_t fpcr = saved_;
            __asm__ volatile("msr fpcr, %0" : : "r"(fpcr));
#endif
        }

        ScopedFlushDenormals(const ScopedFlushDenormals&) = delete;
        ScopedFlushDenormals& operator=(const ScopedFlushDenormals&) = delete;

    private:
        std::uint64_t saved_ = 0;
    };
} // namespace tee
