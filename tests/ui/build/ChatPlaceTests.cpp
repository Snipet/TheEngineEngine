// tests/ui/build/ChatPlaceTests.cpp: card A's key chat panel (assistant/view/ChatView.h) where U1's frame mounts it
// (the lead's ask, 25 Nov): closed at first, the top bar's CHAT opens it in the inspector's column over the real Spec's
// frame; empty, its "Add a key" opens the frame's key dialog; with a conversation it draws in that column. The port is
// a made-up one: no test here talks to anything (no transport, no key store). Pictures under out/ui/ (build.chat.*).
#include "build/BuildKit.h"

#if defined(TEE_HAS_CHAT_VIEW)
 #include "ChatView.h"
#endif

namespace
{
    using tee::test::build::Rig;

#if defined(TEE_HAS_CHAT_VIEW)
    struct MadeUpPort final : tee::assistant::ChatPort
    {
        std::uint64_t rev = 1;
        std::vector<tee::assistant::ChatItem> list;
        bool keyed = false;
        std::uint64_t revision() const override { return rev; }
        std::vector<tee::assistant::ChatItem> items() const override { return list; }
        bool busy() const override { return false; }
        bool ready(std::string& why) const override
        {
            if (!keyed)
                why = "no API key is saved: add one to chat with Claude";
            return keyed;
        }
        bool send(const std::string&, std::string& why) override
        {
            why = "this test's chat sends nothing";
            return false;
        }
        void stop() override {}
        std::string question() const override { return {}; }
        bool answer(const std::string&) override { return false; }
        void add(tee::assistant::ChatItem::Kind k, std::string text, std::string tool = {})
        {
            tee::assistant::ChatItem i;
            i.kind = k;
            i.text = std::move(text);
            i.tool = std::move(tool);
            list.push_back(std::move(i));
            ++rev;
        }
    };

    void shot(Rig& rig, const char* name)
    {
        rig.host->tick(30);
        rig.host->draw();
        rig.host->writePng((std::string(TEE_UI_OUT_DIR) + "/" + name + ".png").c_str(), 1);
    }
#endif

    BUILD_TEST(chatPlaceTest, "build.chat")
    {
#if !defined(TEE_HAS_CHAT_VIEW)
        std::printf("  (no chat panel in this build: card A's assistant/view is not built here)\n");
        return 0;
#else
        tee::ui::AppFrame& f = *rig.frame;
        rig.host->resize(1100, 700);
        MadeUpPort port;
        auto chat = std::make_unique<tee::assistant::view::ChatView>(
            tee::assistant::view::ChatView::Hooks{ [&f] { f.openKeyDialog(); } }, 0x7A000000u);
        tee::assistant::view::ChatView* const view = chat.get();
        view->attach(&port);
        f.mountChat(std::move(chat));
        rig.host->tick(10);
        CHECK(!f.chatOpen() && !view->shown());
        f.show(f.find("tune"));
        f.openChat(true);
        rig.host->tick(30);
        CHECK(f.chatOpen() && view->shown() && view->bounds().x >= f.stageRect().right());
        // Empty, without a key: its why and "Add a key", which opens the frame's key dialog.
        const auto add = view->centreOfAddKey();
        CHECK(add.has_value());
        shot(rig, "build.chat.empty");
        if (add)
        {
            rig.host->click(add->x, add->y);
            rig.host->tick(10);
            CHECK(f.keyDialogOpen());
            f.keyDialog()->close();
        }
        // A conversation, as card A's chat lists it.
        port.keyed = true;
        port.add(tee::assistant::ChatItem::Kind::User, "Why does the torque fall off past 6000 rpm?");
        port.add(tee::assistant::ChatItem::Kind::Tool, "read 14 fields of the intake and the cams", "get_fields");
        port.add(tee::assistant::ChatItem::Kind::Assistant,
                 "The intake cam closes at **62 degrees** after the bottom: past 6000 rpm the runners' wave arrives "
                 "after it has shut.\n\n- Open the phaser's high-speed target by 8 degrees\n- or shorten the runners");
        rig.host->tick(30);
        CHECK_MSG(view->shownText().find("62 degrees") != std::string::npos, view->shownText());
        shot(rig, "build.chat.talk");
        f.openChat(false);
        rig.host->tick(10);
        CHECK(!view->shown() && f.inspector() != nullptr);
        return 0;
#endif
    }
}
