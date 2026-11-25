// ui/kit/widgets/Widget.h: what every widget of card J2 shares (ui/kit/widgets/). A widget is an Item (kit/gui/Items.h,
// J1's): owned by a Panel, which ticks it, draws it, hits it and forwards the pointer, the wheel, keys and accessibility
// to it, as FunkGui's own widgets and J1's controls are. Its owner places it (setPlace: a .gui element's cell, a board's
// layout, the application's frame).
//
// The rules of a FunkGui Panel hold: time only from tick(dt), every motion eased through funkgui::ease (so a headless
// frame after settle() is exact), draw() changes nothing, no clock or environment read. Colours are the theme's
// (funkgui::Theme: dark, one accent); a stroke is opaque (Look::over premixes a translucent one over the ground).
#pragma once

#include "kit/gui/Items.h"

#include <funkgui/canvas/Canvas.h>
#include <funkgui/core/Col.h>
#include <funkgui/core/Ease.h>
#include <funkgui/core/Geometry.h>
#include <funkgui/core/Theme.h>
#include <funkgui/core/TypeScale.h>

#include <cstdint>
#include <cstdio>
#include <string>

namespace tee::ui
{
    // The widgets' colours, from a theme: the ground, a face a little lighter, an edge, inks and the one accent.
    struct Look
    {
        funkgui::Col ground, face, faceHi, edge, text, label, dim, value, accent, accentDim, signal;
        // J2 (card A's chat): what went wrong (a message refused, a fault), apart from the one accent: a red that reads on
        // the theme's ground (a lighter one on a dark ground, a deeper one on paper).
        funkgui::Col error;

        static Look of(const funkgui::Theme& th) noexcept
        {
            Look l;
            l.ground = th.ground;
            l.face = funkgui::premix(th.ground, th.ink16, 0.55f);
            l.faceHi = funkgui::premix(th.ground, th.ink16, 1.0f);
            l.edge = th.ink16;
            l.text = th.ink70;
            l.label = th.ink52;
            l.dim = th.ink32;
            l.value = th.ink100;
            l.accent = th.accent;
            l.accentDim = th.accentDim;
            l.signal = th.signal;
            const int light = th.ground.r + th.ground.g + th.ground.b;
            l.error = light > 3 * 128 ? funkgui::Col{ 0xC4, 0x22, 0x3A, 0xFF } : funkgui::Col{ 0xF2, 0x55, 0x66, 0xFF };
            return l;
        }
        // c at alpha a over the ground, opaque (a curve's colour: funkgui::premix).
        funkgui::Col over(funkgui::Col c, float a) const noexcept { return funkgui::premix(ground, c, a); }
    };

    inline constexpr funkgui::Col kClear{ 0, 0, 0, 0 };                           // no fill (an outline's)
    inline constexpr funkgui::TextStyle kWidgetLabel = funkgui::type::kCaption;   // 10 px, spaced: LABELS
    inline constexpr funkgui::TextStyle kWidgetText{ 11.0f, 0.4f, 0.03f, false };  // a value, a row
    inline constexpr funkgui::TextStyle kWidgetSmall = funkgui::type::kMicro;      // 10 px: ticks, cells
    inline constexpr funkgui::TextStyle kWidgetValue = funkgui::type::kValueS;     // 18 px: a gauge's readout

    class Widget : public Item
    {
    public:
        explicit Widget(uint32_t id) : id_(id) {}

        uint32_t id() const noexcept { return id_; }
        const funkgui::Rect& bounds() const noexcept { return bounds_; }
        void setBounds(const funkgui::Rect& r) noexcept { bounds_ = r; }
        void setShown(bool s) noexcept { shown_ = s; }
        void setTitle(std::string t) { title_ = std::move(t); }
        // U1: its hover help (a true sentence of what it does or shows); "" (the default): none.
        void setHelp(std::string h) { help_ = std::move(h); }
        bool help(std::string& title, std::string& body, funkgui::Rect& anchor) const override
        {
            if (help_.empty() || !shown())
                return false;
            title = title_;
            body = help_;
            anchor = bounds_;
            return true;
        }
        const std::string& title() const noexcept { return title_; }

        // Item
        void setPlace(const funkgui::Rect& r, bool shown) override
        {
            bounds_ = r;
            shown_ = shown;
        }
        bool shown() const override { return shown_ && !bounds_.isEmpty(); }
        bool contains(funkgui::Point p) const override { return shown() && bounds_.contains(p); }
        funkgui::Point centre() const override { return { bounds_.centreX(), bounds_.centreY() }; }
        void tick(float dt, bool hovered, bool focused, funkgui::Point pointer) override
        {
            hover_ = funkgui::ease::hover(hover_, hovered, dt);
            focused_ = focused;
            pointer_ = pointer;
            step(dt);
        }
        bool settled() const override { return (hover_ == 0.0f || hover_ == 1.0f) && still(); }
        void pointerDown(const funkgui::PointerEvent&, funkgui::GestureController&, funkgui::HostServices&) override {}
        void specLine(char* out, std::size_t n) const override
        {
            std::snprintf(out, n, "%s %s", kind(), title_.c_str());
        }

    protected:
        virtual void step(float /*dt*/) {}               // its own eases, after the hover's
        virtual bool still() const { return true; }      // its own eases have landed
        virtual const char* kind() const = 0;

        // A focus ring around r (when the focus came by Tab).
        static void ring(funkgui::Canvas& c, const Look& l, const funkgui::Rect& r, float radius)
        {
            c.rrect(r.x - 2.0f, r.y - 2.0f, r.w + 4.0f, r.h + 4.0f, radius + 2.0f, kClear, 1.5f, l.accent);
        }

        uint32_t id_;
        funkgui::Rect bounds_{};
        bool shown_ = true;
        std::string help_;
        float hover_ = 0.0f;
        bool focused_ = false;
        funkgui::Point pointer_{};
        std::string title_;
    };
}
