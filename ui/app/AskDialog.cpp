// ui/app/AskDialog.cpp: see AskDialog.h.
#include "app/AskDialog.h"

#include <funkgui/text/FontService.h>
#include <funkgui/text/TextFit.h>

#include <algorithm>

namespace tee::ui
{
    namespace
    {
        constexpr float kPad = 18.0f, kLine = 17.0f, kButtonW = 96.0f, kButtonH = 30.0f;

        float widthOf(const std::string& s, const funkgui::TextStyle& st)
        {
            return funkgui::text::width(funkgui::FontService::get().atlas(), s.c_str(), st);
        }
        std::string upperOf(std::string s)
        {
            for (char& ch : s)
                ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
            return s;
        }
    }

    AskDialog::AskDialog(uint32_t id) : Widget(id)
    {
        title_ = "Claude asks";
        shown_ = false;
    }

    void AskDialog::open(std::string question, std::string yes, std::string no, std::function<void(bool)> answered)
    {
        question_ = std::move(question);
        yes_ = yes.empty() ? std::string("Yes") : std::move(yes);
        no_ = no.empty() ? std::string("No") : std::move(no);
        answered_ = std::move(answered);
        focusYes_ = false;   // the safe answer has the focus
        open_ = true;
        ++revision_;
    }

    void AskDialog::answer(bool yes)
    {
        if (!open_)
            return;
        open_ = false;
        ++revision_;
        auto f = std::move(answered_);
        answered_ = nullptr;
        if (f)
            f(yes);
    }

    std::vector<std::string> AskDialog::lines() const
    {
        std::vector<std::string> out;
        const float w = card().w - 2.0f * kPad;
        std::string line;
        std::size_t i = 0;
        while (i < question_.size())
        {
            const std::size_t e = std::min(question_.find(' ', i), question_.size());
            const std::string word = question_.substr(i, e - i);
            i = e + 1;
            if (word.empty())
                continue;
            const std::string tried = line.empty() ? word : line + " " + word;
            if (!line.empty() && widthOf(tried, look::kBody) > w)
            {
                out.push_back(line);
                line = word;
            }
            else
                line = tried;
        }
        if (!line.empty())
            out.push_back(line);
        if (out.size() > 8)   // a question longer than the card holds: the last line cut
        {
            out.resize(8);
            out.back() += " ...";
        }
        return out;
    }

    funkgui::Rect AskDialog::card() const noexcept
    {
        const float w = std::min(kCardW, bounds_.w - 24.0f);
        // its height for the lines (computed with the width alone: lines() reads card().w only)
        float h = 0.0f;
        {
            const float tw = w - 2.0f * kPad;
            int n = 1;
            float lineW = 0.0f;
            std::size_t i = 0;
            while (i < question_.size())
            {
                const std::size_t e = std::min(question_.find(' ', i), question_.size());
                const float ww = widthOf(question_.substr(i, e - i), look::kBody);
                const float space = lineW > 0.0f ? widthOf(" ", look::kBody) : 0.0f;
                if (lineW > 0.0f && lineW + space + ww > tw)
                {
                    ++n;
                    lineW = ww;
                }
                else
                    lineW += space + ww;
                i = e + 1;
            }
            h = kPad + 22.0f + static_cast<float>(std::min(n, 8)) * kLine + 18.0f + kButtonH + kPad;
        }
        return { std::floor(bounds_.centreX() - w * 0.5f), std::floor(bounds_.centreY() - h * 0.5f), w, h };
    }

    funkgui::Rect AskDialog::button(bool yes) const noexcept
    {
        // The safe answer at the right (where a dialog's default is), the other left of it.
        const funkgui::Rect c = card();
        const float y = c.bottom() - kPad - kButtonH;
        const float x = c.right() - kPad - kButtonW - (yes ? kButtonW + 10.0f : 0.0f);
        return { x, y, kButtonW, kButtonH };
    }

    void AskDialog::press(funkgui::Point p)
    {
        if (!open_)
            return;
        if (button(true).contains(p))
            answer(true);
        else if (button(false).contains(p))
            answer(false);
    }

    void AskDialog::pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&)
    {
        press({ e.x, e.y });
    }

    bool AskDialog::type(const funkgui::KeyEvent& e)
    {
        if (!open_)
            return false;
        switch (e.key)
        {
            case funkgui::Key::escape: answer(false); return true;
            case funkgui::Key::enter:
            case funkgui::Key::space: answer(focusYes_); return true;
            case funkgui::Key::tab:
            case funkgui::Key::left:
            case funkgui::Key::right:
                focusYes_ = !focusYes_;
                ++revision_;
                return true;
            default: return true;   // every key is the dialog's while it is open
        }
    }

    funkgui::Cursor AskDialog::cursorAt(funkgui::Point p) const
    {
        return button(true).contains(p) || button(false).contains(p) ? funkgui::Cursor::pointingHand : funkgui::Cursor::normal;
    }

    void AskDialog::draw(funkgui::Canvas& c, const funkgui::Theme& th, bool) const
    {
        if (!open_)
            return;
        const look::Palette p = look::Palette::of(th);
        const funkgui::Rect r = card();
        c.rrect(r.x + 3.0f, r.y + 4.0f, r.w, r.h, 8.0f, p.over(p.faint, 0.6f));
        c.rrect(r.x, r.y, r.w, r.h, 8.0f, p.raised, 1.5f, p.text);
        c.text("CLAUDE ASKS", r.x + kPad, r.y + kPad - 2.0f, look::kTitle, p.value);
        float y = r.y + kPad + 22.0f;
        for (const std::string& l : lines())
        {
            c.text(l.c_str(), r.x + kPad, y, look::kBody, p.text);
            y += kLine;
        }
        for (const bool yes : { true, false })
        {
            const funkgui::Rect b = button(yes);
            const bool focus = yes == focusYes_;
            c.rrect(b.x, b.y, b.w, b.h, 5.0f, p.panel, focus ? 2.0f : 1.0f, focus ? p.selected : p.edge);
            c.text(upperOf(yes ? yes_ : no_).c_str(), b.centreX(), b.y + 10.0f, look::kCaps, p.value, funkgui::Align::centre);
        }
    }

    void AskDialog::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        if (!open_)
            return;
        funkgui::A11yItem q;
        q.id = id_;
        q.role = funkgui::A11yRole::staticText;
        q.bounds = card();
        q.title = "Claude asks";
        q.value = question_;
        q.readOnly = true;
        out.push_back(q);
        for (const bool yes : { true, false })
        {
            funkgui::A11yItem b;
            b.id = id_ + (yes ? 1u : 2u);
            b.role = funkgui::A11yRole::button;
            b.bounds = button(yes);
            b.title = yes ? yes_ : no_;
            out.push_back(b);
        }
    }

    bool AskDialog::a11yAction(uint32_t id, funkgui::A11yAction a, double, funkgui::GestureController&)
    {
        if (!open_ || a != funkgui::A11yAction::press || (id != id_ + 1 && id != id_ + 2))
            return false;
        answer(id == id_ + 1);
        return true;
    }

    uint64_t AskDialog::drawKey() const { return DrawKey{}.mix(open_ ? 1 : 0).mix(revision_).mix(focusYes_ ? 1 : 0).h; }
}
