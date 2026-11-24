// ui/kit/gui/Style.h: from TestingPlayground, Source/editor/ChatStyle.h at 47bac40327ed9e8e59928500de9f3beb63fe8636 (ui/kit/SEED.tsv); what was changed: docs/ui-kit.md.
// Style.h: the colours, text sizes and line icons the controls share (a menu control's box, Dropdown.h).
// Cut down from the plugin's chat column's look to the palette, the text styles and the icons a panel draws.
#pragma once

#include <funkgui/canvas/Canvas.h>
#include <funkgui/core/Col.h>
#include <funkgui/core/Theme.h>
#include <funkgui/text/TextStyle.h>

#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

namespace tee::ui::style
{
    struct Palette
    {
        funkgui::Col card;             // the chat column's ground
        funkgui::Col cardEdge;
        funkgui::Col bubble;           // the user's messages
        funkgui::Col code;             // code blocks, inline code
        funkgui::Col codeEdge;
        funkgui::Col composer;         // the message box
        funkgui::Col composerEdge;
        funkgui::Col hover;            // a control under the pointer
        funkgui::Col raised;           // a card on the card (actions, chips)
        funkgui::Col text, soft, dim, faint;
        funkgui::Col accent, onAccent;
        funkgui::Col danger;           // errors (the product's signal colour, as the editor's)
    };

    inline Palette palette(const funkgui::Theme& th)
    {
        const bool dark = th.ground.r < 128;
        constexpr funkgui::Col white{ 255, 255, 255, 255 }, black{ 0, 0, 0, 255 };
        Palette p;
        p.card = dark ? funkgui::mix(th.ground, th.ink100, 0.035f) : funkgui::mix(th.ground, white, 0.62f);
        p.cardEdge = dark ? funkgui::mix(th.ground, th.ink100, 0.085f) : funkgui::mix(th.ground, th.ink100, 0.11f);
        p.bubble = dark ? funkgui::mix(th.ground, th.ink100, 0.115f) : funkgui::mix(th.ground, th.ink100, 0.07f);
        p.code = dark ? funkgui::mix(th.ground, black, 0.32f) : funkgui::mix(th.ground, th.ink100, 0.035f);
        p.codeEdge = dark ? funkgui::mix(th.ground, th.ink100, 0.07f) : funkgui::mix(th.ground, th.ink100, 0.09f);
        p.composer = dark ? funkgui::mix(th.ground, th.ink100, 0.07f) : white;
        p.composerEdge = dark ? funkgui::mix(th.ground, th.ink100, 0.14f) : funkgui::mix(th.ground, th.ink100, 0.16f);
        p.hover = dark ? funkgui::mix(th.ground, th.ink100, 0.12f) : funkgui::mix(th.ground, th.ink100, 0.08f);
        p.raised = dark ? funkgui::mix(th.ground, th.ink100, 0.055f) : funkgui::mix(th.ground, white, 0.85f);
        p.text = th.ink100;
        p.soft = th.ink70;
        p.dim = th.ink52;
        p.faint = th.ink32;
        p.accent = th.accent;
        p.onAccent = dark ? funkgui::mix(th.ground, black, 0.4f) : white;
        p.danger = th.signal;
        return p;
    }

    // The one typeface (JetBrains Mono) at a reading size; weight through the SDF threshold (~0.05 reads semibold).
    inline constexpr funkgui::TextStyle kBody{ 12.0f, 0.0f, 0.0f, false };
    inline constexpr funkgui::TextStyle kStrong{ 12.0f, 0.0f, 0.065f, false };
    inline constexpr funkgui::TextStyle kSmall{ 10.5f, 0.0f, 0.0f, false };
    inline constexpr funkgui::TextStyle kSmallStrong{ 10.5f, 0.0f, 0.06f, false };
    inline constexpr funkgui::TextStyle kCodeText{ 11.0f, 0.0f, 0.0f, false };
    inline constexpr funkgui::TextStyle kTitle{ 12.5f, 0.15f, 0.07f, false };
    inline constexpr funkgui::TextStyle kH1{ 14.0f, 0.0f, 0.075f, false };
    inline constexpr funkgui::TextStyle kH2{ 13.0f, 0.0f, 0.075f, false };
    inline constexpr funkgui::TextStyle kH3{ 12.0f, 0.0f, 0.075f, false };
    inline constexpr float kBodyLine = 19.0f;      // px between lines of a paragraph
    inline constexpr float kSmallLine = 15.0f;
    inline constexpr float kCodeLine = 16.0f;

    // Line icons, drawn in a box of `size` px centred on (cx, cy).
    enum class Icon : uint8_t
    {
        none, plus, chevronDown, chevronUp, chevronLeft, chevronRight, cross, stop, check, play, alert
    };

