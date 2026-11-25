// ui/app/AskDialog.h: a question the assistant puts to the owner (card A's SessionParts::ask; the lead's ask, 25 Nov):
// "Claude asks to load "Hot cam". The open build, "Demo V8", has unsaved changes, which would be lost. Load it?" with
// LOAD and KEEP (the words are the asker's: YES and NO when it names none). A card over the window, one question at a
// time (the frame queues the rest: AppFrame::ask). The safe answer (no: nothing is lost) has the keyboard's focus and is
// what Escape gives; Enter and Space press the button with the focus, Tab moves it. Nothing answers yes by itself.
//
// ChatSlot: the place the shell's chat panel is mounted in (AppFrame::mountChat), made when the frame is, before the
// two dialogs, so that they draw over the chat and take a press before it whenever they are open. It forwards every
// call to the widget mounted in it (none: nothing shown).
#pragma once

#include "app/Look.h"
#include "kit/widgets/Widget.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace tee::ui
{
    class AskDialog final : public Widget
    {
    public:
        static constexpr float kCardW = 460.0f;

        explicit AskDialog(uint32_t id);

        // Shows `question` with the two answers' words; `answered` is called once, with true for `yes`.
        void open(std::string question, std::string yes, std::string no, std::function<void(bool)> answered);
        void answer(bool yes);                          // as the owner would (a press, Escape)
        bool isOpen() const noexcept { return open_; }
        const std::string& question() const noexcept { return question_; }
        bool focusOnYes() const noexcept { return focusYes_; }

        funkgui::Rect card() const noexcept;
        funkgui::Rect button(bool yes) const noexcept;
        void press(funkgui::Point p);
        bool type(const funkgui::KeyEvent& e);

        // tee::ui::Widget
        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        funkgui::Cursor cursorAt(funkgui::Point p) const override;
        void pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&) override;
        bool key(const funkgui::KeyEvent& e, funkgui::GestureController&) override { return type(e); }
        bool wheel(const funkgui::WheelEvent&, funkgui::GestureController&, double) override { return open_; }
        bool takesAllKeys() const override { return open_; }
        bool focusable() const override { return open_; }
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t id, funkgui::A11yAction a, double, funkgui::GestureController&) override;
        uint64_t drawKey() const override;

    protected:
        const char* kind() const override { return "dialog"; }

    private:
        std::vector<std::string> lines() const;          // the question wrapped to the card

        bool open_ = false, focusYes_ = false;
        std::string question_, yes_, no_;
        std::function<void(bool)> answered_;
        std::uint64_t revision_ = 0;
    };

    class ChatSlot final : public Item
    {
    public:
        void set(std::unique_ptr<Item> item) { item_ = std::move(item); }
        Item* get() const noexcept { return item_.get(); }

        void tick(float dt, bool hovered, bool focused, funkgui::Point pointer) override
        {
            if (item_)
                item_->tick(dt, hovered, focused, pointer);
        }
        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool ring) const override
        {
            if (item_)
                item_->draw(c, th, ring);
        }
        bool contains(funkgui::Point p) const override { return item_ && item_->contains(p); }
        funkgui::Cursor cursorAt(funkgui::Point p) const override { return item_ ? item_->cursorAt(p) : funkgui::Cursor::normal; }
        void pointerMove(const funkgui::PointerEvent& e) override { if (item_) item_->pointerMove(e); }
        void pointerExit() override { if (item_) item_->pointerExit(); }
        void pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController& g, funkgui::HostServices& h) override
        {
            if (item_)
                item_->pointerDown(e, g, h);
        }
        void pointerDrag(const funkgui::PointerEvent& e, funkgui::GestureController& g) override { if (item_) item_->pointerDrag(e, g); }
        void pointerUp(const funkgui::PointerEvent& e, funkgui::GestureController& g) override { if (item_) item_->pointerUp(e, g); }
        void doubleClick(const funkgui::PointerEvent& e, funkgui::GestureController& g) override { if (item_) item_->doubleClick(e, g); }
        bool wheel(const funkgui::WheelEvent& e, funkgui::GestureController& g, double t) override { return item_ && item_->wheel(e, g, t); }
        bool magnify(const funkgui::MagnifyEvent& e) override { return item_ && item_->magnify(e); }
        bool takesButtons(funkgui::Point p) const override { return item_ && item_->takesButtons(p); }
        bool key(const funkgui::KeyEvent& e, funkgui::GestureController& g) override { return item_ && item_->key(e, g); }
        void accessibility(std::vector<funkgui::A11yItem>& out) const override { if (item_) item_->accessibility(out); }
        bool a11yAction(uint32_t id, funkgui::A11yAction a, double v, funkgui::GestureController& g) override
        {
            return item_ && item_->a11yAction(id, a, v, g);
        }
        void specLine(char* out, std::size_t n) const override
        {
            if (item_)
                item_->specLine(out, n);
            else if (n > 0)
                out[0] = '\0';
        }
        bool settled() const override { return !item_ || item_->settled(); }
        uint64_t drawKey() const override { return item_ ? item_->drawKey() : 0; }
        funkgui::Point centre() const override { return item_ ? item_->centre() : funkgui::Point{}; }
        bool shown() const override { return item_ && item_->shown(); }
        bool focusable() const override { return item_ && item_->focusable(); }
        void focusChanged(bool f) override { if (item_) item_->focusChanged(f); }
        bool takesAllKeys() const override { return item_ && item_->takesAllKeys(); }
        void setPlace(const funkgui::Rect& r, bool shown) override { if (item_) item_->setPlace(r, shown); }
        bool help(std::string& t, std::string& b, funkgui::Rect& a) const override { return item_ && item_->help(t, b, a); }
        int helpPart() const override { return item_ ? item_->helpPart() : 0; }

    private:
        std::unique_ptr<Item> item_;
    };
}
