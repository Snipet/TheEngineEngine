# The vehicle, in a straight line (card I)

`car.model = 1`: everything from the flywheel to the road, in `sim/vehicle/` (`vehicle.fm`, module `Vehicle`, and what
it is built of). `car.model = 0`, the Spec's default, is the reference's `car.fm`, untouched. This page says what the
model computes, where each number came from, which numbers are judgement, and what the tests measured. Nobody here can
drive or hear: everything claimed below is a measurement of the model, not of a car.

## Files

| File | What |
|---|---|
| `vspec.fm` | `VehSpec` (the fields appended to `spec.car`), `VehEnv` (what it reads of `spec.env`), `VehTel` (appended to `tel.car`) |
| `tyre.fm` | the compounds' table, the surfaces' table, the slip curve (`VehTyreCurve`), one tyre (`VehTyre`) |
| `diff.fm` | the coupling law (`vehCouple`) and the differential's settings (`VehDiff`) |
| `converter.fm` | the torque converter (`VehConverter`) |
| `gearbox.fm` | the five gearboxes' logic (`VehGearbox`), every 16 samples |
| `strip.fm` | the tree and the timers (`VehStrip`) |
| `sound.fm` | rumble, squeal, clunk and wind, with filters of its own |
| `vehicle.fm` | `Vehicle`: the bodies, the per-sample step, the loads, the brakes, the dyno, the telemetry |

The model reads only its own `VehSpec` and `VehEnv`; the `Car` seam copies `spec.car` and `spec.env` into them when
their `rev`s move. So the same code runs standalone (`tests/sim/vehicle/rig.fm`, a made-up engine) and in the engine.

## The bodies and the step

Bodies, each with its speed: the crank (the engine's; `step(w, tq, dt)` takes its speed and torque and returns its
new speed), a converter's turbine while it is not joined to the gearbox, the gearbox's output (at the wheels' side of
the final drive; its inertia is the propeller shaft's and crown wheel's, 0.05 kg m^2 at the final drive's input,
judgement, plus the reference's 0.015 kg m^2 of gearbox input through the ratio squared), four wheels, and the car (or a
chassis dyno's roller, 1500 kg equivalent, judgement).

What joins them:

- **Couplings** (clutches, lock-up, a gear's clutch packs and bands, brakes, a differential's locking): each sample a
  coupling passes the torque that would bring its two sides to one speed by the sample's end, given every other torque
  on them, clamped to its capacity (`vehCouple`). It never overshoots, so it only takes energy out; it holds a load at
  rest exactly; it needs no locked/unlocked state. A gear's efficiency (0.97 by default; a CVT's belt 0.90) takes its
  share of the power whichever way it flows.
