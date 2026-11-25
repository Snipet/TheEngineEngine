// ui/app/KeyDialog.h: the key chat's key (the lead's ask, 25 Nov, on card A's interface: "the key's dialog masked").
// A card over the window, opened by the chat's "Add a key" or its KEY button: what is saved (the store's mask, "saved,
// ending ...abcd", never more), a field the key is typed or pasted into, shown as one star a character and never as
// itself, and SAVE, REMOVE and CLOSE. Saving hands the typed key to the shell (FrameServices::saveKey, the system's
// key store) and forgets it here at once, taken or not; closing forgets it too. A refusal is shown in words.
//
// While open it covers the window (every press and key is its own: the frame behind waits) and takes every key:
// Enter saves, Escape closes, Cmd/Ctrl-V pastes the clipboard's text (printable ASCII; spaces and line breaks
// dropped), Backspace deletes the last character (Cmd/Ctrl: all). Accessibility: the field says how many characters
// it holds, never what they are.
#pragma once

#include "app/Look.h"
#include "kit/widgets/Widget.h"

#include <functional>
#include <string>
#include <vector>

namespace tee::ui
{
    class KeyDialog final : public Widget
    {
    public:
        struct Hooks
        {
            std::function<bool(std::string typed, std::string& why)> save;
            std::function<std::string()> mask;          // "none saved", "saved, ending ...abcd", or why there is no store
            std::function<bool(std::string& why)> remove;
            std::function<std::string()> clipboard;     // the paste's text (the host's clipboard)
            std::function<void()> closed;               // after it closed (the frame gives the focus back)
        };
        static constexpr int kMaxKey = 512;
        static constexpr float kCardW = 440.0f, kCardH = 214.0f;

        KeyDialog(uint32_t id, Hooks hooks);

        void open();
        void close();
        bool isOpen() const noexcept { return open_; }

        // For tests and the frame: how many characters are typed (never what), the line under the field (a refusal or
        // what was done), the store's mask as shown, and where things are.
        std::size_t typed() const noexcept { return key_.size(); }
        const std::string& message() const noexcept { return message_; }
        bool messageIsError() const noexcept { return error_; }
        const std::string& mask() const noexcept { return mask_; }
        funkgui::Rect card() const noexcept;
        funkgui::Rect field() const noexcept;
        funkgui::Rect button(int i) const noexcept;     // 0 SAVE, 1 REMOVE, 2 CLOSE
        bool canRemove() const noexcept { return mask_.rfind("saved", 0) == 0; }
        // The press and the key, as pointerDown and key() take them (the tests' way in, without the Panel's controllers).
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
        void save();
        void remove();
        void forget();                                  // the typed key wiped
        void say(std::string text, bool error);

        Hooks hooks_;
        bool open_ = false, error_ = false;
        std::string key_, message_, mask_;
        std::uint64_t revision_ = 0;
    };
}
