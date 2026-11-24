# The integration queue

*Kept by the lead. What the packages asked of each other and of the lead in their standalone handoffs
(`docs/handoffs/`), in one place, for the integrator (card D) and for each package as it integrates after K3b.
A line is struck through when it has landed.*

## The signal

K3b has landed when `docs/handoffs/k3b.md` exists **and** `sim/core/monolith.fm` is gone. Nobody integrates before
that. Then, in this order, so that each step has a green null test to stand on: D becomes the integrator and adds
the hooks below as no-ops; E, H, F, G and I each append their Spec and telemetry fields and wire their model 1
behind its switch, model 0 staying the reference bit for bit; the lead settles any seam two packages disagree on.

## Asked of the lead

| From | What | State |
|---|---|---|
| H | `ctl.bakeMaps` (a press counter) in `spec/controls.fm`; in `spec/scope.fm`: `bakeCount`, `bakedNx[7]`, `bakedNy[7]`, `bakedX[7][16]`, `bakedY[7][16]`, `baked[7][256]` | passed to K3 (12:30); D adds them if K3b lands without |
| H | `Map16::at` and `Curve16::at` blend as `a*(1-f) + b*f`, so a breakpoint reads exactly | passed to K3 |
| I | `spec/env.fm`: air `pressure` (or density), `surface` (dry, damp, wet, snow, prepared strip), `surfaceLeft` (-1: the same), `grade` | D adds them at integration (the lead agrees) |
| J1 | the output's newest samples for a spectrum: `TelemetrySource::outputTail` | ~~done by B~~; J2 wires it into the reference's panel |
| D, E, H | one rule for packages' tests: `tests/sim/<package>/CMakeLists.txt`, added by a glob | ~~done by the keel~~ |

## Seams the packages want (the integrator adds them; names as the askers wrote them)

