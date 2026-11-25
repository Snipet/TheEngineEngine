# The model's compressor map, as built (for the published-map comparison the lead has a research agent reading)

The three compressors the lead named, as F's matching builds them on the tip (the design-point rule, designShare
0.85; the truck as D2's case: boost.model 1, single, VGT, gateSpring 220 kPa). Read from the running machine
(boost.mach.comp[0]); the lines computed from F's own formulas (sim/boost/compressor.fm), air at 101325 Pa and
288-300 K. "Surge" is the flag's line (0.8 of the knee's share on each speed line), "island" the peak-efficiency
flow (phiB), "choke" the speed line's choke flow; M is the tip Mach number of the speed line through that point
(560 m/s at the tip limit: M 1.61 to 1.65).

| compressor (as built) | inducer mm | PR | surge flow kg/s (M) | island flow kg/s | choke flow kg/s | island/surge | choke/surge | eta near surge | at tip limit? |
|---|---|---|---|---|---|---|---|---|---|
| turbo-four-20-gdi | 44 | 1.5 | 0.082 (0.83) | 0.152 | 0.262 | 1.85 | 3.18 | 0.65 | |
| turbo-four-20-gdi | 44 | 2.0 | 0.100 (1.11) | 0.184 | 0.262 | 1.85 | 2.64 | 0.66 | choke beyond (M 1.86 > 1.65) |
| turbo-four-20-gdi | 44 | 2.5 | 0.105 (1.30) | 0.190 | 0.262 | 1.81 | 2.50 | 0.66 | choke beyond (M 2.18 > 1.65) |
| turbo-four-20-gdi | 44 | 3.0 | 0.105 (1.44) | 0.190 | 0.262 | 1.81 | 2.50 | 0.64 | choke beyond (M 2.42 > 1.65) |
| diesel-20-common-rail | 38 | 1.5 | 0.062 (0.83) | 0.114 | 0.197 | 1.85 | 3.18 | 0.65 | |
| diesel-20-common-rail | 38 | 2.0 | 0.074 (1.11) | 0.138 | 0.197 | 1.85 | 2.64 | 0.66 | choke beyond (M 1.86 > 1.65) |
| diesel-20-common-rail | 38 | 2.5 | 0.079 (1.30) | 0.142 | 0.197 | 1.81 | 2.50 | 0.66 | choke beyond (M 2.18 > 1.65) |
| diesel-20-common-rail | 38 | 3.0 | 0.079 (1.44) | 0.142 | 0.197 | 1.81 | 2.50 | 0.64 | choke beyond (M 2.42 > 1.65) |
| truck-15l-six | 88 | 1.5 | 0.320 (0.83) | 0.591 | 1.019 | 1.85 | 3.18 | 0.65 | |
| truck-15l-six | 88 | 2.0 | 0.386 (1.11) | 0.714 | 1.019 | 1.85 | 2.64 | 0.66 | choke beyond (M 1.86 > 1.61) |
| truck-15l-six | 88 | 2.5 | 0.408 (1.30) | 0.739 | 1.019 | 1.81 | 2.50 | 0.66 | choke beyond (M 2.18 > 1.61) |
| truck-15l-six | 88 | 3.0 | 0.408 (1.44) | 0.739 | 1.019 | 1.81 | 2.50 | 0.64 | choke beyond (M 2.42 > 1.61) |

## What sets each (the constants to set from the published maps)

