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
- **The passage below 1 mm: tried and refuted (02:10).** N1 built the frustum normal to a single 45-degree seat face
  with a coefficient of 0.8 (0.04 to 0.06 of share) and the lead ruled it in at 01:49 on geometry alone. Held before
  the push because a two-stage test's crossing moved 950 rpm; P's flow sheets (dbb23107, section 12) then showed real
  heads passing 0.70 to 0.85 of the head-diameter curtain at 0.100 to 0.200 in, a coefficient of one or more on that
  frustum, which no seat width saves. Dropped unpushed; the ruling withdrawn. The tip's 0.73 of the curtain sits on a
  production LS7 head's sheet within 0.03 from 0.100 to 0.300 in. Under 0.100 in, where the overlap flow passes, no
  sheet reaches: unmeasured, and the law there is the curtain's slope carried down, marked as that.
- **What agrees and what misses (02:10).** The one engine on its own cam card agrees with its own idle manifold: the
  7.0 on the LS7's card reads 38.9 kPa against an LS7's 38 (a forum figure). The misses are on cards of judgement: the
  hot hatch at 53 to 56 against the 4A-GE's 48 or lower, on about 256 degrees at the seat and 34 of overlap where
  that engine's timing as found is 240 and 18 (N1's probe: share 0.36 at 24 degrees of overlap, 0.27 at 14); the V12
  at 55 to 58 against a class of normal engines at 30 to 44, with no card or figure of its own. So the gap that is
  left is content: P gives the hot hatch its engine's timing if it is that engine; the V12 stays a recorded miss.

**The yardstick changes**: the 0.20 to 0.27 of one small engine may not be these engines' band. Idle manifold
pressure is the total trapped mass and every service manual gives it; P is collecting the real engines' (the
model's: 40.3 V10, 54.7 V12, 53.3 hot hatch, 48.4 the 3.2, 38.9 the 7.0 on its card, 29.6 economy four, before the
passage change, which was then dropped). P's rows (341d5bec, 148e623f): economy 1.5 at 20 to 34 (Honda, cited); the
4A-GE at 48, possibly a minimum vacuum; the LS7 at 38 (forum); the V10 and V12 on MOTOR's class of normal engines,
30 to 44; the 3.2's row dropped (a throttle a runner, no plenum). They are idle targets with their basis from table L
on; nothing is tuned to them, and the burned share is no longer a target.

## Open: the cam bake's part-load rows (U1's measurement, 25 Nov 01:43)

The intake cam's bake measures full load and parks the cam below 0.75 of ambient manifold. At 3000 rpm and light load
the auto rule asked 13.6 degrees where the baked table asks 0, so when a bake goes into use on a running engine the
note changes over 0.3 s (the phaser's own slew; no click; nothing audible at idle). Whether part load takes the auto
rule's value, or is measured (on what criterion: fuel, not torque), is not decided. C's and N1's, after table L.

## Open: the two-stage build's high stage never pays (N1, 25 Nov 02:04 to 02:07)

`two-stage-cam-20`, each stage held, 3000 to 7500 rpm: on the tip stage B (280 / 248 on 12.65 mm, the K20A's lobes
borrowed) never out-pulls stage A (236 / 204 on 8.5 mm) below 7500, so the change at 6000 rpm is a torque dip of 8 %
(17 N m) on an engine kind known for the opposite. Honda's own changeover for the F20C is 5850 rpm (cited three
times; the card takes it for table L). N1's ledger: stage A's volumetric efficiency still rises to 0.93 at 7500 while
paying 1.05 bar of pumping, with 405 K at closing; the port limit's tanh is flat past a quarter of the valve's
diameter, so B's 49 % more lift buys 5.6 % more area; nothing floats. To do, in order: the K20A2 flow sheet past
0.300 in against the model's bench on that valve (the limit's form); stage A at 7500, the cylinder's and the port's
pressures through the intake stroke and the same with the runner too short to ring (the runner's 0.30 m is
judgement); P looking for real dyno curves with each stage held. Not to be fixed by moving the switch or the lobes
toward the engine's ratings.

## Open, audible in main: the rotary's sequential join hunts (N2, 25 Nov 03:10)

`archetypes/rotary-twin-13`, full load, swept in 250 rpm steps: steady at 4000 (67 to 73 kPa) and at 4500 (42 to 46);
at 4250 the boost runs 26 to 110 kPa and the torque 170 to 263 N m, on a18f1bd4 as on N2's working tree. The join is
decided on a flow band (0.8 to 1.0 of the join flow) that the join itself moves the engine out of: it joins, boost
falls, it closes. F's case passed by when its snapshot looked. A real pair spins the second turbo up before it joins
and stays joined on speed with hysteresis; its fault is a dip. N2: a marked case at the join citing the rotary; the
cure proposed to the lead after the map's commit, before the cap.

## Open: the matching's design flow (N2, 25 Nov 03:08; the old "match the charge system", X-021)

`machine.fm`: `mMatch = vd nDesign/120 x 1.204 x PR x 0.9`, a charge at ambient temperature. Ruled 03:10: the charge's
temperature at the design point from the build's own compressor and `icEffect`, in N2's map commit, table first.
The truck (GateTest's build, no intercooler stated) is sized for 0.86 kg/s and passes 0.494. first.json's matched
inducer is 8 % over the IS20's area today (29 % with the sizing kept) and by the lead's arithmetic comes to about the
IS20's under the correction on today's formula. The 1.15 and the flat boost target at the design speed remain F's
judgement; first.json will state its own wheel (compInducer 0.0425, compExducer 0.054, turbWheel 0.050).

## Open, for N2 in this order (lead, 25 Nov 04:36)

1. **The wastegate loop's overshoot** (new with the map's commits; `main` waits on it): the test's four at 3500 makes
   129 kPa against 100 and settles after about 8 s; not windup at a stop. To try: an integral that acts only in a
   window about the target over a feed-forward duty, and a limit on its authority. Shown on first.json's tip-in at
   2500, 3500 and 4500 with and without the IS20's wheel, the take-ups unchanged.
2. **The sequential join** (the rotary hunts at 4250 in `main`): as proposed and ruled 04:08.
3. **The tractor's low-speed end**: the knife's edge at about 1060 rpm, the hole under it (11 to 16 kPa; 697 N m at
   1000) and the dip at 905; the take-up of 7.86 s against the makers' 1.5 to 4.4; with `ecu.afc` 0.77 the settled
   torque at 1100 is air-limited (1929 N m on 79 kPa). Whether a guard that opens the vanes on a flag fed by the
   flow the vanes control is the cap's self-reference again.
4. Recorded, not chased: the turbo four at 1500 and 1800 with the IS20's wheel (232 and 270 N m against the maker's
   350 from 1500): the turbine side, no housing figure found; the truck test build passing 66 % of its design flow
   (in surge cycles at its design speed); F's 0.9 against the diesels' 106 and 110 %.

## Open, for D2 (lead, 25 Nov 04:36)

`ecu.afc` 0.77 in (a patent's "typically about 1.3"), with what it moved recorded; then the full-load fuel curve as
designed (tables to the lead before its push; torque and power rows of a build whose curve comes from its published
torque marked "set by the fuel curve" and out of the counts); diesel-v8-truck's rated point air-limited under the
cap (its own open item).

## Open: the governed diesel's compressor sized at its torque-peak point (N2's (b), ruled 25 Nov 09:13)

Nothing in sim/ yet; measured with `compInducer` stated on the tip. **The rule:** the compressor's surge margin (10 %
right of the flag's line, BorgWarner's figure) held along the whole climb, PR 1.3 to target, at the build's stated
torque-peak speed (the maker's sheet; P's sources section 20); F's 1.15 at rated only where it gives the smaller
wheel; petrol builds unchanged. Derived default where a build states none: the lowest speed within 2 % of its fuel
curve's maximum, else 0.6 of rated (judgement). No 1.1 factor (N2's proposal, declined: a constant placed where the
model stops holding boost).

N2's table (one agent's, not re-measured): tractor 83.2 to 76.8 mm at 1000 (80.6 at 1100), truck test build 82.8 to
76.2; pickup and 2.0 common rail unchanged at 0.6 of rated, **to be run again at the makers' 1700 and 1750**.
Tractor held 30 s, N m and kPa: 1000: 651/13 steady today, **775/17 to 75 pulsing on 76.8**, 657/14 steady on 80.6;
1100: 1910/76 surging today, 2722/251 and 2720/237 steady; 1300 and 1900 within 2 %. At rated every wheel is in
over-speed (the relief opening the vanes), not choke.

**Order:** N2 measures the turbine nozzle's inlet factor as a sensitivity (1.247 today, 1.10, 1.00, 0.95) on both
wheels: torque and boost with the 1 s spread at 1000 / 1100 / 1300 / 1900, p3/p2, BSFC, take-up at 1100 by P's
command, and whether the build models EGR there. One patent's 0.90 to 0.93 (no EGR) is the only source; P found no
second. Then: if 1000 holds on 76.8, (b) and the nozzle go in at the maker's point; if not, the tractor and truck
state 1100, marked. Pushed only after the next `main`, on the lead's word.

## Ruled 25 Nov 09:40: the nozzle factor stays; the vane surge guard is the defect (N2's nozzle-sensitivity.md, 44878e30)

