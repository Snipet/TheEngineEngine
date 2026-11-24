# VALVES

*The workspace of `ui/views/valves/` (package U2). The design is [../plan/ui-design.md](../plan/ui-design.md) §3. Its
tests are `tests/ui/views/ValvesTests.cpp`; its pictures `out/ui/valves.*.png` and the contact sheet
`out/ui/sheet.u2.png`.*

## What the stage shows

Angles are the cycle's: 0 the firing top, 360 the overlap's top, 720 the cycle. The lift curves are made by the
simulator's own generator (`sim/valvetrain/cam.fm`, ported in `ValvesModel.cpp`): a sine to a power from the lobe's
fatness, its ends eased by a ramp solved so the 1 mm duration comes out as asked. So the curve under a handle is the
curve the engine runs.

**Two layouts.** On the frame's small stage the editor runs across the whole width, the timing ring under it at the
left, and one panel whose tabs choose BENCH, PHASERS or SPRINGS at the right. On a large stage the editor has the left,
the ring and the bench the right, and the phasers and the springs sit under the editor; each folds to a one-line title
when the build has nothing for it, and the editor takes the height. `ui.valves.framestage` pictures both, with E's
fields and with none, and checks the text as ENGINE's test does.

| Region | What it is |
|---|---|
| CAM EDITOR | Intake (cool, left of the overlap) and exhaust (hot) lift against crank angle over the whole cycle, TDC and BDC marked and named (FIRING, OVERLAP). Overlap is the shaded area under both, and its angle at the seat is the first number over the plot; then the lobe separation, and the **room** between each valve and a flat-top piston near the overlap's top (amber under 1 mm). The piston's crown is ghosted as a grey curve on the same length scale (its room above the crown), and a bar in each valve's ink marks where that valve comes nearest it. A key under the axis says what each faint line is. A phaser's range is a band along the top, the intake's advancing, the exhaust's retarding, with a disc where it is now; when it has moved, the parked lobe is ghosted behind the running one. A second stage is ghosted and named (STAGE A or B); the stage engaged is the solid one. A vertical line is the crank now. |
| TIMING | The classic diagram: the exhaust's open period as the outer arc, the intake's as the inner, round TDC (up) and BDC (down); IVO, IVC, EVO, EVC in the four corners as a cam card quotes them (BTDC, ABDC, BBDC, ATDC); the overlap in the middle. |
| FLOW BENCH | Flow (CFM at 28 in of water) against lift, 0.050 to 0.800 in, for each valve's head (`sim/valvetrain/bench.fm` through `area.fm`'s effective area), a mark at a quarter of each valve's diameter (where more lift buys little), and a disc where each valve's lift is now. |
| PHASERS AND STAGES | Each phaser's place now out of its range, as a number and a bar; the stages ("B from 5500 rpm") and which is engaged, a lamp and words; the continuous lift's share now. |
| SPRINGS AND CYLINDERS, LIVE | The speed the valves float at and the margin below it (amber, and the words NEAR FLOAT, within 500 rpm); each cylinder as a box, outlined amber where it may rest, hatched and dimmed while it rests, with how many rest now in words. |

## What is clickable

