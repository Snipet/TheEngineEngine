# U4: the TUNE, CAR and DYNO workspaces

Branch `claude/awesome-gates-ntdb58`. Pages: `docs/ui/tune.md`, `car.md`, `dyno.md`. Pictures: software rasteriser only.
**Done** (`ctest -R '^ui\.(tune|car|dyno)\.'`, 17 tests, all pass on Linux; `tee-view-tests all` on the merged tree:
1 of 80 fails, `frame.ribbon`, which pictures AIR):
- The frame's stage: every stage lays out for the rect it is given and folds on a small one (~660x570: DYNO narrows
  its side and cursor card, TUNE folds its map list into PREV/NEXT and drops the slices, CAR wraps its words and
  compacts driveline and gearing). `dyno.framestages.pictures` (nightly) pictures each at 660x570 and 1100x860 (`out/ui/*.frame*.png`);
  `dyno.text` holds no text over text and none off the stage at 660x570, 1100x700, 1100x860, 1600x1000 (d1), with a
  self-check that it does catch both. Goldens at the frame's exact stage rects wait for U1's numbers.
- DYNO `ui/views/dyno/`: `dyno.stage`, `dyno.large` (+`dyno.paper`), `dyno.run` (start, watch, land, rename, CSV, stop),
  `dyno.absent`, `dyno.sheet.pictures` (nightly). Shared drawing pieces in `dyno/Chart.h` (dashes, ticks, `chart::Layer`).
- TUNE `ui/views/tune/`: `tune.stage`, `tune.large` (+`tune.paper`), `tune.edit` (block, nudge, fill, smooth, typed,
  undo, the dyno asked), `tune.inspector`, `tune.absent`. Demo data: `tune/TuneDemo.h`.
- CAR `ui/views/car/`: `car.stage`, `car.large` (+`car.paper`), `car.select`, `car.topspeed`, `car.absent`. Demo data:
  `car/CarDemo.h`.
- Goldens at 1100x700 and 1600x1000, density 2 and 1, graphite; paper at 1100x700 d2. Contact sheet
  `out/ui/sheet.u4.png` (rows DYNO, TUNE, CAR; columns 1100x700, 1600x1000 at d1, then d2). I looked at every
  `*.dpi1.png` stage and large picture, the paper ones, and the sheet before blessing.
- Kit, additive (`ui/kit/widgets/Tables.*`), exactly: `MapEdit` members `ax_, ay_` (the block's anchor, set by
  `select()`), `block(c0, r0, c1, r1)`, `selectBlock(...)`; Shift+click and Shift+arrows move the selected cell keeping
  the anchor; a 2 px accent outline round a block of more than one cell; `drawKey` mixes the anchor. Nothing else
  changed: `tee-ui-tests all` 47 of 47 pass, its goldens untouched.
