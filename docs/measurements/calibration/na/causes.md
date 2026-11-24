# The engines that breathe by themselves: causes (card N1)

*Card N1, 2026-11-24. Measured on the engine dyno (`scripts/ledger.py`: MODE_DYNO_ENGINE, full throttle, warm, each
speed held 1.2 s and averaged over 1.0 s) and with the probe (`tests/sim/calibration/na/Probe.cpp`, hidden tags
`[.probe]` and `[.probe-idle]`). The probe reads cylinder 1's own state at each intake closing and splits the
volumetric efficiency into its factors, VE = (p_ivc / p_air) x (T_air / T_ivc) x (V_ivc / V_d) x (1 - burned share).
The product matches `tel.engine.veAir` to within 0.005. It also integrates the throttle's mass flow against the fresh
mass every cylinder traps. The engine is chaotic: every figure is a mean over cycles. "Measured" means run here;
"suspected" means not yet isolated. Figures from 4c70e77's simulator unless said (the tip at 16:20 did not compile).*

## A. The walls' heat (engine.wallHeat): measured, a level, not a spread

Woschni's walls with card H's whole burn, against the reference's walls, on all 16 NA petrol archetypes:

- Peak torque rises by 17 to 25 % and peak power by 21 to 27 %, nearly uniformly.
- Torque over its target's middle goes from 0.71 to 1.08 to 0.88 to 1.30, so pattern 1's spread is the same 1.5x
  either way.
- VE does not move (within 0.02).
- The walls take 15 to 19 % of the burn's heat under either model (economy four, 2500 to 6300 rpm). This is in
  Heywood's range for heat transfer at full load (§12.1: 15 to 35 % of the fuel's energy to the coolant, of which the
  in-cylinder share is the larger part).
- The rise is all the burn's kept share: 0.97 against the reference's 0.60 + 0.20 x manifold, about 0.78 at full
  throttle. The fuel accounts close: the burn's heat is 0.91 of the fuel's lower heating value at lambda 0.88, the
  rich mixture's own limit.
