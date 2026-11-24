// tools/common/Sampler.h: a sampling profiler for the thread that renders, small enough to read. Another thread
// interrupts the rendering thread with a signal every quarter of a millisecond, and the handler stores the program
// counter the signal interrupted, read from the context the kernel saved: it only stores, into memory taken before,
// with no allocation and no lock. Afterwards each sample is looked up in the program's code ranges
// (fminus::Program::codeRanges), which says whose function's code it was in. FMinus's tests/library/ProfileTest.cpp
// is the reference this follows.
//
// Linux and macOS (POSIX signals). On Windows available() is false and start() does nothing: a sampler there suspends
// the thread and reads its context, which nobody has written here yet.
#pragma once

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <thread>
#include <vector>

#if !defined(_WIN32)
#include <csignal>
#include <pthread.h>
#if defined(__APPLE__)
#include <sys/ucontext.h>
#else
#include <ucontext.h>
#endif
#endif

namespace tee::tools
{
    class Sampler
    {
    public:
        static bool available() noexcept
        {
#if defined(_WIN32)
            return false;
#else
            return true;
#endif
        }

        // Samples the calling thread from now until stop(): at most `capacity` samples, one every `microseconds`.
        void start(std::size_t capacity = std::size_t{1} << 20, int microseconds = 250)
        {
#if !defined(_WIN32)
            store().assign(capacity, 0);
            taken().store(0);
            struct sigaction action = {};
            action.sa_sigaction = onSample;
            action.sa_flags = SA_SIGINFO | SA_RESTART;
            sigemptyset(&action.sa_mask);
            sigaction(SIGPROF, &action, &previous_);
            const pthread_t target = pthread_self();
            running_.store(true);
            sampler_ = std::thread([this, target, microseconds] {
                while (running_.load(std::memory_order_relaxed))
                {
                    pthread_kill(target, SIGPROF);
                    std::this_thread::sleep_for(std::chrono::microseconds(microseconds));
                }
            });
#else
            (void)capacity;
            (void)microseconds;
#endif
        }

        // Stops, and gives every sample's program counter.
        std::vector<std::uintptr_t> stop()
        {
            std::vector<std::uintptr_t> counters;
#if !defined(_WIN32)
            running_.store(false);
            if (sampler_.joinable())
                sampler_.join();
            // Ignoring the signal discards one still pending (POSIX), so the handler that was there never meets it.
            struct sigaction ignore = {};
            ignore.sa_handler = SIG_IGN;
            sigemptyset(&ignore.sa_mask);
            sigaction(SIGPROF, &ignore, nullptr);
            sigaction(SIGPROF, &previous_, nullptr);
            const std::size_t n = std::min(taken().load(), store().size());
            counters.assign(store().begin(), store().begin() + static_cast<std::ptrdiff_t>(n));
#endif
            return counters;
        }

    private:
#if !defined(_WIN32)
        // The handler's memory: one sampler a process at a time.
        static std::vector<std::uintptr_t>& store()
        {
            static std::vector<std::uintptr_t> samples;
            return samples;
        }
        static std::atomic<std::size_t>& taken()
        {
            static std::atomic<std::size_t> count{0};
            return count;
        }

        static std::uintptr_t programCounter(const ucontext_t& context)
        {
#if defined(__APPLE__) && defined(__aarch64__)
            return static_cast<std::uintptr_t>(__darwin_arm_thread_state64_get_pc(context.uc_mcontext->__ss));
#elif defined(__APPLE__) && defined(__x86_64__)
            return static_cast<std::uintptr_t>(context.uc_mcontext->__ss.__rip);
#elif defined(__linux__) && defined(__x86_64__)
            return static_cast<std::uintptr_t>(context.uc_mcontext.gregs[REG_RIP]);
#elif defined(__linux__) && defined(__aarch64__)
            return static_cast<std::uintptr_t>(context.uc_mcontext.pc);
#else
            (void)context;
            return 0;
#endif
        }

        static void onSample(int, siginfo_t*, void* context)
        {
            const std::size_t at = taken().load(std::memory_order_relaxed);
            if (at >= store().size())
                return;
            store()[at] = programCounter(*static_cast<const ucontext_t*>(context));
            taken().store(at + 1, std::memory_order_release);
        }

        struct sigaction previous_ = {};
        std::thread sampler_;
        std::atomic<bool> running_{false};
#endif
    };
} // namespace tee::tools
