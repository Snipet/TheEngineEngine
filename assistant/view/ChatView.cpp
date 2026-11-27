// assistant/view/ChatView.cpp: the key chat's panel (ChatView.h).
#include "ChatView.h"

#include "kit/gui/Style.h"

#include <algorithm>
#include <cstdio>

namespace tee::assistant::view
{
    namespace
    {
        constexpr float kPad = 12.0f;          // the column's inset
        constexpr float kGap = 10.0f;          // between blocks
        constexpr float kNewChatRowH = 34.0f;  // the row "New chat" sits in, over the conversation (only when shown)
        constexpr float kBubblePad = 8.0f;     // inside the user's bubble
        constexpr float kComposerH = 40.0f;    // the message box's row
        constexpr float kSendW = 56.0f;

        std::string label(ChatItem::Kind k)
        {
            switch (k)
            {
                case ChatItem::Kind::User: return "you";
                case ChatItem::Kind::Assistant: return "claude";
                case ChatItem::Kind::Tool: return "tool";
                case ChatItem::Kind::Thinking: return "thinking";
                default: return "note";
            }
        }

        // The chat's colours from the kit's widget look (ui/kit/widgets/Widget.h, the frame's tokens), in the shape the
        // ported markdown draws with (ui/kit/gui/Style.h's Palette).
        tee::ui::style::Palette paletteOf(const funkgui::Theme& th)
        {
            const tee::ui::Look l = tee::ui::Look::of(th);
            tee::ui::style::Palette p = tee::ui::style::palette(th);
            p.card = l.ground;
            p.cardEdge = l.edge;
            p.bubble = l.face;
            p.code = l.face;
            p.codeEdge = l.edge;
            p.composer = l.face;
            p.composerEdge = l.edge;
            p.hover = l.faceHi;
            p.raised = l.faceHi;
            p.text = l.text;
            p.soft = l.label;
            p.dim = l.label;
            p.faint = l.dim;
            p.accent = l.accent;
            p.danger = l.signal;
            return p;
        }

        constexpr std::size_t kThinkingShown = 280;   // the last characters of a long summary
    }

    ChatView::ChatView(Hooks hooks, uint32_t id) : Widget(id), hooks_(std::move(hooks)), a11yBase_(id)
    {
        scroll_.row = tee::ui::style::kBodyLine;
        setTitle("Chat with Claude");
        setHelp("Claude, with your API key: ask about the build, or ask it to change something. Each change it makes is "
                "one step the app's Undo takes back. Enter sends; Escape stops an answer.");
    }

    funkgui::Rect ChatView::composer() const noexcept
    {
        return {bounds_.x + kPad, bounds_.bottom() - kComposerH - kPad * 0.5f, bounds_.w - 2.0f * kPad, kComposerH};
    }

    funkgui::Rect ChatView::sendButton() const noexcept
    {
        const funkgui::Rect c = composer();
        return {c.right() - kSendW - 6.0f, c.y + 6.0f, kSendW, c.h - 12.0f};
    }

    funkgui::Rect ChatView::conversation() const noexcept
    {
        const float bottom = composer().y - kGap - (asking_ ? questionH_ + kGap : 0.0f);
        const float top = bounds_.y + (newChatShown_ ? kNewChatRowH : 0.0f);   // under "New chat", when it is there
        return {bounds_.x, top, bounds_.w, std::max(0.0f, bottom - top)};
    }

    std::optional<funkgui::Rect> ChatView::newChatButton() const
    {
        if (!newChatShown_)
            return std::nullopt;
        return funkgui::Rect{bounds_.right() - kPad - 84.0f, bounds_.y + 6.0f, 84.0f, kNewChatRowH - 10.0f};
    }

