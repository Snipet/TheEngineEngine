# Cause 2 of pattern 2: the matched wastegate could not hold its spring

Measured with `tee-calib-boost` (README.md) on 99c6d5c and 83a8327: both with card G's network feeding the turbine's
housings (95d7d16) and D's e9a9748.

## Two states

The 2.0 L turbo four (target 120 kPa, spring 60) spooled at 5271 rpm, with F's matched gate (0.85 of the nozzle's
area, 26.3 mm), runs away: the charge pipe at 255 to 290 kPa, the gate wide open, knock retard pinned at its 15
degrees, the port gas at 1375 K, 421 to 460 N m (27 bar). Brought up from 2000 rpm in steps it held 120 kPa to 6500
rpm (backpressure.md, before G). Which state a run lands in depends on its history: N1's ledger (steps from 1500) and
P's predictor land in the runaway.

Steps from 1501 rpm (as the ledger), matched gate:

| rpm | torque N m | BMEP bar | IMEP bar | PMEP bar | FMEP bar | VE air | charge pipe kPa g | plenum kPa a | IC out K | turbine in kPa a | exhaust port mean kPa a | intake port mean kPa a | port ex/in | gate | shaft krpm | comp eta | SC kW | lambda | spark | 50% burn | knock ret | port gas K |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1501 | 183 | 11.6 | 12.9 | 0.01 | 1.18 | 0.96 | 18 | 118 | 298 | 113 | 123 | 118 | 1.04 | 0.00 | 59 | 0.41 | 0.0 | 0.87 | 15.4 | 8.0 | 0.0 | 941 |
| 2754 | 328 | 20.7 | 22.5 | 0.24 | 1.46 | 1.67 | 123 | 220 | 313 | 165 | 240 | 217 | 1.11 | 0.17 | 137 | 0.51 | 0.0 | 0.81 | 9.1 | 15.6 | 7.8 | 1102 |
| 4014 | 358 | 22.7 | 24.9 | 0.47 | 1.73 | 1.81 | 123 | 219 | 328 | 177 | 254 | 220 | 1.16 | 0.85 | 129 | 0.71 | 0.0 | 0.82 | 12.4 | 10.1 | 2.9 | 1202 |
| 5271 | 436 | 27.6 | 31.7 | 2.08 | 2.04 | 2.54 | 262 | 349 | 373 | 273 | 508 | 347 | 1.46 | 1.00 | 193 | 0.62 | 0.0 | 0.80 | 689.1 | 43.8 | 15.0 | 1372 |
| 6528 | 352 | 22.3 | 27.7 | 3.02 | 2.36 | 2.22 | 237 | 321 | 395 | 288 | 550 | 321 | 1.71 | 1.00 | 206 | 0.55 | 0.0 | 0.80 | 245.4 | 22.5 | 14.8 | 1442 |

The first half-seconds of a spool at 5271 rpm (each row 0.5 s; torque, charge pipe, plenum, turbine in, exhaust
port, port ratio, lambda, half-burn, knock retard, port gas): the boost overshoots in the first half second and never
comes back.

 447 | 281 | 368 | 349 | 669 | 1.82 | 0.80 | 63.9 | 14.9 | 1136 
 460 | 290 | 377 | 336 | 650 | 1.72 | 0.80 | 23.1 | 15.0 | 1254 
 460 | 285 | 372 | 319 | 612 | 1.64 | 0.80 | 22.9 | 15.0 | 1314 
 455 | 278 | 364 | 305 | 577 | 1.58 | 0.80 | 23.0 | 15.0 | 1344 
 449 | 271 | 358 | 296 | 554 | 1.55 | 0.80 | 23.0 | 15.0 | 1359 
 445 | 267 | 354 | 291 | 541 | 1.53 | 0.80 | 22.9 | 15.0 | 1366 

## The experiment: one thing changed, spooled at 5271 then 6528 rpm

Gate 26.3 mm (the matched size, given explicitly):

| rpm | torque N m | BMEP bar | IMEP bar | PMEP bar | FMEP bar | VE air | charge pipe kPa g | plenum kPa a | IC out K | turbine in kPa a | exhaust port mean kPa a | intake port mean kPa a | port ex/in | gate | shaft krpm | comp eta | SC kW | lambda | spark | 50% burn | knock ret | port gas K |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 5271 | 421 | 26.6 | 30.6 | 1.91 | 2.02 | 2.46 | 255 | 342 | 379 | 268 | 489 | 342 | 1.43 | 1.00 | 187 | 0.64 | 0.0 | 0.80 | 643.2 | 22.9 | 15.0 | 1375 |
| 6528 | 349 | 22.1 | 27.4 | 2.99 | 2.36 | 2.21 | 239 | 323 | 400 | 288 | 549 | 323 | 1.70 | 1.00 | 205 | 0.56 | 0.0 | 0.80 | 513.1 | 22.8 | 15.0 | 1447 |

