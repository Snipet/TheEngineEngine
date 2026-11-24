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
- **The traces** (under): the last six seconds, sampled at 20 Hz: lambda measured (solid) against asked (dashed);
  knock intensity as bars and the retard it pulled as a line; the advance. The newest value of each at the right.
- **Lamps**: limiter (with fuel cut or spark cut and its speed), launch, overrun cut, traction, knock: a filled lamp
  and words when active, a ring and "off" when not, "-" while the program gives no flags.

## What is clickable

A map's row (shows it, selects its field: the inspector shows its formula switch and the map); a cell (type, or drag
up and down: the kit); Shift and a click, or Shift and the arrows (a block: the kit, extended by U4); `- STEP`,
`+ STEP`, Page Up and Page Down (Shift: five steps) nudge the block; SMOOTH or `s` (each cell halfway to the mean of
its four neighbours; one cell chosen: the whole map); FILL BETWEEN CORNERS or `i` (bilinear from the block's corners
in the axes' own values). Each edit is one gesture of the document: one step of undo, and the dyno answers. A lamp
selects its part of the ECU (`ecu.limiter`, `ecu.launch`, ...); a lane its section.

## What is derived

The trail and slices are the map read where the telemetry puts the engine. BAKE asks the simulator for a bake (`FrameServices::bakeMaps`,
`ctl.bakeMaps`) and, when the Scope's `bakeCount` moves, copies `scope.baked*` into the table and turns the formula
off, one step of undo; with no way to ask it copies the newest bake there is. Nothing else is computed here.

Tests: `ui.tune.stage`, `ui.tune.large` (with `tune.paper`), `ui.tune.edit`, `ui.tune.inspector`, `ui.tune.absent`.