One agent's table, read by the lead on the branch, not re-measured. **The nozzle's inlet factor stays 1.247:** one
source (a patent's estimate), no cure for 1000 rpm, an effect only at rated (BSFC 190 to 183, take-up slower).
**The vane surge guard** (opens the vanes on the surge flag) pins on the 80.6 mm wheel (657 N m at 1000 against 873
with its gain at 0) and cycles at 1 Hz on the 76.8 (775 pulsing 17 to 75 kPa against 947 at 73 kPa steady): the
cap's self-reference again, and no cure in principle, because at one engine speed the swallowing line and the surge
line both run toward the origin. A real VGT has a vane map and a matched wheel, not a surge detector. So "state 1100
on the tractor and truck" is withdrawn; **(b) goes in at the makers' speeds together with the guard's removal** if
N2's next table holds (tractor, truck, 2.0 common rail at 36.9 mm with its rated cell, the pickup; the flags in every
cell; the default-tier cases that name the guard). A wheel too big then surges and says so (U3's oversize warning).
Asked from the same runs: the turbo's energy ledger at the tractor's 1000 cell (by the lead's unchecked arithmetic
73 kPa should need p3/p2 near 1.03 at eta_c 0.65, eta_t 0.60, T3 900 K; the model needs 1.40), and why the engine
sits at phi/knee 0.88 where the rule's flow law put it near 0.99. Before any push: a second measurement of the key
cells by another agent, and `main` moved.

## 25 Nov 09:58: the guard stays (the lead's 09:40 ruling withdrawn); the hole below 1100 may be the burn, not the turbo

N2's vane-guard-off.md (37da9aa3; one agent's, read by the lead on the branch). **The truck refutes "remove the
guard":** with its opening gain at 0 the vanes shut to their stop and the compressor surges about 70 % of the time
(1300: 408 N m at 31 kPa against 2199 at 169 with the guard; TurboTest:88 and :108 fail). The same guard makes the
holes elsewhere (tractor 900: 637 against 845; 1000: 775 pulsing against 947 steady; pickup 1300: 576 against 810;
the 2.0 at 1750 and 2000). Neither keeping nor removing it gives the right engine. It stays; its replacement (a vane
floor by speed, N2's suggestion, or a limit on the boost target by measured flow) is open, table first.

**The turbo's ledger at the tractor's 1000 cell (N2, guard off):** eta_t 0.66, eta_c 0.54 to 0.57, vanes 0.63,
0.208 kg/s, p3 244, p4 106, p2 175 kPa, **T3 673 K**. The short term is T3 (a factor 0.75 against 900 K), then the
compressor's efficiency.

**The lead's arithmetic from N2's cells (unconfirmed, with D2 to read the code):** at 1000 rpm and 14 kPa the tractor
makes 657 N m at BSFC 398 to 404 on fuel smoke-limited at lambda 1.30: about 0.89 g of fuel a cycle, 38 kJ, for 8.3
kJ of brake work: brake efficiency 22 % where about 40 is expected, with a cold exhaust where 900 K or more is. Low
work and a cold exhaust together mean the fuel's heat is not arriving (the burn at the smoke limit, or the walls).
If so the hole below 1100, the 7.9 s take-up, "the turbine cannot hold 1000" and the guard's pulse are one fault
upstream of the turbo. Asked of D2 (09:58): the cylinder's energy ledger at that cell with 1300 beside it, and what
in the code falls toward the smoke limit. **(b) holds where it is until that is in.** Asked of N2 meanwhile: the
rule's own evaluation against the model's phi, knee and tip Mach at that cell (the rule promised a margin near 0.99
of the knee; N2's outside estimate reads 0.88).

## Found 25 Nov 10:04 (D2; code read by the lead): the reference's idle spark trim scales a governed diesel's heat on the dyno

`sim/ecu/ecu.fm:209`: `trim = rpmAvg < idleRpm + 450 ? clamp(1 + 1.6 (idleRpm - rpmAvg)/1000, 0.55, 1.35) : 1`.
Driven (drive != 0) with the pedal open it is clamped to 1 or more; on the dyno and the bench (drive == 0) it is
not. `chargeDiesel` multiplies every diesel charge's heat by it (`sim/combustion/combustion.fm`, `q = ... *
ecu->trim`). So a governed diesel held on the dyno below its ECU idle + 450 rpm at full pedal burns about half the
fuel its rack lets through, and `tel.ecu.fuelFlow` over-reads by the same factor.

D2's ledger, tractor as shipped, per cylinder-cycle: 1000 rpm: reported injected 6365 J, charged and released 3330 J
(every burn lit, none unburned), brake 1336 J (638 N m): **40.1 % on what burned**, port 728 K. 1300 rpm: 13853
reported, 13206 released, brake 43.4 %, port 921 K. Nothing burns badly: the 22 % brake efficiency the lead computed
from N2's cells is 40 % on half the fuel. Second confirmation: C's holds had the tractor's "knife's edge" at about
1060 rpm, a step at idleRpm + 450 seen from outside.

**One fault upstream of the turbo:** the tractor's hole below about 1100 on the dyno, the take-up held at 1100,
"the turbine cannot hold 1000" (half the exhaust energy; T3 673 K) and probably the vane guard's pulse there.

**Ruled:** under the metered governor `chargeDiesel` does not multiply by `ecu->trim` (gated on `diesel.metered`;
the reference to the bit; a diesel on the reference's lever unchanged). D2 prepares it locally with its test (books
against telemetry within 5 %), the before-and-after table (tractor 800 to 1300, take-up, pickup, 2.0, truck), warm
idles, and the whole default tier; **pushed on the lead's word after `main` has moved** on P's bake. Then N2's
low-speed cells, P's take-up and table L's low diesel points are read again, and (b) and the guard are judged on
the engine as it then is. **Open, asked of D2 as a read:** whether the same trim cuts a petrol's full-pedal torque
on the dyno below idle + 450, and which predictor points or table L rows sit in that band.

## The owner's five todos (25 Nov, about 10:58 UTC; from running the Mac app the night before, "might not reflect the current project state")

Checked against the tree at d0a7330e by the lead. None was already whole.

1. **Fuel controls for octane.** Exists: `spec.fuel.grade` (Regular 87, Midgrade 89, Premium 91, Race, E85, Methanol,
   Diesel, Propane, Hydrogen) under `fuel.model` 1, with `knockDelay`; the knock-limit table's key hashes the fields,
   so a change of grade re-bakes. Missing: no view names `grade` (grep of ui/views, ui/app, app: nothing), so the
   owner cannot reach it but through the generic inspector; no 93; no octane as a number. To do: the grade as a
   first-class control in TUNE (and what it costs or gains shown on the DYNO ribbon and the knock cells), an octane
   number (AKI and RON shown), the bake following it. Owner to be named after the diesel `main` (U4's TUNE, with C
   for the bake and N1 for the knock model's octane term). Touches sim/ only if the octane becomes a number.
2. **A Claude chat in the app**, by an Anthropic API key or by Claude Desktop over MCP; the logic to come from the
   TestingPlayground repository (public; cloned read-only at /home/user/snipet/testingplayground, 47bac40). A local
   read-only agent is mapping its chat and MCP code (files, transport, tools, key storage, what needs JUCE). Then a
   plan (the app has no JUCE and no HTTP client yet: FMinus's web library, libcurl on Mbed TLS, is the candidate),
   reviewed, then one implementation session. Keys never in the repository, the logs or a build file.
3. **The DRIVE view: more gauges, analog and digital, in several styles.** U1's (ui/views/drive). Telemetry that
   exists only; a gauge that wants a channel the simulator does not publish is listed, not added.
4. **The exhaust pops sound artificial.** They are: `sim/exhaust/pop.fm` is the reference's carved Pop, "a sharp
   pressure pulse and a crackle after it" (random clicks on a decay), fired by H's fuel account
   (`spec.ecu.overrun.fuel`, `retard`) at a bank's collector (`slots.fm` `pop`, `popAt`). To do: the bang as late
   heat release in the pipe's gas (the unburned charge's own energy, lit by the gas's temperature, shaped by the
   pipe it burns in), no synthesized crackle; which shipped builds pop at all at their defaults; before-and-after
   clips for the owner's ear, since nobody in the fleet can hear. A sim/ change: batched with a re-key. Owner: G.
5. **Hover help on dials.** Exists: every field's `ui::help` text reaches the kit (`ui/kit/bind/host/BuildDoc.cpp`,
   `info.help`), and the kit's items know `hovered`. Missing: nothing draws it. U1's, first.

**Order, the fleet kept thin (the owner's word, 10:53):** U1 now on 5 then 3 (ui/ and app/ only, no re-key);
after the diesel `main`: G on 4 and the octane control (1), in one sim/ window with one re-key; 2 as a plan first.

## The pops: G's inventory and the design ruled (25 Nov 11:24; the owner's todo 4)

**Inventory (G's probe, one agent's; nothing changed):** every path ends in the carved `Pop` (a half-sine of 0.25
to 0.85 ms, then single-sample random clicks dying over 4 to 16 ms). Under `ecu.model` 1 the account's store lights
when a blowdown is over 900 K and its pulse is `30 kPa (1 - exp(-m / 7 mg))`: 16 mg and 70 mg both give about 30
kPa. At the defaults what feeds it is the **fuel-cut limiter** (fired cylinders at lambda 0.87, the cut cylinders'
air beside them): first.json held on its limiter fires 25 to 50 bangs a second at about 26 kPa; the rally five's
spark cut 212 a second at the ceiling; the overrun itself is silent after 0.3 s. `ecu.pops` does nothing under
model 1. So the owner most likely heard first.json on its limiter.

**Ruled, to be built by G (sim/exhaust, and with the lead's word the seams in `sim/ecu/overrun.fm` and
combustion.fm's account call; H has no live session):**
- A bang is heat released in the collector's gas, fed to the junction as a volume source
  `q = (gamma - 1) / (gamma p) Qdot`; its energy the account's own fuel mass times the LHV; no pulse shape, no
  crackle, no ceiling formula.
- The store split: **premixed** (a charge that never burned: spark cut, misfire, an overrun charge lit late)
  burns as a flame through its slug; **unmixed** (a rich charge's excess meeting a cut charge's air) burns only as
  the slugs mix, slowly: heat, not a report. A stock fuel-cut limiter then has no bangs to speak of.
- The flame speed from the slug's own state (a published laminar-speed correlation at its temperature and
  pressure, the pipe flow's turbulence, the burned gas's expansion, a slug burning both ways), not a flat 20 m/s:
  by the lead's unchecked arithmetic about 7 ms for a 0.9 m slug where the flat figure gives 45.
- `exhaust.bang` appended and **visible** (Carved / Burned in the pipe; the new one the default under
  `exhaust.model` 1), so the owner compares by ear in the app; `exhaust.model` 0 and the reference ECU's dice to
  the bit. Anti-lag's heat into the turbine waits.
- Clips for the owner (first.json: the limiter held 2 s, a lift from 6000, a full-throttle upshift; the rally five
  on its spark cut), both ways; scenario files in content/scenarios and rows in the listening pack, with the lead's
  permission. Pushed on the lead's word, in the re-key after D2's.

## Card A's first asks, ruled 11:21

`add_subdirectory(assistant)` after shell: yes. `Build::undoStep()`, `redoStep()`, `gestureOpen()` (ids from 1,
never reused; append-only): yes, pushed early and alone with the library's skeleton and the step-id tests so the
host/ change is on the branch before the next bake starts. Added: the assistant's `predict_curve` and `run_dyno`
join the predictor's and the bake's sharing (`yieldTo`), with a test. The frame's drain calls in app/Main.cpp and
app/Offline.cpp (B's, asleep) at A2, the exact lines to the lead first.

## D2's two cures are on the branch (25 Nov 11:25; diffs read by the lead)

- **2cf55103, the diesel:** `trimD = diesel.metered != 0 && ecu->pedal >= 0.02f ? 1.0f : ecu->trim` in
  `chargeDiesel`. Cranking (the reference ECU's pedal 0) keeps its 1.35; cold starts identical to the reading.
  Tractor at full pedal on the dyno, 800 / 900 / 1000 rpm: 792 / 603 / 638 to 1176 / 1309 / 1552 N m (D2's table).
  The listen case passes unsplit (the tractor's largest 0.35; its old onset falls at 965 to 995 rpm, not at the
  trim's step, so that comment's cause was not the trim).
- **c16af6a2, the petrols (`ecu.model` 1, `sim/ecu/idle.fm`):** on the bench with the pedal open the trim is
  `clamp(tr, 1.0, 1.35)` below target + 450, as the driven branch already was. Full-load dyno inside the band, N m:
  small-block 5.7 at 800 / 950 / 1100: 325 / 253 / 151 to 380 / 395 / 409; economy four at 850 / 1000 / 1150: 85 /
  69 / 43 to 93 / 101 / 107; V12 at 1000 / 1150 / 1300: 489 / 368 / 218 to 566 / 566 / 591; unchanged above the
  band. Bench free revs come close to the in-car ones (first.json to 97 % of the redline 1.48 to 1.31 s). Idles and
  cold starts identical to the reading.
- Whole default tier on the merged tip 28e41156: 540 of 540 (D2).
- **Found, not caused, not chased:** the nightly "diesel cold, full" case is red on `main` as it stands (governors 1
  and 2 on models 0 take 17.5 s at 233 K against 8 s). D2's, after this window.
- Known remainder: `tel.ecu.fuelFlow` under-reads by the trim while cranking and with the pedal shut.

## Two content findings from N1 for a content-only push after P's bake (25 Nov 11:39; one agent's, P's rows second)

- **`rotary-twin-13`: the ports from the 13B-REW's cited timing**, not the cam dial at 0.55 (judgement, some 47 deg
  of overlap). Eccentric degrees: exhaust 75 BBDC to 48 ATDC; intakes 45 (primary) and 32 (secondary) ATDC to 50
  ABDC (the shop manual as quoted on the LinkECU forum, MotoIQ, AusRotary; agrees with the lead's recollection).
  `rotaryPorts = (130.0, 392.0, 381.3, 573.3)`, 11 deg of overlap, the two intake ports as one (judgement, stated).
  N1's ledger at 5000 rpm: 244.5 to 320.0 N m at the same boost (residual 0.149 to 0.076, VE on the manifold 0.72
  to 0.97), against 294 firm to 10 %; the curve 290 to 309 from 2500 to 6200. **To check:** 309 N m at 2500 on one
  turbo, above the card's peak. N1 predicted +6 to 10 % and got +31: under boost the overlap's backflow is the cost.
- **`two-stage-cam-20`: the high intake lobe on the F20C's own card.** Today the stages cross at 7188 rpm (makers'
  5500 to 5900): B fills better to bottom centre and then pushes 13 % of its charge back through a closing its
  runner's wave no longer covers; B's air is 4.7 % short at 6000 and nothing else is. Three sourced levers (the
  F20C's 36 / 31 valves, the K20A2 sheet's port qualities, the RBC plenum) did not move the crossing toward the
  range. The sensitivity: duration 43 rpm a degree, the runner not monotone. **With B at 247 deg at 1 mm on a
  centreline of 98** (the AP1 F20C's intake VTEC lobe from an S2KI thread, unverified by its poster; the card's 110
  was copied from A) **the crossing is 5802 rpm** and B is 4 % over A at 6000. Basis "forum, unverified" unless P
  finds the service manual's table. The owner reported this build's cam change as inaudible and invisible.
- Next for N1: the idle residual (the V10's 0.46 and the V12's 0.61 bar at idle against 0.30 to 0.44), by a ledger.

## Held for the next window (after P's bake at dbd64a5c has moved `main`); the lead calls the order

Everything here is built or being built locally and is pushed only on the lead's word, each after the whole default
tier on its merged tip. One bake follows the lot.

| What | Whose | State at 11:48 | Under |
|---|---|---|---|
| `Build` step ids (`undoStep`, `redoStep`, `gestureOpen`), then the assistant's A1 | card A | e260d2b and d97da7b local, tier green on an earlier tree | host/, assistant/ |
| The knock bake's light-load pedals (0.15 and 0.08 where 0.25 still knocks); `kKnockBakeVersion` 5 | C | built, bake tests green; V12's 1000 rpm row 0 up to 0.65 atm where it read 23.2 deg flat | host/ |
| (b): the governed diesel's compressor at its torque-peak speed, the VE's back-pressure term from Heywood's relation | N2 | being built; tractor 1000 rpm 2580 N m at 225 kPa on the 76.8 mm wheel (N2); the lead re-measures before the push | sim/boost, content (P states the speeds) |
| The surge setpoint limit in place of the vane guard | N2 | table first; not ruled in | sim/boost |
| The pops, the exhaust's half (`exhaust.bang`, `PipeBurn`, the junction's source) | G | built on `g/bang-wip` 8c58a37e; harmless alone | sim/exhaust |
| The pops, the account's seam (`FuelAccount` split, the charge's call) | N1 (H's model-on paths), after G's half is up | plan asked for | sim/ecu/overrun.fm, sim/combustion |

**Content only, for P after `main` moves** (no re-bake but the builds touched): the rotary's cited ports; the
two-stage build's high lobe on the F20C's centre; and, **only if N1's two checks pass**, the V10's exhaust at 210 deg
(Audi SSP 994376AG; the lead recalls that programme as the S6 / S8's 7000 rpm engine, not the card's 8700 rpm one).

**Recorded, nobody's yet:** the V12 at 1000 rpm and 0.93 to 0.96 atm still knocks with control's retard pinned at
14.9 deg, its cell 46.6 deg and flagged (C); is 15 deg the control's designed ceiling (N2's to say). N3-16 stays on the
V12's card: no source found for the L539's cams (N1). The nightly "diesel cold, full" red (D2, in hand).

## N3-16, the idle residual: N1's ledger (25 Nov 11:42; one agent's)

Per litre a cycle the V10, the V12 and the 7.0 do the same work against nearly the same friction: friction is not the
term. (1) The knock table's lowest-speed row was copied flat to idle's load (the V12 idling at 10.8 deg after top
centre; table off, 22.8 before), but the manifold barely moves with it (67.1 to 64.0 kPa): at about 0.5 burned share
the burn is dilution-limited. C's cure above. (2) **The term is the residual through the overlap**: the burned share
at intake closing is 0.43 on the V10 and 0.42 to 0.51 on the V12; the intake's parked centre 10 deg later takes 4 and
8 kPa off. The V10's card is the Audi / Lamborghini 5.2 FSI (not the S85); Audi's SSPs give a 42 deg range, the
adjusters locked at standstill with the intake advanced and the exhaust retarded, and an exhaust opening period of
210 deg; no idle positions, no valve events. With the exhaust at 210 deg at 1 mm (the card: 232) the V10 idles at
40.0 kPa, inside its class's 30 to 44.

## Added to the held list (25 Nov 12:02)

| What | Whose | State | Under |
|---|---|---|---|
| The start's fuel apart from the full-load curve (the curve a running engine's limit only); the tractor split out of the listen quick case as `[!mayfail]` naming the start-history finding | D2 | 9ef985c2 local; bisected to D2's own 75109f7f; cold cases 17.5 to 2.9 s at 233 K; 539 of 540 before the split | sim/, tests/sim/listen |
| A 93 grade, appended | N1 | asked | sim/combustion/fuels.fm, sim/spec/fuel.fm |
| The assistant's A2 (the MCP server `tee-mcp`, the link with its peer check, the cards, the `.mcpb`) and its wiring in app/ and shell/settings (`allowClaudeDesktop`, default false) | card A | 6d1b977 local; assistant.sim 14 cases | assistant/, app/, shell/ |
| The frame's "Allow Claude Desktop" switch row and status line (then the cards strip and the yes/no dialog) | U1, after card A's push | not started | ui/ |

**The lead's own second measurements** (its own build of the tip, `tee-dyno`, 20 s settle and 10 s measure, full
pedal): the tractor as shipped 800 / 900 / 1000 / 1100 / 1300 rpm: 1218 / 1357 / 1599 / 2090 / 2757 N m (D2: 1176 /
1309 / 1552 at the first three; before the cure 792 / 603 / 638); on the 76.8 mm wheel: 1260 / 1448 / **2580** /
2718 / 2811 (N2: 2580 at 1000, 2719 at 1100). Both confirmed.

**Closed:** the V10's exhaust at 210 deg does not go on its card (N1's two checks: Audi's SSP 994376AG is the S6 / S8's
engine by its own table; the top end loses 5 %). N3-16 stays recorded on the V10's and the V12's cards.
**Open, the owner asked:** octane as a free number (U4's list, item 2); not started.
**Nightly, D2 bisecting:** TurboTest.cpp:118, the big single's boost after 2 s at 1300 rpm 28.2 against 30 kPa.

## 25 Nov 12:16: (b) built and ruled in; the cap refuted; the bake held at one red listen assertion

- **P's re-bake (31e23ae1, local) is whole but for one assertion**: tests/sim/listen/EngineTest.cpp, "the auto level:
  cameras near and far brought together", `fabs(rev[1][1] - rev[1][0]) < 2.0` reads 2.64 on the re-baked first.json
  (1.5 on the last content): true levels Rear / Driver -19.2 / -21.8 dB, with the auto level -10.3 / -7.7. The lead
  ruled `CHECK_NOFAIL` on that one line with the finding named; **P's session refused it as weakening a test without
  the owner's leave, and the lead did not route around it.** The owner is asked (mark it and move `main`, or hold
  `main` for L's cure in the next window). L is on the finding. Until the owner answers, `main` stays at d0a7330e and
  31e23ae1 stays local.
- **N2's (b), built on a local branch (fe4582fa; b-in-the-matching.md, fde5dcd4):** `spec.boost.peakRpm` ("Torque peak
  from"; 0: the lowest fuel-curve speed within 2 % of the curve's most, else 0.6 of rated); the wheel the smaller of
  F's 1.15 at rated and 10 % right of the flag's line from ratio 1.3 to the design ratio at the torque peak; VE
  `0.9 (1.4 rc - pe/pi) / (1.4 rc - 1)` (Heywood's ideal-cycle factor as recalled; **page not checked**, a search by
  the lead did not return the equation; 0.884 against 0.879 measured). Wheels: tractor 83.9 to 76.8 mm, the truck's
  sets 81.8 to 74.7, the 2.0 39.4 to 36.9; the pickup and the combo large diesel unchanged; the other shipped diesels
  run the reference turbo and are untouched. Tractor 1000 rpm 1509 to 2579 N m (the lead's own build: 2580); the 2.0
  at 2000 rpm 176 to 329; **TurboTest.cpp:118 green by (b) alone** (the single 28.3 to 62.8 kPa at 1300).
- **Ruled in for the window, on conditions:** 540 of 540, which needs D2's cure of BrakeTest.cpp:229 first (the VGT
  brake on High at 2000 rpm spins the 74.7 mm wheel's shaft to 11480 rad/s against omegaMax 11124; D2 asked for what
  a maker's brake does about turbo speed); the integrator's two lines in `sim/engine.fm`; P states `peakRpm` 1750 on
  the 2.0 and 1700 on the pickup.
- **Withdrawn: the surge setpoint limit (the cap).** N2 measured that it latches on the climb at the torque peak
  (tractor 1000: 2145 N m at 95 kPa against 2579 at 225; the truck 1 to 41 kPa): the flow it needs is the flow it
  withholds. The lead's proposal, refuted.
- **Closed: the turbine's match point does not move** (matched at the peak, 1300 rpm is worse: 49.8 kPa, surging).
- **The guard, table first after (b):** (i) the surge flag on the cycle's mean flow, not the instantaneous (where it
  pins with the vanes open and the flag at 0.5, the 2.0 at 1750 and the pickup at 1300, nothing is surging; the lead's
  suggestion), against (ii) N2's test of whether opening helps, against the guard as it is.
- Recorded for D2: the truck's sets lose VE 0.609 to 0.543 for p3/p2 1.22 to 1.32 at 1900 rpm (1997 to 1705 N m).

## 25 Nov 12:42: what changed in the held list since 12:16, and the next window's order

- **P's bake is on the branch at ce169231** (31e23ae1 merged with L's restated listen cases). Nothing was marked: L
  restated the camera case and the defaults case on what the auto level holds (an exposure on peaks: each camera's
  loudest moment at its target; the gap in typical levels tied to the crests' difference with the remainder of the
  largest peaks' gap within 3.5 dB; the tractor's check on the idle's floor, not its start-history-sensitive
  largest). The lead had prepared a `CHECK_NOFAIL` on the owner's leave and dropped it unpushed. The lead's own
  build: listen 21 of 21 on the baked tip ("typical +2.6 dB = crests' difference +5.6 dB -2.9"). P's rows, table N:
  114 of 847; the NA curves below idle + 450 whole (small block at 1032 rpm 199.7 to 402.5 N m). **The tractor held
  at 1100 did not move: not the trim, the compressor** (P; on the 76.8 mm wheel 1100 holds 2718).
- **(b) leaves the next window: it goes in as a pair with D2's cure of the brake.** On the 74.7 mm wheel no vane
  position holds the shaft on the rise of a High brake at 2000 rpm (fully open from engagement: 1.012); earlier
  blowdown (the lead's ruled cure) lowers the settled shaft 0.5 % per 10 deg and makes the rise worse: withdrawn.
  D2's v8 holds the limit on both wheels at 1500, 2000 and 2400 (the vanes fed forward by speed, an integral on the
  shaft toward 0.93, two cylinders held off at engagement above 1800 rpm, a cylinder cut past 0.985) and is the
  basis; to settle before it is final: the hunting at 2000 on the smaller wheel (313 to 353 kW) and whether
  something measurable at engagement tells the wheels apart so the matched wheel keeps its 372 kW. BrakeTest's 330
  kW is read once the brake has settled (a published steady figure); the shaft's limit over the whole engagement.
- **The guard: both candidates out** (N2, guard-candidates.md, local). (i) the flag on the cycle's mean (the
  lead's): the mean flow is left of the line at the pinned cells too. (ii) the opening test: frees the pinned cells
  (pickup 1300 576 to 809 N m; the 2.0 at 1750 158 to 252) and hunts on the truck. **The lead's reading, being
  tested by N2:** the truck's low state is its own backflow (VE 0.61, minus 11 % for p3/p2 1.22 to 1.32; its sets
  have no measured valve timing), which the guard rescues by lowering p3; a copy of the truck's sets with the
  tractor's valve events, guard off, decides it.
- **The auto level stays as it is** (L): the percentile of 0.1 s windows keeps the cameras' order and the body's
  level but lets a rev come in 3.8 dB loud before it settles. Next to measure: today's instant rise, an isolated
  crack passed to the limiter. The pops on the limiter do not pull the body down (0.7 dB at most): the lead's
  suspicion, refuted.
- **Dropped:** D2's `[!mayfail]` split of the tractor's quick case (L's restatement makes it unnecessary; D2's
  start-fuel commit merged with the tip passes the listen suite unmarked: 21 of 21).
- **Started 12:33:** card A on A3 (the key chat; its transport built and tested on a loopback server, 532e9f9
  local; needs `libcurl4-openssl-dev libsecret-1-dev` in scripts/bootstrap-cloud.sh and the CI image: the lead's
  to add); U5 on a help line for every unbound control of every stage, with a test; M on the idle's spectrum at the
  opening camera (the owner's hum report, never confirmed closed); J2 on three designs of the inspector (12:20).
- U1 holds 6e6c768e (the ribbon's unsettled marks) and the fix for CLASSIC's small dials.

**The next window's order, once `main` has moved** (each on a green whole tier of its merged tip; a late item rides
the window after):
1. card A: e260d2b (`Build` step ids) alone, then A1, A2 and the wiring;
2. N1: the 93 grade and `spec.fuel.octane` (fe250071, 4e057986, b225b7a6); then C: the knock bake's lighter pedals
   with the octane in the key (version 5); then U4: the number in TUNE;
3. D2: 9ef985c2 (the start's fuel);
4. G: the exhaust's half of the pops; then N1's seam, G reviewing;
5. ui only, any time the freeze allows: U1's ribbon marks and CLASSIC fix, U5's help lines, J2's design pictures.
Then sim/, host/ and tools/ freeze and P bakes, with the content-only items (the rotary's cited ports; the
two-stage build's high lobe at 247 deg on a centre of 98).

## 25 Nov 13:16: `main` held by macOS; the diesel package's root cause

- **The baked tip ce169231 is green on Linux and Windows and RED on macOS, 2 of 545, both U4's fuel control:**
  `ui.tune.fuel` (SegFault) and `ui.build.tune.fuel` ("The simulator's fuel table was not read": AKI - RON - MON -).
  The lead's diagnosis by reading ui/views/tune/Fuel.cpp: the table's values are written `91.0f` and read with
  `istringstream >> double`, which fails on libc++ (the trailing `f` is gathered and the conversion refused), so the
  table comes back empty on macOS only. U4 is released from the freeze for one ui/tests push with the cure; `main`
  moves to that commit when Linux and macOS are green on it (sim/, host/, tools/ still the bake's).
- **The diesels' root cause (D2, c24c127e local): 88 deg of valve overlap** from the reference's cam dial at 0.9999 on
  every D2 build (diesel-v8-truck, diesel-mechanical-classic, diesel-pickup-tuned, diesel-marine-v12, the two
  trucks). Harmless on the reference turbo, ruinous under a machine's back pressure: the truck at 1000 / 1300 rpm
  makes 665 / 1121 N m on the dial, 1521 / 2572 on the tractor archetype's events, 758 / 1324 with the dial's overlap
  alone. Ruled: all of them come off the dial (cited events where found, else the tractor archetype's as its
  judgement), the fixture states a boost target (the no-target spring law limit-cycles every 2 s: N2).
- **The corrected fixture shows the machine's turbo at its limit while firing:** with a 260 kPa target the shaft
  stands at 1.000 to 1.050 of omegaMax at 1500 to 2000 rpm with the vanes wide open, on both wheels, so no brake
  control can engage (D2). N2 re-reads the match on a healthy engine first (the wheel past its choke at rated, the
  turbine's match, or a target held flat where a maker's tapers); then D2 retunes the brake once.
- N2's (b) after its rule's correction (ed48581f: the map's own efficiency at the margin's point, the cooler's
  drop): tractor 76.2 mm, the truck's sets 73.9, the 2.0 36.6. The lead's build, tractor on 76.2: 1458 / 2577 / 2718
  / 2815 N m at 900 / 1000 / 1100 / 1300. The guard's removal is cff355f9 on (b): no shipped build gains a low
  state; tractor 900 1403 to 2080 N m, pickup 1500 698 to 955, the 2.0 at 1750 158 to 253 (N2).
- **The diesel package (a later window, together):** D2's builds off the dial and the fixture's target; N2's match
  re-read, (b), the guard's removal; D2's brake cure on the result.
- Card A: all of A1, A2 and A3 built and tested locally (the transport on a loopback server, the turn loop on
  scripted streams, the keychain store, the chat panel on this tree's kit with a thinking row). For J2 from it: a
  `TextField` where only Enter commits and leaving keeps the draft (multi-line with Shift+Enter); an error colour in
  the kit's Look.
- L: today's exposure stays (the percentile pumps; the crack rule is inert; the throttle's cue pumps or swells); the
  tractor's cabin written down as the design's limit. **L's first item now: the starter's peak holds the exposure
  6.1 to 6.8 dB low for ten seconds and more after every start** (M's table; first.json's `sound.autoGain` +9.2
  against +16.0 with the starter muted), for the next window if ready.
- M: the owner's hum closed at a warm idle (the pump's 3895 Hz from +8.6 dB over its band at Side on the owner's
  build to undetectable; the voices' sum 39 to 56 dB under the body). The fan's 280 Hz tone returns whenever the fan
  runs (+7.6 to +21.7 dB over its band): a cure held for a later window (its tones 10 dB under its own rush).
  A fixed mode near 2.2 to 2.3 kHz excited at the firing rate, no voice: for G and L.

## 25 Nov 13:43: `main` at 76420f32; the window is open

- **`main` moved d0a7330e to 76420f32 at 13:43 UTC** (fast-forward): run 45623044537 green on linux-x64 (545 of 545,
  13:38) and macos-arm64 (13:42); sim/, host/, tools/ equal the bake's tools sha dbd64a5c by the lead's diff (0
  lines). In it since d0a7330e: D2's two cures of the idle spark trim (the dyno's low end, petrol and diesel), P's
  bake on them (table N: 114 of 847 missed), U1's hover help and DRIVE's CLASSIC, DIGITAL and RACE styles, U4's fuel
  control with its macOS cure (the table read by `strtod`, not a stream: libc++ refuses `91.0f`), L's restated listen
  cases. Nothing marked, nothing weakened. Windows was still building when `main` moved (not a gate).
- **The diesel match, not in this window** (N2, tractor-map-reading.md and rated-levers.md, local): the cited shroud
  (above ratio 3 only) changes no wheel and is not built; the 1.15 floor costs the tractor 23 % at 1000 and does not
  clear rated; the map's width is not the fault (2.64 at ratio 2.6); the model's top line at the tip limit: knee
  3.93, island 3.36; at a flat 260 kPa no wheel is clear at rated; 80.4 mm with the taper's 202 clears rated (0.963)
  and loses 16 % at 1000; 76.2 mm with a rated target of 160 to 180 keeps 1000 (0.98). **No ruling on the rated end
  yet:** first the model's map against P's published maps in shape (top line, the surge line's place at ratio 2.8 to
  3.0, flow per wheel area), then what a lower rated target costs at rated (power, lambda, the smoke limit). P's
  sources, section 21 (1fa216c0): BorgWarner S300SX3 and S400SX-E, choke over surge 3.4 to 4.0 at ratio 2.0 and 2.35
  to 2.6 at 3.0, top lines at 561 and 607 m/s; for the speed margin at rated only a patent's 94 % and a forum's
  figure.
- **D2's (c), local 811c744d:** its four builds off the 88 deg dial (the archetypes' events as judgement, idle trims
  with them); on their own turbos the V8 3016 to 3143 N m, the marine 3412 to 3501, the others within 2 %; idles and
  starts as before. **GradeTest :169 is red on it and NOT restated yet:** the stronger brake takes the engine under
  the cut-out and "the rig runs away at idle" at a road speed above the one it began at in the same gear; the lead
  asked whether the driveline opened (the integrator's) or the brake has no cut-in (D2's) before any change of gear.
- N2's (b) reaches D2 as one patch file under docs/handoffs/patches/ (a docs-only push), for a first retune of the
  brake.

## 25 Nov 13:59: the window's first pushes; two findings held outside it

- **On the branch since `main` moved (tip 1f02d786):** the steward's 47b69019 (libcurl and libsecret in
  scripts/bootstrap-cloud.sh) and 884e192b (the soak only on `[soak]` / `[soak-all]`, by hand, or nightly at 03:23
  UTC once `main` carries the file); U5's 94bd0261 (a help line on every item of every stage, `ui.stages.help`
  failing on an item without one; 546 of 546 on U5's merged tip); N1's fe250071, 4e057986, b225b7a6 (the 93 grade,
  `spec.fuel.octane` in AKI, 0 = the grade's own). C's knock bake version 5 and U4's octane number follow N1's.
  76420f32 (= `main`) is green on Windows too (13:50).
- **The compressor map's two judgement coefficients are low against published maps** (N2 on P's sources, section
  21: BorgWarner S300SX3 and S400SX-E): the rise `0.45 U^2/(cp T)` gives a knee of 3.93 at 560 m/s where the maps
  peak at 4.6 to 4.7 at 561 (back-solved 0.512 to 0.539 at 521 to 561 m/s, 0.467 at 607; the lead's own arithmetic
  on 4.65 at 561 m/s: 0.516); the choke flow per inducer area 164 kg/s m^2 at ratio 3.0 against 184 and 224. The
  width is about right (2.64 at 2.6 against 2.7 and 3.0). **Ruled: a model-wide table, local, predictions first**
  (both coefficients from the two maps by a rule stated first; validated on the same maps' lower speed lines, which
  P has read, 7ad4c0bc; then every boosted build re-read, petrol validation rows included). It lands in its own
  window with its own bake. **(A) is the fallback and is measured:** the tractor on (b)'s 76.2 mm with the vanes
  fully open at rated makes 482 to 483 kW at a shaft of 0.969 to 0.985 and keeps the published 1000 (2580 N m);
  fuel and lambda (1.67 to 1.75) unchanged, the smoke limit never cuts.
- **The robotised clutch slips open under a strong compression brake and never closes** (D2's trace of GradeTest
  :169 on the package's valves: -1450 N m at 1150 rpm; slip -0.4 to -30 rad/s in 1.3 s; the brake drops out at its
  cut-out, correctly; the engine at idle with the road at 1816 rpm in gear 11). The case keeps its gear and gains a
  check that the brake is still on at the end; the cure is the integrator's (vehicle.fm about :720-765), asked
  13:57, local until called. Not reachable on `main`'s builds as far as measured (the old valves' -1324 N m held).
- **Nits from the lead's look at U5's pictures (the lead's own build of 25d7b8e1):** SCOPE's tooltip for the
  pressure plot lies over the knock-by-cylinder row its own text points to (U5, a later ui push); DYNO's "none yet:
  edit the build" runs past its panel's right edge (U4, with the octane number's push).

## 25 Nov 15:23: the window closed for sim; the bake done; what was found while it ran

**On the branch since `main` (76420f32), all in P's bake (tools sha 758bb12f's):** the steward's packages and the
soak by request only; U5's stage help; N1's 93 grade and `spec.fuel.octane`; C's knock bake version 5; U4's octane
number; D2's start fuel; L's start exposure; G's bangs burned in the pipe with N1's seam (default still Carved);
the integrator's clutch held closed on the overrun (c1ab538a); card A's assistant through A3 with its fixes (the
manifest's valid JSON, the link's state under `!_WIN32`, the name rule: c457e849); J2's three inspector designs and
the kit's draft field and error colour; X's soak shards; U1's ribbon marks, CLASSIC dials and a tab for every
inspector page (cb3e97a9); B's SIGPIPE ignored in the app's mains (fb33cb9a). The lead's own runs: 581 of 581 on
88ebef01; the 79 affected cases on the frozen tip. P: pre-bake tier 577 of 577; the bake 14:56 to 15:14, 28 knock
tables and 14 cam maps, the gate's 31 idles before and after, five builds Current at once.

**Process, the lead's faults and their cures.** (1) "A whole tier on the merged tip before every push" could not
converge with a dozen pushers (three agents on a third run): changed to one green tier on a merged state, then
merge a moved tip and push, CI judging the combination. (2) U1's permission check allows only a pure fast-forward
of a fully tested state; it was not worked around: the tip was held still for it (two runs lost to pushes that
crossed the notice; the third, on a named sha, went in). For U1 the tip is held from its merge to its push.

**CI on the assistant (card A), each found by the first build on that platform:** Windows: unused private fields
(clang-cl), cured in A3; macOS and Windows: the save-as test on a case-blind disk: ruled that a name differing only
in case is the same name on every platform, new names plain ASCII, the test reading the folder's listing (macOS
584 of 584 on c457e849); macOS: `assistant.sim` killed by SIGPIPE: the link never raises it (c5ef4d7: SO_NOSIGPIPE
on every socket, the wake channel a socketpair, EPIPE = the peer is gone, a test that fails on any SIGPIPE) and the
application ignores it (B); the `.mcpb`'s manifest was not valid JSON (the steward's first build of it), cured with
a parse test. **Never run against Claude Desktop or the API.** Unverified until the owner tries: which MCP revision
Claude Desktop speaks (the bridge answers the handshake of 2025-11-25; a client of 2026-07-28 alone fails);
Gatekeeper on a CI-downloaded `.mcpb` (the quarantine flag); the Keychain and a real key.

**GARAGE lists the test suite's fixtures** (app/GarageLibrary.cpp walks content/builds recursively: combo/'s five
archetypes, pack/ and findings/ x001 to x040, builds reduced to diverge; in `main` today). Now: U5's filter with a
test. Next window: the fixtures leave content/ in one step (X moves; N1 repoints BoostInletTest.cpp, Probe.cpp,
X022Test.cpp; the integrator core/EngineTest.cpp:566; P the manual's table).

**The soak has never been green; X's triage** (each bisected, none a pop, no bound moved; Known.h): X-030, X-031
(the spark's ramp after the cut, c035fe75; the downshift under fuel.model 1): the steward; X-032 (576c1c6e, the
idle valve's integral through start), X-034 (68e683f1, the valve's area), X-037 (valve.dpOverlap on the pickup):
N1; X-033 (5c85736b): N2 finding whose; X-035 (custom engine without boost, the large diesel): the integrator and
D2; X-036 (turbo V8 dual, running mean 0.0057 to 0.0069 against 0.005): the integrator and the steward; X-029,
X-038 (a primary or tail of hundredths of a millimetre): G, T, N2; X-039 (a 50 cc single with a 4 L blower): N2;
X-040 (a 0.025 mm runner): L. One cure to weigh across the near-zero pipes: the Spec's lower bounds on diameters.
Shards: 16 triples and 4 fuzz ranges; about 53 minutes on the runner where it took 92 (first sharded run pending).

**The compressor map (N2 on P's sources, sections 21 and 22).** Two judgement coefficients were low against the
BorgWarner S300SX3 and S400SX-E (the rise 0.45 against 0.51 to 0.54 back-solved; the choke flow); a flat correction
was refuted by the same maps' lower lines; the surge line at speed sat 1.5 times too far left. Fitted to all seven
cited maps with a hold-out (one heavy-duty map predicting the other): c(M) = 0.608 - 0.157(M - 1); the knee
(0.349 + 0.551(M - 1)) kAr; the fall 0.652; choke 0.506; mean error of the peak ratio 3 % (today's 14 %), of the
surge flow 6 % (26 %); the two heavy-duty wheels disagree beyond reading precision (trim, not entered). **Ruled:
that map, with one floor at the lowest cited line (Mach 0.67) for the knee and the efficiency.** The lead's wrong
guesses on the way, each refuted by N2's measurement: the turbo four's loss as surge or class (it was the
efficiency law read at Mach 0.30, under every cited line); the 2.0's vanes as a control fault (the back-pressure
limit holds them, and forced shut the pumping takes the gain back); the nozzle matched at the torque peak (it is
matched at rated on the flat target).
**THE DIESEL PACKAGE, N2's half final (668c642e local; docs/handoffs/patches/n2-package.patch):** the ruled map,
(b), the whole-flow nozzle, no guard, the taper from the fuel curve or ceiling. Tractor 72.4 mm: 2397 / 2580 / 2721
N m at 900 / 1000 / 1100; rated on the taper 482 kW at 0.899 of the shaft's limit, no flags (a flat 260: 0.986 to
0.990). Truck's sets 69.8 mm: 2507 / 2653 / 1867 at 1000 / 1300 / 2100, a choke flag at 2100 (known limit). The
2.0 38.4 mm: 252 N m at 1750 against VW's 320 (a miss, to be a row; the branch as shipped reads 154), rated clear.
Pickup 64.7 mm. Boosted validation rows: misses 45 to 46. MapTest's two knee cases restated on the cited
heavy-duty lines (not loosened). **Research after it, replacing (b) only if every build keeps both ends:** rule (c)
(the smallest wheel that clears rated) with the turbine's open area matched on its own efficiency at rated; first,
why the engine flags choke where the rule's check passes.
**D2's half:** its builds off the 88 deg dial (811c744d), the fixture's target, **the torque and power ceiling**
(`ecu.torqueMax`, `ecu.powerMax`: the ECU's fuel ceiling for electronically governed engines; the truck's fuel
curve, computed from its own published torque and power, was the forbidden kind and goes; **rows a ceiling holds
by construction leave the validation counts**; a mechanical pump states a delivery, not a torque), the brake
retuned once, GradeTest's check that the brake is still on at the end.

**The clutch (the integrator).** The paddles' robotised clutch followed the engine's own speed (0.357 of capacity
at 1116 rpm on the truck). In the bake: held closed by the road's speed once it slips backwards (c1ab538a).
Measured on `main`'s law: a forward slip of 130 to 160 rpm for 4 to 5 s under a floored pedal at 1070 rpm. Local
for the next window: one latch for both directions (87629688; set on the first lock, released by a shift, neutral,
the clutch pedal or the stall band); all 46 vehicle cases and the combination's fast tier identical; the dog box's
case and the grade's case move as the law says; a patch for bike.fm (B2's).

**The bangs.** G's clips are with the owner (in G's session): Carved against Burned for first.json's fuel-cut
limiter (Burned: 0 bangs), the flat-plane V8's spark cut (peaks -3.0 dBFS against -10.7; 10 dB over the body),
a fuelled overrun (1 bang in 7.5 s), the rally five's anti-lag (19 dB over the body). Local: 8c9f4c20 (a bang's
turbulence from the flow the ports let in, capped at sound: each bang's own waves had stirred the next one's flame)
and BangTest's books (222 kJ for 5.3 g at 42.2 MJ/kg). The auto level is pulled down 8 to 13 dB by the bangs: L's
rule to measure (a part over twice the window's 80th-percentile part does not raise the exposure). The turbine's
books do not see the burns ahead of it (for N2's lane). The default flips only on the owner's ear.

**The 2.3 kHz "mode"** (L, G): the ground's reflection at the Tailpipe camera (a path difference of 0.262 m:
notches at 650, 1957, 3270 Hz), not a pipe or the body. Candidates for a later window: a reflection falling with
frequency; a few millimetres of slow jitter on the image's path.

**For the owner to choose:** the inspector (J2's three designs; Rows recommended, the card next); the bangs'
default after the clips; the idle's level; the listening questions.

## 25 Nov 18:37: `main` at 43006034; what the baked commit met on the way; the next window

- **`main` moved 76420f32 to 43006034 at 18:37:19 UTC** (fast-forward). Run 45669105123: linux-x64 587 of 587,
  windows-x64 580 of 580 (attempt 1), macos-arm64 587 of 587 (attempt 2: GitHub cancelled attempt 1 for runner
  capacity before a step ran; the lead re-ran that one job). sim/, host/ and tools/ are P's bake's (758bb12f's; 0
  lines by the lead's diff). The dispatched soak on the same content: 217 of 219 in 66 minutes (N2's known diesel
  turbo full; fuzz random 103, a dc mean of 0.0062 against 0.005, X classifying).
- **Four timing-dependent tests found by CI on the baked commit and cured before `main` moved, none a fault of the
  engine or the bake:** the assistant's connect burst refused on macOS (the listener's backlog of 4: now SOMAXCONN,
  the test waiting on `accepted`); the host's stress case on Windows (it slept 50 ms where it must wait for
  commandsApplied; C: one writer, nothing can stay failed while audio runs); the assistant's thread count on Linux,
  twice (the kernel's /proc entry lags pthread_join by under 0.1 ms; `allowDesktop(false)` does join); with them
  card A restated five instant-count assertions and widened three duration bounds to 5 s (each still separating a
  10 s or 300 s fault). The lead did not re-run a failed test to get green.
- **P's rows on the baked content: 112 of 847 missed** (table N: 114). The V12 idles at +30.9 deg BTDC where it sat
  at -14.5. The rotary's power and torque in band on its cited ports (206 kW, 313 N m). For N1: the rotary's torque
  peaks at the dyno's lowest step (2294 rpm, one turbo; band 4500 to 5500); the two-stage's combined curve notches
  13 N m across its switch; a dyno hold below the switch reads stage B (the start overshoots to 6190 rpm and the
  hysteresis holds).
- **The diesel package is complete locally and green on its own tree** (N2's eleven commits,
  docs/handoffs/patches/n2-package.patch: the ruled map, (b), the whole-flow nozzle, no guard, the taper, the VGT
  loop's conditional integration, the integral held while the ECU owns the vanes, X-039's inlet node solved
  implicitly, the shaft's ceiling at 0.948 of omegaMax; D2's half: its builds off the 88 deg dial, the fixture's
  target, `ecu.torqueMax` / `ecu.powerMax`, the brake retuned once without its own shaft integral, the cylinder
  cut's case, GradeTest's checks; the integrator's one latch 87629688). Tractor at 1000 rpm 1507 to 2584 N m; the
  truck's sets at 2000 out of a relay cycle through the relief (+4.3 %, the choke flag gone); the brake 287 / 361 /
  331 kW; the release's shaft at most 0.981 (was 1.080). Known limits in docs/models/diesel.md.
- **The click check is restated on its property** (X, e792f732 local): a click is a step its neighbouring firing
  periods do not have; it still sees X-014 and X-015 on their old tree, clears X-033, X-034 and X-035 as firing
  trains, keeps X-030, X-031, X-032.
- **Research list (N2), nothing built:** rule (c) (the smallest wheel that clears rated, a mean phi of 0.96) with a
  turbine matched at rated, which needs the exhaust's back pressure after the turbine from G; a feed-forward vane
  position (the same missing p4); the re-application's ring of about 6 s and its +16 % peak; the six's unsteady
  cell at 2000.
- **Research list (the steward): X-030, X-031, X-032, X-036, nothing landed** (measured locally on 87a014f5, the
  pull, six shift phases, the click step and X's excess beside it; the scratch rows in the steward's hand-off).
  - *A wrong prediction, kept:* with `exhaust.model` on, `exhaust.bang` 1 and 0 read the same, number for number, at
    the tip and with the cure: the bang account books only the limiter's, the overrun's and a misfire's burns, not a
    dog box's cut. Under either choice the dog cut sounds the same today.
  - *The cure tried* (a dog box's cut takes the fuel with the spark, `launch.dogLeft` in Ecu1's `fuelCut`; c035fe75's
    ramp reverted): it clears X-030 on the V8 at every phase and not on the small four, so it did not land:

    | fuel+ecu, pull | excess bound | the tip (reds of 6) | the cure (reds of 6) |
    |---|---|---|---|
    | V8 | 4.03 | 4.73 (11.02 s, X-031), 4.71 [cut], 3.73, 3.86, 3.53, 3.13 (2) | 2.86, 2.17, 2.32, 3.28, 2.39, 4.49 (11.02 s) (1) |
    | small four | 2.85 | 3.21, 3.05, 3.13, 3.17, 3.41 [cut], 2.45 (5) | 3.18 (8.70 s), 4.70 (8.52 s, the cut's start), 2.57, 2.39, 2.44, 3.19 (11.03 s) (3) |

    With `exhaust.model` on as well, the V8 is green at every phase at the tip (excess 1.9 to 2.8) and with the cure
    (2.9 to 3.2). The small four's 8.52 s step falls as the cut begins: under fuel.model 1 the wall film still feeds
    the cylinders after the injectors stop. Next: the cut's start and end apart (the fuel cut some ms ahead of the
    spark, as an injection-led shifter does), both builds, six phases, before any cure is proposed.
  - *X-031* (the V8's downshift at 11.02 s) is not the cut: one phase in six whatever the cut is, and before
    c035fe75 too. The event there is to be found.
  - *Realism:* a real ignition-cut quickshifter (most motorcycles', many sequential cars') sends the cut's charge to
    the pipe and can pop; a fuel cut does not. While nothing books a cut's charge, a fuel cut loses nothing audible.
    When G's account books it, the cut's kind becomes a stated choice by gearbox (ignition or fuel), not a default.
  - *X-032* (the turbo six's tour, exhaust+fuel+ecu): the cause is that combination's unsettled start and idle (N1's
    traces), not the post-start hold, which stays. *X-036* not yet measured.

## The owner's rulings and a new fault, 25 Nov 19:10 UTC

The owner answered the open questions and named the wheel. In their words:

- **The bangs' default**: "The 'Burned in the pipe' sounds better, so yes". `exhaust.bang` 1 becomes the default, in
  this window, before the bake: G, on its 8c9f4c20 and BangTest (slot 4 of the sim order). Every default-run test
  that assumed Carved is restated on its property or asks for 0 by name; nothing loosened. The owner did not say
  whether the spark-cut and anti-lag bangs are too loud: levels stay. L's exposure rule follows G's push; the
  steward's X-030 cure is tested with `exhaust.bang` 1 on G's fix.
- **The inspector**: "As recommended": Rows now, and the card on the picture next (`docs/inspector-designs.md`,
  designs 1 and 3). J2 finishes Rows as shipped code and writes the frame's hook for U1; then the card, ENGINE and
  EXHAUST first. Dials is dropped.
- **The idle's loudness**: "As designed". No change.
- **The wheel**: "I have a Logitech G29 controller with a manual shifter attached. I should not need any external
  software for it to work, as TestingPlayground was working without any new installed software/drivers (lights
  included)." So no probe first: card B ports TestingPlayground's G29 code (IOKit's HID manager on macOS, the
  switch out of compatibility mode, 900 degrees, the spring, the five shift lights, the Driving Force Shifter;
  Linux through the kernel's hid-lg4ff; nothing on Windows yet). B's session may not read that repository (its
  permission check refused the attach, and B did not clone it another way): the lead, whose session holds the
  clone given for cards J and A, put the files unchanged in `docs/reference/testingplayground-wheel/` with their
  sha256 for the SEED.tsv rows. The lead's rulings for the port: pedals and clutch to `Driver::setAnalog`; paddles
  shift; the H shifter is the lever on a manual box with the automatic clutch off; steering is read and drives
  nothing (a straight line); the lights follow the number the DRIVE view's own shift lights use; the PS button is
  the starter; a WHEEL row in SETTINGS; the gamepad path must not also drive from the G29; `tee-wheel-dump` ported
  for the owner's terminal; fake-device tests only. The macOS code is compiled by CI and never run in the cloud:
  the owner's Mac is its first run. B's generic probe (`TheEngineEngine --probe-input`, f25b4a88) stays, for any
  other wheel and for Windows.
- **A fault the owner saw in `main`** (a screenshot of DRIVE's classic dials at idle): "text on visual dials
  overlaps the needle". The small dials' numbers crowd the hub under the needle; the large dials' needle strikes
  through the ring of numbers ("0" on ROAD SPEED); the small scales show converted values (51, 76, 102 psi). U1:
  a needle never crosses a number or the read-out, round steps in the shown unit, and a picture test of every dial
  at rest, mid scale and full scale.

**The order of this round changed at 19:15.** U1's pure fast-forward was crossed a second time (X's 0f677370, seven
minutes after the lead named the tip; U1 had 582 of 582 on 238bfc7b + a6e00140 and did not loop). The lead's fault:
no "holding" was collected from X first. U1 now goes LAST in the round, with the dialog, the dials' fix and the
Rows hook in one push; the lead collects a "holding" from every pusher before naming its tip. Released at 19:15:
D2 (the combined diesel package), then B2; B (`wholeBuilds` 4561f81e and the probe); the steward (2fc03a65: every
push since a16dc1f0 has started a soak); C; card A (9833352).

## Open, after the bake: the mechanical voices' cost on first.json (M, 25 Nov 20:10)

`tee-bench --engine --scenarios free-revs` on first.json with the fan forced on: voices.model 1 at 3054 ns a 96 kHz
sample against 2722 with voices.model 0, 0.24 R (simulator / reference 2.004 against 1.766), against the voices'
budget of 0.08 R (measured 0.06 to 0.07 R on the V8 when the package landed). Not the fan's tone (that is some
+0.025 R while the fan runs, FanVoice 32 to 71 ns). One machine, one run of three repeats; its run-to-run spread is
some 0.04 R. M profiles where it goes after the bake (`tee-bench --profile --engine`), before proposing anything.

## The round at 19:44 UTC: what landed, what was found, what was ruled

**Landed on the branch since the diesel package (e9b2ef3f):** B2's bike clutch 9eb34e2a (60e87aef; the bike's suite
on the merged state, 582 of 582 before it); the steward's 2fc03a65 (a soak only by a tag alone on a line); card A's
"Add a key" 98333523 and ae571141 (61ca5f0d, 587 of 587); **G's bang fix and the default, Burned in the pipe**
(81ec1b12: 8c9f4c20, BangTest, 9e71f6c7; 581 of 581; no test restated); **B's G29, first slice, macOS**
(5b66951a: `shell/input/g29/`, 591 of 591 on Linux with fake devices; the macOS body never compiled until CI's
macOS job, never run until the owner's Mac; the owner has the steps); J2's kit, Rows and the part card (4dd26210:
d426a0a9, 7e7c4ef7). The lead's own build at fec0f1fc: 51 of 51 on the diesel, boost and turbo tests.

**CI has not yet completed a run with the package**: each push replaced the waiting jobs. In flight at 19:43: macOS
on 60e87aef, Linux and Windows on 61ca5f0d (all three carry the package); 4dd26210 waits behind them and is the
first macOS compile of the wheel.

**G's predictions against results on the default's flip** (first.json, Burned against Carved): revMax 0.45 / 0.46;
exposed -5.2 / -5.3 dBFS; auto level +5.5 / +6.7 dB; near and far held (the crests' difference +0.6 from +2.4).
G's first prediction (revMax 1 to 3 dB lower) was wrong: the carved pops were not the largest peak. On a fuel-cut
build nothing bangs under Burned; the builds that burn are the spark cuts, anti-lag and a rich overrun: L measures
the exposure's duck on those before landing any rule (under 2 dB: nothing lands).

**N1, items 1 and 2 are one fault** (the two-stage's 13 N m notch; "the start's overshoot latching stage B"): the
cam's switch has a band (up at 612.6 rad/s, down 31.4 below) and the predictor read a point inside it in whatever
stage it arrived with: from a higher point (B held), or stepped from below with the absorber loop's overshoot of
about 13 % carrying it over the switch. Not the key start. N1's cure and test are a patch for C
(`docs/handoffs/patches/n1-predict-stage.patch`): 5740 rpm reads A at 192.1 N m (was B at 181.3). The lead's
conditions to C: nothing-latched builds bit for bit; the predictor does not learn the cam's name (the program says
in one place that a state is latched which an upward sweep would not hold); the overshoot path cured too. **Open,
C measures first:** N1 saw the unramped steps between points move the two-stage's other points by up to 5 %; is a
predicted point path-dependent on a build WITHOUT stages? If yes, it touches every validation row and comes to the
lead before any cure.

**N1, item 3, the rotary's low end, is N2's machine:** on the new map it still peaks at 2294 rpm (312.7 N m, one
turbo at 0.70 bar). Above the 4500 rpm join the secondary compressor carries a tenth of the air (0.016 to 0.029
against 0.13 to 0.20 kg/s), its shaft sits near 10.7 k, the primary runs into choke (0.72 falling to 0.50), boost
0.45 to 0.53 bar with the gate shut. N2: a diagnosis by measurement first; a small cure inside sim/boost in this
window, a change of topology in the next.

**Ruled, near-zero bores (X-038 exhaust, X-040 intake; G):** a Spec range cannot refuse them, because 0 means
"derived" on every bore. Now: in the builder and intake.fm's read sites, a stated bore in (0, floor) is raised to
the floor and the telemetry says which (floors: exhaust pipes 10 mm, intake runner 8 mm, snorkel 10 mm; judgement,
the smallest real part of the kind); bit for bit for every other build; finite at the floor shown, or it is a
solver finding. Before the freeze or next window, never between (the bake's keys hash sim/). Next window, C:
refusal at input in the loader and the inspector, by a rule in the host, NOT a new `fminus::` attribute (the
compiler is the owner's).

**A note on clocks:** N1's and L's containers run about five to seven minutes ahead of the lead's and the
server's. A time in a message is still from `date -u`, and is that container's.

## Card C, for the next window (25 Nov 19:58 UTC)

- **The stepped dyno approaches a held step from below where `tel.engine.hysteresis` reads 1.** N1 measured it is
  exposed as the predictor was: the two-stage held at 597 rad/s reads stage B, 191.6 N m when the run starts from the
  default `from` (150: the jump's overshoot, some 13 %, crosses the up switch), 184.7 when started at 590. Dyno.cpp's
  steps are its own timeline (the absorber's set speed), not the predictor's Rig, so the predictor's `approach()` is
  lifted into a helper both call, with a stepped-dyno test on the two-stage (each held step under the switch reads A).
  Not in this window's push; the predictor's side did not wait for it.

## 25 Nov 20:41 UTC: the window closed; what it landed; what waits

**FROZEN at 9c33806c** (C's predictor, merged on U1's 765fb16f). The trees the bake is of: `sim` 94c3c479,
`host` 80df52e0, `tools` 50b8ac3a.

**Landed since the 19:44 entry:**
- B: `tee-wheel-dump` 90cfbf0e (the G29's HID elements beside the table's rows). macOS: CI built and ran the
  application with the wheel's code at adf53f54, 594 of 594 (Windows 587 of 587); never yet run on a wheel.
- card A: `list_builds` / `load_build` on `wholeBuilds` (adf53f54): 66 whole builds by folder-qualified name, a
  name never a path, fixtures and parts refused.
- G: the exhaust's stated bores raised to a 10 mm floor, the telemetry naming which (5ff6b27d; X-038, X-029; the
  solver's own limit measured between 2 and 5 mm on three cases); the back-pressure note for N2 (docs).
- L: a burst does not set the auto level's exposure (a375b473): a part over FOUR times the parts' running 80th
  percentile goes to the limiter (at twice, a free rev's own pulses counted: the kei and the V12 went red).
  Burned against Carved, the body under a bang: the rally five's lift 6.4 dB under, now 0.8 over; its anti-lag
  3.5 under, now 2.1; **the V8's lift stays 4.5 under** (dense bangs are its body): not cured, stated in listen.md.
- M: the fan heard as rushing air with a tone inside it (d18a798b): the 280 Hz line against its third octave in
  the mix from +7.6 / +10.4 / +21.7 / +18.9 dB (Tailpipe, Rear, Side, Driver) to -23.6 / -21.9 / -14.4 / -15.5;
  uneven spacing's use cited (SAE 2008-01-0569), its size judgement.
- N1: `tel.engine.hysteresis` and `tel.engine.latched` (3bfc234b, with the integrator's lines, read and agreed;
  kCamBakeVersion 4); `tel.valve.dpWave`, the overlap pressure less an exhaust brake's back pressure, bounded
  as before, the brake's own pressure bounded by its stated limit (b07522fc; X-037 no longer reproduces, -230 kPa);
  the intake's bores' floors, runner 8 mm and snorkel 10 mm (28d4df13; X-040 runs finite at the floor).
- C: the predictor reads a point as an upward sweep reaches it where the program says a part can latch
  (cfab843b; kKnockBakeVersion 6): the let-down first, then the question, upward moves ramped, `approachForced`
  where it never clears. The two-stage at 5740 rpm reads stage A at 192.1 N m (was B at 181.3, the 13 N m
  notch). Builds without stages: bit for bit (the economy four's 16 points). Measured first: without stages a
  point's path moves it by one to two of its own standard errors (0.2 to 1.0 % on six points, three builds, no
  direction repeating).
- U1 (765fb16f, a pure fast-forward of 593 of 593 on exactly that state): the yes/no dialog for the assistant's
  questions; **the DRIVE dials** (a needle never crosses a number or the read-out; round steps in the shown unit;
  `drive.dials` checks every dial at rest, mid scale and full scale in both unit systems); **the frame shows
  Rows** (the owner's pick) and the part card; the settings' WHEEL row (shown once card B serves it).

**Not landed, by their owners' own second measurement:** the steward's X-030 cure (the fuel cut through the dog
cut cleared the V8 and moved the small four's reds, one worse: wall film feeds the cylinders after the injectors
stop); N2's joined pair on the rotary (evening the pair gains under 2 %: the pair's match is the fault).

**For the next window** (each with its owner's note in docs/handoffs or above): the rotary's pair and rule (c) and
the rated match (N2; G's back-pressure fit); X-030 / X-031 / X-032 / X-036 (the steward; X-032 is the turbo
six's unsettled start and idle); the stepped dyno's approach from below (C) and the dyno loop's 13.5 % step
overshoot, a PI's zero (the integrator); refusal of a bore at input, by a rule in the host and no new `fminus::`
attribute (C); the workspaces' warning for a raised bore; L's restated X-039 check with N2's inlet solve; the
V8's dense bangs and the exposure (L); the cut's kind as a choice once the account books a cut's charge (G, the
steward); the voices' cost on first.json, 0.24 R against 0.08 (M); the fixtures out of content/ (X, N1, the
integrator, P), after the bake; the wheel on Linux and Windows, a learn mode (B).

**Process, for the next round:** U1's pure fast-forward landed at the third attempt, once thirteen sessions had
each answered "holding" BEFORE the tip was named. A hold sent into a running turn is not read until that turn's
next message boundary: C announced a push while "holding" was still unread, and only a message that breaks into
the turn stopped it. So: the hold is asked first, every answer is collected, and a session that has not answered
is interrupted, not assumed.

## 25 Nov 22:05 UTC: THE BAKE IS ON THE BRANCH (223f4301); what it met; what `main` waits for

**The baked commit:** abadfb4b (P; `[soak]` alone on its last line), on the branch at 223f4301. Full bake, knock
version 6 and cam version 4: 28 knock tables, 14 cam maps, all 31 builds gated idle, warm and cold; the five
proofs (first.json, the V10, the tractor, the hot hatch, the rotary) read Current at once on an empty cache.
The trees are the frozen ones: `sim` 94c3c479, `host` 80df52e0, `tools` 50b8ac3a. P's rows and report follow.

**Two default-run tests met the content at their bounds, and neither was the bake's fault:**
- #348 `diesel.governor-injection` (before the bake): its "none" arm assumed the 2.0 archetype states no ceiling;
  the archetype now states VW's 320 N m. D2 made both arms explicit (ea1480e4), shown to hold and to bite.
- #337 `core: the engine's cases`, "the application's first build, written after a default init, runs as an init
  with it would" (after the bake): 2.6 % against 2 %. The integrator measured: 1.7 % on the old tables already;
  the read (3 to 4 s after full throttle) sat on a turbo still rising 7 %; with the same six bias draws on each
  route the routes pair within 1 %, so not the init order. Restated (8c0f746a): read at 5 to 6 s; bound 2.5 % from
  the spread of fourteen inits (2.4 %); a new EXACT check that every model switch is the same on all routes,
  which catches a dropped field a torque bound cannot (three shown). Baked: 349.2 / 346.7 / 349.2 N m, 0.7 %.
- **OPEN, the integrator's first item of the next window:** with the same bias draws AND the same states of both
  noise streams the two routes still part by up to 1.5 % at 5 to 6 s. Some other starting state differs between
  "written after init" (what the application does on every load) and "initialised over it". Find it by the diff
  of every field right after start; a field one route sets and the other does not is a fault. Until then the
  2.5 % bound sits 0.1 % over the measured spread and may go red at a later bake with no fault.

**Landed after FROZEN (ui, app, shell, tests, docs only):** B's WHEEL row (f4ac7487; macOS and Linux), the G29 on
Linux (7cc11146: evdev, the kernel's hid-lg4ff; never on a wheel), the lint fix for its three files (cbc78cfb:
B's Linux push failed scripts/lint.sh, the Linux CI job's first step; B found it, the lead ran the lint to
confirm), the whole-chain test for the assistant's question (8f0359fe) and its waiting-on-state form with a
print at a miss (9eb0dc09); U1's dial scales (8b6eae90: round steps filling three quarters of the sweep), Rows
attached at launch (1ce8d1b0: J2 found the launch workspace's rows dead until a workspace switch), J2's full
stop with U1's two re-blessed frame.help goldens (b79ac48e); J2's build.rows and the part card in the half of
the stage away from the press (3aaf92fd, b08060ad).

**RED on macOS, and it gates `main`:** `shell.app.cases`, tests/shell/app/AskChainTest.cpp:157 (the assistant's
load over a dirtied document: the frame's question never opens), 2 of 2 macOS runs (f1f60ab8, 67a4ebe5); green
on Linux and Windows. So not a slow runner: on macOS the question does not reach the frame, or the call returns
without asking. B's 9eb0dc09 prints the call's reply, the dialog, the document and the engine at the miss; the
next macOS run says which. If it is in the tools, card A's.

**Process faults of this stretch, each told by the one who made it:** J2 pushed 63131c17 on a tier count read
from a stale log (its watch matched the previous run's "EXIT" line); the real tier then passed 593 of 593; J2
now waits on the process and writes fresh logs. B's Linux push went out without the lint; B found it. The lead
"assigned" G another card's files (the intake's): a session's paths are not the lead's to widen; N1, who holds
them, did the work. A hold sent into a running turn is not read until that turn ends: the lead now breaks in
(priority now) on any session that may be mid-push.

## 25 Nov 23:25 UTC: `main` AT b557c4a1; the bake's report and soak; the next window's list

**`main` moved, fast-forward, 43006034 to b557c4a11ef924a805b95a6f79ab223ac9cd7b9c** at 23:25:03 UTC. CI run
45718145430 on that commit, read by the lead job by job: Linux 604 of 604, macOS 603 of 603, Windows green; it
contains the bake abadfb4b and its trees are the bake's (`sim` 94c3c479, `host` 80df52e0, `tools` 50b8ac3a).

**The red that held it, and its cure (B):** `shell.app.cases` on macOS alone, three runs of three. B's print at the
miss: "no question after 0.034 s, 1 frames; the call: an error {"error":"the owner said no; nothing was
loaded"}; the frame: its ask dialog made; the document: v8, unsaved changes; the engine: ready". Cause:
`app::Shown`'s destructor calls `panelGoing`, which clears the frame's asker; the rig move-assigned a `Shown` from
a temporary; libstdc++ and MSVC leave a moved-from `std::function` empty, libc++ keeps a small one, so on macOS
the temporary's destructor ran the hook and unhooked the live frame. The application itself constructs its
`Shown` in place (Main.cpp), so its dialog should work on a Mac; nobody here can run one. Cure b557c4a1: the hook
is a type whose move empties its source on every library, with a case that pins it. The lead's first guess (the
Apple-only WHEEL branch) was wrong; B refuted it by reading.

**P's report on the bake (Table P): 113 of 847 missed** (30 of 204 cited), against 112. In: the 2.0's boost; the
tractor's lug and take-up (the diesel package's wheel); the two-stage's peak torque (211 N m); the 6.2's speed
of peak torque; the 2.0's speed of peak torque once read where its held plateau begins (2145 rpm; the arg-max
of a flat line read 3491 to 3824). Out: the 2.0's "Torque at 1750 rpm", 244.8 N m against 304 to 336 (VW's 320);
the 3.5 TT pickup's peak torque 623 against 630; the rally five's speed of peak torque; the kei's header gas
1111 K; the two-stage's crossing (now the sustained crossing, 6539 rpm; the first tie read 3102) and its
sharpest point. **Re-timing alone** (the ten boosted builds, 60 s runs read over the last 30 s, dyno steps held
20 s): 111 missed on the old timing, 114 on the new, same content; three rows crossed an edge, all out; every
re-timed build's header gas rose 2 to 5 % (the 10 s runs had not soaked the pipe). **The two-stage's notch is
still there, about 17 N m, and is no longer a misread:** 192.9 at 5705 on stage A, 176.6 at 5879 on stage B,
195.2 at 6052: the cam switches about 100 rpm before the high lobe overtakes the low, and the high stage trades
the lead within 5 N m from 5750 to 6500 and is ragged at 3000 to 3500 (193, 193, 166). P's prediction (under
3 N m) was wrong and is said so.

