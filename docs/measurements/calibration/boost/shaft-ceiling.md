# The shaft's ceiling in the boost loop: predictions, 25 Nov ~17:05, before any run
The law (the lead's ruling: the threshold is omegaMax itself): the loop's error is the lesser of the boost's
(target - boost)/target and the shaft's headroom expressed as the boost's, 1 - (omega/omegaMax)^2, the most loaded shaft.
The square is the map's own law: the pressure rise goes with the tip speed squared (compressor.fm, c(M) U^2 / cp T).
So the two errors are in one unit, and at steady state the loop holds the lesser of the target and the boost at which
the shaft stands at 1.00. The conditional integration acts on that one error as before. The relief at 1.05 stays.
No new number.
1. The truck's sets at 2000, the 10 s lift and re-application: the shaft's most from 1.066 to 1.00-1.02; no
   over-speed flag; no ring (one approach, settled within 2 s).
2. Shaft-limited steady states move to the shaft at 1.00:
   - the truck at 2000 (0.97-0.99 today) and 2100 (0.964): boost +0-3 %, torque within 1 %;
   - the tractor on a flat 260 at rated (0.986-0.990): +0-1 %;
   - the 2.0 at 4000 (0.815): unchanged, as it is not shaft-limited.
3. Every other cell of the package's table, and the boosted archetypes' steady rows: bit for bit where the shaft
   stays under sqrt(1 - e) of its limit. The tractor at 1000 (shaft 0.81, boost 18 % under) sits near that line:
   unchanged or within 0.5 %.
4. Null 10/10; boost 34/34; calib-boost 10/10; diesel the same five reds.

## Measured (n2-b-local with the X-039 cure; scratch copies for the relief and the margin)

omegaMax on the truck's sets is 11902 rad/s: the shaft the ceiling holds with the relief off. The 1.066 earlier
(vgt-windup's follow-up) was read against D2's 11291; against 11902 the old peak is 1.061.

**1. The truck's sets at 2000, full load:**

| loop | shaft's most | shaft in steady running | over-speed rows | N m |
|---|---|---|---|---|
| today (no ceiling) | 1.061 (after a lift) | 0.866-1.053, cycling every 0.8 s from the first pull | 19 of 100 | 1999 |
| ceiling at omegaMax | 1.060 | 0.872-1.052, cycling | 20 of 100 | 1998 |
| ceiling at omegaMax, the relief off | 1.108 (the approach) | 0.999-1.001, steady | | 2031 |
| today, the relief off | 1.184 | 1.003-1.090, cycling | | 1976 |
| **ceiling at 0.948 omegaMax (built)** | **1.049 (a step), 1.051 (after a lift)** | **0.947-0.950, steady** | 3 (step), 12 (lift) | **2090** |

**What it shows:**
- The cycle was there at steady full load, not only after a re-application. It is a relay between the over-speed
  relief (past 1.05 the vanes snap wide open) and the loop closing them again.
- A ceiling at omegaMax holds 1.00 steady with the relief off. But its approach overshoots 10.8 %, through the relief,
  so with the relief on the cycle remains.
- So the ceiling stands that overshoot under the relief: 1.05 / 1.108 = 0.948, the margin the measured overshoot
  needs (the lead's rule). Its peaks are 1.049-1.051: at the relief, not past it.

**2. The package's cells, the last 30 of 60 s, before -> after:**

| cell | N m | boost kPa | shaft | flags |
|---|---|---|---|---|
| truck's sets 2000 | 2003 -> 2090 (+4.3 %) | 207.6 -> 210.3 | 0.945 (0.926-0.969) -> 0.948 steady | 1.67 -> 0 |
| truck's sets 2100 | 1870 -> 1950 (+4.3 %) | 207.2 -> 204.9 | 0.965 (0.947-0.985) -> 0.959 steady, vanes 1.0 | 1.98 -> 0 |
| tractor, flat 260 at rated | 2330 -> 2389 (+2.5 %) | 244.6 -> 227.1 | 0.989 -> 0.948 | 0 |
| tractor, taper 202 at rated | 2430 -> 2429 | 212.3 -> 212.2 | 0.888 | 0 |
| 2.0 at 4000 | 298.8 -> 298.9 | 169.4 | 0.820 | 0 |
| tractor 1000 | 2585 -> 2582 (-0.1 %) | 223.9 -> 229.6 | 0.823 (0.783-0.839, wandering) -> 0.829 steady | 0.03 -> 0 |
| every other cell (tractor 900/1100, truck 1000/1300, 2.0 1750, pickup 1300/1700/2800) | within 0.1 % | within 0.2 kPa | same | same |

On the truck at 2100 the shaft stands at 0.959 with the vanes wide open: the open nozzle's limit, as in Q2. The
ceiling holds what the vanes can.

**3. The boosted archetypes' steady rows (the harness's 4 s settle):** 34 of 53 cells move under 0.2 % and 0.5 kPa.
The 19 that move more are cells still spooling at 4 s. The ceiling's error is the lesser one during a spool whenever
the shaft's headroom is under the boost's error, so the spool's path changes. Settled over 60 s:

| cell | 60 s before | 60 s after |
|---|---|---|
| the 2.0 at 1900 | 299.7 N m / 125.1 kPa | 300.1 / 125.6 |
| the pickup at 1500 | 982.8 / 141.0 | 982.8 / 141.9 |
| the pickup at 1900 | 1006.6 / 200.0 | 1006.3 / 200.0 |

These were +1.5 %, +4.5 kPa and -0.5 % at 4 s. The tractor at 1900 is the shaft-limited cell above. The petrol cells
(at most +3.0 % on the straight six at 2000, the cell known to wander 465-518 N m) move inside their own spread. Only
the supercharged V8 is bit for bit; it has no turbo shaft.

**4. Tests:** null 10/10, boost 34/34, calib-boost 10/10. Diesel is 25/28 with the same five reds (D2's).

**The predictions:**
- 1 (the shaft's most 1.00-1.02, no ring): wrong at omegaMax (the relay stays), right at 0.948 (1.049-1.051, steady).
- 2: the truck's moves were bigger than predicted (+4.3 % torque, not within 1 %), from ending the cycle; the
  tractor's flat 260 gives +2.5 %; the 2.0 is unchanged, as predicted.
- 3 (bit for bit elsewhere): wrong at the 4 s settle (the spool's path), right when settled, within 0.1 %.
- 4: right.
