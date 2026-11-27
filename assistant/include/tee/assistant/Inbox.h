// assistant/include/tee/assistant/Inbox.h: how the assistant's worker reaches the document. The build, its file and the
// library of builds belong to the frame's thread and have no lock (host/include/tee/Build.h, shell/files/BuildFiles.h),
// so a worker never touches them: it posts a job, the frame runs it once a frame (drain(), called in beforeTick beside
// the script's events), and the job's reply comes back through its Ticket (docs/plan/w3-a-detailed.md, "The inbox").
//
//   // the worker
//   tee::assistant::Ticket t = inbox.post([&] { return Reply::of({{"name", file.build().name}}); });
//   Reply r = t.wait(2.0, &cancel);
//   // the frame, once a frame
//   inbox.drain();
//
// What it promises:
//   - post() never blocks: a full queue (kCapacity) or a closed inbox answers at once, without queueing.
//   - drain() runs at most kPerFrame jobs a frame, in the order they were posted. A job may answer "not yet"
//     (std::nullopt): it stays first in the queue and the drain stops there until the next frame, so what was posted
//     after it never overtakes it.
//   - A reply is never wrong about whether the job ran. A ticket is waiting, running, done or abandoned; the worker
//     whose wait ran out (or was cancelled) abandons a job still waiting, and the frame never runs it; one the frame
//     has begun is waited for (it is one frame's work) and its reply given.
//   - close() (the frame, at shutdown, once every worker has stopped posting): every job still queued is answered "the
//     app is closing" without running, and every later post is answered the same.
//
// Threads: post() and Ticket from any thread; drain() and close() from the frame's thread only.
#pragma once

#include <nlohmann/json.hpp>

#include <atomic>
#include <cstddef>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>

namespace tee::assistant
{
    class Inbox;

    // A job's answer: a value, or an error in the assistant's own words (never a path).
    struct Reply
    {
        bool ok = false;
        nlohmann::json value;
        std::string error;

        static Reply of(nlohmann::json v)
        {
            Reply r;
            r.ok = true;
            r.value = std::move(v);
            return r;
        }
        static Reply failed(std::string why, nlohmann::json v = nullptr)
        {
            Reply r;
            r.error = std::move(why);
            r.value = std::move(v);
            return r;
        }
    };

    // What every refusal of the inbox itself says.
    inline constexpr const char* kBusy = "the app is busy; try again in a moment";
    inline constexpr const char* kClosing = "the app is closing";
    inline constexpr const char* kTimedOut = "timed out before the app took it: nothing was done";
    inline constexpr const char* kCancelled = "cancelled before the app took it: nothing was done";

    class Ticket
    {
    public:
        Ticket() = default;

        // Waits at most `seconds` (real time) for the reply, looking at `cancel` (may be null) every few milliseconds.
        // A job the frame has not begun when the time runs out or `cancel` is raised is abandoned: never run, answered
        // kTimedOut or kCancelled. A job the frame has begun is always waited for.
        Reply wait(double seconds, const std::atomic<bool>* cancel = nullptr);
        // The reply is there (wait() would not wait).
        bool ready() const;

        struct State;   // Inbox.cpp's

    private:
        friend class ::tee::assistant::Inbox;
        explicit Ticket(std::shared_ptr<State> s) : state_(std::move(s)) {}
        std::shared_ptr<State> state_;
    };

    class Inbox
    {
    public:
        static constexpr std::size_t kCapacity = 32;
        static constexpr std::size_t kPerFrame = 8;

        // A job: the frame runs it; std::nullopt means "not yet" (run it again next frame, nothing after it first).
        using Job = std::function<std::optional<Reply>()>;

        Inbox() = default;
        Inbox(const Inbox&) = delete;
        Inbox& operator=(const Inbox&) = delete;
        ~Inbox();

        // Any thread.
        Ticket post(Job job);
        // Called after each job is queued, on the posting thread, outside the inbox's lock (any thread may post): the
        // app wakes its main thread to drain, so that a window that does not tick (minimised) still answers. Set it
        // before the first post; may be empty.
        void setWake(std::function<void()> wake);
        std::size_t waiting() const;
        bool closed() const;

        // The frame's thread. drain() returns the jobs that finished this frame.
        std::size_t drain(std::size_t most = kPerFrame);
        void close();

    private:
        struct Entry
        {
            Job job;
            std::shared_ptr<Ticket::State> state;
        };
        mutable std::mutex mutex_;
        std::deque<Entry> queue_;
        std::function<void()> wake_;
        bool closed_ = false;
    };
}
