// tests/assistant/McpTest.cpp: Claude Desktop's path end to end, without Claude Desktop: the bridge (tee-mcp, run as
// Claude Desktop runs it, JSON-RPC on its stdin and stdout) and the app's link (McpLink.h) over a socket in a folder this
// test made, the app a real simulator, document and library on the simulated clock (Rig.h). Nothing leaves the machine.
// macOS and Linux (Windows has no link in this version).
#include "Rig.h"
#include "tee/assistant/Cards.h"
#include "tee/assistant/McpLink.h"
#include "tee/assistant/McpWire.h"
#include "tee/assistant/Session.h"

#include <catch2/catch_test_macros.hpp>

#if !defined(_WIN32)
 #include <csignal>
 #include <poll.h>
 #include <sys/stat.h>
 #include <sys/wait.h>
 #include <unistd.h>

using namespace tee;
using namespace tee::test;
using namespace tee::assistant;
using nlohmann::json;
namespace fs = std::filesystem;

namespace
{
    // Counts SIGPIPEs while it lives, in place of the default (which ends the process). No case here ignores SIGPIPE:
    // a write that would raise it must not happen at all (the library guards every one: McpWire.h's sendAll), and a
    // case checks the count is still 0, so Linux's MSG_NOSIGNAL cannot hide a guard that macOS lacks.
    std::atomic<int> gSigpipes{0};
    struct SigpipeWatch
    {
        struct sigaction was{};
        SigpipeWatch()
        {
            struct sigaction sa{};
            sa.sa_handler = [](int) { gSigpipes.fetch_add(1); };
            sigemptyset(&sa.sa_mask);
            sigaction(SIGPIPE, &sa, &was);
        }
        ~SigpipeWatch() { sigaction(SIGPIPE, &was, nullptr); }
        int count() const { return gSigpipes.load(); }
    };

    // A private folder of the case's own for the link.
    std::string linkFolder(const std::string& name)
    {
        const fs::path dir = fs::path(TEE_SOURCE_DIR) / "out" / "assistant" / ("link-" + name);
        std::error_code ec;
        fs::remove_all(dir, ec);
        fs::create_directories(dir.parent_path());
        mkdir(dir.c_str(), 0700);
        chmod(dir.c_str(), 0700);
        return dir.string();
    }

    // tee-mcp as Claude Desktop starts it: its stdin and stdout pipes, TEE_MCP_DIR the test's folder.
    struct Bridge
    {
        pid_t pid = -1;
        int in = -1, out = -1;
        wire::Lines lines;

        explicit Bridge(const std::string& dir)
        {
            int toChild[2], fromChild[2];
            REQUIRE(pipe(toChild) == 0);
            REQUIRE(pipe(fromChild) == 0);
            pid = fork();
            REQUIRE(pid >= 0);
            if (pid == 0)
            {
                dup2(toChild[0], 0);
                dup2(fromChild[1], 1);
                close(toChild[1]);
                close(fromChild[0]);
                setenv("TEE_MCP_DIR", dir.c_str(), 1);
                execl(TEE_MCP_EXE, "tee-mcp", static_cast<char*>(nullptr));
                _exit(127);
            }
            close(toChild[0]);
            close(fromChild[1]);
            in = toChild[1];
            out = fromChild[0];
        }

        ~Bridge()
        {
            if (in >= 0)
                close(in);   // the client gone: the bridge ends
            int status = 0;
            for (int i = 0; i < 200 && waitpid(pid, &status, WNOHANG) == 0; ++i)
                usleep(10000);
            if (waitpid(pid, &status, WNOHANG) == 0)
            {
                kill(pid, SIGKILL);
                waitpid(pid, &status, 0);
            }
            close(out);
        }

        void send(const json& msg)
        {
            const std::string s = msg.dump() + "\n";
            REQUIRE(write(in, s.data(), s.size()) == static_cast<ssize_t>(s.size()));
        }

        // The next line from the bridge within `seconds` (real time), the rig's frames run meanwhile; null when none.
        json next(Rig* rig, double seconds)
        {
            const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(static_cast<int>(seconds * 1000));
            std::string line;
            while (std::chrono::steady_clock::now() < end)
            {
                if (lines.next(line))
                    return json::parse(line);
                pollfd p{out, POLLIN, 0};
                if (::poll(&p, 1, 2) > 0)
                {
                    char buf[16384];
                    const ssize_t n = read(out, buf, sizeof buf);
                    if (n <= 0)
                        return nullptr;
                    lines.add(buf, static_cast<std::size_t>(n));
                }
                if (rig != nullptr)
                    rig->frame();
            }
            return nullptr;
        }

