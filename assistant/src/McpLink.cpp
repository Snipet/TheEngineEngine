// assistant/src/McpLink.cpp: the app's end of Claude Desktop's link (McpLink.h, McpWire.h).
#include "tee/assistant/McpLink.h"
#include "tee/assistant/Json.h"

#include "tee/assistant/McpWire.h"

#include <algorithm>
#include <cstdio>
#include <sstream>

#if !defined(_WIN32)
 #include <poll.h>
 #include <sys/time.h>
#endif

namespace tee::assistant
{
    using nlohmann::json;

    struct McpLink::Client
    {
        int fd = -1;
        std::mutex write;                // one whole line at a time; fd closed under it
        wire::Lines lines;
    };

    namespace
    {
        // A tool's MCP annotations, honest about what it does (TestingPlayground's chat-audit DOCS-2): read-only only
        // when it changes nothing at all; a load replaces the open build (after the owner said yes when it had unsaved
        // changes); nothing reaches outside the machine.
        json annotations(const ToolInfo& t)
        {
            return {{"title", t.title},
                    {"readOnlyHint", t.readOnly},
                    {"destructiveHint", t.name == "load_build"},
                    {"idempotentHint", t.readOnly},
                    {"openWorldHint", false}};
        }

        std::string brief(const json& v)
        {
            if (v.is_string())
                return v.get<std::string>();
            return dumpText(v);
        }

        // What a card says of a call that changed something.
        std::string summaryOf(const std::string& tool, const json& value)
        {
            if (tool == "set_fields")
            {
                std::string s;
                for (const json& c : value.value("changed", json::array()))
                {
                    if (!s.empty())
                        s += "; ";
                    s += c.value("path", std::string()) + " " + brief(c.value("before", json())) + " -> "
                         + brief(c.value("after", json()));
                    if (s.size() > 400)
                    {
                        s += "; ...";
                        break;
                    }
                }
                return s;
            }
            if (tool == "load_build")
                return "loaded \"" + value.value("loaded", std::string()) + "\"";
            if (tool == "save_build_as")
                return "saved as \"" + value.value("saved", std::string()) + "\"";
            return tool;
        }
    }

    McpLink::McpLink(Tools& tools, Drivers& drivers, Cards& cards, std::function<double()> clock, Options options)
        : tools_(tools),
#if !defined(_WIN32)
          drivers_(drivers),
#endif
          cards_(cards), clock_(std::move(clock)), options_(std::move(options))
    {
#if defined(_WIN32)
        (void)drivers;   // no link on Windows in this version: nothing drives through it
#endif
    }

    McpLink::~McpLink()
    {
        stop();
    }

    McpLink::View McpLink::view() const
    {
        std::lock_guard<std::mutex> lock(viewMutex_);
        return view_;
    }

    json McpLink::toolList() const
    {
        json list = json::array();
        for (const ToolInfo& t : tools_.list())
            list.push_back({{"name", t.name},
                            {"title", t.title},
                            {"description", t.description},
                            {"inputSchema", t.inputSchema},
                            {"annotations", annotations(t)}});
        return list;
    }

    void McpLink::remember(const Job& job, const Result& r)
    {
        {
            std::lock_guard<std::mutex> lock(viewMutex_);
            ++view_.calls;
        }
        if (r.isError)
            return;
        if (job.name != "set_fields" && job.name != "load_build" && job.name != "save_build_as")
            return;
        if (job.name == "set_fields" && !r.value.contains("undoStep"))
            return;   // nothing changed
        Card c;
        c.tool = job.name;
        c.summary = summaryOf(job.name, r.value);
        if (job.name == "set_fields")
            c.step = r.value["undoStep"].get<std::uint64_t>();
        cards_.add(std::move(c));
    }

#if defined(_WIN32)

    bool McpLink::start(std::string& why)
    {
        why = wire::kNoLink;
        return false;
    }

