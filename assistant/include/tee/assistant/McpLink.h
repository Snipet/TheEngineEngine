// assistant/include/tee/assistant/McpLink.h: the running app, reachable by Claude Desktop's MCP server (the bridge,
// assistant/mcp/Main.cpp) over a Unix socket in a private folder (McpWire.h). It exists only while the owner's switch
// "Allow Claude Desktop" is on: start() makes the socket, stop() removes it and closes every connection. Off, the app
// listens on nothing.
//
// Two threads of its own. The poll thread accepts connections (each peer's user checked: another user's is closed
// unanswered), reads their lines and answers at once what never waits: hello, tools, cancel. Each call goes to the
// worker, one at a time in the order they came, and runs as a turn of Claude Desktop (Tools.h: Turn, Driver::Desktop),
// so the chat and Claude Desktop never drive at once. A call that changed the build or the library leaves a card
// (Cards.h). A cancel stops its call: one waiting is answered "cancelled" without running, the running one through its
// flag. A connection that closes takes its calls with it.
//
// Replies are whole lines written under the connection's own lock; a peer that stops reading for 5 s is dropped, so
// neither thread ever waits on it for longer.
//
// Threads: start(), stop() and view() from the frame's thread (or any one thread). The tools wait for the frame (the
// inbox): stop() must be called before the frame stops draining it. Destroyed: stopped.
#pragma once

#include "tee/assistant/Cards.h"
#include "tee/assistant/Tools.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace tee::assistant
{
    struct McpLinkOptions
    {
        std::string folder;          // empty: wire::folder()
        long expectedUser = -1;      // the peer's user allowed; -1: this process's (a test names another)
        std::string version = "0.1.0";
    };

    class McpLink
    {
    public:
        using Options = McpLinkOptions;

        McpLink(Tools& tools, Drivers& drivers, Cards& cards, std::function<double()> clock, Options options = {});
        ~McpLink();
        McpLink(const McpLink&) = delete;
        McpLink& operator=(const McpLink&) = delete;

        // The switch on: the socket made, the threads started. False, with `why`, when it cannot be (another app holds
        // the socket, the folder is not private, Windows). Already on: true.
        bool start(std::string& why);
        // The switch off: the socket gone, every connection closed, the running call cancelled and waited for.
        void stop();

        struct View
        {
            bool listening = false;
            int connections = 0;         // bridges connected now
            bool running = false;        // a call runs now
            std::string tool;            // the one running, else the last
            std::uint64_t calls = 0;     // calls answered since start
            std::uint64_t refused = 0;   // connections refused for their user
        };
        View view() const;

    private:
        struct Client;
        struct Job
        {
            std::shared_ptr<Client> client;
            std::string id;              // the request's id, as JSON text
            std::string name;
            nlohmann::json arguments;
            std::shared_ptr<std::atomic<bool>> cancel;
        };

        void poll();
        void work();
        void request(const std::shared_ptr<Client>& client, const std::string& line);
        void closed(const std::shared_ptr<Client>& client);
        void answer(Client& client, const std::string& id, const nlohmann::json& result);
        void refuse(Client& client, const std::string& id, const std::string& why);
        void wake();
        nlohmann::json toolList() const;
        void remember(const Job& job, const Result& r);

        Tools& tools_;
        Drivers& drivers_;
        Cards& cards_;
        std::function<double()> clock_;
        Options options_;
        std::string dir_, path_;
        int listen_ = -1;
        int wake_[2] = {-1, -1};
        std::atomic<bool> stop_{false};
        std::thread poller_, worker_;

        mutable std::mutex jobMutex_;
        std::condition_variable jobCv_;
        std::deque<Job> jobs_;
        std::shared_ptr<Job> running_;   // the call running (jobMutex_)
        bool workerStop_ = false;

        mutable std::mutex viewMutex_;
        View view_;
    };
}