        // A request's reply (other lines, notifications, are kept aside in `seen`).
        json ask(Rig* rig, int id, const std::string& method, json params = json::object(), double seconds = 60.0)
        {
            send({{"jsonrpc", "2.0"}, {"id", id}, {"method", method}, {"params", std::move(params)}});
            const auto end = std::chrono::steady_clock::now() + std::chrono::milliseconds(static_cast<int>(seconds * 1000));
            while (std::chrono::steady_clock::now() < end)
            {
                json m = next(rig, 0.5);
                if (m.is_null())
                    continue;
                if (m.contains("id") && m["id"] == id)
                    return m;
                seen.push_back(m);
            }
            return nullptr;
        }

        std::vector<json> seen;
    };

    json toolResult(const json& reply)
    {
        REQUIRE(reply.contains("result"));
        return json::parse(reply["result"]["content"][0]["text"].get<std::string>());
    }

    struct App
    {
        Rig rig;
        Drivers drivers;
        Cards cards;
        McpLink link;
        App(const std::string& name, const std::string& dir, long user = -1)
            : rig(name), link(*rig.tools, drivers, cards, [this] { return rig.clock; }, McpLinkOptions{dir, user, "0.1.0"})
        {
        }
        ~App() { link.stop(); }
    };
}

TEST_CASE("assistant: Claude Desktop's path, bridge to app: tools, a change with its card and Undo, a cancel",
          "[assistant][sharedsim]")
{
    const std::string dir = linkFolder("path");
    App app("mcp-path", dir);
    std::string why;
    const bool started = app.link.start(why);
    INFO(why);
    REQUIRE(started);
    CHECK(fs::exists(wire::socketIn(dir)));
    Bridge bridge(dir);

    const json init = bridge.ask(&app.rig, 1, "initialize",
                                 {{"protocolVersion", "2025-06-18"}, {"capabilities", json::object()},
                                  {"clientInfo", {{"name", "test"}, {"version", "0"}}}});
    REQUIRE(init.contains("result"));
    CHECK(init["result"]["protocolVersion"] == "2025-06-18");
    CHECK(init["result"]["capabilities"]["tools"]["listChanged"] == true);
    bridge.send({{"jsonrpc", "2.0"}, {"method", "notifications/initialized"}});

    const json list = bridge.ask(&app.rig, 2, "tools/list");
    REQUIRE(list.contains("result"));
    const json& tools = list["result"]["tools"];
    CHECK(tools.size() == app.rig.tools->list().size());
    for (const json& t : tools)
    {
        INFO(t.dump());
        CHECK(t.contains("inputSchema"));
        const bool readOnly = t["annotations"]["readOnlyHint"].get<bool>();
        const std::string name = t["name"].get<std::string>();
        CHECK(readOnly == (name != "set_fields" && name != "load_build" && name != "save_build_as"));
        CHECK(t["annotations"]["openWorldHint"] == false);
    }

    // a change: answered, and a card with its Undo, live while its step is on top
    const auto fields = app.rig.settable(2);
    const double before = app.rig.build().get(fields[0].first->path);
    const json set = bridge.ask(&app.rig, 3, "tools/call",
                                {{"name", "set_fields"},
                                 {"arguments", {{"changes", {{{"path", shortPath(*fields[0].first)}, {"value", fields[0].second}}}}}}});
    REQUIRE(set.contains("result"));
    CHECK(set["result"]["isError"] == false);
    const json done = toolResult(set);
    const std::uint64_t step = done["undoStep"].get<std::uint64_t>();
    const std::vector<Card> cards = app.cards.list();
    REQUIRE(cards.size() == 1);
    CHECK(cards[0].step == step);
    CHECK(cards[0].tool == "set_fields");
    CHECK(cards[0].summary.find(shortPath(*fields[0].first)) != std::string::npos);
    CHECK(app.cards.canUndo(cards[0].id, app.rig.build()));
    // something else done since: its Undo is not live, and refuses
    app.rig.build().set(fields[1].first->path, fields[1].second);
    CHECK_FALSE(app.cards.canUndo(cards[0].id, app.rig.build()));
    CHECK_FALSE(app.cards.undo(cards[0].id, app.rig.build(), why));
    CHECK(app.rig.build().get(fields[0].first->path) != before);
    // that undone, the card's step is on top again: its Undo takes back exactly its change
    REQUIRE(app.rig.build().undo());
    REQUIRE(app.cards.canUndo(cards[0].id, app.rig.build()));
    REQUIRE(app.cards.undo(cards[0].id, app.rig.build(), why));
    CHECK(app.rig.build().get(fields[0].first->path) == before);
    CHECK_FALSE(app.cards.canUndo(cards[0].id, app.rig.build()));

    // an error is an error result, never a protocol error, and leaves no card
    const json bad = bridge.ask(&app.rig, 4, "tools/call",
                                {{"name", "set_fields"},
                                 {"arguments", {{"changes", {{{"path", "ctl.throttle"}, {"value", 1}}}}}}});
    CHECK(bad["result"]["isError"] == true);
    CHECK(app.cards.list().size() == 1);

    // a call the client cancels: never answered; the app's call stops, and the next call is answered
    bridge.send({{"jsonrpc", "2.0"}, {"id", 5}, {"method", "tools/call"}, {"params", {{"name", "predict_curve"}}}});
    while (!app.link.view().running)
        app.rig.frame();
    bridge.send({{"jsonrpc", "2.0"}, {"method", "notifications/cancelled"}, {"params", {{"requestId", 5}}}});
    const auto t0 = std::chrono::steady_clock::now();
    while (app.link.view().running && std::chrono::steady_clock::now() - t0 < std::chrono::seconds(10))
        app.rig.frame();
    CHECK_FALSE(app.link.view().running);
    const json after = bridge.ask(&app.rig, 6, "tools/call", {{"name", "get_status"}});
    CHECK(after["result"]["isError"] == false);
    for (const json& m : bridge.seen)
        CHECK_FALSE((m.contains("id") && m["id"] == 5));

    // a tool that does not exist: a protocol error (MCP: -32602), not a tool's failure
    const json unknown = bridge.ask(&app.rig, 9, "tools/call", {{"name", "delete_everything"}});
    REQUIRE(unknown.contains("error"));
    CHECK(unknown["error"]["code"] == -32602);
    CHECK(bridge.ask(&app.rig, 7, "ping").contains("result"));
    CHECK(bridge.ask(&app.rig, 8, "no/such")["error"]["code"] == -32601);
}

