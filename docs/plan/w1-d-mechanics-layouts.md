# Card D: mechanics, layouts, and the integrator

*Wave 1. Repository: TheEngineEngine, integration branch `claude/awesome-gates-ntdb58`. Read
[01-rules.md](01-rules.md), [02-contracts.md](02-contracts.md), `sim/README.md` (K3's, all of it: you keep it
true for the whole simulator from now on), then the reference: `sim/ref/layouts.fm`, `sim/ref/cylinder.fm`
(`volumeAt`, `volumeSlope`), and in `sim/ref/engine.fm` `configure()`, `boreStroke()`, the crank's part of
`sampleTick` (friction, the starter, the load), `heat16()`.*

**OWNS**: `sim/core/**`, `sim/engine.fm` (with the lead), `sim/spec/engine.fm`, `sim/spec/env.fm` (with the
lead), the `tel.engine` section, `tests/sim/core/**`, `content/builds/engines/**`, `docs/models/mechanics.md`,
and `sim/README.md`.

**Budget**: 0.06 R a sample for eight cylinders (the inertia torque and the work integral); the cost a cylinder
of everything else as the reference.

## Two jobs

**The integrator.** Other packages will ask for a call in the order of work, a value from another module, a
change under the fade. You add each as a hook with a default that does nothing, keep `Engine::sampleTick` and
`control16` readable, keep `sim/README.md` exact, and run the whole suite before every push to `sim/engine.fm`.
When two packages disagree about a seam, you propose and the lead settles.

**The mechanics**, below. With `engine.custom` off (the default) all of it is the reference.

## Layouts as data

- `cylinders` (1 to 16). For each: its bank (0 to 3), its crank pin's angle, which of the two revolutions it
  fires on, its place along the crank (for the rocking moments). For each bank its angle. From these the firing
  angles are **derived** when the Spec is applied (a cylinder's top dead centre comes when the crank is at its
  bank's angle less its pin's; the revolution bit picks the firing one) and written to telemetry, so the host
  can show the firing order and refuse a build whose order makes no sense.
- A rotary: 1 to 4 rotors, three faces each, with its port timing as fields (peripheral, side, bridge as
  presets of them).
- **Presets** (`content/builds/engines/`): the reference's seventeen, exactly; and V4s (90 and 65 degrees),
  a VR6, V6s at 60 degrees and at 90 with common pins (odd-fire) and with split pins, a straight eight, V10s at
  72 and 90 degrees, a V16, W12 and W16, triples with a 120-degree and a T-plane crank, parallel twins at 360,
  180 and 270, V-twins at 45, 60 and 90, three- and four-rotor rotaries. Each preset's firing intervals are a
  test (below).

## Geometry and sizing

Bore, stroke, rod length, compression, as fields when `custom` is on (the reference's size and bore-over-stroke
dials are the other view of the same thing; keep both consistent). What the reference derives from the engine's
size stays derived unless a section overrides it with a non-zero field: the pipes' bores (card G), the throttle
and the plenum (card E), the inertia, the idle and the redline. The redline as a field (0: by the layout, or by
piston speed for a custom engine: state the limits you use and why).

## Moving masses

- Piston, pin and rings; the rod, a third of it moving with the piston; the flywheel and crank as an inertia
  (the reference's flywheel dial, now also in kg m^2).
- **Inertia torque** on the crank every sample, for each cylinder, from the slider-crank's geometry (the
  cylinder already has the sine, the cosine and the root: `dx/dtheta` is its volume slope over the piston's
  area; add the second derivative): the reciprocating mass's `-m w^2 x' x''`, and its share of the inertia the
  crank feels, which varies round the turn. At high revs this is larger than the gas's torque and reshapes the
  crank's ripple.
- **Shaking forces and moments** by order (first and second), summed over the cylinders along their banks' axes
  and their places on the crank; balance shafts as options that cancel an order. These are signals for wave 2
  (the cabin, the picture, the wheel's rim) and numbers in telemetry now.

## Friction and the bench

- Friction by part instead of one curve: rings and skirts (piston speed, and peak pressure), bearings (speed),
  the valvetrain (by its kind, card E's field), accessories (the pumps, an alternator's load, an air conditioner
  as a switch). The reference's oil-thickness factor stays. Chen and Flynn's form is the model; say what each
  coefficient is and where it came from.
- **The engine dyno** (`MODE_DYNO_ENGINE`): an absorber on the crank holds `ctl.dynoSpeed`, or sweeps it at
  `ctl.dynoRate`, by a loop that knows the engine's inertia; the vehicle is out of the picture. The run itself
  (where to start and stop, the points) is the host's (card C).
- **Work, per cylinder per cycle**: the integral of `p dV` over the cycle, split into the power loop and the
  pumping loop: indicated, pumping, friction and brake mean effective pressures; volumetric efficiency from the
  mass trapped at intake closing over what the displacement would hold at the manifold's or the air's density
  (both, labelled); peak pressure and its angle; with card H's fuel flow, specific fuel consumption.

## Telemetry

Torque now and its mean over a cycle, power, the mean effective pressures, volumetric efficiency, mean piston
speed, the firing angles as derived, the shaking by order, each cylinder's peak pressure and angle, the crank's
speed ripple.

## Tests

1. `custom` off: the null test.
2. Volume against angle against the closed form, for rod ratios from 2.8 to 4.5.
3. **Firing intervals from geometry**, a table of known engines: a flat-plane four 180-180-180-180; a crossplane
   V8 (1-8-4-3-6-5-7-2) 90 even, each bank 180-90-180-270; a 90-degree V6 with common pins 90-150 alternating,
   with 30-degree split pins 120 even; a 45-degree V-twin 315-405; a 270-degree twin 270-450; a T-plane triple
   180-270-270; a 72-degree V10 72 even. Every preset's intervals as its file says.
4. Inertia torque: its work over a cycle at constant speed is zero; its waveform for one cylinder matches the
   closed form; it grows with the square of the speed.
5. Shaking: a flat-plane four has no first-order force and a second-order one; a straight six neither; a boxer
   four no force and a rocking moment; a crossplane four a rocking moment and no second-order force. Balance
   shafts cancel what they claim.
6. The dyno holds a set speed within 10 rpm against full throttle and sweeps at its rate; the brake torque it
   reports equals the absorber's.
7. The default V8 at full throttle: brake mean effective pressure between 10 and 13 bar across its mid-range,
   friction between 0.8 and 2.5 bar from idle to the redline (state the sources); volumetric efficiency between
   0.75 and 1.05.
8. A sixteen-cylinder build starts, runs and stops; its cost, reported.
9. The whole suite, after every change to `sim/engine.fm`.

## Not in this card

Valves, pipes, turbos, fuel, the ECU, the car: their cards. Hearing the shake (wave 2). Two-stroke cylinders
(wave 2; leave room: the cylinder's cycle length is already a field).
