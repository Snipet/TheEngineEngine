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