TEST_CASE("assistant: the switch: off, nothing listens and the bridge says so; on, the tools appear", "[assistant][sharedsim]")
{
    const std::string dir = linkFolder("switch");
    App app("mcp-switch", dir);
    CHECK_FALSE(fs::exists(wire::socketIn(dir)));
    Bridge bridge(dir);
    bridge.ask(&app.rig, 1, "initialize", {{"protocolVersion", "2025-11-25"}});
    bridge.send({{"jsonrpc", "2.0"}, {"method", "notifications/initialized"}});
    const json empty = bridge.ask(&app.rig, 2, "tools/list");
    CHECK(empty["result"]["tools"].empty());
    const json off = bridge.ask(&app.rig, 3, "tools/call", {{"name", "get_status"}});
    CHECK(off["result"]["isError"] == true);
    CHECK(toolResult(off)["error"].get<std::string>().find("Allow Claude Desktop") != std::string::npos);

    std::string why;
    const bool started = app.link.start(why);
    INFO(why);
    REQUIRE(started);
    // the bridge finds the app and says the list changed
    bool changed = false;
    for (int i = 0; i < 20 && !changed; ++i)
    {
        const json m = bridge.next(&app.rig, 0.5);
        changed = m.is_object() && m.value("method", "") == "notifications/tools/list_changed";
    }
    CHECK(changed);
    CHECK(bridge.ask(&app.rig, 4, "tools/list")["result"]["tools"].size() == app.rig.tools->list().size());

    // off again: the socket gone, the bridge's next call refused in words
    app.link.stop();
    CHECK_FALSE(fs::exists(wire::socketIn(dir)));
    CHECK_FALSE(app.link.view().listening);
    const json gone = bridge.ask(&app.rig, 5, "tools/call", {{"name", "get_status"}});
    CHECK(gone["result"]["isError"] == true);
}

