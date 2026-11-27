// assistant/src/Transport.cpp: the key chat's HTTP (Transport.h): libcurl where the build has it (TEE_HAS_CURL), else a
// transport that refuses in words.
#include "tee/assistant/Transport.h"

#include <algorithm>
#include <cctype>
#include <chrono>

#if defined(TEE_HAS_CURL)
 #include <curl/curl.h>
 #include <mutex>
#endif

namespace tee::assistant
{
    namespace
    {
        std::string lower(std::string s)
        {
            for (char& c : s)
                c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
            return s;
        }
    }

    bool urlAllowed(std::string_view url, std::string& why)
    {
        why = "only https is used (plain http to this machine alone)";
        const std::string u = lower(std::string(url));
        const bool secure = u.rfind("https://", 0) == 0;
        if (!secure && u.rfind("http://", 0) != 0)
            return false;
        // the authority: up to the path, the query or the fragment
        const std::string rest = u.substr(secure ? 8 : 7);
        const std::string authority = rest.substr(0, rest.find_first_of("/?#"));
        // no user part ("http://localhost:80@elsewhere/" is elsewhere's), nothing a parser may read another way
        for (const char c : authority)
            if (c == '@' || c == '\\' || c == '%' || static_cast<unsigned char>(c) <= ' ' || c == 127)
            {
                why = "the address has a part that could name another host: refused";
                return false;
            }
        if (authority.empty())
            return false;
        if (secure)
            return true;
        // plain http: this machine alone, by its exact names
        std::string host = authority, port;
        if (host.rfind("[::1]", 0) == 0)
        {
            port = host.substr(5);
            host = "[::1]";
        }
        else if (const auto colon = host.find(':'); colon != std::string::npos)
        {
            port = host.substr(colon);
            host = host.substr(0, colon);
        }
        if (!port.empty() && (port.size() < 2 || port[0] != ':'
                              || port.find_first_not_of("0123456789", 1) != std::string::npos))
            return false;
        return host == "127.0.0.1" || host == "localhost" || host == "[::1]";
    }

#if defined(TEE_HAS_CURL)

    namespace
    {
        // curl_global_init once, for the process (never cleaned up: the process's end does it).
        bool curlStarted()
        {
            static const bool ok = curl_global_init(CURL_GLOBAL_DEFAULT) == CURLE_OK;
            return ok;
        }

        struct Exchange
        {
            const BodySink* body = nullptr;
            HttpResponse* out = nullptr;
            const std::atomic<bool>* cancel = nullptr;
            bool stoppedBySink = false;
            bool redirect = false;
            std::chrono::steady_clock::time_point deadline;
        };

        std::size_t onHeader(char* data, std::size_t size, std::size_t n, void* user)
        {
            auto& x = *static_cast<Exchange*>(user);
            const std::size_t len = size * n;
            std::string line(data, len);
            while (!line.empty() && (line.back() == '\n' || line.back() == '\r'))
                line.pop_back();
            if (line.rfind("HTTP/", 0) == 0)
            {
                // a new status line (a 100 Continue before the real one): the headers start again
                x.out->headers.clear();
                const auto sp = line.find(' ');
                x.out->status = sp == std::string::npos ? 0 : std::atoi(line.c_str() + sp + 1);
            }
            else if (const auto colon = line.find(':'); colon != std::string::npos)
            {
                std::string value = line.substr(colon + 1);
                const auto first = value.find_first_not_of(" \t");
                value = first == std::string::npos ? std::string() : value.substr(first);
                x.out->headers.emplace_back(lower(line.substr(0, colon)), value);
            }
            return len;
        }

        std::size_t onBody(char* data, std::size_t size, std::size_t n, void* user)
        {
            auto& x = *static_cast<Exchange*>(user);
            const std::size_t len = size * n;
            if (x.cancel != nullptr && x.cancel->load())
                return 0;   // curl stops: a write error, which is reported as cancelled
            if (x.out->status >= 300 && x.out->status < 400)
            {
                x.redirect = true;   // its body (and its Location) never reaches the caller
                return 0;
            }
            if (*x.body && !(*x.body)(x.out->status, std::string_view(data, len)))
            {
                x.stoppedBySink = true;
                return 0;
            }
            return len;
        }

        int onProgress(void* user, curl_off_t, curl_off_t, curl_off_t, curl_off_t)
        {
            auto& x = *static_cast<Exchange*>(user);
            if (x.cancel != nullptr && x.cancel->load())
                return 1;
            return std::chrono::steady_clock::now() > x.deadline ? 1 : 0;
        }

        class CurlTransport final : public Transport
        {
        public:
            bool post(const HttpRequest& r, const BodySink& b, HttpResponse& out, const std::atomic<bool>* c) override
            {
                return run(r, true, b, out, c);
            }
            bool get(const HttpRequest& r, const BodySink& b, HttpResponse& out, const std::atomic<bool>* c) override
            {
                return run(r, false, b, out, c);
            }

