# U5: GARAGE, SCOPE and the atmosphere shaders (branch `claude/awesome-gates-ntdb58`)

**Done, by test** (`tee-view-tests`; pictures: FunkGui's software rasteriser, `out/ui/`, looked at before blessing):
- SCOPE (`ui/views/scope/`, registered): `scope.stage`(+`.dpi1`), `scope.large`(+`.dpi1`), `scope.paper`, `scope.campbell`,
  `scope.logpv`, `scope.absent` (goldens); `scope.fft`, `scope.freeze`, `scope.rest`. Own 4096-point Hann FFT, Campbell
  cells, triggered wave, P-theta with valve events/spark/peak, P-V lin and log-log, ports. Inspector: `sound` page.
- GARAGE (`ui/views/garage/`, NOT registered, see below): `garage.stage`(+`.dpi1`), `garage.large`(+`.dpi1`), `garage.paper`,
  `garage.compare`(+`.dpi1`) (goldens); `garage.search`, `garage.parts`, `garage.files`, `garage.rest`. Library seam
  `Library.h`; `DemoLibrary` (labelled DEMO LIBRARY); `ServicesLibrary` over today's `FrameServices`.
- FX (`shaders/fx/*.sc`, `ui/views/fx/Fx.h`): `fx.software` (helpers record nothing on the rasteriser), `fx.sources`.
  Compiled here by bgfx's shaderc (built in a `TEE_BUILD_APP=ON` tree) for SPIR-V and Metal through `tee_add_panel_shaders`;
  Direct3D 11 needs a Windows host (CI's, once the CMake line below is in). Drawn once on the GPU path: a scratch harness
  (not committed) on FunkGui's DesktopHost, Vulkan on Mesa's lavapipe under `scripts/xvfb-run.sh`, 90 frames, all four
  registered and drawn: `out/ui/fx.gpu.lavapipe.png` (looked at). No person has seen them on a real GPU.
- Frame stages: `scope.framestage`, `garage.framestage` (AppFrame::stageRect at 1100x700 and 1600x1000; goldens
  `scope.frame.*`, `garage.frame.*`; no glyph outside the stage, across a panel edge or over another glyph).
- Pictures: `out/ui/scope.*.png`, `garage.*.png`, `fx.software.png`, contact sheet `out/ui/sheet.u5.png`. Docs: `docs/ui/scope.md`,
  `garage.md`, `fx.md`.

**Read, all on the branch now** (14:10): telemetry `engine.speed`, `engine.redline`, `engine.crank`, `ecu.advance`,
`sound.rate`; Scope `degrees`, `pressure`, `volume`, `liftIntake`, `liftExhaust`, `portIntake`, `portExhaust`;
`outputTail`. Absent at run time they draw empty plots with words and dashes (`scope.absent`). Fields:
`engine.layout`, `engine.cylinders`, `engine.displacement`/`bore`/`stroke`, `boost.kind`, `fuel.kind`, any field by FieldInfo.

**Wanted**
1. U1: register GARAGE: `ui/views/garage/register.h` = `TEE_UI_WORKSPACE(garage, makeGarageWorkspace)`; it
   replaces the placeholder that `ui.frame.garage` drives (that test must change). `frame.scope` in `ui.frame.sheet`
   needs U1's re-bless (it now shows the real SCOPE).
2. Shell (card B): a `garage::Library` over `tee::files::BuildLibrary` + `BuildFile` (they map one to one; add the
   part, the values by path, the dyno runner's curve cached by `Build::hash()`).
3. Done by others since: the four names above; `WorkspaceContext::library` (GARAGE uses it); the shell's
   `app/GarageLibrary.h`.
4. CMake, `ui/CMakeLists.txt`: the `tee_add_panel_shaders(tee_ui_app NAME fx ...)` line in `docs/ui/fx.md`, and
   `shaders::add(fxShaders())` where the app adds its tables. It must be in `ui/` (the target's directory): from
   another directory the shaderc commands are not attached (found here).

**Not done / not run**: no workspace calls the fx helpers yet (U3's EXHAUST, U1's DRIVE may). Nobody has seen these
stages on a real display or GPU; macOS and Windows only by CI, which had not reached these commits when this was written.
Baseline: `ui.app.sheet` failed before my work. **Measured**: my 19 view tests pass, 14 s here.
