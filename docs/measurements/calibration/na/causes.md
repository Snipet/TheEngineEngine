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

## 17:20 to 17:50

**The lead's gas hypothesis is refuted** (struck by the lead). The cylinder's gas is not constant-gamma air:
`gammaAt` falls from 1.39 to 1.24, and the energy is pV/(gamma - 1). Gross indicated efficiency is right at low speed
and 5 to 10 % over at mid and high speed: an open observation, not a miss to fit.

**The phaser rule revised.** P's new runners changed where each engine wants its closing. On the V12 (0.36 m), the
best fixed advance by speed:

| rpm | 2000 | 3500 | 5000 | 6500 | 8000 |
|---|---|---|---|---|---|
| Best advance, deg | 30 | 30 | 10 | 0 | 0 |

The 3.2 six (0.51 m) and the economy four (0.59 m) differ again. The economy four wants its earliest closing in mid
range.

- The overlap cap is removed. It was set while the cylinder made mass on its backflow; with the cylinder conserving,
  it cost the long cams 15 % at 2000 rpm.
- The target closing now reaches 67 degrees at three quarters of the redline.
- Scheduled against each engine's best fixed advance: V12 0 to -2 %; 3.2 six 0 to -6 %; economy four 0 to -8 %
  (1500 and 4500 rpm worst).
- No single rule does better, since the best closing also follows the runner's tune. Card C's baked map is the exact
  answer.

**The intake's books** (the probe's `[.probe-books]`, from card D's running totals, g/s over 2 s). The cylinders now
conserve: in through the valves equals out through the exhaust and the trapped air. The runners did not: a linear
runner keeps volume, and the plenum gave up less than the valves took.

| Engine | Valves ÷ throttle, idle | 3000 rpm | 6000 to 7000 rpm |
|---|---|---|---|
| Flat-plane V8, before | 1.37 | 1.14 | 1.15 |
| Economy four, before | 1.06 | 1.05 | 1.01 |
| Flat-plane V8, after | 1.006 | 0.99 | 0.97 |
| Economy four, after | 1.009 | 0.98 | 0.97 |
| 3.2 six (ITBs), after | 1.000 | 1.000 | 1.000 |

The fix: `Intake` takes the valves' net mass from Engine (`mValves`, `haveValves`) and the plenum gives up exactly
that. The ITBs' throttle air is the valves' too. The waves are unchanged, and full-load torque moves -3 to +4 %.

The remaining 2 to 3 % at full load is the books' throttle term, not the engine. `airThrottle` converts `qThr` at
`pBox`, where `qThr` was converted at the box's pressure `pBox + box.p`: an over-read by the box's depression.

It waits for D's two lines in `engine.fm`; until then the intake keeps its old path.

## G. The burn's angle against speed, and where an overrun's bang lights (the lead's go, 17:37)

**The burn's angle against speed.** Under fuel.model 1 a petrol burn's angle is now multiplied by
(rpm / 3000)^kBurnSpeedExp (`sim/combustion/combustion.fm`, header). The form is Heywood §9.3's (the burn angles grow
with speed, more slowly than it). The exponent, 1/3, is judgement (turbulent flame speed near the turbulence
intensity) and is one named constant, to be corrected from the figure.

