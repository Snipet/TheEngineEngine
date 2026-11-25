// tests/assistant/view/ChatViewTest.cpp: the key chat's panel (assistant/view/ChatView.h) over a made-up ChatPort: what
// it shows (the empty chat's why and its Add a key, the conversation with markdown, tool rows and notices, Claude's
// question), what typing, Enter, Send and Stop do, and that it lays out again only when the port moved. Pictures from
// FunkGui's software rasteriser go to out/assistant/ (nobody here has a display); no golden is kept.
#include "ChatView.h"

#include <funkgui/canvas/Canvas.h>
#include <funkgui/canvas/SoftRaster.h>
#include <funkgui/core/Theme.h>
#include <funkgui/text/FontService.h>

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <string>
#include <vector>

using namespace tee::assistant;
using tee::assistant::view::ChatView;

namespace
{
    struct FakePort final : ChatPort
    {
        std::uint64_t rev = 1;
        std::vector<ChatItem> list;
        bool isBusy = false, canChat = true;
        std::string why = "no API key is saved: add one in the chat's settings";
        std::string asked;
        std::vector<std::string> sent, answers;
        int stops = 0;
        mutable int reads = 0;
        bool refuse = false;

        std::uint64_t revision() const override { return rev; }
        std::vector<ChatItem> items() const override
        {
            ++reads;
            return list;
        }
        bool busy() const override { return isBusy; }
        bool ready(std::string& w) const override
        {
            if (!canChat)
                w = why;
            return canChat;
        }
        bool send(const std::string& text, std::string& w) override
        {
            if (refuse)
            {
                w = "Claude is still answering: stop it or wait";
                return false;
            }
            sent.push_back(text);
            return true;
        }
        void stop() override { ++stops; }
        std::string question() const override { return asked; }
        bool answer(const std::string& text) override
        {
            answers.push_back(text);
            asked.clear();
            ++rev;
            return true;
        }
        void add(ChatItem::Kind k, std::string text, bool error = false, std::string tool = {})
        {
            ChatItem i;
            i.kind = k;
            i.text = std::move(text);
            i.error = error;
            i.tool = std::move(tool);
            list.push_back(std::move(i));
            ++rev;
        }
    };

    void tick(ChatView& v)
    {
        v.tick(1.0f / 60.0f, false, true, {-1.0e4f, -1.0e4f});
    }

    void typeText(ChatView& v, const std::string& s)
    {
        for (const char c : s)
        {
            funkgui::KeyEvent e;
            if (c == ' ')
                e.key = funkgui::Key::space;
            else
            {
                e.key = funkgui::Key::character;
                e.ch = static_cast<char32_t>(c);
            }
            v.type(e);
        }
    }

    void key(ChatView& v, funkgui::Key k)
    {
        funkgui::KeyEvent e;
        e.key = k;
        v.type(e);
    }

    // The view drawn by the software rasteriser, written to out/assistant/<name>.png; the count of pixels not the ground.
    std::size_t picture(const ChatView& v, const std::string& name, int w, int h)
    {
        const funkgui::Theme th = funkgui::Theme::graphite();
        const auto& atlas = funkgui::FontService::get().atlas();
        funkgui::Canvas c(atlas);
        funkgui::FrameInfo info;
        info.logicalW = w;
        info.logicalH = h;
        info.clear = th.ground;
        c.begin(info);
        v.draw(c, th, false);
        const funkgui::Image img = funkgui::rasterise(c.end(), atlas);
        const std::filesystem::path dir = std::filesystem::path(TEE_SOURCE_DIR) / "out" / "assistant";
        std::filesystem::create_directories(dir);
        funkgui::writePng(img, (dir / (name + ".png")).string().c_str());
        std::size_t inked = 0;
        for (std::size_t i = 0; i + 3 < img.rgba.size(); i += 4)
            if (img.rgba[i] != th.ground.r || img.rgba[i + 1] != th.ground.g || img.rgba[i + 2] != th.ground.b)
                ++inked;
        return inked;
    }
}

TEST_CASE("assistant: the chat panel, empty: why it cannot chat yet, and its Add a key", "[assistant][view]")
{
    FakePort port;
    port.canChat = false;
    int dialogs = 0;
    ChatView v({[&] { ++dialogs; }});
    v.attach(&port);
    v.setBounds({0.0f, 0.0f, 420.0f, 560.0f});
    tick(v);
    CHECK(v.shownText().find("no API key") != std::string::npos);
    REQUIRE(v.centreOfAddKey().has_value());
    v.press(*v.centreOfAddKey());
    CHECK(dialogs == 1);
    CHECK(picture(v, "chat-empty", 420, 560) > 1000);
    // with a key: what the chat is for, and no dialog on a press
    port.canChat = true;
    ++port.rev;
    tick(v);
    CHECK(v.shownText().find("Ask Claude about the engine") != std::string::npos);
    v.press(*v.centreOfAddKey());
    CHECK(dialogs == 1);
}

