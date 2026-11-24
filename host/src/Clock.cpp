// host/src/Clock.cpp: tee::threadCpuSeconds (Clock.h).
#include "tee/Clock.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <ctime>
#endif

namespace tee
{
    double threadCpuSeconds() noexcept
    {
#if defined(_WIN32)
        FILETIME created, exited, kernel, user;
        if (GetThreadTimes(GetCurrentThread(), &created, &exited, &kernel, &user) == 0)
            return 0.0;
        const auto ticks = [](const FILETIME& t) {
            return static_cast<double>((static_cast<unsigned long long>(t.dwHighDateTime) << 32) | t.dwLowDateTime);
        };
        return (ticks(kernel) + ticks(user)) * 1.0e-7;   // FILETIME counts 100 ns
#else
        timespec now{};
        if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &now) != 0)
            return 0.0;
        return static_cast<double>(now.tv_sec) + 1.0e-9 * static_cast<double>(now.tv_nsec);
#endif
    }
} // namespace tee
