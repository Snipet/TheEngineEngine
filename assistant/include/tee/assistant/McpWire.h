// assistant/include/tee/assistant/McpWire.h: how the running app (McpLink.h) and the bridge Claude Desktop starts
// (assistant/mcp/Main.cpp, tee-mcp) find and talk to each other. Header only, no JUCE, nothing of the host's, so the
// bridge stays small. The design is TestingPlayground's (Source/mcp/McpProtocol.h at 47bac40), rewritten, with a check
// of the peer's user that it does not have (docs/plan/w3-a-assistant.md, "The MCP path").
//
//   the folder   private to the user (0700, the user's own, no symlink; anything else is refused, never repaired):
//                Linux: $XDG_RUNTIME_DIR/TheEngineEngine-mcp, else /tmp/TheEngineEngine-mcp-<uid>; macOS:
//                $TMPDIR/TheEngineEngine-mcp ($TMPDIR is the user's own there); TEE_MCP_DIR when set (the tests').
//                In it one socket, app.sock, which exists only while the app's "Allow Claude Desktop" is on.
//   the peer     each end checks the other's user (SO_PEERCRED on Linux, getpeereid on macOS): another user's
//                connection is closed unanswered.
//   a request    one JSON object a line: {"id": 1, "method": "hello" | "tools" | "call" | "cancel", "params": {...}}
//                  hello   -> {"app", "version"}
//                  tools   -> {"tools": [MCP tool, ...]}
//                  call    {name, arguments} -> {"content": [{"type": "text", "text"}], "isError"}: one at a time, in
//                          order
//                  cancel  {id} -> {"cancelled": bool}: a waiting call is answered "cancelled", the running one stops
//                          as soon as it can (and still answers)
//   a reply      one line, {"id": 1, "result": ...} or {"id": 1, "error": "why"} (with "code", a JSON-RPC error's, when
//                the bridge must answer its client with one: -32602 for a call of a tool that does not exist), exactly one
//                a request
//
// macOS and Linux. Windows has no link in this version: every function here says so (kNoLink) and the app's switch
// stays off.
#pragma once

#include <cstddef>
#include <cstdlib>
#include <string>

#if !defined(_WIN32)
 #include <cerrno>
 #if defined(__APPLE__)
  #include <fcntl.h>      // (not on Linux: glibc's declares a function tee(), which clashes with namespace tee)
 #endif
 #include <sys/socket.h>
 #include <sys/stat.h>
 #include <sys/types.h>
 #include <sys/un.h>
 #include <unistd.h>
#endif

namespace tee::assistant::wire
{
    inline constexpr const char* kNoLink = "Claude Desktop's link is for macOS and Linux in this version";
    inline constexpr std::size_t kMaxLine = 4u * 1024u * 1024u;   // a request or a reply; the tools' are far smaller

    // The folder of the link (not made here).
    inline std::string folder()
    {
        if (const char* d = std::getenv("TEE_MCP_DIR"); d != nullptr && *d != '\0')
            return d;
#if defined(_WIN32)
        return {};
#elif defined(__APPLE__)
        const char* t = std::getenv("TMPDIR");
        std::string base = t != nullptr && *t != '\0' ? t : "/tmp";
        if (base.back() == '/')
            base.pop_back();
        return base + "/TheEngineEngine-mcp";
#else
        if (const char* x = std::getenv("XDG_RUNTIME_DIR"); x != nullptr && *x == '/')
            return std::string(x) + "/TheEngineEngine-mcp";
        return "/tmp/TheEngineEngine-mcp-" + std::to_string(static_cast<unsigned long>(geteuid()));
#endif
    }

    inline std::string socketIn(const std::string& dir)
    {
        return dir + "/app.sock";
    }

#if !defined(_WIN32)
    // The folder is private: a directory (no symlink), the user's own, no access for the group or others. False, with
    // `why`, otherwise: nothing is changed in a folder that is not.
    inline bool isPrivate(const std::string& dir, std::string& why)
    {
        struct stat st{};
        if (lstat(dir.c_str(), &st) != 0)
        {
            why = "the link's folder is not there";
            return false;
        }
        if (!S_ISDIR(st.st_mode))
        {
            why = "the link's folder is not a folder (a link or a file is refused)";
            return false;
        }
        if (st.st_uid != geteuid())
        {
            why = "the link's folder belongs to another user";
            return false;
        }
        if ((st.st_mode & 077) != 0)
        {
            why = "the link's folder is open to other users";
            return false;
        }
        return true;
    }