| Press on | Does | The inspector shows |
|---|---|---|
| a peak's ring | drag across: the centreline; up and down: the lift | the cam (the stage that runs first, then the other), the phasers |
| a seat mark (the tab at each end of a lobe) | drag: the seat duration about the centreline; the 1 mm duration keeps its distance below it | the same |
| the plot elsewhere | selects the lobe under the pointer (`valve.intake`, `valve.exhaust`) | the same |
| the ring | `valve.timing` | the model (lab or the reference's dial), both cams |
| the bench | `valve.head` | valves a cylinder, their diameters, the ports' quality |
| phasers and stages | `valve.phasers` | variable timing, two stages, lift control |
| springs and cylinders | `valve.springs` | springs, deactivation |

Each drag writes the fields through the document inside one gesture: one step of undo, and the dyno ribbon answers.
While stage B is engaged the handles edit stage B's fields. The lift axis holds still during a drag. With only the
reference's CAM dial (no lab fields) the curves are the dial's (224 degrees on 112 when mild, 300 on 106 when wild)
and there are no handles; with the lab's model switched off, the editor says the engine runs the dial.

## A two-stroke: PORT TIMING

With `engine.cycle` 1 the stage is PORT TIMING (the rail still says VALVES): the ports' open shares against one turn
(the Scope's when it is a two-stroke's, else from the geometry: a port's top edge stands where the slider-crank puts
the crown at its opening angle), the exhaust's with the power valve shut and open ghosted, a timed inlet's window shaded
(a reed's has no fixed timing, said in words), the crankcase's pressure on its own scale; marks to drag at each port's
opening and closing (its duration about bottom dead centre: `tsExhaustDur`, `tsTransferDur`, one undo step); the
port-timing ring (exhaust outside, transfers inside, a timed inlet innermost); the power valve's duration against speed
with the speed now; the crankcase and scavenging. Pages: Ports, Power valve, Crankcase, Scavenging. `ui.valves.twostroke`.

## E's integration

The fields and telemetry are E's own now (`sim/spec/valve.fm`, `tel.valve`): the phasers' targets show as a ring beside
the disc of where each phaser is; a free valve (`inKind`/`exKind` 1) is drawn and dragged by where it opens and closes
(`freeInOpen` ...), its peak moving both; the second exhaust event (`auxOn`) is a small hot lobe; FLOATING NOW replaces
the margin's label while `valve.floating` is set; the ring and the overlap take the simulator's events (`valve.ivo` ...
`overlap`, phased) when telemetry has them; the bench takes the simulator's (`valve.benchLift[i]` ...) when every
element is in telemetry, else this port of `bench.fm` from the head's fields, and says which.

## Cold oil

The lab lets a phaser move only with the oil above 313 K (40 C) and 70 kPa (valvetrain/phaser.fm), and stage B only
above 333 K (60 C) and 100 kPa (valvetrain/lab.fm); below, the phaser stays parked and stage A runs. From a cold start
the oil follows the coolant some 4 minutes behind (core/heat.fm), so for the first minutes of a run the phaser and the
second stage do nothing, by the model's rule. VALVES says so while it holds (`engine.oilTemperature`,
`engine.oilPressure`): the editor's band reads "INTAKE PHASER PARKED: OIL COLD", the PHASERS AND STAGES panel
"PARKED: OIL 15 C, MOVES AT 40 C" under the phaser's "0 of 60 deg, to 3" and "B WAITS: OIL 15 C, NEEDS 60 C" under the
stage's lamp, in amber; the words go once the oil is warm. `ui.valves.oil`; on the first build through the frame (the
TRY FIRST strip showing, the handles dragged where they are drawn, the phaser and stage B from telemetry):
`ui.build.valves.first` (tests/ui/build/ValvesFirstTests.cpp).

## What is read, and from where

By path through `kValveFields` in `ui/views/valves/ValvesModel.h` (package E's `sim/valvetrain/lab-spec.fm` names,
appended to `spec.valve`; the demo's `valve.intakeDuration`, `intakeLift`, `intakeCentre` and the exhaust's as
fallbacks; `valve.cam` for the reference). Telemetry E has not added yet, named there as a guess: `valve.inPhase`,
`valve.exPhase`, `valve.stage`, `valve.lift`, `valve.floatSpeed`, `valve.active`; absent, they draw dashes. The engine's
bore, stroke, rod and compression come through `ui/views/engine/EngineModel.h`; the crank from `engine.crank`.

## Derived, not set

The lift curves, the events and overlap, the lobe separation, the room above the piston (a flat-top piston and no
pockets: an estimate, said so), the flow bench, the float margin.
