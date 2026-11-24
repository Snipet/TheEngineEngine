// ui/app/TryFirst.h: the first launch's card (the lead's ask, 2026-11-24 20:19): "TRY FIRST", the build's name and
// the opening of its notes wrapped to the card's width, a strip across the stage's top; the frame places the workspace
// under it, so it covers nothing (the lead's correction, 20:28: over DRIVE it hid the speedometer).
// The frame shows it while FrameServices::tryFirst says something, and closes it for good (FrameServices::
// tryFirstDone, a per-viewer setting the shell keeps) at its Close button or once the engine has been started and
// stopped.
#pragma once

#include "app/Look.h"
#include "kit/widgets/Widget.h"

#include <string>
#include <vector>

namespace tee::ui
{
    class TryFirstCard final : public Widget
    {
    public:
        // A strip across the stage's top: "TRY FIRST" and the name on one line, the opening under them; the Close
        // button (the frame's) at the right, the text kept clear of it.
        static constexpr float kPad = 12.0f, kLineH = 16.0f, kHeadH = 24.0f, kButtonRoom = 88.0f;
        static constexpr int kMaxLines = 6;

        explicit TryFirstCard(uint32_t id);
        // The build's name and its notes (the opening paragraph is shown: up to the first blank line).
        void setText(std::string name, const std::string& notes);
        const std::string& opening() const noexcept { return opening_; }
        // The opening wrapped to a card `width` wide (kMaxLines at most, the last cut with an ellipsis), and the card's
        // height for it.
        static std::vector<std::string> wrap(const std::string& text, float width);
        float heightFor(float width) const;

        void setPlace(const funkgui::Rect& r, bool shown) override;   // wraps the opening to the new width
        void draw(funkgui::Canvas& c, const funkgui::Theme& th, bool focusRing) const override;
        bool focusable() const override { return false; }
        void accessibility(std::vector<funkgui::A11yItem>& out) const override;
        bool a11yAction(uint32_t, funkgui::A11yAction, double, funkgui::GestureController&) override { return false; }
        uint64_t drawKey() const override;

    protected:
        const char* kind() const override { return "card"; }

    private:
        std::string name_, opening_;
        std::vector<std::string> lines_;
    };
}