| quantity | constant (compressor.fm / machine.fm) | value now | basis |
|---|---|---|---|
| choke flow's scale | mc = 0.45 rho01 a01 A_ind arWide | 0.45 | judgement (a choked eye's flow coefficient) |
| the inducer's size | A_ind so that the full choke is 1.15 x the design flow (machine.fm, aAnn) | 1.15 | judgement |
| choke flow against speed | x (0.3 + 0.7 min(M / 1.2, 1)) | 0.3, 1.2 | judgement: **saturates at M 1.2**, so every line above PR ~1.8 chokes at one flow, where real maps' choke grows with speed |
| the knee (the surge line's place on a speed line) | phiK, a share of the choke flow | 0.5 | judgement |
| the surge flag's line | 0.8 x phiK | 0.8 | judgement |
| the island | phiB | 0.725 | judgement |
| the island's width in flow | eta = etaM - 0.9 (phi - phiB)^2 | 0.9 | judgement |
| efficiency against tip speed | etaM = etaMax - 0.25 (M - 1.1)^2 | 0.25, 1.1 | judgement (N3's "Mach term") |
| peak efficiency | etaMax (spec compEta) | 0.76 | the spec's default |
| pressure rise | tau_knee = 0.45 U^2 / (cp T), tau0 0.88, tauC 0.35 of it | 0.45, 0.88, 0.35 | judgement (a slip factor and losses) |
| the tip limit | omegaMax = 560 m/s / r_tip | 560 | judgement |
| A/R's widening | arWide = (A/R / 0.6)^0.25 | 0.25 | judgement |

## Read

The model's shape is the same for every wheel: island at 1.8 to 1.85 times the surge flow and choke at 2.5 to 3.2
times it at ratios 1.5 to 3.0. Choke saturates at the tip Mach of 1.2 (above PR ~1.8 every line chokes at one
flow). At PR 2 the surge flow is 0.38 of the full choke flow (the truck: 0.386 against 1.019 kg/s). Recalled, to be
replaced by the agent's table: published truck and diesel wheels put the surge line at PR 2 nearer 0.25 to 0.30 of
choke, and a ported shroud lower still. If the table confirms it, the knee (phiK), the surge locus (0.8) and the
choke's growth with speed (the 1.2 saturation) are the three constants that move.

## The map corrected from the published maps (written before the runs)

Changed, and only these (the lead's ruling): the knee phiK = (0.45 + 0.2 (M - 1)) x (A/R / 0.6)^-0.3, between 0.2
and 0.7 (was 0.5 x the A/R factor at every speed), and the choke flow growing with the tip Mach number to 1.63, the tip
limit's (was 1.2). Read by a mirror of the map in published-compressor-maps.md's terms (choke at the right end of
each ratio's line, surge on the flag's locus 0.8 phiK):

| PR | surge/choke (was) | published middle (range) | island/choke (was) | published | choke at the ratio / its largest (was) | published |
|---|---|---|---|---|---|---|
| 1.5 | 0.24 (0.31) | 0.24 (0.22-0.26) | 0.53 (0.61) | 0.51-0.61 | 0.90 (1.00) | 0.69-0.92 |
| 2.0 | 0.29 (0.38) | 0.30 (0.26-0.34) | 0.56 (0.70) | 0.62-0.76 | 1.00 (0.99) | 0.91-0.94 |
| 2.5 | 0.36 (0.42) | 0.37 (0.32-0.42) | 0.64 (0.74) | 0.62-0.91 | 0.96 (0.95) | 0.98-1 |
| 3.0 | 0.44 (0.45) | 0.44 (0.41-0.47) | 0.72 (0.78) | 0.75-0.78 | 0.90 (0.88) | ~1 |

The island at 2.0 and 3.0 sits a little left of the published (0.56 against 0.62 or more; 0.72 against 0.75): left
alone, as the ruling said. The efficiency 0.1 of choke right of surge reads 0.70-0.72 in these terms against the
small wheels' 61-69 % (a different reading from the 0.64-0.66 above, which took it at the surge point): left alone.
No map was published for a 2 L diesel's VGT compressor; it takes these ranges.

**Prediction, the truck at 1300 rpm** (D2's case; written before the run): on the old map the surge flow at PR 1.45
to 2 is 0.31 to 0.39 kg/s and the engine passes about 0.18 kg/s per unit of ratio, 0.26 to 0.35 kg/s: left of surge,
so two states. On the corrected map the surge flow there is 0.22 to 0.30 kg/s: the engine's line right of it. I
expect one state, from 800 and from 2000 within 5 % of each other, at or above the upper branch's 45 kPa and 1440 N m
(about 45 to 60 kPa); not the old map's 209 kPa, which the closed exhaust took away, not the map.

**Measured** (the prediction failed): the truck at 1300 keeps two states, 28.3 / 62.1 kPa (map only) and 25.6 / 58.6
(map and the sizing kept at the design point's tip speed), against 24.8 / 45.1 on the present tree. The full-load
tables (turbo four 1500-6200, 2.0 common-rail 1800-4000, three trees) are in the message to the lead and in
map-and-sizing.patch's companion below; the change itself is held, uncommitted in sim/, as
`map-and-sizing.patch` in this directory until the lead rules on (a) the sizing and (b) the knee's dynamic surge.

| turbo four, N m / charge kPa g | 1500 | 1800 | 2500 | 4000 | 6200 |
|---|---|---|---|---|---|
| present tree | 213 / 14 | 227 / 20 | 305 / 75 | 366 / 124 | 289 / 88 |
| map only | 231 / 21 | 257 / 33 | 394 / 123 | 367 / 124 | 290 / 88 |
| map and sizing | 222 / 17 | 239 / 25 | 319 / 86 | 366 / 124 | 289 / 88 |

| 2.0 common-rail, N m / charge kPa g | 1800 | 2500 | 4000 |
|---|---|---|---|
| present tree | 164 / 17 | 336 / 172 | 289 / 173 |
| map only | 172 / 21 | 337 / 171 | 288 / 171 |
| map and sizing | 168 / 19 | 332 / 172 | 290 / 174 |

## The held change's final form (the lead's ruling on the sizing; measured, not yet in)

`map-and-sizing.patch` now holds: the knee's law (floored at a tip Mach of 0.8, under the published maps' lowest
lines: at an idle the knee moved with each firing pulse and the tractor's no-spread clatter comb fell 12.3 -> 3.7 dB),
the choke's growth to 1.63, the island at 0.58 of the way to choke (the ridge 0.60 / 0.62 / 0.69 / 0.78 at ratios
1.5 to 3, inside the published ranges, at their edges at 2 and 3; surge/choke unchanged), the sizing rule's 1.15
read at the design point's tip speed (F's 1.15, its wording under the new law), F's map cases read at each speed's
own knee, and the sequential case's low point at 2000 rpm (at 2500 the engine now draws the join flow and the valve
cycles: a finding).

| turbo four, N m / charge kPa g | 1500 | 1800 | 2500 | 4000 | 6200 |
|---|---|---|---|---|---|
| a18f1bd4 | 213 / 14 | 227 / 20 | 305 / 75 | 366 / 124 | 289 / 88 |
| map alone | 218 / 16 | 242 / 26 | 394 / 123 | 364 / 124 | 290 / 88 |
| map and sizing | 209 / 13 | 227 / 20 | 313 / 80 | 364 / 124 | 289 / 88 |

The diesels' take-ups (settled N m, a18f1bd4 -> map and sizing): tractor 1300 1939 -> 2245, 2.0 at 2500 249 -> 331,
pickup 1600 654 -> 693; the other six within 1 %.

**The truck at 1300, held 30 s** (heat settled 1 s into each speed; 1 s windows): one state, reached slowly from
below: from 800 16.1, 19.5, 26.8, 50.7, 55.3, 53.2 kPa at 5 to 30 s; from 2000 52 to 56 kPa throughout. The 5 s case
reads the climb. Fuel 20.18 g/s on the rack's maximum in both (not a smoke limit: an air-fuel ratio near 7 low and 10
high). The prediction above (one state within 5 % in the case's window) **failed**: 28.3 / 62.1 kPa then, 15.5 / 52.3
on the final form, against the 45-60 it said.

Open for the lead's ruling: F's EngineTest's shaft speed at 3500 rpm (79,668 against its 80,000 class bound: the wheel
the sizing rule now builds is some 9 % larger), and the truck case's window.

## Which surge line (the lead's question), measured

The 0.24 / 0.29 / 0.36 / 0.44 above are on the **flag's locus, 0.8 of the knee**. On the knee itself (the speed line's
peak) the same map reads 0.30 / 0.37 / 0.45 / 0.55 of choke: above the published ranges at every ratio.