- With Woschni, brake efficiency is 0.32 to 0.36 at full load. Modern engines run 0.31 to 0.36 there (Heywood §2.10;
  makers' BSFC maps of 230 to 270 g/kWh).
- Port gas reads 1350 to 1520 K, against the 1100 to 1300 the ledger holds it to. Suspected: this is the smoothed
  port temperature reading the blowdown's hot early gas, not a mass-averaged one.

**Which the archetypes should use: Woschni (1).** It is the physics, and the reference's walls count the wall loss
twice, once in the burn's share and once through the walls. It does not close pattern 1. It moves the mild engines
further over (1.19 to 1.30 x target) and brings the tuned ones close (0.88 to 0.98).

## B. Pattern 1: VE does not separate the heads; the exhaust is where the tuned engines lose (measured, routed to G)

| | VE peak, as built | Textbook (Heywood §6.2, figs. 6-9 and 6-10; Blair ch. 6) |
|---|---|---|
| Mild engines | 0.95 to 1.06 | 0.85 to 0.92 |
| Tuned engines | 0.89 to 0.96 | 1.0 to 1.1 |

Experiments on the 3.2 L six at 7600 rpm:

| Change | VE | Torque | PMEP, bar |
|---|---|---|---|
| Port quality +25 % (a racing head) | +3 % | | |
| 0.35 m runner | 0.99, at 6500 rpm only | | |
| Free muffler | | +5 % | 0.95 → 0.72 |
| Exhaust valves 1.2x | | | 0.95 → 0.83 |
| An open exhaust, a dual one, or exhaust.model 0 | identical to the bit | | |

The last row is not a result about the engine: **G's exhaust builder is not wired into the engine yet**, so every
`spec.exhaust` field but the reference's four dials is ignored.

The probe at 7600 rpm:

- the exhaust port's mean pressure is 154 kPa, 53 kPa over the air. Bosch's *Automotive Handbook* gives 10 to 30 kPa
  of back pressure at rated power;
- the cylinder averages 70 kPa through the intake stroke, against a 97 kPa manifold;
- the trapped gas is 0.975 of the swept volume at the air's density (no net ram), 7 % of it burned.

The flat-plane V8 traps 10 to 26 % burned gas at 2000 to 6000 rpm (the six 4 to 7 %). Its loss is residual, through
the overlap, with no tuned exhaust to scavenge it.

The reference sizes its pipes by displacement per cylinder alone (`engine.fm`, `kPipe`, the 0.45th power), not by the
flow a high-revving engine pushes.

The reference's pipes' diameters x1.3 (a scratch tree), at 3000, 5500 and 7500 rpm:

| Engine | Torque | PMEP at the top, bar |
|---|---|---|
| 3.2 six | 226 → 253, 267 → 282, 249 → 263 N m | 0.93 → 0.65 |
| Flat-plane V8 | +5 to +10 % | |
| V12 | +4 to +11 % | |
| Economy four | 0 to +4 % | |

Sizing the pipes by flow separates the classes the right way, but not by enough alone.

Suspected and not yet isolated, since G's network is not in the engine: with tuned primaries and flow-sized pipes, the
overlap's residual falls and the tuned engines' VE rises toward 1.0 to 1.1.

Routes:
- **G**: wire the builder; the pipes sized by flow.
- **P**: port quality by class of head. All 30 archetypes use the defaults, 1.35 and 1.5.

## C. The intake phaser's placeholder schedule over-advances long cams (measured; E's path, on the lead's word)

`Phaser::schedule` puts the whole range of advance in at full load from 12 to 50 % of the redline. On cams parked
near their best full-load centre, that is too much overlap:

| Engine, speed | Torque with the schedule | Torque, phaser parked |
|---|---|---|
| 3.2 six, 3000 rpm | 227 N m (VE 0.71) | 274 N m (VE 0.86) |
| V12, 5000 rpm | 457 N m | 554 N m |
| V10, 4900 rpm | 397 N m | 424 N m |

It cuts dips into 6 of the 7 phased NA archetypes but hardly moves their peaks.

The best advance by speed, from a fixed-centreline sweep:

| Engine | Low speed | High speed |
|---|---|---|
| 3.2 six | 30 deg | 0 at the top |
| Economy four | 45 deg | 15 at the top |

The best intake closing rises with speed on both, as Heywood §6.3 has it, but no rule in shares of the range fits both.

Makers calibrate the map on a dyno. **Route: P's generator writes `ecu.maps.camIn` from a sweep.** The full-load row
for the seven phased NA archetypes is in [cam-maps.md](cam-maps.md): fixed centrelines in 10-degree steps, seven
speeds.

How much the phaser is worth, measured:

| Schedule | Mean torque over seven speeds | Peak torque |
|---|---|---|
| The calibrated maps, against parked | +2.5 to +5.2 % | +2 to +8 % |
| Half the range (a constant, not proposed: it would be tuning to fit), against the schedule as it is | flat-plane V8 +5 %, 3.2 six +7 %, V12 +6 %, V10 +3 %; the mild ones -0.3 to -1 % | |

So the phaser is a shape fix worth a few percent, not pattern 1's cause.

**Fixed (the lead's decision, 16:26).** `Phaser::schedule` (E's) now schedules the intake by a target intake-closing
angle at the seat: 20 degrees after bottom at a quarter of the redline and below, rising to 67 at the redline. The
advance is what brings each cam's parked closing there, capped by an overlap of 40 + 60 x (the speed's share of the
redline) degrees.

The lead suggested 30 to 40 degrees at low speed. The sweep's best closings for the short cams were 3 to 18 degrees
(the economy four), so with 35 the 3.5 V6 lost 50 N m of its peak. 20 lets the overlap cap hold the long cams back
instead.

Before and after, mean torque over seven speeds, as built:

| Engine | Before | After | Calibrated maps |
|---|---|---|---|
| Flat-plane V8 | 344 | 355 | 365 |
| Four-cam V8 | 436 | 441 | 451 |
| 3.2 six | 246 | 264 | 267 |
| V10 | 409 | 419 | 424 |
| V12 | 491 | 522 | 536 |
| Economy four | 137 | 136 | 139 |
| 3.5 V6 | 319 | 311 | 316 |

