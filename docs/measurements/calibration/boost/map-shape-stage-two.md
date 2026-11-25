# The fitted map, stage two: the engines on it, bracketed by the two heavy-duty wheels (25 Nov, local)

## The variants

All on n2-b-local's tree: (b), the whole-flow nozzle, and no guard. Copies of sim/ only.

| variant | c(M) = c0 + c1 (M - 1) | phiK = (k0 + k1 (M - 1)) kAr | f | k |
|---|---|---|---|---|
| today | 0.45 | 0.40 + 0.178 | 0.35 | 0.45 |
| **all seven** | 0.608 - 0.157 | 0.349 + 0.551 | 0.652 | 0.506 |
| S400-only fit | 0.597 - 0.151 | 0.334 + 0.558 | 0.603 | 0.525 |
| S300-only fit | 0.581 - 0.077 | 0.385 + 0.501 | 0.754 | 0.462 |

The rule's own Mach lookups follow each map: the flag's tau 0.9966 c(M) 0.4 M^2, and the island's tau fraction from f.

## 1. The residuals in physical units

This is model minus published, in %.

| | peak ratio, per line | surge-end flow | right-end flow | right-end ratio | passenger surge flow | passenger choke at the last contour |
|---|---|---|---|---|---|---|
| **today**: mean / mean abs / worst | -14.4 / 14.4 / -20.2 | -11.3 / 25.6 / +62.7 | -7.5 / 12.2 / +18.0 | -30.0 / 30.0 / -52.6 | +15.4 / 17.8 / +51.7 | -1.9 / 12.6 / -33.2 |
| **all seven** | +1.0 / 3.1 / +6.0 | +2.1 / 5.7 / +20.0 | +4.1 / 9.8 / +32.8 | +3.1 / 8.3 / -20.6 | -4.8 / 9.9 / -29.1 | +2.3 / 7.5 / -14.3 |
| S400 only | -0.4 / 3.2 / -5.6 | +2.3 / 6.2 / +15.6 | +7.9 / 10.1 / +37.7 | -2.9 / 6.2 / -26.8 | -4.2 / 11.3 / -31.4 | +8.6 / 10.2 / +17.8 |
| S300 only | +1.8 / 2.9 / +16.3 | -1.1 / 6.9 / +30.4 | -5.1 / 10.8 / +21.1 | +15.0 / 15.6 / +31.7 | -0.4 / 7.9 / -21.3 | -10.0 / 10.7 / -23.6 |

