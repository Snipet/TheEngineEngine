# The boosted ledger across D's gamma change (Gatowski, 3990ee47 and a845b07f)

The lead's 20:04 ask: regenerate the boosted ledger, say which numbers moved more than 3 %, re-check the tests that
hold measured values, re-tune nothing. "before" is the simulator at 3990ee47's parent, "after" at a845b07f (the lean
term in): nothing else differs. Each boosted archetype on the engine dyno (tee-calib-boost, MODE_DYNO_ENGINE, warm-up
8 s, then each speed 5 s and measured over 1 s, stepped up) at 0.4, 0.65 and 0.9 of its redline. Both trees predate the
test cell's cooler (10345ed): the cooler soaks in dead air in both alike.

| archetype | rpm | torque before / after N m | change % | boost kPa b / a | knock retard b / a | turbine in kPa b / a |
|---|---|---|---|---|---|---|
| boxer-four-turbo-unequal | 2800 | 381 / 364 | -4.4 | 100 / 100 | 15.0 / 14.8 | 202 / 202 |
| boxer-four-turbo-unequal | 4550 | 398 / 376 | -5.4 | 100 / 100 | 7.9 / 3.6 | 237 / 237 |
| boxer-four-turbo-unequal | 6300 | 354 / 329 | -7.2 | 101 / 101 | 0.0 / 0.0 | 269 / 271 |
| diesel-20-common-rail | 1696 | 189 / 187 | -1.1 | 36 / 37 | 0.0 / 0.0 | 144 / 147 |
| diesel-20-common-rail | 2756 | 333 / 323 | -2.8 | 151 / 150 | 0.0 / 0.0 | 293 / 294 |
| diesel-20-common-rail | 3816 | 318 / 308 | -2.9 | 169 / 169 | 0.0 / 0.0 | 393 / 393 |
| flat-plane-v8-39-twin-turbo | 3200 | 709 / 666 | -6.1 | 130 / 130 | 15.0 / 14.9 | 150 / 150 |
| flat-plane-v8-39-twin-turbo | 5200 | 743 / 711 | -4.4 | 130 / 130 | 8.5 / 4.3 | 160 / 159 |
| flat-plane-v8-39-twin-turbo | 7200 | 627 / 589 | -6.1 | 130 / 130 | 3.1 / 0.0 | 169 / 168 |
| inline-five-turbo-rally | 2800 | 419 / 401 | -4.3 | 131 / 130 | 12.8 / 9.9 | 223 / 220 |
| inline-five-turbo-rally | 4550 | 438 / 419 | -4.5 | 130 / 130 | 9.7 / 5.8 | 238 / 240 |
| inline-five-turbo-rally | 6300 | 382 / 359 | -6.1 | 130 / 130 | 3.8 / 0.0 | 268 / 269 |
| kei-660-triple-turbo | 3200 | 98 / 93 | -5.0 | 65 / 65 | 5.1 / 0.7 | 166 / 166 |
| kei-660-triple-turbo | 5200 | 82 / 77 | -6.3 | 38 / 38 | 0.0 / 0.0 | 167 / 168 |
| kei-660-triple-turbo | 7200 | 63 / 59 | -6.9 | 11 / 11 | 0.0 / 0.0 | 156 / 157 |
| pickup-diesel-67-inline-six | 1230 | 635 / 630 | -0.8 | 33 / 35 | 0.0 / 0.0 | 137 / 139 |
| pickup-diesel-67-inline-six | 1998 | 1031 / 1019 | -1.2 | 222 / 225 | 0.0 / 0.0 | 360 / 363 |
| pickup-diesel-67-inline-six | 2767 | 971 / 936 | -3.6 | 187 / 187 | 0.0 / 0.0 | 405 / 417 |
| rotary-twin-13 | 3200 | 231 / 221 | -4.5 | 70 / 70 | 15.0 / 15.0 | 169 / 169 |
| rotary-twin-13 | 5200 | 265 / 225 | -15.0 | 70 / 48 | 15.0 / 12.1 | 216 / 190 |
| rotary-twin-13 | 7200 | 254 / 243 | -4.5 | 70 / 70 | 14.9 / 7.7 | 260 / 264 |
| straight-six-twin-turbo-30 | 2800 | 498 / 488 | -2.1 | 100 / 100 | 14.9 / 14.3 | 151 / 150 |
| straight-six-twin-turbo-30 | 4550 | 502 / 478 | -4.9 | 100 / 100 | 9.3 / 5.6 | 155 / 156 |
| straight-six-twin-turbo-30 | 6300 | 467 / 441 | -5.7 | 100 / 100 | 5.1 / 1.2 | 171 / 171 |
| supercharged-v8-62 | 2480 | 834 / 790 | -5.3 | 58 / 57 | 7.5 / 2.9 | 101 / 101 |
| supercharged-v8-62 | 4030 | 857 / 801 | -6.6 | 66 / 65 | 4.3 / 0.0 | 101 / 101 |
| supercharged-v8-62 | 5580 | 775 / 717 | -7.5 | 84 / 83 | 4.4 / 0.0 | 101 / 101 |
| three-cylinder-10-turbo | 2600 | 176 / 167 | -4.9 | 100 / 100 | 15.0 / 13.7 | 216 / 216 |
| three-cylinder-10-turbo | 4225 | 171 / 164 | -4.3 | 100 / 100 | 10.0 / 6.9 | 230 / 228 |
| three-cylinder-10-turbo | 5850 | 152 / 143 | -5.4 | 100 / 100 | 5.5 / 1.2 | 256 / 256 |
| tractor-15-inline-six | 806 | 796 / 788 | -1.0 | 12 / 11 | 0.0 / 0.0 | 120 / 119 |
| tractor-15-inline-six | 1309 | 2203 / 2161 | -1.9 | 249 / 247 | 0.0 / 0.0 | 485 / 491 |
| tractor-15-inline-six | 1813 | 2170 / 2127 | -2.0 | 218 / 217 | 0.0 / 0.0 | 448 / 442 |
| turbo-four-20-gdi | 2720 | 361 / 341 | -5.7 | 120 / 119 | 15.0 / 14.7 | 170 / 171 |
| turbo-four-20-gdi | 4420 | 372 / 356 | -4.5 | 120 / 120 | 10.2 / 6.2 | 188 / 187 |
| turbo-four-20-gdi | 6120 | 282 / 264 | -6.5 | 82 / 82 | 0.1 / 0.0 | 181 / 181 |
| twin-turbo-hot-vee-v8-40 | 2720 | 713 / 673 | -5.7 | 119 / 120 | 15.0 / 15.0 | 157 / 158 |
| twin-turbo-hot-vee-v8-40 | 4420 | 724 / 683 | -5.6 | 121 / 121 | 11.2 / 7.5 | 166 / 167 |
| twin-turbo-hot-vee-v8-40 | 6120 | 640 / 609 | -4.9 | 120 / 120 | 8.7 / 4.5 | 182 / 183 |
| v6-35-twin-turbo-pickup | 2400 | 606 / 569 | -6.0 | 100 / 102 | 15.0 / 15.0 | 209 / 212 |
| v6-35-twin-turbo-pickup | 3900 | 575 / 546 | -5.0 | 100 / 100 | 14.2 / 11.3 | 228 / 228 |
| v6-35-twin-turbo-pickup | 5400 | 515 / 492 | -4.5 | 100 / 100 | 10.4 / 6.4 | 250 / 250 |

