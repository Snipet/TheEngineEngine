# Item 3: the tractor and the 2.0 at rated, the two levers (25 Nov, local; on n2-b-local 8f0bc2c2's tree)

## The variants

- **base:** (b), the whole-flow nozzle, no guard.
- **S15:** a ported shroud as cited. Holset US 6,540,480 B2 gives "up to 15% additional surge margin" above a pressure
  ratio of 3 (P's sources, 2026-11-25-0032-judgement-sources.md). It is applied as the knee (the flag and the island)
  x 1/1.15 at tip Mach 1.434 and above (the knee's line at PR 3), in the map and in the rule.
- **S15all:** the same at every speed. That is beyond the citation, so it is an upper bound only.
- **floor:** F's 1.15 at rated as the wheel's floor. It binds for both builds, so their wheels are as before (b).

Holds are 20 s, read over the last 8 s, from below and from above.

## Results

| variant | wheel | tractor 900 | tractor 1000 (up / down) | tractor 1100 | tractor 1900: shaft, flags, boost |
|---|---|---|---|---|---|
| base | 76.2 | 2068 | 2579 / 2583 | 2719 | **1.007**, 2.15, 204 |
| S15 | 76.2 (unchanged) | 2068 | 2574 / 2583 | 2722 | 1.011, 2.30, 202 |
| S15all | 81.6 | 1946 | 2563 / 2583 | 2718 | **1.014**, 2.66, 227 |
| floor | 83.9 | 1742 | **1976 / 1988** | 2319 / 2384 | **1.009**, 2.78, 240 |

| variant | wheel | 2.0 1500 | 2.0 1750 (up / down) | 2.0 2000 | 2.0 4000: shaft, flags, kW |
|---|---|---|---|---|---|
| base | 36.6 | 211 | 278 / 251 | 321 | **1.011**, 1.93, 124 |
| S15 | 36.6 (unchanged) | 211 | 278 / 251 | 321 | 1.012, 2.02, 123 |
| S15all | 39.2 | 203 | **243 / 236** | 317 | **0.948**, 0.00, 124 |
| floor | 39.4 | 192 | **222 / 221** | 296 | **0.932**, 0.00, 125 |

## Reading

### Measured

- **The shroud as cited changes nothing.** The rule's binding ratio on the climb at the torque peak lies below 3, so
  the rule picks the same wheels. The patent says nothing below 3.
- **Extended to every speed (beyond the citation),** the shroud lets the rule pick larger wheels. The 2.0 then clears
  its rated limit (0.948, no flags) at a cost of 13 % at 1750 (278 to 243). The tractor stays at its limit
  (1.014, choke flag).
- **The floor clears the 2.0's rated limit (0.932)** at a cost of 20 % at its 1750. The tractor loses 23 % at its
  1000 (2579 to 1976) and **still sits at its limit at rated** (1.009, 240 kPa).

### The tractor: no wheel takes it off its limit at rated

- omegaMax is a tip-speed limit (560 m/s at the exducer). The ratio a wheel can make at that tip speed does not grow
  with the wheel's size; only the wheel's distance from choke changes.
- The tractor's loop asks 260 kPa at rated:
  - the 76.2 mm wheel gives 204 kPa at its limit, near choke;
  - the 81.6 mm wheel gives 227 kPa at its limit;
  - the 83.9 mm wheel gives 240 kPa at its limit.
- With the vanes free, the loop drives the shaft to the over-speed relief in every case.
- Only a lower rated target clears it. 160-180 kPa gave 0.979-0.984 (healthy-match.md). The derived taper (202 at
  1900) is not low enough: it holds the torque peak's lambda, and the engine at rated already runs leaner than that.

### Suspected, not measured

- A rated target taken from the wheel's own map would leave the tractor clear without costing the torque peak: the
  ratio at the tip limit at rated flow, less a margin in tip speed.
- It is static by speed, computed at the match, so it cannot latch as the flow-based cap did.
- It would need its margin stated; I have no source for a maker's tip-speed margin. Woollenweber's rated point "at only
  94% of the maximum speed" (US 5,025,629) is one patent's example.

## Predictions (n11/predictions.md), and how they fared

1. S15: the tractor's wheel to about 80 and its rated shaft to about 0.95; the 2.0 unchanged. **Wrong for the
   tractor**: no change, because the binding ratio is below 3. **Right for the 2.0.**
2. S15all: the 2.0 at about 38.5 mm, its rated shaft about 0.97, its 1750 within 3 %. **Wheel and shaft about right
   (39.2, 0.948). The 1750 wrong**: -13 %.
3. Floor: rated shafts 0.93-0.95, peaks lost. **The 2.0 right (0.932). The tractor's rated wrong**: still 1.009. The
   peaks' losses as predicted (1976; 222).

Listen for (nobody here can hear): the tractor at 1900 full load on every variant. The relief chuffs at the shaft
limit with the turbine's whine at its highest pitch. At a 160-180 kPa rated target, the whine drops and the chuff
stops.
