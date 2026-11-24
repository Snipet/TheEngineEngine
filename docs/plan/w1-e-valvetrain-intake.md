# Card E: the valvetrain lab and the induction

*Wave 1. Repository: TheEngineEngine, integration branch `claude/awesome-gates-ntdb58`. Read
[01-rules.md](01-rules.md), [02-contracts.md](02-contracts.md), `sim/README.md` (K3's: `Valvetrain`, `Intake`,
`Cylinder`), then the reference: `sim/ref/cylinder.fm` (`lift`, `flowArea`, `areaAt`, the table `kCamArea` and
its comment), `valves()` in `sim/ref/engine.fm`, the runners and the plenum in `pipes()` and `sampleTick`, and
the intake's sound in `sim/ref/sounds.fm`.*

**OWNS**: `sim/valvetrain/**`, `sim/intake/**`, `sim/spec/valve.fm`, `sim/spec/intake.fm`, the `tel.valve` and
`tel.intake` sections and their Scope arrays, `tests/sim/valvetrain/**`, `tests/sim/intake/**`,
`content/builds/cams/**`, `docs/models/valvetrain.md`, `docs/models/intake.md`.

**Budget**: 0.05 R a sample. The cylinder's inner loop must stay a table lookup, as it is.

**Start standalone**: the cam generator, the area tables, the phaser, the stage logic and the float rule are all
functions of angle and speed that need no engine to test.

## What the reference does

One dial sets both cams' duration (224 to 300 degrees) and their separation; one tabulated curve gives every
valve's flow area over its event; areas are fixed shares of the piston's. A big cam's lope, the reversion and the
tuned lengths already come out of its gas model. That stays as `valve.model = 0` and `intake.model = 0`, the
defaults.

## The valvetrain

**A cam, as a person specifies one**, for intake and exhaust apart: duration at the seat, duration at 1 mm
(the two together set how gentle the ramps are), lift at the valve, centreline, and a fatness (how long it
dwells near full lift). A generator makes the lift curve from them: the reference's shape (`sin^p` with ramps),
with `p` and the ramp's length as the knobs. Measure back from the generated curve what was asked for; they must
agree.

**From lift to area.** Valves a cylinder (2 to 5) and their diameters (0: chosen from the bore, as real heads
are); the curtain `pi d L` while the lift is low, the throat past about a quarter of the diameter, with the
discharge coefficient's fall: the reference's `flowArea` already has the measured shape, so keep its form and
give it the geometry. A port's quality is one number. **The 129-point area table of each profile is rebuilt at
the 16-sample rate, never per sample**, one table a tick at most, when the Spec or a slow state moves.

**Phasers** (variable timing): each cam's centreline moves by a target from the ECU (card H's maps; a simple
speed-and-load schedule of your own when the ECU has none), at a slew rate that depends on oil pressure and
temperature (from `Heat`), within its range, parked when the oil is cold. Only the events' angles move: no table
is rebuilt.

**Two stages** (cam switching): a second profile for either cam; the change above a speed with hysteresis, and
only with oil pressure and a warm engine. Each cylinder takes the new profile while its valve is shut, so the
change needs no crossfade and takes one cycle to go round the engine, as a real one does.

**Continuous lift**: lift and duration scale together with a command; with `liftThrottle` on, the ECU leaves the
plate open and this is the load control. Blend between a few prebuilt tables.

**Deactivation**: a set of cylinders whose valves stay shut (exhaust first, so each keeps a gas spring) and whose
fuel stops (a function `Combustion` asks: is cylinder c active?), entered and left at the right phase, under
light load in a speed window.

**Free valves**: opening and closing angles and lift given outright, with fast flanks; late and early intake
closing as presets of it; and a second exhaust event in a cycle, which wave 2's compression brake will use: the
seam to the cylinder must allow an auxiliary event beside the main one.

**Float.** From the spring's rate and preload, the valvetrain's effective mass by kind (pushrod, rocker, bucket,
finger; pneumatic never floats) and the cam's own deceleration over the nose: the speed past which the valve
leaves the cam. Beyond it the valve lands late and bounces (a little area for some degrees after it should be
shut), the cylinder loses charge, the power falls over; a clatter event for the sound. No damage.

**The flow bench**: the head's flow at 28 inches of water against lift, computed from the geometry when the Spec
changes, in telemetry: the number tuners quote.

## The induction

- **Throttle kind**: one body and a plenum (the reference); or one plate a runner (ITB) with trumpets open to the
  air box or the air, each runner's far end a radiating open end (the reference's own end-reflection filter),
  idle air through a small common rail. The plate's area law and the body's size are the reference's, with the
  size a field (0: from the displacement, as now).
- **Runners**: length and bore as fields (0: as now); a second length and a speed at which it changes (glide the
  duct's length: it is built to change smoothly). The peaks this moves are the waveguide's own.
- **Plenum** volume (0: as now).
- **The air box**: a volume and a snorkel (a mass of air in a neck on a spring of air) in place of the
  reference's resonant filter on the throttle's flow; what the snorkel lets out is its sound.
- **Upstream** is card F's last volume (its pressure and temperature), or the air outside.
- **Sound sources**: the snorkel's flow as it changes; with ITBs, what each trumpet radiates (much louder, with
  the runners' own resonances in it); the plate's hiss as now.

## Telemetry and Scope

Each cam's phase asked and reached, the stage, the lift command, the active cylinders, a float flag; the four
valve events' angles as they are now, and the overlap at 1 mm; the flow bench's curve; the manifold's pressure
and its ripple. Scope: both valves' lift by degree for cylinder 1 (K3 leaves the arrays; you fill them from the
real profile).

## Tests

Standalone:
1. The generator: duration at the seat and at 1 mm, lift and centreline measured from the curve match what was
   asked within 1 degree and 1 %, for a mild, a hot and a race cam; no negative lift; acceleration continuous.
2. Area against lift: rises with the curtain, flattens near a quarter of the diameter; four valves beat two at
   the same bore.
3. The phaser reaches a step target at its slew rate, slower cold, and never leaves its range.
4. The stage change: every cylinder switches with its valve shut; the events' angles never jump mid-lift.
5. Float: the speed at which it begins rises with the spring and falls with mass and with a sharper cam.
6. The air box's resonance where its volume and neck say.

Integrated (K3's `EnginePlayer`; `tee-dyno` when card C has it):
7. `model = 0`: the null test.
8. Advancing the intake cam raises torque low down and costs it at the top; a later intake closing moves the
   torque peak up. The directions, and that the sizes are plausible (a few percent to twenty), on the default V8
   and a four.
9. Two stages: the two cams' torque curves cross, and the change set at the crossing shows no dip; the step in
   the output's samples at the change is no larger than between any two firings.
10. Deactivation on a V8: half the firing lines in the output's spectrum; the manifold pressure rises for the same
    torque.
11. ITBs: the radiated intake level at full throttle well above the plenum's (state the dB); the idle's manifold
    ripple larger.
12. A longer runner moves the torque peak down by about what the acoustics predict (within 20 %).
13. Past the float speed the power falls; nothing goes non-finite at twice it.

## Not in this card

The ECU's maps for the phasers and the lift (card H; you read targets). The compression brake itself (wave 2
uses your auxiliary event). Carburettors (later). Two-stroke ports (wave 2, its own cylinder).
