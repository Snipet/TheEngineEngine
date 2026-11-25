# The machine's match on a healthy engine: why the shafts run at their limit firing (25 Nov, local; n2-b-local)

## Setup

- n2-b-local: (b), its charge correction, and no guard.
- Instrumented copies give the shaft as a share of omegaMax (560 m/s at the exducer's tip), phi over the knee, and
  phi over choke.
- The truck is my stand-in for D2's fixture: its sets with the tractor's valve events and no target. D2's own sets
  were asked for and have not arrived.
- Full load, 10-12 s settled, heat settled. Predictions are in n9/predictions.md.

## 1. The sweep

Cells give N m, the shaft's share, the vanes, phi/knee, phi over choke, and the flags.

| build | torque peak | rated |
|---|---|---|
| tractor 76.2 | 1000: 2579, 0.93, v0.68, 1.06, 0.52 | 1600: 2625, **1.044**, v1.00; 1900: 2261, **1.044**, v1.00, 0.89, os |
| truck-b 73.9 | 1000: 2491, 0.88 | 1900: 2129, **1.008**; 2100: 1834, **1.037**, os |
| truck-m 81.8 (matched) | 1300: 2597, 0.87 | 1900: 0.947; 2100: 1837, 0.964 |
| pickup 65.4 | 1700: 1009, 0.88, boost 200 / 200 | 2200: 0.953; 2800: 945, **1.044**, boost **241 / 200**, os |
| 2.0 36.6 | 1750: 277, 0.74 | 3500: 0.929; 4000: 283, **1.045**, boost **167 / 140**, os |

At 1.044 the shafts are held by the over-speed relief, which acts at 1.05.

## 2. Which of the three

Measured at the rated cells:

| variant | tractor 1900 | truck-b 2100 | pickup 2800 | 2.0 4000 |
|---|---|---|---|---|
| (b) as built | 2259 N m, 1.044, v1.00, p3/p2 1.59 | 1834, 1.037, 1.51 | 943, 1.044, 1.62 | 282, 1.045, 1.60 |
| rated target lowered (tractor 200 / 170; truck 180) | 2272 / 2305, 1.044 / 0.989, **boost 216 / 195 over the target, vanes 1.00** | 1831, 1.044 | | |
| open nozzle x1.15 | 2315, 1.044 | 1874, 1.031 | 971, 1.034 | 288, 1.045 |
| open nozzle x1.3 | 2338, 1.045 | 1890, 0.994 | 999, 0.986 | 292, 1.045 |
| open nozzle x1.6 | 2340, 0.991 | 1902, **0.929** | 1007, **0.958** | 296, 1.012 |
| **VGT nozzle matched to the whole flow (x1/0.65 = 1.54)** | 2345, 1.010 (choke flag) | 1899, **0.936** | 1007, **0.958** | 296, 1.037 |
| the same + rated target 160 / 180 (tractor), 120 (2.0) | **2423, 0.979 / 2422, 0.984** | | | **294, 0.944** |

### Measured: it is the turbine's match first

The flat targets are not the cause:
- **Lowering the target alone changes nothing.** The boost stays over it with the vanes wide open.
- With the vanes open the boost **exceeds** the target: the pickup 241 against 200, the 2.0 167 against 140.

So the turbine, even open, over-drives the compressor at rated.

The reason is in the match itself:
- The matched nozzle passes 65 % of the design flow, "the gate passing the rest at full power" (docs/models/boost.md).
- That is a wastegate's match. A VGT's gate is only a relief at 1.2 x the target.
- So a VGT's open nozzle is about 35 % short at rated.

Matching it to the whole flow:
- brings the pickup to 0.958 (+6.5 % rated power: 277 to 295 kW) and the truck to 0.936 (+3.7 %);
- leaves every torque peak within 0.1 %: tractor 2582, pickup 996, truck 2467 at 1300;
- **except the 2.0's 1750: 274 -> 257 (-6 %)**, its vanes at 0.37 near their 0.30 stop.

### Measured: then the tractor's compressor

- On the whole-flow nozzle the tractor at 1900 still runs at 1.010, with the choke flag, the vanes at 0.79 chasing
  260, and 206 kPa.
- That is (b)'s torque-peak wheel past its choke at rated: the trade accepted earlier, which now has a cost.
- A rated target of 160 to 180 kPa brings it to 0.98, at 482 kW (+7 % on today's 450, fuel-limited, lambda 1.69).

### Measured: the 2.0 needs both

- Whole flow plus a rated target of 120 gives 0.944 and 123 kW (+4 %).
- Without the lower target it stays at 1.037, boost over target with the vanes open, T3 947-1000 K against the
  matching's 900.

## 3. Proposal, with the brake in mind

1. **A variable nozzle's matched open area passes the whole design flow:** 1.0 in place of 0.65 for VGTs; wastegates
   unchanged. It is the match's own statement, corrected, and needs no new constant.
   - Measured gains: rated shafts 0.94-0.96 on the pickup and the truck, and +4 to +7 % rated power.
   - Cost: the 2.0's 1750 at -6 %.
   - For the brake (suspected, D2's to measure): at the stop (vaneMin 0.3) the area is 1.54 x 0.3 = 0.46 of today's
     open nozzle, so the brake's back pressure at a given vane position falls. The brake loses range at the shut end
     and gains headroom at the shaft. vaneMin, as a share, may want to fall with it.
2. **The boost target tapers to rated, as a maker's map does.** That is content, not sim: ecu.model 1 already has the
   boost map by speed and gear (Map16 `boost`); the archetypes run it flat at the build's level.
   - Taken from the build's own full-load fuel curve: the boost that gives the torque peak's lambda at each speed's
     fuel. The tractor's 326 -> 246 mg gives about 25 % less charge density at rated.
   - It brings the tractor under 0.98 and the 2.0 under 0.95, with power up because the turbine's back pressure
     falls (p3/p2 1.59 -> 1.17).
   - P or C should state the map, with a source if one exists for any of the three classes.
3. **Not yet clear of the limit:** the tractor at 0.98 even with both. D2 asked for headroom for the brake.
   - The remaining lever is (b)'s rated end: a ported shroud (the lead's recorded item) widens the map so the rule can
     pick a larger wheel for the same surge margin.
   - Or a rated cap in the rule, the choke at rated, F's 1.15, as a floor on the wheel. That gives back part of the
     torque peak's margin.
   - Either needs a table first.

## Predictions, and how they fared

1. All at 0.93 or more at rated, with the vanes open and the ratio below the torque peak's. **The shafts right (all at
   the relief). The ratio wrong**: over the target on the pickup and the 2.0.
2. The cause mainly the compressor. **Wrong**: the turbine's match first. The compressor only on the tractor, after
   the turbine.
3. A taper alone saves the tractor at no cost. **Wrong alone**: the boost overruns the lower target with the vanes
   open. Right only on the whole-flow nozzle.
4. The 2.0 needs the compressor. **Wrong**: the turbine and a taper (0.944).
5. Two kinds at rated; the 15 % nozzle brings the 2.0 and the pickup under 0.95; T3 at rated 1000-1100 K. **Two kinds
   partly right. The 15 % nozzle too small** (1.034 / 1.045). **T3 wrong**: 830-960 K.
6. The whole-flow VGT nozzle at 0.93-1.00, peaks within 2 %, the tractor at 220 under 0.95. **Shafts right; peaks
   right except the 2.0 (-6 %); the tractor at 220 wrong** (1.004; 160-180 gives 0.98).

Listen for (nobody here can hear):
- **The pickup at 2800 rpm full load on the whole-flow nozzle:** the turbo's whine about 8 % lower in pitch than
  today's, and the relief's chuff gone.
