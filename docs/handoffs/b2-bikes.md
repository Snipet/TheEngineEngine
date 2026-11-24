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
`content/builds/bike-*.json` (superbike, supersport-600, naked-twin, cruiser, adventure-twin, small-single). Every one
carries E's lab (valve and intake model Lab, a body a runner; heads and cams by class) and states 300 K, 101,325 Pa.
**Follow-ups (15:05)**: validation targets `content/validation/bike-{superbike,supersport-600,cruiser,vtwin-90}.json`
(class power and torque reported, not failed); listening scenarios `tests/sim/bike/listen/*.json` (README there);
`clutchDry`, `tel.car.sprocketSpeed`, `Car::sprocketSpeed()`; README's Car entry has the bike's part.
**Cost**: bike vehicle ~0.014 R (car model ~0.028 R same run; tee-bench --engine, repeat 5).

**Not done / wanted**:
- D/E: with the lab on the bikes make superbike 77 kW (class ~145), 600 63 (~90), naked twin 45, cruiser 61 (in class),
  boxer 77, single 22; the breathing each needs is tabled in docs/models/bike.md. The integrated superbike only nears its
  wheelie (9.3 of 11.0 m/s^2). Re-run `bike.class.*` on the real engine when D's friction split or E's ram lands.
- G: the builder's 4-2-1 isn't read by the engine yet; bikes use the dials (headers 0.25, pipe 0.2, muffler 0.15).
- Pre-existing, not mine: `sim: every Spec field has its range...` fails on `exhaust.*` and `car.gears`. FMinus: no issue.

**Spec** (spec.car, after card I's): riderMass, riderHeight, riderForward, tuck, cdATuck, pitchInertia, bikeTyre,
primaryRatio, frontSprocket, rearSprocket, chainEfficiency, chainLash, slipper, slipperTorque, quickshifter,
quickshiftCut, autoBlip, wheelieControl, wheelieAngle, clutchDry. Bike meaning of card I's: brake = front lever, handbrake = rear.
**Telemetry** (tel.car, after card I's): liftAngle, liftRate (deg, deg/s), bikeFlags (1 hop/lock, 2 looped, 4 wheelie
control, 8 slipper, 16 quickshift, 32 blip, 64 stoppie), chainLash (counter), chainLashSpeed, chainLashSide,
backTorque, slipperSlip, quickshifts, blips, wheelieLimit, sprocketSpeed. Wheel arrays: [0] front, [2] rear. Side view: wheelbase,
liftAngle (pitch about the rear contact). Mechanical voices: `Car::lashStrike()` per sample.

**Listen to**: the eight scenarios of `tests/sim/bike/listen/` (quickshifts at 12,000, auto-blips, crossplane against
screamer, V4 big bang, 45-degree idle, the 600 at 16,000, the chain's clunk, a downshift without a slipper). Nobody has heard any.