    void McpLink::stop() {}
    void McpLink::poll() {}
    void McpLink::work() {}
    void McpLink::request(const std::shared_ptr<Client>&, const std::string&) {}
    void McpLink::closed(const std::shared_ptr<Client>&) {}
    void McpLink::answer(Client&, const std::string&, const json&) {}
    void McpLink::refuse(Client&, const std::string&, const std::string&) {}
    void McpLink::wake() {}

#else

    bool McpLink::start(std::string& why)
    {
        if (listen_ >= 0)
            return true;
        dir_ = !options_.folder.empty() ? options_.folder : !options_.base.empty() ? wire::folderIn(options_.base) : wire::folder();
        if (!wire::makePrivate(dir_, why))
            return false;
        path_ = wire::socketIn(dir_);
        sockaddr_un a{};
        if (!wire::address(path_, a))
        {
            why = "the link's path is too long";
            return false;
        }
        // another app already listening there: leave it be; a socket left by one that ended: removed
        std::string ignored;
        if (const int other = wire::connectTo(dir_, ignored); other >= 0)
        {
            close(other);
            why = "another TheEngineEngine already has Claude Desktop's link";
            return false;
        }
        unlink(path_.c_str());
        const int fd = wire::newSocket();
        if (fd < 0)
        {
            why = "no socket";
            return false;
        }
        const mode_t was = umask(077);
        const bool bound = bind(fd, reinterpret_cast<const sockaddr*>(&a), sizeof a) == 0;
        umask(was);
        // The system's largest backlog: macOS refuses a connect at once (ECONNREFUSED) when the queue is full, where
        // Linux holds it, and the poll thread accepts only as fast as it is woken: a few bridges (or one reconnecting
        // as Claude Desktop restarts it) must never be turned away for arriving together.
        if (!bound || ::listen(fd, SOMAXCONN) != 0 || !wire::wakePair(wake_))
        {
            close(fd);
            unlink(path_.c_str());
            why = "the link's socket could not be made";
            return false;
        }
        listen_ = fd;
        stop_ = false;
        {
            std::lock_guard<std::mutex> lock(jobMutex_);
            workerStop_ = false;
        }
        {
            std::lock_guard<std::mutex> lock(viewMutex_);
            view_ = View{};
            view_.listening = true;
        }
        poller_ = std::thread([this] { poll(); });
        worker_ = std::thread([this] { work(); });
        return true;
    }

    void McpLink::stop()
    {
        if (listen_ < 0)
            return;
        stop_ = true;
        wake();
        if (poller_.joinable())
            poller_.join();   // it closes every connection, cancelling their calls
        {
            std::lock_guard<std::mutex> lock(jobMutex_);
            workerStop_ = true;
            jobs_.clear();
            if (running_)
                running_->cancel->store(true);
        }
        jobCv_.notify_all();
        if (worker_.joinable())
            worker_.join();
        close(listen_);
        unlink(path_.c_str());
        listen_ = -1;
        close(wake_[0]);
        close(wake_[1]);
        wake_[0] = wake_[1] = -1;
        std::lock_guard<std::mutex> lock(viewMutex_);
        view_.listening = false;
        view_.connections = 0;
        view_.running = false;
    }

    void McpLink::wake()
    {
        // (a socket pair, not a pipe: sendAll, never a signal; a full or closed pair needs no second wake)
        if (wake_[1] >= 0)
        {
            const char c = 1;
            wire::sendAll(wake_[1], &c, 1);
        }
    }