TEST_CASE("assistant: the chat panel shows the conversation: markdown answers, tool rows, notices", "[assistant][view]")
{
    FakePort port;
    port.add(ChatItem::Kind::User, "Raise the idle a little");
    port.add(ChatItem::Kind::Assistant,
             "I changed **engine.idleTrim** from 0 to 50.\n\n- measured: idle at `812 rpm`\n- assumed: smoother\n\n"
             "```\nengine.idleTrim 50\n```");
    port.add(ChatItem::Kind::Tool, "done", false, "set_fields");
    port.add(ChatItem::Kind::Tool, "the build has no field \"x\"", true, "describe_field");
    port.add(ChatItem::Kind::Notice, "Claude declined this request (cyber)", true);
    ChatView v({});
    v.attach(&port);
    v.setBounds({0.0f, 0.0f, 420.0f, 560.0f});
    tick(v);
    const std::string s = v.shownText();
    INFO(s);
    CHECK(s.find("you: Raise the idle a little") != std::string::npos);
    CHECK(s.find("engine.idleTrim") != std::string::npos);
    CHECK(s.find("**") == std::string::npos);   // the marks are read, not shown
    CHECK(s.find("tool: set_fields: done") != std::string::npos);
    CHECK(s.find("tool (error): describe_field:") != std::string::npos);
    CHECK(s.find("note (error): Claude declined") != std::string::npos);
    CHECK(picture(v, "chat-conversation", 420, 560) > 5000);
}

TEST_CASE("assistant: the chat panel's box: typing, Enter sends, a refusal shown as its hint", "[assistant][view]")
{
    FakePort port;
    ChatView v({});
    v.attach(&port);
    v.setBounds({0.0f, 0.0f, 420.0f, 560.0f});
    tick(v);
    typeText(v, "What is open?");
    CHECK(v.draft() == "What is open?");
    key(v, funkgui::Key::backspace);
    CHECK(v.draft() == "What is open");
    key(v, funkgui::Key::enter);
    REQUIRE(port.sent.size() == 1);
    CHECK(port.sent[0] == "What is open");
    CHECK(v.draft().empty());
    // refused: the draft kept, the why as the box's hint
    port.refuse = true;
    typeText(v, "Again");
    v.press(*v.centreOfSend());
    CHECK(port.sent.size() == 1);
    CHECK(v.draft() == "Again");
    CHECK(v.hint().find("still answering") != std::string::npos);
    // a non-ASCII character typed is kept whole (UTF-8), and goes back whole
    port.refuse = false;
    v.setDraft("");
    funkgui::KeyEvent e;
    e.key = funkgui::Key::character;
    e.ch = U'é';
    v.type(e);
    CHECK(v.draft() == "\xC3\xA9");
    key(v, funkgui::Key::backspace);
    CHECK(v.draft().empty());
}

TEST_CASE("assistant: the chat panel while Claude answers: Stop, by the button and by Escape", "[assistant][view]")
{
    FakePort port;
    port.add(ChatItem::Kind::User, "Tell me a story");
    port.isBusy = true;
    ChatView v({});
    v.attach(&port);
    v.setBounds({0.0f, 0.0f, 420.0f, 560.0f});
    tick(v);
    v.press(*v.centreOfSend());
    CHECK(port.stops == 1);
    key(v, funkgui::Key::escape);
    CHECK(port.stops == 2);
    CHECK(port.sent.empty());
}

TEST_CASE("assistant: the chat panel puts Claude's question over the box, and Enter answers it", "[assistant][view]")
{
    FakePort port;
    port.add(ChatItem::Kind::User, "Load a build");
    port.asked = "Which build: the V8 or the V10?";
    ++port.rev;
    ChatView v({});
    v.attach(&port);
    v.setBounds({0.0f, 0.0f, 420.0f, 560.0f});
    tick(v);
    CHECK(v.shownText().find("question: Claude asks: Which build") != std::string::npos);
    typeText(v, "the V10");
    key(v, funkgui::Key::enter);
    REQUIRE(port.answers.size() == 1);
    CHECK(port.answers[0] == "the V10");
    CHECK(port.sent.empty());
    tick(v);
    CHECK(v.shownText().find("question:") == std::string::npos);
}

TEST_CASE("assistant: the chat panel lays out again only when the port moved, and follows the end", "[assistant][view]")
{
    FakePort port;
    ChatView v({});
    v.attach(&port);
    v.setBounds({0.0f, 0.0f, 420.0f, 300.0f});
    tick(v);
    const int reads = port.reads;
    for (int i = 0; i < 10; ++i)
        tick(v);
    CHECK(port.reads == reads);   // nothing moved: nothing read
    for (int i = 0; i < 30; ++i)
        port.add(i % 2 == 0 ? ChatItem::Kind::User : ChatItem::Kind::Assistant, "line " + std::to_string(i));
    tick(v);
    CHECK(port.reads == reads + 1);
    CHECK(v.contentHeight() > 300.0f);
    CHECK(v.atEnd());
    CHECK(picture(v, "chat-long", 420, 300) > 3000);
}
