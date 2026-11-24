// tests/host/SimHostTest.cpp: the threaded host (card C, section 2) on the stand-in: commands in order and whole, the
// guard, the checks, nothing allocated on the audio thread, a made-up audio thread against a busy UI thread, the load
// meter, the scope, prepare, two instances at once.
#include "Support.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <cstring>
#include <limits>
#include <thread>
#include <vector>

using namespace tee;
using namespace tee::test;

namespace
{
    struct Blocks
    {
        std::vector<float> left, right;
        explicit Blocks(int n) : left(static_cast<std::size_t>(n)), right(static_cast<std::size_t>(n)) {}
        void play(SimHost& host, int count = 1)
        {
            for (int i = 0; i < count; ++i)
            {
                host.process(left.data(), right.data(), static_cast<int>(left.size()));
                host.service();
            }
        }
        double peak() const
        {
            double p = 0.0;
            for (std::size_t i = 0; i < left.size(); ++i)
                p = std::max({p, static_cast<double>(std::fabs(left[i])), static_cast<double>(std::fabs(right[i]))});
            return p;
        }
        bool finite() const
        {
            for (std::size_t i = 0; i < left.size(); ++i)
                if (!std::isfinite(left[i]) || !std::isfinite(right[i]))
                    return false;
            return true;
        }
    };

    std::vector<unsigned char> curveBytes(const SimHost& host, float y, int n)
    {
        const FieldInfo* f = host.fields().find("spec.engine.torqueShape");
        std::vector<unsigned char> bytes(f->size, 0);
        for (const FieldInfo::Leaf& leaf : f->leaves)
            for (std::size_t k = 0; k < leaf.count; ++k)
            {
                double v = 0.0;
                if (leaf.name == "x")
                    v = 100.0 * static_cast<double>(k);
                else if (leaf.name == "y")
                    v = y;
                else if (leaf.name == "n")
                    v = n;
                writeScalar(bytes.data() + leaf.offset + k * scalarBytes(leaf.scalar), leaf.scalar, v);
            }
        return bytes;
    }

    SimControls running()
    {
        SimControls c;
        c.ignition = Start;
        c.throttle = 0.4f;
        return c;
    }
} // namespace

TEST_CASE("a host plays the stand-in: the Spec at its defaults, controls in, telemetry out", "[simhost]")
{
    SimHost host;
    std::string errors;
    REQUIRE(host.attach(standinSim(), offline(), errors));
    Blocks b(512);
    host.setControls(running());
    b.play(host, 200);   // about 2 s
    TelemetryFrame t;
    REQUIRE(host.telemetry(t));
    CHECK(tel(host, t, "tel.engine.state") == 2);              // cranked and running
    CHECK(tel(host, t, "tel.engine.speed") > 100.0);
    CHECK(tel(host, t, "tel.engine.redline") == 680.0);        // spec.engine.redline's default reached the simulator
    CHECK(tel(host, t, "tel.probe.level") == 0.3f);
    CHECK(t.host.block == 200);
    CHECK(t.host.samples == 200 * 512);
    CHECK(b.peak() > 0.01);
    CHECK(b.peak() <= 1.0);
}

TEST_CASE("a set is clamped to its range, bumps its section's rev once a drain, and a stale ref is refused", "[simhost]")
{
    SimHost host;
    std::string errors;
    REQUIRE(host.attach(standinSim(), offline(), errors));
    Blocks b(256);
    b.play(host);
    TelemetryFrame t;
    REQUIRE(host.telemetry(t));
    CHECK(tel(host, t, "tel.probe.engineRev") == 0);

    CHECK(host.set(ref(host, "spec.engine.redline"), 5000.0));   // its range ends at 1200
    CHECK(host.set(ref(host, "spec.engine.idle"), 90.0));
    CHECK(host.set(ref(host, "spec.engine.idle"), 95.0));
    CHECK_FALSE(host.set(ref(host, "tel.engine.speed"), 1.0));    // no Spec field
    CHECK_FALSE(host.set(ref(host, "spec.engine.rev"), 7.0));     // the host's own
    CHECK(host.readSpec(ref(host, "spec.engine.redline")) == 1200.0);
    b.play(host);
    REQUIRE(host.telemetry(t));
    CHECK(tel(host, t, "tel.engine.redline") == 1200.0);
    CHECK(tel(host, t, "tel.engine.idle") == 95.0);
    CHECK(tel(host, t, "tel.probe.engineRev") == 1);              // three writes, one drain, one bump
    CHECK(tel(host, t, "tel.probe.soundRev") == 0);               // another section's untouched

    FieldRef stale = ref(host, "spec.engine.idle");
    stale.generation += 1000;
    CHECK_FALSE(host.set(stale, 100.0));
}

