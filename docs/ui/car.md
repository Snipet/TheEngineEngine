# CAR (`ui/views/car/`)

*Package U4. The vehicle as built: four drawings from the Spec's fields, moved by telemetry.*

## What the stage shows

- **The car from the side, to scale**: the body as `look::carProfile`'s outline (DRIVE's car), squatting or diving a
  few px with the load moved, the wheels at the tyre's radius on the
  wheelbase, the centre of mass (a disc with a cross, "CM") at its height and place; the wheelbase dimensioned. Under
  each axle a bar of its live load as a share of the weight, a tick at its load at rest, the number and the share;
  under them "SQUAT" or "DIVE" with the load moved (`car.pitch` where the program gives it). Drag as an arrow back from the tail and downforce as arrows on the
  axles, with numbers, at the speed the car is doing.
- **The grip curve**: force over load against slip for the chosen compound on the chosen surface (the tyre model's
  own Magic Formula and table, `sim/vehicle/tyre.fm`: S over its peak, held at its value at slip 1 beyond), its peak marked, the dry curve dashed when the road is not
  dry; each driven wheel's live point, green below the peak, amber and "PAST THE PEAK" beyond it.
- **The driveline**, from above, front at the left, for the layout chosen: engine, clutch (converter, belt), gearbox
  with its gear, and front, rear and centre differentials by kind (OPEN, LSD for a clutch pack, VISCOUS, T. BIAS for torque-biasing, LOCKED),
  shafts to the wheels; driven wheels filled; the torque reaching each axle; a slipping wheel outlined in amber with
  "SLIP 16 %".
- **The gearing chart**: road speed against engine speed for each gear to the limiter, each gear's speed there at the
  right, the gear in use in the accent, where the car is now; shift points (where the next gear pulls harder, from
  the predicted torque); the top speed as a dashed line.

## What is clickable

The body (mass and geometry), a wheel (tyres), a caliper (brakes), the drag arrow (aero), the grip curve (tyres),
the driveline (differentials; its gearbox and clutch: gearbox), the gearing chart (gearbox and ratios). The selection
is a group path (`car.body`, `car.tyres`, `car.diffs`, `car.gearbox`, `car.aero`, `car.brakes`); `inspectorPage()`
lists that group's fields the build has, as a generated page.

## What is derived

Static axle loads (mass, weight on the front), drag and downforce (`0.5 rho v^2 CdA`, `ClA`, rho 1.2), the shift points
and the top speed: the highest speed at which the predicted power through the gearbox's efficiency meets drag and
rolling resistance (the compound's coefficient), over every gear, or the limiter's speed when it gets there first;
labelled "derived". A field not in the build is drawn from the Spec's default and starred.

Tests: `ui.car.stage`, `ui.car.large` (with `car.paper`), `ui.car.select`, `ui.car.topspeed`, `ui.car.absent`.