**Where the model goes unstable** (a centrifugal supercharger on F's rig at a fixed speed, bypass and blow-off
valve off, the throttle closed in steps of 0.004, 1.5 s each, then 0.2 s of samples):

| tip Mach | knee | steady down to (phi / knee) | oscillation starts (phi / knee) | flow reverses from (phi / knee) |
|---|---|---|---|---|
| 0.75 | 0.410 | 0.92 | 0.89 | 0.77 |
| 0.76 | 0.410 | 0.92 | 0.88 | 0.76 |
| 0.91 | 0.433 | 0.94 | 0.91 | 0.82 |

So the model's real surge line is near 0.9 of the knee, and it runs steadily on the rising branch between there and
the knee.

**The truck at 1300 rpm** (truck-15l-six with GateTest's sets; the compressor's phi from its telemetry by the map's
own formulas, inducer 89.2 mm): the low state sits at phi 0.257 to 0.263 of its choke at tip Mach 0.53 to 0.66,
**0.63 to 0.64 of the knee, and steady**: block by block (10.7 ms) the flow is 0.124 to 0.165 kg/s, the firing pulses,
never reversing, the surge flag on 100 % of the time. The high state sits at 0.65 to 0.68 of the knee and is **a deep
surge cycle**: the flow -0.08 to +0.50 kg/s, a period near 75 ms, the flag on 52 %. At 2000 rpm and its design ratio
(3.17) the point is at 0.91 of the knee. The engine passes 0.494 kg/s there against the 0.86 the matching assumes
(VE 0.9 at the design ratio's density): its operating line lies left of the knee everywhere.

**Prediction for the lead's (3) and (4)** (written before any run): with the knee moved to where the published surge
lines are (0.8 of today's law, so the flag's locus becomes the knee) and the model made to surge, not sit, left of it,
the truck at 1300 has **one state from both sides, and it is a surge cycle at about 50 kPa** (its point is still left
of the new knee, 0.78 to 0.82 of it), not 200 kPa. The low state vanishes because it cannot sit; the boost stays low
because the wheel is too big for what this engine passes. The cure for the boost is the wheel's size (a build's
stated hardware, or the matching's VE), not the map.

**The sequential pair's join** (F's rig, layout 3, comp[0] held 3.5 s): the 2500 rpm point's draw is 0.91 of the join
flow on a18f1bd4 and on the map alone, 0.97 on map and sizing: inside the valve's 0.8-1.0 hysteresis on every tree,
so the valve's state there depends on history. Read 0.5 s later than the case reads it, a18f1bd4's valve is 0.93 open.
The first compressor there: phi 0.73 of a 0.086 kg/s choke (a18f1bd4), 0.88 of 0.077 (map alone), 0.70 of 0.103
(map and sizing).