**The generic (all-seven) map against a published wheel:**
- the peak ratio is within 3 % on average, 6 % at worst (today's: 14 % low);
- the surge-end flow is within 6 % on average, 20 % at worst;
- the right ends are within 8-10 % on average, 21-33 % at worst.

The right ends are placed by the unchanged efficiency law and P's contour range.

## 2. The engines

Holds of 20 s, read over the last 8 s.

| cell | today | all seven | S400-only | S300-only |
|---|---|---|---|---|
| (b)'s wheel: tractor / truck's sets / 2.0 | 76.2 / 73.9 / 36.6 | **72.4 / 69.8 / 38.4** | 71.1 / - / 38.0 | 75.3 / - / 38.0 |
| tractor 900 | 2068 | **2401** | 2402 | 2235 |
| tractor 1000 (up / down) | 2579 / 2583 | 2584 / 2586 | 2587 / 2588 | 2581 / 2584 |
| tractor 1100 | 2719 | 2723 | 2722 | 2723 |
| **tractor 1900, flat 260**: shaft, kW, kPa | 1.007, 466, 204 | **0.990**, 463, 244 | **0.999**, 464, 241 | **0.970**, 464, 245 |
| **tractor 1900, taper 202** (each map's own 260 wheel) | 1.003, 466 | **0.905, 483** | **0.919, 483** | **0.886, 482** |
| 2.0 1750 (up / down) | 278 / 251 | **247 / 244** | 251 / 246 | 245 / 241 |
| 2.0 4000: shaft, kW | 1.011, 124 | **0.814, 125** | 0.822, 125 | 0.833, 125 |
| truck's sets (target 260) 1000 / 1300 / 1900 / 2100 | 2502 / 2617 / 2147 / 1888 | 2504 / **2664** / 2138 / 1870 | - | - |
| truck's sets 2100 shaft | 0.986 | 0.963 | - | - |

Notes on the table:
- **The tractor's 202 rows hold each map's 260-sized wheel**, because the target sets the rule's design ratio. My first 202 runs let the wheel follow the target (75.1 / 73.9 / 76.5 mm). Those are set aside.

### Measured

- **The tractor's 1000 is held on every map** (2581-2588).
- **The bracket does not flip the rated answer.**
  - A flat 260 kPa at rated is not clear on any of the three maps: 0.970-0.999. That is under Woollenweber's 6 %
    margin on all.
  - The taper's 202 kPa (the build's own fuel curve at the torque peak's lambda) is clear on all three: 0.886-0.919,
    with the vanes open, at 483 kW.
  - So on the fitted map (A)'s lower target is not needed. The derived taper is. The generic default does not put the
    tractor over its limit on either published wheel.
- **The 2.0's rated limit clears** (1.011 to 0.81-0.83 on every map). Its 1750 loses 11 % on every map (278 to
  241-251), with the wheel growing 5 % (36.6 to 38.4 mm).
  - Why the wheel grows: the 2.0 climbs at low Mach, where the new c reaches the ratio sooner and the knee sits lower.
    The rule then allows a larger wheel.
  - Its 1750 was already surge-flagged on today's map (0.32-0.38). It is 0.42-0.52 now.
- **The truck's sets:** the wheel shrinks 5.5 % and 1300 gains 2 %. 2100 sits at 0.963.
- **The tractor's 900 gains 16 %** (2068 to 2401) on the all-seven and S400 maps, and 8 % on the S300.

## 3. Every shipped boosted build's validation rows (tee-validate --simulator)

"Before" is n2-b-local's tree (b, nozzle, no guard), not main. "After" is the all-seven map. 14 builds.

| build | misses before -> after | rows moving more than 3 % or changing pass/miss |
|---|---|---|
| boxer-four turbo | 4 -> 4 | |
| 2.0 common rail | 2 -> 2 | |
| flat-plane 3.9 twin turbo | 2 -> 2 | |
| inline five rally | 2 -> 2 | speed at peak torque 3510 -> 4180 rpm (pass) |
| kei triple | 3 -> 3 | peak power 54.5 -> 52.3 kW (-3.9 %, miss) |
| pickup 6.7 | 3 -> 3 | |
| rotary 13 | 5 -> 5 | peak power 180 -> 174 kW (-3.7 %, miss) |
| straight six twin turbo 3.0 | 2 -> 2 | speed at peak torque 2841 -> 2169 rpm (pass) |
| supercharged V8 6.2 | 4 -> 4 | |
| 1.0 three | 1 -> 1 | |
| **tractor** | **3 -> 4** | header gas at full load 704 -> 693 K (-1.5 %: pass -> **miss**) |
| **turbo four 2.0** | 9 -> 9 | **torque at 1500 rpm 197 -> 179 (-9 %), at 1800 239 -> 210 (-12 %)**, both misses before and after |
| twin-turbo hot vee 4.0 | 2 -> 2 | speed at peak torque 3437 -> 2796 rpm (pass) |
| V6 3.5 twin turbo pickup | 3 -> 3 | |

**Totals: 45 -> 46 misses.**

### Findings, reported and not tuned

- **The turbo four's low end falls 9-12 %.** Suspected: it states its own wheel (42.5 mm, kept), and the new knee sits
  further right at speed, so its low-speed climb meets the surge line sooner. Not measured: its flags at 1500-1800.
- **The kei and the rotary lose about 4 % of peak power.**
- **The tractor's header gas misses by 10 K.**
- **Three builds' speed of peak torque moves 600-700 rpm,** all inside their bands. Their boost comes in earlier or
  later at the plateau's edge.

## Predictions (stage one's, written before these runs), and how they fared

1. (b)'s inducers shrink 8-12 %. **Half right**: the tractor -5 %, the truck -5.5 %; **the 2.0 grows 5 %**.
2. The tractor's 1000 within 3 %. **Right** (0.2 %).
3. A flat 260 at rated under the limit at about 0.90, so (A) not needed. **Wrong**: 0.97-1.00. The taper's 202 gives
   0.89-0.92, so the derived taper is needed, and (A)'s lower target is not.
4. The 2.0's 1750 within 5 %, its 4000 under 1.0. **1750 wrong** (-11 %); **4000 right** (0.81).
5. Petrols: misses change by 2-4; the turbo four and the 1.0 three may gain low-speed surge. **Count wrong** (+1,
   45 -> 46). **The turbo four's low end did fall (9-12 %); the 1.0 three did not move.**
