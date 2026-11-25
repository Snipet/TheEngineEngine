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

## ~~The exhaust gas is too cool~~ (claimed by F at 14:33; **not reproduced, withdrawn 14:45**)

**Withdrawn.** The integrator measured it (`tests/sim/core/EnergyExplore.cpp`): at full load on the dyno `exh.tPort`
is 1100 to 1330 K (default V8 at 4000 rpm: 1214 K after 12 s; F's own turbo four: 1326 K), reached from near ambient
with a 0.8 s time constant; one cylinder's balance: work 36 % of the fuel's energy, walls 11 % plus the reference's
20 % cut at the burn, exhaust 38 %. F most likely read `tPort` in the first half second, or the gas after the
pipes' cooling, or what its own stand-in hands the housings: F is checking and re-running the matched turbine. Kept
from it: the balance becomes a test with bounds (D), the port temperature a validation target (C), and with the new
models the 20 % cut moves into the wall heat (D with H, not urgent). What follows is the alarm as first written.


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

## Found 14:55: three of the four shipped builds stand in air at -40 degrees

`content/builds/v8.json`, `v10.json` and `large-diesel.json` (the reference's presets, converted) carry
`env.temperature` 233.15 K, the dial's lowest value; `default.json` carries 300 K. Air at 233 K is 28 % denser than
at 300 K, so every figure measured on those three is a cold-day figure: the V8's 696 to 700 N m and 346 to 347 kW
(card C's validation), the vehicle tests' drag (two had to state their air), the null test's pairs (which must keep
the presets as they are). For content (package P) and the owner: is -40 meant, or was the dial left at its stop?
Shipped builds should stand at 288 or 300 K; the reference's presets stay as they are under `content/ref/` and for
the null test. Until settled, quote dyno figures with the build's air.
- **E**: the contract test is red only on `spec.valve.valves`, `auxOn`, `stages`, `liftControl`, `liftThrottle`.
- **D, done 14:42**: the car reads `spec.env`; the six values for the ECU forward from the vehicle model.

## From H's integration (14:50)

- **H**: test 10 (traction) now that the car's values forward; **the idle** (lopes 900 to 1500 rpm on the V8, stalls
  at 800: the validation's loud miss); the kept share in `charge1` as the combustion efficiency alone, with D.
- **D**: pass `oxygenShare()` in; the cylinder's wall heat (half-burn at best torque is 0.3 degrees after top where
  engines show 6 to 12; the port gas runs 1155 to 1330 K where 900 to 1100 is usual); the README says -1 from
  `camTarget` and `boostTarget` means "use your own schedule".
- **E, F**: treat -1 from `camTarget` / `boostTarget` as "use your own schedule". **G**: `popAt` is still a stub.
- **C**: `PredictTest.cpp:94` assumes the first take is coarse: fails on a fast Windows runner.
- **U4, U5**: the ECU's final telemetry, flags, maps and baked arrays (sent 14:57).

## FunkGui's 40 candidate rows (`fg.desktop.menu`): looked at, not adopted here (16:07)

The lead looked at the menu overlay's pictures in `/home/user/FunkGui/build-desktop/sandbox/fg.desktop.menu/`
(both themes: the header, the tick, the bullet, the disabled rows, the open submenu with its hover) and they are
right. `tools/golden.py report build-desktop` says: 68 tests, 67 pass, 1 MISSING (these 40 rows), nothing drifted.
They are **not adopted from this machine**: FunkGui's goldens have a base written from the owner's arm64 Mac and
overlays by architecture; this container is x86_64 Linux, and adopting needs the bless variable that FunkGui's own
rules keep for its lead. For the owner, on the Mac, in FunkGui at v0.28.0 or later, after a `desktop` build and a
look at the two pictures:
`FUNKGUI_ALLOW_BLESS=1 python3 tools/golden.py adopt build-desktop --only fg.desktop.menu --reason "the menu overlay, new in 0.28.0"`.
Until then the test reports MISSING and fails nothing.

## For FMinus's owner: the launch wait, and three wishes (16:24, from Q's `docs/measurements/q7-compile.md`)

With every package in, the program is 466 kB of machine code and takes 3.4 to 4.2 s to compile on the cloud
machine (the reference: 158 kB, 1.4 s; the owner's Windows machine compiled the reference in 2.11 s, so expect
about 5 to 6 s there). The front end is 0.12 s of it; the rest is LLVM at -O2, in proportion to the code. Measured
and struck: -O0 compiles in 0.30 s and runs at 2.6 to 2.8 times the cost (a twin-turbo V8 at about 160 % of a slow
core): no "first sound unoptimised, then swap"; -O1 saves 8 %; there is no `param` in the program.

What we can do ourselves: define a switched-off model out of the program (`Program::Options::defines` per model):
measured on boost, the listener's model 1 and the voices together: 23 % less code, 0.72 s less, the running cost
unchanged; about 2.1 s for a build that uses none of the new models. The price is a recompile with a crossfade
when a switch flips, and a test matrix per set of defines. **Held** (the lead): not until the launch wait is
judged on the owner's machines.

What only FMinus can do (wishes, not bugs; the user writes the compiler, and nobody here edits it):
1. **An object cache keyed by the source and the options**: an unchanged program loads in milliseconds, which is
   every launch but the first after an update. The largest gain for a person using the application.
2. **Compiling in parallel** (modules split, ORC's concurrent compile): about 1 to 1.3 s on four cores, nothing
   for any package to change.
3. **Compiling a function on its first call**: a model that is switched off is never compiled; it needs care with
   `[[clang::nonblocking]]` (nothing may compile on the audio thread: the host would have to warm the paths a
   switch can reach before it is flipped).

Meanwhile the application opens its window first and says that it is compiling (card B).

**Added 16:57, from Q's `docs/measurements/q8-aot.md`: a fourth, smaller wish, and what it would buy.** The program
has grown to about 600 kB and 5 to 6 s here. Shipping the engine compiled ahead of time is feasible as FMinus stands
except for one thing: FMinus always targets the machine it runs on (`detectHost`; the driver has no `-mcpu` or
`--target`), so an object made on a CI runner carries that runner's processor (here AVX-512) and would die with an
illegal instruction on an older one.

4. **`-mcpu` / target features on `fminus -c`** (applied to the optimiser and the code generator: x86-64-v3 for
   Windows and Linux, apple-m1 for macOS), or a public `Program::emitObject` taking the same. With it: CI makes one
   engine library a platform for each set of defines we ship (about 6 s each); at launch only the front end runs
   (0.12 to 0.26 s: the Schema, the layout, the nonblocking proof) and the library is loaded; the JIT remains for a
   `sim/` that has been edited. Our side then: C (a front-end-only `tee::Program` binding entry points from a
   loaded library, keyed by FMinus's version, the defines and a hash of every file read), B and the steward (CI),
   and the proof that every null pair and X's combinations are identical to the bit between the JIT and the
   library. Not measured: the library's speed against the JIT's (a generic processor loses AVX-512).

The lead's reading, for the owner to overrule: the object cache (wish 1) is the simplest for everyone (nothing in
CI, nothing to prove equal, it follows each user's own processor, and it covers a `sim/` a person has edited); it
leaves only the first launch after an install or an update slow. Wish 4 makes even that one instant, at the price
of the machinery above. Either makes per-model defines unnecessary.

## From calibration (N1, N2; 16:10 to 16:26)

- **For C, later**: the host bakes an `auto` cam map (`ecu.maps.camIn`, `camEx`) by a sweep with the fast dyno
  (for each speed the advance that makes the most torque at full load, capped by the overlap the idle can
  stand), as makers calibrate, so a build a person makes gets what the archetypes get from N1's swept tables.
  Until then `Phaser::schedule` is a rule by target intake-valve-closing angle (N1).
- **For C, now**: the fast dyno reads 9 to 10 % over the stepped dyno on a boosted engine (slow states not
  settled); P's table reads boosted archetypes from the stepped dyno meanwhile.
- **For H's path (N2), after the gate**: over-boost protection in the ECU (a fuel cut above a threshold over the
  target; a Spec field; a telemetry flag). Nothing acts on the over-boost flag today.
- **A boosted engine has two states** until N2's gate lands: stepped up from low speed it runs away (the gate
  wide open, knock retard pinned); every figure taken that way is the runaway's.
- **For D**: one decision for "this engine's walls are Woschni's", read by both `engine.fm:821` and
  `combustion.fm:261` (today the wall loss is taken twice under `fuel.model` 1 with `wallHeat` 0).
- **Open question (N1)**: with the walls counted once the mild engines are 1.19 to 1.30 of their targets and the
  port gas is 1350 to 1520 K: is the burned gas's real heat capacity (gamma near 1.25 at 2500 K) in the cylinder's
  model, or was the reference's kept share standing in for it?


## Open, waiting for a source: valve float speed (N1, 19:01)

Measured on the big-block 7.4 (282 deg at the seat, 11.9 mm valve lift): with the archetype's spring (124 N/mm,
774 N on the seat) the valves float at 6159 rpm; with a real road spring (60 N/mm, 580 N) at 4673; with 75 N/mm
and 650 N at 5102. The original revs to 5500 to 6000 without float, so the model floats about 25 % low in speed
and a build that must rev like its original carries about 1.8 times a real spring's force. From the float
condition (`sim/valvetrain/float.fm`): effective mass 0.32 kg at the valve, the cam's nose deceleration 0.0169 m
per crank rad^2.

