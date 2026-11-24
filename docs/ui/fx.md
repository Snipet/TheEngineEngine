# The atmosphere shaders

*Package U5. Shaders: `shaders/fx/` (`glow.sc`, `flame.sc`, `smoke.sc`, `shimmer.sc`, listed for the build in
`fx.gui`). Helpers: `ui/views/fx/Fx.h`. Tests: `tests/ui/views/FxTests.cpp`.*

Light and atmosphere only (ui-design.md section 1): a glow round a hot header, flame at a tail on the overrun, tyre
smoke, heat shimmer, drawn behind or beside what a workspace draws with primitives. Take every shader away and no fact
is lost: each workspace still says the temperature, the pop and the slip in numbers, words and tints.

## One line each (for U3's EXHAUST and U1's DRIVE)

```cpp
#include "views/fx/Fx.h"                       // namespace tee::ui::fx
// in step(dt): phase_ = std::fmod(phase_ + dt * rate, 1.0f);   (rate below; no clock anywhere)
// in drawStage, BEFORE the primitives the effect belongs to (it is light behind them):
fx::hotWall(c, headerRect, ctx_.live.get("exhaust.portTemperature"), p, phase_);   // K
fx::overrunPop(c, tailRect, popSize01, phase_, /*fromRight*/ true);                   // 0..1, eased by you
fx::tyreSmoke(c, wheelRect, ctx_.live.get("car.wheelSlip"), p, phase_);            // slip ratio
fx::heatHaze(c, aboveRect, bonnetKelvin, p, phase_);                                  // K
```

| Helper | Its quantity (SI) | Nothing below | Full at | Phase rate | Its rect | Picture (lavapipe) |
|---|---|---|---|---|---|---|
| `hotWall` | a wall's temperature, K | 700 K | 1100 K (hotter's ink above 1000 K) | 0.7 /s | the part and a margin round it; `inner` = the part's share (0.4) | `out/ui/fx.gpu.glow.png` |
| `overrunPop` | a pop's size, 0..1 (ease it down over ~0.15 s) | 0 | 1 | 3 /s | from the pipe's end outwards, about 4 pipe widths long, 1.5 high | `out/ui/fx.gpu.flame.png` |
| `tyreSmoke` | the wheel's slip ratio (absolute) | 0.12 | 0.45 | 0.4 /s | the wheel and the room above and behind it | `out/ui/fx.gpu.smoke.png` |
| `heatHaze` | a surface's temperature, K | 450 K | 900 K | 0.6 /s | the air above the surface | `out/ui/fx.gpu.shimmer.png` |

Each returns whether it drew. The lower-level `glow`, `flame`, `smoke`, `shimmer` take the amount 0..1 and a colour
directly. Keep the fact elsewhere: the temperature in numbers and the tint, the pop in its word, the slip in its bar.

**Cost.** On the CPU: one look-up of a cached id and one recorded draw (a quad) per call, nothing on the software
rasteriser. On the GPU: one quad; per pixel, the glow is a few multiply-adds, the flame one value-noise lookup (four
hashes), the smoke three octaves (twelve hashes), the shimmer two sines; small next to the frame's own primitives. Not
measured: no GPU here, and lavapipe's times (it runs on the CPU) say nothing about one.

**Silence, never an error box.** A helper records nothing (and returns false) when its binary is not in the build,
when FunkGui's registry holds no program for it (the software rasteriser, every test, a GPU that refused it), when its
amount is 0, or when `fx::setEnabled(false)`. So a headless picture shows the primitives alone, not a placeholder
(`fx.software`), and a build or a GPU that refuses a shader draws nothing in its place.

**What the shaders may read**: only FunkGui's `u_params` (as `in0`..`in15`), `u_rect` (`u_size`) and `u_viewSize`;
no uniform of their own (Direct3D 11 refused one), no include, no font, no text (`fx.sources` holds that). Their
parameters: glow (heat, r, g, b, phase, inner), flame (strength, phase, from the right), smoke (density, phase, r, g, b),
shimmer (heat, phase, r, g, b). Colours are the palette's, so both themes read.

**Building them.** `ui/CMakeLists.txt` lists them with `tee_ui_app` (`tee_add_panel_shaders(tee_ui_app NAME fx ...)`;
it must be in that target's directory, or shaderc's commands are not attached). The application adds the table before
`shaders::registerAll`: `tee::ui::shaders::add(tee::ui::shaders::fxShaders());` (header `<tee/ui/shaders/fx.h>`). With
a GPU layer they are compiled for SPIR-V and Metal on every host, and Direct3D 11 on a Windows host.

**A back end that refuses one.** At run time a program the GPU refuses is not registered (`registerAll` lists it in
its `why`), so its helper draws nothing: the effect is absent there, the rest of the frame is whole. At build time a
shader shaderc rejects fails the build (cmake/TeeShaders.cmake), for any panel's shader: the four here use nothing but
float arithmetic, `sin`, `fract`, `floor`, `mix`, `clamp`, `smoothstep` and `step`, which every profile takes.

**Seen on**: Mesa's software Vulkan (lavapipe) under Xvfb only, if at all (the handoff says). No person has seen them
on a real GPU.
