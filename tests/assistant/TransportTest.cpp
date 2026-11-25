// tests/assistant/TransportTest.cpp: the key chat's HTTP (assistant/.../Transport.h) against a server of the test's own
// on the loopback: a stream delivered as it comes, a stream cut mid-body, a cancel mid-stream, a redirect refused, an
// error status with its retry-after. Nothing leaves the machine; no key exists here (the header carries a placeholder).
#include "tee/assistant/Transport.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

TEST_CASE("assistant: only https, or plain http to this machine, is ever asked", "[assistant]")
{
    using tee::assistant::urlAllowed;
    std::string why;
    CHECK(urlAllowed("https://api.anthropic.com/v1/messages", why));
    CHECK(urlAllowed("http://127.0.0.1:8080/v1/messages", why));
    CHECK(urlAllowed("http://localhost:1/x", why));
    CHECK(urlAllowed("http://[::1]:9/x", why));
    for (const char* bad : {"http://api.anthropic.com/v1/messages", "ftp://127.0.0.1/", "file:///etc/passwd",
                            "http://127.0.0.1.evil.example/", "http://localhost.evil.example/", "gopher://x"})
    {
        INFO(bad);
        CHECK_FALSE(urlAllowed(bad, why));
    }
}

#if defined(TEE_TEST_CURL) && !defined(_WIN32)   // (POSIX sockets: macOS and Linux)

 #include <arpa/inet.h>
 #include <netinet/in.h>
 #include <sys/socket.h>
 #include <unistd.h>

 #include <atomic>
 #include <chrono>
 #include <csignal>
 #include <functional>
 #include <thread>
 #include <vector>

namespace
{
    using namespace tee::assistant;

    // An HTTP server on 127.0.0.1 that answers each request with `respond` (given the socket and the request's text).
    class Loopback
    {
    public:
        explicit Loopback(std::function<void(int fd, const std::string& request)> respond) : respond_(std::move(respond))
        {
            std::signal(SIGPIPE, SIG_IGN);
            fd_ = socket(AF_INET, SOCK_STREAM, 0);
            REQUIRE(fd_ >= 0);
            const int one = 1;
            setsockopt(fd_, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
            sockaddr_in a{};
            a.sin_family = AF_INET;
            a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
            a.sin_port = 0;
            REQUIRE(bind(fd_, reinterpret_cast<sockaddr*>(&a), sizeof a) == 0);
            REQUIRE(listen(fd_, 4) == 0);
            socklen_t n = sizeof a;
            getsockname(fd_, reinterpret_cast<sockaddr*>(&a), &n);
            port_ = ntohs(a.sin_port);
            thread_ = std::thread([this] { serve(); });
        }
        ~Loopback()
        {
            stop_ = true;
            shutdown(fd_, SHUT_RDWR);
            close(fd_);
            thread_.join();
        }
        std::string url(const std::string& path = "/v1/messages") const
        {
            return "http://127.0.0.1:" + std::to_string(port_) + path;
        }
        int requests() const { return requests_.load(); }
        std::string lastRequest() const { return last_; }

    private:
        void serve()
        {
            while (!stop_)
            {
                const int c = accept(fd_, nullptr, nullptr);
                if (c < 0)
                    return;
                std::string req;
                char buf[4096];
                while (req.find("\r\n\r\n") == std::string::npos)
                {
                    const ssize_t got = read(c, buf, sizeof buf);
                    if (got <= 0)
                        break;
                    req.append(buf, static_cast<std::size_t>(got));
                }
                // the body, by its length
                const auto lenAt = req.find("Content-Length: ");
                if (lenAt != std::string::npos)
                {
                    const std::size_t want = std::stoul(req.substr(lenAt + 16));
                    const std::size_t have = req.size() - (req.find("\r\n\r\n") + 4);
                    std::size_t more = want > have ? want - have : 0;
                    while (more > 0)
                    {
                        const ssize_t got = read(c, buf, std::min(sizeof buf, more));
                        if (got <= 0)
                            break;
                        req.append(buf, static_cast<std::size_t>(got));
                        more -= static_cast<std::size_t>(got);
                    }
                }
                last_ = req;
                ++requests_;
                respond_(c, req);
                close(c);
            }
        }

        std::function<void(int, const std::string&)> respond_;
        int fd_ = -1;
        int port_ = 0;
        std::thread thread_;
        std::atomic<bool> stop_{false};
        std::atomic<int> requests_{0};
        std::string last_;
    };

    void say(int fd, const std::string& s)
    {
        [[maybe_unused]] const ssize_t n = write(fd, s.data(), s.size());
    }

    std::string chunk(const std::string& s)
    {
        char size[32];
        std::snprintf(size, sizeof size, "%zx\r\n", s.size());
        return size + s + "\r\n";
    }

    const char* const kStreamHead = "HTTP/1.1 200 OK\r\nContent-Type: text/event-stream\r\nTransfer-Encoding: chunked\r\n"
                                    "request-id: req_test\r\n\r\n";