Gate 32 mm (1.5 times the area):

| rpm | torque N m | BMEP bar | IMEP bar | PMEP bar | FMEP bar | VE air | charge pipe kPa g | plenum kPa a | IC out K | turbine in kPa a | exhaust port mean kPa a | intake port mean kPa a | port ex/in | gate | shaft krpm | comp eta | SC kW | lambda | spark | 50% burn | knock ret | port gas K |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 5271 | 346 | 21.9 | 25.0 | 0.99 | 2.06 | 1.79 | 126 | 218 | 332 | 194 | 287 | 218 | 1.31 | 0.75 | 133 | 0.75 | 0.0 | 0.82 | 14.5 | 8.0 | 0.0 | 1254 |
| 6528 | 334 | 21.2 | 25.2 | 1.58 | 2.45 | 1.80 | 129 | 218 | 339 | 213 | 325 | 218 | 1.49 | 0.93 | 141 | 0.75 | 0.0 | 0.82 | 14.5 | 8.0 | 0.0 | 1298 |

Knock control off, matched gate: still runs away (the gate is the cause; knock's late burn feeds it):

| rpm | torque N m | BMEP bar | IMEP bar | PMEP bar | FMEP bar | VE air | charge pipe kPa g | plenum kPa a | IC out K | turbine in kPa a | exhaust port mean kPa a | intake port mean kPa a | port ex/in | gate | shaft krpm | comp eta | SC kW | lambda | spark | 50% burn | knock ret | port gas K |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 5271 | 421 | 26.7 | 30.4 | 1.52 | 2.19 | 2.25 | 209 | 299 | 361 | 238 | 410 | 299 | 1.37 | 1.00 | 168 | 0.70 | 0.0 | 0.80 | 14.7 | 29.1 | 0.0 | 1260 |
| 6528 | 379 | 24.0 | 29.3 | 2.75 | 2.56 | 2.17 | 221 | 306 | 385 | 273 | 510 | 306 | 1.67 | 1.00 | 195 | 0.60 | 0.0 | 0.80 | 14.7 | 8.0 | 0.0 | 1335 |

## Why, and the change

A wastegate must be able to bypass nearly all of the exhaust when the turbine at full flow could make several times
what the compressor needs; that, not the turbine's size, is what sizes it (Bell, *Maximum Boost*, ch. 6; Watson and
Janota, *Turbocharging the Internal Combustion Engine*, ch. 9). `sim/boost/machine.fm` now sizes the matched gate so
that, open, it alone passes the turbo's whole share of the engine's matched flow at the nozzle's design state (2.4
bar, 1150 K, choked): `aGate = aNoz / 0.65` for a turbo of size 1 (35 mm on the 2 L four, 22 mm on the kei), and the
same area whatever `boost.size` (a smaller turbo needs a larger gate for its nozzle, not a smaller one).

## After (83a8327 with the change)

Steps from 1501 rpm:

| rpm | torque N m | BMEP bar | IMEP bar | PMEP bar | FMEP bar | VE air | charge pipe kPa g | plenum kPa a | IC out K | turbine in kPa a | exhaust port mean kPa a | intake port mean kPa a | port ex/in | gate | shaft krpm | comp eta | SC kW | lambda | spark | 50% burn | knock ret | port gas K |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1501 | 183 | 11.6 | 12.9 | 0.01 | 1.18 | 0.96 | 18 | 118 | 298 | 113 | 123 | 118 | 1.04 | 0.00 | 59 | 0.41 | 0.0 | 0.87 | 15.4 | 8.0 | 0.0 | 941 |
| 2754 | 323 | 20.5 | 22.2 | 0.24 | 1.44 | 1.65 | 122 | 220 | 313 | 165 | 240 | 217 | 1.10 | 0.09 | 137 | 0.50 | 0.0 | 0.81 | 10.2 | 14.4 | 7.7 | 1104 |
| 4014 | 355 | 22.5 | 24.7 | 0.46 | 1.74 | 1.82 | 124 | 219 | 328 | 177 | 253 | 219 | 1.15 | 0.46 | 129 | 0.71 | 0.0 | 0.82 | 12.5 | 10.1 | 2.8 | 1191 |
| 5271 | 346 | 21.9 | 24.9 | 0.99 | 2.06 | 1.79 | 126 | 218 | 333 | 193 | 287 | 218 | 1.32 | 0.61 | 133 | 0.75 | 0.0 | 0.82 | 14.5 | 8.1 | 0.3 | 1254 |
| 6528 | 333 | 21.1 | 25.1 | 1.60 | 2.45 | 1.80 | 129 | 218 | 340 | 212 | 326 | 218 | 1.50 | 0.77 | 141 | 0.74 | 0.0 | 0.82 | 14.5 | 8.0 | 0.6 | 1300 |

