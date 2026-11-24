# U4: the TUNE, CAR and DYNO workspaces

Branch `claude/awesome-gates-ntdb58`. Pages: `docs/ui/tune.md`, `car.md`, `dyno.md`. Pictures: software rasteriser only.

**Done** (`ctest -R '^ui\.(tune|car|dyno)\.'`, 15 tests, all pass on Linux):
- DYNO `ui/views/dyno/`: `dyno.stage`, `dyno.large` (+`dyno.paper`), `dyno.run` (start, watch, land, rename, CSV, stop),
  `dyno.absent`, `dyno.sheet`. Shared drawing pieces in `dyno/Chart.h` (dashes, ticks, `chart::Layer`).
- TUNE `ui/views/tune/`: `tune.stage`, `tune.large` (+`tune.paper`), `tune.edit` (block, nudge, fill, smooth, typed,
  undo, the dyno asked), `tune.inspector`, `tune.absent`. Demo data: `tune/TuneDemo.h`.
- CAR `ui/views/car/`: `car.stage`, `car.large` (+`car.paper`), `car.select`, `car.topspeed`, `car.absent`. Demo data:
  `car/CarDemo.h`.
- Goldens at 1100x700 and 1600x1000, density 2 and 1, graphite; paper at 1100x700 d2. Contact sheet
  `out/ui/sheet.u4.png` (rows DYNO, TUNE, CAR; columns 1100x700, 1600x1000 at d1, then d2). I looked at every
  `*.dpi1.png` stage and large picture, the paper ones, and the sheet before blessing.
- Kit, additive (`ui/kit/widgets/Tables.*`): `MapEdit` block selection (Shift+click, Shift+arrows; `block()`,
  `selectBlock()`); its outline drawn only for more than one cell. `tee-ui-tests all`: 47 of 47 pass.

**Read, and on the branch today?** Fields: `ecu.spark` (demo; yes), `ecu.maps.{spark,lambda,boost,camIn,camEx,pedal,lift}`
and `...Auto` (no: `sim/ecu/settings.fm`, awaiting K3b), `boost.duty` (demo), `ecu.limiter.speed` / `ecu.limiter` (demo).
`car.mass`, `car.wheelbase`, `car.finalDrive`, `car.drive`, `car.tyre` (demo: radius) yes in the demo; `car.cgHeight`,
`frontWeight`, `track`, `cdA`, `clA`, `aeroBalance`, `tyreRadius`, `gearbox`, `gears`, `ratio[i]`, `gearEfficiency`,
`brake*`, `centre*`/`front*`/`rear*` diff fields, `env.surface` (VehSpec; no: starred defaults until integration).
Telemetry: `engine.speed/state/manifold/throttle/redline/flags`, `car.speed/gear`, `ecu.knockRetard` exist;
`ecu.lambda`, `exhaust.lambda`, `ecu.knock`, `ecu.spark`, `ecu.flags` (ECU_FLAG_*), `car.accel`, `car.flags`,
`car.wheelLoad[i]`, `wheelSlip[i]`, `wheelTorque[i]` do not (names I chose: one line each in the tables). Scope:
`bakeCount`, `bakedNx/Ny/X/Y`, `baked` (in scope.fm; not in the demo: BAKE says so).

**Wanted**:
- U1: `DynoSource` a partial trace of the run in progress (I shade the swept span from the plan's rate); `DynoRequest`
  `steps` and `gear` (chassis); a save-file service for CSV (now the clipboard, `HostServices::copyText`); a way to
  press `ctl.bakeMaps` (BAKE only reads a bake already made); the demo hook, to move TuneDemo/CarDemo into.
- H/I at integration: the telemetry names above (or tell me theirs: a line each).
- The plan of a run is not a Spec field, so DYNO's inspector is empty and its plan is on the stage.

**Not done**: the auto switch was not exercised (the demo has no `...Auto` field); GPU path under Xvfb not run;
macOS/Windows only by CI. Others' failures seen: `ui.frame.workspaces`, `ui.build.frame` (scope.gui notes), `ui.exhaust.*` once.