    void McpLink::poll()
    {
        std::vector<std::shared_ptr<Client>> clients;
        const uid_t allowed = options_.expectedUser >= 0 ? static_cast<uid_t>(options_.expectedUser) : geteuid();
        while (!stop_)
        {
            std::vector<pollfd> fds;
            fds.push_back({wake_[0], POLLIN, 0});
            fds.push_back({listen_, POLLIN, 0});
            for (const auto& c : clients)
                fds.push_back({c->fd, POLLIN, 0});
            if (::poll(fds.data(), static_cast<nfds_t>(fds.size()), 1000) < 0)
            {
                if (errno == EINTR)
                    continue;
                break;
            }
            if (fds[0].revents != 0)
            {
                char drain[64];
                while (read(wake_[0], drain, sizeof drain) > 0)
                {
                }
            }
            const std::size_t known = clients.size();   // those fds[] has; a client accepted now waits a turn
            if ((fds[1].revents & POLLIN) != 0)
            {
                const int fd = wire::acceptFrom(listen_);
                if (fd >= 0)
                {
                    uid_t uid = 0;
                    if (!wire::peerUser(fd, uid) || uid != allowed)
                    {
                        close(fd);   // another user's: closed unanswered
                        std::lock_guard<std::mutex> lock(viewMutex_);
                        ++view_.refused;
                    }
                    else
                    {
                        timeval tv{5, 0};
                        setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv);   // (SO_NOSIGPIPE: acceptFrom's)
                        auto c = std::make_shared<Client>();
                        c->fd = fd;
                        clients.push_back(c);
                        std::lock_guard<std::mutex> lock(viewMutex_);
                        view_.connections = static_cast<int>(clients.size());
                        ++view_.accepted;
                    }
                }
            }
            std::vector<std::shared_ptr<Client>> gone;
            for (std::size_t i = 0; i < known; ++i)
            {
                const short ev = fds[i + 2].revents;
                if (ev == 0)
                    continue;
                const auto& c = clients[i];
                char buf[16384];
                const ssize_t n = (ev & (POLLIN | POLLHUP)) != 0 ? read(c->fd, buf, sizeof buf) : -1;
                if (n <= 0)
                {
                    gone.push_back(c);
                    continue;
                }
                c->lines.add(buf, static_cast<std::size_t>(n));
                if (c->lines.overflow())
                {
                    gone.push_back(c);
                    continue;
                }
                std::string line;
                while (c->lines.next(line))
                    if (!line.empty())
                    {
                        try
                        {
                            request(c, line);
                        }
                        catch (const std::exception&)   // the last guard (X-063): answered, and the next line read
                        {
                            refuse(*c, "null", "the app could not answer this request");
                        }
                    }
            }
            for (const auto& c : gone)
            {
                closed(c);
                clients.erase(std::remove(clients.begin(), clients.end(), c), clients.end());
                std::lock_guard<std::mutex> lock(viewMutex_);
                view_.connections = static_cast<int>(clients.size());
            }
        }
        for (const auto& c : clients)
            closed(c);
    }

    void McpLink::closed(const std::shared_ptr<Client>& client)
    {
        {
            std::lock_guard<std::mutex> lock(jobMutex_);
            jobs_.erase(std::remove_if(jobs_.begin(), jobs_.end(), [&](const Job& j) { return j.client == client; }),
                        jobs_.end());
            if (running_ && running_->client == client)
                running_->cancel->store(true);
        }
        std::lock_guard<std::mutex> lock(client->write);
        if (client->fd >= 0)
        {
            close(client->fd);
            client->fd = -1;
        }
    }

    void McpLink::answer(Client& client, const std::string& id, const json& result)
    {
        const std::string line = "{\"id\":" + id + ",\"result\":" + dumpText(result) + "}\n";
        std::lock_guard<std::mutex> lock(client.write);
        if (client.fd >= 0 && !wire::sendAll(client.fd, line.data(), line.size()))
            shutdown(client.fd, SHUT_RDWR);   // gone, or not reading: the poll thread sees it close and drops it
    }

    void McpLink::refuse(Client& client, const std::string& id, const std::string& why)
    {
        const std::string line = "{\"id\":" + id + ",\"error\":" + dumpText(json(why)) + "}\n";
        std::lock_guard<std::mutex> lock(client.write);
        if (client.fd >= 0)
            wire::sendAll(client.fd, line.data(), line.size());   // (a refusal to a peer gone: nothing more to do)
    }

    void McpLink::request(const std::shared_ptr<Client>& client, const std::string& line)
    {
        const json msg = json::parse(line, nullptr, false);
        if (!msg.is_object() || !msg.contains("id") || !(msg["id"].is_number_integer() || msg["id"].is_string())
            || !msg.contains("method") || !msg["method"].is_string())
        {
            refuse(*client, "null", "not a request");
            return;
        }
        const std::string id = dumpText(msg["id"]);
        const std::string method = msg["method"].get<std::string>();
        const json params = msg.contains("params") && msg["params"].is_object() ? msg["params"] : json::object();
        if (method == "hello")
            answer(*client, id, {{"app", "TheEngineEngine"}, {"version", options_.version}});
        else if (method == "tools")
            answer(*client, id, {{"tools", toolList()}});
        else if (method == "cancel")
        {
            const std::string which = params.contains("id") ? dumpText(params["id"]) : std::string();
            bool found = false;
            Job dropped;
            {
                std::lock_guard<std::mutex> lock(jobMutex_);
                for (auto it = jobs_.begin(); it != jobs_.end(); ++it)
                    if (it->client == client && it->id == which)
                    {
                        dropped = std::move(*it);
                        jobs_.erase(it);
                        found = true;
                        break;
                    }
                if (!found && running_ && running_->client == client && running_->id == which)
                {
                    running_->cancel->store(true);
                    found = true;
                }
            }
            if (dropped.client)
                answer(*client, which,
                       {{"content", json::array({{{"type", "text"}, {"text", dumpText(json({{"error", kCancelled}}))}}})},
                        {"isError", true}});
            answer(*client, id, {{"cancelled", found}});
        }
        else if (method == "call")
        {
            if (!params.contains("name") || !params["name"].is_string())
            {
                refuse(*client, id, "a call names its tool");
                return;
            }
            if (tools_.find(params["name"].get<std::string>()) == nullptr)
            {
                // an unknown tool is a protocol error in MCP (-32602), not a tool's failure: the bridge says so
                const std::string out = "{\"id\":" + id + ",\"error\":" + dumpText(json("no tool named " + params["name"].get<std::string>().substr(0, 64)))
                                         + ",\"code\":-32602}\n";
                std::lock_guard<std::mutex> lock(client->write);
                if (client->fd >= 0)
                    wire::sendAll(client->fd, out.data(), out.size());
                return;
            }
            Job j{client, id, params["name"].get<std::string>(),
                  params.contains("arguments") ? params["arguments"] : json::object(),
                  std::make_shared<std::atomic<bool>>(false)};
            {
                std::lock_guard<std::mutex> lock(jobMutex_);
                if (jobs_.size() >= 16)
                {
                    refuse(*client, id, "too many calls waiting");
                    return;
                }
                jobs_.push_back(std::move(j));
            }
            jobCv_.notify_one();
        }
        else
            refuse(*client, id, "no method \"" + method.substr(0, 40) + "\"");
    }

    void McpLink::work()
    {
        for (;;)
        {
            std::shared_ptr<Job> job;
            {
                std::unique_lock<std::mutex> lock(jobMutex_);
                jobCv_.wait(lock, [&] { return workerStop_ || !jobs_.empty(); });
                if (workerStop_)
                    return;
                job = std::make_shared<Job>(std::move(jobs_.front()));
                jobs_.pop_front();
                running_ = job;
            }
            {
                std::lock_guard<std::mutex> lock(viewMutex_);
                view_.running = true;
                view_.tool = job->name;
            }
            Result r;
            // the last guard (X-063): whatever a call throws ends that call, answered in words, never the app; the
            // worker serves the next
            try
            {
                Turn turn(tools_, drivers_, Driver::Desktop, clock_);   // one call: one turn of Claude Desktop's
                r = turn.call(job->name, job->arguments, job->cancel.get());
            }
            catch (const std::exception&)
            {
                r = Result{true, {{"error", "the app could not complete this call; nothing more was done"}}};
            }
            remember(*job, r);
            answer(*job->client, job->id,
                   {{"content", json::array({{{"type", "text"}, {"text", r.text()}}})}, {"isError", r.isError}});
            {
                std::lock_guard<std::mutex> lock(jobMutex_);
                running_.reset();
            }
            std::lock_guard<std::mutex> lock(viewMutex_);
            view_.running = false;
        }
    }

#endif
}
