# D: mechanics, layouts, the integrator (integration done)

Package D, branch `claude/awesome-gates-ntdb58`, cloud session. Model, sources, judgement:
[docs/models/mechanics.md](../models/mechanics.md); the seams: [sim/README.md](../../sim/README.md) ("A custom
engine", "The hooks"). Pushed through bf0ec81; this file and the test fix after it.

**Done, by test** (`tee-tests-core`, 21 tests, all pass; `tee-tests "[sim]"` null test bit-identical on every pair):
- 1: `NullTest` unchanged; `custom off: a build's custom fields filled in change nothing, to the bit` (0 of 96 256 differ)
- 2 to 5 (standalone, as before): slider-crank, firing intervals (33 presets, the reference's 17 exact), inertia torque
  with a 20 s soak, shaking and balance shafts
- 6: `the engine dyno holds a set speed within 10 rpm at full throttle...` (V8 build with the biggest cam, the custom
  V8, the custom six: 1991.7 to 2000.7 rpm at 2000, absorber = gas less friction to 0.1 %); `...sweeps at its rate`
  (300 rpm/s: +0.2 rpm mean error, 49 rpm worst, the cam's surges)
- 7: `the default V8 at full throttle: BMEP...` warm (oil at 100 C): BMEP 10.2 to 10.8 bar 1500 to 4500 rpm (8.5 at
  5500, 7.5 at 6400); FMEP 1.03 to 2.46 bar 800 to 6400; VE 0.86 to 0.94 (air). Cold (just started) the reference's
  oil factor puts FMEP at 3.3 bar near the redline. Sources: docs/models/mechanics.md
- 8: `a sixteen-cylinder build starts, runs and stops`: 9756 rpm within 2 s, idles 764, stops
- 9: the whole suite after each push to `engine.fm`: 306 tests, all mine pass; failing, not mine: ContractTest on
  `spec.exhaust.primaryTo/collectorTo/sides/tips` (G) and `spec.car.gears` (I) (no unit, quantity or choices), and six
  `ui.*` golden frames (the tests do not load the simulator)

**Integrated**: `engine.custom` (layout from geometry, bore/stroke/rod, redline by piston speed, a rotary's ports,
energy-exact crank with the inertia torque, friction by part; a geometry change under the fade); MODE_DYNO_ENGINE on
every engine (40 rad/s loop; the bench's load dial rests); `tel.engine`: torque (brake), gasTorque, firing, power,
IMEP, PMEP, FMEP, BMEP, VE x2, peak pressure, piston speed, ripple, dyno torque and speed, inertia torque, shake x4,
firingAngle[], cylPeak[], cylPeakAngle[]. As integrator: every hook of the integration queue as a stub in its owner's
module, called only with its package's `model` 1; the model switches; env, Scope and telemetry fields the lead listed;
names settled (README "The hooks").

**Measurements**: R = 1942 ns (this container). Default engines with all hooks and reports on: 1.050 R against the
reference (K3a: 1.011): +0.04 R. Custom V8 as the default (18.9 % vs 18.8 % of a core at 48 kHz); the V16 1.63 times
that, about 1.7 R. Inertia, crank step and work integral standalone: 69 ns (8 cyl), 117 ns (16).

**Not done**: the housing seam's per-sample sequence (F and G add it at step 4; README says where); shaking as a sound
(wave 2); the two-stroke (card T) stands where 5caecb4 left it (docs/models/twostroke.md).

**Conventions to know**: the reference's tables fire their orders mirrored (Engine places a cylinder at theta +
fire); a custom layout fires in its geometry's order. `tel.engine.firingAngle` reports what runs.

**FMinus issues**: none. **To listen to**: the V16 and the custom V8 on free-revs (out/ via tee-render --engine
--build content/builds/engines/v16-45.json); the dyno sweep of the V8 build (its cam's surges at low speed).

**Since (to 6612cd9)**: X-007 safety (wrapCycle, guard/recover, FLAG_FAULT = 4); `fuel.model` 1 and custom imply
Chen-Flynn friction, and `fuel.model` 1 implies Woschni walls; shaking for every engine; `TEE_ENGINE_RATE_48000`;
`veManifold` and `veAir` bounded to 0 to 4, `veManifold` held below 5 kPa of manifold (E's ITB plenum reads -4330 Pa on
the lift). The rotary twin's torque: 320/380 N m at 4000/6500 rpm with every model 0 (the reference's rotary, ~1.3 times
a real 13B's BMEP); boost.model 1 doubles its boost at 4000 rpm (F's). Hidden `[.rotary]` in EngineExplore prints it.
Open: a rotary wall heat under fuel.model 1 and the engine.fm tidies (areasFor, walls if/else, voicesInputs,
crankStep), both awaiting the lead.

**Since (to 24 Nov 18:00)**: the walls' loss counted once (e81f79f: `Engine::woschniWalls()` into `comb.walls`); the
backflow conserved on the physical cylinder (e8c6575: port-side stores at intake and exhaust; with N1's plenum lines,
5fdc5ad, throttle, intake valves and exhaust valves agree within 1 % on the flat-plane V8); `tel.engine.chargeTemperature`,
`trappedMass` (959454c, the air's temperature until a charge: X-017); `ecu.overBoostRef`; a two-stroke's mechanics
from its own cylinders and its BMEP 2 pi T / V (fc8a3ba, 5fdc5ad); the tidies `areaExhaust`, `crankStep`,
`voicesInputs` (85ef0a6, c138d0f); boost.model 1 implies the builder's exhaust (6b7e4b7, `Exhaust::wanted`). The
rotary's chamber walls and seals (2df7ea6): both rotary points now short by their breathing (docs/models/mechanics.md).
Open: Boost::exhaustStandIn's removal (N2, then engine.fm's call); D2's flap and vanes released on hd.on's fall (D2).

**Listen to (the owner's ears)**: big-cam engines on the model-on path (fuel.model 1): idle and lope, now from real
residuals (flat-plane V8 45, V12 65, V10: idles 998, 882, 952 rpm); every archetype's low-speed torque and idle after
the backflow and the walls' single count; a boost.model 1 build's exhaust (the builder's now, coupled to its housings);
the rotary twins at full load (port gas 1100 to 1400 K).

**Since (to 24 Nov 18:25)**: a diesel's walls counted once (9e3a44f: chargeDiesel keeps 0.98 with Woschni's walls,
the reference's 0.62 without; the "dead diesels" of 17:59 were a marginal start in a one-second-crank harness, not a
fault in the engine); a test that the diesel archetypes and the turbo four make torque and keep their books within 1 %;
Engine no longer calls Boost's exhaust stand-in (910fed3); a firing engine let go from its start runs past half its
idle speed, both ECUs, null unchanged (a64e511); tel.engine.layoutReason (6141a60). Reviewed: card I's car.traction,
D2's flap and vane release (62484fc). Failing, not card D's: diesel.diesel turbo (the VGT's 2x over the single).

**Since (to 24 Nov 19:10)**: the cylinder's side of an overrun's pop (76690a7: `Cylinder::exUnburnedJ`, the per-burn
books; N1 takes it into H's store); the model switches' decisions taken again after every readSpec (f89d40c: before,
a switch set while stopped on a build with the default layout and size never reached `comb.walls`, `conserve`,
`exh.implied`); core under the test budget, its long forms [nightly]; the V12's teeth traced to the intake runner's
standing wave (not the cylinder; N1's end loss ab65e06 takes most of it); a firing engine let go from its start runs.
**Listen to**, added: an overrun's pops on the model-on path once N1 lights exUnburnedJ in the pipe.

**Listening note (19:30)**: builds and renders before f89d40c are not the ones to judge the default V8's model-on sound on, where a model switch was written while the engine stood (after the program's init, with no layout or size change to follow it): those ran without the physical cylinder and the implied exhaust.

**Listening note (20:20)**: under the physical cylinder every petrol engine's blowdown is a little softer and cooler since 3990ee47 (Gatowski's gamma); the default V8 at full load is the one to hear.

**Open for the next window (25 Nov, the lead's note)**: the engine dyno's absorber loop (sim/core/dyno.fm) overshoots a
step in its set speed by some 13 % (N1: 150 to 597 rad/s reaching about 655, estimated, not traced), which is what let a
held dyno step latch the two-stage cam's high stage. The cause is the loop's form, not its tuning: a PI placed at zeta 1
puts a zero at s = -wn/2, and that zero alone overshoots a step by about 13.5 % though the poles are critically damped.
The candidate cure is set-point weighting (the proportional part on the speed alone, not on a step of the set speed;
the integral and the feed-forward of a sweep's acceleration kept), or a first-order prefilter on the set speed at wn/2:
either takes out the step's overshoot without slowing the loop's hold of a lumpy cam. To measure first (trace the step
on the default V8 and the two-stage), predict the change in every dyno test's print, and weigh against C's ramped
approach (C ramps the stepped dyno's approach next window; with the ramp the overshoot may no longer matter).

**Open for the next window, its FIRST item by the lead's ruling (25 Nov 22:00; core :680, first.json's two routes; a state that differs between the application's own load and an init is a real fault, however small)**: 8c0f746a (on the branch as f6c5d1ba) reads the routes settled (5 to 6 s)
within 2.5 %, against a measured spread of 2.4 % across fourteen inits (each route's cylinders draw their own bias), with
every model switch held exactly across the routes. The margin is thin: a later bake may go red with no fault (the lead's
point). The sharper form, the same bias draws on both routes and the paired difference bounded, does not yet pair them:
with the same bias the pairs differ 0.06 to 1.6 % at 5 to 6 s; with the same bias and the same states of both engine
noise streams (noise.rs, ex.noise.rs) still 1.37 / 0.09 / -1.46 %. Some other state differs between "written after
init" and "initialised over it" and the run diverges from it. Next: diff every field right after start between the two
routes, pair them on what differs; if they then agree to well under 1 % bound the paired difference by its measure; if
something real differs (a field one route sets and the other does not), it is a finding in the init order: to the lead
before the bound moves. Changes after this note are docs only; not pushed while the branch is held.

**Done, 26 Nov (the window after the bake; on the branch at d3907e05)**: the item above is closed, and with it these.
- **first.json's two routes, field by field (#337).** A default case compares every field of the Engine instance between
  "written after init" (the application's every load) and "initialised over it". Its findings: the runners' wall
  (Duct::set keyed the wall on length and sound speed, not the bore or rate: G's cure e7088e2b, taken by the runners with
  N1's intake.fm line), the init order (below), and the start's noise (below). Today the routes differ in no
  configuration field; 2594 fields that hold still differ, every one of a named inert kind (counts in use, the model-0
  exhaust's pipes, the passivity cache, revision counts). The torque check, paired from before the key, is exact at
  1500 rpm full load (231.750646 N m on both). At 3000 rpm the knock loop's walk (a 1e-6 nudge moved the mean 1.44 %) made
  any bound there a test of the knock loop.
- **The init order** (393519ea): a stopped engine configures the engine the Spec describes at once, not at the next key.
  Before, the application's load of first.json reported the default V8's redline, cylinders and size until the key.
  It carries G's exitTemperature() guard, with G's agreement.
- **The start's noise**: the key's first block drew each route's own noise before the shared streams were set (the
  valves' hiss); that alone parted ecu.target in its last digits. The cases set both streams before the key.
- **Custom engines**: the straight six's split headers kept under custom (823e45cd: twin turbos had fed one wheel); the
  general split by firing order for a one-bank engine whose table splits its headers or whose twin turbos sit a bank
  each (9c107de6, e0bdc69f); a null test that a custom engine written from its table configures as the table does.
  The 129 custom builds in content/ (probe [.customtopo], TEE_CUSTOM_LIST a list of their paths): 57 two banks (topo 2),
  18 split (topo 1), 54 one collector (topo 0), the same before and after. Of the 54, twin turbos only on the two
  rotary-twin-13 builds, a sequential pair whose second turbine takes its gas through the transfer valve: one collector
  by design (N2's; its weakness after the join is N2's next window).
- **The custom friction** read 1 bar over the reference's on the large diesel: the physical friction's cold oil (oilK
  1.505, 6 s from a cold Bench start); warm it is the reference's within 2 %. The predictor and the dyno read warm oil
  (oilK 0.985 at a warm-held point; coolant 368 K at P's rows).
- **The core package's budget**: 111.5 to 87.5 s here (the fan's cycle over 10 s, the energy cases on the shared
  player); the rest is three compiles each case needs by design (the lead accepted it).

**Open for the next window (26 Nov)**: heat.fm's oil viscosity (viscK 15 at 20 C oil, 5.9 at 40 C, against some 7 to 9
for a 15W-40) against a sourced viscosity table; the engine dyno's overshoot (above, after C's ramp); a V custom engine's
bank angles in the custom null (one-bank builds only today).

**Later, after X-047's cure (the lead's ruling, 26 Nov; docs/plan/x047-shift-cut.md, the steward's page)**: under
contract (b) the engine is cut while the vehicle asks and the number Car::cutRequest() hands is no longer read as a
length. Its name and meaning are the integrator's to tidy in car.fm (a flag, or the seconds remaining; the comment at
car.fm:847 says "how long"), with model 1's shiftTime and model 0's 0.05 out of it. Not in X-047's change.

**X-053 (X's, 26 Nov 11:11; mine, not yet read): a model switched while running.** X's Part B judges a switch thrown
mid-run in its own 0.3 s window from each flip. The turbo six's pull with flips at 5.0, 6.5, 8.0 and 9.5 s, 17 copies,
against the nightly bound 2.67: engine.wallHeat over in 3/17 (1.65 / 2.08 / 3.27), exhaust.model in 10/17 (2.05 /
2.84 / 3.89). Reproduce once B's harness is on the branch: FLIPPROOF="5 turbo-six" BOUNDPROOF_DT="..."
build-dev/bin/tee-tests-combo "[.flipproof]". First read: what the fade (cutWant, holdModel's held switch) does across a
running flip for those two models, and whether their step is the fade's or the model's.

**X-055 (X's, 26 Nov 18:11; my part, not yet read): the flat-plane twin-turbo V8 as built on the pull**
(content/builds/archetypes/flat-plane-v8-39-twin-turbo.json, the nightly's shipped-pull case). Mine: its 1-2 upshift on
the dual clutch, 1 of 17 copies over the nightly bound 2.33 (2.66 at +0.01 K, at 6.882 s; lowest 1.23, median 1.61),
the same on the tip and on the steward's cut probe (a DCT cuts nothing, so X-047 doesn't reach it). N2 is named for
the boost, the steward for the limiter (7 of 17); the "rest" (1 of 17, 2.76 at 4.532 s) has no owner yet. Reproduce:
BOUNDPROOF="corpus flat-plane-v8-39-twin-turbo pull" BOUNDPROOF_DT="<17 nudges, -0.15 to +0.15 K>" tee-tests-combo
"[.boundproof]". First read: the DCT's torque handover (gearbox.fm:294-320, aFrom / aTo across dur, a smoothstep) at
1-2 with the boost spooled: whether the step is the handover's shape, the driveline's lash or the boost's response to
the load step (N2's).

## Queue item 18, X-056: the sequential's dog shift at rest (next window; design and tests, nothing under sim/ yet)

Cause: gearbox.fm:283 sets the robot clutch to 1.0 for a dog shift whatever it held, and the synced latch is cleared
while shifting (gearbox.fm:177), so at rest the shift closes a clutch the launch had open. Cure of record: hold the
robot value from when the shift began, through the shift.

Tests (all through the application's path, the shipped archetypes/inline-five-turbo-rally, warm, no throttle):

1. At rest, up, up: engine above 0.85 x idle throughout and the car below 0.5 km/h. Fails today.
2. A synced 1-2 at 1500 rpm on the overrun: robot stays 1.0. Passes today, and must still pass.
3. The lead's case (measured on 35a733d1, tee-headless): at rest, E at 3.2 s (gear 1, 1024 rpm), E at 3.9 s (gear 2:
   424, 348, 217 rpm, back to 1074 by 4.6 s), Q at 5.0 s (2 to 1, 100 ms shut with its blip; 376 rpm at 5.05 s, state
   0 at 5.15 s, stalled for good). Test: at rest, up, up, down; the engine still running one second after the down.
   Fails today.

Case 3 measured on the Bench (27 Nov, tests/sim/core/GearsTest.cpp and the hidden "[.x056lead]"): it does NOT stall.
With the same spacing as the lead's trace (0.7 s between the ups, 1.1 s from the second up to the down) and the first
press 0.2, 1.0, 2.0, 3.0 or 3.2 s after the Bench's start, the engine falls to 316-402 rpm after the up to 2 and to
156-294 rpm after the down, and the state stays 2 (running) throughout. What differs between the two paths that can be
named without guessing: the idle the engine sits at (the Bench 1249-1257 rpm before the first press; the application's
trace 1024 rpm in first and 1074 rpm after recovering) and the start (the Bench's start() against the application's
key and cranking). Whether the downshift's blip reaches the engine the same way through the shell's frame is not
measured. Open; GearsTest marks case 3 on its dip (never under 0.85 x idle) as well as on still running, and it fails
today on the dip. The cure's prediction below covers both paths.

After the guard's shift (case 2), synced in second near 1000 rpm on the overrun, the robot slips between 0.28 and 0.57
("[.x056guard]"): the clutch letting go as the revs fall under the idle. The lead: not numbered; it gets a number only
if X's judge or an ear finds it.

**Item 18 measured (27 Nov, 89963aee):** the misses, as the lead took them. At rest the engine still dips 65 rpm under
idle (1190 against 1255; 30 was predicted) after an up: the up's 60 ms ignition cut (gearbox.fm, cut = t < 0.06)
still fires at a standstill with the clutch open, where it cuts nothing but the idle. A QUESTION for item 16's owner
(the steward, launch.fm's cut for as long as the vehicle asks), not for today: should the cut be asked at all when the
clutch is open? The robot reaches 0.03 (the launch's own drag at idle), not 0. On the application's path the car is at
0.53 km/h at 6 s: the idle creep in first before any shift (0.28 km/h at 3.85 s on both trees), so the prediction's
wording was wrong, not the cure.

Prediction for the held-value cure, to check when the window opens: the robot stays 0 through all three shifts of
case 3 (and both of case 1), since each begins with the launch's clutch open at rest, and so the engine neither dips
on the up to 2 nor stalls on the down to 1.

## Queue item 19, X-057: redlineFor at a governor's high idle (next window; notes, nothing under sim/ yet)

Read in code (12:15 UTC), D2's two facts hold:
- ecu.fm:125-130: on ecu.model 0, redlineFor returns fmaxf(base, idleRpm + 1500): a floor at idle + 1500 rpm. Under
  model 1, a stated limiter speed (at least idle + 200).
- engine.fm:786-789: spec.engine.redlineSpeed is read only when customUse != 0.

So three governed builds cannot state a redline at their high idle in content (D2's numbers, high idle against the
floor): diesel-v8-truck 2014 against 2324, diesel-mechanical-classic 2268 against 2355, diesel-marine-v12 1890 against
2208. The pickup is cured in content by D2; the two 15 L trucks already sit under their high idle (2188 against 2226).
The rule must let a governed engine's redline go beneath the idle + 1500 floor.

Null, corrected: every shipped build bit-identical except those three, by name. For each the design says what moves
(the band and the lights; diesel-v8-truck and the classic are eighteen-speeds with stated ratios, so their shift points;
the marine V12 has no gearbox) and whether anything keyed on the redline is baked for them (D2 found no baked table in
any of its builds; to check).

The six ecu.model 0 builds that state a governor, measured on 3ca7b90c ("[.x057figs]", 2.0 s after start; none states
highIdle, so the high idle is ratedSpeed x (1 + droop)): diesel-v8-truck 2014.3 against ecu.redRpm 2338.7 today;
diesel-mechanical-classic 2267.9 against 2364.9; diesel-marine-v12 1890.0 against 2361.5; truck-15l-six and its bobtail
2225.9 against 2186.6 (under: bit-identical under the min rule); diesel-pickup-tuned, after D2's f77a46e6 (rated 303.7 rad/s =
2900.1 rpm; re-read on 8d2e60f0): high idle 3074.1 against 3073.8 today (under: bit-identical). (My first read, on
3ca7b90c, predated f77a46e6: 2225.9 against 3264.8.) So item 19 moves exactly three builds, as ruled. D2's
tests/sim/diesel/RedlineTest.cpp:130 ([!mayfail]) checks the same three as GearsTest's X-057 case ([!shouldfail]). The lead's ruling (12:37): redlineFor itself, min(today's value, the governed high idle) for a stated
governor, the idle + 1500 floor not holding the cap up.

The steward's ecu.fm change (12:55, local, for my review): redlineFor computes today's value as before, then for a
stated governor only returns min(that, high), high = ecu.highIdle if above 0, else ratedSpeed x (1 + max(droop,
0.005)) (heavy.fm:201-203 without its fallback onto the redline). Their null (48 shipped builds driven, audio hash and
engine.redline): diesel-v8-truck 2330.55 -> 2014.33, diesel-mechanical-classic 2364.92 -> 2267.88, diesel-marine-v12
2361.48 -> 1890.04 rpm; the other 45 bit-identical.

In item 19's push (the lead, 12:55): GearsTest's X-057 case loses its [!shouldfail] mark (it reads what the ECU holds,
and item 19 turns it green); D2's full form (tests/sim/diesel/RedlineTest.cpp:130, which reads the files and stays
failing for the three since the files still state nothing) is restated on the ECU's value or dropped in favour of
GearsTest's; D2's gating form (the builds that shift by their redline) stays.

**Item 19 left the window (the lead, 13:05): X-059.** The steward's cap reviewed on a scratch tree (TEE_CORE_SIM, one
tee-tests-core binary, hidden "[.item19readers]"; 8d2e60f0 + 75d67996's diff by hand): it moves a reader its null did
not see. boost.applyMachine sizes the machine from ecu.redRpm (machine.fm:464, designShare x redRpm), where a governed
diesel's design speed is its rated speed (as designSpeed takes it at engine.fm:526). X-059 is N2's, with D2 for what
the three should make at low speed, from sources; the rule and the match land together next window. GearsTest's
X-057 case keeps its [!shouldfail]; D2's RedlineTest stays. The figures N2 starts from (Bench; before -> after):

| build | engine.redline (rad/s) | boost.mach.redRpm | dyno full pedal 1200 rpm | 1600 rpm | 1900 rpm |
|---|---|---|---|---|---|
| diesel-v8-truck | 245.02 -> 210.94 | 2343.33 -> 2014.33 | 2751.79 -> 3076.95 N m (+11.8 %), boost 108299 -> 134970 Pa | 3185.75 -> 3171.74, 139997 both | 3074.51 -> 3074.12, 139997 both |
| diesel-mechanical-classic | 247.65 -> 237.49 | 2364.92 -> 2267.88 | 2016.16 -> 2056.33 (+2.0 %), 82341 -> 86817 Pa | 2160.43 -> 2166.41, 128116 -> 131532 | 2160.33 -> 2154.65, 139850 -> 139977 |
| diesel-marine-v12 | 247.29 -> 197.92 | 2361.48 -> 1890.04 | 2677.17 -> 3208.46 (+19.8 %), 77295 -> 108574 Pa | 3334.74 -> 3538.54 (+6.1 %), 123334 -> 139997 | -448.82 -> -423.50 (over its high idle), 42301 -> 56723 |

Free rev at full pedal, 3-6 s after: v8-truck 1998.3-2009.4 -> 1998.0-2009.7 rpm; classic 2202.4-2360.8 ->
2203.2-2361.6 (the reference's cut does not hold a governed diesel at redRpm); marine 1826.9-2125.4 -> 1831.2-2111.4;
sound level within 0.35 dB. car.m1.redW read 6493.5 rpm on all three before and after in mode 0 (the vehicle's copy
not read in drive: not measured). truck-15l-six and diesel-pickup-tuned print identically before and after. The
steward's gate added since: a cap only with fuel kind != 0 and governor != 0 (all six governed builds state Electronic
or Mechanical).

Numbered for the list, not for now: X-058 (X's), archetypes/v6-35-twin-turbo-pickup, an automatic put into D at rest,
the "start" window, 1 of 17 over its bound (3.57 against 3.22), 45 ms after the lever and before first is in.
