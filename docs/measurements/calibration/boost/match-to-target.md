# Matching the turbo to the build's own pressure ratio: measured, not adopted

The lead's item (2) after the diesels' finding: F's matching sizes every turbo for a pressure ratio of 1.9 at the
redline, whatever the build asks (the tractor 3.6, the pickup 3.0). Tried: the compressor passing the engine's air at
the build's own ratio (its target or spring, at least 1.9), the turbine's design pressure scaled with it (so the nozzle
keeps its size), the gate likewise. "before" is 5269fdf's simulator with this branch's sim/boost and the back-pressure
limit (5eaac6f) (the tip's diesels do not fire since 5fdc5ad); "after" adds the matching. Engine dyno, 5 s a point, at
0.4, 0.65 and 0.9 of each redline. The 2.0 L common-rail diesel does not fire on either tree (a separate fault).

| archetype | target kPa | rpm | torque before / after N m | charge pipe kPa g before / after | gate or vanes b / a | p3/p2 b / a | flags b / a |
|---|---|---|---|---|---|---|---|
| boxer-four-turbo-unequal | 100 | 2799 | 390 / 389 | 105 / 105 | 0.01 / 0.01 | 1.11 / 1.11 | 0.5 / 0.5 |
| boxer-four-turbo-unequal | 100 | 4549 | 417 / 416 | 106 / 106 | 0.33 / 0.32 | 1.18 / 1.19 | 0.1 / 0.0 |
| boxer-four-turbo-unequal | 100 | 6299 | 366 / 366 | 109 / 109 | 0.44 / 0.45 | 1.32 / 1.31 | 0.0 / 0.0 |
| diesel-20-common-rail | 140 | 1920 | -196 / -191 | 0 / 0 | 0.30 / 0.30 | 0.99 / 0.99 | 0.0 / 1.0 |
| diesel-20-common-rail | 140 | 3120 | -196 / -191 | 0 / 0 | 0.30 / 0.30 | 0.99 / 0.99 | 0.0 / 1.0 |
| diesel-20-common-rail | 140 | 4320 | -196 / -191 | 0 / 0 | 0.30 / 0.30 | 0.99 / 0.99 | 0.0 / 1.0 |
| flat-plane-v8-39-twin-turbo | 130 | 3199 | 768 / 665 | 141 / 142 | 0.33 / 0.15 | 0.64 / 0.66 | 0.9 / 0.9 |
| flat-plane-v8-39-twin-turbo | 130 | 5199 | 776 / 767 | 154 / 155 | 0.68 / 0.63 | 0.70 / 0.71 | 0.0 / 0.6 |
| flat-plane-v8-39-twin-turbo | 130 | 7199 | 649 / 639 | 159 / 159 | 0.85 / 0.82 | 0.74 / 0.74 | 0.0 / 0.0 |
| inline-five-turbo-rally | 130 | 2799 | 432 / 414 | 135 / 136 | 0.11 / 0.04 | 0.95 / 1.02 | 0.4 / 0.5 |
| inline-five-turbo-rally | 130 | 4549 | 464 / 462 | 137 / 137 | 0.51 / 0.48 | 1.04 / 1.06 | 0.0 / 0.0 |
| inline-five-turbo-rally | 130 | 6299 | 402 / 399 | 138 / 138 | 0.59 / 0.56 | 1.11 / 1.12 | 0.0 / 0.0 |
| kei-660-triple-turbo | 65 | 3199 | 98 / 98 | 60 / 60 | 0.10 / 0.10 | 1.06 / 1.06 | 0.5 / 0.5 |
| kei-660-triple-turbo | 65 | 5199 | 87 / 87 | 38 / 38 | 0.47 / 0.47 | 1.18 / 1.18 | 0.0 / 0.0 |
| kei-660-triple-turbo | 65 | 7199 | 73 / 73 | 17 / 17 | 0.83 / 0.83 | 1.37 / 1.37 | 0.0 / 0.0 |
| pickup-diesel-67-inline-six | 200 | 1279 | 498 / 510 | 93 / 100 | 0.47 / 0.46 | 1.40 / 1.41 | 0.5 / 0.8 |
| pickup-diesel-67-inline-six | 200 | 2079 | 813 / 748 | 214 / 212 | 0.89 / 0.70 | 1.15 / 1.38 | 0.0 / 0.2 |
| pickup-diesel-67-inline-six | 200 | 2879 | 681 / 789 | 228 / 274 | 0.92 / 0.02 | 1.41 / 1.29 | 4.0 / 0.0 |
| rotary-twin-13 | 70 | 3199 | 229 / 229 | 72 / 72 | 0.04 / 0.04 | 1.01 / 1.01 | 0.5 / 0.5 |
| rotary-twin-13 | 70 | 5199 | 309 / 309 | 75 / 75 | 0.39 / 0.39 | 1.30 / 1.30 | 0.0 / 0.0 |
| rotary-twin-13 | 70 | 7199 | 280 / 280 | 76 / 76 | 0.13 / 0.13 | 1.43 / 1.43 | 1.0 / 1.0 |
| straight-six-twin-turbo-30 | 100 | 2799 | 226 / 227 | 101 / 101 | 0.09 / 0.08 | 0.82 / 0.83 | 1.0 / 1.0 |
| straight-six-twin-turbo-30 | 100 | 4549 | 308 / 306 | 77 / 84 | 0.69 / 0.67 | 1.09 / 1.09 | 7.0 / 7.0 |
| straight-six-twin-turbo-30 | 100 | 6299 | 246 / 253 | 53 / 57 | 0.88 / 0.89 | 1.30 / 1.27 | 7.0 / 7.0 |
| supercharged-v8-62 | 62 | 2479 | 871 / 871 | 67 / 67 | 0.00 / 0.00 | 0.64 / 0.64 | 0.0 / 0.0 |
| supercharged-v8-62 | 62 | 4029 | 909 / 909 | 95 / 95 | 0.00 / 0.00 | 0.59 / 0.59 | 0.0 / 0.0 |
| supercharged-v8-62 | 62 | 5579 | 832 / 832 | 143 / 143 | 0.00 / 0.00 | 0.51 / 0.51 | 8.0 / 8.0 |
| three-cylinder-10-turbo | 100 | 2599 | 180 / 179 | 101 / 101 | 0.10 / 0.07 | 1.08 / 1.10 | 0.4 / 0.4 |
| three-cylinder-10-turbo | 100 | 4224 | 182 / 182 | 101 / 100 | 0.40 / 0.40 | 1.19 / 1.19 | 0.2 / 0.2 |
| three-cylinder-10-turbo | 100 | 5849 | 175 / 175 | 102 / 102 | 0.61 / 0.63 | 1.30 / 1.29 | 0.0 / 0.0 |
| tractor-15-inline-six | 260 | 899 | 533 / 394 | 83 / 34 | 0.38 / 0.39 | 1.42 / 1.42 | 1.0 / 1.0 |
| tractor-15-inline-six | 260 | 1462 | 1784 / 1216 | 290 / 132 | 0.65 / 0.71 | 1.41 / 1.40 | 4.0 / 1.0 |
| tractor-15-inline-six | 260 | 2024 | 1420 / 1485 | 257 / 259 | 0.85 / 0.91 | 1.41 / 1.40 | 4.0 / 0.0 |
| turbo-four-20-gdi | 120 | 2719 | 372 / 360 | 124 / 124 | 0.23 / 0.16 | 0.77 / 0.79 | 0.4 / 0.5 |
| turbo-four-20-gdi | 120 | 4419 | 400 / 397 | 125 / 125 | 0.69 / 0.66 | 0.87 / 0.87 | 0.0 / 0.0 |
| turbo-four-20-gdi | 120 | 6119 | 308 / 307 | 88 / 88 | 0.84 / 0.81 | 1.00 / 1.00 | 0.0 / 0.0 |
| twin-turbo-hot-vee-v8-40 | 120 | 2719 | 769 / 710 | 128 / 133 | 0.26 / 0.15 | 0.70 / 0.72 | 0.9 / 0.6 |
| twin-turbo-hot-vee-v8-40 | 120 | 4419 | 743 / 737 | 136 / 137 | 0.59 / 0.53 | 0.76 / 0.77 | 0.2 / 0.7 |
| twin-turbo-hot-vee-v8-40 | 120 | 6119 | 673 / 668 | 145 / 145 | 0.79 / 0.76 | 0.83 / 0.84 | 0.0 / 0.0 |
| v6-35-twin-turbo-pickup | 100 | 2400 | 634 / 625 | 105 / 103 | 0.10 / 0.08 | 1.01 / 1.05 | 0.8 / 0.6 |
| v6-35-twin-turbo-pickup | 100 | 3900 | 619 / 620 | 110 / 110 | 0.43 / 0.39 | 1.12 / 1.13 | 0.3 / 0.3 |
| v6-35-twin-turbo-pickup | 100 | 5400 | 579 / 581 | 115 / 114 | 0.56 / 0.54 | 1.28 / 1.28 | 0.0 / 0.0 |

**Reading.** The builds asking about a bar barely move, as they should. The tractor is WORSE where it matters: at
1462 rpm the larger compressor no longer spools (290 -> 132 kPa, 1784 -> 1216 N m); the pickup is mixed (813 -> 748
at 2079 rpm, 681 -> 789 at 2879); the twin-turbo V8s lose 60 to 100 N m at their lowest point. A heavy diesel's turbo
is matched to its peak-torque flow with the vanes giving the low speed, not to its rated flow at its full ratio: the
old 1.9-at-the-redline sizing happens to give the tractor that (its flow at 2250 rpm and 1.9 is its flow at about 1190
rpm and 3.6). So the change is not made. What a right rule would be: size the compressor where the build asks its
target at its torque peak (a speed a build does not state today), with the surge line short of it; open.
