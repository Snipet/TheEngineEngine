// ui/ref/Strip.cpp: see Strip.h.
#include "ref/Strip.h"

#include "kit/gui/GuiLayout.h"
#include "kit/gui/Looks.h"

#include <funkgui/core/Col.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace tee::ui
{
    namespace
    {
        constexpr funkgui::TextStyle kButtonText{ 10.0f, 2.0f, 0.06f, false };
        constexpr funkgui::TextStyle kGearText{ 18.0f, 0.0f, 0.04f, true };
        constexpr float kRadius = 4.0f;

        funkgui::Rect inset(const funkgui::Rect& r, float d) { return { r.x + d, r.y + d, r.w - 2.0f * d, r.h - 2.0f * d }; }
    }

    // ---- StripButton -------------------------------------------------------------------------------------------------
    const char* StripButton::label() const noexcept
    {
        switch (what_)
        {
            case What::start:     return "START";
            case What::shiftDown: return "-";
            case What::shiftUp:   return "+";
        }
        return "";
    }

    void StripButton::press()
    {
        switch (what_)
        {
            case What::start:     model_.out.startHeld = true; break;
            case What::shiftDown: ++model_.out.shiftDown; break;
            case What::shiftUp:   ++model_.out.shiftUp; break;
        }
        model_.push();
    }

    void StripButton::release()
    {
        if (what_ == What::start && model_.out.startHeld)
        {
            model_.out.startHeld = false;                // the key springs back
            model_.push();
        }
    }

    void StripButton::tick(float dt, bool hovered, bool, funkgui::Point)
    {
        hovered_ = hovered;
        if (keyHeld_ > 0.0f)
        {
            keyHeld_ -= std::max(dt, 0.0f);
            if (keyHeld_ <= 0.0f && !down_)
                release();
        }
    }

    void StripButton::draw(funkgui::Canvas& c, const funkgui::Theme& th, bool ring) const
    {
        if (!shown_)
            return;
        const bool held = down_ || keyHeld_ > 0.0f;
        const bool lit = what_ == What::start && model_.running;
        const funkgui::Col face = held ? funkgui::premix(th.ground, th.accent, 0.55f)
                                : hovered_ ? funkgui::premix(th.ground, th.ink100, 0.14f)
                                           : funkgui::premix(th.ground, th.ink100, 0.07f);
        const funkgui::Col edge = lit ? th.accent : funkgui::premix(th.ground, th.ink100, 0.24f);
        c.rrect(rect_.x, rect_.y, rect_.w, rect_.h, kRadius, face, 1.0f, edge);
        const funkgui::Col ink = held || lit ? th.ink100 : th.ink70;
        c.text(label(), rect_.centreX(), c.capCentreTop(rect_.centreY(), kButtonText), kButtonText, ink, funkgui::Align::centre);
        if (ring)
            looks::focusRing(c, th, rect_);
    }

    void StripButton::pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&)
    {
        if (e.popup)
            return;
        down_ = true;
        press();
    }

    void StripButton::pointerUp(const funkgui::PointerEvent&, funkgui::GestureController&)
    {
        if (!std::exchange(down_, false))
            return;
        if (keyHeld_ <= 0.0f)
            release();
    }

    bool StripButton::key(const funkgui::KeyEvent& e, funkgui::GestureController&)
    {
        if (e.key != funkgui::Key::space && e.key != funkgui::Key::enter)
            return false;
        // A key's repeat keeps the starter held; the other buttons act once a press.
        if (what_ != What::start || !model_.out.startHeld)
            press();
        if (what_ == What::start)
            keyHeld_ = kKeyHold;
        return true;
    }

    void StripButton::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem it;
        it.id = id_;
        it.role = funkgui::A11yRole::button;
        it.bounds = rect_;
        switch (what_)
        {
            case What::start:     it.title = "Start"; it.help = "Hold to run the starter; with the engine running, hold to switch off"; break;
            case What::shiftDown: it.title = "Shift down"; break;
            case What::shiftUp:   it.title = "Shift up"; break;
        }
        out.push_back(std::move(it));
    }

    bool StripButton::a11yAction(uint32_t id, funkgui::A11yAction a, double, funkgui::GestureController&)
    {
        if (id != id_ || a != funkgui::A11yAction::press)
            return false;
        press();
        if (what_ == What::start)
            keyHeld_ = kKeyHold;
        return true;
    }

    void StripButton::specLine(char* out, std::size_t n) const
    {
        std::snprintf(out, n, "%s%s", label(), what_ == What::start && model_.out.startHeld ? "   HELD" : "");
    }

    // ---- StripThrottle -----------------------------------------------------------------------------------------------
    funkgui::Rect StripThrottle::track() const noexcept
    {
        return { rect_.x, rect_.y + kGuiLabelH, rect_.w, std::max(8.0f, rect_.h - kGuiLabelH) };
    }

    float StripThrottle::at(float x) const noexcept
    {
        const funkgui::Rect t = inset(track(), 3.0f);
        return t.w > 0.0f ? std::clamp((x - t.x) / t.w, 0.0f, 1.0f) : 0.0f;
    }

    void StripThrottle::setRest(float rest)
    {
        model_.rest = std::clamp(rest, 0.0f, 1.0f);
        model_.push();
    }

    void StripThrottle::draw(funkgui::Canvas& c, const funkgui::Theme& th, bool ring) const
    {
        if (!shown_)
            return;
        const funkgui::Rect t = track();
        char value[16];
        std::snprintf(value, sizeof value, "%d %%", static_cast<int>(std::lround(model_.pedal * 100.0f)));
        c.text("THROTTLE", rect_.x, rect_.y + 2.0f, kGuiControlLabel, hovered_ || model_.dragging ? th.ink70 : th.ink52);
        c.text(value, rect_.right(), rect_.y + 2.0f, kGuiControlLabel, th.ink70, funkgui::Align::right);
        c.rrect(t.x, t.y, t.w, t.h, kRadius, funkgui::premix(th.ground, th.ink100, 0.06f), 1.0f,
                funkgui::premix(th.ground, th.ink100, 0.22f));
        const funkgui::Rect in = inset(t, 3.0f);
        const float w = in.w * std::clamp(model_.pedal, 0.0f, 1.0f);
        if (w > 0.5f)
            c.rrect(in.x, in.y, w, in.h, 2.0f, model_.dragging ? th.accent : funkgui::premix(th.ground, th.accent, 0.75f));
        if (model_.rest > 0.0f)                           // where it comes back to
            c.hairlineV(in.x + in.w * model_.rest, in.y, in.h, th.ink100);
        if (ring)
            looks::focusRing(c, th, rect_);
    }

    void StripThrottle::pointerDown(const funkgui::PointerEvent& e, funkgui::GestureController&, funkgui::HostServices&)
    {
        if (e.popup)
            return;
        model_.dragging = true;
        model_.drag = at(e.x);
        model_.push();
    }

    void StripThrottle::pointerDrag(const funkgui::PointerEvent& e, funkgui::GestureController&)
    {
        if (!model_.dragging)
            return;
        model_.drag = at(e.x);
        model_.push();
    }

    void StripThrottle::pointerUp(const funkgui::PointerEvent&, funkgui::GestureController&)
    {
        if (!std::exchange(model_.dragging, false))
            return;
        model_.push();                                   // the pedal comes back up, to its rest
    }

    void StripThrottle::doubleClick(const funkgui::PointerEvent&, funkgui::GestureController&) { setRest(0.0f); }

    bool StripThrottle::wheel(const funkgui::WheelEvent& e, funkgui::GestureController&, double)
    {
        if (e.dy == 0.0f)
            return false;
        setRest(model_.rest + (e.smooth ? e.dy * 0.5f : (e.dy > 0.0f ? kStep : -kStep)));
        return true;
    }

    bool StripThrottle::key(const funkgui::KeyEvent& e, funkgui::GestureController&)
    {
        const float step = e.mods.shift ? 0.25f : kStep;
        switch (e.key)
        {
            case funkgui::Key::right:
            case funkgui::Key::up:    setRest(model_.rest + step); return true;
            case funkgui::Key::left:
            case funkgui::Key::down:  setRest(model_.rest - step); return true;
            case funkgui::Key::home:  setRest(0.0f); return true;
            case funkgui::Key::end:   setRest(1.0f); return true;
            default:                  return false;
        }
    }

    void StripThrottle::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem it;
        it.id = id_;
        it.role = funkgui::A11yRole::slider;
        it.bounds = rect_;
        it.title = "Throttle";
        char value[16];
        std::snprintf(value, sizeof value, "%d %%", static_cast<int>(std::lround(model_.pedal * 100.0f)));
        it.value = value;
        it.v = model_.pedal;
        it.step = kStep;
        out.push_back(std::move(it));
    }

    bool StripThrottle::a11yAction(uint32_t id, funkgui::A11yAction a, double v, funkgui::GestureController&)
    {
        if (id != id_)
            return false;
        switch (a)
        {
            case funkgui::A11yAction::setValue:  setRest(static_cast<float>(v)); return true;
            case funkgui::A11yAction::increment: setRest(model_.rest + kStep); return true;
            case funkgui::A11yAction::decrement: setRest(model_.rest - kStep); return true;
            default:                             return false;
        }
    }

    void StripThrottle::specLine(char* out, std::size_t n) const
    {
        std::snprintf(out, n, "THROTTLE   %d %%   REST %d %%", static_cast<int>(std::lround(model_.pedal * 100.0f)),
                      static_cast<int>(std::lround(model_.rest * 100.0f)));
    }

    // ---- StripGear ---------------------------------------------------------------------------------------------------
    std::string StripGear::text() const
    {
        if (!model_.gearKnown)
            return "-";
        return model_.gear < 0 ? std::string("R") : model_.gear == 0 ? std::string("N") : std::to_string(model_.gear);
    }

    void StripGear::draw(funkgui::Canvas& c, const funkgui::Theme& th, bool) const
    {
        if (!shown_)
            return;
        c.text("GEAR", rect_.centreX(), rect_.y + 2.0f, kGuiControlLabel, th.ink52, funkgui::Align::centre);
        const float mid = rect_.y + kGuiLabelH + (rect_.h - kGuiLabelH) * 0.5f;
        c.text(text().c_str(), rect_.centreX(), c.capCentreTop(mid, kGearText), kGearText, th.ink100, funkgui::Align::centre);
    }

    void StripGear::accessibility(std::vector<funkgui::A11yItem>& out) const
    {
        funkgui::A11yItem it;
        it.id = id_;
        it.role = funkgui::A11yRole::staticText;
        it.bounds = rect_;
        it.title = "Gear";
        it.value = text();
        it.readOnly = true;
        out.push_back(std::move(it));
    }

    void StripGear::specLine(char* out, std::size_t n) const { std::snprintf(out, n, "GEAR   %s", text().c_str()); }
}
