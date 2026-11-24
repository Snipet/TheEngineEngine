// ui/app/Look.h: the look every workspace draws with (docs/plan/ui-design.md section 4): colours by meaning, the text
// and line styles, and small helpers for the things every stage draws (a readout, a framed region, a level, a pipe, a
// heat tint, a dash for a value that is not there). All of it is Canvas primitives: rounded rects, capsule segments,
// opaque polylines, stroked arcs and cubics, discs, area strips, ASCII text of 10 logical px or more, clip rects.
//
// Colour means something and is never the only sign:
//   selected  the accent (the reference's orange): the thing selected or active. Also an outline (a ring).
//   cold      cool blue: intake air, anything cold.          Also its place (left of the engine) and its label.
//   hot       orange-red: exhaust, heat.                     Also its place (right) and its label.
//   hotter    red: very hot, the redline.                    Also a zone with its own numerals.
//   ok        green: within limits.                          Also a word ("OK") or a filled lamp.
//   warning   amber: knock, the limiter, slip.               Also a word and a hatched or outlined shape.
// Each is derived for the theme's polarity: on graphite (dark ground) lighter, on paper (light ground) darker, so
// both themes read. A translucent colour is premixed over the ground (Palette::over): every stroke is opaque.
#pragma once

#include "kit/gui/Items.h"
#include "kit/widgets/Widget.h"

#include <funkgui/canvas/Canvas.h>
#include <funkgui/core/Col.h>
#include <funkgui/core/Geometry.h>
#include <funkgui/core/Theme.h>
#include <funkgui/core/TypeScale.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <string_view>

namespace tee::ui::look
{
    using funkgui::Col;

    // ---- colours ----------------------------------------------------------------------------------------------------
    struct Palette
    {
        // From the theme (FunkGui's ink ladder): the ground, a panel a little off it, a raised face, an edge, a grid.
        Col ground, panel, raised, edge, grid;
        // Ink, most contrast first: values; text; labels and units; dim (at rest, absent); faint (ghosts, tracks).
        Col value, text, label, dim, faint;
        // By meaning (above).
        Col selected, selectedDim, cold, hot, hotter, ok, warning;
        bool dark = true;

        static Palette of(const funkgui::Theme& th) noexcept
        {
            Palette p;
            const float lum = 0.2126f * static_cast<float>(th.ground.r) + 0.7152f * static_cast<float>(th.ground.g)
                            + 0.0722f * static_cast<float>(th.ground.b);
            p.dark = lum < 128.0f;
            p.ground = th.ground;
            p.panel = funkgui::premix(th.ground, th.ink16, 0.35f);
            p.raised = funkgui::premix(th.ground, th.ink16, 0.75f);
            p.edge = th.ink16;
            p.grid = funkgui::premix(th.ground, th.ink16, 0.6f);
            p.value = th.ink100;
            p.text = th.ink70;
            p.label = th.ink52;
            p.dim = th.ink32;
            p.faint = th.ink16;
            p.selected = th.accent;
            p.selectedDim = th.accentDim;
            if (p.dark)
            {
                p.cold = { 0x5A, 0xA9, 0xE6 };
                p.hot = { 0xEE, 0x6B, 0x3B };
                p.hotter = { 0xE5, 0x48, 0x4D };
                p.ok = { 0x5B, 0xC2, 0x7A };
                p.warning = { 0xF2, 0xB3, 0x3D };
            }
            else
            {
                p.cold = { 0x1F, 0x6F, 0xB2 };
                p.hot = { 0xC8, 0x51, 0x2A };
                p.hotter = { 0xB3, 0x2D, 0x2E };
                p.ok = { 0x2E, 0x8B, 0x4E };
                p.warning = { 0xA8, 0x6E, 0x05 };
            }
            return p;
        }
        // c at alpha a over the ground, opaque.
        Col over(Col c, float a) const noexcept { return funkgui::premix(ground, c, std::clamp(a, 0.0f, 1.0f)); }
        // c at alpha a over another opaque colour.
        static Col mix(Col under, Col c, float a) noexcept { return funkgui::premix(under, c, std::clamp(a, 0.0f, 1.0f)); }

