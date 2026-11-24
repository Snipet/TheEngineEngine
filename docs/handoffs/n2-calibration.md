# N2: the boosted engines (card N, pattern 2) — handoff

Branch `claude/awesome-gates-ntdb58`. Measurements in `docs/measurements/calibration/boost/` (README, backpressure,
by-hand, gate, causes); tool `tee-calib-boost`, tests `tee-tests-calib-boost` (ctest `calibration.boost.*`).

**Done, by test** (GateTest.cpp, 4 cases, ~50 s): a matched wastegate holds its target spooled at the top speed (2 L
four 120.1/120.0 kPa at 5271/6528 rpm, kei 59.8/60.3 at 6500/7800, gate not saturated); back pressure against boost
(four's exhaust port over intake 1.14/1.29/1.49 at 3500/5000/6500); one state (stepped from 1500: 120.1 kPa, spooled:
120.4); the over-boost cut (old 26.3 mm gate: 249 kPa/434 N m off, 151/250 cut at 150 kPa). F's TurboTest now expects the
spring less the poppet's push (85.4 measured, 84.7 expected). Null test 59/59; tee-tests-boost 30/30; contract passes.

**Causes** (causes.md): 0, the predictor read a boosted engine 9-10 % over the full dyno (P moved to the stepped dyno;
C told through the lead). 1, no back pressure: the ports saw 0.61-0.87 of the intake (G's housings fixed it: 1.06-1.48).
2, the runaway: F's matched gate (0.85 nozzle area) could not hold its spring; spooled at the top the four ran to 255
kPa, gate open, knock retard pinned, 27 bar; fixed (aGate = aNoz/0.65, Bell; Watson and Janota). 3, a rotary's turbo
matched to half its air (D's 2df7ea6; with the sequential gate sized for the whole flow it holds 72-75 kPa against
70). 2b, clicks (X-010/X-014): valves moved in 16-sample stairs and the nozzle's choked branch jumped: both fixed (the
six 39 -> 8, the pickup at its reference; the four 20 against 8 with a recirculating valve; the tractor still 53). SC V8: engine VE flat, the blower's leak model makes the rise (no
source to change it); ~7 % of it is pattern 1.

**Numbers now** (engine.wallHeat 1, as after D's double-count fix): 2.0 four with P's boost taper 401 N m, 222.6 kW
(both in); kei with P's taper 100 N m (in), 47-48 kW at 5250-6000 (in), 52-59 kW above 6490 (the actuator's spring
floors the boost at 20 kPa: P asked for gateSpring 5 kPa); 3.0
twin six 404 N m, 217 kW (UNDER: breathes 0.51-0.65 on the manifold: N1); SC V8 835-936 N m (over).

**Closed with others**: X-010/014/015 (X, c1e1b36); C's predictor waits for boost to settle; D's 959454c: cut on.
**Not done**: the table re-run on the gate (P's); the rotary's torque (not boost); the SC's leak model
(needs a published twin-screw VE map); X-010 (diesel steps, coming from X); N1's ledger runs boosted builds at 1.2 s
settle (asked 4-5 s and a boost-over-target line).

**Outside my paths, asked**: D: engine.fm, `ecu.overBoostRef = boost.overBoostLevel();` after both applyMachine calls,
and the rotary's x2 vd (done locally by D). T: the same x2 for a two-stroke. D2: `boost.vanesAndPurge(hd.vaneCommand(),
0)` (my side in). G: `Boost::externalGates()` given.

**Cost**: the gate is sized at apply time; the cut is a compare every 16 samples: no change to R measured or expected
(tee-bench not run).

**Listen** (nobody here can hear): the 2 L four at full load 5000-6500 rpm (gate open 0.6-0.9 now, not pinned); a
tip-in at 5000 rpm (no overshoot, no knock run); the over-boost cut (boost.gateDiameter 0.0263, ecu.overBoost 150000,
full load 5300 rpm): a rhythmic fuel cut; the kei's taper and its gate's hiss at 6000-7600.
