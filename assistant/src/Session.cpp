// assistant/src/Session.cpp: the assistant as the app holds it (Session.h).
#include "tee/assistant/Session.h"

#include "files/BuildFiles.h"

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
        // the key chat: made now, idle (no thread, no connection, no read of the store) until it is used
        transport_ = makeTransport(transportWhy_);
        keys_ = makeSystemKeyStore(keysWhy_);
        chat_ = std::make_unique<Chat>(*tools_, drivers_, transport_.get(), [this] { return currentKey(); },
                                       systemPromptFor(parts.document.build().fields()), clock);
    }

    std::string Session::currentKey()
    {
        std::lock_guard<std::mutex> lock(keyMutex_);
        if (!keyRead_ && keys_)
        {
            std::string why;
            keyRead_ = keys_->load(key_, why);
            if (!keyRead_)
                key_.clear();
        }
        return key_;
    }

    bool Session::saveKey(std::string typed, std::string& why)
    {
        if (!keys_)
        {
            why = keysWhy_;
            return false;
        }
        if (!checkKey(typed, why))
            return false;
        std::lock_guard<std::mutex> lock(keyMutex_);
        if (!keys_->save(typed, why))
            return false;   // the key held before stays
        key_ = std::move(typed);
        keyRead_ = true;
        return true;
    }

    std::string Session::keyMask()
    {
        if (!keys_)
            return keysWhy_;
        return maskKey(currentKey());
    }

    bool Session::removeKey(std::string& why)
    {
        if (!keys_)
        {
            why = keysWhy_;
            return false;
        }
        std::lock_guard<std::mutex> lock(keyMutex_);
        if (!keys_->remove(why))
            return false;
        key_.clear();
        keyRead_ = true;
        return true;
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

    std::string Session::statusNote() const
    {
        const McpLink::View v = link_->view();
        if (!v.listening)
            return {};
        if (v.running)
            return "Claude Desktop working: " + v.tool;
        if (v.connections == 0)
            return "Claude Desktop allowed";
        std::size_t changes = 0;
        for (const Card& c : cards_.list())
            changes += c.undone ? 0 : 1;
        return "Claude Desktop connected, " + std::to_string(changes) + (changes == 1 ? " change" : " changes");
    }

    void Session::stop()
    {
        if (stopped_)
            return;
        stopped_ = true;
        // The link's worker may be waiting on a job: it gives up by its own limit, or the close below answers it.
        // Closing first would refuse its next job; stopping first cancels the call, which then answers at once.
        chat_->stop();
        chat_->wait();      // the chat's turn ends at once (cancelled), answering what it called
        link_->stop();
        inbox_.close();
    }
}
