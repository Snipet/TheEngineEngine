// host/include/tee/Clock.h: the processor time a thread has used, for measuring what code costs on a machine that is
// doing other things too. Wall time counts the moments the thread was set aside for another process; this does not.
#pragma once

namespace tee
{
    // Seconds of processor time the calling thread has used so far (user and system). The difference of two calls
    // is what the code between them cost. Fine to nanoseconds on Linux and macOS; on Windows it advances in steps of
    // the scheduler's tick (some 16 ms), so only sums over many calls mean anything there.
    double threadCpuSeconds() noexcept;
} // namespace tee