**Read, and on the branch today?** Fields: `ecu.spark` (demo; yes), `ecu.maps.{spark,lambda,boost,camIn,camEx,pedal,lift}`
and `...Auto` (no: `sim/ecu/settings.fm`, awaiting K3b), `boost.duty` (demo), `ecu.limiter.speed` / `ecu.limiter` (demo).
`car.mass`, `car.wheelbase`, `car.finalDrive`, `car.drive`, `car.tyre` (demo: radius) yes in the demo; `car.cgHeight`,
`frontWeight`, `track`, `cdA`, `clA`, `aeroBalance`, `tyreRadius`, `gearbox`, `gears`, `ratio[i]`, `gearEfficiency`,
`brake*`, `centre*`/`front*`/`rear*` diff fields, `env.surface` (VehSpec; no: starred defaults until integration).
Telemetry: `engine.speed/state/manifold/throttle/redline/flags`, `car.speed/gear`, `ecu.knockRetard` exist;
the integrated ECU's `ecu.lambda` (sensor), `ecu.lambdaTarget`, `ecu.knock` (count), `ecu.knockByCylinder[i]`, `ecu.load`, `ecu.advance`, `ecu.flags` (on the branch since 5b3a1c8) (ECU_FLAG_*), `car.accel`, `car.flags`,
`car.wheelLoad[i]`, `wheelSlip[i]`, `wheelTorque[i]` do not (names I chose: one line each in the tables). Scope:
`bakeCount`, `bakedNx/Ny/X/Y`, `baked` (in scope.fm; not in the demo: BAKE says so).
**Wanted**: U1 gave the run's partial trace (DYNO draws it), steps, dwell and gear (SWEEP/STEPS, gear passed), saveText
(SAVE CSV when the shell fills it) and the demo hook (TuneDemo.cpp, CarDemo.cpp). Since: BAKE asks through
`services->bakeMaps` and copies the bake in when it lands (`ui.tune.bake`); CAR draws `look::carProfile` (DRIVE's car).
Still wanted: card C's DynoPlan a gear; the host adapter's `runSoFar()`. H/I: the telemetry names above, or theirs.
**Fuel (owner's todo 1)**: TUNE's fuel band, octane from `sim/combustion/fuels.fm` via the new `FrameServices::simSource`
(app/Offline.cpp serves it); DYNO names curves and runs by fuel. Octane as a number (`fuel.octane`): `tune.octane`. Tests `tune.fuel`, `dyno.fuel`, `build.tune.fuel`.
**Not done**: GPU path under Xvfb not run;
macOS/Windows only by CI. Others' failures seen: `ui.frame.workspaces`, `ui.build.frame` (scope.gui notes), `ui.exhaust.*` once.

**Added by U1 for U4, 26 Nov 2026** (on the lead's word; U4's session idle since 25 Nov): DYNO marks the points the
program itself distrusts (card C's ask: first.json's 1733 rpm point read 293 N m against a steady 272, not settled,
drawn as an ordinary point). `DynoTrace::pointFlags` (ui/app, U1's) keeps card C's reasons apart: not settled, forced,
spooling (a stepped run's, once C's has it) are distrusted (`distrustAt`, `distrustText`); a boost ripple on a settled
point (`boostHunting`) is known to its standard error and only said (`rippleText`), the lead's ruling on C's numbers. In
`DynoWorkspace.cpp`: a hollow dashed ring (`kUnsteadyR` 7.5 px, the text colour) round a distrusted point on the torque
curve, wider than the knock's solid amber ring so both read on one point and the line runs on through; NOT SETTLED in
the legend; the cursor's card says the doubt in full (wrapped, never cut) and a ripple in the label colour without a
mark. The demo: its 6500 rpm unsettled point distrusted; from 185 kPa its 2000 rpm one rippling. Probes: `dynoWordsAt`,
`dynoUnsteadyDrawn`, `dynoPlotX` (Dyno.h). Tests: `dyno.unsteady`; `build.dyno.unsteady`; the picture
`build.dyno.unsteady.pictures` (nightly: first.json's real prediction, out/ui/dyno/). No golden moved.

**Added by U1 for U4, 26 Nov 2026** (on the lead's word, choice B of two pictures): knock that is the curve's state is
said once. A curve knock-limited at three quarters of its measured points or more (four measured at the least; a
limiter's point is not counted: `knockIsState`, probe `dynoKnockIsState`) shows its knock by the amber ticks under the
plot and the legend's tick, with no ring on each point (first.json: all 16 knock-limited, so its two dashed NOT SETTLED
rings are the only marks on the curve); a curve that knocks at a few points keeps a ring at each. The cursor card's knock
line takes the same mark. The cost, accepted: with no ring, a state curve's measured points show only as the ticks
beneath, not on the curve. DO NOT add the rings back as "point markers": a mark on a point means something is true of
that point. Also: the knock rings are drawn after the peaks' discs (a peak shows its own), and the cursor is carried
down through the band under the plot with a dot on each curve. Test `dyno.knock.state` pins the rule.

**Added by U1 for U4, 27 Nov 2026** (card C's bake across edits, options 1 and 2, the owner's yes; the words U1's, the
lead's ruling): TUNE's bake words now come from the frame's `BakeStatus` (ui/app/BakeStatus.h; `bakeStatusOf(services)`
in Workspace.h: `FrameServices::bakeStatus` when served, else `sparkMap`'s string parsed), so TUNE and the status line say
the bake alike: "Knock limit: baking the spark map 12 of 80; in use" (was the host's raw "baking spark map 12 of 80"),
and while the engine plays on the table from before an edit, "...; old table in use" (the label's ink: information, not a
warning). The octane sentence's "(baking the spark map 3 of 9)" likewise (TuneTests' two strings updated to it). The cam
line no longer needs `sparkMap`'s string to say "in use". "In use" comes from the engine's telemetry (ecu.knockTable;
the cam's Auto switch); with none (no engine running, as in the application before a start, or a test without a sim),
the service's own word that a stale table plays (BakeStatus::staleSpark/staleCam) says "old table in use" in its place.
Probe `tuneBakeWords` (Tune.h). Test `frame.bake.stale` (FrameTests.cpp). No golden moved.
A wording question for U4 (the lead's, 27 Nov; not changed): the knock limit's switch is titled "Not baked (no table)",
the state it turns on, so beside a line saying "old table in use" (the switch off, a table in play) the two read oddly
together. Whether the title should say what the switch is now, or name the choice ("Table / Not baked"), is U4's call.

**Added by U1 for U4, 27 Nov 2026** (the lead's word): CAR's "Gearbox and ratios" group no longer lists `car.autoClutch`.
No Spec field has that name (only the demo's ui/app/Demo.cpp does), so the real window showed nothing there. The
automatic clutch is the shell's setting, three-way (card B's `input::ClutchMode`: the shifter's clutch yours, the
default; automatic; yours), set in SETTINGS' CLUTCH row (U1's, FrameServices::clutchMode), one place for it. No golden
moved.
