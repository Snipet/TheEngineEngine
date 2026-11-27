// assistant/src/Session.cpp: the assistant as the app holds it (Session.h).
#include "tee/assistant/Session.h"

#include "files/BuildFiles.h"

#include <thread>

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
        ChatOptions chatOptions;
        chatOptions.noTransportWhy = transportWhy_;   // (Windows: "the chat by API key is for macOS and Linux ...")
        // (no store: its reason is asked for when the key is, keyWhy(), never copied here: a store given later is whole)
        chatOptions.keyWhy = [this] { return keyWhy(); };
        chat_ = std::make_unique<Chat>(*tools_, drivers_, transport_.get(), [this] { return currentKey(); },
                                       systemPromptFor(parts.document.build().fields()), clock, std::move(chatOptions));
        keyState_->changed = [chat = chat_.get()] { chat->refresh(); };
    }

    void Session::useTransport(std::unique_ptr<Transport> transport)
    {
        transport_ = std::move(transport);
        transportWhy_.clear();
        chat_->useTransport(transport_.get());
    }

    void Session::useKeyStore(std::unique_ptr<KeyStore> store)
    {
        keys_ = std::move(store);
        keysWhy_.clear();
        const std::lock_guard<std::mutex> lock(keyState_->mutex);
        keyState_->step = KeyState::Step::Unread;
    }

    std::string Session::currentKey()
    {
        const std::shared_ptr<KeyState> st = keyState_;
        std::shared_ptr<KeyStore> store = keys_;
        const std::lock_guard<std::mutex> lock(st->mutex);
        if (st->step == KeyState::Step::Unread && store)
        {
            st->step = KeyState::Step::Reading;
            // the store's read, off the caller's thread (the frame's, when the panel asks whether it can send); it holds
            // the state and the store, never the session
            std::thread([st, store] {
                std::string key, why;
                const bool ok = store->load(key, why);
                const std::lock_guard<std::mutex> done(st->mutex);
                if (st->step != KeyState::Step::Reading)
                    return;   // a key saved or removed meanwhile is the truth
                st->step = ok ? KeyState::Step::Read : KeyState::Step::Failed;
                st->key = ok ? key : std::string();
                st->why = ok ? std::string() : why;
                if (st->alive && st->changed)
                    st->changed();
            }).detach();
        }
        return st->step == KeyState::Step::Read ? st->key : std::string();
    }

    std::string Session::keyWhy()
    {
        if (!keys_)
            return keysWhy_.empty() ? std::string("this build has no store for the key") : keysWhy_;
        currentKey();
        const std::lock_guard<std::mutex> lock(keyState_->mutex);
        switch (keyState_->step)
        {
        case KeyState::Step::Reading:
            return "reading your saved key (the system may ask whether this app may use it)";
        case KeyState::Step::Failed:
            return "your saved key could not be read" + (keyState_->why.empty() ? std::string() : " (" + keyState_->why + ")")
                   + ": save it again with Key";
        default:
            return {};
        }
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
        if (!keys_->save(typed, why))
            return false;   // the key held before stays
        const std::lock_guard<std::mutex> lock(keyState_->mutex);
        keyState_->step = KeyState::Step::Read;
        keyState_->key = std::move(typed);
        keyState_->why.clear();
        if (keyState_->changed)
            keyState_->changed();
        return true;
    }

    std::string Session::keyMask()
    {
        if (!keys_)
            return keysWhy_;
        const std::string key = currentKey();
        if (const std::string why = keyWhy(); !why.empty())
            return why;
        return maskKey(key);
    }

    bool Session::removeKey(std::string& why)
    {
        if (!keys_)
        {
            why = keysWhy_;
            return false;
        }
        if (!keys_->remove(why))
            return false;
        const std::lock_guard<std::mutex> lock(keyState_->mutex);
        keyState_->step = KeyState::Step::Read;
        keyState_->key.clear();
        keyState_->why.clear();
        if (keyState_->changed)
            keyState_->changed();
        return true;
    }

    Session::~Session()
    {
        {
            // a read still at the store (a Keychain question unanswered) finds the session gone and leaves it alone
            const std::lock_guard<std::mutex> lock(keyState_->mutex);
            keyState_->alive = false;
            keyState_->changed = nullptr;
        }
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
        // the batches of field changes Claude has made (not its loads or saves): "made", since the owner may have undone
        // some with the app's Undo since, which this line cannot see
        std::size_t changes = 0;
        for (const Card& c : cards_.list())
            changes += c.tool == "set_fields" && !c.undone ? 1 : 0;
        return "Claude Desktop connected, " + std::to_string(changes) + (changes == 1 ? " change made" : " changes made");
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
