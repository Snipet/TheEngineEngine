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

Used for a custom engine and whenever card H's combustion is on (`fuel.model` 1: the new combustion implies the
physical cylinder, its walls and its friction together); the reference's curve only under the reference's models. The
reference's curve goes as the piston speed over a square cylinder's of the same size, so its coefficient grows as
1 / (square stroke)^2: right for the V8 it was fitted on, 4.5 bar at 24.8 m/s on a 250 cc cylinder (this law: 3.1).
By bore at three piston speeds (`WorkFrictionTest`, 60 bar peak, warm): 60 mm 1.58 / 2.13 / 2.91 bar at 10 / 17 / 24
m/s, 86 mm 1.44 / 1.99 / 2.74, 104 mm 1.39 / 1.94 / 2.68.

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

A PI loop on the crank's own dynamics (natural frequency 40 rad/s, critically damped, the set speed's acceleration fed
forward): it holds the cycle's mean speed and lets the firing ripple through, as a real dyno does. Against a made-up
engine (330 to 520 N m, ±90 % ripple at eight pulses a cycle) on a V8's crank: holds 3000, 5500 and 1500 rpm to under
0.01 rpm of the mean; the absorber's mean torque equals the engine's less friction to 0.1 %; a 300 rpm/s sweep reaches
its end within a rpm. Left out: the absorber's own inertia and limits.

## Cost

Eight cylinders' inertia torque, the crank's step and the work integral: 69 ns a 96 kHz sample (0.036 R, R = 1942 ns,
the reference V8's on this machine); sixteen: 117 ns. Budget: 0.06 R.

## The coolant, the warm-up and the radiator's fan (sim/core/heat.fm)

The burns give the coolant some 40 % of their heat and the rubbing 60 % of its; the block and its coolant hold some
9 kJ/K a litre, as the size's 0.9th power; the thermostat opens from 363 K. All of these are **judgement**.

Measured from them (`EngineTest`, "a 2 L engine warms from 20 to 90 C in some 5 minutes at idle", [nightly]): the
2 L turbo four warms from 20 to 90 C in 289 s (4.8 min) at idle, the same with the reference's walls. The case's band,
3.5 to 8 minutes, is **judgement** about that measurement. heat.fm first said "some 8 minutes": that was never measured
and had no source, so 4.8 is not a regression. A published idle warm-up time would replace the band.

The radiator's fan (the lead's ruling, 24 Nov): Heat holds its physical state, an electric fan's thermoswitch
(`voices.fanOn`, off 5 K lower, only with the key on) or a viscous clutch's lock, and package M's FanVoice reads it.
Under the physical switches (engine.wallHeat 1 or fuel.model 1) the radiator has `still` 0.05 of a full fan's air
with its fan off, the fan or the road (15 m/s full, driven under car.model 1) the rest; otherwise the reference's full
air, to the bit. `still` is **judgement** from "[.fan]": the default idling warm in still air for 600 s cycles 367 to
373 K, 8 switchings, the fan on 2.1 % (0.02: 20 switchings; 0.1: never reaches its switch), the same to the bit with
voices on and off. Checked by "a warm idle in still air cycles the radiator's fan, and the voices leave it as it was".

## Wall heat (spec.engine.wallHeat)

0 is the reference's: one coefficient by speed for every cylinder, the same all cycle. 1 is Woschni's correlation
(SAE 670931; constants as Heywood, 1988, section 12.4), worked out for each cylinder every 16 samples from its own
pressure, temperature, volume and the mean piston speed, with the burn's term from the motored pressure since intake
closing (`sim/core/wallheat.fm`). It is meant to go with card H's whole burn (fuel.model 1 without the reference's
up-front 20 % cut); the two are switched on together once H's side lands.

