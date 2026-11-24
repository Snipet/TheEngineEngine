# Matching at the build's own design point (the lead's (a'), 24 Nov): the rule, written before the table

Until now F's machine matched every turbo to the engine's flow at its **redline** at a fixed pressure ratio of **1.9**
(`mMatch = vd x redRpm / 120 x 1.204 x 1.9 x 0.9`). That oversizes every turbine whose engine peaks well below its
redline or asks another ratio: a single hides it with its gate; the rotary's sequential pair (26 to 35 kPa against 70
at 6000 to 7200 rpm on the closed exhaust) and the pickup's top end (its vanes fully open at p3/p2 1.41, 462 N m at
3000 rpm) show it. The lead's ruling: match at a design point made only of what the build states, never of the
targets it is judged against.

## The design point (fixed here, before any run; not to be adjusted to bring rows in)

- **Pressure ratio**: the build's own boost, `(p_air + boost) / p_air`, the boost being `spec.boost.target` when set,
  else the wastegate's spring `spec.boost.gateSpring` (what the machine holds unasked); at least 1.3 (a lower ask is
  matched as 1.3: a turbo is not sized for less, judgement).
- **Speed**: spark-ignition engines 0.85 of the build's redline (designers' convention: peak power usually falls at
  0.8 to 0.9 of the redline; 0.85 its middle, judgement, written down before the table). A governed diesel: its
  governed (rated) speed, `spec.ecu.ratedSpeed` when set, else its redline, which is the rated speed the reference
  sets.
- **Air at that point**: the engine's swept flow at the four-stroke-equivalent displacement, volumetric efficiency 0.9
  against the charge's density at the ratio, intercooled to the air's temperature (`1.204 x ratio`): as now, only the
  speed and the ratio change.
- **The turbine's design inlet**: the pressure before it 1.247 times the boost's absolute at the design point (2.4 bar
  over 1.9 atm: the ratio the old match implied, kept), so a nozzle's area moves with the speed and not with the ratio
  (the flow and the pressure scale together); its gas 1150 K for spark ignition, 900 K for a diesel (a petrol
  turbine's inlet at rated power 1150 to 1250 K, a diesel's 850 to 950 K: G's measurements and makers' limits,
  recalled).
- **A sequential pair**: the two matched together to the whole flow, 0.5 each (not 0.55: their sum is the engine's).

## What it changes
The compressor's matched flow (its inducer), the turbine's nozzle, the matched gate (`aNoz / 0.65`), the sequential
pair's join flow (0.4 of the matched flow). Explicit sizes in a build are untouched.

The table below is a measurement of what the rule does on the fourteen boosted archetypes (engine dyno, the closed
exhaust, 0.4, 0.65 and 0.9 of each redline).

## The table (re-taken on G's closing by mass at the housings: the four's housings 0.997 of what the valves let out;
before = the redline match at 1.9, after = this rule; engine dyno, settle 5 s, stepped up; adopted by the lead)

