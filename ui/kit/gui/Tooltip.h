// ui/kit/gui/Tooltip.h: hover help (the owner's ask, 25 Nov: "tooltips for when the user hovers over a dial"). A panel
// that shows one (GuiPanel for its bound controls, Board for its widgets) tells it each tick what is under the pointer
// or holds the keyboard's focus; after a short dwell it asks the panel for the text (a title, the help, a line of facts:
// unit, range, default) and lays it out once, wrapped, beside what it describes: below it, else above, else to a side,
// never over it and never past the panel's edges. draw() only reads what show() made: nothing is allocated there.
#pragma once

#include <funkgui/canvas/Canvas.h>
#include <funkgui/core/Geometry.h>
#include <funkgui/core/Theme.h>

#include <cstdint>
#include <string>
#include <vector>

namespace tee::ui
{
    class Tooltip
    {
    public:
        static constexpr float kDwell = 0.55f;       // seconds under the pointer before it shows
        static constexpr float kFocusDwell = 0.35f;  // seconds a keyboard focus rests before it shows
        static constexpr float kMaxWidth = 320.0f;   // its box's widest, px
        static constexpr float kGap = 6.0f;          // from what it describes and from the panel's edges

        // Each tick: `key` names what is under the pointer or focused (-1: nothing), `dwell` how long it must rest.
        // True once, when it has rested long enough: the owner then calls show() (or not: a thing with nothing to say).
        // Any change of key hides it.
        bool track(float dt, int key, float dwell);
        // A press on `key` (a click, a drag): its help hides and stays away while the pointer stays on it (shown again
        // only after the pointer has left it), as a tooltip should not cover what was just used.
        void pressed(int key) noexcept
        {
            quiet_ = key;
            hide();
        }
        // Lays out `title` (may be empty), `body` (wrapped) and `facts` (wrapped, smaller) beside `anchor` in a panel
        // `w` x `h`. An empty title and body: nothing shown.
        void show(const std::string& title, const std::string& body, const std::string& facts, const funkgui::Rect& anchor,
                  float w, float h);
        void hide();

        bool shown() const noexcept { return shown_; }
        int key() const noexcept { return key_; }
        const funkgui::Rect& box() const noexcept { return box_; }
        const funkgui::Rect& anchor() const noexcept { return anchor_; }
        std::string text() const;                    // every line, joined by newlines (tests, accessibility)
        uint64_t drawKey() const noexcept { return shown_ ? revision_ : 0; }

        void draw(funkgui::Canvas& c, const funkgui::Theme& th) const;

    private:
        struct Line
        {
            std::string text;
            int style = 0;                           // 0 title, 1 body, 2 facts
            float y = 0.0f;
        };
        int key_ = -1, quiet_ = -1;
        float rest_ = 0.0f;
        bool shown_ = false, asked_ = false;   // asked_: track() said so once for this key
        uint64_t revision_ = 0;
        funkgui::Rect box_{}, anchor_{};
        std::vector<Line> lines_;
    };
}