The holes: the 3.2 six at 2700 rpm, 206 to 290 N m; the V12 at 5000 rpm, 495 to 582. Test: `calibration-na.the
intake phaser's schedule cuts no hole in a long cam's full-load curve` (29 s). The valvetrain's and the intake's 21
tests pass; the null test passes, 11 of 11.

## D. Mass created at the intake valve: pattern 4 (measured; D's file, routed to D)

`sim/core/cylinder.fm:290` (the reference has the same line, `sim/ref/cylinder.fm:279`):

    mOutI = qI >= 0 ? qI ppI / (R t) : qI ppI / (R tAir)

Gas the valve pushes back into the runner leaves at the cylinder's temperature. The same volume comes back at the
air's. The runner is a linear waveguide that carries volume with no memory of temperature, so every round trip
creates mass, and the cylinder counts it as fresh air (`rvMass` and `rvInert` track only the burned part).

Measured, throttle mass flow against the fresh mass trapped:

| Engine, condition | Through the throttle | Trapped fresh | Ratio |
|---|---|---|---|
| V12, warm idle on the bench | 0.66 to 1.48 g/s | 19 to 22 g/s | 14 to 32x |
| Economy four, warm idle | 1.9 g/s | 3.0 g/s | 1.58x |
| Economy four, full load | | | 1.05 to 1.12x |
| Small-block, full load | | | 1.09x |
| Flat-plane V8, full load | | | 1.5 to 2.0x |
| 3.2 six (ITBs), full load | | | 1.2 to 3.0x |

At idle the V12's idle valve sits shut at 1000 to 1100 rpm for 900 with the manifold at 80 kPa: the controller has
nothing left to close. **This is pattern 4.**

The experiment (`intake-roundtrip.diff`, a scratch tree, not committed): the pushed-back gas's own mass-weighted
temperature, and gas drawn in while some is waiting is converted at it.

- Economy four idle: 1.06x (from 1.58).
- V12: 7 g/s against 13. It idles at 810 to 930 rpm on a working idle valve, lumpy, IMEP changing sign from second
  to second, as a big cam's does.
- Full-load peaks: mild engines -1 to -4 %; big-overlap ones -11 to -16 % (flat-plane V8 -16, 3.2 six -12, V10 -12,
  V12 -11).

So the created mass had flattered the tuned engines most. Their real filling is poorer, and B and C are what hold
them down. The exhaust side, converted the same way at its outflow's own temperature, changes nothing measurable.
The remaining 2x on the V12 at idle is not explained.

## E. Pattern 3, the rotary: over-boost, not the rotary (measured; to N2)

- Rotary-twin-13 on the dyno: boost 147, 137 and 92 kPa gauge at 2850, 4780 and 6680 rpm, against a 70 kPa target.
  Both gates are wide open (1.0) and the over-boost flag is set. The manifold reaches 277 kPa absolute at 1500 rpm.