**Moved more than 3 %**: torque, on every petrol engine and the rotary (-2 to -7.5 %; the six's low point -2.1 %), and
the larger diesels' rated point (pickup -3.6 % at 2767). The diesels elsewhere -0.8 to -2.9 %. **Not moved**: the
boost everywhere the loop holds a target (within 1 kPa) and the turbines' inlet (within 3 %, the pickup at rated 417
against 405): the turbine's energy changed little for these matches. **Knock retard fell** wherever it was off its
ceiling (the four at 4420 rpm 10.2 -> 6.2 degrees, the hot-vee 11.2 -> 7.5): less work in expansion is a cooler end
gas earlier in it. Where it was pinned at 15 it stays near 15 at the lowest speed (the four, the boxer, the flat-plane,
the hot-vee, the V6, the rotary): that is item 2 of the lead's 19:19, separate.

**One exception, a cause of its own**: the rotary at 5200 rpm read 70 -> 48 kPa in this run. On today's tip it holds
70.0 kPa reached from 3200 or alone, and 49.0 kPa when reached from 7200: the sequential pair's hysteresis keeps the
second turbo in at 5200 (shafts 10876 and 10572 rad/s, the gate shut) and the two share a flow neither can make the
target on. Real 13B-REW pairs drop the secondary on the way down too, but they hold the target with both in. Open:
the sequential pair's control at 5200 rpm, not gamma.

**Tests holding measured values**, on the tip with gamma in: calib-boost (the matched gate's targets, back pressure,
one state, the over-boost cut, the twin six, the VGT's states, the warnings) and boost (33) pass; D2's VGT "at least
the single's boost" misses by 0.3 to 0.5 % (209.2 against 209.7 kPa) on the tip with or without my changes: D2 is told.