**The turbo four's wheel (first.json; turbo-four-20-gdi differs only in name)**, built by the matching: compressor
inducer 44.2 mm, exducer 59.6 (a18f1bd4 and the map alone: the same wheel, the map alone choking it at 0.8 of its full
flow at its design speed), 48.3 / 65.1 mm with the sizing kept; the turbine wheel 0.9 of the exducer (53.6 / 58.6 mm);
the turbine's nozzle 4.59 cm2 on every tree. The IS20 (P, rebuilders, about 1 mm): 42.5 / 54.0 mm compressor, 50.4 mm
turbine inducer. A build can state them today: spec.boost.compInducer, compExducer, compAR, turbWheel, turbAR (0:
matched); what a build cannot state is the trim apart from the two diameters, the turbine's throat area as a number
(it follows A/R and the wheel), or a map.

## The lead's ruling (i): the knee and the flag only; the design flow at the charge's temperature

Ruled: the knee's law x 0.889 (0.8 / 0.9 of today's), the flag (and the surge guard that reads it) at 0.9 of the knee,
the measured onset; the matching's design flow at the charge's density at the design point (the compressor's outlet
at the design ratio, its efficiency the spec's compEta, cooled by the build's icEffect; 0: none) in place of 1.204 x
the ratio. The table before building it (design flow = compressorWants / 1.15 on a18f1bd4; what each engine passes at
its design speed, full load, heat settled, on map and sizing):

