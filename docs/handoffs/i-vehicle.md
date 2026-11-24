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

# I: finding X-007 and the automatic's hunting (fixed)

- **X-007** (a custom single, 18.4 kg piston, light flywheel, model 1, automatic or CVT, ran away in neutral). Two
  causes: `Car::step` stepped the crank with the dial's inertia while `tq` carried the moving masses' inertia torque
  (made for their real inertia: it makes energy against another), and the converter's explicit torque could overshoot
  the speeds' meeting. Now `Car::step(w, tq, dt, jCrank)` takes `mm.jNow` (engine.fm: that call's arguments only, 0 on
  the plain path) and `VehConverter::limit` caps the converter each sample at what brings its sides together. Also: what
  joins the gearbox's output takes its momentum from the wheels and car (energy can only fall), and the shaft and
  output turn with the wheels in neutral. My render of an X-007-like build (built from the description; the finding's
  file was not on the branch) with the tour: no trips, no stops, in the shipped build and with the checks.
- **Tests**: `vehicle.passive` (rig crank of varying inertia, 5 gearboxes x light/heavy flywheel x light/heavy pistons
  x neutral/gear, random pedals, 40 runs of 6 s: energy never above the engine's work by more than 2 %, the float
  crank's rounding at a chattering limiter; crank under 3000 rad/s) and `vehicle.passive.engine` (custom singles at
  their Spec's corners x 5 gearboxes x neutral/gear, checks on, 40 runs of 5 s: no stop, no refused block, crank under
  3000 rad/s, audio finite). With the two fixes taken out the second fails on X-007's corner (automatic, CVT; 8809 rad/s).
- **Hunting**: an automatic shifts up on the engine's own speed (or 97 % of the redline), down on the speed the car's
  speed gives its input, only 10 % under the car's speed at the upshift into the gear, never while a driven tyre is past
  its peak; the kickdown on the car's speed too. `vehicle.gearbox.nohunt`: 200 kW on economy tyres, front and rear
  drive, 15 s of full pedal spinning: ups only.
- Full null test green before the commit, quick null after each merge; `ctest -R '^vehicle\.'` 40 of 40.
- For the owner of `sim/vehicle/bike.fm` (wave 2's motorcycle, in `Car::b1`): `b1.step(w, tq, dt)` has the same
  inertia question with a custom engine; it should take `jCrank` as `m1` does.

# I: a launch under traction control (X-011), FLAG_FAULT in passive.engine, D2's eighteen-speed

- **c0f95c2** `Vehicle::launch`: traction control on + the car's own clutch (paddles' robotised, dual clutch; not the
  dog box, the automatic or the driver's pedal): from a standstill until the clutch has locked 0.1 s, the clutch passes
  at most what the driven tyres carry at the ECU's slip target, PI-trimmed on the slip; the stall guard narrows to 50 to
  250 rpm over idle. `Car::traction(on, slip)`: one line in `Engine::control16` (README's Car entry). Reference V8 in the
  coupe, first, floored, 2 s: wet 5.51 m/s, mean slip 0.081 (off: 4.31, 1.00); dry 9.33, 0.086 (off: 6.99, 1.00); from
  rolling (H's) 11.58, 0.100 (off: 8.48). H10's standstill test passes without [!mayfail] (its comment, H's, still says
  "Expected to fail"). `vehicle.passive.engine` also requires no FLAG_FAULT and HostStats::engineFaults 0; re-proven in a
  scratch copy: fixes out, the automatic's runs fail (392 restarts in 5 s); in, all pass.
- **51908c2** `gearbox` choice 5 "Eighteen-speed" (appended): `Vehicle::eighteen` drives D2's Roadranger; reverse 13.1
  at a crawl in Vehicle (not in rr); `Car::grindOut()` for L. Tests `vehicle.gearbox.eighteen`, `vehicle.passive` (48 runs
  with it). For D2: `ctl.lever` 5 is the L slot, buttons 16 splitter, 32 range; the crank seen is the last sample's.

# I: a launch that does not bog (0078996)

Under its launch speed (ecu.launch.speed with launch control on, else 60 % of the redline) and with the pedal floored, a
launch's clutch passes at most what the engine makes beyond what takes it up to that speed in 0.1 s; the launch's
clutch is sized on the most torque the engine has shown; the launch goes on through an upshift taken before the clutch
locks. P's drag runs (its qm scenarios, its shift times as placed; first gear's numbers from the green):

| Build | 60 ft before / after | 0-72 km/h | 0-100 km/h | 1/4 mile | launch slip 0-2 s (after) |
|---|---|---|---|---|---|
| rotary NA side port | 3.29 / 2.59 | 7.45 / 6.02 | 19.26 / 17.31 | 20.18 / 18.60 | 0.03 (engine-limited) |
| v12-65 | 2.43 / 2.37 | 4.69 / 3.96 | 6.13 / 5.32 | 12.91 / 12.30 | 0.09 |
| big-bore 7.0 pushrod | (P: 13.10) | | 5.50 | 12.47 | 0.085 |
| flat-plane 3.9 TT | (P: 11.99) | | 5.07 | 11.61 | 0.067 |

Why the strong cars were slow: a dual clutch's 1-2 handover taken by P's fixed shift time while the launch clutch still
slipped closed at full capacity and spun the tyres for 0.8 s (now the launch continues through it); the launch holds
the target slip on the more-slipping wheel of an open diff (V12: right 0.10, left 0.07); the V12's semi-slicks are cold
(293 K against a window from 320: 12 % of their grip); every run starts from idle at the green (no pre-rev: launch
control would). The rotary's quarter is now set by P's shift times (placed for the old launch: it sits 0.9 s on the
limiter, then three shifts in 1.5 s) and by its clutch (1.35 x the engine's own estimate of its peak) slipping after the
launch: P should re-place the shifts and set car.clutchTorque.
