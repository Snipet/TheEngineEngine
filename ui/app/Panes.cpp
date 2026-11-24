// ui/app/Panes.cpp: see Panes.h.
#include "app/Panes.h"

#include <algorithm>

namespace tee::ui
{
    void Panes::set(std::vector<Pane> panes)
    {
        // A pane that is no longer here loses its press and its pointer.
        for (std::size_t i = 0; i < panes_.size(); ++i)
        {
            const bool kept = std::any_of(panes.begin(), panes.end(), [&](const Pane& p) { return p.panel == panes_[i].panel; });
            if (!kept && panes_[i].panel != nullptr)
            {
                panes_[i].panel->pointerExit();
                panes_[i].panel->closeGestures();
            }
        }
        funkgui::Panel* pressed = pane(pressed_);
        funkgui::Panel* keys = pane(keys_);
        panes_ = std::move(panes);
        pressed_ = keys_ = hover_ = -1;
        for (std::size_t i = 0; i < panes_.size(); ++i)
        {
            if (panes_[i].panel == pressed && pressed != nullptr)
                pressed_ = static_cast<int>(i);
            if (panes_[i].panel == keys && keys != nullptr)
                keys_ = static_cast<int>(i);
        }
        ++revision_;
    }

    int Panes::at(float x, float y) const
    {
        for (std::size_t i = 0; i < panes_.size(); ++i)
            if (panes_[i].panel != nullptr && panes_[i].rect.contains({ x, y }))
                return static_cast<int>(i);
        return -1;
    }

    void Panes::tick(float dt)
    {
        for (const Pane& p : panes_)
            if (p.panel != nullptr)
                p.panel->tick(dt);
    }

    void Panes::draw(funkgui::Canvas& c, const funkgui::Theme& th)
    {
        for (const Pane& p : panes_)
            if (p.panel != nullptr && !p.rect.isEmpty())
            {
                const funkgui::Canvas::ClipScope clip(c, p.rect);
                p.panel->draw(c, th);
            }
    }

    bool Panes::wantsFullRate() const
    {
        return std::any_of(panes_.begin(), panes_.end(), [](const Pane& p) { return p.panel != nullptr && p.panel->wantsFullRate(); });
    }

    void Panes::pointerMove(const funkgui::PointerEvent& e)
    {
        const int now = at(e.x, e.y);
        if (hover_ >= 0 && hover_ != now)
            if (funkgui::Panel* was = pane(hover_))
                was->pointerExit();
        hover_ = now;
        if (funkgui::Panel* p = pane(now))
            p->pointerMove(e);
    }

    void Panes::pointerExit()
    {
        if (funkgui::Panel* p = pane(hover_))
            p->pointerExit();
        hover_ = -1;
    }

    void Panes::pointerDown(const funkgui::PointerEvent& e)
    {
        pressed_ = at(e.x, e.y);
        if (pressed_ >= 0)
            keys_ = pressed_;
        if (funkgui::Panel* p = pane(pressed_))
            p->pointerDown(e);
    }

    void Panes::pointerDrag(const funkgui::PointerEvent& e)
    {
        if (funkgui::Panel* p = pane(pressed_))
            p->pointerDrag(e);
    }

    void Panes::pointerUp(const funkgui::PointerEvent& e)
    {
        if (funkgui::Panel* p = pane(pressed_))
            p->pointerUp(e);
        pressed_ = -1;
    }

    void Panes::doubleClick(const funkgui::PointerEvent& e)
    {
        if (funkgui::Panel* p = pane(at(e.x, e.y)))
            p->doubleClick(e);
    }

    bool Panes::wheel(const funkgui::WheelEvent& e)
    {
        funkgui::Panel* p = pane(at(e.x, e.y));
        return p != nullptr && p->wheel(e);
    }

    bool Panes::magnify(const funkgui::MagnifyEvent& e)
    {
        funkgui::Panel* p = pane(at(e.x, e.y));
        return p != nullptr && p->magnify(e);
    }

    bool Panes::key(const funkgui::KeyEvent& e)
    {
        // Tab with no pane pressed yet: the panes in turn, from the last (the inspector's page) back, the first that
        // takes it keeps the keys.
        if (keys_ < 0 && e.key == funkgui::Key::tab)
            for (int i = static_cast<int>(panes_.size()) - 1; i >= 0; --i)
                if (funkgui::Panel* p = pane(i); p != nullptr && p->key(e))
                {
                    keys_ = i;
                    return true;
                }
        funkgui::Panel* p = pane(keys_);
        const bool used = p != nullptr && p->key(e);
        if (!used && e.key == funkgui::Key::tab)
            keys_ = -1;   // Tab went past the pane's last control: the next Tab starts again from the panes' turn
        return used;
    }

    funkgui::Cursor Panes::cursor() const
    {
        const funkgui::Panel* p = pane(pressed_ >= 0 ? pressed_ : hover_);
        return p != nullptr ? p->cursor() : funkgui::Cursor::normal;
    }

    void Panes::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        for (std::size_t i = 0; i < panes_.size(); ++i)
        {
            if (panes_[i].panel == nullptr)
                continue;
            const std::size_t from = out.size();
            panes_[i].panel->accessibility(out);
            const uint32_t tag = static_cast<uint32_t>(i + 1) << kShift;
            for (std::size_t k = from; k < out.size(); ++k)
            {
                out[k].id |= tag;
                if (out[k].parent != 0)
                    out[k].parent |= tag;
            }
        }
    }

    uint32_t Panes::a11yRevision() const
    {
        uint32_t r = revision_;
        for (const Pane& p : panes_)
            if (p.panel != nullptr)
                r = r * 31u + p.panel->a11yRevision();
        return r;
    }

    void Panes::a11yAction(uint32_t id, funkgui::A11yAction a, double value)
    {
        const int i = static_cast<int>(id >> kShift) - 1;
        if (funkgui::Panel* p = pane(i))
            p->a11yAction(id & ((1u << kShift) - 1u), a, value);
    }

    void Panes::closeGestures()
    {
        for (const Pane& p : panes_)
            if (p.panel != nullptr)
                p.panel->closeGestures();
    }
}
