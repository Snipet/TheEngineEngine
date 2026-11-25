# The VGT loop's integral on the package tree: predictions, 25 Nov ~15:45, before any run
The loop (machine.fm): e = (target - boost)/target; integ += 1.0 e dt, bounded -0.5..+1.0; u = clamp(0.5 e + integ,
0, 0.95); vanes = 1 - u, then held at least vaneMin (0.195 on the tractor, the stop) and opened by the back-pressure
limit's own integral (p3/p2 over 1.4). While the boost is under the target (the spool), e is near 1 and the output sits
at its stop (vanes at vaneMin, or held open by the back-pressure limit), yet integ climbs to its bound +1.0: windup.
Past the target it unwinds at |e| a second (some 0.05/s at 5 % over), so it takes (1.0 - u_eq)/|e| seconds to free
the vanes.
A. The step 1600 -> 1900 rpm (full load, the taper's 202, (b)'s 72.4 mm): the boost overshoots by 8-15 kPa (4-7 %)
   for 15-25 s. The earlier rows on (c)'s 69.7 mm showed it; (b)'s larger wheel has more head left, so it overshoots
   at least as far.
B. Full load at the torque peak 1000 rpm (target 260; the boost there is below it, about 213 measured): the boost never
   reaches the target, so e > 0 all the time and integ sits at +1.0 with the vanes at their stop or held by the
   back-pressure limit. A lift (3 s) and a re-application: no overshoot (the target is never reached); the windup is
   harmless there, but the vanes' recovery after the lift is quick.
   Prediction: no overshoot at 1000; overshoot only where the target is reachable (rated, and the truck's and
   pickup's upper speeds).
C. The cure, conditional integration (no accumulation while the output is at a stop in the error's direction: u at
   0.95, the vanes at vaneMin, or the back-pressure limit holding them open, with e > 0; u at 0 with e < 0): the step
   to rated overshoots under 3 % and settles within 3 s; the steady figures change under 0.5 %.

## Measured (the package tree, n22/sim-pkg, before; the same with the cure, after)

The harness ran 0.5 s windows at full load, with the heat settled once at the start (--heat-settle-until 1) and the
transient run free. "Over" is the time the boost stays more than 3 % over its own steady value after the step or the
re-application. Steady is the last 10 s.

| case | target | peak before | peak after | over, before | over, after | steady boost / vanes / N m (before = after) |
|---|---|---|---|---|---|---|
| A: tractor (b) 72.4 mm, 1600 -> 1900 | 202 | 236.8 (+17.2 %) | 237.6 (+17.6 %) | 8.0 s | 4.0 s | 208.5 / 1.000 / 2427 |
| B: tractor 1000, lift 3 s, re-apply | 260 | 238.3 (under the target) | 238.3 | none | none | 213.8 / 0.448 / 2585 |
| C: truck's sets 1300, lift 3 s, re-apply | 260 | 300.6 (+15.6 %) | 302.1 (+16.2 %) | 9.5 s, ringing | 4.5 s, one peak | 260.0 / 0.600 / 2680 |
| D: pickup 2800, lift 3 s, re-apply | 200 | 238.0 (+19.0 %) | 237.4 (+18.7 %) | 7.5 s | 6.5 s | 200.0 / 0.957 / 1022 |

**What it shows:**
1. **The windup is there on (b).** Short of the target, the integral climbed to its bound with the vanes at their stop
   or held open by the back-pressure limit. Past the target it unwound at the error's own rate and held the boost
   over the target: 8-9.5 s, the truck ringing at +14 % for 7 s after its re-application.
2. **The cure is conditional integration.** The integral does not accumulate while the vanes cannot follow: shutting
   (e > 0) when the output is at 0.95 or the vanes stood last step more open than the loop asked (their stop, or the
   back-pressure limit); opening (e < 0) when the output is at 0. It halves the time over the target and ends the
   truck's ringing. It is not back-calculation, which needs a tracking time this loop has no source for. The stops
   decide, and no number is chosen.
3. **The peak is not the integral's.** At the truck's peak the vanes already stand at their steady 0.55-0.60, and on
   the tractor's step they are opening (0.82 -> 0.97). The boost's rise (100 -> 300 kPa in 2 s) carries the shaft past
   the target before the proportional term (0.5 e) can open the vanes. That is the loop's tuning against the plant: it
   is not cured here, and no gain was picked.
4. **The steady figures are unchanged** to the digit (boost, vanes, torque, the four cases).
5. **The tractor at rated holds 208.5 kPa, 3.3 % over its 202,** with the vanes wide open. That is the nozzle's limit,
   not the loop's (rated-nozzle-q2.md).
6. **Correction to rated-choke-q1.md:** the "wander" of 190-216 kPa was the harness pressing the heat-settle every
   window, not the engine. Run free, the boost holds 208.5 +/- 0.5.

**The predictions:**
- A (8-15 kPa over for 15-25 s): the size was under (+35 kPa at the peak), and the duration over (8 s; the 20 s earlier
  was with the heat pressed every window).
- B (no overshoot at 1000): right.
- C (the cure under 3 % over and settled within 3 s): wrong on the peak (unchanged at +16 %), right on the steady
  figures, half right on the time (4.5 s).

**Tests on the cure:** null 10/10, boost 34/34, calib-boost 9/9. Diesel is 25/28, with the same five assertions red as
before (TurboTest :88/:108/:109, BrakeTest :226/:234: D2's fixture and brake).

**To listen for:** the truck's re-application at 1300 should whoosh up once and hold, not pulse for seven seconds.
