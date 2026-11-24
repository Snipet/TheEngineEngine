// ui/views/fx/Fx.h: light and atmosphere for any stage (docs/ui/fx.md): a glow round something hot, flame at a tail
// on the overrun, tyre smoke, heat shimmer. Each is a fragment shader (shaders/fx/*.sc) drawn over a rect with
// Canvas::shader, behind or beside what a workspace draws with primitives. They carry no fact: take every one away
// and nothing a person reads is lost (ui-design.md section 1). So each helper draws NOTHING, and says so by returning
// false, whenever its shader is not there to draw:
//   - in a build with no GPU renderer (the tables are empty; FunkGui's registry answers none);
//   - on the software rasteriser (tests, pictures): the registry holds no program until the application has given
//     the GPU its shaders (shaders::registerAll), so a headless frame records nothing, not a placeholder;
//   - when the GPU refused the shader, or the effects are turned off (setEnabled(false)).
// No error box, no placeholder, no text: a refusal is silence.
//
// Every helper takes `phase`, a number the caller moves in its own step(dt) (the flicker, the drift, the breathing):
// no clock is read here or in the shaders. Colours are the palette's (look::Palette), passed as straight 0..1 rgb.
// The shaders read only FunkGui's u_params (16 floats), u_rect and u_viewSize (Direct3D 11 refused one that read a
// uniform of its own).
//
//   fx::glow(c, rect, heat, p.hot, phase);          // a hot header: heat 0..1
//   fx::flame(c, rect, pop, phase, fromRight);      // a pop at a tail
//   fx::smoke(c, rect, density, p.dim, phase);      // a spinning tyre
//   fx::shimmer(c, rect, heat, p.text, phase);      // above hot pipes
#pragma once

#include "app/Look.h"

#include <funkgui/canvas/Canvas.h>
#include <funkgui/core/Col.h>
#include <funkgui/core/Geometry.h>

#include <cstdint>

namespace tee::ui::fx
{
    // The shaders (shaders/fx/<name>.sc), as the build's table names them (panel "fx").
    enum class Effect : std::uint8_t { glow, flame, smoke, shimmer };
    const char* fileOf(Effect e) noexcept;          // "glow.sc"

    // Whether an effect would draw now: its binary is in this build, FunkGui holds its program, and effects are on.
    bool available(Effect e);
    // All effects on or off (a setting; on by default). Off: every helper draws nothing.
    void setEnabled(bool on) noexcept;
    bool enabled() noexcept;

    // `inner`: the share of the rect the hot part itself takes (its halo starts there), 0..0.9.
    bool glow(funkgui::Canvas& c, const funkgui::Rect& r, float heat, funkgui::Col color, float phase, float inner = 0.4f);
    bool flame(funkgui::Canvas& c, const funkgui::Rect& r, float strength, float phase, bool fromRight = false);
    bool smoke(funkgui::Canvas& c, const funkgui::Rect& r, float density, funkgui::Col color, float phase);
    bool shimmer(funkgui::Canvas& c, const funkgui::Rect& r, float heat, funkgui::Col color, float phase);

    // The same from the quantities a stage already reads (SI), each a one-line call:
    //   hotWall      a wall's temperature, K: no glow under 700 K, full at 1100 K (a header glows dull red from about
    //                800 K). Colour: the palette's hot, turning hotter above 1000 K.
    //   overrunPop   a pop's size 0..1 (the caller eases it down over about 0.15 s after the pop).
    //   tyreSmoke    the wheel's slip ratio (0 rolling, 1 locked or spinning freely): none under 0.12, full at 0.45.
    //   heatHaze     a surface's temperature, K: none under 450 K, full at 900 K.
    // `phase` in cycles (0..1 wraps), moved by the caller's step(dt): about 0.7 a second for a glow's breathing,
    // 3 for a flame's flicker, 0.4 for smoke's drift, 0.6 for shimmer.
    bool hotWall(funkgui::Canvas& c, const funkgui::Rect& r, double kelvin, const look::Palette& p, float phase,
                 float inner = 0.4f);
    bool overrunPop(funkgui::Canvas& c, const funkgui::Rect& r, double size01, float phase, bool fromRight = false);
    bool tyreSmoke(funkgui::Canvas& c, const funkgui::Rect& r, double slip, const look::Palette& p, float phase);
    bool heatHaze(funkgui::Canvas& c, const funkgui::Rect& r, double kelvin, const look::Palette& p, float phase);

    // The amounts those use, for tests and for a stage that wants to know whether anything would show.
    float hotWallAmount(double kelvin) noexcept;
    float tyreSmokeAmount(double slip) noexcept;
    float heatHazeAmount(double kelvin) noexcept;
}
