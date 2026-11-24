# Card I: the vehicle, in a straight line

*Wave 1. Repository: TheEngineEngine, integration branch `claude/awesome-gates-ntdb58`. Read
[01-rules.md](01-rules.md), [02-contracts.md](02-contracts.md), then `sim/README.md` (K3's: the `Car` module's
seam), then the reference's `sim/ref/car.fm` and the driving part of `sim/ref/engine.fm` (`drive16`,
`followLever`) with their comments: they are your baseline and the stability lessons are in them.*

**OWNS**: `sim/vehicle/**`, `sim/spec/car.fm`, the `tel.car` section, `tests/sim/vehicle/**`,
`content/builds/cars/**`, `docs/models/vehicle.md`.

**Budget**: 0.05 R a sample with every part in use (02-contracts.md §9).

## The seam (it exists already; keep it)

`car.step(w, tq, dt)`: once a sample, the crank's speed and the torque on it (the cylinders' less friction, plus
the starter) in; the crank's speed after the vehicle has had its say out. `car.control(...)` every 16 samples:
the pedals, the lever, the shift counters, the mode. `car.road()`: one sample of what the road and the driveline
sound like. To the ECU (card H) you offer, through functions named in `sim/README.md`: the driven wheels' slip
(for traction control), the gear, whether a shift wants an ignition cut or a blip, the speed. From the Spec you
read `spec.car` and `spec.env` (air density, surface, grade); from `Mechanics` the engine's inertia.

## What the reference does, and what changes

One tyre carries the whole car at 1.05 g with a curve that never falls off; no weight moves; six fixed gears chosen
by a rule; drag only. That stays selectable, bit for bit, as `car.model = 0` ("reference"), which is the Spec's
default. `car.model = 1` is everything below; every build in `content/builds/cars/` uses it.

## The model

