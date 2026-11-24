# Handoff: card J, slice J2 (widgets, units, binding, frame) — branch `claude/awesome-gates-ntdb58`

**Done** (`ctest -R '^ui\.'`: 53 in `cmake -S ui -B build-ui`, 56 in the root `dev` preset; whole root suite 152/152, Linux):
- Item 6, widgets (`ui/kit/widgets/`): `ui.button.click`, `ui.toggle.click`, `ui.stepper.click`, `ui.textfield.type`,
  `ui.list.select`, `ui.instruments` (Bar, Lamp, Gauge), `ui.plot.zoom`, `ui.curve.drag`, `ui.map.edit`; the gallery
  `ui.widgets.gallery` (goldens `widgets.controls/.instruments/.tables`). `Board`: a Panel of placed items.
- Item 7, units (`ui/kit/bind/Units.*`): `ui.units` (17 quantities, C/F offsets, typed units, settings lines).
- Item 8, binding (`ui/kit/bind/`): `ui.fieldport`, `ui.fieldparams`, `ui.page.generated` (every field kind of a
  made-up Spec: `tests/ui/MockSpec.h`), `ui.gui.auto`, `ui.gui.widgets`, `ui.page.split`. `.gui` gains dotted Spec
  paths, `auto "section"`, `plot gauge map curve bar lamp` (`docs/gui-language.md`, "What J2 added").
- `BuildDoc` adapts card C's real `tee::Build` (`tee_ui_build`, root only); on the real `sim/` (31 fields, 9 sections):
  `ui.build.frame` (every non-hidden field on a page, no notes), `ui.build.set` (a drag sets it, one undo step),
  `ui.build.files` (saved and loaded by the garage: `Build::diff` empty).
- Item 9, the frame (`ui/app/AppFrame.*`, target `tee_ui_app`): `ui.app.frame`, `ui.app.set` (knob, menu, map; Cmd-Z),
  `ui.app.start`, `ui.app.garage` (list, load, rename, save, units), `ui.app.resize` (900x560 to 1920x1200).
- Item 10: goldens `app.<screen>` at 1280x800; contact sheets **`out/ui/sheet.png`** and **`sheet.dpi1.png`** (density
  1 too, as asked; text >= 10 px): 9 screens at 1024x640 and 1600x1000. Software rasteriser; blessed after looking.
- Lead's extras: `outputTail` wired in `GuiPanel` itself (no `Options::scope`: the source's tail), so makeMainPanel's
  OUT spectrum and every page read it (`ui.display.tail`). macOS: see below.

**Not done, and why**
- The frame is not what the application shows: `makeMainPanel` is still `RefPanel`. Waits on the shell (below).
- No table field in the real Spec yet: map/curve tested through `MemoryDoc` only; table axis ranges guessed by
  quantity (`axisRange`), as the Spec has no attribute for them.
- Units are not saved anywhere (`Units::save/load` give settings lines; no settings file yet).
- Wave-2 views not wired (Dyno/Scope plot `engine.speed`). No GPU picture of J2's; macOS/Windows: CI only.

**macOS arm64 goldens (run 45390597717)**: geometry hash only. `tee_ui_test_funkgui` compiled FunkGui's INTERFACE
sources without `-ffp-contract=off`; Apple clang on arm64 (FMA) fuses there. Flag added (7591856). CI: awaiting a run that is not cancelled.
**Changes to J1 files (additive; J1's 29 tests and goldens unchanged)**: `Item::setPlace`; `GuiKind::widget` and
`GuiNode::widget`; dotted names in the lexer and `telemetryFields`; `GuiPanel::Options::{widget, autoPage, inset*}`,
`GuiPanel::widgets()`; `GestureSink` in `Ports.h`; `kMaxDialLabel` 14 to 18, `kMaxOptionLabel` 10 to 16.

**Wanted outside my paths**
0. Card B: `app/` compiles FunkGui too (tee-headless, the app) without `-ffp-contract=off`: its pictures differ on arm64.
1. Shell (card B): when it plays `SimHost`, make a `tee::Build`, `tee::ui::BuildDoc`, `tee::ui::Units` (loaded from a
   settings file via `Units::load`) and show `AppFrame` with `buildServices(build, content/builds,
   Presets::userDirectory(), appServices)`; link `tee_ui_build`; call `build.flush()` each frame.
2. Card C / K3: an attribute for a table axis's range (e.g. `ui::axisRange(lo, hi, lo, hi)`), or ranges on x and y.
**FunkGui**: no bug met. **Look at**: `out/ui/sheet*.png`, `out/ui/app.*.png`, `out/ui/widgets.*.png` (`tee-ui-tests all`).