- **The shafts**: the reference's spring with play between the output and the differentials, 9 N m/rad a kg of car
  at the wheels (20 for a truck), 0.009 rad of play (0.015), damped to 0.4 of critical for the gear in, pushing only.
  Unlike the reference, a shift does not unwind them (a dual clutch's handover would otherwise have a hole).
- **The tyres** (below), each a force from its tread's deflection.

Order in a sample: the tyres' and the shafts' forces from where they stand; the clutch, converter and gears; the
centre differential, then each axle's; the brakes and the rolling resistance; every speed moves; then the shafts' wind-up
and the tyres' deflections move by the new speeds (semi-implicit Euler). Speeds of the output, wheels and car, the
distance and the tyres' deflections are doubles: in floats, at 96 kHz, a coast-down lost its rolling resistance below
the speed's resolution (it decelerated in steps of 0.18 m/s^2) and a car held on a hill drifted by tens of micrometres.

## Passivity at the crank (finding X-007)

A custom single with a 18.4 kg piston on a light flywheel ran away through an automatic in neutral (1395 to 14,594
rad/s in 8 ms). Two causes, both fixed: the crank was stepped with the inertia `drive16` gave (the dial's) while the
moving masses' inertia torque, made for the crank's real inertia round the turn, was in its torque, which makes energy;
`step` now takes the crank's inertia of the moment (`mm.jNow`). And the converter's torque, explicit from the speeds as
they stand, could overshoot where the inertia on a side is small against it: `VehConverter::limit` scales it each
sample to at most what brings the impeller and the turbine to one speed within the sample. A body that joins the
gearbox's output (a gear's input by a synchromesh or a dog, a turbine at the end of a shift) is brought to its speed
with the momentum taken from the wheels and the car, so the energy can only fall; the propeller shaft and the output
turn with the wheels in neutral too. Tests `vehicle.passive` (a crank of varying inertia in the rig: the energy never
stands above the engine's own work by more than 2 %, the crank's float rounding at a chattering limiter) and
`vehicle.passive.engine` (custom singles at their Spec's corners with each gearbox, in neutral and in gear, the checks
on: no stop, no refused block, the crank under 3000 rad/s); with the two fixes taken out, the latter fails on X-007's
corner with the automatic and the CVT.

## Tyres

`Fx = Fz mu S(kappa)`, with `S` Pacejka's Magic Formula `sin(C atan(B k - E (B k - atan(B k))))` scaled to a peak of 1
and tabulated (129 points over a slip of 0 to 1; held beyond). `kappa = u / sigma`, `u` the tread's deflection:
`du/dt = (w R - v) - |v| u / sigma` (the relaxation-length model, Pacejka, *Tire and Vehicle Dynamics*, ch. 7), which
is a spring at a standstill: a car held on its brakes leans on its tyres and stays. Below 2 m/s a damping of half
critical acts on the deflection's rate. `mu` is the compound's peak times the surface's factor, times the temperature's
factor, times `1 - loadSens (Fz - Fz0) / Fz0` (never below half). The left wheels grip 1 % more than the right
(judgement: what lets an open differential choose which wheel to spin).

| Compound | B | C | E | peak slip | tail S(1) | mu (dry) | load sens. | Fz0 N | Crr | T K | +/- K | cold loss | hot loss | sigma m | J/K |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| economy | 15.0 | 1.50 | 0.1 | 0.12 | 0.78 | 0.90 | 0.10 | 4000 | 0.0080 | 320 | 40 | 0.05 | 0.20 | 0.20 | 2000 |
| street | 21.5 | 1.65 | 0.8 | 0.11 | 0.75 | 1.00 | 0.10 | 4000 | 0.0100 | 325 | 40 | 0.06 | 0.20 | 0.20 | 2000 |
| sport | 13.0 | 1.55 | -0.8 | 0.10 | 0.70 | 1.10 | 0.12 | 4000 | 0.0110 | 335 | 35 | 0.10 | 0.22 | 0.18 | 1800 |
| semi-slick | 19.5 | 1.65 | 0.5 | 0.09 | 0.65 | 1.25 | 0.13 | 4000 | 0.0120 | 350 | 30 | 0.18 | 0.25 | 0.16 | 1600 |
| slick | 17.5 | 1.65 | 0.0 | 0.08 | 0.60 | 1.45 | 0.15 | 4000 | 0.0130 | 365 | 25 | 0.30 | 0.25 | 0.15 | 1500 |
| drag radial | 9.5 | 1.75 | 0.5 | 0.16 | 0.65 | 1.30 | 0.08 | 4500 | 0.0150 | 335 | 25 | 0.20 | 0.25 | 0.25 | 1600 |
| drag slick | 6.0 | 1.75 | -0.2 | 0.20 | 0.60 | 1.30 | 0.08 | 4500 | 0.0250 | 340 | 20 | 0.35 | 0.30 | 0.35 | 1500 |
| winter | 10.5 | 1.40 | -0.9 | 0.15 | 0.85 | 0.85 | 0.10 | 4000 | 0.0120 | 290 | 30 | 0.03 | 0.30 | 0.20 | 2000 |
| truck | 20.5 | 1.60 | 0.8 | 0.13 | 0.80 | 0.80 | 0.10 | 25000 | 0.0055 | 320 | 40 | 0.05 | 0.20 | 0.40 | 8000 |
| motorcycle | 13.5 | 1.55 | -0.2 | 0.11 | 0.72 | 1.10 | 0.10 | 1500 | 0.0120 | 335 | 30 | 0.15 | 0.25 | 0.15 | 1000 |

Where they came from:

- **B, C, E** are fitted (a grid search, recorded in the commit that made them) to two targets per compound: the slip at
  the peak and what a fully spinning or locked tyre keeps. The targets are **judgement** inside published ranges:
  peak longitudinal slip of 0.08 to 0.15 for road and race tyres, higher for soft, tall-sidewall drag tyres (Pacejka,
  ch. 1 and 4; Milliken and Milliken, *Race Car Vehicle Dynamics*, ch. 2); locked-wheel friction some 0.6 to 0.85 of the
  peak (the gap ABS exists for). The fit is exact to the targets within 0.005 of slip and 0.02 of tail (test
  `vehicle.tyre.curve`).
- **mu (dry)**: judgement within the commonly published 0.8 to 1.0 for road tyres on dry asphalt, 1.2 to 1.5 for
  semi-slicks and slicks warm; a drag slick's 2.6 on a prepared strip (1.3 x 2.0) is what a 1.1 to 1.3 s 60-foot time
  needs. None is one tyre's measured data.
- **Surfaces** (dry, damp, wet, snow, prepared strip): judgement; dry 1, wet 0.5 to 0.7 for road tyres and 0.2 to 0.3
  for slicks, snow 0.2 to 0.3 for summer tyres and 0.45 for winter tyres, a strip's traction compound 1.05 for road
  tyres up to 2.0 for drag slicks.
- **Rolling resistance**: 0.006 to 0.010 for truck and economy tyres and 0.010 to 0.015 for performance tyres
  (published ranges of the coefficient); drag slicks at low pressure 0.025 is judgement. It rises with the square of the
  speed, a quarter more at 70 m/s (judgement, of the published order).
- **Temperature**: the windows and losses are judgement. The tread surface warms by half the power its slip scrubs and
  cools at `(15 + 8 sqrt(v))` W/K (judgement): test `vehicle.tyre.burnout` measures slicks from 293 K to 340 K in 2.4 s
  of a burnout on the line lock.
- **Relaxation lengths** 0.15 to 0.4 m: judgement, of the order Pacejka gives for the longitudinal direction.

## Loads

Static by the mass, the front's share and the grade (its normal part; its slope part moves `m g sin(theta) h / L`
rearward). Under acceleration `m a h / L` rearward, through a first-order lag of 0.2 s (the card's) on the acceleration
measured every 16 samples. Downforce `0.5 rho ClA v^2`, split by the balance. A live axle: the propeller shaft's torque
(the shafts' at the wheels over the final drive) over the track loads the left rear and unloads the right. The front
cannot go below zero: the car accelerates no harder than `g L_front_share / h` (the front's static load times `L / (m h)`),
what the tyres push beyond that is lost to lifting the front (not otherwise modelled), and a flag says so.

## Differentials

One element: a split of the input torque, and a coupling between its two sides whose capacity, the locking torque,
`preload + kSpeed |dw| + kTorque |T|`, is the most the two sides' torques may differ by (the bias torque). All zero is
open, preload and torque term a clutch pack, speed term a viscous unit, `locked` a spool. Used for the centre (its split
`centreSplit`) and each driven axle.

## Gearboxes

`gearbox.fm`'s header says each kind's logic and timings. An automatic shifts up on the engine's own speed and down on the speed the car's speed gives its input, never while a driven tyre is past its peak and only a tenth under the car's speed at the upshift (it hunted 1-2-1 on spinning tyres: test `gearbox.nohunt`). The ratios with `autoGearing` (default on) are the
reference's rule: six gears are the reference's six; sixteen or more its truck's; otherwise geometric (first
`3.15 sqrt(n/6)`, top `0.86 sqrt(6/n)` for six or more, 1.0 for four or fewer: judgement); the final drive puts the
second-highest gear's redline at the speed the engine's power holds against the air and the rolling. Converter: the
capacity table `(K_stall / K(nu))^2` is the shape of the published curves (Kotwicki, SAE 820393), judgement; the torque
ratio falls linearly from `converterRatio` (1.9) at stall to 1 at the coupling point (0.85). Clutch capacity 1.35 times
the engine's peak torque, as the reference; an automatic's bands 1.5 times the stall ratio times it (judgement).

## Brakes, resistance, modes

Brakes: `brakeDecel` g of the car's weight at the tyres' radius at a full pedal (the reference's pedal shape, `b^1.5`),
split by the bias; ABS lets a wheel off (gain times 0.6) each 16 samples it slips past 1.2 times its tyre's peak and
reapplies by 0.15 a step, above 1.5 m/s; the line lock holds the front brakes, the handbrake 0.3 g on the rear. Drag
`0.5 rho CdA v^2`, rolling on each wheel as a holding torque, the grade's `m g sin(theta)`. The chassis dyno: the driven
wheels on a roller of 1500 kg equivalent; the absorber holds the driven wheels' rims at the speed that puts the crank at
`ctl.dynoSpeed` through the gear in (a first-order hold of 0.1 s), or sweeps it at `ctl.dynoRate`, or, with neither, is
off (an inertia dyno). The strip: `strip.fm`'s header.

## What the tests measured (standalone, `ctest -R '^vehicle\.'`)

| Test | Result |
|---|---|
| `tyre.curve`, `tyre.surface` | every compound's peak slip and tail as fitted; surfaces in order; winter best on snow |
| `tyre.braking` | street tyres, dry: ABS 0.997 g, locked wheels 0.754 g (tail 0.75 x mu) |
| `diff.open`, `.clutchpack`, `.viscous`, `.locked`, `.centre` | split surface (left on snow): open torques equal to 1 %, the snow wheel spinning; clutch pack bias = 60 + 0.25 T to 2 %; viscous = 15 dw to 3 %; locked speeds equal; centre 35 % to 0.5 %; torques add up to 1e-4 |
| `loads.rest`, `.accel`, `.lag`, `.downforce`, `.liveaxle`, `.wheelie` | hand calculations to 0.1 to 0.2 %; lag 0.62 of the step at 0.2 s; wheelie: 5.86 m/s^2 against a limit of 5.89 |
| `coastdown` | the analytic solution to 4 digits at 10, 30 and 60 s |
| `topspeed` | 49.17 m/s against power over drag and rolling 49.38 (the tyres' slip takes 0.4 %) |
| `energy` | engine work = kinetic energy + every loss, unaccounted under 0.1 % (front, rear, all-wheel drive; grade, split surface, braking) |
| `gearbox.stall` | converter stall speed against `(w/K)^2` to 0.1 % for K 11, 13.5, 17 |
| `gearbox.dualclutch` | 2nd to 3rd: least torque during the handover 1075 N m against 1497 x 0.743 = 1112 after; the paddles' robotised manual drops to -647 (a hole) |
| `gearbox.sequential` | cut asked for 60.0 ms and released; the downshift's blip to first's speed within 0.1 % |
| `gearbox.cvt` | full pedal: 619 to 628 rad/s against 612 (converter slip 2.5 %); half: 348.5 against 348.5 |
| `gearbox.manual` | refused into first at speed with the clutch up, in with it down; no reverse at speed |
| `strip.timers`, `strip.tree` | constant acceleration: every timer and trap speed to 2e-5 s and 1e-5; red light for leaving early |
| `hill` | 10 % up and down, each drive: settles 0.58 mm into its tyres, then moves under 1e-5 mm in 58 s |
| `soak` | ten minutes (two a gearbox) of random pedals, shifts, surfaces and modes: finite, bounded |
| `dyno.hold`, `dyno.sweep` | crank held at 400.0 rad/s; wheels 0.970 of the engine's power; sweep 100.0 rad/s^2 |
| `cars.files`, `cars.traction` | the three archetypes' files valid; at equal power, weight and tyres the most each accelerates: all-wheel 0.88 g, rear 0.52 g, front 0.50 g |

**Cost**: `tee-tests-vehicle "[.bench]"` (by hand: noisy on a shared machine): 83 to 100 ns a sample with every part in
use (all-wheel drive, every differential, aero, live axle), less the rig's engine, processor time, dev build. The
reference V8 (`tee-bench --quick` at the same time) was 2915 ns a sample: 0.03 R against a budget of 0.05 R.

## Not modelled

Steering and cornering, combined slip, camber, tyre pressure and wear, suspension (the pitch is a lag on the load, not
a body), the differential's own friction and inertia, active differentials, launch and traction control (card H's, on
`slip()`), gear whine and wind as heard (wave 2), a motorcycle's and a truck's extras (wave 2).
