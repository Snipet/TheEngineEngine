# SCOPE

The instrument bench: six live plots of what happens inside the engine and what comes out of it. They update only
while SCOPE is shown, and **FREEZE** holds all of them. Hover over any plot for a cursor line with each trace's value
in its title line.

## What you see

| Plot | What it shows |
|---|---|
| CYLINDER PRESSURE | Cylinder 1's pressure over one cycle, in crank degrees after the firing top centre (top and bottom centres ticked). The valves' open periods as bands (INTAKE, EXHAUST) with IVO, IVC, EVO, EVC in numbers; the spark as a dashed line; the point where half the charge has burned as a dotted line; the peak marked, with its pressure and angle. Under it the knock retard by cylinder (amber with its degrees while the ECU retards) and the knocks counted. |
| P-V LOOP | The same cycle against cylinder volume, linear or log-log (**LOG P-V**), with exhaust opening, intake closing and the spark as points. |
| PORTS | The intake port's pressure (solid, blue) and the exhaust port's (dashed, orange) over the cycle, with the valves' bands and the outside air's pressure as a reference line. |
| OUTPUT WAVE | The last whole cycles of the sound output (about 50 ms), lined up on cylinder 1's cycle so it stands still at a steady speed. |
| SPECTRUM | The output's spectrum in dB against frequency (log scale), smoothed over about 0.12 s. The firing frequency and its multiples are marked and numbered. |
| SPEED MAP | The spectrum against engine speed: each cell is filled as the engine sweeps through that speed, brighter for louder. The firing orders are drawn as slanted dashed lines, the speed now in the accent colour. **CLEAR MAP** empties it. |

The **firing frequency** is how many cylinders fire per second: rpm / 60 x cylinders / 2 for a four-stroke (a V8 at
3000 rpm: 200 Hz). An "order" is a multiple of the crank's rotation rate: a V8's firing is order 4.

If the simulator has not supplied something yet, its plot is empty with words saying what is missing; nothing is
made up.

Typical values: peak cylinder pressure at full load is about 50 to 80 bar in a naturally aspirated petrol engine and
over 100 bar in a turbo petrol or a diesel; best torque comes with the peak some 10 to 20 degrees after top centre
and half the charge burned at about 8 to 10 degrees after.

## What you can change

The inspector here holds the **listener**: where the sound is heard from and how it is mixed (the sound section:
View, Distance, Room, Level, Intake, Mechanical, Listener, Camera, Place, Engine place, Windows, Ground). It stays
here until the listener has a workspace of its own.

The plots' own buttons: FREEZE, LOG P-V, CLEAR MAP.

## Things to try

1. **Move the spark.** On TUNE, add 10 degrees with Timing (`ecu.timing`), then come back: the spark line moves
   earlier, the pressure peak rises and moves towards top centre, and on regular fuel the knock rows may light. Take
   the degrees back off.
2. **Pumping at idle.** Switch on LOG P-V at idle, then hold full throttle on the dyno. At idle the lower loop (the
   work spent pulling air past a nearly shut throttle) is large; at full throttle it nearly disappears.
3. **Fill a speed map.** CLEAR MAP, then rev "Four-cam 5.0 L V8" slowly from idle to the redline in neutral. Compare
   which orders carry energy with a map from "Flat-plane 4.5 L V8". Listen to both sweeps.
4. **Ports and runners.** Watch PORTS at full throttle while changing Runner length on AIR: the waves in the intake
   port shift in time against the intake valve's band.
5. **Freeze a knock.** Run on regular fuel with extra Timing, press FREEZE when the knock count rises, and look at the
   pressure trace near its peak.

## What is not there yet

- The listener's settings live here until the listener has its own workspace.
