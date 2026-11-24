// ui/views/engine/StageDraw.h: small drawing helpers the ENGINE and VALVES stages share (text by its anchor, a point
// on a circle by degrees from up). Canvas primitives only.
#pragma once

#include "app/Look.h"

#include <cmath>
#include <string>
#include <string_view>

namespace tee::ui::stage
{
    using funkgui::Align;
    using funkgui::Canvas;
    using funkgui::Col;
    using funkgui::Point;

    inline constexpr double kPi = 3.14159265358979323846;

    // A point `deg` clockwise from up, `r` from c.
    inline Point polar(Point c, double deg, double r)
    {
        const double a = deg * kPi / 180.0;
        return { c.x + static_cast<float>(r * std::sin(a)), c.y - static_cast<float>(r * std::cos(a)) };
    }
    // The Canvas's arc angle (radians from +x, clockwise) of `deg` clockwise from up.
    inline float arcAngle(double deg) { return static_cast<float>((deg - 90.0) * kPi / 180.0); }

    // Text placed by its anchor: left, centre or right of x; y its top.
    inline void put(Canvas& c, const std::string& s, float x, float y, const funkgui::TextStyle& st, Col ink,
                    Align a = Align::left)
    {
        float x0 = x;
        if (a != Align::left)
        {
            const float w = c.textWidth(s.c_str(), st);
            x0 = a == Align::centre ? x - w * 0.5f : x - w;
        }
        c.text(s.c_str(), x0, y, st, ink);
    }
    // A readout (look::readout) whose unit is left off when the number is the dash: "-" stands alone.
    inline float readout(Canvas& c, const look::Palette& p, float x, float y, std::string_view label, const std::string& number,
                         std::string_view unit = {}, const funkgui::TextStyle& style = look::kNumberM, Col ink = look::kClear,
                         Align align = Align::left)
    {
        return look::readout(c, p, x, y, label, number, number == "-" ? std::string_view() : unit, style,
                             number == "-" ? look::kClear : ink, align);
    }

    // Text centred on a point (its capitals' middle at cy).
    inline void putCentred(Canvas& c, const std::string& s, float x, float cy, const funkgui::TextStyle& st, Col ink)
    {
        put(c, s, x, c.capCentreTop(cy, st), st, ink, Align::centre);
    }
}
