# The VGT's whole-flow nozzle with its stop scaled, and the derived taper (25 Nov, local; n2-b-local 3520482c)

## Built (local, not pushed)

- A variable nozzle's matched open area passes the whole design flow. A wastegated turbine keeps 0.65.
- The vanes' stop is scaled by 0.65 (0.30 -> 0.195), so the shut area is the same metal as before. A stated
  turbAR keeps the stated stop.

## Full load, settled, the whole range

Instrumented copy. Cells give N m / kPa, then the shaft's share of its limit, then the vanes.

| build | below the peak | torque peak | above | rated |
|---|---|---|---|---|
| tractor 76.2 | 900: 2113 / 109, 0.74 | 1000: 2584 / 215, 0.90, v0.44 | 1300: 2761 / 261, 0.99 | 1900: 2339 kW 465, **1.007, choke flag** |
| pickup 65.4 | 1300: 820, 0.65; 1500: 953 | 1700: 996 / 191, 0.86, v0.48 | 2200: 1020, 0.90 | 2800: 1011 kW 296, **0.956** |
| 2.0 36.6 | 1500: 209 / 63 | 1750: **276** / 108, 0.73, v0.38 | 3000: 334, 0.82 | 4000: 296 kW 124, **1.024**, boost 160 / 140 with the vanes open |
| truck, D2's fixture + target 260, 73.9 | 900: 2168 | 1000: 2500 / 233, 0.94 | 1300: 2620 | 2100: 1890 kW 416, 0.983 |
| truck, the same, matched 81.8 | 1000: 2109 / 104 | 1300: 2607 / 258, 0.98 | 1900: 0.981 | 2100: 1868, 1.002 |

- **Every torque peak** is within 0.2 % of (b) as it was: the tractor 2584 against 2580, the pickup 996 against 995.
- **The 2.0's 1750 is back**: 276 against 274 with the old nozzle, and 257 with the old stop.
- **Take-ups**: not rerun. The peaks' boosts are reached within the 10 s settle in every row.

## The derived taper

The taper keeps p2 (absolute) in proportion to the fuel a stroke from the build's own curve, capped at the target.
That holds the torque peak's lambda.

| build | 1600 | 1800 | 1900 | 2000 | 2100 |
|---|---|---|---|---|---|
| tractor (target 260, peak 1000) | 238 | 217 | 202 | 186 | 171 |
| truck's sets (target 260 stated, peak 1000) | 235 | 210 | 195 | 180 | 165 |

The pickup and the 2.0 state no fuel curve (their flat line), so the derived taper is flat. There is nothing to state
until their curves are stated.

### Measured with it

| cell | flat | taper |
|---|---|---|
| tractor 1600 | 2657 N m / 210 kPa, 0.959, p3/p2 1.38 | 2660 / 209, 0.958, 1.38 |
| tractor 1900 | 2337 / 205, 1.008, 1.41, PMEP 1.98 bar | 2346 / 201, 1.003, 1.39, 1.89 bar |
| truck 1900 | 2144 / 180, 0.961 | 2145 / 176, 0.956 |
| truck 2100 | 1882 / 176, 0.984 | 1891 / 168, 0.973 |

- On the whole-flow nozzle the boost at rated is already below the flat target. The tractor makes 205 against 260:
  its compressor is past its choke (Mach 1.66, phi 0.87 of choke) and the back-pressure limit holds p3/p2 at 1.40.
- So the derived taper (202 at 1900) is close to what the engine makes anyway, and changes almost nothing: under 1 %
  on power, the shaft 1.003.
- **p3/p2 does not fall to 1.17.** That came only with a target of 160-180 (healthy-match.md), which is below the
  derived taper.

### BSFC and the ports' ratio, the tractor at 1900

| | old nozzle | whole flow |
|---|---|---|
| BSFC (23.7 g/s) | 190 g/kWh (450 kW) | **183 g/kWh** (465 kW) |
| exhaust-to-intake ports | 1.59 | **1.42** |

- BSFC is now inside 175-205.
- The ports' ratio is still above the patent's 0.90-1.07.
- **Rated power is 465 kW against the published 451**, and 482 with a target of 160. D2's fuel curves were fitted to
  published torque through the old pumping, and need deriving again at the end, as the lead said.

## The truck cases on D2's fixture (valves from the tractor, idleTrim -50), as committed: no target, the spring law

Run on the tree with the whole-flow nozzle. The fixture was applied to the content file locally and then restored.

| case | result |
|---|---|
| TurboTest:88 / :108 / :122 | VGT 92 kPa against the single's 205; fail |
| TurboTest's one state at 1300 | 150 / 154 kPa from 1100 / 2000; pass |
| GateTest's one state, short | pass |
| GateTest's one state, full | the VGT 149 against the fixed 206; :197-198 fail |
| BrakeTest :226 / :229 (High at 2000) | manifold 227 against 250; shaft 14416 against 11513; fail |

**Is it the nozzle?** No. The same fixture at 1300, held from 1100:

| case | boost | torque |
|---|---|---|
| single, fixed | 207 kPa | 2721 N m |
| VGT, spring law, old nozzle | 155 | 2469 |
| VGT, spring law, whole flow | 156 | 2487 |
| VGT, target 260, old nozzle | 274 | 2602 |
| VGT, target 260, whole flow | 204 | 2617 |

- **Measured:** the VGT's shortfall against the single under the spring law is the same with either nozzle. The
  back-pressure limit holds it at about 155 kPa. The fixed single has no limit.
- **D2 reported 185 against 196 for :88 / :108 on the new fixture.** I read 155 with the old nozzle on my copy of it.
  Suspected: exhaust.model or another set differs. To be reconciled with D2.
- With the fixture's target (260), the whole-flow VGT makes 204 kPa and 2617 N m, against the old nozzle's 274 kPa
  and 2602 N m. The torque is fuel-limited either way, so the lower boost costs nothing at 1300.
- **BrakeTest is D2's.** With the whole-flow nozzle the brake's vanes sat at 0.23, above the scaled stop of 0.195. So
  the brake commands a position, not the stop, and that position is now a larger area. The brake needs restating on
  the new nozzle: in the package, as the lead ordered.

## Predictions (n10/predictions.md)

1. Peaks within 1 %, the 2.0's 1750 back to about 274, rated shafts as before. **Right** (276; 0.956 / 0.983 /
   1.007 / 1.024).
2. Taper: the tractor at 1900 about 0.98, p3/p2 about 1.2, ports about 1.2, BSFC 190-195, about 480 kW. **Wrong**:
   1.003, 1.39, 1.40, about 183, 467. The derived taper is not low enough to move it.
3. TurboTest :88/:108 pass on D2's fixture with the whole-flow nozzle. **Wrong**: the committed cases run the spring
   law, which falls short of the single with either nozzle.
4. The taper's numbers: right as computed; their effect as above.