- With `boost.kind` Natural it peaks at 257 N m (VE 1.01 at 4590 rpm, 0.62 to 0.82 elsewhere), brake efficiency
  0.28 to 0.31: within about 20 % of an NA 13B Renesis (211 N m, the maker's figure).
- The doubling is pattern 2's: the gates cannot bypass the rotary's exhaust flow.

## F. The intake's charge heating: a level, not a spread (measured; a scratch tree)

The intake's model leaves out the heat the manifold's and the ports' walls give the charge. The gas reaches the valve
at the outside air's temperature. Heywood §6.2.1 has a rise of some 10 to 40 K at full load, more at low speed.

The experiment: the intake valve's gas taken 25 K warmer (`engine.fm`, `stepAreas(..., intk.tAir + 25)`), at 2500,
4500 and 6000 rpm:

| Engine | VE | Torque |
|---|---|---|
| Economy four | 0.950, 1.043, 1.032 → 0.881, 0.971, 0.961 | -9 % |
| 3.2 six | | -9 % |
| Small-block | | -9 % |

It is a level, like A: it would take back about a third of Woschni's gain, on every engine alike. It is physics left
out (E's intake), worth adding with a source for the rise by speed and load. It does not separate the classes.

**Added (on the lead's word, 16:46), waiting for one line of D's.** `Intake::control`, model 1, heats the charge:

    rise = (T_coolant - T_air)(1 - exp(-NTU)),   NTU = 0.29 (m / m_ref)^-0.2

That is a stream past water-jacketed walls; h goes as the flow's 0.8th power, hence the -0.2. The rise is some 19 K at
full load, 35 K at idle, none cold. The 0.29 is judgement within Heywood's 10 to 40 K. It needs Engine to set
`intk.tCoolant = heat.tCool` before `intk.control` (D's file). Until then it does nothing.

In a scratch tree with that line, against the tree without it:

| Engine | Torque at 1500 rpm | Torque from 4000 rpm to the top |
|---|---|---|
| Economy four | -9 % | -7 to -8 % |
| 3.2 six | -11 % | -7 % |
| Small-block | -9 % | -7 % |
| V12 | -10 % | -6 to -7 % |

## Since 16:46 (measured on the tip)

**Pattern 4 is closed** by card D's conserving cylinder (the round trip's store, on the physical path). Warm idle on
the bench (the probe's `[.probe-idle]`):

| Engine | Idle before | Idle now (target) | Idle valve | Manifold |
|---|---|---|---|---|
| V12 | 1244 | 882 rpm (900) | 0.15 | 90 kPa |
| V10 | 1166 | 952 rpm (950) | 0.11 | 80 kPa |
| Flat-plane V8 | 1340 | 998 rpm (1000) | 0.12 | 89 kPa |

Every idle valve is working now. The economy four's balance at idle is 1.048 (was 1.58), and the test is unskipped.
Still unexplained: the big-overlap engines trap about twice the throttle's air at idle (V12 11.6 against 5.9 g/s).

**B on the builder (in the engine since 16:17).** On the 3.2 six:

- The archetypes' exhausts as built give results identical to the bit to exhaust.model 0: the reference's topology,
  with its road converter and chambered muffler.
- Open (no converter, straight-through): port mean 154 → 120 kPa at 7600 rpm, burned share 7.2 → 4.3 %, torque +6 to
  +10 %.
- Primary length from 0.4 to 1.4 m: ±2 %.
- Runner length is the lever:

| Runner | Best VE | Torque there | At |
|---|---|---|---|
| 0.25 m (as built) | 0.94 | 300 N m | 5000 rpm |
| 0.35 m | 0.99 | 305 N m | 6000 to 7000 rpm |
| 0.50 m | 1.05 | 335 N m | 5000 rpm |
| 0.70 m | 1.04 | 330 N m | 4000 to 5000 rpm |

So the tuned engines' shortfall is content: the runner, the exhaust and the port quality. Values went to P.

**The half-burn readings of 29 to 39 degrees** do not reproduce: 0 of 98 cycles over 20 degrees. The late burns now
are knock retard on regular fuel at 11:1 (economy four: 13.7 degrees at 1500 rpm, 0.2 at 6336).

**Gas properties (the lead's hypothesis).** `gammaAt` falls from 1.39 to 1.24, near Heywood's values. Gross indicated
efficiency is 0.34 to 0.36 at 1500 rpm and 0.38 to 0.42 from 4000 rpm up: 5 to 10 % over 0.8 of the fuel-air cycle at
speed. Port gas runs 1100 to 1530 K. Suspected: too little heat leaves at speed (Woschni under-reads during SI
combustion; no exhaust-port wall loss). Proposed to the lead and D, not changed.

**Fixed by N1, acting for E and H**, measured before and after:

- the phaser by target intake closing (C);
- the ITB manifold reading floored at 8 kPa (it read -11.4 kPa on the lift);
- a rotary's burn 0.92: -4 to -6 %, boosted 318 N m at 5000 rpm for 290 to 315;
- the charge's heating (F): in, waiting for D's `intk.tCoolant` line.