| Asker | From | Function | Meaning |
|---|---|---|---|
| E | Cylinder (D) | `step(a, dDeg, dt, areaE, areaI, bE, zE, tPipe, bI, zI, tAir)`, and `exhaustFlowAt`, `intakeFlowAt` taking areas | the cylinder takes its two areas (m^2) from the valvetrain instead of reading its own table |
| E | Heat (D) | oil pressure (Pa gauge), oil temperature (K) | the phasers' slew |
| E | Ecu (H) | `camTarget(int cam)` (crank degrees; -1: the lab's own schedule), `liftCommand()` | phaser and lift targets |
| E gives | all | `Valvetrain::areaEx(c, a)`, `areaIn(c, a)`, `evo(c)`, `evc(c)`, `ivo(c)`, `ivc(c)`, `active(c)`, `plateOpen()`, `clatter` | |
| H | Cylinder (D) | `burned()`; leave for `Combustion` to set `qTot`, `burnDur`, `cBurn`, `knock` at intake closing | |
| H | Boost (F) | `oxygenShare()` (1 without nitrous) | nitrous |
| H gives | F | `Ecu::boostTarget()` (Pa over ambient), `antiLag()`, `nitrous()` | the gate's or vanes' loop is F's, toward H's target |
| H | Exhaust (G) | `pop(int bank, float amplitude)` (Pa), `gasTemperature(int bank)` (K) | bangs by account; **G provides both** (`XBuild::pop`, `gasTemperature`) |
| H | Car (I) | `drivenSlip()`, `speed()` (m/s), `gear()`, `upshifts()`, `cutRequest()` (s), `blipRequest()` (plate 0 to 1) | I named them `slip()`, `speedNow()`, `gearNow()`, `cutWanted()`, `blipSpeed()` (crank rad/s): **the integrator picks one set** |
| F gives | Exhaust (G) | `int Boost::housingOf(int bank, int group, int banks)` (which housing a collector ends in; -1 none); `float Boost::housing(int h, float drive, float admit, float tGas)` (drive = sum of 2a/z, admit = sum of 1/z over the feeding ends; returns the housing's pressure over outside; G writes b = p - a); `float Boost::downpipe(int side, float a, float z)` (the end's pressure; what went in is `dpMass[side]`, `dpTemp[side]`); `float Boost::gatePipe(int t, float a, float z)` (an external gate's own pipe; `gateMass[t]`, `gateTemp[t]`) | F's proposal, **accepted by G** (docs/handoffs/g-exhaust.md) with one refinement: `drive` includes each pipe's junction-loss hold, and the downpipe's `z` includes the converter in series. Per sample: `exhaust.read()`; cylinders; `exhaust.prepare()`; `setHouse(h, Boost::housing(...))`; `setDown(s, Boost::downpipe(...))`; `setGate(t, Boost::gatePipe(...))`; `exhaust.step(q)`. The integrator fills the housings from `Boost::housingOf` and zeroes the reference's restriction |
| F gives | Intake (E) | `Boost::pressure()`, `temperature()` in place of `pBox` and `tAir`; `draw(m)`, then `tick(omegaCrank, pPlenum)` each sample; `control(d16)` every 16 | |
| F gives | Combustion (H), Mechanics (D) | `oxygen()` (H asked for `oxygenShare()`: one name, the integrator's choice); `crankTorque` | |
| F | Ecu (H) | `cmdTarget`, `cmdVanes`, `cmdAntiLag`, `cmdNitrous`, `cmdPurge`; the gate's PI loop is F's | H gives `boostTarget()`, `antiLag()`, `nitrous()`: the integrator maps them |
| D | Crank (K3's) | with `engine.custom` on, the crank is stepped by `MovingMasses::advance` (energy-exact with a varying inertia); off, the reference's step | |

## Tidying, at each package's integration

The glob is in (`tests/CMakeLists.txt`, 423f385; 212 of 212 tests pass in a clean tree at cce8618). Not yet to its
rule: boost, vehicle and exhaust name their test targets `tee-<package>-tests` through a `targets.cmake` (the rule:
`tee-tests-<package>`, made in the package's `CMakeLists.txt`); core prefixes its tests "core: " (the rule:
"core."); valvetrain and ecu still build through a `standalone.cmake` and need a `CMakeLists.txt`. Each package
fixes its own when it next touches its tests, and deletes its hook.

## Findings that bind everyone (also in 01-rules.md §4)

- An explicit step of a square-root orifice between two volumes chatters at rest: solve it in closed form.
- An explicit `dw/dt` step loses energy when the inertia varies round the turn.
- The reference's `car.fm` keeps the car's speed in a `float`: at 96 kHz small forces (rolling resistance in a
  coast-down) fall below its resolution. The new vehicle model uses `double` for speeds, distance and tyre
  deflection. The reference stays as it is behind `car.model = 0`.
- Every orifice between two volumes in `sim/boost/` is solved implicitly (`boostOrificeStep`) and volumes keep their
  mass in `double`; F suggests the intake's throttle be solved the same way when E integrates.
- From the exhaust builder: the reference clears its pipes before sizing them in `configure`, so a steepening pipe
  starts timed for its old delay and leaks the first pulse as a ramp; the builder settles after sizing, model 0 keeps
  the reference's order. A one-dimensional resonator must carry its end correction in its length. A bypass joined at
  a chamber is shorted by the chamber's volume.
- **Over budget**: a typical dual exhaust with an X, stubs and valves costs +0.31 R against the card's 0.25 R, nearly
  all of it the eleven extra pipes at the reference's cost a pipe (54 to 62 ns). For package Q with G after K3b:
  skip the filter loops and the tracker on bare pipes, or a tier without the bypass tubes.
- FMinus has no `atanf` and a program may declare no external function: a package that needs one writes it
  (`sim/vehicle/` has `vehAtan`, a polynomial good to 2e-6 rad). Candidates for a shared `sim/core/mathx.fm` at
  integration, so there is one copy.