    // Makes the folder when it is not there (0700), then checks it as isPrivate does.
    inline bool makePrivate(const std::string& dir, std::string& why)
    {
        if (dir.empty())
        {
            why = "there is no folder for the link";
            return false;
        }
        if (mkdir(dir.c_str(), 0700) != 0 && errno != EEXIST)
        {
            why = "the link's folder could not be made";
            return false;
        }
        return isPrivate(dir, why);
    }

    // The user at the other end of a connected Unix socket; false when the system does not say.
    inline bool peerUser(int fd, uid_t& uid)
    {
 #if defined(__APPLE__) || defined(__FreeBSD__)
        gid_t gid = 0;
        return getpeereid(fd, &uid, &gid) == 0;
 #else
        struct ucred cred{};
        socklen_t n = sizeof cred;
        if (getsockopt(fd, SOL_SOCKET, SO_PEERCRED, &cred, &n) != 0 || n != sizeof cred)
            return false;
        uid = cred.uid;
        return true;
 #endif
    }

    // A socket not inherited by a program the process starts.
    inline int newSocket()
    {
 #if defined(SOCK_CLOEXEC)
        return socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
 #else
        const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd >= 0)
            fcntl(fd, F_SETFD, FD_CLOEXEC);
        return fd;
 #endif
    }

    inline int acceptFrom(int listener)
    {
 #if defined(SOCK_CLOEXEC) && !defined(__APPLE__)
        return accept4(listener, nullptr, nullptr, SOCK_CLOEXEC);
 #else
        const int fd = accept(listener, nullptr, nullptr);
        if (fd >= 0)
            fcntl(fd, F_SETFD, FD_CLOEXEC);
        return fd;
 #endif
    }

    // Two connected sockets, neither blocking: a thread's wake-up (written to one, read from the other).
    inline bool wakePair(int fds[2])
    {
 #if defined(SOCK_CLOEXEC) && defined(SOCK_NONBLOCK)
        return socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC | SOCK_NONBLOCK, 0, fds) == 0;
 #else
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, fds) != 0)
            return false;
        for (int i = 0; i < 2; ++i)
        {
            fcntl(fds[i], F_SETFD, FD_CLOEXEC);
            fcntl(fds[i], F_SETFL, O_NONBLOCK);
        }
        return true;
 #endif
    }

    // A socket's address for `path`; false when the path is too long for one (sun_path: 104 bytes on macOS).
    inline bool address(const std::string& path, sockaddr_un& out)
    {
        out = sockaddr_un{};
        out.sun_family = AF_UNIX;
        if (path.size() + 1 > sizeof out.sun_path)
            return false;
        for (std::size_t i = 0; i < path.size(); ++i)
            out.sun_path[i] = path[i];
        return true;
    }

    // Connects to the app's socket in `dir`, checking the folder and the peer. -1, with `why`, when it cannot.
    inline int connectTo(const std::string& dir, std::string& why)
    {
        if (!isPrivate(dir, why))
            return -1;
        sockaddr_un a{};
        if (!address(socketIn(dir), a))
        {
            why = "the link's path is too long";
            return -1;
        }
        const int fd = newSocket();
        if (fd < 0)
        {
            why = "no socket";
            return -1;
        }
        if (connect(fd, reinterpret_cast<const sockaddr*>(&a), sizeof a) != 0)
        {
            close(fd);
            why = "TheEngineEngine is not open, or \"Allow Claude Desktop\" is off in it";
            return -1;
        }
        uid_t uid = 0;
        if (!peerUser(fd, uid) || uid != geteuid())
        {
            close(fd);
            why = "the link is held by another user's program: refused";
            return -1;
        }
        return fd;
    }
#endif

    // Bytes in, whole lines out. A line longer than kMaxLine makes the reader fail (the connection is then closed).
    class Lines
    {
    public:
        void add(const char* data, std::size_t n) { buffer_.append(data, n); }
        // The next whole line (without its '\n' and a '\r' before it); false when there is none yet.
        bool next(std::string& line)
        {
            const std::size_t at = buffer_.find('\n', scanned_);
            if (at == std::string::npos)
            {
                scanned_ = buffer_.size();
                return false;
            }
            line.assign(buffer_, 0, at);
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            buffer_.erase(0, at + 1);
            scanned_ = 0;
            return true;
        }
        bool overflow() const noexcept { return buffer_.size() > kMaxLine; }

    private:
        std::string buffer_;
        std::size_t scanned_ = 0;
    };
}