    // A convex polygon filled (its vertices in order): cut at its vertices' x into the canvas's trapezoids.
    inline void fillConvex(funkgui::Canvas& c, const float* xs, const float* ys, int n, funkgui::Col ink)
    {
        float cuts[16];
        int m = 0;
        for (int i = 0; i < n && m < 16; ++i)
        {
            bool seen = false;
            for (int k = 0; k < m; ++k)
                seen = seen || std::fabs(cuts[k] - xs[i]) < 1.0e-4f;
            if (!seen)
                cuts[m++] = xs[i];
        }
        for (int i = 1; i < m; ++i)                   // a few: by insertion
            for (int k = i; k > 0 && cuts[k - 1] > cuts[k]; --k)
                std::swap(cuts[k - 1], cuts[k]);
        for (int k = 0; k + 1 < m; ++k)
        {
            const float a = cuts[k], b = cuts[k + 1];
            float top0 = 1.0e9f, top1 = 1.0e9f, bot0 = -1.0e9f, bot1 = -1.0e9f;
            for (int i = 0; i < n; ++i)
            {
                const float x0 = xs[i], y0 = ys[i], x1 = xs[(i + 1) % n], y1 = ys[(i + 1) % n];
                if (std::fmin(x0, x1) > a + 1.0e-4f || std::fmax(x0, x1) < b - 1.0e-4f || std::fabs(x1 - x0) < 1.0e-6f)
                    continue;                         // an edge across the whole slab: its y at both ends
                const float ya = y0 + (y1 - y0) * (a - x0) / (x1 - x0), yb = y0 + (y1 - y0) * (b - x0) / (x1 - x0);
                top0 = std::fmin(top0, ya);
                top1 = std::fmin(top1, yb);
                bot0 = std::fmax(bot0, ya);
                bot1 = std::fmax(bot1, yb);
            }
            if (top0 < bot0 || top1 < bot1)
                c.area(a, b, top0, top1, bot0, bot1, ink);
        }
    }

    inline void drawIcon(funkgui::Canvas& c, Icon icon, float cx, float cy, float size, funkgui::Col ink)
    {
        const float s = size * 0.5f;                  // half the box
        const float w = std::fmax(1.2f, size * 0.1f); // the stroke
        switch (icon)
        {
            case Icon::none:
                break;
            case Icon::plus:
                c.segment(cx - s * 0.7f, cy, cx + s * 0.7f, cy, w, ink);
                c.segment(cx, cy - s * 0.7f, cx, cy + s * 0.7f, w, ink);
                break;
            case Icon::chevronDown:
            {
                const float xs[3] = { cx - s * 0.5f, cx, cx + s * 0.5f };
                const float ys[3] = { cy - s * 0.22f, cy + s * 0.28f, cy - s * 0.22f };
                c.polyline(xs, ys, 3, w, ink);
                break;
            }
            case Icon::chevronUp:
            {
                const float xs[3] = { cx - s * 0.5f, cx, cx + s * 0.5f };
                const float ys[3] = { cy + s * 0.22f, cy - s * 0.28f, cy + s * 0.22f };
                c.polyline(xs, ys, 3, w, ink);
                break;
            }
            case Icon::chevronLeft:
            {
                const float xs[3] = { cx + s * 0.22f, cx - s * 0.28f, cx + s * 0.22f };
                const float ys[3] = { cy - s * 0.5f, cy, cy + s * 0.5f };
                c.polyline(xs, ys, 3, w, ink);
                break;
            }
            case Icon::cross:
            {
                const float a = s * 0.55f;
                const float x1[2] = { cx - a, cx + a }, y1[2] = { cy - a, cy + a };
                const float x2[2] = { cx - a, cx + a }, y2[2] = { cy + a, cy - a };
                c.polyline(x1, y1, 2, w, ink);
                c.polyline(x2, y2, 2, w, ink);
                break;
            }
            case Icon::chevronRight:
            {
                const float xs[3] = { cx - s * 0.22f, cx + s * 0.28f, cx - s * 0.22f };
                const float ys[3] = { cy - s * 0.5f, cy, cy + s * 0.5f };
                c.polyline(xs, ys, 3, w, ink);
                break;
            }
            case Icon::stop:
                c.rrect(cx - s * 0.45f, cy - s * 0.45f, s * 0.9f, s * 0.9f, s * 0.18f, ink);
                break;
            case Icon::check:
            {
                const float xs[3] = { cx - s * 0.6f, cx - s * 0.15f, cx + s * 0.65f };
                const float ys[3] = { cy + s * 0.02f, cy + s * 0.48f, cy - s * 0.5f };
                c.polyline(xs, ys, 3, w * 1.2f, ink);
                break;
            }
            case Icon::play:
            {
                const float xs[4] = { cx - s * 0.4f, cx + s * 0.6f, cx - s * 0.4f, cx - s * 0.4f };
                const float ys[4] = { cy - s * 0.6f, cy, cy + s * 0.6f, cy - s * 0.6f };
                c.polyline(xs, ys, 4, w, ink);
                break;
            }
            case Icon::alert:
                c.disc(cx, cy, s * 0.78f, ink.withAlpha(0.0f), w, ink);
                c.segment(cx, cy - s * 0.42f, cx, cy + s * 0.1f, w * 1.2f, ink);
                c.disc(cx, cy + s * 0.38f, w * 0.75f, ink);
                break;
        }
    }
}