TEST_CASE("assistant: the link refuses another user's peer, a folder open to others, and a second app", "[assistant][sharedsim]")
{
    // a peer of another user: the link expects a user this process is not; the connection is closed unanswered
    {
        const std::string dir = linkFolder("peer");
        App app("mcp-peer", dir, static_cast<long>(geteuid()) + 1);
        std::string why;
        const bool started = app.link.start(why);
    INFO(why);
    REQUIRE(started);
        const int fd = wire::connectTo(dir, why);
        REQUIRE(fd >= 0);
        const std::string hello = "{\"id\":1,\"method\":\"tools\"}\n";
        wire::noSigpipe(fd);
        wire::sendAll(fd, hello.data(), hello.size());   // (the link may have closed it already: never a signal)
        char buf[256];
        pollfd p{fd, POLLIN, 0};
        REQUIRE(::poll(&p, 1, 5000) > 0);
        CHECK(read(fd, buf, sizeof buf) <= 0);   // closed, nothing said
        close(fd);
        for (int i = 0; i < 100 && app.link.view().refused == 0; ++i)
            usleep(10000);
        CHECK(app.link.view().refused == 1);
        CHECK(app.link.view().connections == 0);
    }
    // a folder open to others: the link is not made, and the bridge does not connect through it
    {
        const std::string dir = linkFolder("open");
        chmod(dir.c_str(), 0755);
        App app("mcp-open", dir);
        std::string why;
        CHECK_FALSE(app.link.start(why));
        CHECK(why.find("open to other users") != std::string::npos);
        CHECK(wire::connectTo(dir, why) < 0);
    }
    // a second app: refused while the first listens; the first's socket untouched
    {
        const std::string dir = linkFolder("second");
        App first("mcp-first", dir);
        App second("mcp-second", dir);
        std::string why;
        REQUIRE(first.link.start(why));
        CHECK_FALSE(second.link.start(why));
        CHECK(why.find("another") != std::string::npos);
        const int fd = wire::connectTo(dir, why);
        CHECK(fd >= 0);
        if (fd >= 0)
            close(fd);
    }
}

TEST_CASE("assistant: shutting down with a call waiting on the frame cancels it at once, and the frame need not drain",
          "[assistant][sharedsim]")
{
    const std::string dir = linkFolder("shutdown");
    Rig rig("mcp-shutdown");
    auto session = std::make_unique<Session>(SessionParts{
        *rig.file, *rig.library, [&]() -> SimHost* { return &rig.sim->host(); },
        [&] { return rig.sim->host().sim(); }, [&] { return rig.clock; }, nullptr, nullptr, McpLinkOptions{dir, -1, "0.1.0"}});
    std::string why;
    REQUIRE(session->allowDesktop(true, why));
    CHECK(session->desktopAllowed());
    Bridge bridge(dir);
    bridge.ask(nullptr, 1, "initialize", {{"protocolVersion", "2025-11-25"}}, 5.0);
    // a person holds a knob: the batch's job waits on the frame; then the frame stops draining (the app closing)
    const auto fields = rig.settable(1);
    rig.build().beginGesture();
    bridge.send({{"jsonrpc", "2.0"}, {"id", 2}, {"method", "tools/call"},
                 {"params", {{"name", "set_fields"},
                             {"arguments", {{"changes", {{{"path", shortPath(*fields[0].first)}, {"value", fields[0].second}}}}}}}}});
    while (!session->desktop().running)
        session->frame();
    const double stored = rig.build().get(fields[0].first->path);
    const auto t0 = std::chrono::steady_clock::now();
    session->stop();   // no frame drains from here on
    CHECK(std::chrono::steady_clock::now() - t0 < std::chrono::seconds(2));
    CHECK_FALSE(fs::exists(wire::socketIn(dir)));
    rig.build().endGesture();
    CHECK(rig.build().get(fields[0].first->path) == stored);
    // the bridge answered its client: the call ended, nothing changed
    const json r = bridge.next(nullptr, 5.0);
    REQUIRE(r.is_object());
    CHECK(r["id"] == 2);
    CHECK(r["result"]["isError"] == true);
    session.reset();
}

namespace
{
    int threadsOfThisProcess()
    {
        std::error_code ec;
        int n = 0;
        for (auto it = fs::directory_iterator("/proc/self/task", ec); !ec && it != fs::directory_iterator(); it.increment(ec))
            ++n;
        return n;   // 0 where there is no /proc (macOS): the check below is then the folder's alone
    }
}

