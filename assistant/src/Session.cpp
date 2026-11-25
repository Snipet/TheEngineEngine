// assistant/src/Session.cpp: the assistant as the app holds it (Session.h).
#include "tee/assistant/Session.h"

namespace tee::assistant
{
    Session::Session(SessionParts parts)
    {
        const std::function<double()> clock = parts.clock;
        std::function<bool()> given = std::move(parts.othersBusy);
        std::function<bool()> busy = [this, given] { return appBusy_.load() || (given && given()); };
        tools_ = std::make_unique<Tools>(Host{inbox_, parts.document, parts.library, std::move(parts.simHost),
                                              std::move(parts.sim), parts.clock, std::move(parts.ask), Limits{},
                                              std::move(busy)});
        link_ = std::make_unique<McpLink>(*tools_, drivers_, cards_, clock, std::move(parts.link));
    }

    Session::~Session()
    {
        stop();
    }

    bool Session::allowDesktop(bool on, std::string& why)
    {
        if (stopped_)
        {
            why = kClosing;
            return false;
        }
        if (!on)
        {
            link_->stop();
            return true;
        }
        return link_->start(why);
    }

    void Session::stop()
    {
        if (stopped_)
            return;
        stopped_ = true;
        // The link's worker may be waiting on a job: it gives up by its own limit, or the close below answers it.
        // Closing first would refuse its next job; stopping first cancels the call, which then answers at once.
        link_->stop();
        inbox_.close();
    }
}
