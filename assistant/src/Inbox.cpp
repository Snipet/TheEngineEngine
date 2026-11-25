// assistant/src/Inbox.cpp: the queue between the assistant's worker and the frame (Inbox.h).
#include "tee/assistant/Inbox.h"

#include <chrono>
#include <condition_variable>

namespace tee::assistant
{
    struct Ticket::State
    {
        enum class Phase
        {
            Waiting,
            Running,
            Done,
            Abandoned,
        };
        std::mutex mutex;
        std::condition_variable cv;
        Phase phase = Phase::Waiting;
        Reply reply;

        void finish(Reply r)
        {
            {
                std::lock_guard<std::mutex> lock(mutex);
                reply = std::move(r);
                phase = Phase::Done;
            }
            cv.notify_all();
        }
    };

    namespace
    {
        std::shared_ptr<Ticket::State> answered(const char* why)
        {
            auto s = std::make_shared<Ticket::State>();
            s->phase = Ticket::State::Phase::Done;
            s->reply = Reply::failed(why);
            return s;
        }
    }

    Reply Ticket::wait(double seconds, const std::atomic<bool>* cancel)
    {
        if (!state_)
            return Reply::failed(kClosing);
        using clock = std::chrono::steady_clock;
        const auto deadline = clock::now() + std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(seconds));
        std::unique_lock<std::mutex> lock(state_->mutex);
        for (;;)
        {
            if (state_->phase == State::Phase::Done)
                return state_->reply;
            if (state_->phase == State::Phase::Running)
            {
                // begun: it is one frame's work, and its reply must be the truth
                state_->cv.wait(lock, [&] { return state_->phase != State::Phase::Running; });
                continue;
            }
            const bool stop = cancel != nullptr && cancel->load(std::memory_order_acquire);
            if (stop || clock::now() >= deadline)
            {
                state_->phase = State::Phase::Abandoned;
                return Reply::failed(stop ? kCancelled : kTimedOut);
            }
            state_->cv.wait_for(lock, std::chrono::milliseconds(5));
        }
    }

    bool Ticket::ready() const
    {
        if (!state_)
            return true;
        std::lock_guard<std::mutex> lock(state_->mutex);
        return state_->phase == State::Phase::Done;
    }

    Inbox::~Inbox()
    {
        close();
    }

    Ticket Inbox::post(Job job)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (closed_)
            return Ticket(answered(kClosing));
        if (queue_.size() >= kCapacity)
            return Ticket(answered(kBusy));
        auto s = std::make_shared<Ticket::State>();
        queue_.push_back({std::move(job), s});
        return Ticket(std::move(s));
    }

    std::size_t Inbox::waiting() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    bool Inbox::closed() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return closed_;
    }

    std::size_t Inbox::drain(std::size_t most)
    {
        std::size_t finished = 0;
        for (std::size_t taken = 0; taken < most; ++taken)
        {
            Entry e;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                if (queue_.empty() || closed_)
                    break;
                e = std::move(queue_.front());
                queue_.pop_front();
            }
            {
                std::lock_guard<std::mutex> lock(e.state->mutex);
                if (e.state->phase == Ticket::State::Phase::Abandoned)
                    continue;   // its worker gave up on it: never run
                e.state->phase = Ticket::State::Phase::Running;
            }
            std::optional<Reply> r = e.job();
            if (!r)
            {
                // not yet: first again, and nothing after it this frame. It counts as waiting once more, so that its
                // worker may still give up on it.
                {
                    std::lock_guard<std::mutex> lock(e.state->mutex);
                    e.state->phase = Ticket::State::Phase::Waiting;
                }
                e.state->cv.notify_all();
                std::lock_guard<std::mutex> lock(mutex_);
                queue_.push_front(std::move(e));
                break;
            }
            e.state->finish(std::move(*r));
            ++finished;
        }
        return finished;
    }

    void Inbox::close()
    {
        std::deque<Entry> left;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            closed_ = true;
            left.swap(queue_);
        }
        for (Entry& e : left)
        {
            std::lock_guard<std::mutex> lock(e.state->mutex);
            if (e.state->phase == Ticket::State::Phase::Waiting)
            {
                e.state->reply = Reply::failed(kClosing);
                e.state->phase = Ticket::State::Phase::Done;
            }
        }
        for (Entry& e : left)
            e.state->cv.notify_all();
    }
}
