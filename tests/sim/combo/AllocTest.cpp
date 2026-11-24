// tests/sim/combo/AllocTest.cpp: package X's "nothing allocates in process", with every switch on, through the host
// the application uses, while the pedals, the lever and the switches move. operator new is counted on the thread that
// calls SimHost::process (this executable replaces the global operator new: not on Windows, where card C's replacement
// in tee-host-tests crashed its listing; the test is skipped there). What this proves and what it does not: an
// allocation through C++'s operator new on the audio thread, by the host or anything it calls, is counted; a malloc
// made directly by the JIT's code is not seen here: that side is held by FMinus's [[clang::nonblocking]] check on
// Engine::process, which the host refuses a program without (02-contracts.md section 1).
#include "Combo.h"

#include "tee/Build.h"
#include "tee/Sim.h"

#include <catch2/catch_test_macros.hpp>

#include <atomic>
#include <cstdlib>
#include <new>
#include <vector>

#if !defined(_WIN32)
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
            p = std::aligned_alloc(alignment, (n + alignment - 1) / alignment * alignment);
        else
            p = std::malloc(n);
        if (p == nullptr)
            throw std::bad_alloc();
        return p;
    }
} // namespace

void* operator new(std::size_t n) { return allocate(n, 0); }
void* operator new[](std::size_t n) { return allocate(n, 0); }
void* operator new(std::size_t n, std::align_val_t a) { return allocate(n, static_cast<std::size_t>(a)); }
void* operator new[](std::size_t n, std::align_val_t a) { return allocate(n, static_cast<std::size_t>(a)); }
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
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p, std::size_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { std::free(p); }
void operator delete(void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::align_val_t) noexcept { std::free(p); }
void operator delete(void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
void operator delete[](void* p, std::size_t, std::align_val_t) noexcept { std::free(p); }
#endif

TEST_CASE("nothing on the audio thread allocates with every switch on and the switches flipped while it runs",
          "[fast][fast-2][combo][realtime]")
{
#if defined(_WIN32)
    SKIP("the counting operator new is not built on Windows");
#else
    std::string errors;
    std::shared_ptr<tee::Sim> sim = tee::test::compiled(tee::test::simDirectory(), tee::Sim::Options{}, errors);
    REQUIRE(sim);
    for (const char* build : {"turbo-v8-dual", "small-four"})
    {
        INFO(build);
        tee::SimHost host;
        tee::LoadOptions lo;
        lo.worker = false;
        lo.maxBlock = 512;
        REQUIRE(host.attach(sim, lo, errors));
        tee::Build b(host.fields());
        std::vector<std::string> warnings;
        REQUIRE(b.loadFile(combo::buildPath(build), tee::BuildPart::Whole, warnings, errors));
        for (const combo::Switch& w : combo::switches())
            b.set(w.path, w.on);
        b.attach(&host);
        std::vector<float> l(512), r(512);
        tee::SimControls c;
        c.mode = 1;
        c.ignition = 2;
        long counted = 0;
        for (int i = 0; i < 94 * 12; ++i)   // 12 s at 48 kHz in blocks of 512
        {
            const double t = i * 512.0 / 48000.0;
            c.ignition = t < 2.0 ? 2 : 1;
            c.throttle = t > 3.0 ? static_cast<float>(0.5 + 0.5 * ((i / 40) % 2)) : 0.0f;
            if (i == 94 * 4)
                ++c.shiftUp;
            if (i == 94 * 7)
                ++c.shiftUp;
            if (i % 200 == 199)   // a switch flipped from the UI's side every 2 s
            {
                const auto& sw = combo::switches();
                const combo::Switch& w = sw[static_cast<std::size_t>(i / 200) % sw.size()];
                b.set(w.path, (i / 200) % 2 ? 0 : w.on);
            }
            host.setControls(c);
            b.flush();
            const long before = gCount.load();
            tCounting = true;
            host.process(l.data(), r.data(), 512);
            tCounting = false;
            counted += gCount.load() - before;
            host.service();
        }
        tee::TelemetryFrame tf;
        REQUIRE(host.telemetry(tf));
        CHECK(counted == 0);
        CHECK(tf.host.guardTrips == 0);
        CHECK(tf.host.stops == 0);
        b.attach(nullptr);
    }
#endif
}
