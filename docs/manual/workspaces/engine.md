# ENGINE

The bottom end: how many cylinders, how they are arranged and fire, and their bore, stroke, rod and compression.

## What you see

Every picture here turns with one angle: cylinder 1's place in its cycle, 0 where it fires. **LIVE** takes it from
the running crank. **SLOW** replays the last recorded cycle at 2, 4 or 8 seconds a cycle so you can follow the valves
and the gas; it is a replay of a cycle already run, not the engine slowed down. With the engine stopped the pictures
stand at crank 0 and say so.

On a small window the block sits above one panel with three tabs (FIRING, P-V LOOP, DERIVED) and the cylinder in
section takes the right side. On a large window all of them are shown at once.

| Region | What it shows |
|---|---|
| BLOCK FROM ABOVE | Each bank's cylinders as numbered bores, front at the left. Each cylinder lights as it fires and fades over the first third of its power stroke; the number of the last to fire is written at the top right. Under the block, the crank's throws seen from the front, each pin where the crank has it now; cylinders sharing a throw are labelled together. A rotary draws its rotors and their three faces. |
| FIRING | The cycle as a ring with cylinder 1 at the top: a tick and number at each firing, an inner ring for each bank, a needle at the angle now. In the middle the intervals in words ("even, 90" or "270-180-90-180"), each bank's intervals and the firing order. |
| CYLINDER IN SECTION | Cylinder 1 cut through, with bore, stroke, rod and the space above the piston at top centre all to scale. Both valves lift by the real cam, the cams turn at half crank speed, the gas is tinted by its pressure (cool below outside air, hot above) with the number beside it, and the spark is marked at the ECU's advance. Under it: the angle, the stroke's name, the spark advance, both lifts, bore x stroke and the rod. |
| DERIVED FROM THE BUILD | Displacement, bore/stroke, rod/stroke, mean piston speed at the limiter (m/s, or ft/min with mph units), compression, the swept volume of one cylinder, each with the formula it comes from. |
| PRESSURE AGAINST VOLUME | One cylinder's pressure against its volume, linear or log-log (two buttons), each stroke in its own colour. The big loop is the work done; the small loop at the bottom is the work spent pumping air in and out. "NO SCOPE YET" until the engine has run. |

Typical values: mean piston speed at the redline of a road petrol engine is 20 to 26 m/s; above about 25 m/s the
engine is working hard. Bore/stroke over 1 is "oversquare" (short stroke, revs), under 1 "undersquare".

## What you can change

| Click | Inspector shows |
|---|---|
| A cylinder | That cylinder's bank, crank pin, which revolution it fires on, and its place along the crank (for a custom engine), then the layout |
| The block elsewhere, or the ring | The layout: the preset menu and the Custom engine switch (`engine.custom`); cylinders, rotors, bank angles; idle and redline trims; rough |
| The cylinder in section | Displacement, Bore / stroke, Bore, Stroke, Rod length, Compression; moving masses (piston, rod, flywheel); friction by part |

With nothing selected the inspector shows all the engine's settings. Hovering a cylinder rings it faintly; the
selected one has a coloured ring.

## Things to try

1. **Read a crossplane V8's burble.** Load "Four-cam 5.0 L V8" and open FIRING. The engine fires evenly every 90
   degrees, but each bank's ring shows 180-90-180-270. Now load "Flat-plane 4.5 L V8": each bank fires evenly. Listen
   for the difference at idle and on a slow rev, especially from the exhaust camera.
2. **Piston speed.** On "3.2 L straight six, high-revving", raise Stroke (`engine.stroke`) by 5 mm. DERIVED shows the
   displacement and the mean piston speed at the limiter rise. Watch the dyno ribbon: the low and middle torque rise;
   compare the top end.
3. **Compression.** Raise Compression (`engine.compression`) one point at a time on "Small-block 5.7 L,
   carburettor-like" and watch the ribbon. Then go to TUNE and watch the knock traces at full load on regular fuel.
4. **Watch a cycle slowly.** Rev to about 3000 rpm, switch to SLOW at 8 seconds a cycle, and follow the cylinder in
   section: exhaust valve opening before the bottom, both valves open together at the top (overlap), the spark before
   top centre, the gas colour at peak pressure.
5. **The pumping loop.** Open P-V LOOP in LOG-LOG at idle, then at full throttle on the dyno. At idle the small bottom
   loop (pumping) is large compared with the work loop; at full throttle it almost vanishes.
6. **Make your own layout.** Turn on Custom engine and change one cylinder's Crank pin (`engine.pin`). The FIRING
   ring and the intervals update. If the geometry makes two cylinders fire at once, the program refuses the layout
   (the telemetry "Custom layout refused" is set).

## What is not there yet

- SLOW replays the last cycle the Scope recorded; true slow motion of the engine is not available.
