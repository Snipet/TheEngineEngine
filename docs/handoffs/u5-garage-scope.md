# U5: GARAGE, SCOPE and the atmosphere shaders (branch `claude/awesome-gates-ntdb58`)

**Done, by test** (`tee-view-tests`; pictures: FunkGui's software rasteriser, `out/ui/`, looked at before blessing):
- SCOPE (`ui/views/scope/`, registered): `scope.stage`(+`.dpi1`), `scope.large`(+`.dpi1`), `scope.paper`, `scope.campbell`,
  `scope.logpv`, `scope.absent` (goldens); `scope.fft`, `scope.freeze`, `scope.rest`. Own 4096-point Hann FFT, Campbell
  cells, triggered wave, P-theta with valve events/spark/peak, P-V lin and log-log, ports. Inspector: `sound` page.
- GARAGE (`ui/views/garage/`, NOT registered, see below): `garage.stage`(+`.dpi1`), `garage.large`(+`.dpi1`), `garage.paper`,
  `garage.compare`(+`.dpi1`) (goldens); `garage.search`, `garage.parts`, `garage.files`, `garage.rest`. Library seam
  `Library.h`; `DemoLibrary` (labelled DEMO LIBRARY); `ServicesLibrary` over today's `FrameServices`.
- FX (`shaders/fx/*.sc`, `ui/views/fx/Fx.h`): `fx.software` (helpers record nothing on the rasteriser), `fx.sources`.
- Pictures: `out/ui/scope.*.png`, `garage.*.png`, `fx.software.png`, contact sheet `out/ui/sheet.u5.png`. Docs: `docs/ui/scope.md`,
  `garage.md`, `fx.md`.

**Read, and whether on the branch**: telemetry `engine.speed`, `engine.redline`, `engine.crank` (yes); `ecu.advance`,
`sound.rate` (no: spark unmarked, 48 kHz assumed). Scope `degrees`, `pressure`, `volume`, `liftIntake`, `liftExhaust` (yes);
`portIntake`, `portExhaust` (no: PORTS says so; `ScopeDemo.h` makes them up for tests). `outputTail` (yes). Fields:
`engine.layout`, `engine.cylinders`, `engine.displacement`/`bore`/`stroke`, `boost.kind`, `fuel.kind`, any field by FieldInfo.

**Wanted**
1. U1: `WorkspaceContext` gains `garage::Library* library = nullptr;` (forward declaration in Workspace.h), the frame
   ticks it. Then register GARAGE: `ui/views/garage/register.h` = `TEE_UI_WORKSPACE(garage, makeGarageWorkspace)`; it
   replaces the placeholder that `ui.frame.garage` drives (that test must change). `frame.scope` in `ui.frame.sheet`
   needs U1's re-bless (it now shows the real SCOPE).
2. Shell (card B): a `garage::Library` over `tee::files::BuildLibrary` + `BuildFile` (they map one to one; add the
   part, the values by path, the dyno runner's curve cached by `Build::hash()`).
3. Sim/packages: Scope `portIntake`, `portExhaust` (cylinder 1's port pressures per degree, Pa abs); telemetry
   `ecu.advance` (deg BTDC), `sound.rate` (Hz).
4. CMake, `ui/CMakeLists.txt`: the `tee_add_panel_shaders(tee_ui_app NAME fx ...)` line in `docs/ui/fx.md`, and
   `shaders::add(fxShaders())` where the app adds its tables.

**Not done / not run**: no person has seen anything here on a real display or GPU. macOS/Windows: by CI only.
