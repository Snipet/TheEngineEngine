// ui/kit/widgets/Basic.cpp: see Basic.h.
#include "kit/widgets/Basic.h"

#include <funkgui/widgets/Scroll.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <utility>

namespace tee::ui
{
    namespace
    {
        using funkgui::Align;
        using funkgui::Canvas;
        using funkgui::Col;
        using funkgui::Rect;

        // Text centred on a line at y (its caps' middle).
        void textAt(Canvas& c, const std::string& s, float x, float cy, const funkgui::TextStyle& st, Col col,
                    Align a = Align::left)
        {
            c.text(s.c_str(), x, c.capCentreTop(cy, st), st, col, a);
        }

        // A value 0..1 with no text of its own.
        std::string formatNumber01(float v)
        {
            char buf[32];
            std::snprintf(buf, sizeof buf, "%.2f", static_cast<double>(v));
            return buf;
        }

        // A widget's accessibility ids are its own and the next 63 (a list's rows): Board's and GuiPanel's stride.
        constexpr int kIds = 63;

        funkgui::A11yItem a11yBase(uint32_t id, funkgui::A11yRole role, const Rect& r, const std::string& title)
        {
            funkgui::A11yItem it;
            it.id = id;
            it.role = role;
            it.bounds = r;
            it.title = title;
            return it;
        }
    }

    // ---- Button ------------------------------------------------------------------------------------------------------
    Button::Button(uint32_t id, std::string label, std::function<void()> onPress) : Widget(id), onPress_(std::move(onPress))
    {
        title_ = std::move(label);
    }

    void Button::fire()
    {
        if (!enabled())
            return;
        ++presses_;
        if (onPress_)
            onPress_();
    }

    void Button::step(float dt)
    {
        down_ = funkgui::ease::toward(down_, held_ ? 1.0f : 0.0f, dt, 0.05f);
        litShown_ = funkgui::ease::toward(litShown_, isLit() ? 1.0f : 0.0f, dt, 0.08f);
    }

    void Button::draw(Canvas& c, const funkgui::Theme& th, bool focusRing) const
    {
        const Look l = Look::of(th);
        const Rect& r = bounds_;
        const float rad = std::min(6.0f, r.h * 0.5f);
        const bool on = enabled();
        const Col face = on ? funkgui::mix(funkgui::mix(l.face, l.faceHi, hover_), l.accentDim, std::max(down_, litShown_)) : l.ground;
        const Col edge = on ? funkgui::mix(l.edge, l.accent, std::max(down_ * 0.6f, litShown_)) : l.edge;
        c.rrect(r.x, r.y, r.w, r.h, rad, face, 1.0f, edge);
        // U1: a title wider than the button is cut with an ellipsis (a tab's), never drawn over its neighbours.
        textAt(c, fitText(c, upper(title_), kWidgetLabel, std::max(10.0f, r.w - leftRoom_ - 10.0f)), r.centreX() + leftRoom_ * 0.5f,
               r.centreY(), kWidgetLabel,
               on ? funkgui::mix(l.text, l.value, std::max(hover_, litShown_)) : l.dim, Align::centre);
        if (focusRing)
            ring(c, l, r, rad);
    }

