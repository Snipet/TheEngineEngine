# sim/: the simulator

One FMinus program: `engine.fm` and what it includes. Its one main module is `Engine`
([docs/plan/02-contracts.md](../docs/plan/02-contracts.md), section 1). `sim/ref/` is the reference it was carved from,
never edited: the null test (`tests/sim/NullTest.cpp`) holds the simulator to it, sample for sample.

Status: **K3b done** (`docs/handoffs/k3b.md`): the reference's `Shared` carved into the modules below, the null test
identical to the bit on every pair it plays. The packages (cards D to I) own their modules from here and keep this
file true for them. Card D (the integrator from here: a package wanting a call in the order of work asks D for it, with
its place and its default that does nothing) has added the custom engine, the engine dyno and the mechanics' telemetry
(below, and docs/models/mechanics.md).

## Files

| Path | Module | Owner | Responsibility |
|---|---|---|---|
| `engine.fm` | `Engine` (and `Dials`) | lead and D | the contract's blocks; the rates; the order of work below; the fade; configure (the engine for its layout and size) |
| `spec/` | `Spec` and its sections, `Controls`, `Telemetry`, `Scope`, `Curve16`, `Map16` | the contract | `limits.fm` (MAX_CYL ...), `tables.fm` |
| `core/crank.fm` | `Crank` | D | the crank's angle and speed; the torque's ripple |
| `core/mechanics.fm` | `Mechanics` | D | friction, the bench's load, the starter, the inertia |
| `core/heat.fm` | `Heat` | D | the air, the coolant, the oil, the chamber's walls |
| `core/cylinder.fm` | `Cylinder` | D | one cylinder's gas (or a rotor's face): volume, valves' flows, burn, torque |
| `core/layouts.fm` | `Layout` | D | the seventeen layouts: firing, banks, ports, sizes |
| `core/layout.fm` | `LayoutGeom` | D | a custom layout as geometry (banks, pins, revolutions, places; rotors): firing angles, order, intervals |
| `core/kinematics.fm` | `SliderCrank`, `Sizing` | D | the piston's x, x', x'' and volume; size and bore/stroke dials to bore and stroke; the redline by piston speed |
| `core/inertia.fm` | `MovingMasses` | D | the reciprocating masses: inertia torque, J(theta), the crank's energy-exact step |
| `core/shake.fm` | `ShakeByOrder` | D | shaking forces and rocking moments, first and second order; balance shafts |
| `core/friction.fm` | `FrictionModel` | D | friction by part (Chen and Flynn's form) |
| `core/work.fm` | `CycleWork`, `CylWork` | D | p dV per cylinder per cycle: IMEP, PMEP, BMEP, peak pressure, volumetric efficiency |
| `core/dyno.fm` | `Dyno` | D | MODE_DYNO_ENGINE's absorber: holds or sweeps the crank |
| `valvetrain/valvetrain.fm` | `Valvetrain` | E | each cylinder's valve events and areas: the CAM dial (model 0) or the lab (`valvetrain/lab.fm`, model 1) |
| `intake/intake.fm` | `Intake` | E | runners, plenum, throttle, idle valve; model 1: the air box, throttles a runner, two-length runners |
| `boost/boost.fm` | `Boost` | F | turbos or a supercharger: shaft, boost, valves, drag, sounds |
| `exhaust/exhaust.fm` | `Exhaust` | G | every pipe after a port, the junctions, flow noise, the system's heat, the tails |
| `exhaust/duct.fm` | `Duct`, `Valve`, `lossFlow`, `lossKap`, `lossHold` | G | a pipe as a waveguide; a valve solved with its pipe; a junction's loss |
| `exhaust/pop.fm` | `Pop` | G | a pop of unburned fuel in a collector |
| `combustion/combustion.fm` | `Combustion` | H | the charge at intake closing: fuel, spark, burn's length, knock, pops; diesel |
| `ecu/ecu.fm` | `Ecu` | H | the state, the speed it goes by, idle and redline, the driver, idle valve, trim, limiter, cuts, stall |
| `combustion/heavy.fm` | `HeavyDiesel` | D2 | a heavy diesel's governor (fuel by the rack), compression-release and exhaust brakes, combustion-noise measures (docs/models/diesel.md) |
| `vehicle/car.fm` | `Car` | I | clutch, gearbox, driveline, tyres, the road; the shifter and paddles; the road's sound |
| `sound/extras.fm` | `Extras` | wave 2 | intake mouth, valvetrain, injectors, block, knock, clatter, pump, chain, starter |
| `sound/voices.fm` and `sound/*.fm` | `Voices` (`GearVoice`, `DriveVoice`, `FanVoice`, `FuelVoice`, `ValveVoice`, `GateVoice`, `StarterVoice`) | M | spec.voices.model 1: gear whine and rattle, final drive, belts and cam drive, fan, fuel pump and direct injectors, lifters and float, wastegate, starter; a motorcycle's chain, lash and dry clutch (`BikeVoice`) (docs/models/voices.md) |
| `listen/listen.fm`, `listen/hall.fm`, `listen/scene.fm`, `listen/cabin.fm`, `listen/places.fm` | `Listen`, `Hall`, `Scene`, `CabinBoom`, `Place` | L | views, distance, room, compressor, limiter, ceiling, resampler; model 1: sources in their places, cameras, the cabin, places, the computed level |
| `lib/` | `OnePole`, `DcBlock`, `Biquad`, `Noise` | anyone (read) | filters; the reference's white noise |

Other files in these directories are packages' standalone work, not included by `engine.fm` yet (`core/twostroke/`: card
T's two-stroke, standalone). The carved files'
names were chosen not to collide with them; a package integrating its work replaces or extends the carved module named
above.

## The contract

```c
module Engine
{
    Spec      spec;     // spec/spec.fm: what the machine is. The host writes it between calls and moves a section's rev
    Controls  ctl;      // spec/controls.fm: what the driver does. Written before each call
    Telemetry tel;      // spec/telemetry.fm: what the machine does. Written by process
    Scope     scope;    // spec/scope.fm: cylinder 1's last whole cycle by degree, while ctl.scopeOn is set
    void init(float hostRate);                                                    // allocates: off the audio thread
    void process(float* restrict left, float* restrict right, int n) [[clang::nonblocking]];
    void end();                                                                   // frees: off the audio thread
};
```

- `process` zeroes and writes `n` samples into both buffers, finite and within +-1 (Listen's soft ceiling).
  `process(left, right, 0)` applies the Spec, the Controls and the ignition and renders nothing.
- `tee::EnginePlayer` refuses a program whose `Engine::process` FMinus does not report as nonblocking; FMinus has then
  proved it reaches no `malloc`, `calloc` or `free`. Nothing had to change for that.
- FMinus initialises no field: the host writes every Spec and Controls field's `fminus::default` before `init`.
- A section's `rev` moves when the host writes one of its fields; `Engine::readSpec` copies a section into `set` (the
  `Dials`) only then, at the start of a call.

### The first Spec, the Controls, the Telemetry, the Scope

The Spec is the reference's 32 dials in their sections; `spec/*.fm` has every field with its range, default, label and
unit. SI where 02-contracts.md says: the displacement in m^3 (the dial's litres / 1000), the air in K
(`F = (K - 300) * 1.8 + 80.33`, the reference's own 300 K at 80.33 F). The trims stay in rpm, the timing in crank
degrees, the level in dB, every 0-to-1 dial a fraction with its meaning unchanged.

| Spec field | The reference's dial (`set.` field) |
|---|---|
| `engine.layout`, `.displacement`, `.boreStroke`, `.compression`, `.flywheel`, `.idleTrim`, `.redlineTrim`, `.rough`, `.benchLoad` | `layout`, `size` (L), `bs`, `comp`, `inertia`, `idle`, `redline`, `rough`, `load` |
| `valve.cam` | `cam` |
| `boost.kind`, `.level`, `.size`, `.blowOff` | `asp`, `turbo`, `tsize`, `bov` |
| `exhaust.headers`, `.pipe`, `.crossover`, `.muffler` | `headers`, `pipe`, `xpipe`, `muffler` |
| `fuel.kind` | `fuel` |
| `ecu.timing`, `.pops`, `.starter` | `timing`, `pops`, `starter` (0 or 1) |
| `car.kind`, `car.mass` | `drive` 1 or 2 (in MODE_DRIVE), `mass` |
| `env.temperature` | `ambF` |
| `sound.view`, `.distance`, `.room`, `.level`, `.intake`, `.mechanical` | `view`, `distance`, `room`, `level`, `intake`, `mech` |

What replaces the reference's keys (`Engine::readControls`, `Engine::ignition`, each call):

| Control | What the engine does |
|---|---|
| `mode` MODE_BENCH (and MODE_DYNO_ENGINE: below) | no vehicle (`set.drive` 0); `ctl.throttle` is the throttle by hand (`set.modwheel`) |
| `mode` MODE_DRIVE (and MODE_DYNO_CHASSIS: the vehicle model's rollers) | the car (`spec.car.kind` Car) or the truck: `set.drive` 1 or 2; `ctl.throttle` is the pedal (`set.gas`) |
| `brake`, `clutch`, `lever` | `set.brake`, `set.clutch`, `set.lever` |
| `shiftUp`, `shiftDown` | each increase a press of the right (bit 16) or left (bit 32) paddle: down until `car.lastButtons` shows Car's `drive16` saw it, then up until it saw that; a press counted meanwhile waits its turn |
| `ignition` 2, start | while stopped or running down (state 0 or 3): `keyDown(36, 1)`, the reference's start key: it cranks (without a starter it runs at its idle at once). Held: started again if it stops |
| `ignition` leaves 2 | `keyUp(36)`; still cranking (state 1): state 0, the starter stops |
| `ignition` 0, off | cranking or running: state 3, running down (`ecu.stallT` 0, `ecu.offArmed` false) |
| `ignition` rising from off (voices.model 1 only) | the engine renders from the key's turn, so the fuel pump's prime is heard before a start; and it keeps rendering while `ex.busy()` (card M) |
| `holdSpeed` > 0 (bench, running) | the key driver (`Ecu::driver`) aimed at that speed: a key of the pitch whose firing it is (`set.keys` 1), velocity 1 |
| `mode` MODE_DYNO_ENGINE (running) | on the bench, the absorber (`Dyno`) holds `dynoSpeed`, or sweeps from where the crank is at `dynoRate` (rad/s^2); the bench's load dial and the inertia it adds rest meanwhile |
| `buttons` (`BTN_LAUNCH`, `BTN_NITROUS`), `bakeMaps` | ecu.model 1 only: launch control, nitrous; a bake of the maps into the Scope on each increase |
| `handbrake`, `steer`, `buttons`' other bits, `strip` | not read yet |

`set.hold` is 1e9 s: the engine never switches itself off. `Engine` renders only while it sounds: from a start until
`audible()` is false (stopped, the room rung out, the car at rest) with the key up; otherwise it writes silence and the
Telemetry stands still.

Telemetry (`Engine::writeTelemetry`, after each call): `engine.state` (0 off, 1 cranking, 2 running, 3 running down),
`.speed`, `.redline`, `.idle` (rad/s), `.throttle` (the plate), `.manifold` (Pa absolute), `.coolant` (K),
`.oilPressure` (Pa over the air), `.flags` (`FLAG_LIMITER` 1, `FLAG_FUEL_CUT` 2), `.cycle` (0 to 1), `.crank`
(degrees, 0 to 2160: modulo the cycle, 720 or a rotary's 1080, it is cylinder 1's angle, 0 at its firing top dead
centre, for every layout, custom ones too); `boost.pressure` (Pa over the air); `exhaust.portTemperature` (K); `ecu.knockRetard` (degrees);
`car.speed` (m/s), `.gear`, `.brake`, `.clutch`; `sound.peak` (the call's largest sample).

Card D's, appended to `tel.engine` (`Engine::mechanicsTelemetry`, every engine unless said): `torque` (the brake
torque: the gases' less friction and a supercharger's drag over some 0.1 s; on the dyno, the absorber's mean),
`gasTorque` (the gases', its 20 ms mean), `firing` (Hz: firings a second, every cylinder's), `power`, `imep` (gross), `pmep`, `fmep`, `bmep` (4 pi torque / displacement), `veManifold`,
`veAir` (the fresh charge at intake closing over the swept volume at those densities), `peakPressure`, `pistonSpeed`,
`ripple` (the crank's speed's spread over the last cycle), `dynoTorque`, `dynoSpeed`, `shake1`, `shake2` (N), `rock1`,
`rock2` (N m: a reference layout's cylinders taken upright); a custom engine's only: `inertiaTorque`; `cylinders`; `layoutRefused` (1: a custom geometry was refused); `oilTemperature` (K); and the arrays (readable by path, not
gauges) `firingAngle[c]` (degrees after cylinder 1's at which it fires), `cylPeak[c]`, `cylPeakAngle[c]`.

Scope (`Engine::scopeTap`, each sample while `ctl.scopeOn`): cylinder 1's `pressure` (Pa), `volume` (m^3),
`liftExhaust`, `liftIntake` (0 to 1) at each whole degree of its cycle, copied whole into the Scope as the cycle comes
round (`degrees`: 720, a rotary's 1080; `cycles` counts them), and `pipePressure[0..7]` (Pa absolute: the first header,
collector pipe, main pipe, chamber, tube, chamber, tail, and the plenum) at that moment. It only reads: the sound is the
same with it on (the null test's `views` runs with it on). Off, it costs one test a sample. The baked maps' fields
(`bakeCount`, `bakedNx[7]`, `bakedNy[7]`, `bakedX[7][16]`, `bakedY[7][16]`, `baked[7][256]`: map k's z[i][j] at
`baked[k][i * 16 + j]`, k in maps.fm's order spark, lambda, boost, camIn, camEx, pedal, lift) are filled by ecu.model
1 on each `ctl.bakeMaps` increase.

## The order of work

`Engine::process` → `readSpec`, `readControls`, `ignition`, then `render` (the resampler: `Listen::needs`, `renderAt`
of 16 engine samples, `Listen::pushPart`, `Listen::resampled`) → `renderAt` (per 16-sample part: the view's and the
engine's change at the fade's quiet, where `configure` runs; `Listen::block`; then each sample `control16` every 16th,
`sampleTick`, `Car::road`, the fade's gain, `Listen::mix`; then `Listen::finish`) → `writeTelemetry`.

**One sample** (`Engine::sampleTick`):

1. `exh.read()`, `intk.read(np)`: the waves arriving at every end.
2. `crank.advance(dt)`: the angle.
3. Each cylinder: its angle; `cyl[c].step(a, dDeg, dt, vt.areaEx(c, a), vt.areaIn(c, a), bE, zE, tH, bI, zI, tAir)`
   with its valves' areas and its header's and runner's arriving waves and impedances (a rotary's faces sharing a
   port solved together first: `exhaustFlowAt`, `intakeFlowAt`, with the areas too); its flows summed per port into `qe`, `qi`; its torque into the total; its gas into `exh.eSum`,
   `exh.mSum`; `work.sample(c, a, p, v)`; `comb.sample(cy, c, dt)` (card H's model 1: nothing at the defaults); at intake closing `work.trapped(...)` and, if `vt.active(c)`, `comb.charge(...)`; its events to `ex.*Click` (the
   valves' louder while they float: `vt.clatter`); then `scopeTap()` while scopeOn.
4. `exh.step(qe, np)`: the ports' waves in, the collectors, the merge, the converter, the muffler, the tails out
   (`exh.outL`, `exh.outR`), every pipe written.
5. The runners' near ends; `intk.plenum(np, rNear, fuel, plate, ecu.bypass, dt)`, the plate `ecu.throttle` (1 while
   `vt.plateOpen()`: the lift is the load control).
6. `mech.torques(...)`; the friction `tf` (`mech.tf`, or a custom engine's `fric.torque(...)` on the last cycle's peak
   pressure); `heat.fricJ`; `boost.drag(...)`; the dyno's `tqAbs = dyno.step(...)` while `dynoOn`; the crank: a custom
   engine's `mm.advance(...)` (the energy-exact step with the inertia torque) on the bench, `car.step(...)` with
   `mm.torque(...)` added when driven; else `crank.bench(...)` (the dyno's torque and inertia in place of the load
   dial's while `dynoOn`) or `car.step(...)`; `crank.ripple(torque)`; the brake torque's mean and the cycle's speed
   spread (reports only).
7. `ex.tick(qThr, drop, crankHz, dps)`, `boost.sound(crankHz, mdot, bov)`, `ex.tickStarter(crankHz, starterOn)`.

**A two-stroke** (`spec.engine.cycle` 1 on a custom engine: `cycleUse`, card T, docs/models/twostroke.md): after step 2,
`twoStrokeTick(dDeg)` in place of steps 3 to 7: `two.step(...)` (each cylinder with its crankcase and its own expansion
chamber: the gases' torque; the throttle is `ecu.throttle` plus 0.12 of `ecu.bypass`, the ECU's `fuelCut` the spark's);
`scopeTwo()` while scopeOn; `mech.torques(...)`; friction by part on `two.peak`; the dyno; the custom engine's crank
(`mm.advance` or `car.step`); `ex.tick`, `boost.sound` and `ex.tickStarter` on the two-stroke's flows. Its stingers'
`two.outL`, `two.outR` (times `TS_LEVEL`) replace `exh.outL`, `exh.outR` in `listen.mix`. Every 16 samples, before the
driver, `two.control16(...)` (the power valve, the pipes' gas, the flows' losses), and a moved `two.signature(...)` or
`cycle` asks for the fade. In `configure`, last: `two.configure(...)`, `lay.n`, `lay.cyc` = 360 and `lay.fire` from its
cylinders, then `mechanicsSetup()` again. The Scope's cycle is 360 degrees: `liftExhaust`, `liftIntake` carry the
exhaust and transfer ports' open shares, `portIntake` the crankcase's pressure; `pipePressure[0..5]` along the chamber,
`[6]` the crankcase. At `cycle` 0 it costs one integer test a sample.

**Every 16 samples** (`Engine::control16`): `crank.sense()`; `heat.control(...)` and the walls into the cylinders;
`ecu.sense(...)`; the size and the layout watched (the fade); the dials glided (`exh.sm*`, `boost.glide`, `vt.smCam` →
`valves()`, the compression → `cyl[c].setup`, the bore/stroke → `boreStroke`, `valves`); `ecu.driver(...)`; driven,
`car.drive16(...)`; `ecu.control(...)` (true: the engine has come to rest, `crank.omega` 0); `exh.flowRate()`;
`boost.control(...)`, `intk.pBox`, `intk.tAir`, `intk.control(...)`; `exh.gas(d16)`; the cylinders' wall heat transfer (`hA`);
`exh.heatUp(...)`; `pipes()` (`exh.pipes`, `intk.runners`); `exh.flow16()`, `intk.flow16(...)`; `exh.bands()`; with spec.engine.wallHeat 1, each cylinder's `hA` from `wall.coefficient(...)` (Woschni: core/wallheat.fm) in place of the reference's; `mechanics16()` (the cycle's speed spread; a custom engine's inertia with the
bench's load, its oil factor and its shaking at this speed; the dyno stopped when its mode ends); then, with `spec.voices.model` 1 or the voices gliding out, `ex.control16(...)` (card M's voices' inputs).

**`configure`** (the engine for the layout and the size: at init, at a start while stopped, under the fade): the
layout, the fuel and the induction taken; the heat stood (`heat.stood`); every size (each cylinder's displacement, the
pipes' bores, the bore and stroke, the throttle and plenum, the inertia `mech.jBase`, the gearing's torque); the
cylinders set up at their angles (each with its small bias, a draw of the noise); the ports' banks; `valves()`; a
running engine's cylinders given the gas where they stand; every pipe, the paths and the junctions at rest
(`exh.clear`, `intk.clear`, `listen.resetPaths`); the clatter for the fuel; the layout's loudness trims; stopped, the
exhaust's heat soaked to where a long idle leaves it; `pipes()`; `mechanicsSetup()` (the moving masses, friction,
shaking, work integral and dyno for the engine as configured).

**The physical cylinder**: friction by part (Chen and Flynn's, `core/friction.fm`, by mean piston speed) is used for a
custom engine and whenever `fuel.model` is 1; Woschni's wall heat (`core/wallheat.fm`) whenever `fuel.model` is 1 or
`engine.wallHeat` is 1. The new combustion implies the physical cylinder, its walls and its friction together; the
reference's one friction curve (fitted to its V8, too high for small bores: 4.5 bar at 24.8 m/s on a 250 cc cylinder)
and its one wall coefficient only under the reference's models. `engine.custom` means only that the geometry is the
build's own. The engine runs at 96 kHz; compiled with `TEE_ENGINE_RATE_48000` defined (card C's prediction service
only) at 48 kHz.

**A custom engine** (`spec.engine.custom`, card D): `configure` takes the layout dial's table first (its pipes, idle and
sizes) and then `customLayout()`: `geo.derive(&spec.engine)` and the Layout's `n`, `cyc`, `rotary`, `fire`, `bank` (the
mechanical bank's parity: the exhaust's two sides), `port`, `ports`, `topo` from it. Engine places cylinder c at
`crank.theta + fire[c]` in its cycle, so a larger `fire` fires sooner: a custom layout's `fire` is the cycle less its
derived angle, and it fires in the order its geometry gives (the reference's tables run their orders the other way
round: the same engines, mirrored; `tel.engine.firingAngle` reports each engine as it runs). `bore` and `stroke` (both
above 0) set the swept volume and the bore/stroke ratio the dial glides to (`bsOf`); `rodLength` the rod ratio every
`Cylinder::setup` gets (`rodRatio`, else 3.3); the redline is `redlineSpeed` or 25 m/s of piston speed (13.5 a diesel)
in `boreStroke`; a rotary's port fields override the cam dial's in `valves()` (`customPorts`). A change to any of
these fields is seen through `geometry()` (a weighted sum of them, 0 while custom is off) and made as a new layout is:
at a start while stopped, under the fade while running. A geometry whose cylinders fire together cannot run (the
Spec's own defaults, every pin 0, are one): it is refused, the layout dial's table runs with the custom sizes, masses
and friction (`geo.fromTable`), and `tel.engine.layoutRefused` says so. With custom off nothing of this is read:
`tests/sim/core/EngineTest.cpp` plays a build with every custom field filled in and custom off, to the bit the same.

## The modules, their functions and what they read

Every function below is called by `Engine` only, at the time given. Where a module takes another's state as an
argument the argument is named; where it reads a pointer set in `setup`, the pointer is named. Nothing writes another
module's fields except where said.

**Crank** (`core/crank.fm`): `omega` (rad/s), `theta` (0 to 2160 degrees), `rpm` (as of the last `sense`), the
torque's mean and ripple (`tqNow`). `float advance(float dt)` each sample first; `void bench(float torque, float tf,
float tsc, float tl, float ts, float j, float dt)` each sample on the bench; `void ripple(float torque)` each sample
after; `void sense()` every 16, first. Driven, `Engine` sets `omega` from `Car::step`, and to 0 when `Ecu::control`
says the engine came to rest.

**Mechanics** (`core/mechanics.fm`): `jBase` (set by configure), `idleTq` (set each part), this sample's `tf`, `tl`,
`ts`. `void torques(float rpm, float omega, float strokeK, float viscK, int fuelUse, float smCR, float vTotal, float
load, float redRpm, float tAmb, float starterOn, float vd)` each sample; `float inertia(float flywheel, float load)`.

**Heat** (`core/heat.fm`): `tAmb`, `tCool`, `tOil`, `tSurf`, `warm`, `warmS`, `viscK`, `pOil`; it gathers `heatJ` (the
burns', written by Combustion) and `fricJ` (the friction's, written by Engine). `void setup()` in init; `void stood(int
state, float ambF)` in configure; `void control(float d16, float ambF, float vTotal, int state, float rpm, float
omega)` every 16 and in configure (d16 0); `float coldIdle(int fuelUse)`.

**Cylinder** (`core/cylinder.fm`, the reference's; `MAX_CYL` of them): its gas and events. `setup(...)` in
configure (and as the compression glides), `reset(float a)` in configure, `void step(float a, float dDeg, float dt,
float areaE, float areaI, float bE, float zE, float tPipe, float bI, float zI, float tAir)` each sample, its valves'
open areas (m^2) the valvetrain's; `float exhaustFlowAt(float areaE, float bE, float zE, float tPipe)`, `float
intakeFlowAt(float areaI, float bI, float zI, float tAir)` for a rotary's shared ports; `lift`, `volumeAt` read by Combustion and the Scope. Valvetrain writes its
`evo`, `edur`, `ivo`, `idur`, `aE`, `aI`, `sparkAt`, `burnDur`; Combustion its charge (`qTot`, `sparkAt`, `burnDur`,
`knock`, `cBurn`, `beta`, `dur2`); Engine its `tw` (Heat's `tSurf`) and `hA`.

**Valvetrain** (`valvetrain/valvetrain.fm`, card E): `smCam`, `lastCam`, `model` (in use), `lab` (`ValveLab`),
`clatter`. `void apply(ValveSpec* s)` from `readSpec` when `spec.valve`'s rev moved; `void events(Cylinder* cyl, int n,
int rotary, float cyc, float bore, float vTotal)` from `Engine::valves()` (configure; when `smCam` or the bore moved):
the reference's events and areas, and there `spec.valve.model` is taken (model 1 builds the lab's tables then, and
writes each cylinder's `evo`, `edur`, `ivo`, `idur`, `aE`, `aI` from them every 16 samples, so the cylinders detect
their events by the lab's); with valve.model 1 the hooks below (`oil`, `control16` every 16 after the ECU; each
sample `float areaEx(Cylinder* cy, int c, float a)`, `float areaIn(Cylinder* cy, int c, float a)`: the reference's
areas until the lab is taken, then the lab's tables, the auxiliary event summed into the exhaust); always: `bool
active(int c)` (false: the cylinder rests, no charge), `int plateOpen()`, `clatter`, `float liftEx(int c, float a)`,
`float liftIn(int c, float a)` (the Scope's, 0 to 1 of the valve's largest), `void telemetry(ValveTel* t)` after each call.

**Intake** (`intake/intake.fm`, card E): `run[MAX_PORTS]`, `endRun[]`, `pPlenum`, `mPlenum`, `tAir`, `pBox`, `qThr`,
`aThrottle`, `aBypass`, `vPlenum` (the last three, `tAir` and `pBox` set by Engine), `model`, `itbs`, `box`, `itb[]`,
`len`, `sound` (model 1's snorkel or trumpets, not yet mixed: wave 2's Extras still rings its own box). `void
setup(float rate)`, `void release()`, `void clear()` (there `spec.intake.model` and the throttle's kind are taken);
`void apply(IntakeSpec* s)` from `readSpec`; `void runners(int np, float bike, float bore, float rate)` from
`pipes()`; `void control(float d16, float omega, float vTotal, float cycle)` every 16 after `intk.tAir` (model 1's
sizes, the runner's length, the box, the ripple); `void flow16(int np, float xi, float k16)` every 16; `void read(int
np)` each sample, first; `void plenum(int np, float* rNear, int fuelUse, float throttle, float bypass, float dt)` each
sample, after the exhaust; `void telemetry(IntakeTel* t)` after each call.

**Boost** (`boost/boost.fm`): `smTb`, `smTs`, `boost` (bar), the shaft, the valves, `turbo` (this sample's sound).
`void setup(float rate, Noise* noise)` (Extras' stream); `float tsOf(float tsize)`; `void glide(float level, float
tsize, float kDial)` every 16; `void control(int kind, float size, float exhaustPower, float throttle, float load,
float bovLevel, float rpmFrac, float crankHz, float tsz, float d16)` every 16; `void sound(float crankHz, float
massFlow, float bovLevel)` each sample between Extras' `tick` and `tickStarter` (one stream, in that order); `float
drag(int asp, float vTotal, float omega)` each sample. Its turbine's restriction on the exhaust is in
`Exhaust::pipes` (from `smTb`, `smTs`).

*Boost with `spec.boost.model` 1 (card F's machine, `boost/machine.fm`; docs/models/boost.md).* Model 0 runs the
above unchanged; model 1 makes `control` keep `boost` (bar) from the machine, `sound` the machine's sound, `drag` its
torque on the crank, and leaves the reference's turbine restriction out (`Exhaust::pipes` should get `aspUse` 0 then).
The machine's functions, all on `Boost`, all doing nothing in model 0:
- `void applyMachine(BoostSpec* s, float vd, float redRpm, float pAmb, float tAmb)`: when `spec.boost.rev` moves and in
  `configure` (vd the displacement m^3, the redline rpm, the air outside Pa and K). A change of the model, kind,
  layout, scrolls or supercharger rebuilds the machine at rest: make it under the fade.
- **The exhaust (card G's network)**, each sample after `prepare()` and before `step()`:
  `int housingOf(int bank, int group, int banks)` (the housing a collector ends in, -1 none: its pipe goes on as
  without a turbo); for each housing h fed, `float housing(int h, float drive, float admit, float tGas)` with drive =
  sum over its feeding ends of 2 a / z (with each end's junction-loss hold), admit = sum of 1 / z, tGas their gas
  (K): returns the housing's pressure over the outside, and each end writes b = p - a; for each downpipe side,
  `float downpipe(int side, float a, float z)` (z with the converter in series): the end's pressure over the outside
  (the pipe writes p - a; `mach.dpMass[side]` kg/s, `mach.dpTemp[side]` K went in); for each turbo t with an
  external gate, `float gatePipe(int t, float a, float z)` likewise (`mach.gateMass[t]`, `mach.gateTemp[t]`). A
  housing no pipe feeds is stepped by `tick`.
  Until card G's network calls these, `Engine` calls `void exhaustStandIn(float m, float mt, float z)` after
  `exh.step` (this sample's cylinder gas, kg/s, and its mass times temperature; the collector pipe's impedance): the gas
  shared among the fed housings as a flow, the downpipes open ends. It goes when the network's calls come in.
- **The intake (card E's)**: `float pressure()` and `float temperature()` before the throttle (Pa absolute, K), set
  into `intk.pBox` and `intk.tAir` every 16 samples after `control16`; each sample after the plenum `void draw(float
  m)` with the throttle's mass flow out of that volume (kg/s; negative pushed back), then `void tick(float omegaCrank,
  float pPlenum)` (rad/s; the plenum's Pa absolute): the charge path, the shafts, the sounds; then `float
  crankTorque` (N m taken from the crank, Mechanics's `tsc`).
- **Every 16 samples**: `void control16(float d16, float target, float antiLag, float nitrous)` with the ECU's
  `boostTarget()` (Pa over the air; 0 or less, H's -1 included: none, the spec's target or the spring), `antiLag()` (0 off, 1 on) and
  `nitrous()` (above a half, the window open). The gate's and vanes' loop toward the target is Boost's. `void
  airSpeed(float v)` the car's speed through the intercooler (m/s); `void vanesAndPurge(float vanes, int purges)` the
  ECU's vane position (below 0: Boost's loop) and purges counted, not called yet.
- **Combustion (H)**: `float oxygen()` (by mass, 0.232 air) or `float oxygenShare()` (over air's, 1 without nitrous).
- **After a call**: `void telemetry(BoostTel* t, float tCharge)` (tel.boost's card F fields; with no machine in the path the two charge temperatures read `tCharge`, Intake's `tAir`), `void scope(Scope* sc)` while
  `ctl.scopeOn` (the compressor map's lines and the trail).

**Exhaust** (`exhaust/exhaust.fm`): every pipe (`prim`, `sec`, `pipe`, `ch1`, `mid`, `ch2`, `tail`, `cross`),
`pop[2]`, its dials glided (`smMuf`, `smHdr`, `smPipe`, `smXp`), the junctions' losses and drives, the flow noise, the
gas's and the walls' temperatures (`tGas`, `tWall`, `tUse`, `tPort`), `mdot`, `outL`, `outR`, `shell[]`. Pointers:
`Noise*` (Engine's), `Layout*`, `Heat*`. `void setup(float rate, Noise* noise, Layout* lay, Heat* heat)`,
`release()`, `clear()`; `void soak(float tIn, float m, float dt, float bore, float vAir)`; `void pipes(float bore, int
aspUse, float smTb, float smTs, float rate)`; every 16: `flowRate()`, `gas(float d16)`, `heatUp(float d16, float bore,
float vAir)`, `flow16()`, `bands()`; each sample: `read()` first, `step(float* qe, int np)` after the cylinders.
Inside: `node`, `coneNoise`, `flowBand`, `jetBand`, `backFc`.

**Combustion** (`combustion/combustion.fm`; card H): `droop`, `dilution[MAX_CYL]`; fuel.model 1's `one` (Charge1),
`gas[MAX_CYL]` (EndGas), `diesel` (DieselFields). Pointers: `Noise*`, `Ecu*`, `Heat*`, `Layout*`, `Pop*`, `FuelSpec*`.
`void setup(float rate, Noise* noise, Ecu* ecu, Heat* heat, Layout* lay, Pop* pops, FuelSpec* fuel)` in init; `void
charge(Cylinder* cy, int ci, int fuelUse, float pPlenum, float rpm, float smCR, float crBs, float burnK, float pops,
float rough, float timing)` at a cylinder's intake closing (and, at a start without a starter, for the cylinders
between intake closing and spark); `void sample(Cylinder* cy, int c, float dt)` each sample after cylinder c's `step`
(returns at once under fuel.model 0 and ecu.model 0). Under both models 0 it is the reference's `charge` and
`chargeDiesel`, bit for bit. Under either model 1 a gasoline charge is `charge1`: the firing (`Ecu::one.fire`: burn,
fuel cut, spark cut, overrun), the spark and lambda from `Ecu1` (ecu.model 1) or the reference's; the heat from
`Charge1` and the fuel's row (fuel.model 1) or the reference's; knock from `EndGas` ticked every 4 samples in `sample`
(fuel.model 1, running only) or the reference's rule; the unburned fuel and air into `Ecu::one.account`, whose bangs
`sample` fires in the `Pop`s. A diesel under fuel.model 1 takes `spec.fuel`'s `soiAdd`, `pilot`, `rail`, `overFuel`.
Reads the cylinder's `p`, `t`, `m`, `mInert`, `ang`, `burnAt`, `xPrev` (the burned share), the Ecu's cuts, trim,
throttle, bypass, speeds and `one`, the Heat's temperatures; writes the cylinder's charge (`qTot`, `sparkAt`,
`burnDur`, `knock`, `cBurn`, `beta`, `dur2`; under fuel.model 1, at a knock or pre-ignition during the cycle, its
`qTot`, `knock`, and for pre-ignition `burnAt`, `xPrev`, `burnDur`, `sparkAt`), Heat's `heatJ`, the Ecu's `kRet`, its
knock control, its sensors (`sensor`, `meter`, `watch`), and fires `Pop`s.

**Ecu** (`ecu/ecu.fm`; card H): `state`, `idleRpm`, `redRpm`, `rpmAvg`, `rpmRate`, `throttle`, `pedal`, `bypass`,
`trim`, `fuelCut`, `limiting`, `dfco`, `starterOn`, `kRet`, the key list (`keyCount`, `keyNote`, `keyVel`: the
reference's key interface, which `Engine::ignition` and holdSpeed drive); ecu.model 1's `one` (Ecu1: `sim/ecu/ecu1.fm`
and the files beside it), `in1`, `sensor` (the lambda sensor), `meter` (the injectors), `watch` (cylinder 1's burn).
`void setup(float d16, EcuSpec* spec, FuelSpec* fuel, Controls* ctl, EcuTel* tel, Scope* scope, Heat* heat)` in init;
`float targetOf(...)`; every 16: `void sense(float d16, float rpm, float idleBase, float idleDial, float cam, float
coldIdle, float redBase, float redlineDial)` first, `float driver(float d16, int drive, float bend, float bendRange,
int keys, int rotary, int n)`, `bool control(float d16, float pedIn, int drive, float modwheel, float pressure, float
rpm, int fuelUse, float warm, bool shiftCut, int gear, float clutch, float omega, float manifold, float carSpeed,
float gasT)` (manifold: `intk.pPlenum`; carSpeed: the car's, m/s, 0 on the bench; gasT: `exh.tPort`, where the bangs
light). `control` runs the reference's (`control0`) and, under ecu.model 1, `control1` over it: Ecu1's plate, idle
valve, trim, idle speed, limiter, overrun and cut flags in the reference's fields, `kRet` the most of its per-cylinder
retards, `tel.ecu`'s fields, and a bake into the Scope on each `ctl.bakeMaps` increase. The state machine, the key and
the held-speed driver stay the reference's under both models. The hooks (card H fills them; each called only under its
caller's model 1): `float camTarget(int cam)` (0 intake advance, 1 exhaust retard, crank degrees; -1 the valvetrain's
own schedule: under ecu.model 0, or that cam's map on its formula), `float liftCommand()` (the lift map's, 1 on its
formula; 0 under ecu.model 0), `float boostTarget()` (Pa over the air; -1 the boost section's own level: under ecu.model
0 or the boost map on its formula), `float antiLag()`, `float nitrous()` (0 or 1), `void vehicle(float drivenSlip,
float speed, int gear, int upshifts, float cutRequest, float blipRequest)` (every 16 samples, driven, ecu.model 1:
stored for the next `control1`; the blip's crank speed made a plate as the reference's paddle blip does). Combustion
fires the reference's `Pop`s for the bangs until G's `popAt` is filled, and reads `Exhaust::gasTemperature(0)` (through
`control`'s gasT). Engine's `keyDown`,
`keyUp`, `ignition` and `configure` write its state too.

**Car** (`vehicle/car.fm`): the clutch, gearbox, driveline, tyres, the car's speed `v`, `gear`, the shifter and the
paddles (`lastButtons`, `shiftCut`, ...). `void setup(float rate, CarSpec* carSpec, EnvSpec* envSpec, Controls*
controls, CarTel* carTel)` (Engine passes `&spec.car`, `&spec.env`, `&ctl`, `&tel.car`; Car reads them only with
`spec.car.model` 1); `void followLever(int drive, int lever)` at a key
and in `drive16`; `float drive16(int drive, float d16, float jEngine, float peak, float gearTorque, float kg, float
redRpm, float bike, float clutch, float brake, int lever, int buttons, float gas, float rpm, float idleRpm, float
rpmAvg)` every 16 when driven; `float step(float w, float tq, float dt)` and `float road(float kSq)` each sample when
driven; `kmh()`, `moving()`. With `spec.car.model` 1 (card I) `drive16` hands everything to its `Vehicle m1`
(`vehicle/vehicle.fm`, docs/models/vehicle.md): it copies spec.car and spec.env into `m1.spec`, `m1.env` when their revs
move, reads ctl's counters, buttons, mode, strip and dyno itself, writes its fields of tel.car, and keeps `v`, `gear`,
`shiftCut` (a dog box's or a paddle's upshift cut) and `lastButtons` as Engine and the ECU read them; it returns the
pedal with a downshift's blip in it. `step`, `road` go to `m1`; `followLever` does nothing. Model 0, the default, is the
reference's code, untouched (the null test).

*The motorcycle (card B2).* With `spec.car.model` 1 and `spec.car.kind` Motorcycle (2), Car holds `bikeOn` 1 and its
`Bike b1` (`vehicle/bike.fm`, docs/models/bike.md) takes `m1`'s place: `drive16` calls `bike16` every 16 (spec.car's
fields the bike reads, card I's and card B2's block, copied into `b1.spec` and `b1.bs` with the env when their revs
move; `b1.control`; tel.car's card I and card B2 fields filled; `gear`, `v`, `shiftCut` (the quickshifter's cut) kept;
it returns the throttle the engine is given: the rider's, less the wheelie control's and a clutch upshift's roll-off,
with an auto-blip's); `step` and `road` go to `b1`. A change of kind or model puts it at rest in neutral. For the ECU,
`drivenSlip()`, `speedNow()`, `gearNow()`, `upshifts()`, `cutRequest()` (the quickshift cut's length) and
`blipRequest()` give the bike's. For the mechanical voices: `float lashStrike()` each sample (rad/s at the rear wheel
with which the driveline's play closed this sample, 0 otherwise; `tel.car.chainLashSide` says on or off the throttle)
and `float sprocketSpeed()` (the front sprocket, rad/s; its teeth are `spec.car.frontSprocket`, the rear's
`rearSprocket`; `spec.car.clutchDry` a dry clutch), `float bikeGearboxInput()` (the gearbox's input shaft, rad/s; in
neutral the clutch's basket) and `float bikeGearboxRatio()` (the gearbox's own ratio in the gear in, 0 in neutral or
between dogs), all 0 without the motorcycle.

**Extras** (`sound/extras.fm`): the engine bay's sources; its own `Noise noise` (Boost draws from it too). `setup(float
rate, float boxHz, VoicesSpec* voices)`; `setClatter(int diesel, float swept)` and `pumpOn` in configure; the clicks at
the cylinders' events (`valveClick(amp, intake)`: 1 at intake closing, 0 at exhaust closing); `void tick(float qThrottle,
float dropRatio, float crankHz, float dpdt, int intakeModel, float intakeSound, float lashStrike)` (lashStrike: card B2's `Car::lashStrike()`; with `spec.intake.model` 1, card E's
`intk.sound` is the intake's voice in place of the box filter) then `void tickStarter(float crankHz, float starterOn)` each sample;
`control16(...)` last in `Engine::control16`, only while `spec.voices.model` is 1 or the voices glide out (card M: the
car's gearbox, the torque, the coolant, the key, the boost and gate, `vt.clatter`); `busy()` (the pump's prime, a fan,
the starter running down); out: `intake`, `mech`, `block`, `comb`, `starter`, `mouth` (the intake's mouth alone, before the throttle's hiss), and with model 1 each voice's own:
`vo.gearbox`, `vo.axle`, `vo.front`, `vo.head`, `vo.tank`, `vo.gateOut`, `vo.starter`, `vo.chain`, `vo.clutch` (docs/models/voices.md).

**Listen** (`listen/listen.fm`, package L; docs/models/listen.md): `setup(float rate, float hostRate)`, `release()`,
`resetPaths()`; per part `block(int v, float intakeK, float mechK, float distance, float room, float levelDb, int asp,
float exTrim, float gainNorm, int nPipes, float bike, float tailD, SoundSpec* ss, SoundTel* st, float carSpeed, int
driven, float vTotal, int nCyl, float fx, float fy, float mx, float my, float* fire, float cyc, int rotary)` (Engine
passes `&spec.sound`, `&tel.sound`, `car.speedNow()`, `set.drive != 0`, `vTotal`, `lay.n`, a custom engine's `shake.fx`,
`fy`, `mx`, `my` (else 0), `lay.fire`, `lay.cyc`, `lay.rotary`); each sample `mix(int i, float xL, float xR, float in,
float tqNow, float mech, float block, float turbo, float starter, float comb, float sh0, float sh1, int driven, float
road, float cutG, float gearbox, float axle, float front, float head, float tank, float gate, float voices, float
mouth)` (package M's `ex.vo.gearbox`, `axle`, `front`, `head`, `tank`, `gateOut` and their sum `all` with
spec.voices.model 1, else 0; `ex.mouth`); per
part `finish(float* left, float* right, int at, int m)`; the resampler `needs()`, `pushPart()`, `resampled(float* left,
float* right, int i)`. With `spec.sound.model` 0 the new arguments are not read: the reference's path. With 1:
`Scene` (`listen/scene.fm`), `CabinBoom` (`listen/cabin.fm`), `Place` (`listen/places.fm`); it writes `tel.sound`'s
`camera`, `range`, `doppler`, `roadPosition`, `loudness`, `exhaustLevel`, `mixLevel`, `limiter`, `ceilingParts`.

**Noise** (`lib/noise.fm`): the reference's streams, as it has them: Engine's (`Engine::noise`: the cylinders' bias,
Combustion's scatter and pops, the exhaust's valve, flow and jet noise, in the order of the work above), Extras' (and
Boost's sounds), and Car's and each Pop's own. The order of the draws is part of the sound: a module that draws from
a shared stream draws where the reference did.

## Safety on the audio thread (card D)

Nothing in `process` loops a number of times set by a state's value: an angle is wrapped by `wrapCycle`
(`core/mathx.fm`: the same subtraction as before at most eight times, the remainder in closed form past that, a
non-finite angle 0), the valvetrain's included. A table read from a state clamps its index both ways. Once a call
(`Engine::guard`), before and after the render: a crank whose speed is not finite, negative or past five times the
redline (at least 3000 rad/s), or whose angle is not finite, is put at rest and the engine made afresh (`recover`:
configure, the car reset); and a call whose output has a sample that is not finite is silenced and recovered the same
way. Either sets `FLAG_FAULT` in `tel.engine.flags` for the host's guard. Never in normal running.

## The hooks (the integrator's, card D)

Each package's model is switched by its Spec section's `model` (0: the reference's, bit for bit; 1: the package's):
`valve.model` (E), `intake.model` (E), `boost.model` (F), `exhaust.model` (G), `fuel.model` (H: combustion),
`ecu.model` (H), `car.model` (I), and `engine.custom` (D). A switch changed while the engine runs is made under the fade, as a new layout is (`Engine::holdModel`: the Spec
keeps the model in use until the fade's quiet, where `configure` makes the change and moves the section's rev; stopped,
at once). A hook is called only when its model is 1, so at the
defaults nothing below runs. Its body is a stub, in the owner's module, that the owner fills; the call's place is the
integrator's.

| Owner fills | Function | Units, meaning | Called by Engine |
|---|---|---|---|
| E | `float Valvetrain::areaEx(Cylinder* cy, int c, float a)`, `areaIn(...)` | m^2, cylinder c's valves' effective areas at its angle a (stubs: the cylinder's own table, as the reference) | each sample, for `Cylinder::stepAreas(a, dDeg, dt, areaE, areaI, bE, zE, tPipe, bI, zI, tAir)` and a rotary's shared ports (`exhaustFlowArea`, `intakeFlowArea`); valve.model 1 |
| E | `void Valvetrain::oil(float pressure, float temperature)` | Pa over the air, K: from `Heat::oilPressure()`, `oilTemperature()` | every 16, valve.model 1 |
| E | `void Valvetrain::control16(float d16, float omega, float redline, float load, float theta, float* fire, float inTarget, float exTarget, float lift)` | rad/s, rad/s, 0 to 1 (`ecu.throttle`), crank degrees and the layout's firing offsets; the intake's and the exhaust's phaser targets in crank degrees from parked (-1: its own schedule) from `Ecu::camTarget(0)`, `camTarget(1)`; the lift command 0 to 1 from `liftCommand()` (0 or less: none, the stub's 0 included) | every 16, valve.model 1 |
| H | `float Ecu::camTarget(int cam)` (0 intake, 1 exhaust), `liftCommand()` | as above; **-1 from `camTarget` means "use your own schedule"** (whenever ecu.model is 0 or that map is on auto) | from the valvetrain's hook |
| H | `float Ecu::boostTarget()`, `antiLag()`, `nitrous()` | Pa over the air (**-1: use your own schedule**, also whenever ecu.model is 0 or the boost map is on auto); 0 or 1; 0 to 1 | into `Boost::control16`, boost.model 1 |
| H | `void Ecu::vehicle(float drivenSlip, float speed, int gear, int upshifts, float cutRequest, float blipRequest)` | the car's values below | every 16, driven, ecu.model 1 |
| I | `float Car::drivenSlip()`, `float speedNow()`, `int gearNow()`, `int upshifts()`, `float cutRequest()`, `float blipRequest()` | slip ratio (0 rolling); m/s; -1 R, 0 N, 1 up; a counter (one more at each upshift begun); s of cut asked for (0 none); rad/s of crank speed a downshift's blip asks for (0 none). **These names are final**: H's and I's own differed, and `gear()` and `slip()` are Car's fields already | into `Ecu::vehicle` |
| D (done) | `float Cylinder::burned()` | kg, the burned gas in it now; Combustion sets `qTot`, `burnDur`, `cBurn`, `knock` at intake closing, as now | by Combustion |
| F | `float Boost::pressure()`, `temperature()` | Pa absolute, K: the air at the throttle (stubs: the reference's boost, 300 K) | every 16, boost.model 1, into `intk.pBox`, `intk.tAir` |
| F | `void Boost::draw(float m)`, `void tick(float omegaCrank, float pPlenum)` | kg/s drawn through the throttle; rad/s, Pa | each sample, after the plenum, boost.model 1 |
| F | `void Boost::control16(float d16, float target, float antiLag, float nitrous)` | the ECU's (named control16: `Boost::control` is the reference's) | every 16, boost.model 1 |
| F | `float Boost::crankTorque` | N m the machinery takes from the crank | each sample in place of `drag`, boost.model 1 |
| F | `float Boost::oxygenShare()` | the charge's oxygen over air's (1 without nitrous, and with boost.model 0). **Final name** (F wrote `oxygen()`, H asked `oxygenShare()`) | every 16 into `comb.oxygen`, which Combustion's charge (ecu/fuel model 1) uses |
| F (done) | `void Boost::applyMachine(BoostSpec* s, float vd, float redRpm, float pAmb, float tAmb)`; `void airSpeed(float v)`; `void exhaustStandIn(float m, float mt, float z)`; `void telemetry(BoostTel*, float tCharge)`, `void scope(Scope*)` | the machine sized to the engine; the car's m/s through the intercooler; this sample's cylinder gas (kg/s, kg K/s) into the housings until G's network calls them; tel.boost's and the Scope's card F fields | in configure and when spec.boost's rev moves; every 16 before control16; each sample after `exh.step`; after each call (the Scope while scopeOn); all but the last two only with boost.model 1 |
| F, G | `int Boost::housingOf(int bank, int group, int banks)`, `float housing(int h, float drive, float admit, float tGas)`, `float downpipe(int side, float a, float z)`, `float gatePipe(int t, float a, float z)` | as docs/plan/integration-queue.md has them (stubs: -1, 0) | not yet: with exhaust.model or boost.model 1, the sequence `exh.read()`; cylinders; `exh.prepare()`; the housings, downpipes and gates; `exh.step(q)` goes in `sampleTick` step 4, in place of `exh.step(qe, np)`: G and F add it there together and tell the integrator |
| G | `void Exhaust::popAt(int bank, float amplitude)`, `float gasTemperature(int bank)` | Pa; K (stubs: nothing; `tPort`). Named popAt: `Exhaust::pop` is its Pops | by Combustion (card H) |
| M | `void Extras::control16(float d16, int ignition, int state, float crankHz, float coolant, int driven, int boxKind, int truck, int gear, float wIn, float ratioTotal, float fd, float wOut, float torque, float peak, float engaged, int asp, float boostShare, float gateOpen, int gateExternal, float flowShare, int cylinders, float cycleDeg, float clatter, float displacement, int diesel, int isBike, float wSprocket, int frontTeeth, int dryClutch, float primaryRatio, float lever)` | s; ctl.ignition; ecu.state; Hz; K; 0 bench; `spec.car.gearbox` (model 0: 0); car.truck; -1 R, 0 N; rad/s at the gearbox's input; crank turns to the wheels' (0 in N); the final drive; rad/s of the propshaft; N m (tqBrake) and the engine's most (peakTorque()); the clutch 0 out to 1 in; aspUse; boost over its most; the gate's opening 0 to 1 (boost.model 1: card F's most open gate, else the reference turbo's at its most boost); `spec.boost.gate` with boost.model 1; exhaust mass flow over its most; lay.n, lay.cyc; vt.clatter; m^3; fuelUse; card B2's motorcycle (car.model 1 and car.bikeOn: then the gearbox's arguments are the bike's own, `Car::bikeGearboxInput()`, `bikeGearboxRatio()`, `gearNow()`, kind sequential, final drive 1 and no propshaft), `Car::sprocketSpeed()` rad/s, `spec.car.frontSprocket`, `.clutchDry`, `.primaryRatio`, the lever 1 - set.clutch | every 16, last, with spec.voices.model 1 (and 16 samples after it returns to 0, to glide out) |
| T (done) | `TwoStroke` (`core/twostroke/twostroke.fm`): `setup(float rate)`, `release()`, `configure(EngineSpec*, float bore, float stroke, float rod, float compression, float theta, float pAir, float tAir)`, `control16(EngineSpec*, float omega, float d16, float tWall, float tAir)`, `float step(EngineSpec*, float theta, float dDeg, float dt, float omega, float throttle, int fire, float pAir, float tAir)`, `float signature(EngineSpec*)`, `openExhaust(c)`, `openTransfer(c)`; fields `outL`, `outR`, `mIn`, `mdot`, `peak`, `purity`, `delivery`, `trapping`, `valve`, `tPipe`, `vd`, `n`, `offset[]` | N m from step; m^3/s^2 radiated; kg/s; Pa | with spec.engine.cycle 1: setupAll/releaseAll; configure's end; every 16 before the driver; each sample after the crank (twoStrokeTick); tel.engine.ts* in mechanicsTelemetry |
| M | `bool Extras::busy()` | the pump's prime, a fan or the starter running down | `audible()`, voices.model 1 (card D, with rendering from the ignition's rise) |
| D2 | `void HeavyDiesel::control16(d16, fuelUse, state, pedal, omega, idleRpm, redRpm, sinceStart, clutch, mdot, tGas, boostBar, vTotal, n, bore, tAmb)`, `void feed(DieselFields* d)` | the governor's rack, the brakes' state; into Combustion's `diesel` (`lever` -1: the reference's) | every 16 after `ecu.control`; sets `hd.on` (1 only on a diesel with one of spec.ecu's D2 fields in use) |
| D2 | `float HeavyDiesel::brakeArea(int c, float a)`, `back`, `void sample(Cylinder* cy, int c, float a)` | m^2 the compression brake opens cylinder c's exhaust valve at angle a; Pa added to the header's arriving wave (half the exhaust brake's back pressure); cylinder 1's measures | each sample, cylinder by cylinder, only while `hd.on`: then the cylinder steps by `stepAreas` with `vt.areaEx(...) + brakeArea`, `bE + back` |
| D2 | `void HeavyDiesel::telemetry(EcuTel* t, DieselFields* d, int fuelUse, int fuelModel)` | `tel.ecu.smoke` (a diesel under fuel.model 1 or D2) and D2's tel.ecu fields | after each call |

The lead's fields, added by the integrator: `spec.env.pressure` (Pa), `.surface`, `.surfaceLeft` (-1 the same),
`.grade` (rise over run), not read by the reference's engine; `scope.portIntake`, `scope.portExhaust` (cylinder 1's
port pressures by degree, Pa absolute); `tel.ecu.advance` (cylinder 1's spark, degrees before top dead centre),
`tel.sound.rate` (the host's rate, Hz); `tel.engine.firing` and the dyno (above).

## Adding a subsystem

Its state goes into its own module in its directory. `Engine` holds one, sets it up in `setupAll`, frees it in
`releaseAll`, and calls it at its place in the order of work (a new hook in `control16` or `sampleTick` that does
nothing at its Spec section's defaults). Its Spec section gets the fields (appended), and `Engine::readSpec` passes
them on when the section's `rev` moves. Its own tests go in `tests/sim/<package>/`; the null test must stay green with
its fields at their defaults.

## The null test

`tests/sim/NullTest.cpp` plays each build of `content/builds` (the reference's Defaults, V8, V10 and Large Diesel,
converted) on each engine scenario of `tests/sim/scenarios` (`free-revs`: neutral, blips, the limiter, the overrun,
off, a start again while running down, off; `drive-car`; `truck`; `bench`; `views` (the five views, the distance, the
room and the level moved; the Scope on); `cold-start` (in the build's own air: -40 F for the presets);
`supercharged`; `quad-turbo` (its blow-off valve under 0.15: the surge)), every layout with a wild cam, and two other
rates and blocks (44.1 kHz in 333, 96 kHz in 64). `tee::playTwin` (host/include/tee/EnginePlayer.h) gives the
reference the same inputs: its dials from the Spec by the same float conversions, its Host's pedals, lever and paddle
bits from the Controls as `readControls` makes them, the start key at the ignition's start, and off (for which the
reference had no key) written into its Shared's state as `ignition` writes it. Result: every pair identical to the bit.

## Left as the reference has it

Nothing in the model was changed. Noted on the way, for the packages to decide:

- The crank's speed is an explicit `domega/dt` step with a constant inertia (it would lose energy once the inertia
  varied round the turn: a free single, 15 % in 20 s): a custom engine steps its kinetic energy exactly instead
  (`MovingMasses::advance`, 2e-5 in 20 s); the reference's step stays with custom off.
- `Set` survives as `Dials set` (`engine.fm`): the reference's dials as the engine has taken them, in its units. The
  modules are handed their values as arguments; reading the Spec directly needs one decision first: the reference's
  `init` configures the Init V8 and only the first start configures the build (a second `configure`, a second set of
  the cylinders' bias draws from the noise), which the null test depends on.
- The key list (`Ecu::keyCount`, `keyNote`, `targetOf`) is the reference's key interface, kept because the start, the
  key driver and holdSpeed run through it; HOLD's idle-out and the driven key-held switch-off are unreachable now.
- `Exhaust::step` declares a local `noise` (the jet's band); the shared stream is `noiseSrc` there.
- The tables of `core/layouts.fm` hold 12 cylinders a layout (the arrays hold `MAX_CYL`, 16).
- Several modules read others' state through pointers set in `setup` (Combustion, Exhaust) or take fields directly
  (Engine hands values on), not through functions; the integrator (card D) turns those packages need into functions.
- The pipes' sizes (`Exhaust::pipes`) are recomputed every 16 samples whether anything moved or not (cost).
- `Engine::configure` scales `lay.pipeD` and `lay.tailD` in place (correct only because it reloads the layout first).