    void ChatView::relayout()
    {
        const bool wasAtEnd = atEnd();
        blocks_.clear();
        const float w = std::max(80.0f, bounds_.w - 2.0f * kPad - 8.0f);
        float y = kPad;
        std::vector<ChatItem> items;
        std::string question, why;
        bool canChat = false;
        if (port_ != nullptr)
        {
            items = port_->items();
            question = port_->question();
            busy_ = port_->busy();
            canChat = port_->ready(why);
            newChatShown_ = port_->canClear();
            // a message the API could not take comes back to the box (unless the owner has typed since)
            if (std::string back = port_->takeReturned(); !back.empty() && draft_.empty())
                draft_ = std::move(back);
        }
        else
            newChatShown_ = false;
        for (const ChatItem& item : items)
        {
            Block b;
            b.kind = item.kind;
            b.error = item.error;
            b.y = y;
            float bottom = y;
            switch (item.kind)
            {
                case ChatItem::Kind::User:
                {
                    // the owner's own words, as typed (no markdown), in a bubble at the right
                    const float bw = w * 0.8f;
                    b.text = tee::ui::asciiText(item.text);
                    float yy = kBubblePad;
                    bottom = markdown::flow(b.layout, metrics_, b.text, markdown::Role::body, kBubblePad, bw - 2 * kBubblePad, yy,
                                            tee::ui::style::kBodyLine, false)
                             + kBubblePad;
                    float widest = 0.0f;
                    for (const auto& s : b.layout.spans)
                        widest = std::max(widest, s.x + metrics_.adv(s.role) * static_cast<float>(s.text.size()));
                    const float shown = std::min(bw, widest + kBubblePad);
                    b.layout.shapes.insert(b.layout.shapes.begin(), {{0.0f, 0.0f, shown, bottom}, markdown::ShapeKind::codeCard});
                    b.x = kPad + (w - shown);
                    b.h = bottom;
                    break;
                }
                case ChatItem::Kind::Assistant:
                {
                    b.text = tee::ui::asciiText(item.text);
                    float yy = 0.0f;
                    markdown::layout(b.layout, metrics_, b.text, item.text, w, yy);
                    b.x = kPad;
                    b.h = std::max(yy, tee::ui::style::kBodyLine);
                    break;
                }
                case ChatItem::Kind::Tool:
                {
                    b.text = item.tool + (item.error ? ": " + tee::ui::asciiText(item.text) : std::string(": done"));
                    b.layout.spans.push_back({0.0f, 0.0f, tee::ui::style::kSmallLine, b.text,
                                              item.error ? markdown::Role::danger : markdown::Role::dim});
                    b.x = kPad + 4.0f;
                    b.h = tee::ui::style::kSmallLine;
                    break;
                }
                case ChatItem::Kind::Thinking:
                {
                    // a summary of Claude's thinking: dim and small, its last part when long; while none is given
                    // and it streams, a word that says Claude is thinking (the wait is honest)
                    std::string t = tee::ui::asciiText(item.text);
                    if (t.size() > kThinkingShown)
                        t = "..." + t.substr(t.size() - kThinkingShown);
                    b.text = t.empty() ? std::string(item.streaming ? "Thinking..." : "Thought") : "Thinking: " + t;
                    if (t.empty() && !item.streaming)
                        continue;   // a finished thought with nothing to say: no row
                    float yy = 0.0f;
                    b.h = markdown::flow(b.layout, metrics_, b.text, markdown::Role::faint, 0.0f, w, yy,
                                         tee::ui::style::kSmallLine, false);
                    b.x = kPad;
                    break;
                }
                case ChatItem::Kind::Notice:
                {
                    b.text = tee::ui::asciiText(item.text);
                    float yy = 0.0f;
                    b.h = markdown::flow(b.layout, metrics_, b.text, item.error ? markdown::Role::danger : markdown::Role::small,
                                         0.0f, w, yy, tee::ui::style::kSmallLine, false);
                    b.x = kPad;
                    break;
                }
            }
            y += b.h + kGap;
            blocks_.push_back(std::move(b));
        }
        // a turn running with nothing of Claude's arriving yet (the request on its way, a tool running): said so
        if (busy_ && (items.empty() || items.back().kind == ChatItem::Kind::User || items.back().kind == ChatItem::Kind::Tool
                      || (items.back().kind != ChatItem::Kind::Assistant && items.back().kind != ChatItem::Kind::Thinking)))
        {
            Block b;
            b.kind = ChatItem::Kind::Thinking;
            b.y = y;
            b.text = "Claude is working...";
            b.layout.spans.push_back({0.0f, 0.0f, tee::ui::style::kSmallLine, b.text, markdown::Role::faint});
            b.x = kPad;
            b.h = tee::ui::style::kSmallLine;
            y += b.h + kGap;
            blocks_.push_back(std::move(b));
        }
        // empty: what the chat is, or why it cannot be used yet
        empty_ = markdown::Layout{};
        emptyShown_ = items.empty();
        if (emptyShown_)
        {
            float yy = kPad;
            const std::string words = canChat
                                          ? "Ask Claude about the engine on the bench: what a field does, what to change "
                                            "for more low-end torque, what the curve says. Each change it makes is one "
                                            "step you can undo."
                                          : why;
            yy = markdown::flow(empty_, metrics_, tee::ui::asciiText(words), markdown::Role::dim, kPad, w, yy,
                                tee::ui::style::kBodyLine, false);
            addKeyY_ = yy + kGap;
        }
        addKeyShown_ = emptyShown_ && !canChat && hooks_.addKey != nullptr;
        // Claude's question, over the box
        question_ = markdown::Layout{};
        asking_ = !question.empty();
        questionH_ = 0.0f;
        if (asking_)
        {
            float yy = 8.0f;
            questionH_ = markdown::flow(question_, metrics_, "Claude asks: " + tee::ui::asciiText(question), markdown::Role::strong,
                                        8.0f, w - 16.0f, yy, tee::ui::style::kBodyLine, false)
                         + 8.0f;
            question_.shapes.insert(question_.shapes.begin(), {{0.0f, 0.0f, w, questionH_}, markdown::ShapeKind::codeCard});
        }
        scroll_.content = y;
        scroll_.view = conversation().h;
        scroll_.clamp();
        if (wasAtEnd || busy_)
            scroll_.moveTo(scroll_.max());   // follow the conversation as it grows, unless the owner scrolled up
        laidWidth_ = bounds_.w;
    }

