# The ECU (card H, `ecu.model 1`)

*What `sim/ecu/` models, with each rule, where each number comes from, and which are judgement. `ecu.model 0` (the
default) is the reference's ECU unchanged (its `control16`'s driver, idle valve, spark trim, limiter, overrun cut and
state machine; `charge()`'s knock rule, one retard for all cylinders, and pops by chance). Status: built and tested
standalone on a one-line engine (tests H3, H4, and the parts' below); not yet wired into the simulator, which waits for
the split (K3b): see "Integration" at the end.*

## Files

| File | What it holds |
|---|---|
| `maps.fm` | `EcuMap16` (the shape of `Map16`, a stand-in until `spec/tables.fm` lands), `EcuTune` (the formulas), `EcuMaps` (outputs and baking) |
| `limiter.fm` | `CutPattern` (an even pattern of cut firings), `SoftHold`, `RevLimiter` (the four kinds) |
| `idle.fm` | `EcuSpeed` (the speed it goes by), `IdleControl`, `SpeedDriver` (the driver for `ctl.holdSpeed`) |
| `state.fm` | `EcuKey`: the key, cranking, stall, run-down, a start's and a cold engine's fuel |
| `launch.fm` | `LaunchControl`: launch, flat-shift, a dog box's cut and a downshift's blip asked by card I |
| `knockctl.fm` | `KnockControl`: retard per cylinder |
| `overrun.fm` | `OverrunCut` (the reference's), `FuelAccount` (the bangs by account) |
| `traction.fm` | `TractionControl` (and the pit limiter), `AntiLag`, `Nitrous` (their switching) |
| `settings.fm` | the Spec-shaped settings, appended to `spec.ecu` at integration |
| `ecu1.fm` | `Ecu1`: every piece in order every 16 samples, and each firing's decision (`fire`) |

## Maps

Seven maps, 16 by 16, bilinear between breakpoints (exact at them: `a (1 - f) + b f`), flat beyond the ends: spark
advance, lambda, boost target (speed by gear), intake and exhaust cam phase (card E reads them), the pedal's map to
the plate, the lift command. Axes: speed (rpm inside, rad/s in the Spec) and load (the manifold over the air
outside; for a throttle with no plenum, the pedal). Each has `auto`, on by default: the output is then the formula:

- spark (the reference's): `12 + 22 sqrt(min(rpm / red, 1.2)) - 8 (p_manifold / 1 atm - 0.4) + 22 (burnK - 1)`, then
  `spec.ecu.timing` added and each cylinder's knock retard taken off, as the reference's `charge()` does.
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
`ecu.overrun.fuel` (a share of a stoichiometric charge) is injected anyway, burned `ecu.overrun.retard` late.

The bangs by account (`FuelAccount`, per bank): fuel and air leaving a cylinder unburned are added to the bank's store;
both flow out with a 0.25 s time constant (judgement); a hazard rises at `40 /s x hot x burnable / 2 mg`, hot = 0 under
900 K to 1 at 1200 K (judgement for the self-ignition of a rich exhaust's CO, H2 and fuel with air), burnable = the
fuel the store's air can burn; at 1 the burnable fuel burns and card G's pop gets an amplitude of 3e9 Pa a kg
(judgement: a few milligrams give the reference's 20 to 30 kPa). Standalone test (part of the card's 9): at 3000 rpm,
1100 K, 2 mg a firing gives 99 bangs in 2 s, 1.9 mg burned a firing, the largest 23 kPa; 8 mg gives 199 bangs, 7.8 mg
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

## Telemetry (`tel.ecu`, at integration)

Spark asked and delivered, the knock retard in force (the most, and per cylinder), knocks counted, lambda asked and
seen (`watch.fm`'s sensor), injector duty, fuel flow, exhaust temperature, the half-burn angle, peak pressure and its
angle; the flags (`ECU_FLAG_*`: limiter, fuel cut, spark cut, overrun, launch, flat-shift, anti-lag, nitrous, traction,
pit, knock); the idle valve; the state.

## Integration (waits for K3b)

Append `EcuSettings`' fields to `spec.ecu`; switch `EcuMap16` for `spec/tables.fm`'s `Map16` (and its read, if its z
is ordered the other way); in the carved `Ecu`, `ecu.model 1` fills an `EcuIn` every 16 samples and runs `Ecu1`,
`Combustion` asks `Ecu1.fire(c)` at each intake closing; keep model 0 the reference bit for bit (the null test, card
test 5); then the integrated tests 6 to 11.