TEST_CASE("commands arrive in order and whole: a table is never seen half written", "[simhost][commands]")
{
    SimHost host;
    std::string errors;
    REQUIRE(host.attach(standinSim(), offline(48000.0, 64), errors));
    const FieldRef curve = ref(host, "spec.engine.torqueShape");
    Blocks b(64);
    float lastFirst = -1.0f;
    int k = 0;
    for (int round = 0; round < 300; ++round)
    {
        // several tables a block, each all one value: a half-written one would show a spread
        for (int j = 0; j < 1 + round % 7; ++j, ++k)
        {
            const auto bytes = curveBytes(host, static_cast<float>(k), 16);
            REQUIRE(host.setBytes(curve, bytes.data(), bytes.size()));
        }
        b.play(host);
        TelemetryFrame t;
        REQUIRE(host.telemetry(t));
        CHECK(tel(host, t, "tel.probe.curveSpread") == 0.0);
        CHECK(tel(host, t, "tel.probe.curveN") == 16);
        const float first = static_cast<float>(tel(host, t, "tel.probe.curveFirst"));
        CHECK(first == static_cast<float>(k - 1));   // the newest, after every one before it
        CHECK(first > lastFirst);
        lastFirst = first;
    }
}

TEST_CASE("a full queue refuses a write, counts it, and the next frame's retry gets through", "[simhost][commands]")
{
    LoadOptions options = offline();
    options.commandSlots = 8;
    options.poolBytes = 4096;
    SimHost host;
    std::string errors;
    REQUIRE(host.attach(standinSim(), options, errors));
    const FieldRef idle = ref(host, "spec.engine.idle");
    int accepted = 0;
    for (int i = 0; i < 20; ++i)
        accepted += host.set(idle, 60.0 + i) ? 1 : 0;
    CHECK(accepted == 8);
    // a table too large for what is left of the pool, and one too large for any pool
    const FieldRef map = ref(host, "spec.ecu.spark");
    std::vector<unsigned char> big(map.size, 0);
    CHECK_FALSE(host.setBytes(map, big.data(), big.size()));
    Blocks b(128);
    b.play(host);
    TelemetryFrame t;
    REQUIRE(host.telemetry(t));
    CHECK(t.host.commandsRefused == 13);
    CHECK(tel(host, t, "tel.engine.idle") == 67.0);
    CHECK(host.set(idle, 120.0));
    b.play(host);
    REQUIRE(host.telemetry(t));
    CHECK(tel(host, t, "tel.engine.idle") == 120.0);
}

TEST_CASE("the guard catches a planted NaN: no blast, a flag, a fresh instance once the Spec is sane", "[simhost][guard]")
{
    SimHost host;
    std::string errors;
    REQUIRE(host.attach(standinSim(), offline(), errors));
    host.setControls(running());
    Blocks b(512);
    b.play(host, 100);
    REQUIRE(b.peak() > 0.01);

    const float nan = std::numeric_limits<float>::quiet_NaN();
    const FieldRef level = ref(host, "spec.sound.level");
    REQUIRE(host.setBytes(level, &nan, sizeof nan));   // setBytes is not clamped: what a corrupt file could do
    for (int i = 0; i < 20; ++i)
    {
        b.play(host);
        CHECK(b.finite());
        CHECK(b.peak() <= 1.0);
    }
    TelemetryFrame t;
    REQUIRE(host.telemetry(t));
    CHECK(t.host.guardTrips >= 1);
    CHECK(t.host.faulted == 1);
    CHECK(host.stopReason().find("output guard") != std::string::npos);
    CHECK(b.peak() == 0.0);   // silent while the Spec holds the NaN

    CHECK(host.set(level, 0.3));
    for (int i = 0; i < 200; ++i)
    {
        b.play(host);
        CHECK(b.finite());
        std::this_thread::sleep_for(std::chrono::microseconds(1500));   // the restart is rate-limited to 4 a second
    }
    REQUIRE(host.telemetry(t));
    CHECK(t.host.faulted == 0);
    CHECK(t.host.restarts >= 1);
    CHECK(b.peak() > 0.01);
}