    void ChatView::tick(float dt, bool hovered, bool focused, funkgui::Point pointer)
    {
        Widget::tick(dt, hovered, focused, pointer);
        const std::uint64_t rev = port_ != nullptr ? port_->revision() : 0;
        if (rev != seen_ || laidWidth_ != bounds_.w)
        {
            seen_ = rev;
            relayout();
        }
    }

    bool ChatView::submit()
    {
        if (port_ == nullptr || draft_.empty())
            return false;
        if (!port_->question().empty())
        {
            if (port_->answer(draft_))
            {
                draft_.clear();
                hint_.clear();
                return true;
            }
            return false;
        }
        std::string why;
        if (!port_->send(draft_, why))
        {
            hint_ = why;
            return false;
        }
        draft_.clear();
        hint_.clear();
        scroll_.moveTo(scroll_.max());
        return true;
    }

    void ChatView::draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const
    {
        const tee::ui::style::Palette p = paletteOf(th);
        c.rrect(bounds_.x, bounds_.y, bounds_.w, bounds_.h, 8.0f, p.card, 1.0f, p.cardEdge);
        const funkgui::Rect area = conversation();
        const float top = area.y - scroll_.offset;
        if (emptyShown_)
            markdown::draw(c, th, p, empty_, metrics_, bounds_.x, top, area);
        if (const auto nc = newChatButton(); nc.has_value())
        {
            const bool hotNew = nc->contains(pointer_);
            c.rrect(nc->x, nc->y, nc->w, nc->h, 6.0f, hotNew ? p.accent : p.raised, 0.0f, {});
            markdown::drawText(c, "New chat", nc->x + 12.0f, nc->y, nc->h, markdown::Role::smallStrong,
                               hotNew ? p.onAccent : p.text);
        }
        if (const auto ak = addKeyButton(); ak.has_value())
        {
            const bool hotKey = ak->contains(pointer_);
            c.rrect(ak->x, ak->y, ak->w, ak->h, 6.0f, p.accent, hotKey ? 2.0f : 0.0f, p.text);
            markdown::drawText(c, "Add a key", ak->x + 12.0f, ak->y, ak->h, markdown::Role::smallStrong, p.onAccent);
        }
        for (const Block& b : blocks_)
        {
            if (top + b.y + b.h < area.y || top + b.y > area.bottom())
                continue;
            markdown::draw(c, th, p, b.layout, metrics_, bounds_.x + b.x, top + b.y, area);
        }
        scroll_.draw(c, area, th);
        if (asking_)
            markdown::draw(c, th, p, question_, metrics_, bounds_.x + kPad, composer().y - kGap - questionH_, bounds_);
        // the message box: the draft (or its hint), and Send or Stop
        const funkgui::Rect box = composer();
        c.rrect(box.x, box.y, box.w, box.h, 8.0f, p.composer, focusRing ? 2.0f : 1.0f, focusRing ? p.accent : p.composerEdge);
        const std::string shown = !draft_.empty() ? tee::ui::asciiText(draft_)
                                  : !hint_.empty() ? tee::ui::asciiText(hint_)
                                  : asking_        ? std::string("Your answer...")
                                                   : std::string("Ask Claude...");
        markdown::drawText(c, shown, box.x + 10.0f, box.y, box.h, draft_.empty() ? markdown::Role::dim : markdown::Role::body,
                           draft_.empty() ? (hint_.empty() ? p.faint : p.danger) : p.text);
        const funkgui::Rect sb = sendButton();
        const bool hot = sb.contains(pointer_);
        c.rrect(sb.x, sb.y, sb.w, sb.h, 6.0f, busy_ ? p.raised : (hot ? p.accent : p.raised), 0.0f, {});
        markdown::drawText(c, busy_ ? "Stop" : "Send", sb.x + 10.0f, sb.y, sb.h, markdown::Role::smallStrong,
                           busy_ ? p.text : (hot ? p.onAccent : p.text));
    }

