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
#include "tee/assistant/Inbox.h"
#include "tee/assistant/McpLink.h"
#include "tee/assistant/Tools.h"

#include <atomic>
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
        // and nothing connected; "Claude Desktop connected, 3 changes" (running: "Claude Desktop working: set_fields").
        // Until the frame draws the cards, this is how the owner sees that Claude Desktop changes the build.
        std::string statusNote() const;
        Cards& cards() noexcept { return cards_; }
        Tools& tools() noexcept { return *tools_; }
        Drivers& drivers() noexcept { return drivers_; }

        // Every worker stopped (their calls cancelled and waited for), then the inbox closed. Idempotent.
        void stop();

    private:
        Inbox inbox_;
        Drivers drivers_;
        Cards cards_;
        std::unique_ptr<Tools> tools_;
        std::unique_ptr<McpLink> link_;
        bool stopped_ = false;
        std::atomic<bool> appBusy_{false};
    };
}
