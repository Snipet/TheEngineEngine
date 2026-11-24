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

## FunkGui, after card A (13:19)

- ~~The owner pushes the tag `v0.28.0` on FunkGui's main (19d06ab)~~ done 13:28; card B names the tag beside its pinned commit.
- The lead looks at `fg.desktop.menu`'s 40 candidate golden rows and adopts them or says why not.
- Card A's requests outside its paths (`tools/check-headers.sh`, `tools/GalleryApp/CMakeLists.txt`, the POSIX-only
  tools and tests on Windows): `docs/handoffs/a-funkgui-desktop.md`. None blocks TheEngineEngine.
- A notched mouse wheel scrolls slowly on macOS (SDL does not tell it from a trackpad): ask the owner at the Mac test.

## From card C's handoff (13:24)

- **K3**: `host/include/tee/EnginePlayer.h` declares its own `struct tee::Build`; the contract's `tee::Build` is the
  document (`host/include/tee/Build.h`). K3 renames its own (asked 13:25).
- **D, as integrator**: a telemetry field `tel.engine.firing` (Hz), so tests need not work the firing rate out from
  the speed and a cylinder count; the dyno mode in `ctl.mode` and the brake torque in `tel.engine.torque`, which the
  dyno runner and the validation's torque and power targets wait for (they read "n/a" until then).
- **The keel**: `libclang-rt-18-dev` in `scripts/bootstrap-cloud.sh`, then `tee-host-tests` once under the `tsan`
  preset (never run: no runtime here). `tee-bench`'s breakdown by module.
- **The contract's wording**: §7 names `ControlsFrame`; the shell's `tee::ControlsFrame` (`Sources.h`) is the
  interface's and the host's is `tee::SimControls` (every field of §4); the shell's adapter converts. The lead
  corrects 02-contracts.md at the next edit.
- **For H (the ECU's idle) and the owner's ears**: validated against published ranges, the reference itself is off:
  the V8 idles warm at 1221 rpm (600 to 900 expected); the diesel idles at 1129 rpm and is governed at 2600 rpm
  (600 to 800; 1800 to 2400). The reference stays as it is behind its defaults; the new idle control and the
  presets should land inside the ranges, and `content/validation/` then passes.
- **A finding for everyone, macOS**: a static that owns a compiled FMinus program is destroyed after FMinus's and
  LLVM's own statics there, and its destructor then runs on a JIT that is gone (38 host tests crashed at exit; Linux's
  order hides it). Never keep a `tee::Program` or anything holding one in a static with a destructor: free it before
  `main` returns, or keep it in an object that is never destroyed (e69ea09).

## From the interface (13:35)

