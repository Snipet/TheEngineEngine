# The vehicle

Everything from the flywheel to the road, in a straight line: clutch or torque converter, gearbox, differentials,
shafts, tyres, the car's mass and how its load moves between the axles, aerodynamics, brakes, the chassis dyno and the
drag strip's timers.

Vehicle model (`car.model`): **Vehicle** is the model below, used by the shipped archetypes. **Reference** is the
original simulator's car, kept so the two can be compared.

## What is simulated

- **Couplings** (clutches, a converter's lock-up, an automatic's clutch packs, brakes, a differential's locking) that
  pass only as much torque as they can hold and never add energy: a clutch slips, holds, or holds a car on a hill.
- **Shafts** with a little wind-up and play between the gearbox and the wheels, so a dumped clutch makes the car buck.
- **Tyres**: grip against slip by Pacejka's Magic Formula, fitted per compound (economy, street, sport, semi-slick,
  slick, drag radial, drag slick, winter, truck, motorcycle); grip falling a little with load; the tread's own
  springiness; the tread temperature, which a burnout raises; surfaces (dry, damp, wet, snow, prepared strip).
- **Load transfer**: rearward under acceleration (with a short delay standing in for the suspension), forward under
  braking, with the grade; downforce by speed; a live rear axle's twist loading one rear wheel; the front lifting (a
  wheelie limit).
- **Differentials**: open, clutch-pack limited slip, viscous, torque-biasing, locked; front, rear and centre.
- **Gearboxes**: manual (with synchromesh that refuses a gear at the wrong speed), sequential dog box (with an
  ignition cut on upshifts and a throttle blip on downshifts), dual clutch (no gap in drive during a shift),
  automatic with a torque converter, CVT. Ratios can be set or chosen for the engine.
- **Brakes** with bias and ABS; drag; rolling resistance.
- **A chassis dyno** (a roller under the driven wheels) and **the drag strip**: the tree and every timer of a time
  slip.
- **Sounds**: tyre rumble and squeal, driveline clunk, wind.

Measured: a coast-down matches the exact solution to 0.01 %; energy in equals energy out within 0.1 %; quarter miles
within 8 % of the usual rule of thumb from weight and power (a front-drive 1.6 L four 17.25 s at 80 mph; a rear-drive
V8 14.63 s at 103 mph; an all-wheel-drive turbo six 12.07 s at 121 mph).

## What is left out

- Steering and cornering, combined slip, camber, tyre pressure and wear, real suspension.
- The differentials' own friction and inertia; active differentials.
- Motorcycles have their own additions (rider, chain, wheelie control, slipper clutch, quickshifter); see the bike
  model notes in the developers' documentation.

## What the numbers mean

| Setting or reading | Meaning | Typical |
|---|---|---|
| Mass (`car.mass`) | With the driver | 1100 to 2000 kg cars |
| Centre of mass height, Weight on the front | Set how much load moves when accelerating | 0.45 to 0.55 m; 0.40 to 0.62 |
| Drag area (Cd A) | Drag coefficient times frontal area | 0.55 to 0.75 m² cars |
| Tyres, Rolling radius | Compound, and the wheel's effective radius | 0.29 to 0.35 m |
| Final drive, Ratios | Overall gearing | Final drive 3 to 4.5 |
| Wheel slip | (wheel speed - road speed) / road speed | Peak grip near 0.08 to 0.15; much beyond is wheelspin |
| Wheel load | Each tyre's share of the weight now | |
| Load moved to the rear | Squat under acceleration | |
| 60 ft, 1/4 mile, Trap speed | The time slip | 60 ft 1.1 to 1.3 s on drag slicks; 2 s or more on street tyres |

Peak grip on dry asphalt: road tyres 0.8 to 1.0, warm semi-slicks and slicks 1.2 to 1.5. On wet, road tyres keep
about half to two thirds of that; on snow, summer tyres 0.2 to 0.3 and winter tyres about 0.45.

## Known gaps today

- The chassis dyno is not yet a driven mode in the running engine, so DYNO's chassis runs do not work yet.
- An automatic whose tyres spin can shift up on the flaring output speed, and down again as they grip.
- Tyre data are typical values inside published ranges, not one real tyre's measurements.