    HttpRequest request(const Loopback& server)
    {
        HttpRequest r;
        r.url = server.url();
        r.headers = {{"content-type", "application/json"}, {"x-api-key", "placeholder-not-a-key"}};
        r.body = R"({"stream":true})";
        r.stallSeconds = 5.0;
        r.totalSeconds = 20.0;
        return r;
    }
}

TEST_CASE("assistant: the transport hands on a stream as it arrives", "[assistant]")
{
    Loopback server([](int fd, const std::string&) {
        say(fd, kStreamHead);
        for (int i = 0; i < 5; ++i)
        {
            say(fd, chunk("event: ping\ndata: {\"n\":" + std::to_string(i) + "}\n\n"));
            std::this_thread::sleep_for(std::chrono::milliseconds(30));
        }
        say(fd, "0\r\n\r\n");
    });
    std::string why;
    auto t = makeTransport(why);
    REQUIRE(t != nullptr);
    std::vector<std::string> pieces;
    std::string all;
    HttpResponse out;
    REQUIRE(t->post(request(server),
                    [&](int status, std::string_view c) {
                        CHECK(status == 200);
                        pieces.emplace_back(c);
                        all += c;
                        return true;
                    },
                    out, nullptr));
    CHECK(out.status == 200);
    CHECK(out.header("request-id") == "req_test");
    CHECK(pieces.size() >= 3);   // as it came, not in one piece at the end
    CHECK(all.find("{\"n\":4}") != std::string::npos);
    // what was sent: the body and the headers given, and a POST
    CHECK(server.lastRequest().rfind("POST /v1/messages ", 0) == 0);
    CHECK(server.lastRequest().find(R"({"stream":true})") != std::string::npos);
}

TEST_CASE("assistant: a stream cut mid-body is an error, and says nothing of the request", "[assistant]")
{
    Loopback server([](int fd, const std::string&) {
        say(fd, kStreamHead);
        say(fd, chunk("event: message_start\ndata: {}\n\n"));
        say(fd, "40\r\nevent: content_block_delta\n");   // a chunk promised 64 bytes; the connection ends
        shutdown(fd, SHUT_RDWR);
    });
    std::string why;
    auto t = makeTransport(why);
    std::string all;
    HttpResponse out;
    CHECK_FALSE(t->post(request(server), [&](int, std::string_view c) { all += c; return true; }, out, nullptr));
    CHECK_FALSE(out.cancelled);
    CHECK_FALSE(out.error.empty());
    CHECK(out.error.find("placeholder-not-a-key") == std::string::npos);
    CHECK(all.find("message_start") != std::string::npos);   // what came before the cut was handed on
}

TEST_CASE("assistant: a cancel stops the transport mid-stream at once", "[assistant]")
{
    std::atomic<bool> serverDone{false};
    Loopback server([&](int fd, const std::string&) {
        say(fd, kStreamHead);
        for (int i = 0; i < 200 && !serverDone; ++i)
        {
            say(fd, chunk("event: ping\ndata: {}\n\n"));
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    std::string why;
    auto t = makeTransport(why);
    std::atomic<bool> cancel{false};
    int got = 0;
    std::thread canceller([&] {
        while (got == 0)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        cancel = true;
    });
    const auto t0 = std::chrono::steady_clock::now();
    HttpResponse out;
    CHECK_FALSE(t->post(request(server), [&](int, std::string_view) { ++got; return true; }, out, &cancel));
    const auto took = std::chrono::steady_clock::now() - t0;
    canceller.join();
    serverDone = true;
    CHECK(out.cancelled);
    CHECK(took < std::chrono::seconds(2));   // not the server's ten seconds
}

TEST_CASE("assistant: a redirect is refused: not followed, its body and its place not handed on", "[assistant]")
{
    Loopback server([](int fd, const std::string&) {
        say(fd, "HTTP/1.1 302 Found\r\nLocation: http://127.0.0.1:1/elsewhere\r\nContent-Length: 5\r\n\r\nmoved");
    });
    std::string why;
    auto t = makeTransport(why);
    bool bodySeen = false;
    HttpResponse out;
    CHECK_FALSE(t->post(request(server), [&](int, std::string_view) { bodySeen = true; return true; }, out, nullptr));
    CHECK(out.error.find("redirect") != std::string::npos);
    CHECK(out.error.find("elsewhere") == std::string::npos);
    CHECK(out.header("location").empty());
    CHECK_FALSE(bodySeen);
    CHECK(server.requests() == 1);
}

TEST_CASE("assistant: an error status is an answer: its body and its retry-after handed on", "[assistant]")
{
    const std::string body = R"({"type":"error","error":{"type":"rate_limit_error","message":"slow down"}})";
    Loopback server([&](int fd, const std::string&) {
        say(fd, "HTTP/1.1 429 Too Many Requests\r\nContent-Type: application/json\r\nretry-after: 7\r\nContent-Length: "
                    + std::to_string(body.size()) + "\r\n\r\n" + body);
    });
    std::string why;
    auto t = makeTransport(why);
    std::string got;
    int status = 0;
    HttpResponse out;
    REQUIRE(t->post(request(server), [&](int s, std::string_view c) { status = s; got += c; return true; }, out, nullptr));
    CHECK(out.status == 429);
    CHECK(status == 429);
    CHECK(out.header("retry-after") == "7");
    CHECK(got == body);
}

TEST_CASE("assistant: a plain http address off this machine is refused before any connection", "[assistant]")
{
    std::string why;
    auto t = makeTransport(why);
    HttpRequest r;
    r.url = "http://api.anthropic.com/v1/messages";
    HttpResponse out;
    bool called = false;
    CHECK_FALSE(t->post(r, [&](int, std::string_view) { called = true; return true; }, out, nullptr));
    CHECK(out.status == 0);
    CHECK_FALSE(called);
    CHECK(out.error.find("https") != std::string::npos);
}

#endif
