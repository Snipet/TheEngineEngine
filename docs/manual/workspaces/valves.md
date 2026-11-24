# VALVES

The cams and everything that moves the valves: lift, duration, centrelines, phasers, two-stage cams, springs and
cylinder deactivation. The curves drawn here are made by the same cam generator the simulator runs, so what you drag is
what the engine gets.

Angles are crank degrees round the four-stroke cycle: 0 is the firing top centre, 360 the top centre between exhaust
and intake (where the valves overlap), 720 the end of the cycle.

## What you see

| Region | What it shows |
|---|---|
| CAM EDITOR | Intake lift (blue) and exhaust lift (orange) against crank angle over the whole cycle, top and bottom centres marked. The shaded area where both are open is the **overlap**; its angle at the seat is the first number over the plot, then the **lobe separation**, then the **room** between each valve and a flat-top piston near the overlap top (amber under 1 mm). The piston crown is ghosted as a grey curve, and a bar in each valve's colour marks where that valve comes closest to it. A phaser's range is a band along the top with a disc where it is now and a ring where the ECU wants it. A second cam stage is ghosted and named STAGE A or B. A vertical line is the crank now. |
| TIMING | The classic timing diagram: the exhaust's open period as the outer arc, the intake's as the inner one, with IVO, IVC, EVO, EVC in the corners as a cam card quotes them (degrees before or after top or bottom centre) and the overlap in the middle. |
| FLOW BENCH | Flow (CFM at 28 inches of water) against lift from 0.050 to 0.800 in, for each valve, as a flow bench would measure the head. A mark at a quarter of each valve's diameter, past which more lift buys little flow, and a disc where each valve's lift is now. |
| PHASERS AND STAGES | Each phaser's position now within its range, as a number and a bar; the stages ("B from 5500 rpm") with a lamp for the one engaged; the continuous-lift share now. |
| SPRINGS AND CYLINDERS, LIVE | The rpm at which the valves float and the margin below it (amber with NEAR FLOAT within 500 rpm; FLOATING NOW while they do). Each cylinder as a box: outlined amber where it may be deactivated, hatched while it rests. |

On a small window the editor runs across the top with the ring and one tabbed panel (BENCH, PHASERS, SPRINGS) below.
Panels the build has nothing for fold to a title line.

Typical values: street cams 200 to 230 degrees at 1 mm (0.050 in) of lift, race cams 260 and more; lobe separation
106 to 116 degrees; lift 9 to 13 mm.

## What you can change

| Press on | Does | Inspector shows |
|---|---|---|
| A lobe's peak (the ring) | Drag sideways: the centreline. Drag up or down: the lift | That cam (the stage that runs first, then the other), and the phasers |
| A seat mark (the tab at each end of a lobe) | Drag: the duration at the seat, about the centreline; the 1 mm duration keeps its distance | The same |
| Elsewhere on the plot | Selects the lobe under the pointer | The same |
| The timing ring | Selects the timing | The Valvetrain model and both cams |
| The flow bench | Selects the head | Valves a cylinder, valve diameters, port quality |
| Phasers and stages | | Phaser ranges and speed, Two-stage cams, Continuous lift |
| Springs and cylinders | | Valvetrain kind, Spring rate, Spring preload, Valve mass; deactivation |

The main fields: Intake duration at the seat (`valve.inDurSeat`), Intake duration at 1 mm, Intake lift, Intake
centreline (`valve.inCentre`), Intake lobe fatness, and the same for the exhaust. Each drag is one undo step and the
dyno ribbon answers it. While stage B is engaged the handles edit stage B.

A free valve (its actuation set to free) is drawn and dragged by where it opens and closes. The Second exhaust event
(`valve.auxOn`) is drawn as a small extra exhaust lobe.

With the Valvetrain model (`valve.model`) on **Reference**, the engine runs the original simulator's single cam dial:
the editor draws that dial's curves (224 degrees on 112 when mild to 300 on 106 when wild) and has no handles.
Reference is kept so the two models can be compared.

## Things to try

1. **More duration.** On "1.8 L twin-cam roadster", drag the intake's closing seat mark 20 degrees later. On the dyno
   ribbon the torque peak moves up the rev range and the bottom end falls. Note the overlap number growing.
2. **Advance the intake cam.** On "Four-cam 5.0 L V8", drag the intake peak 10 to 20 degrees earlier. Low-rpm torque
   rises and the top end falls a little (the simulator's own test: +13.8 % at 1500 rpm on a V8 with a 280 degree
   cam). Listen to the idle before and after.
3. **Mind the piston.** Add lift and drag both lobes towards the overlap top until the room figure turns amber.
   The simulator does not crash valves into pistons, but the figure shows where a real engine would.
4. **Two stages.** Load "Two-stage-cam 2.0 L", open PHASERS, and rev through 6000 rpm on the bench or the dyno. The
   lamp shows the change to stage B, and the editor swaps the solid and ghosted curves. Listen at the change.
5. **Float.** Lower Spring rate (`valve.springRate`) until the float speed comes inside the redline and NEAR FLOAT
   shows. Rev past it: power falls off steeply (in the simulator's test 21 % at 1.1 times the float speed and 64 % at
   1.25 times) and the valves bounce. Listen for the clatter with Mechanical voices on.
6. **Bigger valves.** Click the flow bench and raise Intake valve diameter (`valve.dIn`) by 2 mm. The bench curve
   lifts; compare the ribbon at the top end.

## What is not there yet

- The compression brake has no switch. Turning on the Second exhaust event makes it act on every cycle, and an engine
  with it on does not start.
- None of the shipped builds uses cylinder deactivation or continuous lift; the settings are there to try.
- A rotary engine keeps the original simulator's ports; the cam editor is for piston engines.
- The room above the piston assumes a flat-top piston with no valve pockets: an estimate.
