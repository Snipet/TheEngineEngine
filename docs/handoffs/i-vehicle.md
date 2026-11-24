# I: the vehicle, in a straight line (standalone half; integration waits for K3b)

Branch `claude/awesome-gates-ntdb58`. Model: `sim/vehicle/vehicle.fm` (`Vehicle`) and `vspec.fm`, `tyre.fm`, `diff.fm`,
`converter.fm`, `gearbox.fm`, `strip.fm`, `sound.fm`; doc `docs/models/vehicle.md` (equations, every number's source,
what is judgement); cars `content/builds/cars/` (light-fwd-four, rwd-v8-coupe, awd-turbo-six; car part only, model 1).

**Done, standalone** (`tee-tests-vehicle`, `ctest -R '^vehicle\.'`, 32 tests, 40 s; a made-up engine in
`tests/sim/vehicle/rig.fm` through `tee::Program`, its `run` proven nonblocking):
1. `tyre.curve`, `tyre.surface`, `tyre.braking` (ABS 0.997 g, locked 0.754 g on street tyres), `tyre.burnout` (slicks
   293 to 340 K in 2.4 s on the line lock).
2. `diff.open`, `.clutchpack`, `.viscous`, `.locked`, `.centre`: split surface; bias = locking torque to 2-3 %; sums 1e-4.
3. `loads.rest`, `.accel`, `.lag`, `.downforce`, `.liveaxle`, `.wheelie`: hand calculations to 0.2 %; wheelie limit 0.5 %.
4. `coastdown` (analytic to 4 digits), `topspeed` (0.4 % under power/drag: tyre slip), `energy` (unaccounted < 0.1 %).
5. `gearbox.stall` (K to 0.1 %), `.dualclutch` (no hole: least 1075 N m vs 1112 after; paddles' manual: -647),
   `.sequential` (cut 60.0 ms, blip to 0.1 %), `.cvt` (within 2.5 %), `.manual` (synchromesh refuses).
6. `strip.timers` (constant acceleration: 2e-5 s), `strip.tree` (stage, ambers, green, red light).
7. `hill` (10 % up/down, every drive: < 1e-5 mm in 58 s), `soak` (10 min random: finite, bounded).
Also `dyno.hold`/`.sweep` (chassis dyno), `cars.files`, `cars.traction` (equal power/weight/tyres: AWD 0.88 g, RWD 0.52,
FWD 0.50).

**Cost**: 83-100 ns a sample, every part in use, less the rig's engine (`"[.bench]"`, dev build, busy machine); the
reference V8 measured 2915 ns at the same time: about 0.03 R (budget 0.05 R).

**Integration, waiting for K3b** (`docs/handoffs/k3b.md` and no `sim/core/monolith.fm`), in this order:
1. Merge. Append `VehSpec`'s fields (all but `mass`, which exists; `model` first) to `CarSpec` in `sim/spec/car.fm`,
   attributes unchanged; append `VehTel`'s fields to `CarTel` (`tel.car`).
2. In the carved `Car` (`sim/vehicle/car.fm`): a `Vehicle m1;`, `setup` calls `m1.setup`; on `spec.car.rev`/`env.rev`
   copy the fields into `m1.spec`/`m1.env` and `m1.apply()`; `configure`/`control`/`step`/`road` dispatch on `model`,
   model 0 the reference's code untouched; `tel.car` filled from `m1.tel`. Expose `slip()`, `gearNow()`, `cutWanted()`,
   `blipSpeed()`, `speedNow()` in `sim/README.md` for card H.
3. Integrated tests: model 0 in the null test; the archetypes' launch order with real engines; quarter-mile ET and trap
   within 8 % of 5.825 (lb/hp)^(1/3) and 234 (hp/lb)^(1/3); a 400 kW RWD on street tyres spins in first; a dumped
   clutch at idle in a tall gear stalls. With `tee::test::Scenario` (card C's, on the branch).

**Wanted outside my paths**:
- Lead, `sim/spec/env.fm`: `density` (or pressure, kg/m^3 derived), `surface` (Dry, Damp, Wet, Snow, Prepared strip),
  `surfaceLeft` (-1 same), `grade` (rise over run); `VehEnv` has their attributes. Until then the seam passes defaults.
- Lead/K3: the reference's `car.fm` integrates the car's speed in float; at 96 kHz small forces fall under its
  resolution (my coast-down did so before doubles). For "Left as the reference has it".
- Card H: `cutWanted()` (60 ms on a dog box's upshift, a paddle upshift's), `blipSpeed()` (rad/s at the crank), `slip()`.

**FMinus**: no bug met. No `atanf` builtin (and the program may declare no external function): `vehAtan`, a polynomial
within 2e-6 rad, used only when a compound is chosen. Attributes go before an array field's size (as documented).

**Look at / listen to** (after integration): the dog box's clunk on an upshift, a burnout's squeal, an open differential
spinning one wheel on a split surface, the buck of a dumped clutch on the shafts. Nobody has heard any of it.

# I: integration (after K3b), 14:06

Branch `claude/awesome-gates-ntdb58`. `spec.car.model` (0 the reference, default; 1 the vehicle) wired in `Car`
(`sim/vehicle/car.fm`): with 1, `drive16`/`step`/`road` go to `Car::m1` (`Vehicle`); model 0 untouched. One edit to
`sim/engine.fm`: `car.setup(rate, &spec.car, &spec.env, &ctl, &tel.car)`. `sim/README.md`'s Car entry updated.
Null test: full `[null]` green (every pair identical to the bit) before the first push; `[null][quick]` before each.

**Integrated tests** (`tee-tests-vehicle "[engine]"`, tee::test::Scenario): `engine.launch` (reference V8, RWD coupe on
street tyres: spins all of first, none in third), `engine.shift` (dog box: car flag 32 and the ECU's fuel cut on in the
upshift, off after; downshift blips the engine 217 -> 354 rad/s), `engine.coastdown` (analytic to 0.01 %),
`engine.stall` (clutch dumped at idle in fourth: stalls), `engine.quartermile` (drag radials, automatic, burnout; ET /
trap against 5.825 (lb/hp)^(1/3), 234 (hp/lb)^(1/3)): FWD 1.6 L four 17.25 s / 79.9 mph (rule 17.71 / 76.9), RWD V8
14.63 / 103.3 (13.73 / 99.3), AWD turbo six 12.07 / 120.7 (11.93 / 114.3): all within 8 %. Whole `ctest -R '^vehicle\.'`:
37 of 37. **Cost** (`tee-bench --engine`, drive-car, 3 runs): default build 1.049 R, the same with the RWD coupe on
model 1 1.089 R: the model adds about 0.04 R (budget 0.05).

**Spec paths** (spec.car, appended after `kind`, `mass`): `model` (Reference, Vehicle), `drive` (Front, Rear, All),
`wheelbase`, `cgHeight`, `frontWeight`, `track`, `cdA`, `clA`, `aeroBalance`, `tyre` (Economy, Street, Sport, Semi-slick,
Slick, Drag radial, Drag slick, Winter, Truck, Motorcycle), `tyreRadius`, `wheelInertia`, `brakeDecel`, `brakeBias`,
`abs`, `gearbox` (Manual, Sequential, Dual clutch, Automatic, CVT), `gears`, `autoGearing`, `ratio[0..17]`,
`reverseRatio`, `finalDrive`, `clutchTorque`, `gearEfficiency`, `shiftTime`, `converterK`, `converterRatio`,
`couplingPoint`, `lockUp`, `cvtLow`, `cvtHigh`, `cvtEfficiency`, `centreSplit`, `centrePreload`, `centreKSpeed`,
`centreKTorque`, `centreLocked`, `frontPreload`, `frontKSpeed`, `frontKTorque`, `frontLocked`, `rearPreload`,
`rearKSpeed`, `rearKTorque`, `rearLocked`, `liveAxle`, `proTree`.
**Telemetry** (tel.car, after speed, gear, brake, clutch): `accel` (m/s^2), `distance`, `shift` (None, Up, Down,
Refused), `wheelSpeed[4]`, `wheelSlip[4]` (slip ratio), `wheelLoad[4]` (N), `wheelForce[4]` (N), `wheelTorque[4]`
(N m), `wheelTemp[4]` (K) (FL, FR, RL, RR), `clutchSlip`, `shaftTorque`, `wheelPower`, `pitch` (N moved rearward),
`flags` (1 wheelspin, 2 wheelie, 4 ABS, 8 a locked wheel, 16 shifting, 32 cut asked, 64 blip asked, 128 traction
limited), `ratio`, `stripState` (Off, Staged, Ambers, Green, Running, Finished, Red light), `tReaction`, `t60`,
`t330`, `t660`, `v660`, `t1000`, `t1320`, `v1320` (s, m/s).
**The tyre curve** for the interface: `Fx = Fz * mu * S(kappa) / Speak`, `S(k) = sin(C atan(B k - E (B k - atan(B k))))`,
odd in k, held at S(1) beyond |k| = 1, B C E per compound in `sim/vehicle/tyre.fm` (`VEH_COMPOUND` columns 0-2),
normalised by its peak; mu = column 3 x `VEH_SURFACE[compound][surface]` x temperature factor x load factor.

**Not done / wanted**:
- `spec.env` `pressure`, `surface`, `surfaceLeft`, `grade`: not on the branch yet. Editing the lead's `env.fm` myself
  was refused by my session's permission check. Until D adds them, model 1 runs on dry asphalt, 1.2 kg/m^3, level
  ground; `Car::takeSpec` has the four lines to change (density = pressure / (287.05 T)).
- The chassis dyno in the engine: `Engine::readControls` treats only MODE_DRIVE as driven; MODE_DYNO_CHASSIS needs
  `set.drive` too (D's line).
- The ECU's names: D's choice not on the branch yet; Car forwards `m1.slip()`, `speedNow()`, `gearNow()`,
  `cutWanted()`, `blipSpeed()` as soon as the names land. The cut already reaches the ECU through `car.shiftCut`, the
  blip through `drive16`'s pedal.
- Card C: a run keeps only scalar telemetry (`Timeline.cpp`), so `car.wheelSlip[2]` etc. are not columns; my tests
  read the flags instead. Asking for array elements as columns.
- Known gap: an automatic whose tyres spin shifts up on the flaring output speed and can shift down again as they hook.
- `vehAtan` is the candidate for a shared `sim/core/mathx.fm` (to D, through the lead).
