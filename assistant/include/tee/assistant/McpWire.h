// assistant/include/tee/assistant/McpWire.h: how the running app (McpLink.h) and the bridge Claude Desktop starts
// (assistant/mcp/Main.cpp, tee-mcp) find and talk to each other. Header only, no JUCE, nothing of the host's, so the
// bridge stays small. The design is TestingPlayground's (Source/mcp/McpProtocol.h at 47bac40), rewritten, with a check
// of the peer's user that it does not have (docs/plan/w3-a-assistant.md, "The MCP path").
//
//   the folder   private to the user (0700, the user's own, no symlink; anything else is refused, never repaired), and
//                found from the system, never from the environment: Claude Desktop starts the bridge with a bare one
//                (HOME, LOGNAME, PATH, SHELL, TERM, USER: the MCP SDK's stdio transport, getDefaultEnvironment), the
//                app has its session's, and both ends must land on the same folder.
//                macOS: confstr(_CS_DARWIN_USER_TEMP_DIR), the per-user /var/folders/../T/ (Libc's gen/confstr.c asks
//                the system's dirhelper; only when that fails does it read TMPDIR), + "/TheEngineEngine-mcp";
//                Linux: /run/user/<uid> when it is a folder, not a link, the user's own and closed to others,
//                + "/TheEngineEngine-mcp"; else (either system) /tmp/TheEngineEngine-mcp-<uid>.
//                TEE_MCP_DIR, when set, names the folder itself (the tests'); a base may be named instead of the
//                system's (McpLinkOptions::base, tee-mcp --base: the tests', to keep out of the real per-user folder).
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
#include <cstring>
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
    // The bridge's longest wait for the app's answer to a call: inside the MCP client's own timeout (60 s by default in
    // the MCP SDK), so the client is always answered, in words, by this server rather than timing out.
    inline constexpr double kCallSeconds = 55.0;

    // The base the link's folder lies in, from the system and never the environment (see above): "" when the system
    // has none to give (then the folder is /tmp/TheEngineEngine-mcp-<uid>).
    inline std::string systemBase()
    {
#if defined(_WIN32)
        return {};
#elif defined(__APPLE__)
        char buf[1024];
        const std::size_t n = confstr(_CS_DARWIN_USER_TEMP_DIR, buf, sizeof buf);
        if (n == 0 || n > sizeof buf)
            return {};
        std::string base(buf);
        while (base.size() > 1 && base.back() == '/')
            base.pop_back();
        return base;
#else
        const std::string run = "/run/user/" + std::to_string(static_cast<unsigned long>(geteuid()));
        struct stat st{};
        if (lstat(run.c_str(), &st) != 0 || !S_ISDIR(st.st_mode) || st.st_uid != geteuid() || (st.st_mode & 077) != 0)
            return {};
        return run;
#endif
    }

    // The link's folder under `base` (the system's, or one a test names): base/TheEngineEngine-mcp, or with no base
    // /tmp/TheEngineEngine-mcp-<uid>.
    inline std::string folderIn(const std::string& base)
    {
        if (!base.empty())
            return base + "/TheEngineEngine-mcp";
#if defined(_WIN32)
        return {};
#else
        return "/tmp/TheEngineEngine-mcp-" + std::to_string(static_cast<unsigned long>(geteuid()));
#endif
    }

    // The folder of the link (not made here): TEE_MCP_DIR when set, else under the system's base.
    inline std::string folder()
    {
        if (const char* d = std::getenv("TEE_MCP_DIR"); d != nullptr && *d != '\0')
            return d;
        return folderIn(systemBase());
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

    // A write to a socket whose peer has gone must never raise SIGPIPE (it would end the app: the owner's window gone
    // because Claude Desktop's bridge went away). Every socket made here takes SO_NOSIGPIPE where the system has it
    // (macOS), and every send is sendAll's, with MSG_NOSIGNAL where the system has that (Linux). The application's
    // main ignores SIGPIPE besides, as a second line.
    inline void noSigpipe([[maybe_unused]] int fd)
    {
 #if defined(SO_NOSIGPIPE)
        const int one = 1;
        setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof one);
 #endif
    }

    // The whole of `n` bytes to a socket: false when the peer has gone (EPIPE, ECONNRESET) or the socket refused it
    // (a send timeout); never a signal. EINTR is retried.
    inline bool sendAll(int fd, const char* data, std::size_t n)
    {
        std::size_t at = 0;
        while (at < n)
        {
 #if defined(MSG_NOSIGNAL)
            const ssize_t sent = send(fd, data + at, n - at, MSG_NOSIGNAL);
 #else
            const ssize_t sent = send(fd, data + at, n - at, 0);   // (the socket's SO_NOSIGPIPE)
 #endif
            if (sent < 0 && errno == EINTR)
                continue;
            if (sent <= 0)
                return false;
            at += static_cast<std::size_t>(sent);
        }
        return true;
    }

    // A socket not inherited by a program the process starts, and never raising SIGPIPE.
    inline int newSocket()
    {
 #if defined(SOCK_CLOEXEC)
        const int fd = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
 #else
        const int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd >= 0)
            fcntl(fd, F_SETFD, FD_CLOEXEC);
 #endif
        if (fd >= 0)
            noSigpipe(fd);
        return fd;
    }

    inline int acceptFrom(int listener)
    {
 #if defined(SOCK_CLOEXEC) && !defined(__APPLE__)
        const int fd = accept4(listener, nullptr, nullptr, SOCK_CLOEXEC);
 #else
        const int fd = accept(listener, nullptr, nullptr);
        if (fd >= 0)
            fcntl(fd, F_SETFD, FD_CLOEXEC);
 #endif
        if (fd >= 0)
            noSigpipe(fd);
        return fd;
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
            noSigpipe(fds[i]);
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

    // Bytes in, whole lines out. A line longer than kMaxLine makes the reader fail (the connection is then closed), or,
    // for a reader that goes on (the bridge's client), is skipped whole: skipLine().
    class Lines
    {
    public:
        void add(const char* data, std::size_t n)
        {
            if (skipping_)
            {
                // the rest of an over-long line: dropped up to its end
                const char* end = static_cast<const char*>(std::memchr(data, '\n', n));
                if (end == nullptr)
                    return;
                skipping_ = false;
                const std::size_t used = static_cast<std::size_t>(end - data) + 1;
                data += used;
                n -= used;
            }
            buffer_.append(data, n);
        }
        // The line now over kMaxLine dropped, whatever of it is still to come too; the lines after it read as usual.
        void skipLine()
        {
            const std::size_t at = buffer_.find('\n');
            if (at == std::string::npos)
            {
                buffer_.clear();
                skipping_ = true;
            }
            else
                buffer_.erase(0, at + 1);
            scanned_ = 0;
        }
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
        bool skipping_ = false;
    };
}