| build | design PR | charge T at design, K | design flow now kg/s | after | engine passes at its design speed | passes / now | passes / after | inducer mm: a18f1bd4 / map+sizing / after |
|---|---|---|---|---|---|---|---|---|
| truck (GateTest's sets) | 3.17 | 338 | 0.896 | 0.776 | 0.516 (2100 rpm) | 58 % | 66 % | 88.1 / 89.2 / 83.0 |
| tractor | 3.57 | 335 | 0.915 | 0.799 | 0.799 (1900 rpm) | 87 % | 100 % | 89.0 / 89.0 / 83.2 |
| 2.0 common-rail | 2.38 | 320 | 0.169 | 0.155 | 0.171 (4000 rpm) | 101 % | 110 % | 38.3 / 41.0 / 39.2 |
| pickup | 2.97 | 328 | 0.521 | 0.465 | 0.495 (2900 rpm) | 95 % | 106 % | 67.1 / 68.8 / 65.0 |
| first.json | 2.18 | 317 | 0.226 | 0.209 | 0.174 (5500 rpm, 99 kPa: short of its 120) | 77 % | 83 % | 44.2 / 48.3 / 46.4 |

The truck in GateTest's sets (a single, a 220 kPa spring, a 0.8 kg/s cooler) passes two thirds of even the corrected
design flow: its point at 2100 rpm is itself in surge cycles, short of its ratio; the temperature is not the whole of
it.

**Prediction, the truck at 1300 rpm after (i) and the temperature** (written before the run): its inducer 83.0 mm (area
-13 %), its knee 0.889 of today's: the low point moves from 0.63 to about 0.81 of the knee, still left of the flag at
0.9, so the surge guard still opens the vanes; left of the onset, so the point cycles rather than sits. One state from
both sides within 5 %, 55 to 65 kPa held until settled; the climb from 800 faster than 19 s to 90 % (the rotor's
inertia some 0.7 of today's), about 12 to 15 s.

## Built and measured (on the tip, N1's 6771c402 in)

The knee 0.40 + 0.178 (M - 1), floored at M 0.8; the flag and the surge guard at 0.9 of it; the island 0.61 of the way
to choke (0.58 put the ridge at 0.604 at a ratio of 2, under the published 0.62; 0.61 reads 0.604 / 0.620 / 0.702 /
0.787, at 3 over by 0.007, inside the readings' +-0.02); the sizing's 1.15 at the design tip speed; the design flow at
the charge's temperature.

**The onset on the flag** (the same rig, the throttle in 0.004 steps): steady down to 0.946 of the knee at M 0.75 and
0.970 at M 0.83; the next step falls into surge with the flow reversing, its mean at 0.90 and 0.90 of the knee.

**The truck at 1300, against the prediction** (written above before the run): one state, 76.1 kPa from 800 and 78.2
from 2000 (heat settled, the last 5 s of 40); the case (GateTest, restated: heat settled, 15 s, read over 5 s) 78.1 and
81.1. The prediction's 55 to 65 kPa was **wrong** (low by some 15 kPa); its one state was right; its 12 to 15 s was
near: 11 s to 90 % from 800. The vanes stand open (1.00), the guard holding them, the point left of the flag.

| turbo four (first.json), N m / charge kPa g | 1500 | 1800 | 2500 | 4000 | 6200 |
|---|---|---|---|---|---|
| tip (a18f1bd4's loop) | 213 / 14 | 226 / 20 | 309 / 79 | 369 / 124 | 289 / 88 |
| the map alone | 220 / 17 | 245 / 28 | 390 / 123 | 368 / 124 | 292 / 88 |
| all of it | 216 / 16 | 237 / 25 | 385 / 124 | 365 / 124 | 288 / 88 |

The 2.0 common-rail at 1800 / 2500 / 4000: 165 / 336 / 288 on the tip, 168 / 335 / 286 with all of it.

| take-up, settled N m (tip -> all of it) | low | mid | high |
|---|---|---|---|
| tractor 1000 / 1300 / 1800 | 653 -> 688 | 1946 -> 2263 (72 -> 219 kPa) | 2224 -> 2137 |
| pickup 1600 / 2400 / 3000 | 657 -> 995 (32 -> 185 kPa) | 997 -> 989 | 452 -> 431 |
| 2.0 1800 / 2500 / 4000 | 161 -> 164 | 250 -> 334 | 287 -> 285 |

The losses at the top (tractor 1800 -3.9 %, pickup 3000 -4.6 %, with more boost) are the turbine's side of the
design flow: a smaller nozzle, the turbine's inlet settling at 1.62 of the boost against 1.46.

**Findings, marked by name**: the variable nozzle's back-pressure limit at its open stop (400 kPa asked: 1.64 against
the case's 1.55; LayoutTest); the wastegate loop's overshoot where the turbo first reaches its target (the turbo four at
3500 rpm: 129 kPa at 3 s, 100 only after some 8 s; TurboTest); a sequential pair cycling at its join (the rotary at
4250: 26 to 106 kPa; GateTest, new). F's "variable vanes make boost at low speed", marked since the closed exhaust,
now passes; the diesel tier's TurboTest:118 (the big single over 30 kPa) and BrakeTest:213 pass.

## first.json with the IS20's wheels (P's figures; prediction written before the run)

Stated: compInducer 0.0425, compExducer 0.054, turbWheel 0.050 (rebuilders, about +-1 mm); A/R left matched. The
matching now builds a 46.4 mm inducer: the IS20's area is 16 % smaller, its exducer 54 against some 62.6 mm, its
turbine wheel 50 against some 56 mm (the rotor's inertia about half). **Prediction**: the low end a little better (a
lighter rotor and a wheel nearer its own flow: 1500 / 1800 rpm some 220 / 245 N m against 216 / 237), the target still
reached at 2500 (about 385), 4000 within 2 % (365), and the top lower, the compressor nearer its choke at 6200 (some
270 against 288). Nothing is sized from the maker's 350.

**Measured** (the same sweep, `--set` on the build, first.json itself unchanged; N m / charge kPa g / compressor eta /
shaft krpm):

| rpm | matched (46.4 mm) | the IS20's wheels | predicted |
|---|---|---|---|
| 1500 | 216 / 16 / .44 / 52 | **232** / 22 / .49 / 71 | ~220 |
| 1800 | 237 / 25 / .48 / 65 | **270** / 41 / .54 / 95 | ~245 |
| 2500 | 385 / 124 / .64 / 124 | 388 / 123 / .69 / 144 | ~385 |
| 3000 | 351 / 122 / .66 / 123 | 362 / 122 / .71 / 145 | |
| 4000 | 365 / 124 / .73 / 126 | 364 / 126 / .74 / 153 | ~365 |
| 5000 | 340 / 116 / .75 / 127 | 340 / 115 / .75 / 156 | |
| 6200 | 288 / 88 / .76 / 120 | 288 / 88 / .74 / 153 | ~270 |

The prediction was **wrong at both ends**: the low end gains more (+7 % and +14 % against +2 and +3 %), and the top
does not fall (288 either way; the IS20's compressor at 6200 still at 0.74, its tip 432 m/s, short of choke). The
maker's 350 from 1500 is still not reached at 1500 to 1800 (232 / 270): the spool there is the turbine's matter.
