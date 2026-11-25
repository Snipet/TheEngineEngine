// ui/app/KeyDialog.cpp: see KeyDialog.h.
#include "app/KeyDialog.h"

#include <algorithm>

namespace tee::ui
{
    namespace
    {
        const char* const kButtons[3] = { "SAVE", "REMOVE", "CLOSE" };

        // The clipboard's text as a key: printable ASCII, no spaces or line breaks (a key copied with its newline).
        std::string keyOf(const std::string& text)
        {
            std::string out;
            for (const char ch : text)
                if (ch > 32 && ch < 127)
                    out.push_back(ch);
            return out;
        }
    }

    KeyDialog::KeyDialog(uint32_t id, Hooks hooks) : Widget(id), hooks_(std::move(hooks))
    {
        title_ = "Anthropic API key";
        shown_ = false;
    }

    void KeyDialog::open()
    {
        forget();
        message_.clear();
        error_ = false;
        mask_ = hooks_.mask ? hooks_.mask() : std::string();
        open_ = true;
        ++revision_;
    }

    void KeyDialog::close()
    {
        if (!open_)
            return;
        forget();
        open_ = false;
        ++revision_;
        if (hooks_.closed)
            hooks_.closed();
    }

    void KeyDialog::forget()
    {
        std::fill(key_.begin(), key_.end(), '\0');   // not left in the buffer's memory
        key_.clear();
        ++revision_;
    }

    void KeyDialog::say(std::string text, bool error)
    {
        message_ = std::move(text);
        error_ = error;
        ++revision_;
    }

    void KeyDialog::save()
    {
        if (key_.empty())
        {
            say("Type or paste a key first.", true);
            return;
        }
        if (!hooks_.save)
        {
            forget();
            say("This window cannot keep a key.", true);
            return;
        }
        std::string why;
        std::string typed = key_;
        forget();                                    // taken or refused, it is not kept here
        const bool ok = hooks_.save(std::move(typed), why);
        mask_ = hooks_.mask ? hooks_.mask() : std::string();
        if (ok)
            say("Saved in the system's key store.", false);
        else
            say(why.empty() ? std::string("The key was not saved.") : why, true);
    }

    void KeyDialog::remove()
    {
        if (!canRemove() || !hooks_.remove)
            return;
        std::string why;
        const bool ok = hooks_.remove(why);
        mask_ = hooks_.mask ? hooks_.mask() : std::string();
        say(ok ? std::string("Removed from the system's key store.") : why, !ok);
    }

    // ---- where things are -------------------------------------------------------------------------------------------
    funkgui::Rect KeyDialog::card() const noexcept
    {
        const float w = std::min(kCardW, bounds_.w - 24.0f), h = kCardH;
        return { std::floor(bounds_.centreX() - w * 0.5f), std::floor(bounds_.centreY() - h * 0.5f), w, h };
    }

    funkgui::Rect KeyDialog::field() const noexcept
    {
        const funkgui::Rect c = card();
        return { c.x + 16.0f, c.y + 92.0f, c.w - 32.0f, 30.0f };
    }

    funkgui::Rect KeyDialog::button(int i) const noexcept
    {
        const funkgui::Rect c = card();
        const float bw = 84.0f, y = c.bottom() - 16.0f - 30.0f;
        // SAVE at the right, CLOSE beside it, REMOVE at the left (away from SAVE).
        if (i == 0)
            return { c.right() - 16.0f - bw, y, bw, 30.0f };
        if (i == 2)
            return { c.right() - 16.0f - 2.0f * bw - 8.0f, y, bw, 30.0f };
        return { c.x + 16.0f, y, bw, 30.0f };
    }

    // ---- input ------------------------------------------------------------------------------------------------------
    void KeyDialog::press(funkgui::Point p)
    {
        if (!open_)
            return;
        if (button(0).contains(p))
            save();
        else if (button(1).contains(p) && canRemove())
            remove();
        else if (button(2).contains(p))
            close();
    }