TEST_CASE("a development build's bounds check stops the call; the host reports FMinus's words", "[simhost][checks]")
{
    SimHost host;
    std::string errors;
    REQUIRE(host.attach(standinSim(true), offline(), errors));
    host.setControls(running());
    Blocks b(512);
    b.play(host, 50);
    REQUIRE(b.peak() > 0.0);
    const std::int32_t outside = 9;
    REQUIRE(host.setBytes(ref(host, "spec.sound.tap"), &outside, sizeof outside));
    for (int i = 0; i < 5; ++i)
    {
        b.play(host);
        CHECK(b.finite());
    }
    TelemetryFrame t;
    REQUIRE(host.telemetry(t));
    CHECK(t.host.stops >= 1);
    INFO(host.stopReason());
    CHECK(host.stopReason().find("bounds check") != std::string::npos);
    CHECK(host.stopReason().find("harmonics") != std::string::npos);
    CHECK(b.peak() == 0.0);
}

TEST_CASE("a second host on a Sim with the checks is refused; without them two hosts play at once", "[simhost]")
{
    Sim::Options withChecks;
    withChecks.checks = true;
    std::string errors;
    auto checked = Sim::compile(standin(), withChecks, errors);
    REQUIRE(checked != nullptr);
    SimHost a, b;
    REQUIRE(a.attach(checked, offline(), errors));
    CHECK_FALSE(b.attach(checked, offline(), errors));

    auto shared = standinSim();
    SimHost c, d;
    REQUIRE(c.attach(shared, offline(), errors));
    REQUIRE(d.attach(shared, offline(44100.0, 128), errors));
    c.setControls(running());
    Blocks bc(512), bd(128);
    bc.play(c, 100);
    bd.play(d, 100);
    TelemetryFrame tc, td;
    REQUIRE(c.telemetry(tc));
    REQUIRE(d.telemetry(td));
    CHECK(tel(c, tc, "tel.engine.speed") > 50.0);
    CHECK(tel(d, td, "tel.engine.speed") == 0.0);   // its own instance: nobody started it
}

TEST_CASE("prepare makes the instance afresh at the new rate with the Spec the host sent", "[simhost]")
{
    SimHost host;
    std::string errors;
    REQUIRE(host.attach(standinSim(), offline(), errors));
    REQUIRE(host.set(ref(host, "spec.engine.idle"), 120.0));
    Blocks b(256);
    b.play(host, 4);
    host.prepare(96000.0, 2048);
    CHECK(host.sampleRate() == 96000.0);
    Blocks big(3000);   // longer than the longest call: cut into calls of 2048 and 952
    big.play(host);
    TelemetryFrame t;
    REQUIRE(host.telemetry(t));
    CHECK(tel(host, t, "tel.engine.idle") == 120.0);
    CHECK(tel(host, t, "tel.probe.calls") == 2);   // the instance is new: two calls since
}

TEST_CASE("the scope is copied only while asked for, and ctl.scopeOn says so", "[simhost][scope]")
{
    SimHost host;
    std::string errors;
    REQUIRE(host.attach(standinSim(), offline(), errors));
    host.setControls(running());
    Blocks b(512);
    b.play(host, 100);
    ScopeFrame s;
    CHECK_FALSE(host.scope(s));   // nothing copied before anybody asked
    b.play(host, 10);
    REQUIRE(host.scope(s));
    const FieldInfo* filled = host.fields().find("scope.filled");
    const FieldInfo* pressure = host.fields().find("scope.pressure");
    CHECK(s.get(*filled) > 0);
    double highest = 0.0;
    for (std::size_t i = 0; i < 720; ++i)
        highest = std::max(highest, s.get(*pressure, i));
    CHECK(highest > 101325.0);
    // nobody asks for a second: it stops being copied
    b.play(host, 100);
    const std::uint64_t copied = s.block;
    ScopeFrame again;
    REQUIRE(host.scope(again));   // this asks again, and gives the last one copied before
    CHECK(again.block > copied);
    CHECK(again.block < 200);   // copies stopped about half a second after the last ask
}

