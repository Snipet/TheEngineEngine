# AIR

The path the charge takes from the air outside to the cylinders: filter, compressor, intercooler, throttle, plenum
and runners, with turbochargers or a supercharger in between.

## What you see

**The charge path** is a row of the parts the build has, left to right. A part the build lacks is not drawn.

- The air box with its filter (none when individual throttle trumpets open to the air).
- The compressors: one, two in parallel, sequential (PRIMARY and SECONDARY), compound (two in series), or four. A
  supercharger instead: Roots (two lobed rotors), twin-screw, or centrifugal.
- The intercooler, the blow-off valve on the charge pipe, the throttle (a butterfly drawn at its opening, with the
  percentage), the plenum, and the runners, or individual throttle bodies each with its trumpet.

Under each part: its live absolute pressure and its temperature. The pipe between two parts is tinted by the air
leaving the first, from blue near outside air to orange about 190 K hotter, with the temperature on it.

Inside the plenum (or above individual throttles) the **ripple**: how much the manifold pressure pulses, peak to
peak, as a number and a bar. The runners are drawn at the length in use, against the longer of a two-length
manifold's lengths. A line under them, RAM NEAR, gives the rpm where the runner's length should help filling most (the
main bump and a lower one), worked out from the runner's length and the cylinder's volume; it updates as you edit.

**The compressor map** (turbos and centrifugal superchargers): speed lines numbered in thousands of rpm, the surge
line (amber, labelled) on the left, the choke line on the right, and five efficiency islands. The operating point
leaves a trail of its last 180 moves, fading with age, so a spool-up, a shift and a surge each leave their shape.
With two turbos there is one map with both points (turbo 2 as a ring marked 2). The panel says FROM THE SIMULATOR when
the map comes from the running engine; with the engine stopped it is computed from the compressor's sizes the same
way. A Roots or twin-screw blower has no map: its panel shows the drive (pulley ratio, bypass, the power it takes).
A naturally aspirated engine shows the manifold (pressure, vacuum, throttle) there instead.

**Beside the map**: pressure ratio, corrected flow and efficiency at the operating point; the shaft speed in
thousands of rpm with a bar against its limit; the second turbo's speed; for variable vanes, a ring of vanes drawn
more nearly radial as they open.

**The gate and the blow-off valve**: each valve drawn lifting off its seat, with its opening; boost against the
target as a bar with the target marked; a trace of recent boost with the target dotted.

**Anti-lag and nitrous**, where the build has them: a lamp with ARMED, ACTIVE or ON, and the bottle's size.

Typical values: a turbo petrol engine runs 0.5 to 1.5 bar of boost; a small turbo's shaft turns at 100,000 to 200,000
rpm; compressor efficiency peaks around 0.70 to 0.78.

## What you can change

Click a part and the inspector shows its settings:

| Click | Main settings |
|---|---|
| Air box | Air box volume, Snorkel length, Snorkel bore |
| A turbo, the map or the shaft | Arrangement, Inducer and Exducer diameter, Compressor A/R, Turbine wheel, Turbine A/R, Housing (Single scroll, Twin scroll), Nozzle (Fixed, Variable vanes), Bearings, Electric assist, Shaft speed limit |
| Supercharger | Supercharger kind, Displacement a turn, Pulley ratio, Step-up gear, Bypass at part throttle |
| Intercooler | Intercooler effectiveness, drop, rated flow, core mass |
| Throttle | Throttle (One body and a plenum, A body a runner), Throttle area, Trumpets open to |
| Plenum | Plenum volume |
| Runners | Runner length (`intake.runnerLength`), Runner bore, Second runner length and Second length from (a two-length manifold) |
| The gate | Wastegate (Internal, External), Gate diameter, Gate spring, Boost target (`boost.target`) |
| Blow-off valve | Blow-off valve (None, To the air, Recirculating), Blow-off opens at, Blow-off diameter |
| Anti-lag, nitrous | Anti-lag; Nitrous shot, Nitrous bottle |

Sizes left at 0 are matched to the engine automatically. The boost model has a switch, Model (`boost.model`):
**Reference** is the original simulator's simpler turbo, kept so the two can be compared; **Machine** is the full
compressor-turbine-shaft model described in [Forced induction](../models/boost.md). The intake parts have the same
kind of switch, Induction model (`intake.model`).

## Things to try

1. **Watch a turbo spool.** On "2.0 L turbo four, direct injection", put the cockpit in dyno or drive mode and floor
   it from low rpm. Follow the operating point's trail up the map and the shaft-speed bar; boost meets its target bar.
2. **Surge without a blow-off valve.** Set Blow-off valve (`boost.blowOffKind`) to None, build boost, then snap the throttle shut. The
   trail runs back to the surge line and the flow reverses. Put the valve back and repeat: the valve opens instead.
   Listen for both.
3. **Runner length.** On "Flat-plane 4.5 L V8" (which has a two-length manifold), make Runner length 10 cm longer.
   The RAM NEAR rpm falls; on the dyno ribbon a torque bump moves down the rev range.
4. **More boost.** Raise Boost target on "3.0 L straight six, twin turbo" by 0.3 bar. Compare the ribbon, then check
   TUNE for knock and the map for how close the point runs to choke at the top.
5. **A blower.** On "Supercharged 6.2 L V8", raise Pulley ratio. Boost rises across the range (a Roots-type blower
   gives boost from low rpm), and so does the power it takes from the crank, shown in its panel.
6. **Sequential turbos.** On "Rotary twin", rev slowly and watch the second turbo's speed and the point where it joins
   (Second turbo joins at, `boost.seqFlow`).

## What is not there yet

- The bypass valve's position, whether anti-lag is active, and the nitrous bottle's contents are not reported by the
  simulator. The stage says what the build has (BYPASS AT PART THROTTLE, ANTI-LAG ARMED, the bottle's size) and the
  nitrous lamp lights while nitrous flows.
- The turbine is fed by a stand-in for the exhaust pipes until the exhaust builder is connected (see
  [Exhaust](../models/exhaust.md)).
