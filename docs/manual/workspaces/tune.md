# TUNE

The ECU: its maps of spark, fuel, boost, cam phase and throttle, and its limiter, launch, overrun, knock, idle,
traction, anti-lag and nitrous settings. The traces under the map say whether the tune is right.

## What you see

- **Maps** (left): spark advance, lambda, boost target, intake cam, exhaust cam, pedal to plate, lift command, and
  wastegate duty. Each row says whether its **formula** or its **table** is in use, or "not in this build".
- **The map** (centre): a table with the numbers and a heat tint at once, axes in your units. The cell the engine is
  in now is lit, and its last two seconds are trailed behind it, older fainter. At the top, the formula switch and
  **BAKE** when the map has a formula ("No formula" otherwise). While the formula is in use, a note says the table is
  kept but not read.
- **Slices** (right): the map cut through the live point along rpm and along load, the live point marked.
- **Traces** (under the map, the last six seconds): lambda measured by the sensor (solid) against lambda asked
  (dashed); knocks as bars, and the spark retard the ECU pulled for them as a line; the spark advance. The newest
  value of each is written at the right.
- **Knock by cylinder**: each cylinder's retard now as a small numbered bar, amber past half a degree.
- **Lamps**: limiter (with fuel cut or spark cut and its rpm), launch, overrun cut, traction, knock. Filled with a word
  when active; a ring and "off" when not.

The maps' axes are engine speed across and load down. Load is the manifold pressure over the outside air (1 is
atmospheric; above 1 is boost); the boost target map uses the gear instead, the pedal map the pedal.

Typical values: lambda about 1.0 at part load and 0.85 to 0.90 at full load on petrol (richer under boost); spark
advance 10 to 40 degrees before top centre depending on rpm and load.

## What you can change

- **A map's row** shows it and selects it; the inspector shows its formula switch (for example Spark: formula,
  `ecu.maps.sparkAuto`).
- **A cell**: type a value, or drag up and down.
- **A block**: Shift and a click, or Shift and the arrow keys. Then `- STEP` and `+ STEP`, or Page Up and Page Down
  (with Shift: five steps), nudge every cell in it. **SMOOTH** (or `s`) moves each cell halfway to the mean of its
  four neighbours. **FILL BETWEEN CORNERS** (or `i`) fills the block from its four corners. Each edit is one undo step
  and the dyno ribbon answers it.
- **BAKE** asks the simulator to write the formula's values into the table and switches the formula off, so you can
  start editing from what the engine was running.
- **A lamp** selects that part of the ECU; the parts without a lamp are on the ECU's inspector pages too:

| Part | Main settings |
|---|---|
| Limiter | Limiter (Fuel cut, Spark cut, Soft, Throttle), Limiter speed, Soft band, Soft retard |
| Launch | Launch control, Launch speed, Launch retard, Flat-shift |
| Overrun | Overrun fuel (`ecu.overrun.fuel`: the share of overrun firings still fuelled), Overrun retard |
| Knock | Knock control, Retard a knock, Most retard, Recovery |
| Idle | Idle speed, Cold fast idle |
| Traction | Traction control, Slip target, Pit limiter |
| Anti-lag, nitrous | on/off, rpm window, retard, lambda |

The fuel is set in the fuel section: Fuel (`fuel.grade`: Regular (87), Midgrade (89), Premium (91), Race, E85, Methanol, Diesel, and two untested rows),
Direct injection, Knock delay, Injector size. Timing adds degrees to the whole spark map; Pops sets the fuel let through on the overrun and the limiter to light
in the pipes.

The ECU has a switch, ECU model (`ecu.model`): **Maps** is the ECU described here; **Reference** is the original
simulator's ECU, kept so the two can be compared, which has no maps and one knock retard for all cylinders.

## Things to try

1. **Find the knock limit.** On "Small-block 5.7 L, carburettor-like", set Fuel to Regular (87). BAKE the spark map,
   select the high-load cells from 2500 to 4500 rpm and press `+ STEP` a few times. Hold full throttle on the dyno and
   watch knock bars appear and the retard line climb, cylinder by cylinder. Switch to Premium and repeat.
2. **Richer is not always stronger.** BAKE the lambda map and move the full-load row from 0.88 towards 0.80, then
   towards 0.95. The ribbon shows torque peaking near 0.90 and falling either side.
3. **Spark-cut limiter.** Set Limiter to Spark cut and hold the engine on the limiter. The unburned fuel goes into the
   exhaust and lights there: listen for the bangs. Compare Fuel cut and Soft.
4. **Overrun pops.** Set Overrun fuel to 0.1, then 0.4, rev to 5000 rpm and lift off. Listen to the overrun, and
   watch the overrun lamp.
5. **Anti-lag.** "Inline five turbo rally" has anti-lag. In gear at about 4000 rpm, lift off the throttle: inside
   its rpm window the ECU retards the spark and enriches the mixture so fuel burns in the exhaust and keeps the turbo
   spinning. Watch the ANTI-LAG lamp, boost and the shaft speed on AIR; listen to the exhaust.
6. **Change gear-by-gear boost.** On "3.0 L straight six, twin turbo", BAKE the boost target map and lower first and
   second gear's values. Launch on DRIVE and compare wheel slip.

## What is not there yet

- The diesel's smoke reading is not wired yet (it reads 0).
- The wastegate duty map is listed but the simulator does not have such a map; the boost machine runs its own
  control loop towards the boost target.
- Launch control is armed by a launch button, or by holding the clutch down at a standstill. The window offers no
  launch button and no way to turn the automatic clutch off yet, so launch control cannot be tried from the keyboard
  today.
