# The vane surge guard's candidates on (b)'s branch (25 Nov, local; n2-b-local f492c0cf, copies of sim/)

## The three variants

- **(0) The guard as it is.**
- **(i) The guard's surge test on the cycle's mean flow.** The compressor's flow is averaged over one engine
  cycle: an exponential mean with time constant 4 pi / omega, at the sample rate. The flag's telemetry is
  unchanged.
- **(ii) The opening test.**
  - A 0.3 s mean of the guard's surge test is judged every 0.5 s.
  - If the guard opened by 0.02 or more and that mean did not fall by 0.05 or more, it gives back at 0.5 a second
    for 0.5 s.

## Method

- (b)'s wheels throughout; the torque peaks stated where P states them (pickup 1700, the 2.0 1750).
- Holds of 30 s, read as the last 10 s means. Cells give N m / kPa (the range of the 1 s means), the vanes, and the
  surge-flag mean.
- The truck uses GateTest's sets, approached from 800 and from 2000.
- Pulls at 25 rpm a 0.5 s.
- Predictions are in n5/predictions.md, written before the builds.

## Results

| cell | (0) as is | (i) cycle mean | (ii) opening test |
|---|---|---|---|
| tractor 900 | 1403 / 31, v1.00 f0.50 | 1404 / 31, f0.50 | 1526 / 44 (33-80), v0.88 |
| tractor 1000 | 2579 / 225 | 2581 / 226 | 2578 / 226 |
| tractor take-up 1000 | 4.5 s | 4.5 s | 4.5 s |
| tractor pull, 1000 / 1200 | 1392 / 2648 | 1388 / 2677 | **1944 / 2734** |
| truck 1000, from below / from above | 845 / 881 | 819 / 839 | 889 / 870 |
| truck 1100 | 1190 / 2112 | 1165 / 1735 | 1080 / 1051 |
| truck 1300 | 2160 / 2198 | **1867 / 1911** | 2086 (45-173) / 2178 (95-181) |
| truck 1900 | 1705 / 1705 | 1705 / 1597 | **1607 (184-209) / 1548** |
| truck pull, 1600 / 1800 | 1803 / 1772 | 1634 / 1639 | **812 / 1016** |
| pickup 1300 | 576 / 21, v1.00 f0.54 | 575 / 21, f0.54 | **809 / 77**, v0.61 |
| pickup 1700 | 996 | 997 | 997 |
| pickup take-up 1700 | 3.5 s | 3.5 s | 3.5 s |
| pickup pull, 1400 / 1600 | 573 / 714 | 570 / 711 | **772 / 978** |
| 2.0 1500 | 151 / 14 | 151 / 14 | 185 / 42 (30-60) |
| 2.0 1750 | 158 / 16, v1.00 f0.35 | 177 / 29 (17-69), f0.37 | **252 / 91**, v0.56 |
| 2.0 2000 / 4000 | 329 / 282 os | 329 / 282 os | 329 / 282 os |
| 2.0 take-up 1750 | 0.5 s to 15 kPa | 0.5 s to 16 | 4.5 s to 87 |
| 2.0 pull, 1800 | 157 | 279 | 191 |

"os" means the over-speed flag is set.

## Reading

### (i) is refuted

- At the pinned cells the flag on the cycle's mean flow still reads 0.50 / 0.54 / 0.37, the same as the
  instantaneous one. The compressor's mean flow is really left of the line there; it is not a dip inside a pulse.
- So it frees nothing, except a pulsing gain at the 2.0's 1750.
- It costs the truck: 1300 is 1867 against 2160 from below, and 1100 from above is 1735 against 2112. The mean flow
  lags the surge's onset, so the guard opens later.

### (ii) frees the pinned cells

- The pickup at 1300 and the 2.0 at 1750 reach their guard-off numbers.
- The tractor's 900 is partly freed, but it pulses.
- The pulls improve under 1400 on the tractor and the pickup.

### (ii) costs the truck where the guard is the cure

- Its pull falls into the low state: 812 against 1803 at 1600.
- 1900 hunts, 1607 (184-209) against 1705, and from above it reads 1548.
- 1300 from below is slower and wider (45-173).

That is the hunting the lead warned of, found where we had not looked: the truck's climb.

**Measured: neither candidate is right on all four builds.**

Suspected, not measured: the truck and the others differ in what opening does. On the truck it lowers the surge
share; elsewhere it does not. A judgement every 0.5 s on a noisy share cannot tell the two cases apart during a
climb, when the share falls anyway as the flow grows.

## Predictions, and how they fared

1. (i) frees the pinned cells and keeps the truck. **Wrong on both.**
2. (ii) frees them less fully and hunts somewhere; the truck later from below. **Freeing: better than predicted.
   Hunting: right, on the truck's 1900 and its pull. The pull's collapse: not predicted.**
3. Neither touches the tractor from 1100, the 2.0 from 2000, the pickup's 1700, or the truck's 1900. **Wrong for
   the truck's 1900, under both.**
4. Take-ups: (i) the same; (ii) up to 1 s slower. **Right for (i). (ii) is the same on the tractor and pickup; the
   2.0's take-up is 4.5 s, but to 87 kPa against 15.**

Listen for (nobody here can hear):
- **The pickup at 1300 rpm, full load:** under (ii), a boosted hiss with a surge chuff, against a flat note today.
- **The truck at 1900 under (ii):** a slow wander in the turbo's pitch, the hunt.
