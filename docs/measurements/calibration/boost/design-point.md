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

## The table (measured 22:01 UTC on the tip with the closed exhaust; before = the redline match at 1.9, after = this rule; engine dyno, settle 5 s, stepped up)

| archetype | rpm | torque before / after N m | change % | boost kPa b / a | turbine in kPa b / a | gate or vanes b / a |
|---|---|---|---|---|---|---|
| boxer-four-turbo-unequal | 2800 | 209 / 271 | +29.7 | 24 / 54 | 131 / 152 | 0.00 / 0.00 |
| boxer-four-turbo-unequal | 4550 | 397 / 392 | -1.1 | 100 / 100 | 230 / 240 | 0.19 / 0.29 |
| boxer-four-turbo-unequal | 6300 | 347 / 339 | -2.4 | 100 / 99 | 261 / 277 | 0.31 / 0.37 |
| diesel-20-common-rail | 1696 | 179 / 188 | +5.0 | 26 / 33 | 133 / 147 | 0.00 / 0.00 |
| diesel-20-common-rail | 2756 | 345 / 335 | -3.0 | 165 / 151 | 291 / 320 | 0.10 / 0.13 |
| diesel-20-common-rail | 3816 | 319 / 318 | -0.0 | 162 / 167 | 381 / 398 | 0.11 / 0.25 |
| flat-plane-v8-39-twin-turbo | 3200 | 689 / 596 | -13.4 | 130 / 130 | 147 / 152 | 0.09 / 0.09 |
| flat-plane-v8-39-twin-turbo | 5200 | 726 / 698 | -3.9 | 130 / 130 | 153 / 162 | 0.47 / 0.57 |
| flat-plane-v8-39-twin-turbo | 7200 | 658 / 636 | -3.2 | 130 / 130 | 178 / 185 | 0.62 / 0.74 |
| inline-five-turbo-rally | 2800 | 284 / 405 | +42.6 | 47 / 128 | 142 / 228 | 0.00 / 0.01 |
| inline-five-turbo-rally | 4550 | 431 / 420 | -2.7 | 129 / 129 | 227 / 247 | 0.24 / 0.35 |
| inline-five-turbo-rally | 6300 | 389 / 375 | -3.6 | 130 / 130 | 269 / 281 | 0.32 / 0.47 |
| kei-660-triple-turbo | 3200 | 92 / 95 | +3.4 | 60 / 65 | 161 / 164 | 0.04 / 0.19 |
| kei-660-triple-turbo | 5200 | 78 / 77 | -1.0 | 38 / 38 | 159 / 163 | 0.39 / 0.55 |
| kei-660-triple-turbo | 7200 | 62 / 62 | +0.4 | 12 / 14 | 151 / 158 | 0.83 / 0.96 |
| pickup-diesel-67-inline-six | 1230 | 619 / 615 | -0.6 | 25 / 27 | 129 / 139 | 0.00 / 0.00 |
| pickup-diesel-67-inline-six | 1998 | 1023 / 1039 | +1.5 | 214 / 230 | 419 / 406 | 0.00 / 0.00 |
| pickup-diesel-67-inline-six | 2767 | 998 / 977 | -2.1 | 163 / 236 | 370 / 518 | 0.09 / 0.08 |
| rotary-twin-13 | 3200 | 132 / 221 | +67.0 | 20 / 70 | 127 / 171 | 0.00 / 0.05 |
| rotary-twin-13 | 5200 | 261 / 220 | -15.9 | 70 / 41 | 208 / 181 | 0.14 / 0.00 |
| rotary-twin-13 | 7200 | 204 / 194 | -4.6 | 35 / 39 | 189 / 225 | 0.00 / 0.00 |
| straight-six-twin-turbo-30 | 2800 | 532 / 507 | -4.7 | 100 / 101 | 146 / 147 | 0.14 / 0.24 |
| straight-six-twin-turbo-30 | 4550 | 510 / 496 | -2.7 | 100 / 100 | 153 / 156 | 0.43 / 0.55 |
| straight-six-twin-turbo-30 | 6300 | 459 / 442 | -3.7 | 100 / 100 | 167 / 173 | 0.59 / 0.74 |
| supercharged-v8-62 | 2480 | 799 / 798 | -0.2 | 54 / 54 | 101 / 101 | 0.00 / 0.00 |
| supercharged-v8-62 | 4030 | 808 / 804 | -0.4 | 60 / 59 | 101 / 101 | 0.00 / 0.00 |
| supercharged-v8-62 | 5580 | 730 / 721 | -1.3 | 76 / 74 | 101 / 101 | 0.00 / 0.00 |
| three-cylinder-10-turbo | 2600 | 166 / 176 | +5.9 | 83 / 100 | 188 / 209 | 0.00 / 0.09 |
| three-cylinder-10-turbo | 4225 | 170 / 168 | -1.2 | 100 / 100 | 219 / 232 | 0.27 / 0.36 |
| three-cylinder-10-turbo | 5850 | 148 / 147 | -0.9 | 101 / 99 | 246 / 260 | 0.39 / 0.46 |
| tractor-15-inline-six | 806 | 828 / 834 | +0.7 | 11 / 14 | 117 / 126 | 0.00 / 0.00 |
| tractor-15-inline-six | 1309 | 2280 / 2293 | +0.5 | 223 / 138 | 460 / 326 | 0.03 / 0.00 |
| tractor-15-inline-six | 1813 | 2305 / 2188 | -5.1 | 163 / 288 | 372 / 607 | 0.13 / 0.00 |
| turbo-four-20-gdi | 2720 | 344 / 331 | -3.8 | 119 / 119 | 162 / 166 | 0.07 / 0.13 |
| turbo-four-20-gdi | 4420 | 372 / 364 | -2.3 | 119 / 121 | 178 / 186 | 0.40 / 0.56 |
| turbo-four-20-gdi | 6120 | 276 / 271 | -1.8 | 83 / 82 | 174 / 179 | 0.60 / 0.67 |
| twin-turbo-hot-vee-v8-40 | 2720 | 699 / 651 | -6.8 | 119 / 121 | 153 / 158 | 0.06 / 0.12 |
| twin-turbo-hot-vee-v8-40 | 4420 | 704 / 713 | +1.3 | 117 / 120 | 162 / 168 | 0.39 / 0.54 |
| twin-turbo-hot-vee-v8-40 | 6120 | 671 / 652 | -2.8 | 120 / 120 | 186 / 197 | 0.56 / 0.69 |
| v6-35-twin-turbo-pickup | 2400 | 544 / 615 | +13.0 | 69 / 101 | 173 / 205 | 0.00 / 0.07 |
| v6-35-twin-turbo-pickup | 3900 | 591 / 581 | -1.7 | 98 / 101 | 219 / 229 | 0.21 / 0.35 |
| v6-35-twin-turbo-pickup | 5400 | 531 / 523 | -1.6 | 100 / 100 | 240 / 259 | 0.41 / 0.50 |

**Read**: the smaller turbines spool the petrol engines whose turbines were oversized for their low end (the boxer at
2800 rpm 24 -> 54 kPa, +30 %; the five 47 -> 128 kPa, +43 %; the V6 pickup +13 %; the triple +6 %) and cost 2 to 4 %
at the top (the turbine's inlet 5 to 15 kPa higher: more pumping), the gates sitting more open. The diesels move both
ways (the tractor at 1309 rpm 223 -> 138 kPa, at 1813 163 -> 288 kPa: its 900 K design turbine is smaller and its
VGT's loop meets it differently; the pickup's top end 163 -> 236 kPa). The flat-plane V8 at 3200 rpm loses 13 % at
the same boost (not yet explained: its knock retard is the suspect). **The rotary's pair is not cured**: at 3200 rpm
the single now spools (20 -> 70 kPa, +67 %), but at 5200 the pair joins earlier and makes 41 kPa, and at 7200 39 kPa
against 70: the pair's second compressor barely flows (0.027 kg/s against the first's 0.146 at one shaft speed and
ratio): its delivery through the check valve into the charge pipe, not the turbine's size, is the suspect. Not
adopted: the lead decides.
