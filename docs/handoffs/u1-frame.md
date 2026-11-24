# Handoff: package U1 (the frame, DRIVE) — branch `claude/awesome-gates-ntdb58`

**Done** (`ctest -R '^ui\.'`, root `dev` preset, Linux; pictures by FunkGui's software rasteriser, no GPU run):
- Step 1 (bd34d6b): `Workspace`, `WorkspaceContext`, `Selection`, `ScopeSource`, `DynoSource`, `Live`, `look::Palette` and
  helpers, `DemoEngine`/`DemoRig`, `ui/views/_example`, the generated registry, `tee-view-tests` (`ui.example.select`,
  `ui.look.palette`, `ui.demo.source`). How-to: `docs/handoffs/u1-interface.md`. U2-U5 have since registered engine,
  exhaust, tune, dyno, scope on it unchanged.
- Step 2, the frame: `ui.frame.workspaces` (ten in order, every inspector page note-free, every non-hidden field on a
  page), `ui.frame.select`, `ui.frame.set` (drag, Cmd-Z, menu, unsaved star), `ui.frame.cockpit` (START, pedals drag and
  spring back, clutch, mode, readouts and dashes, shift lights), `ui.frame.garage` (J2's garage kept; units page),
  `ui.frame.ribbon` (edit -> new curve, old ghosted; folds), `ui.frame.sizes` (1100x700 to 2560x1440; rail narrows
  under 1280), `ui.frame.idle` (no frame wanted at rest), `ui.frame.sheet`; real Spec: `ui.build.frame`, `ui.build.set`.
- Step 3, DRIVE (`ui/views/drive`): `ui.drive.instruments`, `ui.drive.small` (goldens at density 2 and 1).
- Step 4: `ServiceDyno`/`SimScope` (`ui/app/host`, in `tee_ui_build`): `ui.build.dyno` on the real simulator.
- **Contact sheet: `out/ui/sheet.frame.png`** (density 2) and `sheet.frame.dpi1.png`: each workspace at 1100x700 and
  1600x1000; `out/ui/frame.<id>[.dpi1].png`, `frame.engine.paper.png` (the paper theme).

**What the shell must do** (card B; `app/Offline.cpp` already shows the frame for `--engine sim`, through J2's names,
which `AppFrame` keeps): pass the dyno and the Scope, else the ribbon says "no dyno runner":
`ServiceDyno dyno(sim->host().sim(), sim->build()); SimScope scope(sim->host());`
`makeAppFrame(doc, units, player, services, &dyno, &scope)` (both kept alive with the frame, in `Shown::keep`). Fill
the new `FrameServices` members: `unsaved`, `simLoad`, `mode`/`setMode` (ControlsFrame::mode), `clutch` (the mouse's
clutch pedal, -1 when let go). Load the units from the settings (`Units::load`). `--ref-panel` keeps `RefPanel`.

**Not done / waits**: the dyno curve on the real simulator waits for `tel.engine.torque` (the runner says the program
lacks it; the ribbon shows that sentence). Lambda and oil temperature read `engine.lambda`, `engine.oilTemperature`
(not in telemetry.fm yet: dashes). DRIVE's slip, tree and slip times read card I's `car.wheelSlip[i]`, `car.stripState`,
`car.t60`... (waiting for I's integration). No settings button (only units) and no rail pictograms yet. No GPU or Xvfb
run; macOS/Windows: CI only.

**Wanted outside my paths**: (1) Lead: `PanelControls` has no clutch; the frame reports it through
`FrameServices::clutch` until it does. (2) Lead/H: `tel.engine.lambda`, `tel.engine.oilTemperature`. (3) U5:
`ScopeWorkspace::inspectorPage()` starts with `panel "SCOPE"` without braces: a .gui note that fails
`ui.frame.workspaces` and `ui.build.frame` (drop the line). **Additive kit change**: `GuiPanel::setInsets` (J's tests
green). `ui/CMakeLists.txt` and `tests/ui/CMakeLists.txt` gained the view glob and `tee-view-tests` blocks.

**Changed after step 1** (compatible): `WorkspaceContext::services`, `Workspace::shownChanged`, `Live` re-asks absent
names, `DemoEngine::hold`. **FunkGui**: no issue met.
**For a person to look at**: `out/ui/sheet.frame.png`, `frame.drive.png`, `frame.air.png` (placeholder + inspector +
ribbon), `frame.engine.paper.png`; on a Mac: the application, DRIVE while revving (the rpm bar's shift lights, the
pedals dragged), an edit on AIR and the ribbon answering.