TEST_CASE("nothing on the audio thread allocates, and the host's own work in a block is small", "[simhost][realtime]")
{
    SimHost host;
    std::string errors;
    REQUIRE(host.attach(standinSim(), offline(), errors));
    host.setControls(running());
    const FieldRef idle = ref(host, "spec.engine.idle");
    const FieldRef curve = ref(host, "spec.engine.torqueShape");
    Blocks b(512);
    b.play(host, 4);
    ScopeFrame s;
    host.scope(s);
    const long before = AllocationCounter::count();
    for (int i = 0; i < 2000; ++i)
    {
        // a busy UI: a few writes a block, a table now and then, the scope asked for
        REQUIRE(host.set(idle, 70.0 + i % 50));
        if (i % 10 == 0)
        {
            const auto bytes = curveBytes(host, 1.0f, 8);
            REQUIRE(host.setBytes(curve, bytes.data(), bytes.size()));
            host.scope(s);
        }
        SimControls c = running();
        c.throttle = static_cast<float>(i % 100) / 100.0f;
        host.setControls(c);
        AllocationCounter::countThisThread(true);
        host.process(b.left.data(), b.right.data(), 512);
        AllocationCounter::countThisThread(false);
        host.service();
    }
    if (AllocationCounter::available())
        CHECK(AllocationCounter::count() - before == 0);
    else
        WARN("allocations on the audio thread not counted here (Windows or ThreadSanitizer): that part is skipped");
    TelemetryFrame t;
    REQUIRE(host.telemetry(t));
    UNSCOPED_INFO("host's own work a block of 512: mean " << t.host.hostMicrosMean << " us, peak " << t.host.hostMicrosPeak
                                                         << " us; load mean " << t.host.loadMean << ", peak " << t.host.loadPeak);
    CHECK(t.host.hostMicrosMean < 20.0);   // card C's budget
    CHECK(t.host.loadMean > 0.0);
    CHECK(t.host.loadPeak >= t.host.loadMean);
    CHECK(t.host.commandsApplied >= 2000);
    WARN("host's own work a block of 512 at 48 kHz: mean " << t.host.hostMicrosMean << " us, peak "
                                                            << t.host.hostMicrosPeak << " us; the stand-in's load mean "
                                                            << t.host.loadMean);
}

TEST_CASE("a made-up audio thread in a tight loop while another thread sets ten thousand fields and tables",
          "[simhost][threads]")
{
    LoadOptions options = offline(48000.0, 256);
    options.worker = true;   // the host's own thread services it, as in the application
    SimHost host;
    std::string errors;
    REQUIRE(host.attach(standinSim(), options, errors));
    const FieldRef idle = ref(host, "spec.engine.idle");
    const FieldRef harmonic = ref(host, "spec.sound.harmonics[1]");
    const FieldRef curve = ref(host, "spec.engine.torqueShape");

    std::atomic<bool> done{false};
    std::atomic<long> audioAllocations{0};
    std::atomic<int> spreads{0};
    std::thread audio([&] {
        Blocks b(256);
        const long before = AllocationCounter::count();
        AllocationCounter::countThisThread(true);
        while (!done.load(std::memory_order_relaxed))
            host.process(b.left.data(), b.right.data(), 256);
        AllocationCounter::countThisThread(false);
        audioAllocations = AllocationCounter::count() - before;
    });
    std::thread reader([&] {
        TelemetryFrame t;
        ScopeFrame s;
        const FieldInfo* spread = host.fields().find("tel.probe.curveSpread");
        while (!done.load(std::memory_order_relaxed))
        {
            if (host.telemetry(t) && t.get(*spread) != 0.0)
                ++spreads;
            host.scope(s);
            std::this_thread::yield();
        }
    });

    int last = 0;
    for (int i = 0; i < 10000; ++i)
    {
        const auto attempt = [&](auto&& write) {
            while (!write())
                std::this_thread::yield();   // a full queue: try again, as a UI does next frame
        };
        switch (i % 4)
        {
            case 0: attempt([&] { return host.set(idle, 50.0 + i % 100); }); break;
            case 1: attempt([&] { return host.set(harmonic, (i % 10) / 10.0); }); break;
            default:
            {
                const auto bytes = curveBytes(host, static_cast<float>(i), 1 + i % 16);
                attempt([&] { return host.setBytes(curve, bytes.data(), bytes.size()); });
                last = i;
                break;
            }
        }
        if (i % 500 == 0)
        {
            SimControls c = running();
            c.throttle = static_cast<float>(i % 7) / 7.0f;
            host.setControls(c);
        }
    }
    // let the audio thread take the rest
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    done = true;
    audio.join();
    reader.join();
    if (AllocationCounter::available())
        CHECK(audioAllocations.load() == 0);
    else
        WARN("allocations on the audio thread not counted here (Windows or ThreadSanitizer): that part is skipped");
    CHECK(spreads.load() == 0);
    TelemetryFrame t;
    REQUIRE(host.telemetry(t));
    CHECK(tel(host, t, "tel.probe.curveFirst") == static_cast<float>(last));
    CHECK(t.host.commandsApplied == 10000);
}

