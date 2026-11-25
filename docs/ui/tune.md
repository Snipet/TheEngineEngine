# TUNE (`ui/views/tune/`)

*Package U4. The ECU's maps and the traces that say whether the tune is right.*

## What the stage shows

- **Maps** (left): spark advance, lambda, boost target, intake cam, exhaust cam, pedal to plate, lift command (the
  ECU's `EcuMapsSpec`, `ecu.maps.*`) and wastegate duty, each with whether its formula (`auto`) or its table is in
  use, or "not in this build".
- **The map** (centre): the kit's `MapEdit`, heat and numbers at once, axes in the user's units; the cell the engine
  is in lit (the kit) and its last two seconds trailed, older fainter (drawn over the table by a `chart::Layer`).
  At its top the formula's switch and BAKE when the map has a formula, else the words "No formula"; while the
  formula is in use a note over the table says the table is kept but not read.
- **Slices** (right): the map cut through the live point along speed and along load, the live point marked; through
  the middle when the engine is stopped.
- **The traces** (under): the last six seconds, sampled at 20 Hz: lambda from the sensor (`ecu.lambda`, solid)
  against what was asked (`ecu.lambdaTarget`, dashed); knocks as bars (the rise of the `ecu.knock` count each sample)
  and the retard pulled (`ecu.knockRetard`) as a line; the advance (`ecu.advance`). The newest value of each at the right.
- **Knock by cylinder**: each cylinder's retard now (`ecu.knockByCylinder[i]`, as many as `engine.cylinders`) as a
  row of small bars, numbered, amber past half a degree.
- **Lamps**: limiter (with fuel cut or spark cut and its speed), launch, overrun cut, traction, knock: a filled lamp
  and words when active, a ring and "off" when not, "-" while the program gives no flags.

- **The knock limit** (`ecu.maps.knockLimit`, card N2's): degrees off the best-torque spark, baked once for a build by
  card C's dyno sweep, not by the ECU's formulas. Its switch (`knockLimitAuto`) reads "not baked (no table)"; there is no
  BAKE for it. The host's state of that bake (`FrameServices::sparkMap`: "waiting to bake", "baking", "baked, to
  apply", "baked", "bake failed") and whether the program runs the table (`tel ecu.knockTable`) are said at the top
  right of the spark map and of the knock limit; the table changes by itself when a bake lands (one undo step). The
  intake cam's page says the same bake's state ("Intake cam: baking cam; not in use"): its table is set aside
  (`ecu.maps.camInAuto` on) while the calibration bakes the cam pass. On the knock limit, each cell that still knocks
  after the bake's corrections (`FrameServices::stillKnocking`, [speed][load] on the table's grid) is ringed in amber
  with a hatch in its corner, and the line counts them ("...; 3 still knock").

## What is clickable

A map's row (shows it, selects its field: the inspector shows its formula switch and the map); a cell (type, or drag
up and down: the kit); Shift and a click, or Shift and the arrows (a block: the kit, extended by U4); `- STEP`,
`+ STEP`, Page Up and Page Down (Shift: five steps) nudge the block; SMOOTH or `s` (each cell halfway to the mean of
its four neighbours; one cell chosen: the whole map); FILL BETWEEN CORNERS or `i` (bilinear from the block's corners
in the axes' own values). Each edit is one gesture of the document: one step of undo, and the dyno answers. A lamp
selects its part of the ECU (`ecu.limiter`, `ecu.launch`, ...); a lane its section.

## What is derived

The live cell, trail and slices are the map read where the engine is: `engine.speed` across, `ecu.load` down
(the boost target's: `car.gear`; the pedal map's: `ctl.throttle`, else the pedal as sent). BAKE asks the simulator for a bake (`FrameServices::bakeMaps`,
`ctl.bakeMaps`) and, when the Scope's `bakeCount` moves, copies `scope.baked*` into the table and turns the formula
off, one step of undo; with no way to ask it copies the newest bake there is. Nothing else is computed here.

## The fuel

A band across the top when the build has `fuel.grade` (sim/spec/fuel.fm): the grade by its name (the Spec's choices),
PREV FUEL and NEXT FUEL stepping among the spark-lit grades (one gesture; a diesel's own fuel is shown, not stepped);
AKI, RON and MON read from the simulator's own table (`sim/combustion/fuels.fm`'s `kFuelRon`, `kFuelMon`, through
`FrameServices::simSource`, which the application serves from the sim/ it compiled; `ui/views/tune/Fuel.*` reads the
text), a dash for each when the table is not served or has no number; a diesel says "No octane: lit by compression";
an untested grade is in amber and says so in words. One line of what it changes: before any change, how much longer
the end gas waits than on Regular (knock.fm's exponent, read from its text); after a change, the knock-limit map is
shown, the line gives the bake's state as the host says it, and once the table is re-baked, its change in degrees off
boost (load up to 1) and on boost (above 1), the mean over its cells. With `fuel.model` on Reference it says the grade
is not read. Pressing the band (off its buttons) selects `fuel.grade`: the inspector's menu.

Tests: `ui.tune.fuel`, `ui.build.tune.fuel` (the real Spec: the grade changed in the band re-keys the knock limit, undone the same key), `ui.tune.stage`, `ui.tune.large` (with `tune.paper`), `ui.tune.edit`, `ui.tune.inspector`, `ui.tune.absent`, `ui.tune.bake`, `ui.tune.knocklimit`.
