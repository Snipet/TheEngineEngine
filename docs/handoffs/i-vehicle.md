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