        // A temperature as a tint (K): cold blue below `cool`, the text's ink at `warm`, hot through hotter above. Always
        // with a number or a label beside it (colour is never the only sign).
        Col heat(double kelvin, double cool = 300.0, double warm = 400.0, double glow = 1100.0) const noexcept
        {
            if (!std::isfinite(kelvin))
                return dim;
            if (kelvin <= cool)
                return cold;
            if (kelvin <= warm)
                return mix(cold, text, static_cast<float>((kelvin - cool) / (warm - cool)));
            const double t = std::min(1.0, (kelvin - warm) / (glow - warm));
            return t < 0.5 ? mix(text, hot, static_cast<float>(t * 2.0)) : mix(hot, hotter, static_cast<float>(t * 2.0 - 1.0));
        }
        // A pressure as a tint (Pa absolute): cold under the air outside (vacuum), the ink at it, hot above (boost).
        Col pressure(double pa, double ambient = 101325.0, double high = 250000.0) const noexcept
        {
            if (!std::isfinite(pa))
                return dim;
            if (pa < ambient)
                return mix(text, cold, static_cast<float>(std::min(1.0, (ambient - pa) / (ambient * 0.8))));
            return mix(text, hot, static_cast<float>(std::min(1.0, (pa - ambient) / (high - ambient))));
        }
    };

    // ---- type and line ----------------------------------------------------------------------------------------------
    // FunkGui's scale, as it is; nothing under 10 px.
    inline constexpr funkgui::TextStyle kNumberL = funkgui::type::kValueP;   // 24 px tabular: the big number
    inline constexpr funkgui::TextStyle kNumberM = funkgui::type::kValueS;   // 18 px tabular
    inline constexpr funkgui::TextStyle kNumberS{ 13.0f, 0.0f, 0.0f, true }; // 13 px tabular: a table's, a small readout's
    inline constexpr funkgui::TextStyle kUnit = funkgui::type::kMicro;       // 10 px: the unit beside a number
    inline constexpr funkgui::TextStyle kCaps = funkgui::type::kCaption;     // 10 px spaced capitals: LABELS
    inline constexpr funkgui::TextStyle kTitle = funkgui::type::kLatch;      // 13 px spaced: a region's title
    inline constexpr funkgui::TextStyle kBody{ 11.0f, 0.4f, 0.03f, false };  // a sentence
    inline constexpr float kHair = 1.0f;        // grids and frames (Canvas::hairlineH/V where axis-aligned)
    inline constexpr float kTrace = 1.75f;      // a curve
    inline constexpr float kNeedle = 2.0f;      // a needle, a marker
    inline constexpr float kRadius = 6.0f;      // a region's corners
    inline constexpr float kPad = 12.0f;        // inside a region

    // The gauge, plot and diagram styles (for widgets and hand-drawn stages alike).
    struct GaugeStyle
    {
        float arcWidth = 2.0f, majorTick = 9.0f, minorTick = 4.5f, needle = kNeedle, hub = 5.0f;
    };
    struct PlotStyle
    {
        float trace = kTrace, ghost = 1.25f, ghostAlpha = 0.35f, areaAlpha = 0.14f;
    };
    struct DiagramStyle
    {
        float wall = 2.0f;           // a part's outline
        float pipe = 10.0f;          // a pipe's width at 1:1
        float joint = 3.0f;          // a joint's disc
        float selectRing = 2.0f;     // the selected part's ring (in `selected`), offset 3 px out
    };
    inline constexpr GaugeStyle kGauge{};
    inline constexpr PlotStyle kPlot{};
    inline constexpr DiagramStyle kDiagram{};

    // ---- helpers ----------------------------------------------------------------------------------------------------
    // The text of a value that may be absent (not finite): the dash, else printf of `fmt`.
    inline std::string num(double v, const char* fmt = "%.0f")
    {
        if (!std::isfinite(v))
            return "-";
        char b[48];
        std::snprintf(b, sizeof b, fmt, v);
        return b;
    }

    // A framed region of a stage: a panel face, a hairline edge, its title in spaced capitals at the top left. Returns
    // the room inside it (under the title).
    inline funkgui::Rect region(funkgui::Canvas& c, const Palette& p, const funkgui::Rect& r, std::string_view title = {},
                                bool selected = false)
    {
        c.rrect(r.x, r.y, r.w, r.h, kRadius, p.panel, selected ? 1.5f : 1.0f, selected ? p.selected : p.edge);
        if (title.empty())
            return { r.x + kPad, r.y + kPad, std::max(0.0f, r.w - 2 * kPad), std::max(0.0f, r.h - 2 * kPad) };
        c.text(std::string(title).c_str(), r.x + kPad, r.y + 9.0f, kCaps, selected ? p.selected : p.label);
        return { r.x + kPad, r.y + 26.0f, std::max(0.0f, r.w - 2 * kPad), std::max(0.0f, r.h - 26.0f - kPad) };
    }

