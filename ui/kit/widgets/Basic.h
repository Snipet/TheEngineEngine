// ui/kit/widgets/Basic.h: the small widgets of card J2 (item 6): Button, Toggle, Stepper, TextField, List, Bar, Lamp.
// Each is a Widget (Widget.h): placed by its owner, eased, drawn with the Canvas alone. What a control turns is a
// funkgui::ParamPort (a value 0..1: J1's SourcePort, J2's FieldPort), written through the Panel's GestureController (a
// click is one gesture, a drag one, a wheel burst one); what a readout shows it reads through a function each tick.
#pragma once

#include "kit/widgets/Widget.h"

#include <funkgui/params/ParamPort.h>
#include <funkgui/text/LineEdit.h>

#include <functional>
#include <string>
#include <vector>

namespace tee::ui
{
    // A press that does something: a click (released over it), Enter or Space with the focus, a screen reader's press.
    // held(): a press held down (a start button) calls onHold(true) at the press and onHold(false) at the release.
    // lit(): it shows as on (a latched mode) while this answers true.
    class Button final : public Widget
    {
    public:
        Button(uint32_t id, std::string label, std::function<void()> onPress = {});
        void setOnHold(std::function<void(bool)> f) { onHold_ = std::move(f); }
        void setLit(std::function<bool()> f) { lit_ = std::move(f); }
        // U1: room kept at the left (px) for what its owner draws there (the rail's pictogram); the title is centred in
        // the rest.
        void setLeftRoom(float px) noexcept { leftRoom_ = px; }
        // U1: whether it can be pressed now (none: always). Disabled, it draws dim, takes no press, no hold, no key,
        // and tells a screen reader so.
        void setEnabled(std::function<bool()> f) { enabled_ = std::move(f); }
        bool enabled() const { return !enabled_ || enabled_(); }
        int presses() const noexcept { return presses_; }

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        funkgui::Cursor cursorAt(funkgui::Point) const override { return funkgui::Cursor::pointingHand; }
        void pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&) override;
        void pointerDrag(const funkgui::PointerEvent& e, funkgui::GestureController&) override;
        void pointerUp(const funkgui::PointerEvent& e, funkgui::GestureController&) override;
        bool key(const funkgui::KeyEvent& e, funkgui::GestureController&) override;
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t id, funkgui::A11yAction a, double, funkgui::GestureController&) override;
        uint64_t drawKey() const override;

    protected:
        void step(float dt) override;
        bool still() const override { return down_ == (held_ ? 1.0f : 0.0f) && litShown_ == (isLit() ? 1.0f : 0.0f); }
        const char* kind() const override { return "button"; }

    private:
        bool isLit() const { return lit_ && lit_(); }
        void fire();

        std::function<void()> onPress_;
        std::function<void(bool)> onHold_;
        std::function<bool()> lit_, enabled_;
        bool held_ = false, inside_ = false;
        float down_ = 0.0f, litShown_ = 0.0f, leftRoom_ = 0.0f;
        int presses_ = 0;
    };

    // A switch of a bool (or a two-valued int): its label at the left, a track with a knob that slides.
    class Toggle final : public Widget
    {
    public:
        Toggle(uint32_t id, funkgui::ParamPort& port, std::string label);

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        funkgui::Cursor cursorAt(funkgui::Point) const override { return funkgui::Cursor::pointingHand; }
        void pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController& g, funkgui::HostServices&) override;
        bool key(const funkgui::KeyEvent& e, funkgui::GestureController& g) override;
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t id, funkgui::A11yAction a, double, funkgui::GestureController& g) override;
        uint64_t drawKey() const override;

    protected:
        void step(float dt) override;
        bool still() const override { return on_ == target(); }
        const char* kind() const override { return "toggle"; }

    private:
        float target() const { return port_.value01() >= 0.5f ? 1.0f : 0.0f; }
        void flip(funkgui::GestureController& g) { g.tap(port_, target() > 0.5f ? 0.0f : 1.0f); }

        funkgui::ParamPort& port_;
        float on_ = 0.0f;
    };

    // A value stepped: [-] value [+]. A port with steps (a choice, an int) moves a step a click; a real one moves by
    // step01 (Shift: a tenth of it). The wheel, the arrow keys, Home and End, and a screen reader step it too.
    class Stepper final : public Widget
    {
    public:
        Stepper(uint32_t id, funkgui::ParamPort& port, std::string label, std::function<std::string(float v01)> text,
                float step01 = 0.01f);

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        funkgui::Cursor cursorAt(funkgui::Point p) const override;
        void pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController& g, funkgui::HostServices&) override;
        bool wheel(const funkgui::WheelEvent& e, funkgui::GestureController& g, double now) override;
        bool key(const funkgui::KeyEvent& e, funkgui::GestureController& g) override;
        void doubleClick(const funkgui::PointerEvent& e, funkgui::GestureController& g) override;
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t id, funkgui::A11yAction a, double v, funkgui::GestureController& g) override;
        uint64_t drawKey() const override;

        funkgui::Rect minusRect() const noexcept;
        funkgui::Rect plusRect() const noexcept;

    protected:
        const char* kind() const override { return "stepper"; }

    private:
        float stepOf(bool fine) const noexcept;
        float moved(int dir, bool fine) const noexcept;

        funkgui::ParamPort& port_;
        std::function<std::string(float)> text_;
        float step01_;
    };

    // One line of text to type: a click (or Enter with the focus) starts editing with the text selected, Enter
    // commits (onCommit answers whether it took it; a refusal keeps the field open), Escape gives up, and leaving the
    // field commits. While it is not being edited it shows value(). funkgui::text::LineEdit holds the editing.
    class TextField final : public Widget
    {
    public:
        TextField(uint32_t id, std::string label, std::function<std::string()> value,
                  std::function<bool(const std::string&)> onCommit, int maxLength = 32);

        bool editing() const noexcept { return editing_; }
        const std::string& buffer() const noexcept { return edit_.buffer; }
        void beginEdit();

        // J2, for card A's chat (the defaults are the field as it always was):
        //   enterOnly   only Enter commits. Leaving the field (a press elsewhere, Tab, Escape, the focus going) keeps
        //               the typed text as a draft, neither committed nor dropped, shown in the field and taken up again
        //               when it is entered; a commit that is taken empties it back to value() and typing goes on.
        //   multiline   Shift+Enter starts a new line ('\n' in the text); the field shows as many lines as its height
        //               holds, the caret's line in view.
        struct Options
        {
            bool enterOnly = false;
            bool multiline = false;
        };
        void setOptions(Options o) noexcept { options_ = o; }
        const Options& options() const noexcept { return options_; }
        bool hasDraft() const noexcept { return draft_; }
        bool refused() const noexcept { return refused_; }

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        funkgui::Cursor cursorAt(funkgui::Point) const override { return funkgui::Cursor::normal; }
        void pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&) override;
        bool key(const funkgui::KeyEvent& e, funkgui::GestureController&) override;
        void focusChanged(bool focused) override;
        bool takesAllKeys() const override { return editing_; }
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t id, funkgui::A11yAction a, double, funkgui::GestureController&) override;
        uint64_t drawKey() const override;

    protected:
        const char* kind() const override { return "textfield"; }

    private:
        bool commit();

        std::function<std::string()> value_;
        std::function<bool(const std::string&)> onCommit_;
        int maxLength_;
        funkgui::text::LineEdit edit_;
        bool editing_ = false, refused_ = false;
        Options options_{};
        bool draft_ = false;                             // enterOnly: text kept from the last time it was left
        void leave();                                    // enterOnly: stop editing, the text kept as the draft
    };

    // Rows of text, one chosen: a click or the arrow keys choose (onSelect), the wheel scrolls, the chosen row is kept
    // in view.
    class List final : public Widget
    {
    public:
        static constexpr float kRowH = 22.0f;

        List(uint32_t id, std::string label, std::vector<std::string> rows, std::function<void(int)> onSelect = {});
        void setRows(std::vector<std::string> rows);
        const std::vector<std::string>& rows() const noexcept { return rows_; }
        int selected() const noexcept { return selected_; }
        void select(int row, bool notify = true);
        float scroll() const noexcept { return scroll_; }

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        funkgui::Cursor cursorAt(funkgui::Point) const override { return funkgui::Cursor::pointingHand; }
        void pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&) override;
        bool wheel(const funkgui::WheelEvent& e, funkgui::GestureController&, double) override;
        bool key(const funkgui::KeyEvent& e, funkgui::GestureController&) override;
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t id, funkgui::A11yAction a, double, funkgui::GestureController&) override;
        void setPlace(const funkgui::Rect& r, bool shown) override;
        uint64_t drawKey() const override;

    protected:
        void step(float dt) override;
        bool still() const override { return shownScroll_ == scroll_; }
        const char* kind() const override { return "list"; }

    private:
        funkgui::Rect rowsArea() const noexcept;
        void scrollTo(float s);
        void keepInView();

        std::vector<std::string> rows_;
        std::function<void(int)> onSelect_;
        int selected_ = -1;
        float scroll_ = 0.0f, shownScroll_ = 0.0f;
    };

    // A level as a filled bar (a pedal, a duty): value() 0..1, its text at the right (or none), its label over it.
    class Bar final : public Widget
    {
    public:
        Bar(uint32_t id, std::string label, std::function<float()> value01, std::function<std::string()> text = {},
            bool vertical = false);
        float shownValue() const noexcept { return shown01_; }

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        bool focusable() const override { return false; }
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t, funkgui::A11yAction, double, funkgui::GestureController&) override { return false; }
        uint64_t drawKey() const override;

    protected:
        void step(float dt) override;
        bool still() const override { return shown01_ == target_; }
        const char* kind() const override { return "bar"; }

    private:
        std::function<float()> value_;
        std::function<std::string()> text_;
        bool vertical_;
        float target_ = 0.0f, shown01_ = 0.0f;
    };

    // Text that something else says (a build's name, a state, a readout): text() each tick, in a style and an ink,
    // cut with an ellipsis to its width. A caption (optional) over it in the labels' style.
    class Label final : public Widget
    {
    public:
        enum class Ink : std::uint8_t { value, text, label, accent };
        Label(uint32_t id, std::function<std::string()> text, funkgui::TextStyle style = kWidgetText, Ink ink = Ink::value,
              funkgui::Align align = funkgui::Align::left, std::string caption = {});
        const std::string& shownText() const noexcept { return text_; }

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        bool focusable() const override { return false; }
        bool contains(funkgui::Point) const override { return false; }   // presses go through it
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t, funkgui::A11yAction, double, funkgui::GestureController&) override { return false; }
        uint64_t drawKey() const override;

    protected:
        void step(float dt) override;
        const char* kind() const override { return "label"; }

    private:
        std::function<std::string()> source_;
        funkgui::TextStyle style_;
        Ink ink_;
        funkgui::Align align_;
        std::string caption_, text_;
    };

    // A light: on() lights it (in its colour; the accent when none is given), eased; its label beside it.
    class Lamp final : public Widget
    {
    public:
        Lamp(uint32_t id, std::string label, std::function<bool()> on, funkgui::Col color = { 0, 0, 0, 0 });
        float glow() const noexcept { return glow_; }

        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        bool focusable() const override { return false; }
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t, funkgui::A11yAction, double, funkgui::GestureController&) override { return false; }
        uint64_t drawKey() const override;

    protected:
        void step(float dt) override;
        bool still() const override { return glow_ == (lit_ ? 1.0f : 0.0f); }
        const char* kind() const override { return "lamp"; }

    private:
        std::function<bool()> on_;
        funkgui::Col color_;
        bool lit_ = false;
        float glow_ = 0.0f;
    };
}
