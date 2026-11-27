// assistant/view/ChatView.h: the key chat's panel, one item the frame mounts (U1's: where it sits, and the key dialog's
// place): the conversation over a ChatPort (Chat.h), Claude's answers as the light markdown TestingPlayground's chat
// draws (Markdown.h, ported), the tools it used as rows, the chat's notices, Claude's question waiting for the owner,
// and a message box with Send (Enter) and Stop (Escape, or the button while a turn runs).
//
// Its shape is TestingPlayground's ChatView (Source/editor/ChatView.h at 47bac40, assistant/view/SEED.tsv) cut to this
// app's chat: no pictures, plans, checks, models or chat list. It polls the port's revision each tick and lays out again
// only when it moved, or when its width changed. Empty, it says why it cannot chat yet (no key, no transport) with an
// "Add a key" action (Hooks::addKey: the frame's key dialog).
//
//   tee::assistant::view::ChatView view({[&] { openKeyDialog(); }});
//   view.attach(&chat);                 // the ChatPort
//   view.setBounds({x, y, w, h});
//   // a Widget of the kit (ui/kit/widgets/Widget.h: its Look, its hover help): the frame ticks, draws and routes
//   // input to it as to any other
//
// Threads: the frame's.
#pragma once

#include "Markdown.h"

#include "kit/gui/Items.h"
#include "kit/gui/Scroll.h"
#include "kit/widgets/Widget.h"

#include "tee/assistant/Chat.h"

#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace tee::assistant::view
{
    class ChatView final : public tee::ui::Widget
    {
    public:
        struct Hooks
        {
            std::function<void()> addKey;          // the empty chat's "Add a key": the frame's key dialog
        };

        explicit ChatView(Hooks hooks, uint32_t id = 0x7A000000u);

        void attach(ChatPort* port) { port_ = port; seen_ = ~0ull; }

        // What the owner has typed and not sent.
        const std::string& draft() const noexcept { return draft_; }
        void setDraft(std::string s) { draft_ = std::move(s); }
        // Send the draft (Enter): to the question waiting, if there is one, else as a message. False, with the port's
        // why shown as the box's hint, when it was not taken.
        bool submit();

        // A press at a point and a key, as pointerDown and key() take them (what they do, without the Panel's
        // controllers: the tests' way in).
        void press(funkgui::Point p);
        bool type(const funkgui::KeyEvent& e);

        // For tests: what is shown, a line each block (its kind and its text as laid out).
        std::string shownText() const;
        std::optional<funkgui::Point> centreOfSend() const;
        std::optional<funkgui::Point> centreOfAddKey() const;
        // The empty chat's "Add a key" button where it is drawn (none: not offered: the chat can be used, or no hook).
        std::optional<funkgui::Rect> addKeyButton() const;
        // "New chat", in a row over the conversation while there is anything to clear (none: nothing to clear).
        std::optional<funkgui::Rect> newChatButton() const;
        bool atEnd() const noexcept { return scroll_.offset >= scroll_.max() - 0.5f; }
        float contentHeight() const noexcept { return scroll_.content; }
        const std::string& hint() const noexcept { return hint_; }

        // tee::ui::Widget
        void tick(float dt, bool hovered, bool focused, funkgui::Point pointer) override;
        void draw(funkgui::Canvas&, const funkgui::Theme&, bool focusRing) const override;
        funkgui::Cursor cursorAt(funkgui::Point p) const override;
        void pointerDown(const funkgui::PointerEvent&, funkgui::GestureController&, funkgui::HostServices&) override;
        void pointerDrag(const funkgui::PointerEvent&, funkgui::GestureController&) override;
        void pointerUp(const funkgui::PointerEvent&, funkgui::GestureController&) override;
        bool wheel(const funkgui::WheelEvent&, funkgui::GestureController&, double) override;
        bool key(const funkgui::KeyEvent&, funkgui::GestureController&) override;
        bool takesAllKeys() const override { return true; }
        void accessibility(std::vector<funkgui::A11yItem>&) const override;
        bool a11yAction(uint32_t id, funkgui::A11yAction, double, funkgui::GestureController&) override;
        void specLine(char* out, std::size_t n) const override;
        uint64_t drawKey() const override;

    protected:
        const char* kind() const override { return "chat"; }

    private:
        struct Block
        {
            ChatItem::Kind kind = ChatItem::Kind::Notice;
            float y = 0.0f, h = 0.0f;              // in the content
            markdown::Layout layout;               // at x 0 of its column
            float x = 0.0f;                        // its column's left, from the view's
            bool error = false;
            std::string text;                      // as laid out (shownText, accessibility)
        };

        void relayout();
        funkgui::Rect conversation() const noexcept;   // the scrolling part
        funkgui::Rect composer() const noexcept;       // the message box's row
        funkgui::Rect sendButton() const noexcept;

        Hooks hooks_;
        uint32_t a11yBase_;
        ChatPort* port_ = nullptr;
        std::uint64_t seen_ = ~0ull;
        float laidWidth_ = -1.0f;
        std::vector<Block> blocks_;
        markdown::Layout question_;                // Claude's question, over the box
        float questionH_ = 0.0f;
        markdown::Layout empty_;                   // the empty chat's words
        bool emptyShown_ = false;
        bool addKeyShown_ = false;                 // the empty chat cannot be used, and the frame has a key dialog
        bool newChatShown_ = false;                // there is a conversation to clear (the row over it is drawn)
        float addKeyY_ = 0.0f;                     // its top, in the conversation's content
        bool busy_ = false;
        bool asking_ = false;
        std::string draft_, hint_;
        tee::ui::Scroll scroll_;
        markdown::Metrics metrics_;
    };
}
