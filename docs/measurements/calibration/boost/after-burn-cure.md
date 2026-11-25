# The low-speed cells again on D2's burn cure (tip dbd64a5c, 25 Nov, local only)

D2's 2cf55103 gates the ECU's idle spark trim off a metered diesel at an open pedal. Before it, the trim had cut a
governed diesel's charged heat below ECU idle + 450 rpm. The bands, as D2 gave them: tractor 1150, truck 1047,
pickup 1200, the 2.0 1250.

Inside the bands this time: the tractor's 900, 1000 and 1100 cells and the truck's 1000. No pickup or 2.0 cell sat
inside its band, and theirs read the same as before within 1 %.

Method: holds of 30 s from 800 rpm, read as the last 10 s means. Each cell gives torque N m / boost kPa (the range
of the 1 s means over the last 21 s), the vanes, the surge-flag mean, and p3/p2. Guard off means the vane surge
guard's opening gain is 0 in a local copy. Predictions are in the scratchpad (n3/predictions.md), and their results
are listed at the end.

## Tractor

| wheel | guard | 900 | 1000 | 1100 | 1300 | 1900 | t90 @1100 |
|---|---|---|---|---|---|---|---|
| 83.2 (today) | on | 1324 / 24, v1.00 f0.56 | 1509 / 40, v1.00 f0.51 | 1911 / 76, v1.00 | 2760 / 257 | 2207 / 260 os | 0.5 s |
| 83.2 | off | 1772 / 75, v0.65 f0.54 | 2036 / 97, v0.73 | 2423 / 132 | 2760 / 257 | 2205 / 260 os | 3.0 s |
| 76.8 | on | 1403 / 31, v1.00 f0.49 | **2580 / 225 (221-236)**, v0.67 f0 | 2719 / 251 | 2759 / 270 | 2257 / 222 os | 3.5 s |
| 76.8 | off | 2080 / 106, v0.65 f0.45 | 2581 / 226 (222-227) | 2718 / 251 | 2769 / 268 | 2256 / 222 os | 4.0 s |
| 80.6 | on | 1353 / 27, v1.00 | 1586 / 46, v1.00 f0.49 | 2717 / 237 | 2756 / 265 | 2230 / 243 os | 4.0 s |
| 80.6 | off | 1885 / 86, v0.65 | 2244 / 118, v0.73 | 2723 / 237 | 2757 / 265 | 2227 / 243 os | 4.5 s |

"os" means the over-speed flag is set.

The cure's effect on the 76.8 mm wheel at 1000, guard on: 775 N m pulsing 17-75 kPa before, **2580 N m steady at
225 kPa** now. The 1 Hz pulse is gone, and the guard is idle there (surge flag 0).

The 83.2 wheel's t90 of 0.5 s is the take-up to a low plateau, 76 kPa. It does not show a faster spool.

## Truck (GateTest's sets)

| wheel | guard | 1000 | 1100 | 1300 | 1900 |
|---|---|---|---|---|---|
| today | on | 730 / 13 | 839 / 14 | 1522 / 70 (38-71) | 1997 / 209 |
| today | off | 385 / 13, v0.30 | 381 / 16, v0.30 f0.92 | 392 / 26 | 1193 / 128 |
| 76.2 | on | 827 / 13 | 1021 / 20, v1.00 f0.77 | **2205 / 170** (83-173), v0.91 | 1742 / 209 |
| 76.2 | off | 395 / 14, v0.30 | 393 / 19 | 408 / 31, v0.32 f0.72 | 1452 / 192 |

The truck still needs the guard: without it, it falls into the second state on both wheels.

## Pickup (wheel unchanged) and the 2.0

| build | guard | cells |
|---|---|---|
| pickup | on | 1300: 576 / 21, v1.00 f0.54; 1700: 996 / 198 |
| pickup | off | 1300: 810 / 77, v0.61; 1700: 998 / 198 |
| 2.0, 39.1 mm (today) | on | 1500: 144 / 8; 1750: 155 / 14; 2000: 176 / 27; 4000: 284 / 166 |
| 2.0, 39.1 mm | off | 190 / 48; 223 / 69; 277 / 103; 285 / 166 |
| 2.0, 36.9 mm | on | 151 / 14; 158 / 16; **329 / 140**; 281 / 171 os |
| 2.0, 36.9 mm | off | 207 / 61; 253 / 91; 329 / 140; 282 / 171 os |