    // A readout: its label in capitals over the number, the unit small beside it, in `ink` (kClear: the value's ink, dim for
    // a dash). `align` places the block at x.
    // Returns the block's width.
    inline float readout(funkgui::Canvas& c, const Palette& p, float x, float y, std::string_view label,
                         const std::string& number, std::string_view unit = {},
                         const funkgui::TextStyle& style = kNumberM, Col ink = kClear, funkgui::Align align = funkgui::Align::left)
    {
        const Col value = ink.a == 0 ? (number == "-" ? p.dim : p.value) : ink;   // kClear: the value's ink
        const std::string u(unit);
        const float nw = c.textWidth(number.c_str(), style), uw = u.empty() ? 0.0f : c.textWidth(u.c_str(), kUnit) + 4.0f;
        const float lw = label.empty() ? 0.0f : c.textWidth(std::string(label).c_str(), kCaps);
        const float w = std::max(nw + uw, lw);
        const float x0 = align == funkgui::Align::left ? x : align == funkgui::Align::right ? x - w : x - w * 0.5f;
        if (!label.empty())
            c.text(std::string(label).c_str(), x0, y, kCaps, p.label);
        const float ny = y + (label.empty() ? 0.0f : 15.0f);
        c.text(number.c_str(), x0, ny, style, value);
        if (!u.empty())
            c.text(u.c_str(), x0 + nw + 4.0f, ny + style.px - kUnit.px - 1.0f, kUnit, p.label);
        return w;
    }

    // A level 0..1 as a bar in r (horizontal), in `ink`, over a faint track; a tick at `mark` (0..1) when it is >= 0.
    inline void level(funkgui::Canvas& c, const Palette& p, const funkgui::Rect& r, double v01, Col ink, double mark = -1.0)
    {
        c.rrect(r.x, r.y, r.w, r.h, std::min(3.0f, r.h * 0.5f), p.over(p.faint, 0.9f));
        if (std::isfinite(v01))
        {
            const float w = r.w * static_cast<float>(std::clamp(v01, 0.0, 1.0));
            if (w > 0.5f)
                c.rrect(r.x, r.y, w, r.h, std::min(3.0f, r.h * 0.5f), ink);
        }
        if (mark >= 0.0)
        {
            const float mx = r.x + r.w * static_cast<float>(std::clamp(mark, 0.0, 1.0));
            c.segment(mx, r.y - 2.0f, mx, r.bottom() + 2.0f, 1.5f, p.value);
        }
    }

    // A pipe or a wall from a to b: a capsule `width` wide in `fill`, with an edge in `edge` when its alpha is not 0.
    inline void pipe(funkgui::Canvas& c, funkgui::Point a, funkgui::Point b, float width, Col fill, Col edge = kClear)
    {
        if (edge.a != 0)
            c.segment(a.x, a.y, b.x, b.y, width + 2.0f, edge);
        c.segment(a.x, a.y, b.x, b.y, width, fill);
    }

    // The ring that marks a part as selected (its shape's bounds, 3 px out), in the accent: the sign besides colour.
    inline void selectRing(funkgui::Canvas& c, const Palette& p, const funkgui::Rect& r, float radius = kRadius)
    {
        c.rrect(r.x - 3.0f, r.y - 3.0f, r.w + 6.0f, r.h + 6.0f, radius + 3.0f, kClear, kDiagram.selectRing, p.selected);
    }

    // A lamp with a word: a disc (filled when on, a ring when off) and the word beside it. The word carries the meaning.
    inline float lamp(funkgui::Canvas& c, const Palette& p, float x, float cy, std::string_view word, bool on, Col ink)
    {
        if (on)
            c.disc(x + 5.0f, cy, 5.0f, ink);
        else
            c.disc(x + 5.0f, cy, 5.0f, kClear, 1.25f, p.dim);
        const std::string w(word);
        c.text(w.c_str(), x + 15.0f, cy - 5.0f, kCaps, on ? p.value : p.dim);
        return 15.0f + c.textWidth(w.c_str(), kCaps);
    }

    // A text that says what will be here (a placeholder's): a title and lines of plain words, centred in r.
    void placeholder(funkgui::Canvas& c, const Palette& p, const funkgui::Rect& r, std::string_view title,
                     std::string_view body);
}