The independent check: the full-load spark aimed at the half-burn now rises with speed, as every real map does. It was
flat. On the engine dyno (degrees before top; the advance asked stays 13.2 and 13.5, being the formula's base):

| Engine | 1500 rpm | 3000 rpm | 4500 rpm | 6000 rpm |
|---|---|---|---|---|
| 3.2 six, before | 9.2 | 12.0 | 13.4 | 13.4 |
| 3.2 six, after | 6.2 | 13.3 | 16.6 | 19.1 (+13 over 1500) |
| Economy four, before | 0.5 | 4.0 | 8.4 | 12.7 |
| Economy four, after | -2.1 | 6.3 | 13.8 | 18.7 |

The economy four is knock-limited at the bottom: its retard there is 14 degrees. The half-burn sits at 8 to 10.6
degrees from 3000 rpm up.

Gross indicated efficiency barely moves: 0.407 → 0.404 at 6000 rpm on the six, 0.396 → 0.393 on the economy four.
With the spark re-aimed, a longer burn costs about 1 %. So this was a real omission but **not** the cause of the 5 to
10 % over at speed, which stays an open observation. H6 (the half-burn at best torque): 5.47 → 5.59, still an expected
failure.

**Where an overrun's bang lights.** Measured on the V8 preset, both models on:

| Gas | Full load | On the overrun (30 degrees of retard, a quarter of the firings fuelled) |
|---|---|---|
| Cylinder 1 at its exhaust opening | 1511 to 1706 K (1500 to 5000 rpm) | 650 to 1050 K |
| The collector's mean, where the bangs' store lights | about 850 K (H's reading) | 691 to 714 K |

The overrun's collector reading sits on the store's 700 K floor. H9's fuelled-overrun bangs turn on a few kelvin: the
burn's lengthening took 5 K off and the count to none. H9 is skipped on this finding (it is `[nightly]`).

The decision is the lead's: the collector's mean, or the blowdown's gas at the valve, which `overrun.fm`'s own comment
names.

## H. The four-cam V8's no-start on long runners: the parked overlap, not the runner (measured; content, to P)

P's p-idle-warm scenario on the tip, the four-cam 5.0 V8, statistics over 4 to 14 s (target 700 rpm):

| Runner | Mean | sd | Low |
|---|---|---|---|
| 0.45 m | 694 rpm | 87 | 515 |
| 0.50 m | 694 rpm | 110 | 493 |
| 0.57 m | 698 rpm | 134 | 455 |

Every length hunts: a 4-second limit cycle, the idle valve 0 to 0.23, the manifold 50 to 87 kPa. A longer runner only
deepens the dips; on f907a83 they reached a stall.

The idle loop's gains are not it. In scratch trees, on the 0.57 m runner:

- the spark integral's unwind at 1/s instead of 4/s: identical to the bit;
- the valve integral halved: sd 137;
- the spark proportional halved: sd 154.

The plant is: the build parks both cams at a fixed cam's centres (38 degrees of seat overlap at idle), and the
cylinders trap 38 to 67 % burned gas, around the burn's dilution limit. With the intake parked later, on the 0.57 m
runner:

| Intake centre | Idle sd | Range |
|---|---|---|
| 108 (as built) | 134 rpm | 455 to 883 |
| 118 | 66 rpm | 561 to 785 |
| 128 | 6 rpm | 649 to 714 |

Full load is unchanged within ±2.5 %, since the phaser rule advances it back.

A real phaser locks at its late, least-overlap end to crank and idle. Content for P: phased archetypes park the intake
about 20 degrees past the fixed-cam centre.

The 7.0 pushrod V8 (no phaser, 57 degrees of overlap) does not stall on the tip at 0.45 or 0.57 m: 748 rpm, sd 43 to
49, a big cam's lope.

**H9, settled (the lead's ruling, 17:57).** I was wrong above in one respect. The bangs' store reads card H's blowdown
hold (`ecu.blow`: each bank's hottest recent gas at an exhaust opening, falling over some 50 ms), not the collector's
mean.

Measured on the V8 preset, both models on:

| Condition | Blowdown hold | Header wall |
|---|---|---|
| On the limiter at full throttle | 1636 K (H's note had 850) | |
| H9's fuelled overrun, the counted window | 530 to 690 K | 418 K (3.5 s from a cold exhaust) |

On that overrun every fired charge is still burning as its exhaust valve opens (7 or 8 a tenth of a second, every
one). Its flame goes into the pipe, which a gas-temperature hold never saw.

The rule now has two sources:

- a charge still burning at its opening lights the store outright, held as 2000 K: a burning mixture's flame, judgement
  under a stoichiometric flame's 2200 to 2400 K (`combustion.fm`);
- hot gas lights it by self-ignition from 900 K, readily from 1200 K: H's original band, restored (`overrun.fm`).

H9 is unskipped and passes:

| Case | Bangs |
|---|---|
| A clean overrun | 0 |
| A spark-cut limiter, 3 s | 770 |
| A fuelled overrun, 0.1 | 1 (1.3 mg) |
| A fuelled overrun, 0.25 | 0 |

The fuelled overrun has little margin. The limit is now the store's fuel, not the lighting: the store receives each
overrun charge's unburned share as estimated at its spark, not the share still unburned when its valve opens. That
would take the cylinder handing its remainder over at the opening (D's cylinder, H's account). Proposed, not done.

## I. The V12's torque spike: the runner's resonance, too lightly damped (the lead's 18:13; measured)

**What it is.** Held steps (2 s each, so not the predictor's nine cycles), on the V12 with its 0.36 m runner:

| rpm | 6000 | 6330 | 6500 | 6670 | 6850 | 7020 | 7300 |
|---|---|---|---|---|---|---|---|
| Torque, N m | 486 | 551 | 722 | 771 | 692 | 642 | 608 |
| VE | 0.665 | 0.756 | 0.963 | 1.016 | 0.912 | 0.854 | 0.822 |

The spike is real, and it moves with the runner. A 0.44 m runner puts it at 5500 rpm with VE 1.13; a 0.30 m one moves
it above 8000. The primaries' length does not touch it. The phaser rule and the burn's speed term are smooth through
it.

**The fix.** The runner's end in the plenum now loses K ½ρu|u|, solved with the waves as the exhaust's ends do
(`lossFlow`):

- K 1 for what leaves the runner (a jet loses its whole head; G's `ends.fm` convention);
- K 0.2 for what is drawn in (Idelchik: a bellmouth 0.05, a square edge 0.5; judgement between them).

V12 torque at 6000 to 6850 rpm: 492 / 550 / 722 / 770 / 691 → 543 / 608 / 657 / 678 / 662 N m. Peak torque over its
target, sixteen NA archetypes: V12 1.02 → 0.96, V10 1.02 → 0.93, four-cam 1.19 → 1.12; the rest move 0 to 3 %.

E's valvetrain test 12 (the runner's tuning) reads both bumps at the acoustics' ratio: 0.701 and 0.726 against 0.729
predicted (the tip read 0.843 and 0.723). Its lower-bump window now ends at the lower of 4000 rpm and three quarters
of the main bump's speed; the reason is in the test.

**A regression of mine, fixed.** 5fdc5ad's mass books fed the plenum the valves' flow directly. That cut the runners'
coupling, so the throttle and plenum swung on their own: a V8's idle plenum rippled 24 kPa against the runners' 2, and
valvetrain test 11 failed from then on. Now:

- the runners' waves feed the plenum from instant to instant;
- the mismatch with the valves over the last whole cycle is made up after it, followed over some 0.5 s and bounded to
  half the valves' mean flow.

| Check | Result |
|---|---|
| Plenum idle ripple | 3.4 kPa (test 11 passes) |
| Books at full load | 1.000 (flat-plane V8, economy four) |
| Books at idle | 0.997 to 1.003 |

| Idle (rpm, sd) | This tree | Tip |
|---|---|---|
| Economy four | 750, 6 | |
| Flat-plane V8 | 1001, 9 | |
| V12 | 902, 34 | 31 to 78 |
| Four-cam | 698, 45 | 5 |

The four-cam's idle sits on its dilution edge and moves with any change to the plenum.

**The rotary's books balance** (P's side-port NA twin: throttle = valves within 0.2 %, the exhaust 2 % less for its
seals' leakage). The plenum does not pay twice for a shared port: its low VE is something else.

## J. The NA side-port rotary under-breathes: its ports' overlap (measured; content to P, ports to D)

The probe on P's rotary-twin-na-side-port, at intake closing:

- the charge is 425 to 450 K and 13 to 18 % burned, at about the air's pressure, so VE is 0.56 to 0.70;
- `valvetrain.fm` keeps the reference's rotary ports whatever the model (a peripheral exhaust port, side intake
  ports), with the cam dial setting the overlap: 18 degrees of the eccentric shaft at 0, 47 at the build's 0.55, 68
  at 1.

| Dial | VE at 3000 / 5500 / 8000 rpm | Torque, N m | Burned share | Charge |
|---|---|---|---|---|
| 0.55 | 0.56 / 0.66 / 0.70 | 130 / 164 / 179 | 13 to 18 % | 425 to 450 K |
| 0.35 | 0.63 / 0.71 / 0.77 | 149 / 182 / 201 | | |
| 0.15 | 0.71 / 0.76 / 0.83 | 171 / 201 / 219 | 7 to 11 % | 383 to 391 K |

The target is 211 to 216 N m at 5500 rpm.

A Renesis-type side exhaust port has almost no overlap, which the reference's peripheral exhaust cannot express.

- Content (P): the side-port build's dial at 0.15 or 0.
- Model (D, or E's path): side exhaust ports for a rotary.

**Rotary ports from the build (the lead's 18:59).** With the lab model, a rotary whose intake or exhaust is set to free
actuation now takes that port's opening and closing from the free valve's angles, read as four-stroke equivalents (a
rotary stroke is 270 degrees of the eccentric shaft, a piston's 180, so the port's angle is 1.5 times the field's).
The reference's peripheral exhaust closes some 50 degrees after the top; a side exhaust port closes near it.

P's NA side-port twin with RENESIS-like timing, in eccentric degrees from memory (I would put them at ±10 degrees):
the exhaust opens 50 before bottom and closes 3 after top, the intake opens 3 after top and closes 65 after bottom.
As free angles: 146.7 / 362 / 362 / 583.3.

| rpm | 2500 | 4000 | 5500 | 7000 | 8500 |
|---|---|---|---|---|---|
| Torque with the reference's ports (dial 0.15), N m | 165 | 195 | 201 | 202 | 221 |
| VE, reference's ports | 0.69 | 0.76 | 0.77 | 0.76 | 0.84 |
| Torque with side-port timing, N m | 198 | 224 | 231 | 233 | 238 |
| VE, side-port timing | 0.82 | 0.85 | 0.85 | 0.85 | 0.88 |

The target is 211 to 216 N m at 5500 rpm. The curve is flat, as the real engine's is, about 7 % over. Its rise at
8500 comes with the two-stage intake's short runner.

## K. X-022: throttles a runner behind a tiny boosted charge pipe diverged (my regression of 5fdc5ad; fixed)

X's reduced build: ITBs, a supercharger, the charge pipe at its least volume (0.5 l), the intercooler at its least
flow. Under throttle blips the box's pressure rang up within 60 ms of the first opening: 99, 156, 10, 0, 69, 973 and
1738 kPa, on to 30 MPa. Then FLAG_FAULT.

The cause: since 5fdc5ad the ITB branch drew the charge pipe by the valves' mass, which does not answer the pipe's
pressure, and on a half-litre volume the boost machine's explicit step rang up. Two softer forms were tried and
dropped, each with a measurement:

- a per-cycle correction toward the valves' mass, bounded: faulted at 3.4 s;
- the same, faded with the pipe's pressure: faulted at 7.3 s.

So for ITBs the trumpets' own flow draws the box, as before 5fdc5ad. The throttles' outside pressure is also held at
8 kPa absolute at the least (the cylinder's and the reading's floor).

| Check | Result |
|---|---|
| tee-fuzz seed 3 on the reduced build | ok |
| `X022Test.cpp`: ten seconds of idle, and of blips | no fault, all finite |

**Open:** with ITBs the throttle's books and the valves' differ, since the runners' linear waves keep volume, not
mass. On the 3.2 six, throttle over valves reads 0.69 at idle, 1.03 at 3000 rpm and 0.89 at 6000. The cylinders take
the right air (the valves' side is exact). Only the throttle's figure, and a boost machine's draw on ITBs, carry the
error.

The charge pipe still empties under blips on this build (0 to 12 kPa): a supercharger behind an intercooler at its
least flow cannot keep up. That supply is the boost's, N2's.
