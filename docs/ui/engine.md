# ENGINE

*The workspace of `ui/views/engine/` (package U2). The design is [../plan/ui-design.md](../plan/ui-design.md) §3. Its
tests are `tests/ui/views/EngineTests.cpp`; its pictures `out/ui/engine.*.png` and the contact sheet
`out/ui/sheet.u2.png` (made by `ui.u2.sheet`).*

## What the stage shows

Every picture on the stage turns with **one angle**: cylinder 1's place in its cycle (0 where it fires). LIVE takes it
from the crank in telemetry (`engine.crank`, taken modulo the cycle: 720 degrees, a rotary's 1080); SLOW plays the
Scope's last cycle at a pace of 2, 4 or 8 seconds a cycle, from the stage's own clock (`step(dt)` only). The words
under the switch say which is on. With no crank angle the pictures stand at crank 0 and say so; nothing read from the
Scope is shown.

**Two layouts.** On the frame's small stage (the 1100 x 700 window's, 696 x 574) the block stands over one panel whose
tabs choose FIRING, P-V LOOP or DERIVED, and the section takes the right side's full height. On a large stage
(1600 x 1000's, 1128 x 874) the block stands over the ring at the left, the section in the middle, the loop over the
numbers at the right. `ui.engine.framestage` pictures both and checks that no text leaves the stage, crosses a panel's
edge or overlaps other text, on every tab.

| Region | What it is |
|---|---|
| BLOCK FROM ABOVE | Each bank's cylinders as bores, numbered, the front at the left. A bank leaning the way the crank turns is drawn below (the engine's right). Each cylinder lights as it fires and fades over the first third of its power stroke; the number of the one that fired last is at the top right, in words. Under the block, the crank's throws seen from the front, each pin where the crank has it now; cylinders that share a throw are labelled together. A rotary draws its rotors, their three faces turning at a third of the shaft's speed. |
| FIRING | The cycle as a ring, cylinder 1 at the top: a tick and a number at each firing, a quarter tick every 90 degrees, a ring inside for each bank with its own ticks, a needle at the angle now. The intervals in words in the middle ("even, 90" or "270-180-90-180"), each bank's intervals and the firing order under it, so a crossplane V8's 180-90-180-270 a bank reads at a glance. |
| CYLINDER IN SECTION | Cylinder 1 (or the one selected: the Scope's cylinder 1 at that cylinder's angle, said in words) cut through. Bore, stroke, rod and the clearance height (stroke over compression less one) are to scale with each other. The piston only translates; the rod is a segment, the crank a disc, a web and a counterweight arc. Both valves lift by the Scope's lift curves, scaled by the build's full lift where it has one: the stem, the head's tulip and its face open into the chamber; the ports run through the head in each gas's ink; over each stem its cam (a base circle and a nose), turning at half the crank's speed with its nose down at the lift's peak. The gas is tinted by its pressure (cool under the air outside, hot above) with the number beside it; the spark is a burst and the word SPARK at the ECU's advance (`ecu.advance`, degrees before top; none drawn without it). TDC and BDC are marked beside the bore. Under it: the angle, the stroke's name, the spark's advance, both lifts, and bore x stroke and the rod. |
| DERIVED FROM THE BUILD | Displacement, bore over stroke, rod over stroke, mean piston speed at the limiter (m/s, or ft/min where road speed is in mph), compression, swept volume of a cylinder: each with the formula it comes from in small type. |
| PRESSURE AGAINST VOLUME | The Scope's pressure against its volume, linear or log-log (two buttons), each stroke in its ink and named in the key, the point now marked. With no Scope: the axes and "NO SCOPE YET". |

## A two-stroke

With card T's `engine.cycle` 1 the cycle is 360 degrees: `tsCylinders` in a row, firing evenly or together
(`tsFiring`). The section has no valves: its ports are windows in the wall (the exhaust's at the right, the transfers'
at the left with their passage down to the crankcase), each bright where the crown has passed its top edge, the
exhaust's top lowered by the power valve's blade at low speed; the crankcase under the piston is a volume tinted by its
own pressure (the Scope's `portIntake`), with the number under the drawing; the inlet by its kind (a reed's petals
opened by the crankcase's pull, a disc's or a piston port's duct lit while its window is open); the spark at `tsSpark`.
The phase is named: POWER, BLOWDOWN, SCAVENGE, TRAPPING, COMPRESSION. The loop is coloured by power, ports open and
compression. A Scope of another cycle (a four-stroke's 720 degrees) is not drawn. `ui.engine.twostroke`.

## What is clickable

| Press on | Selects | The inspector shows |
|---|---|---|
| a cylinder | `engine.cylinder[i]` | that cylinder's bank, pin, firing revolution and place (a custom engine's), then the layout |
| the block elsewhere, or the ring | `engine.layout` | the layout menu (the presets) and the custom switch; cylinders, rotors and bank angles; idle and redline trims, rough |
| the section | `engine.bore` | the cylinder (displacement, bore/stroke, bore, stroke, rod, compression); moving masses; friction |
| nothing | (unchanged) | with nothing selected, the frame's generated pages of `engine` |

Hovering a cylinder rings it faintly; the selected one has the accent's ring. LIVE, SLOW, the pace, LINEAR and LOG-LOG
are buttons (kit `Button`).

## What is read, and from where

All by path through the table in `ui/views/engine/EngineModel.h` (`kEngineFields`: the branch's path, then the
standalone's or the demo's). Absent reads as NaN and draws a dash.

- Layout: `engine.layout` (a choice; its text names one of the reference's seventeen, whose firing angles are copied
  from `sim/core/layouts.fm`, or a short name like "V8"), or, with `engine.custom` on, `engine.cylinders`,
  `engine.rotors`, `engine.bankAngle[b]`, `engine.bank[i]`, `engine.pin[i]`, `engine.fireRev[i]`, `engine.place[i]`
  derived as `sim/core/layout.fm` does. The seventeen carry no geometry for a picture: their bank angles are the usual
  (a V8's 90 degrees, a flat engine's 180) and their pins follow from fire = bank angle - pin (+360 on the second
  revolution).
- Cylinder: `engine.bore`, `engine.stroke` when above 0, else from `engine.displacement` and `engine.boreStroke`;
  `engine.rodLength` (or `engine.rod`), else 3.3 times the crank's radius as the reference has it; `engine.compression`;
  the limiter `ecu.limiter`, else telemetry `engine.redline` (said in the formula line).
- Telemetry: `engine.crank`, `engine.speed`, `engine.redline`. Scope: `pressure`, `volume`, `liftIntake`, `liftExhaust`.
- Valve lifts for the section's scale: `valve.inLift`/`valve.exLift` (or the demo's `valve.intakeLift`/`exhaustLift`).

## Derived, not set

Displacement, the two ratios, piston speed, the clearance height, the pins of a preset, the firing order and intervals
of a custom layout. Nothing on the stage is editable only by the picture: every field is in an inspector page.
