# Handoff: card J, slice J1 (the interface kit) — branch `claude/awesome-gates-ntdb58`

**Done** (29 tests, `ctest -R '^ui\.'`, pass standalone `cmake -S ui -B build-ui` and in the root's `dev` preset, Linux):
- Items 1, 2: TestingPlayground's `.gui` code at `47bac40` in `ui/kit/gui`, `ui/kit/shaders` (36 rows in `ui/kit/SEED.tsv`;
  `ui.seed`), namespace `tee::ui`, plugin ties cut (table in `docs/ui-kit.md`); `docs/gui-language.md`.
- Bound to the shell's real header: `ui.ports`, `ui.gui.parse`, `ui.gui.notes`, `ui.display.telemetry`.
- Item 5, every control kind and look by scripted input: `ui.knob.drag/.wheel/.keys/.reset`, `ui.slider.drag`,
  `ui.switch.click`, `ui.buttons.click`, `ui.menu.pick`, `ui.xy.drag`, `ui.pointer.area`, `ui.tabs.switch`,
  `ui.resize.layout`, `ui.scroll.wheel`, `ui.keyboard.hidden`, `ui.a11y`; goldens `ui.kinds.golden/.small/.large`.
- Item 3: `cmake/TeeShaders.cmake`, `tee-shadergen`, `tee::ui::shaders::registerAll` (`ui.shader.text`,
  `ui.shader.placeholder`). The reference's `header.sc`, `cluster.sc`, `layout.sc` **compiled** for Metal and SPIR-V here
  through `tee_add_panel_shaders`, with a shaderc I built from bgfx.cmake at FunkGui's pin (99752df).
- Item 4: `RefPanel` + strip + `makeMainPanel`, target `tee_ui` (`ui.ref.panel`, `ui.ref.strip`, `ui.ref.fallback`):
  `sim/ref/panel.gui` read with no notes, all 32 dials worked, at 840x764 (natural), 640x400 and 1920x1200.

**Not done, and why**
- No shader was ever *drawn*: no GPU path ran. The 120-frame Xvfb run waits for FunkGui's desktop host (pin is still
  v0.27.0, core only). Direct3D 11 was not compiled (needs a Windows host). Shaders are not built in CI yet: the
  root has no shaderc until `TEE_BUILD_APP`.
- macOS and Windows: `tee_ui_kit`, `tee_ui_ref` compiled warning-free in CI run 45389000657, but the UI tests did
  not run there (the build stopped at `tee-tests`, below). The goldens are proven on Linux only.
- `scope`/`spectrum out` draw a flat line (`TelemetrySource` has no samples). `needsDraw()` is always true.
- `shaders/ref/` is unused (the sources stay in `sim/ref/`).
**Commits**: 673a9a1..b9580ed (mine: the `ui:`, `build: add ui/`, `docs: the .gui language` ones), plus this file.
**Sizes**: `ui/` 13,400 lines of C++, most of it the seed; the 29 tests take 7 s.
**Wanted outside my paths**
1. Keel: `tee-tests` fails to link on macOS and Windows (Catch2 `StringMaker<std::string_view>::convert` undefined;
   run 45389000657). Not mine, but it hides my tests there.
2. Shell: the RefPlayer adapter must answer `findValue` for the reference's raw fields `showRpm showRed showGear
   showKmh showThr showMap showLamps showPedals showCycle showAir showPipes showBoost` (the three shaders read them).
3. Shell: link `tee_ui`; call `tee::ui::shaders::registerAll(why)` after FunkGui's GPU is up; guard any
   `add_subdirectory(ui)` with `if(NOT TARGET tee_ui)` (the root adds it; I added that one block to the root).
4. Shell or lead: a way to read the output's last 2048 samples on the UI thread, for the reference's `spectrum out`.
5. Lead/card A: when `FunkGui::desktop` exists, `ui/CMakeLists.txt` turns shaders on by itself (`TEE_UI_GPU`);
   someone with that build should run `ui.ref.panel` there (it then checks the three programs are registered).

**FunkGui issues met**: its JUCE-free `writePng` returns false (worked around: `tests/ui/Png.h`).
**For a person to look at** (software rasteriser; shaders are placeholders): run `tee-ui-tests all`, then
`out/ui/ref.page1..3.png`, `ref.small.png`, `ref.large.png`, `kinds.knobs/.switches/.displays/.frames.png`.
On a machine with a GPU, once the app builds: the header, the dashboard and the engine layout shaders, never yet seen.
