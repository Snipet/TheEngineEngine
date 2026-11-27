// assistant/include/tee/assistant/Session.h: the assistant as the app holds it: the inbox, the tools, who drives, the
// cards and Claude Desktop's link, made together and stopped in the right order. The app makes one once its document
// and library exist, calls frame() once a frame (in beforeTick, beside the script's events), and stop() before the
// document goes.
//
//   tee::assistant::Session assistant({document, library, simHost, sim, clock, ask, othersBusy});
//   assistant.allowDesktop(true, why);   // the owner's switch "Allow Claude Desktop" (the settings keep it)
//   // each frame: whether the app's own heavy work runs (the ribbon's curve, the bake), read here on the frame's
//   // thread and kept for the assistant's worker, which yields to it (Tools.h, Host::othersBusy)
//   assistant.frame(dyno.busy() || bakeWorking);
//   // at the end, before the document goes
//   assistant.stop();
//
// Threads: the frame's.
#pragma once

#include "tee/assistant/Cards.h"
#include "tee/assistant/Chat.h"
#include "tee/assistant/KeyStore.h"
#include "tee/assistant/Inbox.h"
#include "tee/assistant/McpLink.h"
#include "tee/assistant/Tools.h"

#include <atomic>
#include <mutex>
#include <memory>
#include <string>

namespace tee::assistant
{
    // What the app gives (Tools.h's Host, less the inbox, which the session owns).
    struct SessionParts
    {
        files::BuildFile& document;
        files::BuildLibrary& library;
        std::function<SimHost*()> simHost;
        std::function<std::shared_ptr<Sim>()> sim;
        std::function<double()> clock;
        std::function<std::future<bool>(const std::string&)> ask;
        std::function<bool()> othersBusy;   // any thread; may be empty (frame()'s flag serves alone)
        McpLinkOptions link{};
    };

    class Session
    {
    public:
        explicit Session(SessionParts parts);
        ~Session();
        Session(const Session&) = delete;
        Session& operator=(const Session&) = delete;

        // Once a frame: the tools' jobs run. `appBusy`: the app's own heavy work runs now (its curves yield to it).
        // The app's wake (Inbox::setWake): called on the posting thread whenever a tool posts a job, so the app drains
        // it (frame()) on its main thread even while its window does not tick (minimised). Set once, before the switch
        // or the chat is used; may be empty. While jobs remain after a drain (waiting(): a job that said "not yet"), the
        // app drains again a little later (a job's limit is 2 s of real time).
        void setWake(std::function<void()> wake) { inbox_.setWake(std::move(wake)); }
        std::size_t waiting() const { return inbox_.waiting(); }

        void frame(bool appBusy = false)
        {
            appBusy_.store(appBusy);
            inbox_.drain();
        }

        // The switch: on makes the link (false, with `why`, when it cannot be), off removes it.
        bool allowDesktop(bool on, std::string& why);
        bool desktopAllowed() const { return link_->view().listening; }

        McpLink::View desktop() const { return link_->view(); }
        // What the status line says of Claude Desktop: "" while the switch is off; "Claude Desktop allowed" while on
        // and nothing connected; "Claude Desktop connected, 3 changes made" (batches of field changes, not loads or
        // saves; made, so undoing one in the app does not lower it) (running: "Claude Desktop working: set_fields").
        // Until the frame draws the cards, this is how the owner sees that Claude Desktop changes the build.
        std::string statusNote() const;
        Cards& cards() noexcept { return cards_; }
        Tools& tools() noexcept { return *tools_; }
        Drivers& drivers() noexcept { return drivers_; }

        // The key chat (A3): the port its panel (assistant/view/ChatView.h) is mounted on. Its transport is the
        // platform's (libcurl), its key the system's store's; where either is missing it says why (ChatPort::ready).
        ChatPort& chat() noexcept { return *chat_; }
        // The key dialog's (U1's): a key as typed, checked and saved in the system's store (false, with why: nothing
        // changed); the saved one's mask ("saved, ending ...abcd"; "reading..." while it is read); the key removed.
        // The saved key is read once, on a thread of its own, the first time the chat asks for it: the window never
        // waits on the store (the Keychain may ask the owner whether this app may use it). A read that fails, or that
        // the owner refuses, is not tried again until a key is saved or removed.
        bool saveKey(std::string typed, std::string& why);
        std::string keyMask();
        bool removeKey(std::string& why);
        // Tests: the store the session uses in place of the system's (before the chat is used).
        void useKeyStore(std::unique_ptr<KeyStore> store);

        // Every worker stopped (their calls cancelled and waited for), then the inbox closed. Idempotent.
        void stop();

    private:
        Inbox inbox_;
        Drivers drivers_;
        Cards cards_;
        std::unique_ptr<Tools> tools_;
        std::unique_ptr<McpLink> link_;
        std::unique_ptr<Transport> transport_;
        std::shared_ptr<KeyStore> keys_;
        std::string transportWhy_, keysWhy_;
        // The saved key, as the reading thread leaves it (shared with that thread, which may outlive the session while
        // the store holds it, at a Keychain question: it then leaves the session alone).
        struct KeyState
        {
            std::mutex mutex;
            enum class Step { Unread, Reading, Read, Failed } step = Step::Unread;
            std::string key, why;
            bool alive = true;              // the session is there (cleared, under the mutex, as it goes)
            std::function<void()> changed;  // the chat told (under the mutex, while alive)
        };
        std::shared_ptr<KeyState> keyState_ = std::make_shared<KeyState>();
        std::string currentKey();           // never waits: starts the read once, "" until it is done
        std::string keyWhy();               // why there is no key now ("" : none is saved)
        std::unique_ptr<Chat> chat_;
        bool stopped_ = false;
        std::atomic<bool> appBusy_{false};
    };
}
