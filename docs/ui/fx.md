# The atmosphere shaders

*Package U5. Shaders: `shaders/fx/` (`glow.sc`, `flame.sc`, `smoke.sc`, `shimmer.sc`, listed for the build in
`fx.gui`). Helpers: `ui/views/fx/Fx.h`. Tests: `tests/ui/views/FxTests.cpp`.*

Light and atmosphere only (ui-design.md section 1): a glow round a hot header, flame at a tail on the overrun, tyre
smoke, heat shimmer, drawn behind or beside what a workspace draws with primitives. Take every shader away and no fact
is lost: each workspace still says the temperature, the pop and the slip in numbers, words and tints.

```cpp
#include "views/fx/Fx.h"
namespace fx = tee::ui::fx;
fx::glow(c, headerRect, heat01, p.hot, phase_);        // phase_ moved in step(dt): no clock anywhere
fx::flame(c, tailRect, pop01, phase_, /*fromRight*/ true);
fx::smoke(c, wheelRect, slip01, p.dim, phase_);
fx::shimmer(c, aboveRect, heat01, p.text, phase_);
```

**Silence, never an error box.** A helper records nothing (and returns false) when its binary is not in the build,
when FunkGui's registry holds no program for it (the software rasteriser, every test, a GPU that refused it), when its
amount is 0, or when `fx::setEnabled(false)`. So a headless picture shows the primitives alone, not a placeholder
(`fx.software`), and a build or a GPU that refuses a shader draws nothing in its place.

**What the shaders may read**: only FunkGui's `u_params` (as `in0`..`in15`), `u_rect` (`u_size`) and `u_viewSize`;
no uniform of their own (Direct3D 11 refused one), no include, no font, no text (`fx.sources` holds that). Their
parameters: glow (heat, r, g, b, phase, inner), flame (strength, phase, from the right), smoke (density, phase, r, g, b),
shimmer (heat, phase, r, g, b). Colours are the palette's, so both themes read.

**Building them.** Nothing globs `shaders/fx/` yet. The one line wanted, in `ui/CMakeLists.txt` after `tee_ui_app`:

```cmake
tee_add_panel_shaders(tee_ui_app NAME fx GUI "${TEE_UI_REPO}/shaders/fx/fx.gui"
  SHADERS "${TEE_UI_REPO}/shaders/fx/glow.sc" "${TEE_UI_REPO}/shaders/fx/flame.sc"
          "${TEE_UI_REPO}/shaders/fx/smoke.sc" "${TEE_UI_REPO}/shaders/fx/shimmer.sc")
```

and, where the application adds its tables before `shaders::registerAll`, `tee::ui::shaders::add(tee::ui::shaders::fxShaders());`
(generated header `<tee/ui/shaders/fx.h>`). The app build in CI then compiles them for SPIR-V and Metal on every
host and for Direct3D 11 on Windows. What was run here is in `docs/handoffs/u5-garage-scope.md`.

**Seen on**: Mesa's software Vulkan (lavapipe) under Xvfb only, if at all (the handoff says). No person has seen them
on a real GPU.