TEST_CASE("LoadOptions::hold writes its values before every block; a path the program lacks is skipped", "[simhost]")
{
    SimHost host;
    std::string errors;
    LoadOptions load = offline();
    load.hold = {{"spec.engine.redline", 600.0}, {"heat.tCool", 368.0}};   // the stand-in has no heat
    REQUIRE(host.attach(standinSim(), load, errors));
    // a write through the queue is undone by the hold at the next block
    REQUIRE(host.set(ref(host, "spec.engine.redline"), 650.0));
    Blocks b(512);
    host.setControls(running());
    b.play(host, 20);
    TelemetryFrame t;
    REQUIRE(host.telemetry(t));
    CHECK(tel(host, t, "tel.engine.redline") == 600.0);
    CHECK(warmEngine().size() == 5);
}

namespace
{
    // One block with the audio thread's allocations counted (where they can be: AllocationCounter::available()), the
    // host serviced after, off the count, as its own thread would.
    long countedBlock(SimHost& host, Blocks& b)
    {
        const long before = AllocationCounter::count();
        AllocationCounter::countThisThread(true);
        host.process(b.left.data(), b.right.data(), static_cast<int>(b.left.size()));
        AllocationCounter::countThisThread(false);
        const long made = AllocationCounter::count() - before;
        host.service();
        return made;
    }

    SimControls keyOn()
    {
        SimControls c;
        c.ignition = 1;
        c.throttle = 0.4f;
        return c;
    }
} // namespace

TEST_CASE("a finite sample beyond full scale is clamped and counted: no fault, the engine runs on", "[simhost][guard]")
{
    SimHost host;
    std::string errors;
    REQUIRE(host.attach(standinSim(), offline(), errors));
    host.setControls(running());
    Blocks b(512);
    b.play(host, 100);
    host.setControls(keyOn());
    b.play(host, 20);
    TelemetryFrame t;
    REQUIRE(host.telemetry(t));
    REQUIRE(tel(host, t, "tel.engine.state") == 2);

    const float loud = 5.0f;   // five times the level's range: the stand-in's output some 1.5, finite (setBytes: unclamped)
    REQUIRE(host.setBytes(ref(host, "spec.sound.level"), &loud, sizeof loud));
    long allocations = 0;
    double peak = 0.0;
    for (int i = 0; i < 60; ++i)
    {
        allocations += countedBlock(host, b);
        CHECK(b.finite());
        peak = std::max(peak, b.peak());
    }
    CHECK(peak <= 1.0);
    REQUIRE(host.telemetry(t));
    CHECK(t.host.guardTrips == 0);
    CHECK(t.host.restarts == 0);
    CHECK(t.host.faulted == 0);
    CHECK(t.host.clippedBlocks > 0);
    CHECK(t.host.largestSample > 1.0f);
    CHECK(t.host.largestSample < 16.0f);
    CHECK(tel(host, t, "tel.engine.state") == 2);   // still running
    if (AllocationCounter::available())
        CHECK(allocations == 0);
}

TEST_CASE("after a fault's fresh instance, an ignition left on cranks it back to running", "[simhost][guard]")
{
    SimHost host;
    std::string errors;
    REQUIRE(host.attach(standinSim(), offline(), errors));
    host.setControls(running());
    Blocks b(512);
    b.play(host, 100);
    host.setControls(keyOn());   // the key on, not held in start: a person who started it and let go
    b.play(host, 20);
    TelemetryFrame t;
    REQUIRE(host.telemetry(t));
    REQUIRE(tel(host, t, "tel.engine.state") == 2);

    const float nan = std::numeric_limits<float>::quiet_NaN();
    const FieldRef level = ref(host, "spec.sound.level");
    REQUIRE(host.setBytes(level, &nan, sizeof nan));
    long allocations = 0;
    for (int i = 0; i < 5; ++i)
    {
        allocations += countedBlock(host, b);
        CHECK(b.finite());
    }
    REQUIRE(host.telemetry(t));
    CHECK(t.host.guardTrips >= 1);
    CHECK(t.host.faulted == 1);
    CHECK(host.stopReason().find("restarted") != std::string::npos);

    CHECK(host.set(level, 0.3));
    for (int i = 0; i < 300; ++i)
    {
        allocations += countedBlock(host, b);
        CHECK(b.finite());
        std::this_thread::sleep_for(std::chrono::microseconds(1500));   // the restart is rate-limited to 4 a second
    }
    REQUIRE(host.telemetry(t));
    CHECK(t.host.faulted == 0);
    CHECK(t.host.restarts >= 1);
    CHECK(t.host.restarting == 0);
    CHECK(tel(host, t, "tel.engine.state") == 2);   // running again, as the person left it
    CHECK(host.controls().ignition == 1);            // the person's controls untouched
    CHECK(b.peak() > 0.01);
    if (AllocationCounter::available())
        CHECK(allocations == 0);
}