**Bodies and states.** The gearbox's input; the propeller shaft's wind-up with its lash (the reference's, kept);
each driven wheel's speed (two an axle); the car's speed and distance; the pitch (a first-order lag on the load
transfer, about 0.2 s); each driven tyre's temperature.

**Tyres.** Longitudinal slip with a relaxation length so it behaves at a standstill (the car holds on a hill by
its brakes, creeps nowhere, and does not ring). Force `Fx = Fz * mu(slip)`, `mu` by the Magic Formula
(`D sin(C atan(B s - E (B s - atan(B s))))`) with a peak and a falloff. Parameters by compound, as a `const`
table: economy, street, sport, semi-slick, slick, drag radial, drag slick, winter, truck, motorcycle; each with
its peak, shape, load sensitivity (grip falls a little as load rises), rolling resistance, the temperature it
works at and how much it loses cold or overheated, and its grip on each surface (dry, damp, wet, snow, a prepared
strip). A tyre warms by the power it scrubs and cools with speed: a burnout brings a slick in. State the numbers
and where each came from in `docs/models/vehicle.md`; where a number is a judgement, say so.

**Loads.** Static by the mass, the wheelbase and the centre of mass; under acceleration `m a h / L` moves from
front to rear through the pitch lag; aero lift or downforce at each axle adds; a live axle's torque reaction
unloads one rear tyre (an option). The front load cannot go below zero: then the acceleration is what the wheelie
allows and a flag says so (a motorcycle's launch limit, wave 2, is this same code).

**Drive.** Front, rear or all wheels. One differential element used three times (centre, front, rear): a torque
split and a locking torque `preload + kSpeed * dOmega + kTorque * |T|`. All zero is open; a preload and a torque
term a clutch-pack; a speed term a viscous unit; locked is locked. A small fixed asymmetry of grip left to right
(and `spec.env`'s split surface) is what lets an open differential spin one tyre.

**Clutch and gearboxes** (`gearbox.kind`):
- *Manual*: the reference's clutch and synchromesh; a gear will not go in with the clutch up and the speeds apart.
- *Sequential dog box*: shifts by the counters; an upshift under power asks the ECU for a cut of some 60 ms and
  needs no clutch; a downshift asks for a blip. Its engagement is abrupt: the driveline's clunk is its sound.
- *Dual clutch*: the next gear's clutch takes the torque over as the first lets go, 100 to 200 ms, no interruption.
- *Torque-converter automatic*: the impeller's torque `K(nu) * w^2` and the torque ratio against the speed ratio
  `nu` (about 1.9 at stall down to 1 at the coupling point, as a table); a lock-up clutch; a shift schedule by
  speed and pedal with kickdown; shifts as overlaps of some 0.3 to 0.5 s.
- *CVT*: the ratio moves to hold the engine at a speed the pedal sets, within its span, at a belt's efficiency.
- Ratios, reverse, final drive and the number of gears (1 to 10; the truck's 18) are the Spec's; `autoGearing`
  (default on) is the reference's rule that gears a car for its engine. The paddle logic, the automatic clutch
  for players without a clutch pedal, and the truck's splitter are the reference's, generalised.

**Brakes.** Front and rear with a bias; ABS on (the reference's "short of the grip") or off (a wheel can lock);
the line lock (front brakes held: `BTN_LINE_LOCK`) and the handbrake (rear).

**Resistance.** Drag `0.5 rho CdA v^2`, rolling by compound and speed, the grade.

**Modes.**
- *Drive*: the road.
- *Chassis dyno* (`MODE_DYNO_CHASSIS`): the driven wheels on a roller with inertia and an absorber that holds
  `ctl.dynoSpeed` or sweeps at `ctl.dynoRate`; the body does not move; telemetry gives torque and power at the
  wheels, so the loss through the driveline and the tyres' slip can be read.
- *The strip*: each increase of `ctl.strip` stages the car at the line, at rest. A tree (three ambers half a
  second apart, then green; a pro tree as an option); a start before green is a red light. Timers: reaction (the
  green to the front wheel leaving the beam, a roll-out of 0.3 m), 60 ft, 330 ft, the eighth mile with its speed,
  1000 ft, the quarter mile and the trap speed (the mean over the last 66 ft). All in `tel.car`, with the run's
  state.

**Sound sources** (signals for wave 2's listener; today mixed as the reference mixes `road()`): the road's
rumble by speed and surface; each driven tyre's squeal and scrub by its slip; the driveline's clunk; the wind,
rising steeply with speed.

## Telemetry

Speed, acceleration, distance, gear, the shift's state; for each wheel its speed, slip, load, force and
temperature; the clutch's or the converter's slip; the shaft's torque; power at the wheels; flags (wheelspin,
wheelie, ABS, lock-up, a shift in progress); the strip's timers.

## Tests

Standalone first (a harness that drives `Car` with a made-up torque curve, so you need no engine):
1. The tyre's curve by compound and surface: where its peak is and what it falls to.
2. The differential element: open, clutch-pack, viscous and locked on a split surface; torque adds up.
3. Loads against a hand calculation at rest, at 0.5 g and with downforce at speed; the wheelie's limit.
4. A coast-down against the analytic solution; top speed against power and drag; energy accounted (work in equals
   kinetic energy plus every loss) to 1 %.
5. Each gearbox: the converter's stall speed against its `K`; a dual-clutch shift without a hole in the torque; a
   sequential upshift's cut requested and released; the CVT holding its speed.
6. The strip's timers against a car of constant acceleration (the formulas are exact there).
7. Standing on a 10 % grade on the brakes for a minute: no creep, no ringing. Ten minutes of random pedals and
   shifts: finite, bounded.

Then with the engine (K3's `EnginePlayer`): `car.model = 0` is the null test's reference; three archetype cars
(a light front-drive four, a rear-drive V8, an all-wheel-drive turbo six) launch in the order traction says
(all-wheel, rear, front, at equal power and weight, when grip is the limit), and their quarter-mile times and trap
speeds sit within 8 % of what weight and power predict by the usual rule of thumb (ET = 5.825 (lb/hp)^(1/3),
mph = 234 (hp/lb)^(1/3)) on tyres that hook; a 400 kW rear-drive car on street tyres spins them in first gear; a
dumped clutch at idle in a tall gear stalls, as the reference does.

## Not in this card

Steering and cornering (not in v1). The engine dyno (card D). Launch and traction control's logic (card H; you
give it slip). Gear whine and wind as heard (wave 2). Motorcycles' and trucks' extras (wave 2), though nothing
here should make them harder.