    void Button::pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&)
    {
        if (e.popup || !enabled())
            return;
        held_ = inside_ = true;
        if (onHold_)
            onHold_(true);
    }

    void Button::pointerDrag(const funkgui::PointerEvent& e, funkgui::GestureController&)
    {
        inside_ = bounds_.contains({ e.x, e.y });
    }

    void Button::pointerUp(const funkgui::PointerEvent& e, funkgui::GestureController&)
    {
        if (!held_)
            return;
        held_ = false;
        if (onHold_)
            onHold_(false);
        if (inside_ && bounds_.contains({ e.x, e.y }))
            fire();
    }

    bool Button::key(const funkgui::KeyEvent& e, funkgui::GestureController&)
    {
        if (e.key == funkgui::Key::enter || e.key == funkgui::Key::space)
        {
            fire();
            return true;
        }
        return false;
    }

    void Button::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem it = a11yBase(id_, funkgui::A11yRole::button, bounds_, title_);
        it.enabled = enabled();
        if (lit_)
        {
            it.checkable = true;
            it.checked = isLit();
        }
        out.push_back(std::move(it));
    }

    bool Button::a11yAction(uint32_t id, funkgui::A11yAction a, double, funkgui::GestureController&)
    {
        if (id != id_ || a != funkgui::A11yAction::press)
            return false;
        fire();
        return true;
    }

    uint64_t Button::drawKey() const { return DrawKey{}.mix(isLit()).mix(title_).mix(enabled()).h; }

    // ---- Toggle ------------------------------------------------------------------------------------------------------
    Toggle::Toggle(uint32_t id, funkgui::ParamPort& port, std::string label) : Widget(id), port_(port)
    {
        title_ = std::move(label);
        on_ = target();
    }

    void Toggle::step(float dt) { on_ = funkgui::ease::toward(on_, target(), dt, 0.07f); }

    void Toggle::draw(Canvas& c, const funkgui::Theme& th, bool focusRing) const
    {
        const Look l = Look::of(th);
        const Rect& r = bounds_;
        const float th_ = std::min(18.0f, r.h - 4.0f), tw = th_ * 1.9f;
        const Rect track{ r.right() - tw - 2.0f, r.centreY() - th_ * 0.5f, tw, th_ };
        textAt(c, upper(title_), r.x, r.centreY(), kWidgetLabel, funkgui::mix(l.label, l.text, hover_));
        c.rrect(track.x, track.y, track.w, track.h, th_ * 0.5f, funkgui::mix(l.face, l.accentDim, on_), 1.0f,
                funkgui::mix(l.edge, l.accent, on_));
        const float knob = th_ - 6.0f;
        const float kx = track.x + 3.0f + knob * 0.5f + (track.w - knob - 6.0f) * on_;
        c.disc(kx, track.centreY(), knob * 0.5f, funkgui::mix(l.text, l.value, std::max(on_, hover_)));
        if (focusRing)
            ring(c, l, track, th_ * 0.5f);
    }

    void Toggle::pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController& g, funkgui::HostServices&)
    {
        if (!e.popup)
            flip(g);
    }

    bool Toggle::key(const funkgui::KeyEvent& e, funkgui::GestureController& g)
    {
        if (e.key != funkgui::Key::enter && e.key != funkgui::Key::space)
            return false;
        flip(g);
        return true;
    }

    void Toggle::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem it = a11yBase(id_, funkgui::A11yRole::toggleButton, bounds_, title_);
        it.checkable = true;
        it.checked = target() > 0.5f;
        out.push_back(std::move(it));
    }

    bool Toggle::a11yAction(uint32_t id, funkgui::A11yAction a, double, funkgui::GestureController& g)
    {
        if (id != id_ || (a != funkgui::A11yAction::toggle && a != funkgui::A11yAction::press))
            return false;
        flip(g);
        return true;
    }

    uint64_t Toggle::drawKey() const { return DrawKey{}.mix(port_.value01()).h; }

    // ---- Stepper -----------------------------------------------------------------------------------------------------
    Stepper::Stepper(uint32_t id, funkgui::ParamPort& port, std::string label, std::function<std::string(float)> text,
                     float step01)
        : Widget(id), port_(port), text_(std::move(text)), step01_(step01)
    {
        title_ = std::move(label);
    }

    Rect Stepper::minusRect() const noexcept
    {
        const float s = std::min(24.0f, bounds_.h);
        return { bounds_.right() - 3.0f * s - 60.0f, bounds_.centreY() - s * 0.5f, s, s };
    }

    Rect Stepper::plusRect() const noexcept
    {
        const float s = std::min(24.0f, bounds_.h);
        return { bounds_.right() - s, bounds_.centreY() - s * 0.5f, s, s };
    }

    float Stepper::stepOf(bool fine) const noexcept
    {
        const int n = port_.numSteps();
        if (n > 0)
            return 1.0f / static_cast<float>(n);
        return fine ? step01_ * 0.1f : step01_;
    }

    float Stepper::moved(int dir, bool fine) const noexcept
    {
        const float s = stepOf(fine);
        const float v = port_.value01() + static_cast<float>(dir) * s;
        return port_.numSteps() > 0 ? std::round(v / s) * s : v;
    }

    funkgui::Cursor Stepper::cursorAt(funkgui::Point p) const
    {
        return minusRect().contains(p) || plusRect().contains(p) ? funkgui::Cursor::pointingHand : funkgui::Cursor::normal;
    }

    void Stepper::draw(Canvas& c, const funkgui::Theme& th, bool focusRing) const
    {
        const Look l = Look::of(th);
        textAt(c, upper(title_), bounds_.x, bounds_.centreY(), kWidgetLabel, funkgui::mix(l.label, l.text, hover_));
        const Rect m = minusRect(), p = plusRect();
        const Rect value{ m.right(), m.y, p.x - m.right(), m.h };
        c.rrect(m.x, m.y, p.right() - m.x, m.h, 5.0f, l.face, 1.0f, l.edge);
        for (const Rect* b : { &m, &p })
        {
            const bool over = b->contains(pointer_);
            c.rrect(b->x, b->y, b->w, b->h, 5.0f, over ? l.faceHi : l.face, 1.0f, l.edge);
            const float cx = b->centreX(), cy = b->centreY(), a = b->w * 0.22f;
            const Col ink = over ? l.accent : l.text;
            c.segment(cx - a, cy, cx + a, cy, 1.5f, ink);
            if (b == &p)
                c.segment(cx, cy - a, cx, cy + a, 1.5f, ink);
        }
        const float v = port_.value01();
        const std::string s = text_ ? text_(v) : formatNumber01(v);
        textAt(c, s, value.centreX(), value.centreY(), kWidgetText, l.value, Align::centre);
        if (focusRing)
            ring(c, l, { m.x, m.y, p.right() - m.x, m.h }, 5.0f);
    }

    void Stepper::pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController& g, funkgui::HostServices&)
    {
        if (e.popup)
            return;
        if (minusRect().contains({ e.x, e.y }))
            g.tap(port_, moved(-1, e.mods.shift));
        else if (plusRect().contains({ e.x, e.y }))
            g.tap(port_, moved(1, e.mods.shift));
    }

    void Stepper::doubleClick(const funkgui::PointerEvent& e, funkgui::GestureController& g)
    {
        const Rect m = minusRect(), p = plusRect();
        if (Rect{ m.right(), m.y, p.x - m.right(), m.h }.contains({ e.x, e.y }))
            g.tap(port_, port_.default01());             // the value itself: back to the default
        else
            pointerDown(e, g, g.host());                 // a fast second click on a sign is a second step
    }

    bool Stepper::wheel(const funkgui::WheelEvent& e, funkgui::GestureController& g, double now)
    {
        if (e.dy == 0.0f)
            return false;
        g.wheelTo(port_, moved(e.dy > 0.0f ? 1 : -1, e.mods.shift), now);
        return true;
    }

    bool Stepper::key(const funkgui::KeyEvent& e, funkgui::GestureController& g)
    {
        switch (e.key)
        {
            case funkgui::Key::up: case funkgui::Key::right: g.tap(port_, moved(1, e.mods.shift)); return true;
            case funkgui::Key::down: case funkgui::Key::left: g.tap(port_, moved(-1, e.mods.shift)); return true;
            case funkgui::Key::home: g.tap(port_, 0.0f); return true;
            case funkgui::Key::end: g.tap(port_, 1.0f); return true;
            default: return false;
        }
    }

    void Stepper::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem it = a11yBase(id_, funkgui::A11yRole::slider, bounds_, title_);
        const int n = port_.numSteps();
        const float v = port_.value01();
        it.value = text_ ? text_(v) : formatNumber01(v);
        if (n > 0)
        {
            it.v = std::round(v * static_cast<float>(n));
            it.hi = n;
            it.step = 1;
        }
        else
        {
            it.v = v;
            it.step = step01_;
        }
        out.push_back(std::move(it));
    }

    bool Stepper::a11yAction(uint32_t id, funkgui::A11yAction a, double v, funkgui::GestureController& g)
    {
        if (id != id_)
            return false;
        switch (a)
        {
            case funkgui::A11yAction::increment: g.tap(port_, moved(1, false)); return true;
            case funkgui::A11yAction::decrement: g.tap(port_, moved(-1, false)); return true;
            case funkgui::A11yAction::setValue:
            {
                const int n = port_.numSteps();
                g.tap(port_, n > 0 ? static_cast<float>(v) / static_cast<float>(n) : static_cast<float>(v));
                return true;
            }
            default: return false;
        }
    }

    uint64_t Stepper::drawKey() const { return DrawKey{}.mix(port_.value01()).h; }

    // ---- TextField ---------------------------------------------------------------------------------------------------
    TextField::TextField(uint32_t id, std::string label, std::function<std::string()> value,
                         std::function<bool(const std::string&)> onCommit, int maxLength)
        : Widget(id), value_(std::move(value)), onCommit_(std::move(onCommit)), maxLength_(maxLength)
    {
        title_ = std::move(label);
    }

    void TextField::beginEdit()
    {
        if (options_.enterOnly && draft_)
        {
            editing_ = true;                             // the draft taken up again, where it was
            return;
        }
        edit_.set(value_ ? value_() : std::string(), maxLength_);
        editing_ = true;
        refused_ = false;
    }

    void TextField::leave()
    {
        draft_ = !edit_.buffer.empty() && edit_.buffer != (value_ ? value_() : std::string());
        edit_.allSelected = false;
        editing_ = false;
    }

    bool TextField::commit()
    {
        if (!editing_)
            return true;
        if (onCommit_ && !onCommit_(edit_.buffer))
        {
            refused_ = true;
            return false;
        }
        refused_ = false;
        if (options_.enterOnly)
        {
            // taken: the field empties back to value() and stays open for the next
            edit_.set(value_ ? value_() : std::string(), maxLength_);
            edit_.allSelected = false;
            draft_ = false;
            return true;
        }
        editing_ = false;
        return true;
    }

    void TextField::draw(Canvas& c, const funkgui::Theme& th, bool focusRing) const
    {
        const Look l = Look::of(th);
        const bool labelled = !title_.empty();
        const float lw = labelled ? std::max(70.0f, c.textWidth(upper(title_).c_str(), kWidgetLabel) + 12.0f) : 0.0f;
        if (labelled)
            textAt(c, upper(title_), bounds_.x, bounds_.centreY(), kWidgetLabel, funkgui::mix(l.label, l.text, hover_));
        const float h = options_.multiline ? bounds_.h : std::min(24.0f, bounds_.h);
        const Rect box{ bounds_.x + lw, bounds_.centreY() - h * 0.5f, bounds_.w - lw, h };
        const Col bad = options_.enterOnly ? l.error : l.accent;
        const Col edge = refused_ ? bad : (editing_ ? l.text : funkgui::mix(l.edge, l.dim, hover_));
        c.rrect(box.x, box.y, box.w, box.h, 4.0f, editing_ ? l.ground : l.face, 1.0f, edge);
        const float tx = box.x + 7.0f;
        const Canvas::ClipScope clip(c, box.reduced(1.0f));
        if (options_.multiline || (options_.enterOnly && draft_))
        {
            // Lines (multiline) or a kept draft: the text as it is, the caret's line in view.
            const std::string& s = editing_ || draft_ ? edit_.buffer : (value_ ? value_() : std::string());
            std::vector<std::string> lines;
            std::size_t at = 0;
            int caretLine = 0;
            float caretX = tx;
            const int caret = std::clamp<int>(edit_.caret, 0, static_cast<int>(s.size()));
            while (true)
            {
                const std::size_t nl = std::min(s.find('\n', at), s.size());
                if (caret >= static_cast<int>(at) && caret <= static_cast<int>(nl))
                {
                    caretLine = static_cast<int>(lines.size());
                    caretX = tx + c.textWidth(s.substr(at, static_cast<std::size_t>(caret) - at).c_str(), kWidgetText);
                }
                lines.push_back(s.substr(at, nl - at));
                if (nl >= s.size())
                    break;
                at = nl + 1;
            }
            constexpr float kLine = 16.0f;
            const int fits = std::max(1, static_cast<int>((box.h - 6.0f) / kLine));
            const int first = std::max(0, caretLine - fits + 1);
            const float top = options_.multiline ? box.y + 4.0f : box.centreY() - kLine * 0.5f;
            for (int i = first; i < static_cast<int>(lines.size()) && i < first + fits; ++i)
                c.text(lines[static_cast<std::size_t>(i)].c_str(), tx, top + kLine * static_cast<float>(i - first) + 2.0f, kWidgetText,
                       editing_ ? l.value : l.text);
            if (editing_)
            {
                const float y = top + kLine * static_cast<float>(caretLine - first);
                c.segment(caretX, y + 1.0f, caretX, y + kLine - 1.0f, 1.5f, l.accent);
            }
        }
        else if (!editing_)
        {
            textAt(c, value_ ? value_() : std::string(), tx, box.centreY(), kWidgetText, l.value);
        }
        else
        {
            const std::string& s = edit_.buffer;
            const float w = c.textWidth(s.c_str(), kWidgetText);
            if (edit_.allSelected && !s.empty())
                c.rrect(tx - 2.0f, box.y + 4.0f, w + 4.0f, box.h - 8.0f, 2.0f, l.accentDim);
            textAt(c, s, tx, box.centreY(), kWidgetText, l.value);
            const std::string before = s.substr(0, static_cast<std::size_t>(std::clamp<int>(edit_.caret, 0, static_cast<int>(s.size()))));
            const float cx = tx + c.textWidth(before.c_str(), kWidgetText);
            c.segment(cx, box.y + 5.0f, cx, box.bottom() - 5.0f, 1.5f, l.accent);
        }
        if (focusRing && !editing_)
            ring(c, l, box, 4.0f);
    }

    void TextField::pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&)
    {
        if (!e.popup && !editing_)
            beginEdit();
    }

    bool TextField::key(const funkgui::KeyEvent& e, funkgui::GestureController&)
    {
        if (!editing_)
        {
            if (e.key == funkgui::Key::enter || e.key == funkgui::Key::space)
            {
                beginEdit();
                return true;
            }
            return false;
        }
        if (e.key == funkgui::Key::enter && options_.multiline && e.mods.shift)
        {
            // a new line where the caret is (over the selection, when it is all selected)
            if (edit_.allSelected)
            {
                edit_.buffer.clear();
                edit_.caret = 0;
                edit_.allSelected = false;
            }
            if (static_cast<int>(edit_.buffer.size()) < maxLength_)
            {
                const auto at = static_cast<std::size_t>(std::clamp<int>(edit_.caret, 0, static_cast<int>(edit_.buffer.size())));
                edit_.buffer.insert(at, 1, '\n');
                edit_.caret = static_cast<int>(at) + 1;
            }
            refused_ = false;
            return true;
        }
        if (e.key == funkgui::Key::enter)
        {
            commit();
            return true;
        }
        if (e.key == funkgui::Key::escape)
        {
            if (options_.enterOnly)
                leave();                                 // the draft kept
            else
                editing_ = refused_ = false;
            return true;
        }
        if (e.key == funkgui::Key::tab)
        {
            if (options_.enterOnly)
            {
                leave();
                return false;                            // Tab goes on; the draft stays
            }
            return !commit();                            // Tab leaves a field that took its text
        }
        refused_ = false;
        edit_.key(e, maxLength_);
        return true;
    }

    void TextField::focusChanged(bool focused)
    {
        if (!focused && editing_ && options_.enterOnly)
        {
            leave();                                     // only Enter commits: the text is kept as the draft
            return;
        }
        if (!focused && editing_ && !commit())
            editing_ = refused_ = false;                 // left with text it would not take: given up
    }

    void TextField::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem it = a11yBase(id_, funkgui::a11yrole::editableText, bounds_, title_);
        it.value = editing_ || draft_ ? edit_.buffer : (value_ ? value_() : std::string());
        out.push_back(std::move(it));
    }

    bool TextField::a11yAction(uint32_t id, funkgui::A11yAction a, double, funkgui::GestureController&)
    {
        if (id != id_ || (a != funkgui::A11yAction::press && a != funkgui::A11yAction::focus))
            return false;
        beginEdit();
        return true;
    }

    uint64_t TextField::drawKey() const
    {
        return DrawKey{}.mix(value_ ? value_() : std::string()).mix(edit_.buffer).mix(edit_.caret).mix(editing_).mix(draft_)
            .mix(refused_).h;
    }

    // ---- List --------------------------------------------------------------------------------------------------------
    List::List(uint32_t id, std::string label, std::vector<std::string> rows, std::function<void(int)> onSelect)
        : Widget(id), rows_(std::move(rows)), onSelect_(std::move(onSelect))
    {
        title_ = std::move(label);
    }

    void List::setRows(std::vector<std::string> rows)
    {
        rows_ = std::move(rows);
        if (selected_ >= static_cast<int>(rows_.size()))
            selected_ = -1;
        scrollTo(scroll_);
    }

    Rect List::rowsArea() const noexcept
    {
        const float top = title_.empty() ? 0.0f : 18.0f;
        return { bounds_.x, bounds_.y + top, bounds_.w, std::max(0.0f, bounds_.h - top) };
    }

    void List::scrollTo(float s)
    {
        const float most = std::max(0.0f, static_cast<float>(rows_.size()) * kRowH - rowsArea().h);
        scroll_ = std::clamp(s, 0.0f, most);
    }

    void List::keepInView()
    {
        if (selected_ < 0)
            return;
        const float top = static_cast<float>(selected_) * kRowH, h = rowsArea().h;
        if (top < scroll_)
            scrollTo(top);
        else if (top + kRowH > scroll_ + h)
            scrollTo(top + kRowH - h);
    }

    void List::select(int row, bool notify)
    {
        if (rows_.empty())
            return;
        row = std::clamp(row, 0, static_cast<int>(rows_.size()) - 1);
        const bool moved = row != selected_;
        selected_ = row;
        keepInView();
        if (moved && notify && onSelect_)
            onSelect_(row);
    }

    void List::setPlace(const Rect& r, bool shown)
    {
        Widget::setPlace(r, shown);
        scrollTo(scroll_);
    }

    void List::step(float dt) { shownScroll_ = funkgui::ease::toward(shownScroll_, scroll_, dt, 0.06f, 0.25f); }

    void List::draw(Canvas& c, const funkgui::Theme& th, bool focusRing) const
    {
        const Look l = Look::of(th);
        if (!title_.empty())
            c.text(upper(title_).c_str(), bounds_.x, bounds_.y + 2.0f, kWidgetLabel, l.label);
        const Rect a = rowsArea();
        c.rrect(a.x, a.y, a.w, a.h, 5.0f, l.face, 1.0f, l.edge);
        {
            const Canvas::ClipScope clip(c, { c.snapX(a.x + 1.0f), c.snapY(a.y + 1.0f), a.w - 2.0f, a.h - 2.0f });
            const int first = std::max(0, static_cast<int>(shownScroll_ / kRowH));
            for (int i = first; i < static_cast<int>(rows_.size()); ++i)
            {
                const float y = a.y + static_cast<float>(i) * kRowH - shownScroll_;
                if (y > a.bottom())
                    break;
                const Rect row{ a.x + 1.0f, y, a.w - 2.0f, kRowH };
                const bool sel = i == selected_, over = row.contains(pointer_) && hover_ > 0.0f;
                if (sel)
                    c.rrect(row.x + 2.0f, row.y + 1.0f, row.w - 4.0f, row.h - 2.0f, 3.0f, l.accentDim);
                else if (over)
                    c.rrect(row.x + 2.0f, row.y + 1.0f, row.w - 4.0f, row.h - 2.0f, 3.0f, l.faceHi);
                textAt(c, rows_[static_cast<std::size_t>(i)], row.x + 9.0f, row.centreY(), kWidgetText, sel ? l.value : l.text);
            }
        }
        const float total = static_cast<float>(rows_.size()) * kRowH;
        if (total > a.h && a.h > 0.0f)                  // a thumb that says where in the rows the view is
        {
            const float th_ = std::max(16.0f, a.h * a.h / total);
            const float ty = a.y + (a.h - th_) * (shownScroll_ / (total - a.h));
            c.rrect(a.right() - 5.0f, ty + 2.0f, 3.0f, th_ - 4.0f, 1.5f, l.dim);
        }
        if (focusRing)
            ring(c, l, a, 5.0f);
    }

    void List::pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&)
    {
        const Rect a = rowsArea();
        if (e.popup || !a.contains({ e.x, e.y }))
            return;
        const int row = static_cast<int>((e.y - a.y + scroll_) / kRowH);
        if (row >= 0 && row < static_cast<int>(rows_.size()))
            select(row);
    }

    bool List::wheel(const funkgui::WheelEvent& e, funkgui::GestureController&, double)
    {
        const float before = scroll_;
        scrollTo(scroll_ - funkgui::scroll::wheelPx(e.dy, e.smooth, kRowH));   // as everything scrolls (Scroll.h)
        return scroll_ != before;
    }

    bool List::key(const funkgui::KeyEvent& e, funkgui::GestureController&)
    {
        const int page = std::max(1, static_cast<int>(rowsArea().h / kRowH) - 1);
        switch (e.key)
        {
            case funkgui::Key::up: select(selected_ < 0 ? 0 : selected_ - 1); return true;
            case funkgui::Key::down: select(selected_ + 1); return true;
            case funkgui::Key::pageUp: select(selected_ - page); return true;
            case funkgui::Key::pageDown: select(selected_ + page); return true;
            case funkgui::Key::home: select(0); return true;
            case funkgui::Key::end: select(static_cast<int>(rows_.size()) - 1); return true;
            default: return false;
        }
    }

    void List::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        out.push_back(a11yBase(id_, funkgui::a11yrole::list, rowsArea(), title_));
        const Rect a = rowsArea();
        for (int i = 0; i < static_cast<int>(rows_.size()) && i < kIds; ++i)
        {
            const float y = a.y + static_cast<float>(i) * kRowH - scroll_;
            if (y + kRowH <= a.y || y >= a.bottom())
                continue;                                // the rows in view
            funkgui::A11yItem it = a11yBase(id_ + 1 + static_cast<uint32_t>(i), funkgui::A11yRole::listItem,
                                            { a.x, y, a.w, kRowH }, rows_[static_cast<std::size_t>(i)]);
            it.parent = id_;
            it.checkable = true;
            it.checked = i == selected_;
            out.push_back(std::move(it));
        }
    }

    bool List::a11yAction(uint32_t id, funkgui::A11yAction a, double, funkgui::GestureController&)
    {
        if (id <= id_ || id > id_ + static_cast<uint32_t>(rows_.size()) || a != funkgui::A11yAction::press)
            return false;
        select(static_cast<int>(id - id_ - 1));
        return true;
    }

    uint64_t List::drawKey() const
    {
        DrawKey k;
        for (const std::string& r : rows_)
            k.mix(r);
        return k.mix(selected_).h;
    }

    // ---- Bar ---------------------------------------------------------------------------------------------------------
    Bar::Bar(uint32_t id, std::string label, std::function<float()> value01, std::function<std::string()> text, bool vertical)
        : Widget(id), value_(std::move(value01)), text_(std::move(text)), vertical_(vertical)
    {
        title_ = std::move(label);
    }

    void Bar::step(float dt)
    {
        target_ = std::clamp(value_ ? value_() : 0.0f, 0.0f, 1.0f);
        shown01_ = funkgui::ease::toward(shown01_, target_, dt, 0.05f, 1e-4f);
    }

    void Bar::draw(Canvas& c, const funkgui::Theme& th, bool) const
    {
        const Look l = Look::of(th);
        const std::string label = upper(title_), text = text_ ? text_() : std::string();
        if (vertical_)
        {
            const float labelH = 16.0f;
            const Rect t{ bounds_.centreX() - 7.0f, bounds_.y + (text.empty() ? 0.0f : labelH), 14.0f,
                          std::max(0.0f, bounds_.h - labelH - (text.empty() ? 0.0f : labelH)) };
            if (!text.empty())
                textAt(c, text, bounds_.centreX(), bounds_.y + 7.0f, kWidgetSmall, l.value, Align::centre);
            c.rrect(t.x, t.y, t.w, t.h, 3.0f, l.face, 1.0f, l.edge);
            const float h = (t.h - 4.0f) * shown01_;
            if (h > 0.5f)
                c.rrect(t.x + 2.0f, t.bottom() - 2.0f - h, t.w - 4.0f, h, 2.0f, l.accent);
            textAt(c, label, bounds_.centreX(), bounds_.bottom() - 6.0f, kWidgetLabel, l.label, Align::centre);
            return;
        }
        c.text(label.c_str(), bounds_.x, bounds_.y + 1.0f, kWidgetLabel, l.label);
        if (!text.empty())
            c.text(text.c_str(), bounds_.right(), bounds_.y + 1.0f, kWidgetSmall, l.value, Align::right);
        const Rect t{ bounds_.x, bounds_.y + 17.0f, bounds_.w, std::min(12.0f, std::max(4.0f, bounds_.h - 19.0f)) };
        c.rrect(t.x, t.y, t.w, t.h, 3.0f, l.face, 1.0f, l.edge);
        const float w = (t.w - 4.0f) * shown01_;
        if (w > 0.5f)
            c.rrect(t.x + 2.0f, t.y + 2.0f, w, t.h - 4.0f, 2.0f, l.accent);
    }

    void Bar::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem it = a11yBase(id_, funkgui::A11yRole::progressBar, bounds_, title_);
        it.v = target_;
        it.readOnly = true;
        it.value = text_ ? text_() : std::string();
        out.push_back(std::move(it));
    }

    uint64_t Bar::drawKey() const { return DrawKey{}.mix(value_ ? value_() : 0.0f).mix(text_ ? text_() : std::string()).h; }

    // ---- Lamp --------------------------------------------------------------------------------------------------------
    Lamp::Lamp(uint32_t id, std::string label, std::function<bool()> on, Col color)
        : Widget(id), on_(std::move(on)), color_(color)
    {
        title_ = std::move(label);
    }

    void Lamp::step(float dt)
    {
        lit_ = on_ && on_();
        glow_ = funkgui::ease::toward(glow_, lit_ ? 1.0f : 0.0f, dt, lit_ ? 0.03f : 0.12f);
    }

    void Lamp::draw(Canvas& c, const funkgui::Theme& th, bool) const
    {
        const Look l = Look::of(th);
        const Col col = color_.a == 0 ? l.accent : color_;
        const float r = std::min(7.0f, bounds_.h * 0.3f);
        const float cx = bounds_.x + r + 2.0f, cy = bounds_.centreY();
        if (glow_ > 0.0f)
            c.disc(cx, cy, r + 3.0f * glow_, l.over(col, 0.25f * glow_));
        c.disc(cx, cy, r, funkgui::mix(l.face, col, glow_), 1.0f, funkgui::mix(l.edge, col, glow_));
        textAt(c, upper(title_), cx + r + 7.0f, cy, kWidgetLabel, funkgui::mix(l.label, l.value, glow_));
    }

    void Lamp::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem it = a11yBase(id_, funkgui::A11yRole::staticText, bounds_, title_);
        it.value = lit_ ? "on" : "off";
        it.readOnly = true;
        out.push_back(std::move(it));
    }

    uint64_t Lamp::drawKey() const { return DrawKey{}.mix(on_ && on_()).h; }

    // ---- Label -------------------------------------------------------------------------------------------------------
    Label::Label(uint32_t id, std::function<std::string()> text, funkgui::TextStyle style, Ink ink, Align align,
                 std::string caption)
        : Widget(id), source_(std::move(text)), style_(style), ink_(ink), align_(align), caption_(std::move(caption))
    {
        text_ = source_ ? source_() : std::string();
        title_ = caption_;
    }

    void Label::step(float) { text_ = source_ ? source_() : std::string(); }

    void Label::draw(Canvas& c, const funkgui::Theme& th, bool) const
    {
        const Look l = Look::of(th);
        const Col ink = ink_ == Ink::value ? l.value : ink_ == Ink::text ? l.text : ink_ == Ink::label ? l.label : l.accent;
        const float x = align_ == Align::left ? bounds_.x : align_ == Align::right ? bounds_.right() : bounds_.centreX();
        float cy = bounds_.centreY();
        if (!caption_.empty())
        {
            c.text(upper(caption_).c_str(), x, bounds_.y + 1.0f, kWidgetLabel, l.label, align_);
            cy = bounds_.y + 14.0f + (bounds_.h - 14.0f) * 0.5f;
        }
        const std::string s = fitText(c, text_, style_, bounds_.w);
        c.text(s.c_str(), x, c.capCentreTop(cy, style_), style_, ink, align_);
    }

    void Label::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem it = a11yBase(id_, funkgui::A11yRole::staticText, bounds_, caption_.empty() ? text_ : caption_);
        it.value = text_;
        it.readOnly = true;
        out.push_back(std::move(it));
    }

    uint64_t Label::drawKey() const { return DrawKey{}.mix(source_ ? source_() : std::string()).h; }
}