**The bake's soak (223f4301): 213 of 221** (215 of 219 at 0f677370). X-029, X-038 and X-040 retire (the bores'
floors: all four fuzz shards pass). Red, by owner, each diagnosed the same evening:
- **Burned in the pipe on a spark-cut limiter (the rally five): the bangs' mean pressure is overstated.** Port
  7.54 bar, turbine inlet 8.94, manifold 3.51 against 3.84 / 4.72 / 2.41 under Carved; `valve.dpWave` -350 kPa
  against the ±300 kPa bound (N1's probe). G: the burn's heat enters as volume at the cold gas's density, so the
  mass flow is overstated by about the temperature ratio; and the turbine's books do not see the heat. The click
  at 8.74 s there is the lift's own blowdown front (it exists under Carved, 0.31 against 0.55), not a bang.
- **The large diesel with `engine.custom` and `boost.model`:** turbine inlet 6.31 bar against 3.13 at steady full
  load, vanes wide open, brake off: the turbine's match ignores the custom engine (X-035's family).
- host.sim.nightly, EngineTest.cpp:930 and :1038 (C: most likely the tests' assumption about tables reading
  Current); the VGT one-state full form (N2's known red, unmoved: 163.1 kPa, 2549 N m); the turbo six's clicks
  (X-032's kind; one new shard, one already red at 0f677370); the large diesel's dyno DC 0.0051 against 0.005
  (X-036's kind).
N1 keeps the ±300 kPa bound on `dpWave`: it caught two real faults on its first night.

**THE NEXT WINDOW, in the order the lead would take it (owners; each a prediction before a measurement):**
1. G with N2: the burn's heat released as hot gas, and the turbine seeing it; then re-measure the rally five on
   its limiter (port, manifold, `dpWave`, the click) before any bound moves. The default stays Burned.
2. The integrator: the starting state that differs between "written after init" (every load in the
   application) and "initialised over it": the diff of every field right after start; then #337's paired form.
3. N2 with the integrator: the turbine's match under `engine.custom` (the large diesel; X-035's family).
4. N1: the two-stage's switch against its curves' crossing (is 5850 rpm cited or chosen?); the high stage ragged
   at 3000 to 3500 and trading the lead from 5750 to 6500.
5. N2: the rotary's joined pair, rule (c), the turbine's rated match, the 2.0 at 1750 (G's back-pressure fit).
6. The steward: X-030 (the dog cut: the cut's kind as a choice once G's account books its charge), X-031,
   X-032 (the turbo six's unsettled start and idle), X-036.
7. C: its two nightly host cases (tests, at any time); the stepped dyno's approach from below; refusal of a
   bore at input by a rule in the host (no new `fminus::` attribute).
8. The integrator: the dyno loop's 13.5 % step overshoot (a PI's zero), weighed against C's ramp.
9. L: X-039's check restated with N2's inlet solve; the V8's dense bangs under the exposure rule (4.5 dB).
10. P: the gas rows' timing (the pipe's soak) as a rule for every build; the fixtures out of content/ with X,
    N1 and the integrator.
11. M: the voices' cost on first.json (0.24 R against 0.08).
Outside sim/ (open at any time): B: the wheel on Windows (SDL 3.4's HIDAPI Logitech driver), the owner's first
run on a real G29; U1 / J2: the part card from the keyboard, a raised bore's warning in EXHAUST and AIR, the
small dials' numbers if the owner asks; card A: nothing owed.

## Card D, for the next window (25 Nov 22:05 UTC; the lead's ruling)

1. **First: a state differs at the start between "written after init" and "initialised over it" (first.json).** The
   first route is what the application does every time a person loads a build, so a stale field there is a real fault,
   however small. Measured on the baked tables at 5 to 6 s: with the same cylinder bias draws written on both routes
   they still part by 0.06 to 1.6 %, and with the same bias and the same states of both engine noise streams
   (noise.rs, ex.noise.rs) by 1.37 / 0.09 / -1.46 %. Next: the diff of every field right after start between the two
   routes; pair them on what differs; anything one route sets and the other does not is a finding in the init order,
   to the lead before any bound moves. Then core :680 bounds the paired difference by its measure (keeping the exact
   check of every model switch and tq[0] == tq[2]); its bound today (f6c5d1ba: 2.5 % against a measured 2.4 % spread
   of fourteen inits) is thin.
2. The engine dyno's absorber loop overshoots a step by some 13 % (a PI's zero at zeta 1): to measure and weigh against
   C's ramped approach (docs/handoffs/d-mechanics.md).
3. The clutch's latch (87629688, on `main` after this bake): nothing open; the bike's form is B2's
   (docs/handoffs/patches/d-bike-synced.patch).

## 25 Nov 23:49 UTC: the window's first half hour: five faults found, none yet on the branch

Nothing under `sim/`, `host/` or `tools/` has been pushed yet (tip a6383652; the trees are still the bake's). Found
since the window opened, each by its owner's measurement, each with a prediction beside it:

1. **The burned bangs' pressure (G; cure local, in its tier).** The housing booked the burn's volume at the cold
   collector's temperature: 2.2 times the mass that exists (the housing's books 3.1 to 3.5). Cure: the housing
   receives the hot gas's temperature, T_coll + Q/(m cp), smoothed and capped at 2600 K. The rally five's tour:
   port 7.54 to 4.98 bar (Carved 3.84); turbine inlet 8.94 to 6.84; manifold 3.57 to 2.98 abs; `dpWave` on the
   limiter -350 to -155 kPa; the lift's largest step 0.527 to 0.088 (Carved 0.098): the click was the pressure,
   and G's prediction that it would stay (3.0 to 3.9) was wrong. Only the rally five's clip changes (its loudest
   10 ms 2.0 dB lower, its rms 2.2 dB higher); sent to the owner. New for N2: the boost on the bench limiter
   swings 0.86 to 1.8 bar on about 0.4 s.
2. **The duct's wall (the integrator found it; G cures it in the same push).** `Duct::set` redesigned the wall's
   loss shelves only when the length or the sound speed moved; the bore and the sample rate were not in the key.
   On the application's own load (the default V8, then first.json over it) the intake runners kept the V8's
   shelves. No torque in it (0.01 N m); a fault in every load all the same. Found by the diff of every field
   between "written after init" and "initialised over it": that diff becomes a default-run test (#337's new
   form), with the inert leftovers named; the torque check moves to a point where the knock loop is quiet (a
   1e-6 nudge parts a route from itself by 1.4 % at 3000 rpm full load, more than the routes part).
3. **A custom engine loses its header split (N2 diagnosed; the integrator's file and cure, in its tier).**
   `Engine::customLayout` took the header grouping from the mechanical bank and set topo 0: a custom inline six
   has one collector where its table has two 3-into-1s, so a twin-turbo six fed one turbine (0.410 kg/s at
   4.69 bar) and the other ran backwards with its shaft at 0. Not the turbine's match: both of N2's predictions
   were wrong and said so. Cure 823e45cd: a one-bank custom engine of its table's count keeps the table's split.
   Still to do: headers from the firing order for other custom engines; a default-run null that a custom engine
   equal to its table is the table's engine.
4. **A custom engine's friction (the integrator, open):** FMEP 3.14 bar under custom against 2.13 for the same
   engine (953 N m against 1123 on the large diesel). Unexplained; next.
5. **The exhaust's heat settle scatters every predicted point (N1 found it; G's file, after its push).**
   `settleHeat()` soaks the walls to the port gas of the last 16 samples (0.33 ms of a 20 ms cycle): a blowdown
   pulse or the cool tail by timing, the pipes up to 300 K too hot. The two-stage at 5879 rpm reads 176.6 N m
   with it, 188 to 193 without or on the stepped dyno; other builds scatter about ±100 K and under 2 %. **Table
   P's 17 N m notch is this, not the cam:** the 5850 rpm switch is Honda's cited figure and the held curves
   cross between 5875 and 5938. The lead's reading (the switch 100 rpm early) was wrong; so are the crossing
   row's 6539 rpm and "trading the lead": P re-reads both after the cure. Proposed cure: a cycle's mass-weighted
   port temperature and a cycle-mean flow. It bears on every baked table: the bake at this window's end.
   For G's judgement, not to be fitted: the two-stage's long primaries (1.63 times the layout) carry 7 to 14 N m
   drops within 63 rpm and a path memory of 3 to 5 N m through 12 s holds.

**Ruled:** N2's design for what a turbine is matched to (docs/measurements/calibration/boost/
turbine-match-design.md): (i) p4 and T3 from the engine's own rated point, as a BAKED calibration (with C), not
at every load; until baked a build runs on today's rule and says so; the pass shown to converge; (ii) a build
that cannot meet both ends is flagged, not compromised; (iii) the low-end check never moves the vane stop.
N2's step A (the match on hand-filled p4 and T3) is built and measured locally in this window and NOT pushed:
A lands with B (the bake, host/) in the next window.

**Outside the trees:** B's G29 on Windows (a6383652): read only as a plain joystick through SDL; its tables are
EMPTY until the owner has run `--probe-input` on Windows in each mode (switch at PS3, with and without
Logitech G HUB); nothing is driven from an unprobed axis. Open with the owner: whether G HUB is acceptable on
Windows for the clutch, the shifter, the range and the spring (Windows pads every output report to 17 bytes and
the wheel takes 7: without Logitech's driver nothing can be sent to it there).

## 26 Nov: the fast tier over its cap (the steward; the lead's ruling, 23:56)

CI's Linux log at 4354e4cc (job 139579878691, the reference runner, a Xeon 6975P-C, scale 1.0; scripts/test-budget.sh
on it): the whole fast tier **2492 test-s against 2400**. Shell 68 of 80 (B's chain test in): under, nothing moves.
Over their share, largest overrun first, with each package's heaviest entries:

| package | test-s | budget | owner | heaviest entries (s) |
|---|---|---|---|---|
| diesel | 217 | 135 | D2 | governor-injection 75.9, brakes 48.1, turbo 34.0, truck 33.2 |
| host | 140 | 70 | C | host.sim.cases 133.9 |
| listen | 143 | 80 | L | the auto level near and far 18.5, the voices through Places quick 18.2, the level at the defaults quick 13.8 |
| calibration | 143 | 80 | N2 (boost) | the VGT one state short form 27.6, the matched wastegate 19.0, the stated join speed 17.7 |
| calibration-na | 115 | 80 | N1 | X-022/024/026 together 27.1, the warm start on p-idle-warm 21.0, the start on the dyno 18.1 |
| combo | 202 | 165 | X | combo.fast-3 78.7, fast-2 63.1, fast-1 59.9 |
| core | 120 | 80 | the integrator | core: the engine's cases, one process 114.7 |
| vehicle, ui, ref, sim, boost | 107, 102, 101, 93, 84 | 100, 80, 100, 80, 80 | I, U, K, the integrator, N2 | (small overruns: after the above) |

null is at 373 of its 580; its unused share is what lets the tier pass today.

**The rule (the lead's):** a tests-only item, behind each owner's window work. First group cases onto a kept player
or one built engine (the setup is usually the cost, not the property). Move a case to `[nightly]` only where a
remaining default-run case still guards the same property, and name that case. Never drop one. Report the package's
test-s before and after on the steward's measure (CI's Linux log through scripts/test-budget.sh). No share moves until
the owners have reported what grouping alone recovers. D2 is woken for diesel's only after the window's sim work is
in, unless its 217 keeps growing.

## FREEZE STATE (the lead keeps this the last section; read it before every push)

- **25 Nov 23:25 UTC: `main` IS AT b557c4a1** (the bake abadfb4b; Linux, macOS and Windows green on that commit).
- **25 Nov 23:27 UTC: THE NEXT WINDOW IS OPEN under `sim/`, `host/` and `tools/`** for the items of the list in
  the section above, and nothing else. From the first push under those trees the branch's baked tables are
  stale until the next bake; `main` stays the stable build.
- **Order under `sim/` (each waits for the one before to be on the branch where their files meet; say early what
  will not make the window):**
  1. G, then N2: the burn's heat as hot gas; the turbine seeing it; the rally five on its limiter re-measured.
  2. the integrator: the starting state that differs between the two init routes; #337's paired form.
  3. N2 with the integrator: the turbine's match under `engine.custom`.
  4. N1: the two-stage's switch against its crossing; the ragged high stage.
  5. the steward: X-032 (the turbo six's start and idle), X-031, X-036; X-030 after G's item 1.
  6. C (`host/`): the stepped dyno's approach from below; refusal of a bore at input by a rule in the host.
  7. N2: the rotary's pair, rule (c), the rated match (a design first, to the lead, before any number).
  8. the integrator: the dyno loop's step overshoot, after C's item 6.
  9. L: X-039's check with N2's inlet solve; the V8's dense bangs, after G's item 1.
  10. M: the voices' cost on first.json (measure and report first).
- **Rules, as before:** a prediction before each measurement; no part built from its own validation targets; a
  red default-run test fixed or restated on its property with proof it bites, never loosened; one green whole
  default tier on a merged state, then merge a moved tip and push, saying what was not in the run;
  `scripts/lint.sh` before pushing; wait on the tier's process, not on a line in a reused log; a refusal of a
  session's permission check is never worked around.
- **Outside those trees, open at any time:** `ui/`, `app/`, `shell/`, `assistant/`, `tests/`, `content/` (not the
  baked tables), docs. **U1** pushes only a pure fast-forward of a tested state: it tells the lead; the lead
  collects a "holding" from every pusher (breaking into running turns), then names the tip.
- **Then the lead writes FROZEN FOR THE BAKE here, P bakes, and `main` follows the baked commit's CI.**
