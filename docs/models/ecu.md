# The ECU (card H, `ecu.model 1`)

*What `sim/ecu/` models, with each rule, where each number comes from, and which are judgement. `ecu.model 0` (the
default) is the reference's ECU unchanged (its `control16`'s driver, idle valve, spark trim, limiter, overrun cut and
state machine; `charge()`'s knock rule, one retard for all cylinders, and pops by chance). Status: built, tested
standalone on a one-line engine (H3, H4, the parts) and wired into the simulator's `Ecu` (`sim/ecu/ecu.fm`) behind
`ecu.model`: see "Integrated" at the end.*

## Files

| File | What it holds |
|---|---|
| `maps.fm` | `EcuTune` (the formulas), `EcuMaps` (outputs, an exact read of `spec/tables.fm`'s `Map16`, baking) |
| `limiter.fm` | `CutPattern` (an even pattern of cut firings), `SoftHold`, `RevLimiter` (the four kinds) |
| `idle.fm` | `EcuSpeed` (the speed it goes by), `IdleControl`, `SpeedDriver` (the driver for `ctl.holdSpeed`) |
| `state.fm` | `EcuKey`: the key, cranking, stall, run-down, a start's and a cold engine's fuel |
| `launch.fm` | `LaunchControl`: launch, flat-shift, a dog box's cut and a downshift's blip asked by card I |
| `knockctl.fm` | `KnockControl`: retard per cylinder |
| `overrun.fm` | `OverrunCut` (the reference's), `FuelAccount` (the bangs by account) |
| `traction.fm` | `TractionControl` (and the pit limiter), `AntiLag`, `Nitrous` (their switching) |
| `../spec/ecu.fm` | the settings' modules and `spec.ecu`'s fields of model 1 |
| `ecu1.fm` | `Ecu1`: every piece in order every 16 samples, and each firing's decision (`fire`) |

## Maps

Seven maps, `spec/tables.fm`'s `Map16`, 16 by 16, bilinear between breakpoints (read by `EcuMaps.exactAt`, exact at
them: `a (1 - f) + b f`, where `Map16::at`'s `a + (b - a) f` can be a rounding off at the last), flat beyond the ends: spark
advance, lambda, boost target (speed by gear), intake and exhaust cam phase (card E reads them), the pedal's map to
the plate, the lift command. Axes: speed (rpm inside, rad/s in the Spec) and load (the manifold over the air
outside; for a throttle with no plenum, the pedal). Each has `auto`, on by default: the output is then the formula:

- spark: the half of the burn 8 degrees after the top (docs/models/combustion.md, "The spark"); the charge refines it
  with its own dilution and flame; `spec.ecu.timing` is an offset over it, and each cylinder's knock retard comes off
  it. (Before integration it was the reference's `12 + 22 sqrt(rpm / red) - 8 (load - 0.4) + 22 (burnK - 1)`, which
  put the half-burn 0 to 9 degrees before the top at speed.)
- lambda: 1 to 80 % load, then `1 - 0.12 w - 0.08 b` (w: 0 to 1 over 80 to 100 % load; b: 0 to 1 over 1 to 2.5 load):
  0.88 at full load unboosted, 0.80 at 1.5 bar of boost (judgement: factory tunes' power enrichment).
- boost: the build's boost level in every gear (card F's number); phasers 0; plate = pedal; lift 1.

Baking: `EcuMaps.bakeOne` writes the formula at the map's breakpoints (its own, or the defaults: speed 500 rpm to 10 %
past the redline, load 0.1 to the build's highest, gears 1 up, pedal 0 to 1) to `baked`, `bakedX`, `bakedY`,
`bakedNx`, `bakedNy`. The host copies them into the map and turns `auto` off. Test H3: a map reads its 256
breakpoints exactly and interpolates between within 1e-5; a baked map reproduces its formula at every breakpoint,
bit for bit, for spark, lambda, boost and the pedal.

## Loops

- **Idle** (the reference's, moved): idle speed `max(base + 350 max(cam - 0.35, 0) / 0.65 + cold, 350)`, cold `rise
  clamp((328 K - coolant) / 75 K, 0, 1)` (rise 500 rpm, a diesel's half: `ecu.idle.coldRise`); the idle valve an
  integral (0.5 per 1000 rpm a second) and a proportional part (0.5 per 1000 rpm, 0.2 s ahead); the spark trim `1 +
  1.6 (idle - rpm - 0.08 rate) / 1000` clamped to 0.55 to 1.35 (driven, the reference's car variant); the dashpot. All
  numbers the reference's.
- **The driver for `ctl.holdSpeed`** (the reference's key governor, moved): gains 0.15, 0.15 s look-ahead, integral 0.8,
  spark trim 2.5 per 1000 rpm with its integral.
- **Knock control**: per cylinder, `retard += step (1 + intensity)` up to `most`, given back at `recover` a second
  (1.5, 15, 2: the reference's numbers, which it applied to all cylinders at once); off at the user's risk.
- **Boost control**: the target is the map's (above); the loop that moves the gate or the vanes is card F's (agreed
  seam below).
- **Over-boost cut** (added by card N2): the fuel is cut once the boost (the manifold over the air) has stood over
  `spec.ecu.overBoost` for 0.2 s, until it falls under 85 % of it; 0 takes the boost machine's own over-boost level
  (`Boost::overBoostLevel()`: 1.4 times its target or spring, turbos only, handed over by Engine as `Ecu::overBoostRef`),
  -1 never. Production ECUs have such a boost limit (Bosch ME7's and its kind's); the 0.2 s and 85 % are judgement. It
  is the net under a build whose gate cannot hold its boost: the 2 L four with a 26.3 mm gate, spooled at 5271 rpm,
  runs away to 249 kPa and 434 N m with it off, 151 kPa and 250 N m with it at 150 kPa
  (tests/sim/calibration/boost/GateTest.cpp).

Test H4 (the one-line engine: `tests/sim/ecu/oneline.fm`, eight cylinders, 0.2 kg m^2, 300 N m at a full manifold,
a 40 ms manifold, a plate 30 ms behind, the torque two firings after the intake closes): the warm idle holds at 750
rpm within 0.1 rpm after 20 s; cold (coolant 290 K) it idles at 971 rpm (target 974) and, the coolant warming with a
100 s time constant, comes down to 750 within 30 rpm by ten minutes. Found on the way: with the torque three or four
firings late (270 or 360 degrees) the reference's idle gains limit-cycle on this plant, plus or minus 75 to 110 rpm;
the reference's idle loop has little phase margin.

## Limiters (`ecu.limiter.kind`; the speed `ecu.limiter.speed`, 0 the layout's redline)

- **Fuel cut** (the reference's): every cylinder's fuel cut over the limit until 140 rpm under it.
- **Spark cut**: the same hysteresis; the injectors keep firing, the sparks stop: the charge goes out unburned (to the
  bangs' account).
- **Soft**: over the last `soft` (300 rpm) the spark is taken back up to `retard` (15 degrees) over the first half,
  then firings are cut by fuel, an even pattern of a rising share (`CutPattern`: a first-order sigma-delta, so a share
  of a third cuts every third firing and no cylinder is cut every cycle), all of them at the limit.
- **Throttle**: the plate's ceiling held 50 rpm under the limit (0.3 of the plate a 1000 rpm, 100 ms of look-ahead, an
  integral closing at 3 and opening at 1 a 1000 rpm a second: judgement, tuned on the one-line engine), with a fuel cut
  200 rpm over the limit behind it.

Test H4, free revs with the pedal floored, a 6500 rpm redline, measured over 4 s after 2 s: fuel cut 6342 to 6564;
spark cut the same (on this plant a cut is a cut); soft 6451 to 6480; throttle 6357 to 6465 (its band asked:
-200/+50); a limiter speed of 5000: within 4700 to 5150.

## Launch, flat-shift, card I's requests

Launch (`ecu.launch.on`): with the launch button, or the clutch down at a standstill (under 1 m/s) and the pedal over
0.9, it holds `ecu.launch.speed` (4000 rpm) by the soft hold over 300 rpm, retard up to `ecu.launch.retard` (20) and
sparks cut (so the fuel burns in the pipe, heating it and the turbine); it lets go as the clutch comes under half way,
the car moves, or the pedal lifts. Flat-shift (`ecu.launch.flatShift`): an upshift made with the clutch while the
pedal stays over 0.9 holds the speed the engine had, the same way, until the clutch comes up. Card I's dog-box cut
(seconds) cuts every spark; its blip (a plate) holds the plate at least that open. Test H4: launch at 4000 holds 3952
to 4000 rpm; let go, the engine runs on to the limiter.

## Overrun and the bangs

The cut (the reference's): pedal under 0.02 for 0.12 s above idle + 700 rpm; out under idle + 320 or with the pedal.
`ecu.overrun.fuel` is the share of the overrun's firings fuelled anyway (each a stoichiometric charge, in the cuts'
even pattern: a little fuel in every cylinder would be too lean to light), burned `ecu.overrun.retard` late; model 1's
charge burns in the cylinder the share `1 - spark after the top / 60 degrees` of it (judgement) and sends the rest to
the pipe.

The bangs by account (`FuelAccount`, per bank): fuel and air leaving a cylinder unburned are added to the bank's store;
both flow out with a 0.25 s time constant (judgement); a hazard rises at `40 /s x hot x burnable / 2 mg`, hot = 0 under
650 K to 1 at 900 K of the simulator's port temperature (`exh.tPort`, which reads some 770 K at full load on the
limiter, some 300 K under measured exhaust temperatures: judgement, for the self-ignition of a rich exhaust's CO, H2
and fuel with air at some 950 to 1200 K), burnable = the
fuel the store's air can burn; at 1 the burnable fuel burns and card G's pop gets an amplitude of 3e9 Pa a kg
(now saturating: 30 kPa (1 - exp(-m / 7 mg)), the reference's largest pop; X-006). Standalone test (part of the card's 9): at 3000 rpm,
1100 K, 2 mg a firing gives 133 bangs in 2 s, 1.9 mg burned a firing, the largest 23 kPa; 8 mg gives 199 bangs, 7.8 mg
a firing, the largest 47 kPa (four times the fuel: four times what burns, larger pulses); no fuel, no air, or a pipe at
800 K: none.

## Traction control, the pit limiter, anti-lag, nitrous

Traction control (`ecu.traction`): `reduce = clamp(3 e + I, 0, 1)`, `I += 6 e dt` within 0 to 1, e = slip - target;
retard `20 min(2 reduce, 1)`, firings cut by fuel `0.75 clamp(2 reduce - 1, 0, 1)` (judgement: spark first, then
cylinders, as factory systems take torque out). Standalone (part of the card's 10, on a toy wheel whose slip settles
over 50 ms toward 0.4 (torque share x 2 - 1)): held at 0.101 against a target of 0.10; off, 0.40. The pit limiter: the
soft hold on the car's speed over its last 2 m/s.

Anti-lag (`ecu.antiLag`): inside its speed window with the pedal under 0.2, running: retard 30, lambda 0.85 (defaults;
judgement). Nitrous (`ecu.nitrous`): armed (its button) above its speed, the pedal over 0.95, in or above its gear:
retard 4, lambda 0.85 of the larger oxygen (defaults; judgement). Card F burns and flows both.

## The key and the engine's state

`ctl.ignition` 2 cranks while held (in gear only with the clutch down), running past 550 rpm; let go first, it stops
cranking; 0 runs it down (stopped under 2 rad/s); a stall is 80 ms under 30 % of the idle (250 to 450 rpm). No fuel
while stopped, running down, or cranking under 120 rpm warm (40 rpm cold): the reference's. Cranking a cold engine
asks lambda times `1 - 0.3 cold` (cold 1 at -20 C, 0 at 90 C; judgement), decaying over 20 s after the start; a cold
engine runs lambda times `1 - 0.08` at 0 C to 1 warm (judgement). Test H4: it cranks, runs, settles over 700 rpm,
stalls under a 400 N m load, and runs down to rest with the key off.

## Telemetry (`tel.ecu`)

`knockRetard` (degrees, the most of the cylinders'; the reference's under model 0), and under model 1 (0 otherwise):
`advanceAsked` and `advance` (degrees before the top: asked, and the last firing's with its knock retard), `knockByCylinder[16]`,
`knock` (a count since the start), `lambdaTarget`, `lambda` (the sensor's), `injectorDuty`, `fuelFlow` (kg/s),
`halfBurn`, `peakPressure`, `peakAngle` (cylinder 1's last cycle), `flags` (`ECU_FLAG_*`: 1 limiter, 2 fuel cut, 4
spark cut, 8 overrun, 16 launch, 32 flat-shift, 64 anti-lag, 128 nitrous, 256 traction, 512 pit, 1024 knock retard, 2048 over-boost cut),
`idleValve`, `idleTarget` (rad/s), `boostTarget` (Pa), `state`, `smoke` (0: the diesel's is not wired yet), `limiter`
(1 while a limiter acts), `load` (the maps' y axis now). Exhaust temperature is `exhaust.portTemperature`.

## Integrated

`Ecu::control` runs the reference's control and, under `ecu.model 1`, `Ecu1` over it (`control1`); the state machine,
the key mapping and the held-speed driver stay the reference's under both models (Ecu1's own `EcuKey` follows the
engine's state for its fuel cuts and enrichment). Both models 0: the null test identical to the bit (59 pairs).

Integrated tests (`tests/sim/ecu/IntegratedTest.cpp`, the V8 in neutral in the car):

- **9**: a clean overrun (a full fuel cut) makes no bang; a spark-cut limiter makes 638 in 3 s (30 g of fuel burned in
  the pipe: the rich full-load charges' whole fuel); the overrun's fuel at a share of 0.1 and 0.4 (30 degrees late)
  burns 7.2 and 19.9 mg in the pipe over 2.5 s of lift (2 and 4 bangs: the pipe cools under its lighting threshold
  within a second or two of the lift).
- **11**: random pedals (and the launch button) through each limiter kind, 40 s each in ctest (`H_SOAK_SECONDS=600` for
  the card's ten minutes, by hand): finite, within +-1, the most speed 6457 to 6709 rpm against the 6500 redline (the throttle kind's the highest), still
  running.
- The bake: each `ctl.bakeMaps` press writes all seven maps into the Scope; the spark's first point is the reference's
  formula.
- **10**: the V8 in the rear-drive coupe (card I's vehicle, street tyres): from rolling in first, floored: the slip held
  at 0.116 (most 0.139) against 0.10, flag 256, no stall, 14.9 m/s against 12.5 without (slip 1.0). Traction control's
  authority fades from 1500 to 300 rpm over the idle (its integral holds meanwhile), or it would drag the engine into a
  stall while a barely moving car's slip ratio stays near 1.

**The idle under model 1** (`content/validation/` through `tee-validate`, the builds with both models 1):

- The reference's idle loop is marginal: at its idle valve's proportional gain the plenum's 0.1 s lag lets it settle
  into a limit cycle from some starts (a mild-cam V8 under model 1: 605 to 798 rpm round 700, the same equations as
  model 0's, which happened to start calm). Model 1 halves the valve's proportional gain within some 150 rpm of the
  target (full beyond 300, and for 3 s after a start, whose flare falls fast toward a stall), and its spark trim has
  an integral that takes the excess torque out once the valve is shut (the torque reserve a real idle's retarded
  spark keeps), giving it back at once under the target.
- A diesel's idle under model 1 is a governor of its idle fuel, with no floor under it (the reference's lever never
  went under 0.12 of full fuel plus the valve's share, which held a heavy diesel at 1121 rpm), a proportional gain four
  times the spark engine's (no plenum between), a warm target of 0.28 of its redline at most 750 rpm (heavy diesels
  idle at some 25 to 30 % of rated speed, 600 to 800 rpm: judgement), and no big cam's bump.
- Measured (8 to 12 s of `idle-warm`, held warm): the default build 700 rpm (680 to 719; manifold 39 kPa); the V8 with
  a cam of 0.35 700 (679 to 718), of 0.6 835 (819 to 851); the large diesel 706 (657 to 727, the governor's approach).
  `tee-validate`: the default build passes every target but the port gas (all builds, task of the energy share); the
  large diesel passes its warm idle (73.97 rad/s), misses its governed speed (2534 rpm against 1800 to 2400: its
  layout's redline; `ecu.limiter.speed` now sets where a diesel's governor cuts under model 1); the V8 build misses its
  warm idle (1159 rpm): it carries `valve.cam` 0.9999, a race cam, so the reference's rule raises its target to 1050
  and its overlap makes it lope (993 to 1316), as race-cammed V8s do. The V8 and large-diesel builds' cams are the
  content's to choose.
- Cold starts with both models (X-003): the enrichment of a start is fuel for the cold walls (`Ecu1::fuelFactor`); the
  burn sees the lambda asked. Both start the V8 at 253 K (0.32 s, 1515 rpm) and 233 K (0.64 s, 1639 rpm).

