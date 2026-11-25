# The truck's low state is its fixture's breathing, not the compressor (25 Nov, local; n2-b-local + copies)

## The diagnostic

- **The fixture:** the truck's GateTest sets (truck-15l-six.json; D2's fixture, unchanged in the repo). In a scratch
  copy only, its `valve` section is replaced by the tractor archetype's (the Lab valvetrain, 232 deg at the seat,
  112 deg centres). The original is the reference cam dial at `cam` 0.9999.
- **The wheel:** (b)'s, 74.7 mm.
- **The guard:** opening gain 0 for "off".
- **The holds:** 30 s, from 800 (up) and from 2000 (down).
- **Fresh-air VE:** fuel x lambda x 14.5 over the displacement's charge at the manifold's pressure and throttle
  temperature.
- **Predictions:** n6/predictions.md, written after one named smoke cell.

## Results

Cells are N m / kPa (the 1 s means' range), the vanes, and p3/p2.

| case | 1000 | 1100 | 1300 | 1900 |
|---|---|---|---|---|
| **tractor's valves, guard off, up** | **2500 / 193** (183-205), v0.61, 1.40 | 2546 / 180 (161-202) | 2493 / 156 | 2128 / 200, choke flag |
| tractor's valves, guard off, down | 2489 / 183 | 2535 / 181 | 2482 / 157 | 2128 / 200 |
| fresh VE (guard off) | 0.919 | 0.905 | 0.904 | 0.889 |
| tractor's valves, guard on, up | **2084 / 99 (63-156)**, v0.87, 1.15 | 2540 / 168 | 2501 / 158 | 2130 / 200 |
| tractor's valves, guard on, down | 2496 / 188 | 2547 / 179 | 2482 / 157 | 2128 / 200 |
| **truck's own valves, guard off, up** | 399 / 15, v0.30 | 399 / 20 | **414 / 33** | 1531 / 200 |
| truck's own valves, guard off, down | 396 / 15 | 398 / 20 | 416 / 33 | 1510 / 199 |
| fresh VE (own valves) | **0.370** | **0.361** | **0.352** | 0.509 |

Take-up at 1000 (t90):
- tractor's valves, guard off: 5.0 s to 195 kPa;
- tractor's valves, guard on: 5.0 s to 194 kPa;
- own valves, guard off: 5.5 s to 14 kPa.

Pulls at 25 rpm a 0.5 s:

| pull | 800 | 1000 | 1200 | 1400 | 1600 | 1800 | 2000 |
|---|---|---|---|---|---|---|---|
| tractor's valves, guard off | 1748 | 2465 | 2570 | 2310 | 2376 | 2197 | 1952 |
| tractor's valves, guard on | 1230 | 1369 | 2542 | 2630 | 2517 | 2287 | 1971 |
| own valves, guard off | 374 | 355 | 349 | 443 | 696 | 1043 | 1642 |

## Reading

### Measured

- **The fixture's own valves give a fresh-air VE of 0.35 to 0.37 in the low state, and 0.51 at 1900.**
  - Fuel is smoke-limited (lambda 1.30) at every one of those cells, so the torque follows the air.
- **With the tractor's valve events and no guard, the truck has no low state.**
  - From below and from above agree within 1 % at every cell.
  - VE is 0.89 to 0.92 and does not follow p3/p2. The back-pressure limit holds 1.38 to 1.43 at every boosted cell
    and VE is unchanged.
- **The guard is then not needed, and where it acts it costs.**
  - At 1000 from below it pins the vanes: 2084 / 99 kPa, pulsing 63-156, against 2500 / 193.
  - On the pull: 1369 against 2465 at 1000, and 1230 against 1748 at 800.

So the lead's reading holds on this table. The truck's second state is the fixture's breathing (the backflow it
gives at a raised p3). It is not the compressor's surge. The guard rescued it by lowering p3.

### Not measured

- Which part of the reference cam dial at 0.9999 gives the 0.35 VE (overlap, duration, or the dial's timing).
- Whether the real X15 class's events resemble the tractor's. The tractor's are themselves the archetype's
  judgement.

### Also noted

- Guard off with the tractor's valves, boost swings 10 to 20 % at 1000 and 1100 (183-205, 161-202).
- At 1900 the choke flag sets with the vanes fully open, at 200 kPa.
- These come from (b)'s 74.7 mm wheel on this fixture. They are not judged here.

## Predictions, and how they fared

1. No low state with the tractor's valves, guard off; 1000 at 1500-2500; VE 0.86-0.92 and insensitive to p3/p2.
   **Right** (1000 at 2500, at the top of the range).
2. 1900 about 2000, above today's 1705. **Right** (2128).
3. Guard on within 3 % except where it pins (1000/1100?). **Right**: it pins at 1000 from below; 1100 is within 1 %.
4. The truck's own valves, guard off: the low state, VE about 0.6 or below. **Right; VE far lower than I said**
   (0.35).
5. The pull with no collapse, above the original pull everywhere. **Right.**

Listen for (nobody here can hear): the truck with the tractor's valve events, guard off, on a full-load pull from
800 rpm. It should be a turbo coming in by 1000 and a steady, heavy note. With its own valves, the same pull stays
flat and unboosted until about 1800.