Spooled at 5271:

| rpm | torque N m | BMEP bar | IMEP bar | PMEP bar | FMEP bar | VE air | charge pipe kPa g | plenum kPa a | IC out K | turbine in kPa a | exhaust port mean kPa a | intake port mean kPa a | port ex/in | gate | shaft krpm | comp eta | SC kW | lambda | spark | 50% burn | knock ret | port gas K |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 5271 | 345 | 21.9 | 25.0 | 0.99 | 2.07 | 1.79 | 126 | 218 | 332 | 194 | 286 | 218 | 1.31 | 0.63 | 132 | 0.75 | 0.0 | 0.82 | 14.5 | 8.0 | 0.6 | 1254 |
| 6528 | 332 | 21.1 | 25.1 | 1.59 | 2.45 | 1.80 | 129 | 217 | 339 | 213 | 325 | 218 | 1.49 | 0.78 | 141 | 0.74 | 0.0 | 0.82 | 14.5 | 8.1 | 0.7 | 1298 |

From 2000 (the 2000 rpm point is 53 kPa with any gate on this commit, the gate shut: that is G's back pressure
slowing the spool at the bottom, not the gate; before G it was 126):

| rpm | torque N m | BMEP bar | IMEP bar | PMEP bar | FMEP bar | VE air | charge pipe kPa g | plenum kPa a | IC out K | turbine in kPa a | exhaust port mean kPa a | intake port mean kPa a | port ex/in | gate | shaft krpm | comp eta | SC kW | lambda | spark | 50% burn | knock ret | port gas K |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 2000 | 252 | 15.9 | 17.4 | 0.07 | 1.29 | 1.27 | 53 | 152 | 305 | 131 | 162 | 153 | 1.06 | 0.00 | 95 | 0.49 | 0.0 | 0.85 | 14.4 | 8.0 | 0.6 | 1036 |
| 3500 | 342 | 21.7 | 23.7 | 0.36 | 1.60 | 1.76 | 122 | 219 | 323 | 171 | 247 | 219 | 1.13 | 0.32 | 134 | 0.61 | 0.0 | 0.82 | 11.2 | 11.7 | 4.6 | 1154 |
| 5000 | 351 | 22.2 | 25.1 | 0.89 | 1.99 | 1.81 | 126 | 219 | 331 | 191 | 281 | 219 | 1.28 | 0.61 | 132 | 0.74 | 0.0 | 0.82 | 14.3 | 29.3 | 1.0 | 1242 |
| 6500 | 332 | 21.0 | 25.1 | 1.57 | 2.44 | 1.80 | 129 | 218 | 338 | 211 | 322 | 218 | 1.48 | 0.78 | 140 | 0.75 | 0.0 | 0.82 | 14.5 | 8.0 | 0.0 | 1297 |

The kei triple (P's 0.65 bar build) and at a 60 kPa target (its spring):

| rpm | torque N m | BMEP bar | IMEP bar | PMEP bar | FMEP bar | VE air | charge pipe kPa g | plenum kPa a | IC out K | turbine in kPa a | exhaust port mean kPa a | intake port mean kPa a | port ex/in | gate | shaft krpm | comp eta | SC kW | lambda | spark | 50% burn | knock ret | port gas K |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 2500 | 67 | 12.9 | 14.4 | 0.08 | 1.37 | 1.11 | 29 | 129 | 301 | 142 | 136 | 129 | 1.05 | 0.00 | 115 | 0.49 | 0.0 | 0.86 | 16.8 | 8.1 | 0.0 | 1065 |
| 3500 | 89 | 17.0 | 18.7 | 0.17 | 1.47 | 1.42 | 66 | 166 | 307 | 177 | 177 | 166 | 1.07 | 0.11 | 169 | 0.57 | 0.0 | 0.84 | 17.0 | 8.1 | 0.0 | 1142 |
| 5000 | 93 | 17.8 | 19.8 | 0.36 | 1.61 | 1.47 | 66 | 165 | 309 | 184 | 186 | 165 | 1.13 | 0.45 | 161 | 0.71 | 0.0 | 0.84 | 17.0 | 8.0 | 0.0 | 1211 |
| 6500 | 87 | 16.7 | 19.1 | 0.60 | 1.80 | 1.40 | 66 | 164 | 309 | 197 | 198 | 164 | 1.21 | 0.60 | 164 | 0.74 | 0.0 | 0.84 | 17.0 | 8.0 | 0.0 | 1265 |
| 7800 | 85 | 16.3 | 19.2 | 0.83 | 2.01 | 1.39 | 66 | 164 | 310 | 210 | 211 | 164 | 1.29 | 0.73 | 168 | 0.75 | 0.0 | 0.85 | 16.9 | 8.1 | 0.0 | 1311 |

| rpm | torque N m | BMEP bar | IMEP bar | PMEP bar | FMEP bar | VE air | charge pipe kPa g | plenum kPa a | IC out K | turbine in kPa a | exhaust port mean kPa a | intake port mean kPa a | port ex/in | gate | shaft krpm | comp eta | SC kW | lambda | spark | 50% burn | knock ret | port gas K |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 3500 | 86 | 16.4 | 18.1 | 0.17 | 1.46 | 1.37 | 60 | 160 | 307 | 171 | 172 | 160 | 1.07 | 0.12 | 163 | 0.56 | 0.0 | 0.85 | 16.9 | 8.0 | 0.0 | 1143 |
| 6500 | 85 | 16.2 | 18.6 | 0.61 | 1.79 | 1.36 | 61 | 159 | 308 | 194 | 195 | 159 | 1.23 | 0.59 | 158 | 0.74 | 0.0 | 0.85 | 17.0 | 8.1 | 0.0 | 1266 |
| 7800 | 82 | 15.7 | 18.6 | 0.81 | 2.00 | 1.35 | 61 | 158 | 309 | 206 | 206 | 158 | 1.30 | 0.72 | 161 | 0.75 | 0.0 | 0.85 | 16.9 | 8.0 | 0.0 | 1310 |

The turbo four: peak torque 355 N m at 4014 rpm (class 350 to 400), 228 kW at 6528 (155 to 230): both inside. The kei:
93 N m (100 to 110), 69 kW at 7800 (44 to 50): the power still needs the production cars' boost taper (P's build).

