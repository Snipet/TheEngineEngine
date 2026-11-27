// tests/assistant/InboxTest.cpp: the inbox between the assistant's worker and the frame (assistant/.../Inbox.h), the
// names the tools take, and who drives. No simulator: these are the inbox's own promises.
#include "tee/assistant/Inbox.h"
#include "tee/assistant/Tools.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <fstream>
#include <atomic>
#include <chrono>
#include <future>
#include <string>
#include <thread>
#include <vector>

using namespace tee::assistant;

TEST_CASE("assistant: a full inbox refuses at once and runs nothing it refused", "[assistant]")
{
    Inbox inbox;
    int ran = 0;
    std::vector<Ticket> tickets;
    for (std::size_t i = 0; i < Inbox::kCapacity; ++i)
        tickets.push_back(inbox.post([&]() -> std::optional<Reply> {
            ++ran;
            return Reply::of(1);
        }));
    const auto before = std::chrono::steady_clock::now();
    Ticket over = inbox.post([&]() -> std::optional<Reply> {
        ran += 1000;
        return Reply::of(2);
    });
    CHECK(over.ready());
    const Reply r = over.wait(10.0);
    CHECK(std::chrono::steady_clock::now() - before < std::chrono::seconds(5));   // not the wait's ten (wide: a loaded runner)
    CHECK_FALSE(r.ok);
    CHECK(r.error == kBusy);
    // the frame drains at most kPerFrame a frame, in order
    CHECK(inbox.drain() == Inbox::kPerFrame);
    CHECK(ran == static_cast<int>(Inbox::kPerFrame));
    while (inbox.drain() > 0)
    {
    }
    CHECK(ran == static_cast<int>(Inbox::kCapacity));
    for (Ticket& t : tickets)
        CHECK(t.wait(0.0).ok);
}

TEST_CASE("assistant: a job its worker gave up on is never run", "[assistant]")
{
    Inbox inbox;
    std::atomic<int> ran{0};
    Ticket t = inbox.post([&]() -> std::optional<Reply> {
        ++ran;
        return Reply::of(1);
    });
    const Reply r = t.wait(0.02);   // nobody drains: the wait runs out
    CHECK_FALSE(r.ok);
    CHECK(r.error == kTimedOut);
    inbox.drain();
    CHECK(ran == 0);

    std::atomic<bool> cancel{true};
    Ticket c = inbox.post([&]() -> std::optional<Reply> {
        ++ran;
        return Reply::of(1);
    });
    CHECK(c.wait(5.0, &cancel).error == kCancelled);
    inbox.drain();
    CHECK(ran == 0);
}

TEST_CASE("assistant: a job the frame has begun is waited for, whatever the worker's limit", "[assistant]")
{
    Inbox inbox;
    std::atomic<bool> inside{false};
    Ticket t = inbox.post([&]() -> std::optional<Reply> {
        inside = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        return Reply::of(7);
    });
    std::thread frame([&] { inbox.drain(); });
    while (!inside)
        std::this_thread::yield();
    const Reply r = t.wait(0.01);   // far shorter than the job: it ran, so its reply is the truth
    frame.join();
    CHECK(r.ok);
    CHECK(r.value == 7);
}

TEST_CASE("assistant: a job not ready yet keeps its place, and nothing overtakes it", "[assistant]")
{
    Inbox inbox;
    std::vector<int> order;
    int tries = 0;
    Ticket a = inbox.post([&]() -> std::optional<Reply> {
        if (++tries < 3)
            return std::nullopt;
        order.push_back(1);
        return Reply::of(1);
    });
    Ticket b = inbox.post([&]() -> std::optional<Reply> {
        order.push_back(2);
        return Reply::of(2);
    });
    CHECK(inbox.drain() == 0);
    CHECK(inbox.drain() == 0);
    CHECK(order.empty());
    CHECK(inbox.drain() == 2);
    CHECK(order == std::vector<int>{1, 2});
    CHECK(a.wait(0.0).ok);
    CHECK(b.wait(0.0).ok);
}

TEST_CASE("assistant: closing answers every waiting job without running it, and every later post", "[assistant]")
{
    Inbox inbox;
    int ran = 0;
    std::vector<std::future<Reply>> waits;
    std::vector<Ticket> tickets;
    for (int i = 0; i < 5; ++i)
        tickets.push_back(inbox.post([&ran, i]() -> std::optional<Reply> {
            ++ran;
            return Reply::of(i);
        }));
    for (Ticket& t : tickets)
        waits.push_back(std::async(std::launch::async, [t]() mutable { return t.wait(30.0); }));
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    inbox.close();
    for (auto& w : waits)
    {
        REQUIRE(w.wait_for(std::chrono::seconds(5)) == std::future_status::ready);
        const Reply r = w.get();
        CHECK_FALSE(r.ok);
        CHECK(r.error == kClosing);
    }
    CHECK(ran == 0);
    CHECK(inbox.post([&]() -> std::optional<Reply> { return Reply::of(0); }).wait(1.0).error == kClosing);
    CHECK(inbox.drain() == 0);
}

