// assistant/include/tee/assistant/Transport.h: how the key chat reaches the Claude API (docs/plan/w3-a-assistant.md, "The
// key path"): one HTTP request, its body handed on as it arrives (a stream), cancellable at any moment. The chat's loop
// (A3's turn) is written against this interface; its tests replay recorded-shape streams through a scripted one, and
// only makeTransport()'s, libcurl's, opens a connection.
//
// What the real one promises (each tested on a loopback server, tests/assistant/TransportTest.cpp):
//   - https only, certificates and host verified; plain http only to the loopback (127.0.0.1, ::1, localhost), the
//     tests' server. Anything else is refused before a connection is made.
//   - no redirect followed: a 3xx is an error that names no place;
//   - nothing verbose: no header, and so no key, is ever printed or kept in an error;
//   - a stalled or cut connection is an error, and so is a stream cut mid-body;
//   - cancel (the flag) stops it within a few tens of milliseconds, wherever it is.
//
// Threads: one request at a time per Transport, on the caller's (the chat's worker) thread.
#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace tee::assistant
{
    struct HttpRequest
    {
        std::string url;
        std::vector<std::pair<std::string, std::string>> headers;   // name, value
        std::string body;                                            // POST's; empty for GET
        double connectSeconds = 15.0;
        double stallSeconds = 60.0;      // no byte for this long: an error
        double totalSeconds = 900.0;     // the whole request, however it goes
    };

    struct HttpResponse
    {
        int status = 0;                  // 0: no answer at all
        std::vector<std::pair<std::string, std::string>> headers;   // names in lower case
        bool cancelled = false;
        std::string error;               // why it failed: the transport's own words, never a header or a key

        // A header's value ("" when absent); `name` in lower case.
        std::string header(std::string_view name) const
        {
            for (const auto& [n, v] : headers)
                if (n == name)
                    return v;
            return {};
        }
    };

    // Body bytes as they arrive, with the status already known. Return false to stop (the response is then cancelled).
    using BodySink = std::function<bool(int status, std::string_view chunk)>;

    class Transport
    {
    public:
        virtual ~Transport() = default;
        // True when the exchange completed (whatever its status); false with `out.error` or `out.cancelled`.
        virtual bool post(const HttpRequest& request, const BodySink& body, HttpResponse& out,
                          const std::atomic<bool>* cancel) = 0;
        virtual bool get(const HttpRequest& request, const BodySink& body, HttpResponse& out,
                         const std::atomic<bool>* cancel) = 0;
    };

    // The platform's: libcurl (macOS's SDK, the system's on Linux). Null, with `why` in words, where there is none in
    // this build (Windows in this version; a Linux build without libcurl's headers).
    std::unique_ptr<Transport> makeTransport(std::string& why);

    // Whether `url` may be asked at all: https, or http to the loopback. False, with `why`.
    bool urlAllowed(std::string_view url, std::string& why);
}
