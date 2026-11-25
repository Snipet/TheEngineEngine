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

**Open for the next window (25 Nov, core :680, first.json's two routes)**: 8c0f746a reads the routes settled (5 to 6 s)
within 2.5 %, against a measured spread of 2.4 % across fourteen inits (each route's cylinders draw their own bias), with
every model switch held exactly across the routes. The margin is thin: a later bake may go red with no fault (the lead's
point). The sharper form, the same bias draws on both routes and the paired difference bounded, does not yet pair them:
with the same bias the pairs differ 0.06 to 1.6 % at 5 to 6 s; with the same bias and the same states of both engine
noise streams (noise.rs, ex.noise.rs) still 1.37 / 0.09 / -1.46 %. Some other state differs between "written after
init" and "initialised over it" and the run diverges from it. Next: diff every field right after start between the two
routes, pair them on what differs; if they then agree to well under 1 % bound the paired difference by its measure; if
something real differs (a field one route sets and the other does not), it is a finding in the init order: to the lead
before the bound moves. Changes after this note are docs only; not pushed while the branch is held.