TEST_CASE("assistant: each post wakes the app once, on the posting thread, outside the inbox's lock", "[assistant]")
{
    // Claude Desktop's calls while the window is minimised (it does not tick): the app drains on the wake instead
    Inbox inbox;
    std::atomic<int> wakes{0};
    std::thread::id wokeOn;
    bool lockFree = false;
    inbox.setWake([&] {
        ++wakes;
        wokeOn = std::this_thread::get_id();
        // the inbox can be asked from the hook: its lock is not held (under it, this would not answer within 1 s)
        auto asked = std::async(std::launch::async, [&] { return inbox.waiting(); });
        lockFree = asked.wait_for(std::chrono::seconds(1)) == std::future_status::ready;
    });
    std::thread::id poster;
    std::thread t([&] {
        poster = std::this_thread::get_id();
        inbox.post([]() -> std::optional<Reply> { return Reply::of(1); });
    });
    t.join();
    CHECK(wakes == 1);
    CHECK(wokeOn == poster);
    CHECK(lockFree);
    CHECK(inbox.waiting() == 1);
    // the wake drains it (as the app's main thread does), and a second post wakes once more
    CHECK(inbox.drain() == 1);
    inbox.post([]() -> std::optional<Reply> { return Reply::of(2); });
    CHECK(wakes == 2);
    // a refused post (closed, or full) wakes nobody: there is nothing to drain
    inbox.close();
    inbox.post([]() -> std::optional<Reply> { return Reply::of(3); });
    CHECK(wakes == 2);
}

TEST_CASE("assistant: a build's name is a name, never a path", "[assistant]")
{
    std::string why;
    CHECK(safeBuildName("my turbo four", why));
    CHECK(safeBuildName("V8 (crossplane) 5.7", why));
    for (const char* bad : {"", "../etc", "..", "a/b", "a\\b", "/etc/passwd", "C:\\x", "C:x", ".hidden", "~root", "a\nb",
                            "trailing.", " lead", "a..b"})
    {
        INFO(bad);
        CHECK_FALSE(safeBuildName(bad, why));
    }
    CHECK_FALSE(safeBuildName(std::string(65, 'a'), why));
    // a listed build's name may carry its folder, and nothing else of a path
    CHECK(listedBuildName("archetypes/flat-plane-v8-45", why));
    CHECK(listedBuildName("v8", why));
    for (const char* bad : {"", "../etc", "..", "/etc/passwd", "a\\b", "C:\\x", "C:x", ".hidden", "~root", "a\nb",
                            "archetypes/", "a//b", "a/../b"})
    {
        INFO(bad);
        CHECK_FALSE(listedBuildName(bad, why));
    }
}

TEST_CASE("assistant: one driver at a time", "[assistant]")
{
    Drivers d;
    std::string why;
    CHECK(d.take(Driver::Desktop, why));
    CHECK(d.take(Driver::Desktop, why));                // Claude Desktop again: the link orders its own calls
    CHECK_FALSE(d.take(Driver::Chat, why));             // the chat is refused while Claude Desktop drives
    CHECK(why.find("Claude Desktop") != std::string::npos);
    d.release(Driver::Desktop);
    CHECK_FALSE(d.take(Driver::Chat, why));
    d.release(Driver::Desktop);
    CHECK_FALSE(d.held());
    CHECK(d.take(Driver::Chat, why));
    CHECK_FALSE(d.take(Driver::Desktop, why));
    d.release(Driver::Chat);
    CHECK(d.take(Driver::Desktop, why));
}

TEST_CASE("assistant: every serialisation in assistant/ goes through dumpText (X-063)", "[assistant]")
{
    // nlohmann's dump() throws on a string that is not UTF-8; dumpText (Json.h) writes U+FFFD instead. A bare `.dump(`
    // anywhere in assistant/ (include/, view/, mcp/, src/) brings the crash back, so it fails here, by file and line.
    namespace fs = std::filesystem;
    const fs::path root = fs::path(TEE_SOURCE_DIR) / "assistant";
    std::size_t files = 0;
    for (const auto& e : fs::recursive_directory_iterator(root))
    {
        const std::string ext = e.path().extension().string();
        if (!e.is_regular_file() || (ext != ".h" && ext != ".cpp") || e.path().filename() == "Json.h")
            continue;
        ++files;
        std::ifstream in(e.path());
        std::string line;
        for (int n = 1; std::getline(in, line); ++n)
        {
            INFO(e.path().lexically_relative(root).generic_string() << ":" << n << ": " << line);
            CHECK(line.find(".dump(") == std::string::npos);
        }
    }
    CHECK(files > 20);
}
