# Step A, the turbine matched on the build's rated p4 and T3 (local): predictions, 25 Nov ~23:55, before any run
The method: pass 0 on today's match (stand-ins), read p4 (the turbine's outlet) and T3 (its housing's gas) at rated;
fill them in by hand (spec.boost.ratedP4, ratedT3), re-match (pass 1), read again, re-match (pass 2). The open area
from each pass; the steady state at rated (boost, vanes, shaft, torque) from each.
1. The open area against today's: within 4 % on the tractor, the 2.0 and the pickup (the 1.247 was within 4 % of the
   rated balance with measured p4 and T3 on these, rated-nozzle-q2.md); the truck's sets move most, smaller by some
   10-18 % (its 1.18).
2. The steady rated rows: within 1 % of torque on the tractor, the 2.0 and the pickup; the truck's sets' boost up a
   few % at 2100 with the vanes off their open stop (the open area smaller), the choke flag unchanged or worse.
3. The convergence: pass 2's open area within 2 % of pass 1's (p4 and T3 move little with a small area change).
4. Rule (c)'s check (bit 64): the tractor on its flat 260 fails (it sat at 0.986-0.990 shaft and phi near the flag),
   the truck's sets fail at 2100 (shaft-limited, phi 0.96), the 2.0 and the pickup pass (phi 0.90, 0.80).

## Measured (local branch n2-match-a, 6115fbf7 + 5b6409fb, not pushed; the tip 9c5067bc under it)

The method: the rated cell (2.0 4000, pickup 2800, truck's sets 2100 on D2's fixture, tractor 1900, its shipped flat
260), 40 s from the step, the heat settled each window, the last 20 s. p4 and T3 are the turbine's outlet and housing
gas as 1 s means. Each pass fills the build's ratedP4 and ratedT3 from the pass before it.

| build | pass | open area cm2 | p3 / p4 kPa | T3 K | boost kPa | vanes | N m | ratedPhi | mismatch |
|---|---|---|---|---|---|---|---|---|---|
| 2.0 | 0 (stand-ins) | 3.994 | 324 / 137.1 | 905 | 167.7 | 1.000 | 299 | 0.855 | 32 |
| 2.0 | 1 | 4.736 | 281 / 136.8 | 954 | 151.7 | 0.977 | 298 | 0.855 | 0 |
| 2.0 | 2 | 5.043 | 270 / 136.7 | 970 | 145.7 | 0.946 | 297 | | 0 |
| 2.0 | 3 | 5.142 | 268 / 136.2 | 965 | 143.2 | 0.924 | 296 | | 0 |
| pickup | 0 | 9.598 | 363 / 131.9 | 846 | 200.6 | 0.951 | 1020 | 0.860 | 32 |
| pickup | 1 / 2 / 3 | 10.915 / 10.953 / 10.973 | 370 / 131.9 | 848-850 | 199.8-199.9 | 0.815 | 1015-1016 | | 0 |
| truck's sets | 0 | 14.199 | 370 / 116.6 | 785 | 207.1 | 0.990 | 1944 | 1.154 | 96 |
| truck's sets | 1 / 2 / 3 | 14.738 / 14.837 / 14.837 | 357-358 / 116.4 | 787 | 201.5-202.1 | 0.969-0.974 | 1953 | | 64 |
| tractor (flat 260) | 0 | 13.758 | 420 / 124.3 | 821 | 227.2 | 0.920 | 2385 | 1.038 | 96 |
| tractor | 1 / 2 / 3 | 15.409 / 15.477 / 15.499 | 431 / 124.3 | 823-824 | 227.5-227.6 | 0.795-0.799 | 2372-2375 | | 64 |

The low end on pass 3's fill against today's (the torque peaks):

| cell | today N m (vanes) | pass 3 N m (vanes) |
|---|---|---|
| 2.0 1750 | 251 (0.371) | 255 (0.287) |
| pickup 1700 | 996 (0.488) | 996 (0.427) |
| truck's sets 1300 | 2678 (0.598) | 2674 (0.570) |
| tractor 1000 | 2589 (0.449) | 2587 (0.399) |

**What it shows:**
1. **The rated balance gives a LARGER open area than the stand-ins, not a smaller one:** the 2.0 +29 %, the tractor
   +13 %, the pickup +14 %, the truck's sets +4.5 %. The balance takes the compressor at its peak (compEta) on the
   match's design flow. On the engine the compressor sat at 0.70-0.74, and the flow differs from the 0.9 VE design
   flow. So the balance asks less expansion than the engine actually needed, and the area comes out large. Q2's
   balance closed to 1-7 % because it used the measured compressor power.
2. **The rows barely move.** Rated torque is within 1.0 % on every build. The low end is within 0.2 %, except the 2.0
   at 1750 (+1.6 %). The vanes take up the difference: at rated they now regulate below their open stop on the 2.0
   (0.92), the pickup (0.82) and the tractor (0.80), so the loop has authority there. The 2.0's boost at 4000 comes
   down from 168 to 143 kPa, to its 140 target, where today it ran over the target with the vanes wide open.
3. **The convergence:** pass 3 against pass 2 moves the area +0.1 % (tractor), 0 (truck), +0.2 % (pickup) and +2.0 %
   (the 2.0). The 2.0 converges slowly, its T3 rising with the area (905 -> 970 K). One re-match is a match on three
   builds, not on the 2.0: it needs two or three passes, or a damped update.
4. **Rule (c)'s check (bit 64)** flags the tractor on its flat 260 (rated phi 1.04) and the truck's sets (1.15); the 2.0
   (0.855) and the pickup (0.86) pass. That is unchanged by the turbine's match, which is right: it is the
   compressor's check.

**The predictions:**
- 1 (the area within 4 %, the truck smaller): wrong. Larger by 4-29 %; the truck moved least, not most.
- 2 (rows within 1 %): right, at rated and at the low end save the 2.0's +1.6 % at 1750.
- 3 (convergence within 2 % at pass 2): right on three, wrong on the 2.0 (6.4 %).
- 4 (bit 64): right on all four.

**For B (the bake):**
- the compressor's efficiency at the design point should be the map's at that point, as (b)'s charge terms are, not
  the peak;
- the rated flow should be the engine's measured one, not the 0.9 VE design flow. The bake has both from its rated run;
- the convergence measure is the open area's change between passes, under 1 % (measured here: 0.0-0.2 % on three
  builds after pass 2), with the 2.0 needing a damped or repeated pass.
