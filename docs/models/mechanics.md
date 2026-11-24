# The mechanics

*Card D ([w1-d-mechanics-layouts.md](../plan/w1-d-mechanics-layouts.md)). What `sim/core/` models of the engine's
moving parts, where each number came from, and what is judgement. Standalone so far: the modules are built and
tested on their own (`tests/sim/core/`, `tee-tests-core`) and join the engine once the split (K3b) has landed.*

With `engine.custom` off (the default) none of this is used and the engine is the reference's.

## The modules

| File | Module | What it is |
|---|---|---|
| `sim/spec/engine.fm` | `EngineSpec` | card D's fields, appended to `spec.engine` after the reference's dials |
| `sim/core/layout.fm` | `LayoutGeom` | banks, pins, firing revolutions, places → firing angles, order, intervals, a sanity flag |
| `sim/core/kinematics.fm` | `SliderCrank`, `Sizing` | the piston's x, x', x'' and the volume; size and bore/stroke dials ↔ bore and stroke; the redline by piston speed |
| `sim/core/inertia.fm` | `MovingMasses` | the reciprocating masses' inertia torque, the varying inertia J(θ), the crank's energy-exact step |
| `sim/core/shake.fm` | `ShakeByOrder` | shaking forces and rocking moments, first and second order; balance shafts |
| `sim/core/friction.fm` | `FrictionModel` | friction by part, Chen and Flynn's form, as FMEP and torque |
| `sim/core/work.fm` | `CycleWork` | ∮p dV per cylinder per cycle, split into power and pumping loops; IMEP, PMEP, BMEP; VE two ways; peak pressure |
| `sim/core/dyno.fm` | `Dyno` | the engine dyno's absorber: holds or sweeps the crank's speed |

## Layouts as geometry

A cylinder is at top dead centre when the crank is at its bank's angle less its pin's angle; it fires on the
revolution `fireRev` says. Firing angles relative to cylinder 1, the order and the intervals follow. A rotary's rotor
fires its three faces a shaft turn apart. The 33 presets in `content/builds/engines/` are written as geometry by
`tests/sim/core/make_presets.py`, which refuses any whose geometry does not give the firing order and intervals the
engine is known for; each file carries them under `"expect"` and `LayoutTest` checks FMinus's derivation against them.
The reference's seventeen reproduce `sim/ref/layouts.fm`'s firing angles exactly.