## A vane floor as the guard's replacement: refuted (diagnostic, guard off, spec vaneMin)

| case | result |
|---|---|
| truck 76.2, floor 0.45 | 1300: 627 / 39 |
| truck 76.2, floor 0.55 | 1300: 792 / 46, surge flag 0.54 |
| truck, today's wheel, floor 0.45 / 0.55 | 1900: 1193 / 128 at both floors |
| the 2.0 at 1500, floor 0.55 | 182 / 38, against 207 / 61 with no floor |
| tractor and pickup | unchanged by either floor |

Measured: the truck leaves its low state only with the vanes near open (0.91, as the guard puts them), and a floor
costs the 2.0. So a fixed floor is neither the truck's cure nor harmless.

## The turbo's ledger at the tractor's 1000 cell, 76.8 mm

These are 1 s means from the instrumented copy. Guard on and off read the same.

| term | before the cure (guard off) | now |
|---|---|---|
| torque / boost | 947 N m / 73 kPa | 2581 N m / 226 kPa |
| T3 | 673 K | **817 K** |
| eta_c | 0.57 | 0.64 |
| eta_t | 0.66 | 0.68 |
| vanes | 0.63 | 0.67 |
| p3/p2 | 1.40 (on the limit) | 1.40 (on the limit) |
| turbine / compressor power | 19.9 / 19.6 kW | 69.9 / 69.4 kW |
| lambda | 1.30 (smoke) | 1.65 (on the fuel curve, 16.1 g/s) |
| fresh-air VE | 0.836 | **0.879** |
| phi/phiK (from inside the model) | 0.877 | **1.032** |
| pulse factor (mean of phi over the ratio of means) | 0.951 | **0.998** |
| tip Mach | 1.005 | 1.496 |
| PR | 1.75 | 3.28 |

## The rule against the model now

Evaluated at the engine's PR 3.28, the rule gives M 1.501, knee 0.4891, mc 0.740 and a flow of 0.391 kg/s.

- **The map:** the model agrees within 0.3 %.
- **Fresh air:** the model passes 0.374, a factor of 0.957. The VE (0.879 against 0.9) is most of it, and p2 (327
  against 332 kPa) the rest.
- **The pulse:** a factor of 0.998.
- **The result:** phi/knee is 1.032 in the model against the rule's 1.079.

The rule's margin at its binding ratio, 1.90 on the climb, is 0.99 by the rule. In the model's terms it is about
0.95: it keeps half of its 10 %. The pulse's 5 % appeared only in the starved, surging state; at the boosted state
it is under 0.3 %.

## Predictions, and how they fared

1. 83.2 low cells well up, 1300/1900 unchanged. **Right** (1000: 638 to 1509).
2. T3 820-900 K, p3/p2 off the limit, eta_c unchanged. **T3 right (817); p3/p2 wrong**: still on 1.40, the vanes
   holding it; **eta_c wrong**: it rose to 0.64.
3. The pulse at 1000 on 76.8 is gone. **Right.**
4. The guard is idle on the tractor, guard on and off within 3 % at every cell. **Wrong**: at 900 on every wheel
   and at 1000 on 83.2 and 80.6, the guard pins the vanes at 1.00, costing 25-40 %.
5. The truck still collapses without the guard. **Right.**
6. Pickup 1300 up. **Wrong**: it was out of the band; unchanged.
7. The 2.0's low cells up. **Wrong**: out of the band; unchanged. I misread the band.
8. VE toward 0.87, pulse stays 0.95. **VE right (0.879); pulse wrong (0.998).**
9. Take-up on 83.2 under 3 s. **Right in number** (0.5 s), but it reaches only a 76 kPa plateau.
10. A floor frees the truck and leaves the rest. **Wrong on the truck; wrong on the 2.0 at 0.55.**
11. Ledger: T3 850-900, eta_c 0.65, phi/knee about 1.0. **Close**: 817, 0.64, 1.03.

Listen for (nobody here can hear): the tractor at 1000 rpm full load on 76.8 mm. It should be a steady,
strong turbo note with no 1 Hz cycle. At 900 rpm, guard on, expect a flat, unboosted sound; guard off, a boosted
hiss with the compressor's surge chuff under it.