        private:
            static bool run(const HttpRequest& r, bool post, const BodySink& body, HttpResponse& out,
                            const std::atomic<bool>* cancel)
            {
                out = HttpResponse{};
                std::string why;
                if (!urlAllowed(r.url, why))
                {
                    out.error = why;
                    return false;
                }
                if (!curlStarted())
                {
                    out.error = "the HTTP library did not start";
                    return false;
                }
                CURL* h = curl_easy_init();
                if (h == nullptr)
                {
                    out.error = "the HTTP library did not start";
                    return false;
                }
                Exchange x;
                x.body = &body;
                x.out = &out;
                x.cancel = cancel;
                x.deadline = std::chrono::steady_clock::now()
                             + std::chrono::milliseconds(static_cast<long long>(r.totalSeconds * 1000.0));
                curl_slist* headers = nullptr;
                for (const auto& [n, v] : r.headers)
                    headers = curl_slist_append(headers, (n + ": " + v).c_str());
                curl_easy_setopt(h, CURLOPT_URL, r.url.c_str());
                curl_easy_setopt(h, CURLOPT_HTTPHEADER, headers);
                if (post)
                {
                    curl_easy_setopt(h, CURLOPT_POST, 1L);
                    curl_easy_setopt(h, CURLOPT_POSTFIELDS, r.body.data());
                    curl_easy_setopt(h, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(r.body.size()));
                }
                else
                    curl_easy_setopt(h, CURLOPT_HTTPGET, 1L);
                curl_easy_setopt(h, CURLOPT_FOLLOWLOCATION, 0L);       // never followed: a 3xx is refused below
                curl_easy_setopt(h, CURLOPT_SSL_VERIFYPEER, 1L);
                curl_easy_setopt(h, CURLOPT_SSL_VERIFYHOST, 2L);
                curl_easy_setopt(h, CURLOPT_VERBOSE, 0L);              // nothing printed: no header, no key
                curl_easy_setopt(h, CURLOPT_NOSIGNAL, 1L);
                curl_easy_setopt(h, CURLOPT_NOPROXY, "127.0.0.1,localhost,::1");   // the loopback never through a proxy
                curl_easy_setopt(h, CURLOPT_CONNECTTIMEOUT_MS, static_cast<long>(r.connectSeconds * 1000.0));
                curl_easy_setopt(h, CURLOPT_LOW_SPEED_LIMIT, 1L);      // a stall: under 1 byte/s for stallSeconds
                curl_easy_setopt(h, CURLOPT_LOW_SPEED_TIME, static_cast<long>(r.stallSeconds));
                curl_easy_setopt(h, CURLOPT_HEADERFUNCTION, onHeader);
                curl_easy_setopt(h, CURLOPT_HEADERDATA, &x);
                curl_easy_setopt(h, CURLOPT_WRITEFUNCTION, onBody);
                curl_easy_setopt(h, CURLOPT_WRITEDATA, &x);
                curl_easy_setopt(h, CURLOPT_NOPROGRESS, 0L);
                curl_easy_setopt(h, CURLOPT_XFERINFOFUNCTION, onProgress);
                curl_easy_setopt(h, CURLOPT_XFERINFODATA, &x);
 #if LIBCURL_VERSION_NUM >= 0x075500
                curl_easy_setopt(h, CURLOPT_PROTOCOLS_STR, "https,http");
 #endif
                const CURLcode rc = curl_easy_perform(h);
                curl_slist_free_all(headers);
                curl_easy_cleanup(h);
                if (cancel != nullptr && cancel->load())
                {
                    out.cancelled = true;
                    out.error = "cancelled";
                    return false;
                }
                if (x.redirect || (out.status >= 300 && out.status < 400))
                {
                    out.headers.clear();   // (a Location names a place: not kept)
                    out.error = "the server answered with a redirect, which is not followed";
                    return false;
                }
                if (x.stoppedBySink)
                {
                    out.cancelled = true;
                    out.error = "stopped";
                    return false;
                }
                if (rc != CURLE_OK)
                {
                    // curl's own short text (never a header: CURLOPT_VERBOSE is off and its error buffer unused)
                    out.error = rc == CURLE_ABORTED_BY_CALLBACK ? std::string("took too long") : curl_easy_strerror(rc);
                    return false;
                }
                return true;
            }
        };
    }

    std::unique_ptr<Transport> makeTransport(std::string& why)
    {
        if (!curlStarted())
        {
            why = "the HTTP library did not start";
            return nullptr;
        }
        return std::make_unique<CurlTransport>();
    }

#else

    std::unique_ptr<Transport> makeTransport(std::string& why)
    {
 #if defined(_WIN32)
        why = "the chat by API key is for macOS and Linux in this version";
 #else
        why = "this build has no HTTP library (libcurl's headers were missing when it was configured)";
 #endif
        return nullptr;
    }

#endif
}