TEST_CASE("assistant: the switch off is the shipped default and costs nothing: no thread, no socket, no folder",
          "[assistant][sharedsim]")
{
    const fs::path dir = fs::path(TEE_SOURCE_DIR) / "out" / "assistant" / "link-never";
    std::error_code ec;
    fs::remove_all(dir, ec);
    Rig rig("mcp-never");
    rig.inbox.close();   // (the rig's own inbox is not used here)
    const int before = threadsOfThisProcess();
    {
        // as the app's frame makes it (app/Offline.cpp, Kept), the switch off
        Session session(SessionParts{*rig.file, *rig.library, [&]() -> SimHost* { return &rig.sim->host(); },
                                     [&] { return rig.sim->host().sim(); }, [&] { return rig.clock; }, nullptr, nullptr,
                                     McpLinkOptions{dir.string(), -1, "0.1.0"}});
        for (int i = 0; i < 30; ++i)
        {
            session.frame(false);
            rig.sim->frame();
        }
        CHECK_FALSE(session.desktopAllowed());
        CHECK(session.statusNote().empty());
        CHECK(threadsOfThisProcess() == before);
        CHECK_FALSE(fs::exists(dir));
        // turned on and off again: everything it made is gone
        std::string why;
        REQUIRE(session.allowDesktop(true, why));
        CHECK(session.statusNote() == "Claude Desktop allowed");
        CHECK(fs::exists(wire::socketIn(dir.string())));
        REQUIRE(session.allowDesktop(false, why));
        CHECK_FALSE(fs::exists(wire::socketIn(dir.string())));
        CHECK(threadsOfThisProcess() == before);
    }
    // and a load over unsaved changes, with no one to ask (the frame's dialog not drawn yet): refused, never silent
    {
        Rig asked("mcp-noask");
        Tools tools(Host{asked.inbox, *asked.file, *asked.library, [&]() -> SimHost* { return &asked.sim->host(); },
                         [&] { return asked.sim->host().sim(); }, [&] { return asked.clock; }, nullptr, Limits{}, nullptr});
        const auto fields = asked.settable(1);
        asked.build().set(fields[0].first->path, fields[0].second);
        REQUIRE(asked.file->dirty());
        const std::uint64_t hash = asked.build().hash();
        const Result r = asked.whileFrames([&] { return tools.call("load_build", {{"name", "v8"}}); });
        CHECK(r.isError);
        CHECK(r.value["error"].get<std::string>().find("cannot be asked") != std::string::npos);
        CHECK(asked.build().hash() == hash);
    }
}

TEST_CASE("assistant: a peer gone mid-reply, or at shutdown, never ends the app with SIGPIPE", "[assistant][sharedsim]")
{
    const SigpipeWatch watch;
    const int before = watch.count();
    const std::string dir = linkFolder("gone");
    App app("mcp-gone", dir);
    std::string why;
    REQUIRE(app.link.start(why));
    // a peer that asks for a large answer and leaves before reading it, many times over: the poll thread's own replies
    // (tools: several KB) and the worker's (a call) both go to a closed socket
    for (int i = 0; i < 20; ++i)
    {
        const int fd = wire::connectTo(dir, why);
        REQUIRE(fd >= 0);
        const std::string ask = "{\"id\":1,\"method\":\"tools\"}\n{\"id\":2,\"method\":\"call\",\"params\":{\"name\":\"get_status\"}}\n";
        wire::sendAll(fd, ask.data(), ask.size());
        close(fd);
        for (int f = 0; f < 3; ++f)
            app.rig.frame();
    }
    // the link sees every one go
    for (int f = 0; f < 600 && (app.link.view().connections != 0 || app.link.view().running); ++f)
    {
        app.rig.frame();
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    CHECK(app.link.view().connections == 0);
    CHECK_FALSE(app.link.view().running);
    // a peer still connected while the switch goes off, and writing after: the app ends nothing but the link
    const int late = wire::connectTo(dir, why);
    REQUIRE(late >= 0);
    for (int f = 0; f < 5; ++f)
        app.rig.frame();
    app.link.stop();
    const std::string more = std::string(64 * 1024, 'x') + "\n";
    CHECK_FALSE(wire::sendAll(late, more.data(), more.size()));   // the link is gone: an error, not a signal
    close(late);
    CHECK(watch.count() == before);
    // and the link starts again and answers
    REQUIRE(app.link.start(why));
    Bridge bridge(dir);
    CHECK(bridge.ask(&app.rig, 1, "ping").contains("result"));
    CHECK(watch.count() == before);
}

#endif