    funkgui::Cursor ChatView::cursorAt(funkgui::Point pt) const
    {
        const auto ak = addKeyButton();
        const auto nc = newChatButton();
        return sendButton().contains(pt) || (ak.has_value() && ak->contains(pt)) || (nc.has_value() && nc->contains(pt))
                   ? funkgui::Cursor::pointingHand
                   : funkgui::Cursor::normal;
    }

    void ChatView::pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&)
    {
        press({e.x, e.y});
    }

    void ChatView::press(funkgui::Point pt)
    {
        if (sendButton().contains(pt))
        {
            if (busy_ && port_ != nullptr)
                port_->stop();
            else
                submit();
            return;
        }
        if (const auto ak = addKeyButton(); ak.has_value() && ak->contains(pt))
        {
            hooks_.addKey();
            return;
        }
        if (const auto nc = newChatButton(); nc.has_value() && nc->contains(pt) && port_ != nullptr)
        {
            port_->clear();
            hint_.clear();
            seen_ = ~0ull;   // laid out again at the next tick
            return;
        }
        scroll_.press(pt, conversation());
    }

    void ChatView::pointerDrag(const funkgui::PointerEvent& e, funkgui::GestureController&)
    {
        if (scroll_.dragging())
            scroll_.drag({e.x, e.y}, conversation());
    }

    void ChatView::pointerUp(const funkgui::PointerEvent&, funkgui::GestureController&)
    {
        scroll_.release();
    }

    bool ChatView::wheel(const funkgui::WheelEvent& e, funkgui::GestureController&, double)
    {
        return scroll_.wheel(e.dy, e.smooth);
    }

    bool ChatView::key(const funkgui::KeyEvent& e, funkgui::GestureController&)
    {
        return type(e);
    }

    bool ChatView::type(const funkgui::KeyEvent& e)
    {
        using K = funkgui::Key;
        switch (e.key)
        {
            case K::enter:
                submit();
                return true;
            case K::escape:
                if (busy_ && port_ != nullptr)
                {
                    port_->stop();
                    return true;
                }
                return false;
            case K::backspace:
                if (!draft_.empty())
                {
                    // one character back, a UTF-8 sequence whole
                    std::size_t n = draft_.size() - 1;
                    while (n > 0 && (static_cast<unsigned char>(draft_[n]) & 0xC0) == 0x80)
                        --n;
                    draft_.erase(n);
                }
                return true;
            case K::space:
                draft_ += ' ';
                return true;
            case K::pageUp:
                return scroll_.moveTo(scroll_.offset - scroll_.view * 0.9f);
            case K::pageDown:
                return scroll_.moveTo(scroll_.offset + scroll_.view * 0.9f);
            case K::character:
            {
                const char32_t ch = e.ch;
                if (ch < 0x20 || ch == 0x7F)
                    return false;
                if (ch < 0x80)
                    draft_ += static_cast<char>(ch);
                else if (ch < 0x800)
                {
                    draft_ += static_cast<char>(0xC0 | (ch >> 6));
                    draft_ += static_cast<char>(0x80 | (ch & 0x3F));
                }
                else if (ch < 0x10000)
                {
                    draft_ += static_cast<char>(0xE0 | (ch >> 12));
                    draft_ += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                    draft_ += static_cast<char>(0x80 | (ch & 0x3F));
                }
                else
                {
                    draft_ += static_cast<char>(0xF0 | (ch >> 18));
                    draft_ += static_cast<char>(0x80 | ((ch >> 12) & 0x3F));
                    draft_ += static_cast<char>(0x80 | ((ch >> 6) & 0x3F));
                    draft_ += static_cast<char>(0x80 | (ch & 0x3F));
                }
                hint_.clear();
                return true;
            }
            default:
                return false;
        }
    }

    void ChatView::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem list;
        list.id = a11yBase_;
        list.role = funkgui::A11yRole::staticText;
        list.bounds = conversation();
        list.title = "Conversation with Claude";
        list.value = shownText();
        list.readOnly = true;
        out.push_back(list);
        funkgui::A11yItem box;
        box.id = a11yBase_ + 1;
        box.role = funkgui::A11yRole::staticText;
        box.bounds = composer();
        box.title = asking_ ? "Your answer to Claude" : "Message to Claude";
        box.value = draft_;
        out.push_back(box);
        funkgui::A11yItem send;
        send.id = a11yBase_ + 2;
        send.role = funkgui::A11yRole::button;
        send.bounds = sendButton();
        send.title = busy_ ? "Stop Claude" : "Send";
        out.push_back(send);
        if (const auto nc = newChatButton(); nc.has_value())
        {
            funkgui::A11yItem fresh;
            fresh.id = a11yBase_ + 4;
            fresh.role = funkgui::A11yRole::button;
            fresh.bounds = *nc;
            fresh.title = "New chat";
            out.push_back(fresh);
        }
        if (const auto ak = addKeyButton(); ak.has_value())
        {
            funkgui::A11yItem add;
            add.id = a11yBase_ + 3;
            add.role = funkgui::A11yRole::button;
            add.bounds = *ak;
            add.title = "Add a key";
            out.push_back(add);
        }
    }

    bool ChatView::a11yAction(uint32_t id, funkgui::A11yAction, double, funkgui::GestureController&)
    {
        if (id == a11yBase_ + 3 && addKeyButton().has_value())
        {
            hooks_.addKey();
            return true;
        }
        if (id == a11yBase_ + 4 && newChatButton().has_value() && port_ != nullptr)
        {
            port_->clear();
            seen_ = ~0ull;
            return true;
        }
        if (id != a11yBase_ + 2)
            return false;
        if (busy_ && port_ != nullptr)
            port_->stop();
        else
            submit();
        return true;
    }

    void ChatView::specLine(char* out, std::size_t n) const
    {
        std::snprintf(out, n, "chat blocks=%zu busy=%d asking=%d draft=%zu", blocks_.size(), busy_ ? 1 : 0, asking_ ? 1 : 0,
                      draft_.size());
    }

    uint64_t ChatView::drawKey() const
    {
        tee::ui::DrawKey k;
        k.mix(seen_).mix(addKeyShown_ ? 1 : 0).mix(newChatShown_ ? 1 : 0).mix(draft_).mix(hint_).mix(scroll_.key()).mix(bounds_).mix(pointer_).mix(busy_ ? 1 : 0);
        return k.h;
    }

    std::string ChatView::shownText() const
    {
        std::string s;
        if (emptyShown_)
            for (const auto& sp : empty_.spans)
                s += "empty: " + sp.text + "\n";
        for (const Block& b : blocks_)
        {
            s += label(b.kind) + (b.error ? " (error)" : "") + ":";
            for (const auto& sp : b.layout.spans)
                s += " " + sp.text;
            s += "\n";
        }
        if (asking_)
            for (const auto& sp : question_.spans)
                s += "question: " + sp.text + "\n";
        return s;
    }

    std::optional<funkgui::Point> ChatView::centreOfSend() const
    {
        const funkgui::Rect r = sendButton();
        return funkgui::Point{r.centreX(), r.centreY()};
    }

    std::optional<funkgui::Rect> ChatView::addKeyButton() const
    {
        if (!addKeyShown_)
            return std::nullopt;
        const funkgui::Rect area = conversation();
        const funkgui::Rect r{area.x + kPad, area.y - scroll_.offset + addKeyY_, 104.0f, 28.0f};
        if (r.y < area.y || r.bottom() > area.bottom())
            return std::nullopt;   // scrolled out of the conversation's part: not pressable where it is not drawn
        return r;
    }

    std::optional<funkgui::Point> ChatView::centreOfAddKey() const
    {
        const auto r = addKeyButton();
        if (!r.has_value())
            return std::nullopt;
        return funkgui::Point{r->centreX(), r->centreY()};
    }
}