Sources of the expected orders: the engines' published firing orders and crank layouts (a crossplane V8's
1-8-4-3-6-5-7-2 with each bank 180-90-180-270, a 90° V6's 90-150 with common pins, a 45° V-twin's 315-405, a T-plane
triple's 180-270-270, and so on). Judgement: the V4s' and W engines' exact pin phasing (any of the published variants
fires with the same intervals up to rotation; the W12's and W16's pins are derived from their published orders), the
places along the crank (a 100 mm pitch, a V's second cylinder a third of a pitch along).

## Kinematics

Exact slider-crank: x = r(1 − cos θ + λ − √(λ² − sin²θ)), with its first and second derivatives in closed form. The
volume is the reference's `Cylinder::volumeSlope` formula. `KinematicsTest` checks x, x', x'' against a double-precision
closed form and finite differences for λ from 2.8 to 4.5 (worst errors under 1e-6 of r).

Sizing: bore = ∛(4 v_d k / π), stroke = bore / k, as the reference's `boreStroke`. Redline when none is given: a mean
piston speed of 25 m/s for gasoline (production engines reach some 21 to 26 m/s at their redlines; judgement within
that range), the reference's 13.5 m/s for a diesel, at most 20 000 rpm.

## Moving masses

J(θ) = J₀ + Σ m x'², and Lagrange gives each cylinder's inertia torque −m ω² x' x''. Reciprocating mass m = piston,
pin and rings plus a share of the rod (a third by default: the usual two-mass split); the rest of the rod turns with
the pin and is in J₀. Default masses when none is given (judgement from typical published weights): piston, pin and
rings 700 kg/m³ × bore³ (0.45 kg at 86 mm, 0.73 kg at 101.6 mm); rod 0.6 kg × (bore / 100 mm)² × (rod / 155 mm).

**Stepping.** The crank's speed is stepped so its kinetic energy ½J(θ)ω² changes by exactly the outside torque's work
(`MovingMasses::advance`). An explicit step of dω/dt with a varying J lost energy (a free-running single: −15 % in
20 s; a light-flywheel V8: −0.3 %); the energy step holds both to 2e-5 (`InertiaTest`'s soak). Below 2 rad/s it falls
back to the plain step, where the inertia torque is nothing.

Measured (`InertiaTest`): the inertia torque's work over a cycle at constant speed is under 1e-4 of its gross; one
cylinder's waveform matches the closed form to 1e-6 of its peak; it scales with ω² exactly. A single at 6000 rpm
(0.65 kg, 45 mm crank): 346 N m peak.

## Shaking

Each cylinder's first- and second-order force (m r ω² cos t and m r ω² cos 2t / λ) along its bank's axis, summed as
two rotating vectors per order; the amplitude is the largest radius of the ellipse they trace. Moments the same,
weighted by each cylinder's place about the crank's middle. Balance shafts cancel an order's force (not its moment).
Left out: the rotating masses (the counterweights are assumed to balance them), orders above the second.

Measured (`ShakeTest`, 0.67 kg, 45 mm, λ 3.33, 6000 rpm): flat-plane four, no first-order force, second-order 14.2 kN;
straight six, nothing of either order; boxer four, no force, a second-order moment of 710 N m; crossplane four and
crossplane V8, no second-order force, a first-order moment; flat-plane V8, a second-order force (20.1 kN). These agree
with the textbook balance of each layout.

## Friction

Chen and Flynn (SAE 650733, 1965): FMEP = A + B p_max + C S_p + D S_p², split by part: rings and skirts (the p_max,
S_p and S_p² terms), bearings (by speed), valvetrain (by kind), a constant, and the accessories (alternator and air
conditioner, as power). The oil-thickness factor multiplies all but the accessories, as the reference's one curve.

The coefficients are **judgement**: fitted so that the reference's V8 at 60 bar peak follows the reference's own
friction curve (1 + 0.10 N + 0.02 N² bar) to within 30 %: 0.95 bar at 700 rpm, 1.12 at 2000, 1.58 at 4000, 2.35 at
6500 (the reference's 1.08, 1.28, 1.72, 2.50). The split between parts follows the usual shares of motored friction;
roller followers at half a flat tappet's; alternator efficiency 0.6; an air conditioner 2.5 kW. None of these was
measured here.

## Work

The trapezoid of p dV over each sample's volume change, the sample's work given to the loop its middle angle lies in
(power: 540° to 180° through the firing top dead centre; pumping: 180° to 540°). `WorkFrictionTest` runs an ideal Otto
cycle and gets its closed-form IMEP and PMEP to 0.2 % (4.4234 bar and 0.5000 bar both). BMEP = 4π T / V (a rotary: two
chambers a rotor). VE at the manifold's density and at the air's, from the mass trapped at intake closing.

## The dyno

A PI loop on the crank's own dynamics (natural frequency 15 rad/s, critically damped, the set speed's acceleration fed
forward): it holds the cycle's mean speed and lets the firing ripple through, as a real dyno does. Against a made-up
engine (330 to 520 N m, ±90 % ripple at eight pulses a cycle) on a V8's crank: holds 3000, 5500 and 1500 rpm to under
0.01 rpm of the mean; the absorber's mean torque equals the engine's less friction to 0.1 %; a 300 rpm/s sweep reaches
its end within a rpm. Left out: the absorber's own inertia and limits.

## Cost

Eight cylinders' inertia torque, the crank's step and the work integral: 69 ns a 96 kHz sample (0.036 R, R = 1942 ns,
the reference V8's on this machine); sixteen: 117 ns. Budget: 0.06 R.