    void KeyDialog::pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&)
    {
        press({ e.x, e.y });
    }

    bool KeyDialog::type(const funkgui::KeyEvent& e)
    {
        if (!open_)
            return false;
        const bool command = e.mods.cmd || e.mods.ctrl;
        switch (e.key)
        {
            case funkgui::Key::enter: save(); return true;
            case funkgui::Key::escape: close(); return true;
            case funkgui::Key::backspace:
                if (command)
                    forget();
                else if (!key_.empty())
                {
                    key_.back() = '\0';
                    key_.pop_back();
                    ++revision_;
                }
                return true;
            case funkgui::Key::character:
                if (command && (e.ch == 'v' || e.ch == 'V'))
                {
                    const std::string pasted = keyOf(hooks_.clipboard ? hooks_.clipboard() : std::string());
                    key_ += pasted.substr(0, static_cast<std::size_t>(std::max(0, kMaxKey - static_cast<int>(key_.size()))));
                    if (!pasted.empty())
                        message_.clear();
                    ++revision_;
                    return true;
                }
                if (!command && e.ch > 32 && e.ch < 127 && static_cast<int>(key_.size()) < kMaxKey)
                {
                    key_.push_back(static_cast<char>(e.ch));
                    ++revision_;
                }
                return true;
            case funkgui::Key::tab: return true;   // the dialog keeps the focus
            default: return true;
        }
    }

    funkgui::Cursor KeyDialog::cursorAt(funkgui::Point p) const
    {
        for (int i = 0; i < 3; ++i)
            if (button(i).contains(p) && (i != 1 || canRemove()))
                return funkgui::Cursor::pointingHand;
        return funkgui::Cursor::normal;
    }

    // ---- drawing ----------------------------------------------------------------------------------------------------
    void KeyDialog::draw(funkgui::Canvas& c, const funkgui::Theme& th, bool) const
    {
        if (!open_)
            return;
        const look::Palette p = look::Palette::of(th);
        const Look l = Look::of(th);
        const funkgui::Rect r = card();
        // The card on a raised face with a strong edge (no translucent veil: every stroke is opaque), a shadow's band.
        c.rrect(r.x + 3.0f, r.y + 4.0f, r.w, r.h, 8.0f, p.over(p.faint, 0.6f));
        c.rrect(r.x, r.y, r.w, r.h, 8.0f, p.raised, 1.5f, p.text);
        const float x = r.x + 16.0f;
        c.text("ANTHROPIC API KEY", x, r.y + 14.0f, look::kTitle, p.value);
        c.text(("KEY  " + (mask_.empty() ? std::string("none saved") : mask_)).c_str(), x, r.y + 36.0f, look::kCaps, p.label);
        c.text("Kept in the system's key store: never shown in full, never written to a log.", x, r.y + 56.0f, look::kBody,
               p.text);
        // The field: a star a character, at most what fits, and the count.
        const funkgui::Rect f = field();
        c.rrect(f.x, f.y, f.w, f.h, 4.0f, p.ground, 1.0f, focused_ ? p.text : p.edge);
        {
        const funkgui::Canvas::ClipScope clip(c, f.reduced(1.0f));   // the field's only
        if (key_.empty())
            c.text("Paste or type the key", f.x + 8.0f, f.y + 9.0f, look::kBody, p.dim);
        else
        {
            const std::string count = std::to_string(key_.size()) + " characters";
            const float cw = c.textWidth(count.c_str(), look::kUnit);
            const float starW = c.textWidth("*", look::kNumberS);
            const std::size_t fits = static_cast<std::size_t>(std::max(1.0f, (f.w - 24.0f - cw) / std::max(1.0f, starW)));
            const std::string stars(std::min(key_.size(), fits), '*');
            c.text(stars.c_str(), f.x + 8.0f, f.y + 8.0f, look::kNumberS, p.value);
            const float cx = f.x + 8.0f + c.textWidth(stars.c_str(), look::kNumberS) + 1.0f;
            c.segment(cx, f.y + 7.0f, cx, f.bottom() - 7.0f, 1.5f, p.selected);
            c.text(count.c_str(), f.right() - 8.0f, f.y + 10.0f, look::kUnit, p.label, funkgui::Align::right);
        }
        }
        // What happened, in words (a refusal in the error's ink, with its own words: colour is not the only sign).
        if (!message_.empty())
            c.text(fitText(c, message_, look::kBody, r.w - 32.0f).c_str(), x, f.bottom() + 8.0f, look::kBody,
                   error_ ? l.error : p.ok);
        for (int i = 0; i < 3; ++i)
        {
            const funkgui::Rect b = button(i);
            const bool enabled = i != 1 || canRemove();
            const bool primary = i == 0 && !key_.empty();
            c.rrect(b.x, b.y, b.w, b.h, 5.0f, primary ? p.selectedDim : p.panel, 1.0f, primary ? p.selected : p.edge);
            c.text(kButtons[i], b.centreX(), b.y + 10.0f, look::kCaps, enabled ? p.value : p.dim, funkgui::Align::centre);
        }
    }

    // ---- accessibility ----------------------------------------------------------------------------------------------
    void KeyDialog::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        if (!open_)
            return;
        funkgui::A11yItem d;
        d.id = id_;
        d.role = funkgui::A11yRole::staticText;
        d.bounds = card();
        d.title = "Anthropic API key";
        d.value = "key " + (mask_.empty() ? std::string("none saved") : mask_) + (message_.empty() ? "" : ". " + message_);
        d.readOnly = true;
        out.push_back(d);
        funkgui::A11yItem f;
        f.id = id_ + 1;
        f.role = funkgui::a11yrole::editableText;
        f.bounds = field();
        f.title = "Key";
        f.value = key_.empty() ? std::string("empty") : std::to_string(key_.size()) + " characters";   // never the key
        out.push_back(f);
        for (int i = 0; i < 3; ++i)
        {
            funkgui::A11yItem b;
            b.id = id_ + 2 + static_cast<uint32_t>(i);
            b.role = funkgui::A11yRole::button;
            b.bounds = button(i);
            b.title = i == 0 ? "Save" : i == 1 ? "Remove" : "Close";
            b.enabled = i != 1 || canRemove();
            out.push_back(b);
        }
    }

    bool KeyDialog::a11yAction(uint32_t id, funkgui::A11yAction a, double, funkgui::GestureController&)
    {
        if (!open_ || a != funkgui::A11yAction::press || id < id_ + 2 || id > id_ + 4)
            return false;
        const int i = static_cast<int>(id - id_ - 2);
        press({ button(i).centreX(), button(i).centreY() });
        return true;
    }

    uint64_t KeyDialog::drawKey() const
    {
        return DrawKey{}.mix(open_ ? 1 : 0).mix(revision_).mix(focused_ ? 1 : 0).h;
    }
}
