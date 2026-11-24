# U5: GARAGE, SCOPE, LISTEN and the atmosphere shaders (branch `claude/awesome-gates-ntdb58`)

**Done, by test** (`tee-view-tests`; pictures: FunkGui's software rasteriser, `out/ui/`, looked at; unchecked ones in `*.pictures`, nightly):
- SCOPE (`ui/views/scope/`, registered): `scope.stage`(+`.dpi1`), `scope.large`(+`.dpi1`), `scope.paper`, `scope.campbell`,
  `scope.logpv`, `scope.absent` (goldens); `scope.fft`, `scope.freeze`, `scope.rest`. Own 4096-point Hann FFT, Campbell
  cells, triggered wave, P-theta with valve events/spark/peak, P-V lin and log-log, ports. Inspector: the frame's.
- GARAGE (`ui/views/garage/`, NOT registered, see below): `garage.stage`(+`.dpi1`), `garage.large`(+`.dpi1`), `garage.paper`,
  `garage.compare`(+`.dpi1`) (goldens); `garage.search`, `garage.parts`, `garage.files`, `garage.rest`. Library seam
  `Library.h`; `DemoLibrary` (labelled DEMO LIBRARY); `ServicesLibrary` over today's `FrameServices`.
- FX (`shaders/fx/*.sc`, `ui/views/fx/Fx.h`): `fx.software` (helpers record nothing on the rasteriser), `fx.sources`.
  Compiled for SPIR-V and Metal here and for Direct3D 11 by Windows CI (run 45420506181: `fx/*.sc -> s_5_0`, no refusal),
  listed in `ui/CMakeLists.txt`; one-line SI helpers (`hotWall`, `overrunPop`, `tyreSmoke`, `heatHaze`). Drawn once: a scratch harness
  (not committed) on FunkGui's DesktopHost, Vulkan on Mesa's lavapipe under `scripts/xvfb-run.sh`, 90 frames, all four
  registered and drawn: `out/ui/fx.gpu.lavapipe.png` (looked at). No person has seen them on a real GPU.
- Frame stages: `scope.framestage`, `garage.framestage` (AppFrame::stageRect at 1100x700 and 1600x1000; goldens
  `scope.frame.*`, `garage.frame.*`; no glyph outside the stage, across a panel edge or over another glyph).
- LISTEN (`ui/views/listen/`, registered; `docs/ui/listen.md`): `listen.stage`(+`.dpi1`, `.paper`), `.framestage`, `.cameras`,
  `.drag`, `.reference`, `.absent`, `.rest`. Asked of L: Spec `listenerX/Y`, `facing`; tel `srcX/srcY/srcLevel[10]`,
  `listenerX/Y`, `facing`, `rearAxleY`, `path*`; until then the model's documented places, said in words.
- Pictures: `out/ui/scope.*.png`, `listen.*.png`, `garage.*.png`, `fx.software.png`, contact sheet `out/ui/sheet.u5.png`. Docs: `docs/ui/scope.md`,
  `garage.md`, `fx.md`.

**Read, all on the branch now** (14:10): telemetry `engine.speed`, `engine.redline`, `engine.crank`, `ecu.advance`,
`sound.rate`; Scope `degrees`, `pressure`, `volume`, `liftIntake`, `liftExhaust`, `portIntake`, `portExhaust`;
`outputTail`. Absent at run time they draw empty plots with words and dashes (`scope.absent`). Fields:
`engine.layout`, `engine.cylinders`, `engine.displacement`/`bore`/`stroke`, `boost.kind`, `fuel.kind`, any field by FieldInfo.

**Wanted**
1. U1: LISTEN's slot after DRIVE, and the re-bless of `ui.frame.garage`, `.ribbon`, `.sheet` it moves; "listen" in U5's
   group of ViewKit's demo hooks. (GARAGE is registered.)
2. Shell (card B): a `garage::Library` over `tee::files::BuildLibrary` + `BuildFile` (they map one to one; add the
   part, the values by path, the dyno runner's curve cached by `Build::hash()`).
3. Done by others since: the four names above; `WorkspaceContext::library` (GARAGE uses it); the shell's
   `app/GarageLibrary.h`.
4. CMake, `ui/CMakeLists.txt`: the `tee_add_panel_shaders(tee_ui_app NAME fx ...)` line in `docs/ui/fx.md`, and
   `shaders::add(fxShaders())` where the app adds its tables. It must be in `ui/` (the target's directory): from
   another directory the shaderc commands are not attached (found here).

**Not done / not run**: no workspace calls the fx helpers yet (U3, U1 asked). Nobody has seen any of it on a real display
or GPU; macOS and Windows by CI only. **Measured**: my view tests all pass here.