Measured (`EnergyTest`, "[.wallheat]": the default V8, 4000 rpm, full throttle, warm, shares of the burn's heat):

| | work | walls | out | gas out | tPort |
|---|---|---|---|---|---|
| reference | 44.8 % | 13.3 % | 47.2 % | 1233 K | 1244 K |
| Woschni | 42.3 % | 17.0 % | 45.4 % | 1205 K | 1218 K |
| fuel.model 1 | 44.9 % | 13.2 % | 46.9 % | 1202 K | 1206 K |
| fuel.model 1 and Woschni | 42.4 % | 16.7 % | 45.3 % | 1183 K | 1185 K |

An engine's 25 to 30 % "to the coolant" includes the exhaust ports' walls and the friction's heat; here the ports'
and the pipes' heat is taken downstream (the exhaust's soak), so some 17 % in the cylinder is in the published range
for its own walls. A thermocouple's 900 to 1100 K reads after the port too.

### A rotary's faces

Under the same switches (the lead's yes of 16:09, 24 Nov), each face's walls are the chamber's own
(`WallHeat::setupRotary`, `coefficientRotary`): Woschni's form over the two side plates (each V / b) and the flank and
the housing's arc (each about sqrt(3) R b), with the gas's speed along them half an apex's (e w + R w / 3) in the mean
piston speed's place. R, e and b come from the face's swept volume in a 13B's proportions (R/e 7, b/R 0.76). With
`fuel.model` 1 the seals leak too (`Cylinder::leakA`): two apex seals and the side and corner seals as an orifice of
0.7 x 1.5 x 2 x 4 um x b to the air's pressure, the fuel in what leaks leaving with it. The 4 um is bounded, not
fitted: at 20 um the twin cannot start (a third of each charge gone at cranking), and a Wankel's leakage is known to
matter at low speed and little at high. A third term, the unburned trailing end (a burn completeness), is card H's
place (N1 acts for it).

Measured (`EngineExplore`, "[.rotarybmep]": the rotary twin archetype, engine dyno, full throttle, warm, every model
on; BMEP in bar):

| | 3000 | 4000 | 5000 | 6000 | 7000 rpm | leaked | port |
|---|---|---|---|---|---|---|---|
| unboosted, before the two terms | 7.98 | 9.26 | 11.12 | 9.89 | 9.25 | | |
| unboosted, with them | 7.23 | 8.42 | 10.22 | 9.21 | 8.64 | 4.0 to 1.7 % | 864 to 1062 K |
| unboosted, and the walls counted once (e81f79f) and the backflow conserved (e8c6575) | 5.53 | 6.10 | 8.50 | 7.85 | 7.47 | 4.3 to 1.8 % | 1076 to 1238 K |
| its turbos at 70 kPa, the same | 11.58 | 10.99 | 15.14 | 13.07 | 13.48 | 4.3 to 2.0 % | 1208 to 1345 K |

| card P's side-port twin (unboosted), at the tip of 24 Nov 18:00 (N1's 0.92 burn completeness, the intake's charge heating and valve mass, the builder's exhaust with boost.model 1) | 7.69 | 7.01 | 7.60 | 7.32 | 7.86 | 4.2 to 2.0 % | 1059 to 1197 K |
| the turbo twin, the same (its boost 47 to 70 kPa) | 8.47 | 10.06 | 11.35 | 10.78 | 11.17 | 5.0 to 2.0 % | 1196 to 1398 K |

At 18:00 both points are short: the side-port twin by some 25 % (VE 0.60 to 0.68), the turbo twin by some 20 to 28 % at
its boost. The walls and the seals take some 8 to 10 % together; what is short is the breathing (the ports' flow, the
intake), not these terms, so nothing is added here. Before 18:00:

The targets: about 14 bar at 0.7 to 0.8 bar of boost (the 13B-REW's 294 N m): met within 10 % at 5000 to 7000 rpm
(13.1 to 15.1). About 10 to 10.5 bar unboosted (a 1.3 L twin-rotor road engine's 210 to 216 N m): 8.5, some 17 % short.
That case is the archetype with its turbos taken off, not a naturally aspirated rotary's ports and intake, so part of the
gap is the build's, not the rotor's (judgement). The unburned trailing end (card H's, N1 acting) is still to come and
takes some 5 % more from both. The port gas runs hot, as a rotary's does.