| archetype | rpm | torque before / after N m | change % | boost kPa b / a | turbine in kPa b / a | gate or vanes b / a | knock retard b / a |
|---|---|---|---|---|---|---|---|
| boxer-four-turbo-unequal | 2800 | 195 / 210 | +7.8 | 10 / 17 | 119 / 125 | 0.00 / 0.00 | 0.0 / 0.0 |
| boxer-four-turbo-unequal | 4550 | 398 / 393 | -1.4 | 100 / 101 | 221 / 231 | 0.06 / 0.17 | 0.0 / 1.0 |
| boxer-four-turbo-unequal | 6300 | 351 / 339 | -3.3 | 100 / 100 | 253 / 265 | 0.19 / 0.30 | 0.0 / 0.0 |
| diesel-20-common-rail | 1696 | 157 / 156 | -0.7 | 14 / 13 | 126 / 124 | 0.00 / 0.00 | 0.0 / 0.0 |
| diesel-20-common-rail | 2756 | 327 / 341 | +4.2 | 141 / 162 | 326 / 305 | 0.07 / 0.00 | 0.0 / 0.0 |
| diesel-20-common-rail | 3816 | 319 / 318 | -0.3 | 160 / 165 | 365 / 383 | 0.00 / 0.12 | 0.0 / 0.0 |
| flat-plane-v8-39-twin-turbo | 3200 | 373 / 399 | +7.2 | 22 / 31 | 111 / 116 | 0.00 / 0.00 | 0.0 / 0.0 |
| flat-plane-v8-39-twin-turbo | 5200 | 712 / 706 | -0.9 | 131 / 131 | 157 / 161 | 0.32 / 0.46 | 6.0 / 4.6 |
| flat-plane-v8-39-twin-turbo | 7200 | 683 / 648 | -5.2 | 130 / 130 | 179 / 185 | 0.49 / 0.57 | 0.0 / 0.3 |
| inline-five-turbo-rally | 2800 | 207 / 218 | +5.3 | 13 / 19 | 116 / 122 | 0.00 / 0.00 | 0.0 / 0.0 |
| inline-five-turbo-rally | 4550 | 426 / 419 | -1.7 | 127 / 132 | 227 / 245 | 0.09 / 0.22 | 5.9 / 6.1 |
| inline-five-turbo-rally | 6300 | 391 / 378 | -3.3 | 130 / 129 | 266 / 280 | 0.21 / 0.28 | 0.0 / 0.0 |
| kei-660-triple-turbo | 3200 | 64 / 72 | +12.2 | 13 / 25 | 121 / 131 | 0.00 / 0.00 | 0.0 / 0.0 |
| kei-660-triple-turbo | 5200 | 78 / 78 | -0.9 | 38 / 38 | 153 / 158 | 0.13 / 0.27 | 0.0 / 0.0 |
| kei-660-triple-turbo | 7200 | 62 / 62 | -1.0 | 12 / 12 | 145 / 148 | 0.55 / 0.68 | 0.0 / 0.0 |
| pickup-diesel-67-inline-six | 1230 | 551 / 534 | -3.2 | 14 / 11 | 123 / 124 | 0.00 / 0.00 | 0.0 / 0.0 |
| pickup-diesel-67-inline-six | 1998 | 1012 / 954 | -5.7 | 198 / 94 | 418 / 240 | 0.00 / 0.00 | 0.0 / 0.0 |
| pickup-diesel-67-inline-six | 2767 | 997 / 984 | -1.3 | 163 / 230 | 369 / 496 | 0.00 / 0.00 | 0.0 / 0.0 |
| rotary-twin-13 | 3200 | 136 / 219 | +61.8 | 22 / 70 | 129 / 171 | 0.00 / 0.06 | 0.0 / 14.9 |
| rotary-twin-13 | 5200 | 212 / 220 | +3.8 | 35 / 44 | 165 / 189 | 0.00 / 0.00 | 8.6 / 9.4 |
| rotary-twin-13 | 7200 | 204 / 192 | -6.0 | 36 / 40 | 199 / 234 | 0.00 / 0.00 | 0.0 / 0.0 |
| straight-six-twin-turbo-30 | 2800 | 297 / 341 | +15.1 | 22 / 41 | 114 / 122 | 0.00 / 0.00 | 0.0 / 0.0 |
| straight-six-twin-turbo-30 | 4550 | 511 / 489 | -4.2 | 100 / 100 | 149 / 153 | 0.30 / 0.39 | 4.2 / 4.5 |
| straight-six-twin-turbo-30 | 6300 | 459 / 434 | -5.5 | 101 / 100 | 165 / 171 | 0.48 / 0.53 | 2.7 / 2.7 |
| supercharged-v8-62 | 2480 | 797 / 796 | -0.1 | 54 / 54 | 101 / 101 | 0.00 / 0.00 | 1.4 / 1.6 |
| supercharged-v8-62 | 4030 | 808 / 803 | -0.6 | 60 / 59 | 101 / 101 | 0.00 / 0.00 | 0.0 / 0.0 |
| supercharged-v8-62 | 5580 | 730 / 721 | -1.3 | 76 / 74 | 101 / 101 | 0.00 / 0.00 | 0.0 / 0.0 |
| three-cylinder-10-turbo | 2600 | 97 / 102 | +5.1 | 13 / 19 | 121 / 128 | 0.00 / 0.00 | 0.0 / 0.0 |
| three-cylinder-10-turbo | 4225 | 176 / 176 | -0.0 | 100 / 101 | 216 / 226 | 0.09 / 0.22 | 6.5 / 9.1 |
| three-cylinder-10-turbo | 5850 | 165 / 160 | -2.8 | 100 / 98 | 247 / 266 | 0.26 / 0.27 | 3.7 / 4.0 |
| tractor-15-inline-six | 806 | 808 / 765 | -5.3 | 11 / 11 | 123 / 142 | 0.00 / 0.00 | 0.0 / 0.0 |
| tractor-15-inline-six | 1309 | 2287 / 1861 | -18.6 | 216 / 66 | 444 / 202 | 0.00 / 0.00 | 0.0 / 0.0 |
| tractor-15-inline-six | 1813 | 2300 / 2263 | -1.6 | 163 / 246 | 370 / 506 | 0.05 / 0.00 | 0.0 / 0.0 |
| turbo-four-20-gdi | 2720 | 199 / 220 | +10.2 | 25 / 35 | 118 / 123 | 0.00 / 0.00 | 0.0 / 0.0 |
| turbo-four-20-gdi | 4420 | 357 / 352 | -1.3 | 119 / 120 | 170 / 176 | 0.20 / 0.34 | 5.1 / 4.6 |
| turbo-four-20-gdi | 6120 | 294 / 288 | -2.1 | 82 / 83 | 173 / 177 | 0.42 / 0.56 | 0.0 / 0.0 |
| twin-turbo-hot-vee-v8-40 | 2720 | 406 / 436 | +7.4 | 22 / 32 | 114 / 119 | 0.00 / 0.00 | 0.0 / 0.0 |
| twin-turbo-hot-vee-v8-40 | 4420 | 699 / 693 | -0.8 | 119 / 120 | 159 / 166 | 0.23 / 0.33 | 7.6 / 7.9 |
| twin-turbo-hot-vee-v8-40 | 6120 | 684 / 664 | -2.9 | 120 / 122 | 185 / 199 | 0.40 / 0.56 | 4.7 / 3.9 |
| v6-35-twin-turbo-pickup | 2400 | 344 / 369 | +7.1 | 14 / 21 | 122 / 130 | 0.00 / 0.00 | 0.0 / 0.0 |
| v6-35-twin-turbo-pickup | 3900 | 584 / 588 | +0.8 | 98 / 101 | 209 / 219 | 0.09 / 0.24 | 9.3 / 10.5 |
| v6-35-twin-turbo-pickup | 5400 | 577 / 560 | -2.9 | 101 / 100 | 252 / 274 | 0.29 / 0.33 | 7.8 / 8.2 |

**Read**: at 0.4 of the redline every petrol engine spools sooner (+5 to +15 %, the kei +12 %, the six +15 %, the
rotary 22 -> 70 kPa, +62 %): their turbines were oversized for their low end. At the top they give 1 to 6 % (the
turbine's inlet 5 to 20 kPa higher, the gates more open). The flat-plane V8's -13 % at 3200 rpm in the first run is
gone (+7 % here): it was taken with the housings over-fed (1.14). **The diesels lose in the middle**: the tractor at
1309 rpm 216 -> 66 kPa (-18.6 %), the pickup at 1998 198 -> 94 kPa (-5.7 %): their compressors, matched now to the
ratio their builds ask (3.0), are larger, and at mid speed sit on the surge line where the surge guard opens the vanes;
to D2 (the lead's condition 3). The rotary's pair at 7200 is unchanged (36 -> 40 kPa): its own fault.