The sequential rotary still holds 100 to 143 kPa against 70 with both gates wide open (below; before D's
four-stroke-equivalent displacement: its turbos are matched to half its air):

| rpm | torque N m | BMEP bar | IMEP bar | PMEP bar | FMEP bar | VE air | charge pipe kPa g | plenum kPa a | IC out K | turbine in kPa a | exhaust port mean kPa a | intake port mean kPa a | port ex/in | gate | shaft krpm | comp eta | SC kW | lambda | spark | 50% burn | knock ret | port gas K |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 1500 | 321 | 15.4 | 16.1 | 0.00 | 1.16 | 1.18 | 70 | 171 | 305 | 171 | 166 | 171 | 0.97 | 0.25 | 166 | 0.49 | 0.0 | 0.85 | 6.3 | 23.0 | 15.0 | 887 |
| 2500 | 469 | 22.5 | 23.3 | 0.05 | 1.29 | 1.77 | 143 | 240 | 322 | 245 | 236 | 241 | 0.98 | 1.00 | 209 | 0.73 | 0.0 | 0.80 | 3.5 | 22.8 | 15.0 | 1027 |
| 4000 | 354 | 17.0 | 18.0 | 0.35 | 1.37 | 1.28 | 100 | 197 | 321 | 228 | 219 | 202 | 1.09 | 1.00 | 185 | 0.75 | 0.0 | 0.82 | 13.2 | 13.3 | 6.7 | 1082 |
| 6500 | 382 | 18.4 | 21.2 | 1.94 | 1.86 | 1.49 | 100 | 195 | 341 | 375 | 363 | 198 | 1.83 | 1.00 | 323 | 0.48 | 0.0 | 0.84 | 11.5 | 13.7 | 7.9 | 1250 |

Tests: `tests/sim/calibration/boost/GateTest.cpp` (ctest calibration.boost.*): the four at 5271 and 6528 rpm and the
kei at 6500 and 7800 (target 60 kPa), spooled at the top, within -15 to +10 % of the target with the gate not
saturated; and the back pressure: the four's exhaust port over its intake 1.14, 1.29, 1.49 at 3500, 5000, 6500 rpm
(checked between 1.0 and 1.7); and one state: the four stepped up from 1500 rpm and spooled at 5271 agree within
10 % (120.1 and 120.4 kPa). The three take 37 s. F's own `TurboTest` "the spring holds its own pressure" now
expects the spring less the exhaust's push on the poppet (kFace of the housing's pressure): the larger gate holds
barely open, so it sits that far under its spring (85.4 kPa for a 90 kPa spring at 7000 rpm; expected 84.7).

With engine.wallHeat 1 (the engine as it will be after D's fix of the double-counted wall loss), the new gate:
the four stepped from 1501 rpm 220, 375, 402, 394, 381 N m (boost 124 to 129 kPa), spooled at 5271 and 6528 395 and
382 (127, 129 kPa); the 3.0 twin-turbo six 356, 404, 362 N m at 2865, 4297, 5730 rpm (100, 101, 86 kPa; at 5730 its gate
reads 0.98 open with the boost under its target, the shaft at 212 krpm: not explained yet, suspected the shaft's
over-speed relief), the same spooled at the top.