Two suspects, both **unsourced** (N1's memory), so nothing was changed (card N: a change needs a reason and a
source): the generator's nose (`cam.fm`, the reference's lift shape) about 2.4 times sharper than a road
flat-tappet cam's; a pushrod train's added mass 0.20 kg where typical parts sum to about 0.14. What would settle
them: a cam card that prints acceleration, or Norton's *Cam Design and Manufacturing Handbook*, for the first;
weighed parts (a maker's catalogue) for the second. A nose limit changes every lab cam's flow area, so it is a
change for whoever has the page, with the table run before and after. Until then the archetypes' spring rates are
the model's equivalents, not real ones (`docs/models/valvetrain.md` says so when N1 has written it).

## Open: "match the charge system" as an action (lead, 19:28, from X-021)

X-021 (a large diesel that cannot pull away with `boost.model` 1) is not a fault of the model: only the turbine and
the compressor are sized to the displacement, and the cooler's rated flow, the gate's spring and the charge volume
keep defaults that suit two to three litres (0.25 kg/s, 70 kPa, 5 L). A 15 L engine with them makes 23 kPa where
its matched values give 206. The fields mean what they say, so the defaults do not silently scale; instead:

- **N2, now**: telemetry that says so (the cooler's rated flow against the compressor's matched flow, the gate's
  spring against the target, the charge volume against the displacement: has and wants), and explicit fields for
  the combination build (X's file).
- **Later**: N2 writes the sizing rule with its sources in the boost model's doc; C's host applies it when asked
  (as it bakes a cam); U3's AIR workspace shows the warning and offers "match to this engine" beside it.

## Open, not reproduced: the V12's slow hot start (card I, 22:40)

Card I saw `v12-65` still cranking at 1.5 s in 318 K air inside its tyre test's start (driven), before d17d6808. D
could not reproduce it at the tip: running at 0.23 to 0.28 s of cranking in 288, 300 and 318 K air, cold and held
warm, on the bench and driven (hidden probe `[.v12start]`, tests/sim/core/EngineExplore.cpp). Parked; it comes back
with its command if it shows again.

## Open, for the owner's ears (lead, 23:17)

M's four (docs/models/voices.md, "Levels at idle"): the pump's prime at key-on and at idle; the fan forced on; the
DI tick against Port; the belt at +12 against 0. And: whether a shift on the reference's burn with ecu.model on
should cut half its firings instead of all (the steward's option 2); whether the key-on prime should be plainly
heard. Asked together once the fixes are in `main`.

## Open: the residual at idle, N3-16, as it stands (lead, 25 Nov 01:49)

Warm-idle burned shares 0.40 (V10), 0.49 (V12), 0.46 (hot hatch), 0.48 (7.0; 0.40 on the LS7's own card) against 0.17
on the economy four. What has been measured, each a document and no fit:

- **The fresh charge is right** (N1): the idles burn 0.33 to 0.45 L/h per litre; P's 496ee41a holds the published
  rates.
- **The drive is honest** (D, `docs/measurements/calibration/na/overlap-drivers.md`): the overlap flow is what the
  steady pressures give; the primary's pulse carries at most 4 % of it; nothing is counted twice; with no overlap flow
  every share would be 0.15 to 0.17.
- **The overlaps are ordinary** (N1): 24 to 39 degrees at the seat, -4 to +6 at 1 mm, every phaser parked at its
  least-overlap end. So all of the overlap flow passes at lifts under about 1 mm. The lead's "the cards have too much
  overlap" was wrong and is withdrawn.
- **Reverse flow alone cannot carry it** (N1, `reverse-flow-sizing.md`): the measured band of one small engine needs
  the reverse flow at 0.2 to 0.3 of forward.
- **The cam cards' flanks by class** (P's af2debe5, for table L): 0.02 to 0.04.
- **The passage below 1 mm** (N1, ruled to commit under `valve.model` 1): the frustum normal to the seat faces, on
  judged seat widths, with a judged coefficient of 0.8, in place of 0.73 of the head-diameter curtain (unsourced, and
  more than the passage holds): 0.04 to 0.06 of share, idle manifolds 5 to 9 kPa lower on the big cams, the
  economy four level, full load up to +8 % in the V12's mid-range.

**The yardstick changes**: the 0.20 to 0.27 of one small engine may not be these engines' band. Idle manifold
pressure is the total trapped mass and every service manual gives it; P is collecting the real engines' (the
model's: 40.3 V10, 54.7 V12, 53.3 hot hatch, 48.4 the 3.2, 38.9 the 7.0 on its card, 29.6 economy four, before the
passage change). They become idle targets with their basis; nothing is tuned to them.

## Open: the cam bake's part-load rows (U1's measurement, 25 Nov 01:43)

The intake cam's bake measures full load and parks the cam below 0.75 of ambient manifold. At 3000 rpm and light load
the auto rule asked 13.6 degrees where the baked table asks 0, so when a bake goes into use on a running engine the
note changes over 0.3 s (the phaser's own slew; no click; nothing audible at idle). Whether part load takes the auto
rule's value, or is measured (on what criterion: fuel, not torque), is not decided. C's and N1's, after table L.