- **D**: the Scope arrays `portIntake`, `portExhaust` (cylinder 1's port pressures by degree, Pa absolute) and the
  telemetry `ecu.advance` (degrees before top dead centre), `sound.rate` (Hz): SCOPE (U5) draws them and shows a
  dash until they exist.
- **G**: pipe pressures per element of the system as built, so EXHAUST (U3) can draw every primary's pulse along its
  own pipe; wall temperatures per element.
- **U1**: `garage::Library* library` on `WorkspaceContext` (U5's `ui/views/garage/Library.h`); the shell implements it.

## From U3 and U4's handoffs (13:45 to 13:52)

- **G with D**: `XSpec`'s `crossover` and `muffler` collide with `spec.exhaust`'s dials of the same names (the
  reference's `xpipe` and `muffler`). One meaning a name: the builder's fields get their own names, or the dials
  become the model-0 fields under names that say so. U3's EXHAUST handles both readings until it is settled.
- **H and I**: the telemetry names the interface reads (sent to both, 13:46): `ecu.lambda`, `ecu.lambdaTarget`,
  `ecu.knock`, `ecu.knockRetard`, one of `ecu.spark` / `ecu.advance`, `ecu.flags`; `car.accel`, `car.flags`,
  `car.wheelLoad[i]`, `car.wheelSlip[i]`, `car.wheelTorque[i]`, `car.stripState`, the time slip's fields.
- **Card C**: on Windows `tee-host-tests.exe --list-tests` exits with heap corruption, which stops every test
  there (the tests' replacement of global `new` and `delete`); the same replacement must be compiled out under
  ThreadSanitizer.
- **An idea for the owner, not for today** (from U3): a slow-time control in the simulator, so the SLOW switches of
  ENGINE and EXHAUST show the waves truly slowed (the sound would fall in pitch with it), instead of replaying the
  Scope's last cycle.

## From Q's profile (14:04; `docs/measurements/q1-profile.md`)

On a quiet cloud core R is **1.79 us a sample** for the reference V8 (the 2.3 us of the keel's handoff was taken
beside builds); the split simulator is 1.00 to 1.03 R. Where it goes, V8 free-revs: `Valve::flow` 24 % (three
dependent passes of a square root and two divisions a solve: G's file, called by D's cylinder); `Duct::read` and
`write` 23 % (29 pipes at about 21 ns: G); `Cylinder::step` and `volumeSlope` 15 % (sin, cos, sqrt a cylinder a
sample, `expf` in the burn: D); `Exhaust::step` 12 % (junctions, noise filters: G); the pipes' coefficients every 16
samples 5 to 6 % (recomputed whether moved or not: G). Smaller: `Listen::resampled` 2.4 % (Q's own, next);
`Boost::sound` 7 % of the twin-turbo diesel (F); `clickAmp`'s `expf`/`logf` a sample in `Engine::sampleTick`, which
moves to `control16` to the bit (D). Q proposes patches for G's, D's and F's modules through the lead once each
package's integration has landed.

## From the vehicle's integration (14:06)

- **D**: `spec/env.fm`'s `pressure`, `surface`, `surfaceLeft`, `grade` (the vehicle runs on dry asphalt at
  1.2 kg/m^3 on level ground until then); `MODE_DYNO_CHASSIS` as a driven mode in `Engine::readControls`; the names
  the ECU reads from the car.
- **Card C**: array elements (`car.wheelSlip[2]`) as columns and assertions in a Scenario.
- **I, later**: an automatic whose tyres spin shifts up on the flaring output speed and can shift down again when
  they hook up.

## The exhaust gas is too cool (found by F, 14:33): D and H, then G, C and U3

At full throttle the reference's port gas (`exh.tPort`, from the cylinders' `eSum` / `mSum`) is 450 to 560 K; a
real petrol engine's is 900 to 1100 K, a diesel's 700 to 1000 K. The work is right (BMEP 10.2 to 10.8 bar), so the
heat goes somewhere else or the sum is not the gas's temperature. **D**: one cylinder's energy balance over a cycle on
the dyno. **H**: the charge's energy against the fuel's heating value, the wall's share. **The lead's decision in
principle**: with the new models on, the gas temperature is physical and validated; model 0 stays the reference.
Depends on it: F's turbine matching (its tests use a turbine at 0.55 of the matched size), G's wave speeds and wall
temperatures, H's overrun and anti-lag, C's validation (a target to add), U3's heat tints.

## From F's integration (14:33)

- **G**: the housing seam's per-sample calls in `sampleTick` step 4; remove `Boost::exhaustStandIn` when they go in.
- **Q**: the boost machine costs 0.24 R against 0.12 R (the pipe-end solve's three passes; double volumes; the
  sounds' noise at 96 kHz); `sim/boost/**` is Q's to change for cost now.
- **U3**: the Spec, telemetry and Scope names (sent 14:35); the compressor's map comes from the Scope's arrays.

## The lead's decision on non-exact optimisations (14:40, from Q's `docs/measurements/q2-levers.md`)

The engine is chaotic: any change that is not exact to the bit decorrelates the waveform at the first sound (even
the air nudged by 1e-4 K), so no sample-level tolerance can judge one. Rule: (1) a non-exact change is judged against
the engine's own sensitivity, an ensemble of at least 8 harmlessly perturbed runs a null-test pair, measure by
measure (band levels, speed trace, idle, steady dyno torque, clicks), and its long-term spectra and steady torque
must agree far more tightly than the fast measures' floor; (2) it lives beside the exact code behind one switch for
the whole program, off in the null test, on in the application only after it passed (1) and the owner has listened
to named before-and-after pairs (in X's listening pack); (3) exact changes come first and need neither.
FMinus's own levers give nothing: -O2 with denormals flushed stays; no `param` on the fuel and cycle switches.
Later, for the shell: compile at -O0 (0.13 s) for a first sound and swap to -O2 at a part boundary.
