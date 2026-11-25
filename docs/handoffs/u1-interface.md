# U1, first step: the `Workspace` interface (for U2 to U5)

*Package U1, branch `claude/awesome-gates-ntdb58`. The frame itself (top bar, rail, inspector, ribbon, cockpit) comes
next; this is what a workspace is written against, and it will not change shape under you. The design is
[../plan/ui-design.md](../plan/ui-design.md).*

## What is on the branch

| File | What |
|---|---|
| `ui/app/Workspace.h` | `Workspace` (a `Board` whose items and drawing keep to its stage), `WorkspaceContext`, `Selection`, `ScopeSource`, `DynoSource` (+ `DynoTrace`, `DynoRequest`), `Live` (telemetry by name, NaN when absent), `registeredWorkspaces()`. Each member documented there. |
| `ui/app/Look.h` | `look::Palette::of(theme)`: ground, panel, raised, edge, grid; inks value, text, label, dim, faint; by meaning `selected`, `cold`, `hot`, `hotter`, `ok`, `warning` (graphite and paper); `heat(K)`, `pressure(Pa)` tints. Type (`kNumberL/M/S`, `kUnit`, `kCaps`, `kTitle`, `kBody`), lines (`kHair`, `kTrace`, `kNeedle`), `kGauge`, `kPlot`, `kDiagram`. Helpers: `num` (a dash for NaN), `region`, `readout`, `level`, `pipe`, `selectRing`, `lamp`, `placeholder`. |
| `ui/app/Demo.h` | `demoFields()` (a made-up Spec with the real sections), `DemoEngine` (telemetry with `sim/spec/telemetry.fm`'s names, a Scope of 720 degrees and 64 pipe points, a dyno that answers edits of `engine.compression` and `boost.level`; `set`/`forget` a name), `DemoRig` (a whole `WorkspaceContext`). Fixed script, no clock. |
| `ui/views/_example/` | The smallest whole workspace (57 lines): a `Gauge` made on the board, a drawn part whose press selects `boost`, a live value in the user's units. |
| `tests/ui/views/` | `tee-view-tests`: `ViewKit.h` (`VIEW_TEST`, `CHECK`, `StageRig`, `golden`, `picture`, `typeKeys`), `Main.cpp`, `ExampleTests.cpp` (`ui.example.select`, `ui.look.palette`, `ui.demo.source`), goldens in `tests/ui/views/golden/`. |

`WorkspaceContext::doc` is a `FieldDoc&` (J2's interface; in the application a `BuildDoc` over card C's `tee::Build`),
not `BuildDoc&` as ui-design.md §5 sketched: `ui/` alone has no `tee::Build`, and a workspace needs only the fields.

## Writing one (U2: `engine`, `valves`; U3: `air`, `exhaust`; U4: `tune`, `car`, `dyno`; U5: `garage`, `scope`)

1. `ui/views/<id>/<Name>Workspace.cpp` (and `.h` if you like): a class deriving `tee::ui::Workspace`; implement
   `id()` (the directory's name), `title()` ("ENGINE"), `sections()` (the Spec sections its inspector shows when
   nothing is selected), `layout(stage)` (place your widgets inside `stage`), and draw the rest in
   `drawStage(canvas, theme)` with `look::Palette::of(theme)`. Optional: `inspectorPage()` (a `.gui` text for the
   selection; default: the frame's generated pages of the selected prefix), `ribbon()` (false on DYNO), `moving()`,
   `drawKey()` (mix what `drawStage` reads, so a still frame is not drawn again), `step(dt)`.
2. Clicking a part: override `pointerDown` (and `pointerMove` for `selection.hover`), call
   `ctx_.selection.select("exhaust.header")` for what was hit, `Board::pointerDown(e)` otherwise. Mark the selected
   part with `look::selectRing` or `region(..., selected)`: never colour alone.
3. Values: `ctx_.live.get("exhaust.portTemperature")` (SI, NaN when absent: draw `look::num(v)`, which is "-");
   fields: `ctx_.doc.get(ctx_.doc.find("valve.intakeLift"))`; Scope: `int h = ctx_.scope.find("pressure");
   ctx_.scope.read(h, buf, n)` only while shown; dyno: `ctx_.dyno.predicted()`, `before()`, runs. Units:
   `ctx_.units.format(Quantity::pressure, pa)`.
4. Register: `ui/views/<id>/register.h`, one line and no include guard (it is included twice):
   `TEE_UI_WORKSPACE(<id>, make<Name>Workspace)`, where `std::unique_ptr<Workspace> make<Name>Workspace(WorkspaceContext&)`
   is in namespace `tee::ui`. Re-run CMake; the build lists it (generated `WorkspaceRegistry.cpp`) and the frame puts it
   on the rail in place of that id's placeholder. Directories starting with `_` are skipped.
5. Rules: Canvas primitives only, opaque strokes (`p.over(c, a)`), text >= 10 px, no rotation, gradients or filled
   polygons; time only from `step(dt)`/`tick`; `drawStage` changes nothing; words, not icons.

## Testing and picturing one

`tests/ui/views/<Name>Tests.cpp`, nothing else to edit (CMake reads the `VIEW_TEST(ident, "name")` lines; one per
line). `StageRig rig(make<Name>Workspace, 1100, 700, /*dpi*/ 2.0f, /*t*/ 4.6)` hosts the workspace alone over the demo;
`rig.host->click(x, y)`, `rig.demo.selection.path()`, `rig.at(t)` (another moment of the script),
`rig.demo.engine.set/forget(name)`. `golden("air.stage", *rig.host)` compares a fingerprint and writes
`out/ui/air.stage.png`; make one at dpi 1 too (`golden("air.stage.dpi1", ...)`). Bless only after looking:
`build-dev/bin/tee-view-tests air.stage --bless`. Run: `ctest --test-dir build-dev -R '^ui\.'`. The frame's contact
sheet will include every registered workspace.

## Added since (all compatible with what was there)

- **The stage's size.** The frame gives a workspace `AppFrame::stageFor(w, h)`: at the smallest window (1100 x 700)
  `{64, 48, 696, 574}`, at 1600 x 1000 `{112, 48, 1128, 874}`; the ribbon (folded or not) is in the inspector's column
  and does not change it. A stage must lay out for the first and look generous at the second. Test both:
  `StageRig rig(makeX, FrameSize::smallest)` and `StageRig rig(makeX, FrameSize::large)` host the workspace in that
  window at that stage.
- **Your own demo data.** A second line in `register.h`: `TEE_UI_DEMO(<id>, <fn>)`, with `void <fn>(DemoExtras& x)` in
  namespace `tee::ui` in your directory. It is called with `x.fields` (append `FieldInfo`s to the made-up Spec) and,
  separately, with `x.engine` (`engine->addTelemetry(name, f)`, `engine->addScope(name, f)`, where `f` reads the script
  through `engine.rpm()`, `throttle()`, `phase()`, `field(path, fallback)`). Every DemoRig made with hooks (the
  default) has every package's data, so the frame's sheet shows real stages. Keep it in your directory; never edit
  `ui/app/Demo.*`. (U1's own goldens use `hooks = false`, so your additions never move them.)
- **Your page's notes are your test's.** `inspectorNotes(*rig.ws, rig.demo)` (ViewKit) is what the `.gui` reader says of
  your `inspectorPage()` now; check it empty. The frame's tests check only U1's pages (placeholders, DRIVE).
- **Demo names added**: telemetry `ecu.advance` (degrees before top dead centre), `sound.rate` (Hz); Scope `portIntake`,
  `portExhaust` (cylinder 1's port pressures per degree, Pa absolute). `DemoEngine::hold(true)` stops the script for
  a picture (the dyno still answers edits).
- **GARAGE's library**: `WorkspaceContext::library` (`garage::Library*`, may be null; `views/garage/Library.h`); the frame
  ticks it once a frame; the shell sets it with `AppFrame::setLibrary`.
- **Dyno**: `DynoRequest::steps` (rad/s, a stepped run), `dwell` (s), `gear` (a chassis run's; 0: the car's; not yet
  in card C's `DynoPlan`); `DynoSource::runSoFar()` (the run so far while `running()`, else nullptr; the demo gives
  it, the host adapter not yet). A curve may come refined (coarse first): each refinement arrives through
  `predicted()` with `revision()` moving; `before()` is the curve before the edit, not before the refinement.
- **Saving a file**: `FrameServices::saveText(suggestedName, text, error)` (`ctx_.services`, may be null or empty): the
  host's save dialog when the shell fills it; until then copy the text (`HostServices::copyText`).
- **Later still**: `look::carProfile(fx, rx, wy, r, squat, xs, ys)` and `look::carWindow(...)` (one car for DRIVE and
  CAR; front to either side); `FrameServices::bakeMaps()` (TUNE's BAKE presses `ctl.bakeMaps`; may be empty); the
  clutch pedal also goes through `PanelControls::clutch`; inspector tabs wrap to two rows past three and a `Button`'s
  title is cut with an ellipsis to its width.
- **Host adapters, revised**: `ServiceDyno` predicts through card C's `tee::PredictService` (coarse curve within about
  0.4 s, refined about 0.6 s, a build seen before at once): `predicted()` is the newest curve for the build as it
  stands (coarse, partial or refined: `refined()` says), `before()` the last refined curve of the build before the
  edit. Runs stay on `tee::DynoService`. `SimScope::generation()` itself asks the host for a newer copy, so a reader
  that waits for it to move (as ENGINE does) gets one (`ui.build.scope`, over a real SimHost).
- **Telemetry names**: `Live::first({...})` and `Live::lambda()` (`ecu.lambda`, settled; the oil's temperature is
  `engine.oilTemperature`). The demo adds `engine.torque`, `power`, `firing`, `imep`, `bmep`, `veAir`.
- **Settings, rail, keys** (frame): the top bar's SETTINGS shows the settings page in the inspector (audio:
  `FrameServices::audioInfo`, `audioMenu`; theme: FunkGui's `UiPreferences`; units; keys); the rail has a pictogram
  beside each word (`look::pictogram(c, id, cx, cy, size, ink)`, yours to use) and short words alone under 1280 px. Tab
  goes round the whole window (the frame's items, the stage's, the inspector's): a workspace's Board does not wrap
  (`Board::setTabWraps(false)`, set by `Workspace`). Check your stage names every control:
  `CHECK_MSG(unnamedControls(*rig.host).empty(), unnamedControls(*rig.host));` (ViewKit).
- **Demo hooks: your own only (important; replaces "by package")**: `StageRig` applies the base demo and the
  workspace's own `TEE_UI_DEMO` hook, no other, so another workspace's data never moves your goldens. A test that
  wants another's names passes them: `StageRig(make, w, h, dpi, t, tee::ui::DemoHooks::of({ "engine", "valves" }))`
  (`DemoRig(t, DemoHooks)` the same). The frame's contact sheet and `frame.real.*` pictures apply every hook (pictures,
  not goldens). **The base demo is frozen** (`ui.demo.base` lists its telemetry, Scope and field count): a new name goes
  in the owning workspace's hook. **No name twice**: `ui.demo.hooks` fails when two hooks, or a hook and the base,
  define one name, naming both (a hook may skip a field the base already has, as AIR's does).
- **The dyno's requests**: `DynoSource::requests()` counts the predicted curves asked for (`ui.frame.ribbon`: one drag,
  one request).
- **`ui.build.*` tests**: `BUILD_TEST(ident, "build.name") { ... return 0; }` with `rig` (the real simulator under the
  frame: `tests/ui/build/BuildKit.h`), in `tests/ui/build/<Name>Tests.cpp`; CMake finds the names. They run as one
  CTest test, `ui.build` (sim/ compiled once a process; the rig reset to a fresh Build, doc and frame before each
  case; each case's name and ok/FAILED in the log); `tee-ui-build-tests build.name` runs one alone; the nightly
  `ui.build.reverse` runs them last first, so a case must not lean on another's leftovers.
- **A preferred unit** (kit): `FieldInfo::preferUnit` ("m" for a listener's distance) is used wherever the person's
  chosen unit is of the same system (mm or cm: m; inches stay inches): `Units::chosen(q, prefer)`, and the optional
  last argument of `toUser`, `toSi`, `number`, `format`, `parse`. The inspector's pages and `fieldText` honour it.
  `BuildDoc` fills it from the host's `FieldInfo::prefer` as soon as that exists (the Spec attribute `ui::prefer("m")`
  is card C's and L's to add); until then a page reads in the chosen unit.
- **A truck's brakes** (card D2): `engineBrakeState(ctx)` (`app/Cockpit.h`): shown for spec.car.kind "Truck" or a
  fitted brake; `stage` 0..3 from `tel.ecu.brakeCylinders` over `engine.cylinders`, `armed` (spec.ecu.brakeSwitch),
  `exhaustOn` (`tel.exhaust.flap` past 5 % where present, else `tel.ecu.backPressure` over 1 kPa).
  `drawEngineBrake(c, p, x, y, state, words)` draws the lamps (DRIVE's scene, top left); the cockpit has them as
  `BrakeLamp` left of the readouts (narrow, in lambda's place). Tests `ui.frame.truck`, `ui.drive.truck`.
- **Pictures cost no budget** (01-rules.md section 3: 150 CPU-s a package, no default test over 30 s): a VIEW_TEST
  whose name ends `.pictures` is labelled `nightly`, so the default run leaves it out and the soak tier makes it
  at every push. Put pictures that check nothing there (the frame's: `ui.frame.pictures`, the contact sheets and
  `frame.real.*`); goldens stay in the default run.
- **A run's walls** (card C's `DynoCurve::settled`, the lead's ruling 1220ca3): `DynoTrace::settled` (1 settled, 0 cold
  walls, -1 not said: a sweep, a prediction) and `wallsWord()` ("walls settled", "cold walls", ""); a stepped run asks
  with `DynoRequest::settleExhaust` (default true). `ServiceDyno` and the demo fill it for runs. Label a run with it;
  never mix the two unlabelled.
- **The top bar's load** (the lead's ask, 19:20; `app/LoadMeter.h`): "62 % of a core" from the PEAK
  (`FrameServices::simLoadPeak`, HostStats::loadPeak; none: `simLoad`, the mean), a bar green under 70 %, amber to 90,
  red past it, and the word (ok, tight, over); "dropouts: n" from `FrameServices::dropouts` when not 0. The verdict is
  the host's `FrameServices::loadFits` (1, 0, -1) when served, else the peak held three seconds. Tight or over: the
  sentence "This build is heavy for this machine at Full: Economy halves it" and a QUALITY button (the settings page)
  in the status line's place; it never switches the quality and goes by itself. Tests `ui.frame.load`.
- **A tachometer's scale** (`look::rpmScale(redline)`, `look::shiftLightBefore`): the cockpit's rev bar and DRIVE's
  dial share it. The top a tenth past the redline, rounded up to a step of 1000 rpm, or 500 under a 4000 rpm redline
  (numerals in hundreds, "X100 RPM": a governed diesel's 2014 reads to 25); the shift lights' distances shrink with
  the redline. A workspace that draws its own tachometer should use it. Test `ui.frame.redline`.
- **Knock-limited points** (card C's 765684a): `DynoTrace::knock`, `retardMin`, `retardMax` per point (empty when the
  source says nothing), `knockLimited(i)`, `knockCount()`, `knockText(i)` ("knock-limited, retard 6 to 10 deg").
  ServiceDyno fills them for predictions and runs; the demo knocks past 160 kPa of boost from 4500 rpm (the default
  80 kPa and edits to 150 do not). The ribbon rings them on the torque curve, ticks them under the plot and says the
  retard in its title row. Card C's `DynoPlan::upward` (default true) runs steps lowest first: read points by speed.
- **TRY FIRST** (the lead's ask, 20:19; `app/TryFirst.h`): on a first launch a strip across the stage's top with the
  build's name and its notes' opening paragraph wrapped to it (six lines at most); while it shows, your workspace's
  `place()` gets the stage under it (shorter by the strip), so it covers nothing of yours: lay out for that too. `FrameServices::tryFirst` gives the notes while it should show (empty: not); the frame calls
  `FrameServices::tryFirstDone` (the shell stores it per viewer) at its Close button or once the engine has been
  started and stopped while it showed. Test `ui.frame.tryfirst`.
- **The spark map's bake** (card C's KnockBakeService, 8572845f): `ServiceBake` (app/host/HostSources.h) considers the
  build after each change and applies a finished bake still current; the shell ticks it once a frame and fills
  `FrameServices::sparkMap` (its state: "no table", "waiting to bake", "baking", "baked, to apply", "baked", "bake
  failed"). TUNE reads `ctx_.services->sparkMap` (may be null or empty). Test `ui.build` case `build.bake`.
- **Engine start** (the lead's ruling: warm, cold the person's choice): `FrameServices::coldStart` / `setColdStart`
  (card B's settings key "coldStart", from the next launch): the settings page's ENGINE START row, Warm and Cold.
- **The bake, cam too** (card C's CalibrationBakeService, 879c59f2): `ServiceBake` now bakes the intake cam, then the
  spark map, and applies each while still the build's; `sparkMap` says "baking cam", "baking spark map", "baking spark
  map again" while it works (the other words as before). The shell's wiring is unchanged.
- **The bake in the status line** (the lead's ruling, 01:33): while FrameServices::sparkMap says the bake works, the
  top bar's status line says its stage ("Baking the cam", "Baking the spark map"...), on every workspace, at
  1100 px too (it takes room from the name). Test `ui.frame.bake`.
