# The tractor at rated, read off the model's map (25 Nov, local; the lead's three questions of 13:39)

## What the map's formulas give

These are compressor.fm's own formulas, computed, not a build:
- the tip limit's Mach is 560 / a = 1.632;
- the knee's rise is tau_K = 0.45 u^2 / (cp T);
- the line runs from 0.88 of tau_K at shut-off to 0.35 of it at choke, by the smooth step and the ellipse;
- the island is 0.61 of the way from the knee to choke;
- PR = (1 + tau)^3.5.

The top line (the tip limit) gives:

| point | PR |
|---|---|
| knee | **3.93** |
| island | **3.36** |
| choke | 1.72 |

The lead's Euler estimate, "well above 4", is the knee's reach, near 3.9 here. The island at the tip limit is 3.36.

### 1. The wheel that puts the tractor's rated flow on its island

Rated flow is computed from VE 0.88, eta_c 0.70 and icEffect 0.75 at 1900 rpm.

| rated boost | rated flow | nearest the island | shaft | wheel |
|---|---|---|---|---|
| 260 kPa (PR 3.57) | 0.773 kg/s | phi 0.69, left of the island (0.81), Mach 1.598 | **0.979** | 92.5 mm |
| the taper's 202 (PR 2.99) | 0.664 kg/s | phi 0.804, on the island, Mach 1.538 | **0.943** | **80.4 mm** |

**260 kPa at rated cannot sit clear of the limit on any wheel this map has.** A ratio of 3.57 at that flow needs a
line within 2 % of the tip limit; the map's island stays below 3.57 until Mach 1.68, past the limit.

### 2. The map's width as the model has it

| ratio | the surge flag's flow | choke-side flow at the tip limit | width |
|---|---|---|---|
| 2.6 | 0.357 of the full choke | 0.943 | **2.64** |
| 2.0 | 0.292 | 0.993 | **3.40** |

- That is wider than the 1.7 to 1.9 a 1000-to-1900 heavy diesel needs at like boost. **The width is not the fault.**
- What limits rated is the top line's ratio near choke at the tip limit, against a flat 260 kPa asked there.
- Suspected, not measured: a published heavy-duty map's top line may reach higher on its island than 3.36 at
  560 m/s. That is for P's published map to say.

### 3. The 80.4 mm wheel, held (base: (b), the whole-flow nozzle, no guard)

| cell | 76.2 mm ((b)) | 80.4 mm |
|---|---|---|
| 900, up / down | 2068 / 2059 | 1859 / 1852 |
| **1000, up / down** | **2579 / 2583** | **2166 / 2180** (110 kPa, surge flag 0.43) |
| 1100 | 2719 / 2721 | 2718 / 2719 |
| 1900, flat 260 | 2341 kW 466, 1.007, choke flag | 2335 kW 465, 1.014, choke flag |
| 1900, target 202 | 2340 kW 466, 1.003 | **2421 kW 482, 0.963, no flags** |

### Measured

- The map's wheel clears rated, at 0.963 and +3.4 % power, **with the taper's 202 kPa**. It costs the makers' 1000:
  2580 to 2170 (-16 %), a hole with the compressor at its flag.
- On (b)'s 76.2 mm the same taper leaves the shaft at 1.003.
- **The two ends cannot both be met on this map with one wheel:** 1000 at its plateau, and rated at 202 under the
  limit.
- The rated end on its own needs a target below the taper: 160-180 kPa on 76.2 mm gave 0.98.

## Predictions (n12/predictions.md), and how they fared

1. The top line's knee about 3.9, its island 3.3-3.4; 260 kPa on no island under the limit; 202 kPa on a wheel of
   about 82-84 mm at about 0.90. **Right on the line and on 260. The wheel and its shaft about right but high**: 80.4
   mm at 0.943 by the map, 0.963 measured.
2. Width about 2.5 at 2.6 and about 3 at 2.0, so not the fault. **Right** (2.64, 3.40).
3. The 80.4 mm wheel's 1000 about 2350, 900 about 1900, 1100 about 2650; rated with 202 about 0.95. **1000 wrong**
   (2170, a hole); 900 about right (1859); 1100 right (2718); rated right (0.963).
