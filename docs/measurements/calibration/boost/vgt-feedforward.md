# The VGT loop's feed-forward from the match (research): predictions, 25 Nov ~16:30, before any run
The match sizes the open nozzle to pass the design flow at p3 = 1.247 times the boost's absolute, choked form
(machine.fm: aNoz = m sqrt(R T) / (0.667 p3)). The same relation, read backward at the moment's flow and gas
temperature, gives the opening that passes that flow at the p3 the target asks:
v_ff = m sqrt(R T3) / (0.667 1.247 (pOut + target) aNoz). The loop then trims around it: u = (1 - v_ff) + 0.5 e +
integ, with the conditional integration kept. The flow is the engine's draw (drawMean, the 0.1 s mean the machine
already keeps); the fuel's 3 % is left to the integral. No new number.
Predictions, the same four cases as vgt-windup.md (after the cure):
- the peak over the target after the step or the re-application: from +16-19 % to under 8 %;
- the time over the steady +3 %: under 2 s;
- the steady boost, vanes and torque: within 0.5 % (the integral trims what the 1.247 misses: Q2 measured p3/p2
  1.17-1.40);
- the tractor's 1000 rpm, under the target throughout: unchanged.

## Measured (n2-b-local with the cure, plus the feed-forward in scratch; the four cases of vgt-windup.md)

| case | peak, the cure alone | peak, with the feed-forward | over steady +3 %, cure -> ff | steady boost / vanes / N m, cure -> ff |
|---|---|---|---|---|
| A: tractor step to rated (202) | +17.6 % | +16.0 % | 4.0 -> 3.0 s | 208.6 / 1.000 / 2427 -> 209.0 / 1.000 / 2427 |
| B: tractor 1000, lift, re-apply (260, out of reach) | under the target | under the target | (to its own steady) | 213.8 / 0.448 / 2585 -> **229.1** / 0.441 / 2582 |
| C: truck's sets 1300, lift, re-apply (260) | +16.2 % | +7.6 % (at +19 s) | 4.5 -> 29.5 s | 260.0 / 0.600 / 2680 -> **222.8 / 0.558 / 2622** |
| D: pickup 2800, lift, re-apply (200) | +18.7 % | **+6.8 %** | 6.5 -> 3.0 s | 200.0 / 0.957 / 1022 -> 200.0 / 0.958 / 1022 |

**What it shows:**
1. **Where the match's relation holds, the feed-forward does what a production loop's does.** On the pickup at 2800 the
   peak falls from +18.7 % to +6.8 % and the time over from 6.5 to 3.0 s, with the steady state unchanged.
2. **It changes the tractor's step little:** that overshoot comes with the vanes opening to their stop, the nozzle's
   limit (Q2).
3. **It breaks the truck's steady state at 1300 rpm.** The boost falls from 260 to 223 kPa and the torque by 2.2 %, with
   the vanes MORE shut (0.558 against 0.600). That reads as the gain reversal (shut vanes pump the engine, task 11's
   ground): the relation's p3 = 1.247 p2 asks the vanes past the boost's maximum at that flow, and the trim, held by
   the conditional integration while the back-pressure limit holds the vanes, cannot bring them back. Suspected; the
   back-pressure limit's state was not read.
4. **The tractor at 1000 settles 7 % higher** (out of reach of its target either way), with the torque the same within
   0.1 %.

**The predictions:**
- the peak under 8 %: right on the pickup and the truck, wrong on the tractor's step;
- under 2 s over: wrong (3.0 s at best, 29.5 on the truck);
- the steady state within 0.5 %: wrong on B and C;
- B unchanged: wrong.

**For the record:** a feed-forward from the match's 1.247 is not shippable. It needs the vane opening's true steady
value for the speed and fuel, which turns on p3/p2 at that point (Q2: 1.17-1.40 measured, from p4 and T3). That is the
same missing p4. Nothing built into the package.
