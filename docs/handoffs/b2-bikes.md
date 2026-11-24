# B2: motorcycles

Branch `claude/awesome-gates-ntdb58`. Model: `sim/vehicle/bike.fm`, `bspec.fm`, `chain.fm`; doc `docs/models/bike.md`.
Chosen by `car.kind` Motorcycle with `car.model` 1 (additive dispatch in card I's `car.fm`; `kind`'s range now 0-2; model
0 takes it as a car). Null test (full) green before the first push, `[null][quick]` before each; card I's 37 green.

**Done** (`tee-tests-bike`, 15 tests, ~20 s): `bike.launch` (superbike lifts, held at 3.1 deg, 10.1 m/s^2 vs limit 10.7,
front load 0, rear short of grip), `bike.launch.cruiser` (never lifts, grip-limited 98 %, front >= 799 N),
`bike.quickshift` (cut 50.0 ms, drive gone, no over-rev/stall), `bike.autoblip`, `bike.slipper` (braking into a corner,
down a gear: no slipper rear slip -0.154 past peak 0.111, flagged hopping/locked; slipper -0.052, never), `bike.lash`,
`bike.soak` (10 min); `bike.class.*` (rig engine with each class's power curve): superbike 9.47 s/150.5/191.8 mph vs
9.9/145/180; 600 10.79/128.6/159.7 vs 10.84/127.8/161; tourer 14.33/95.8/107.0 vs 13.67/99.8/115 (Cycle World and
others; sources and judgement in the doc). Integrated: `bike.engine.launch`, `.shift` (ECU fuel cut on/off in 50 ms,
blip x1.29), `.redline` (600 at 16,028 rpm, 96 kHz, finite), `.firing` (13 engines exact), `.lash`.
**Builds**: 13 engines `content/builds/engines/bike-*.json` (I4 screamer, crossplane, 600; V-twin 90, 45; twins 180,
270, 360; triple; V4 screamer, big bang; boxer; single) from `tests/sim/bike/make_builds.py`; 6 bikes
`content/builds/bike-*.json` (superbike, supersport-600, naked-twin, cruiser, adventure-twin, small-single) with E's ITBs.
**Cost**: bike vehicle ~0.014 R (car model ~0.028 R same run; tee-bench --engine, repeat 5).

**Not done / wanted**:
- D/E: the engine gives bike engines 50-65 % of their class power (litre four 72 kW at 11,500 vs ~145; breathing falls
  above ~9,000 rpm, ITBs no help). So the integrated superbike only nears its wheelie (9.3 of 11.0 m/s^2).
- G: the builder's 4-2-1 isn't read by the engine yet; bikes use the dials (headers 0.25, pipe 0.2, muffler 0.15).
- `sim/README.md` (D): Car's entry should list `lashStrike()` and the bike dispatch.
- Pre-existing, not mine: `sim: every Spec field has its range...` fails on `exhaust.*` and `car.gears`.

**Spec** (spec.car, after card I's): riderMass, riderHeight, riderForward, tuck, cdATuck, pitchInertia, bikeTyre,
primaryRatio, frontSprocket, rearSprocket, chainEfficiency, chainLash, slipper, slipperTorque, quickshifter,
quickshiftCut, autoBlip, wheelieControl, wheelieAngle. Bike meaning of card I's: brake = front lever, handbrake = rear.
**Telemetry** (tel.car, after card I's): liftAngle, liftRate (deg, deg/s), bikeFlags (1 hop/lock, 2 looped, 4 wheelie
control, 8 slipper, 16 quickshift, 32 blip, 64 stoppie), chainLash (counter), chainLashSpeed, chainLashSide,
backTorque, slipperSlip, quickshifts, blips, wheelieLimit. Wheel arrays: [0] front, [2] rear. Side view: wheelbase,
liftAngle (pitch about the rear contact). Mechanical voices: `Car::lashStrike()` per sample.

**FMinus**: no issue met.
**Listen to**: a quickshift at 12,000 rpm on the superbike (the 50 ms cut), an auto-blip downshift, the crossplane four
against the screamer at the same revs, the V4 big bang, the 45-degree twin at idle, the 600 at 16,000 rpm, the chain's
clunk on and off the throttle in first, a clumsy downshift without a slipper (the rear chattering). Nobody has heard any.
