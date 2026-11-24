# CAR

The vehicle the engine drives: its mass and where it sits, the tyres, the driveline, the gearbox and its ratios,
aerodynamics and brakes. Four drawings, made from the build's settings and moved by the running car.

## What you see

| Drawing | What it shows |
|---|---|
| The car from the side, to scale | The body squatting or diving a little as load moves, the wheels at the tyre's size on the wheelbase, the centre of mass (a disc with a cross, "CM") at its height and place, the wheelbase dimensioned. Under each axle a bar of its load now as a share of the car's weight, with a tick at its load at rest; under them SQUAT or DIVE and how much load has moved. At speed, drag as an arrow back from the tail and downforce as arrows on the axles, with numbers. |
| The grip curve | Grip (force over load) against wheel slip for the chosen tyre on the chosen surface, the peak marked; the dry curve dashed when the road is not dry. Each driven wheel's live point sits on it: green below the peak, amber with "PAST THE PEAK" beyond it. |
| The driveline | From above, front at the left, for the layout chosen: engine, clutch (or torque converter, or belt), gearbox with its gear, the front, rear and centre differentials by kind (OPEN, LSD, VISCOUS, T. BIAS, LOCKED), shafts to the wheels. Driven wheels are filled; the torque reaching each axle is written; a slipping wheel is outlined amber with its slip ("SLIP 16 %"). |
| The gearing chart | Road speed against engine speed for each gear up to the limiter, each gear's speed at the limiter at the right, the gear in use highlighted and the car's point on it. Shift points (where the next gear pulls harder, from the predicted torque) and the top speed as a dashed line. |

The top speed is worked out, not measured: the highest speed where the predicted power (less gearbox losses) meets
drag and rolling resistance, or the limiter's speed in top gear if that comes first. It is labelled "derived". A
setting the build does not have is drawn from its default and starred.

A motorcycle (Vehicle set to Motorcycle) is drawn as a bike with its rider, its front lifting in a wheelie, a chain
drive and a slipper clutch where it has one.

Typical values: a road car's drag area (Cd A) is 0.55 to 0.75 m²; weight on the front 0.55 to 0.62 for front-engined
front-drive cars, about 0.40 for mid- and rear-engined ones; peak grip of road tyres on dry asphalt 0.9 to 1.1, with
the peak at about 10 % slip.

## What you can change

| Click | Group | Main settings |
|---|---|---|
| The body | Mass and geometry | Mass (`car.mass`), Driven wheels (Front, Rear, All), Wheelbase, Centre of mass height, Weight on the front, Track |
| A wheel, or the grip curve | Tyres | Tyres (Economy, Street, Sport, Semi-slick, Slick, Drag radial, Drag slick, Winter, Truck, Motorcycle), Rolling radius, Wheel inertia |
| A caliper | Brakes | Brakes, Brake bias to the front, ABS |
| The drag arrow | Aero | Drag area (Cd A), Downforce area (Cl A), Downforce on the front |
| The driveline | Differentials | Torque to the front, and for centre, front and rear: preload, viscous, torque bias, locked; Live rear axle |
| The gearing chart, or the gearbox | Gearbox | Gearbox (Manual, Sequential, Dual clutch, Automatic, CVT), Forward gears, Gear it for the engine, Ratios, Final drive (`car.finalDrive`), Clutch capacity, Shift time, the torque converter's and CVT's settings |

A differential with every setting at 0 is open; preload and torque bias make a clutch-pack limited slip; the viscous
term a viscous unit; Locked a spool. "Gear it for the engine" (on by default) chooses the ratios and final drive from
the engine's power.

The vehicle has a switch, Vehicle model (`car.model`): **Vehicle** is the model described here; **Reference** is the
original simulator's simpler car, kept so the two can be compared. The road (surface, grade, air) is set on
[DRIVE](drive.md).

## Things to try

1. **Weight transfer.** On "Supercharged 6.2 L V8", launch hard in first on DRIVE, then look here: the rear axle's bar
   rises above its tick, the front falls, SQUAT shows how much moved. Raise Centre of mass height and repeat.
2. **Tyres.** Switch Tyres from Street to Drag radial and compare the grip curve: the peak moves to more slip and
   higher grip. Launch again and watch the wheel points stay left of the peak.
3. **An open differential on a split road.** Set Surface, left side to Snow on DRIVE, choose an open rear
   differential, and pull away: one wheel goes amber and spins. Then set Rear locked, or a Rear preload, and repeat.
4. **Final drive.** Lower Final drive by 0.3 on "1.8 L twin-cam roadster". The gearing chart's lines lean over, each
   gear reaches a higher speed at the limiter, and the derived top speed moves.
5. **Downforce.** Give "V12 6.5 L" a Downforce area of 1.0 m² and drive fast: the downforce arrows grow with the
   square of speed, and drag rises with it.

## What is not there yet

- Only straight-line driving: there is no steering or cornering, no combined slip, no suspension (the pitch is a
  delay on the load transfer, not a body on springs), no tyre wear or pressure.
- An automatic whose tyres spin can shift up on the flaring speed and down again as they grip.
