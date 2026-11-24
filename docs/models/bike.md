# The motorcycle (card B2)

`sim/vehicle/bike.fm` (`Bike`, `BikeBox`), `bspec.fm` (`BikeSpec`, `BikeTel`), `chain.fm` (`BikeChain`). Chosen by
`spec.car.kind` = Motorcycle with `spec.car.model` = 1; `Car` (`vehicle/car.fm`) hands `drive16`, `step` and `road` to
its `Bike b1` as it does to card I's `Vehicle m1`. With any other kind, or model 0, nothing here runs (the null test).
Tests: `tee-tests-bike` (`ctest -R '^bike\.'`).

## What it models

**Bodies.** The crank (the engine's); the gearbox's output at the rear wheel's side of the chain (0.05 kg m^2 of
sprocket, chain and cush-drive carrier, plus 0.004 kg m^2 of basket, plates and shafts at the crank's side through the
ratio: judgement); the rear wheel (driven); the front wheel (free); the bike and rider as one mass.

**Clutch.** A coupling (card I's `vehCouple` law: the torque that brings the two sides to one speed within the sample,
clamped to the capacity) through the overall ratio `primaryRatio x ratio[g] x rearSprocket / frontSprocket`, at
`gearEfficiency x chainEfficiency` whichever way the power goes. Capacity `clutchTorque` (0: 1.35 x the engine's peak)
x the lever's engagement (bites a third of the way out, in at three quarters) x the launch helper (closes with the revs
from just over idle: a rider who never stalls). A wet clutch's oil drag when open: 0.6 % of capacity (judgement).
**Slipper**: backwards (the wheel driving the engine) it passes at most `slipperTorque` x capacity (default 0.15:
about a litre four's closed-throttle drag at high revs, so it trims engine braking a little and stops a downshift's
spin-up from reaching the tyre).

**Chain.** A spring with play between the output and the wheel: `chainLash` (default 0.035 rad, about 2 degrees at the
wheel: the dogs, the chain's slack, the cush drive's first give; judgement, where a car's shafts have half a degree),
9 N m/rad per kg of bike and rider (card I's rule for a car's shafts: a sport bike's shunt in first at 4 to 5 Hz),
damped to 0.4 of critical for the gear in. Only pushes. **Each closing of the play is an event**: `tel.car.chainLash`
counts them, `chainLashSpeed` (rad/s at the wheel) is how hard, `chainLashSide` which end (Drive: the throttle opened;
Overrun: it shut); `Car::lashStrike()` is that speed for the one sample (0 otherwise), for the mechanical voices; the
vehicle's own sound strikes its clunk (card I's thud and ring) with it.

**Tyres.** Card I's `VehTyre` (deflection, Magic Formula, load sensitivity, heat). `bikeTyre` sets the peak grip on
dry asphalt (touring 1.10, sport 1.25, hypersport 1.40, race slick 1.55, dual sport 1.00, cruiser 1.05: judgement, in
the 1.0 to 1.5+ range Cossalter, *Motorcycle Dynamics*, 2nd ed., ch. 2, gives) and the curve (card I's motorcycle
compound; a slick's for race slicks).

**Loads and the wheelie.** Centre of mass from the bike's (`cgHeight`, `frontWeight`) and the rider's (`riderMass`,
`riderHeight`, `riderForward`; a full `tuck` lowers them 0.12 m and moves them 0.06 of the wheelbase forward). Both
wheels down: the load moves rearward by m a h / L, lagged 30 ms (a sport bike's chain pull holds the rear up, so the
load arrives nearly at once; with card I's 0.2 s a dumped clutch spun the rear before the load reached it). Under
braking it moves forward; the rear's load cannot go below zero (a stoppie: flagged, not modelled further). When the
moment m a y exceeds the weight's m g x about the rear contact, the front lifts and the bike pitches about the rear
contact: (I + m (x^2 + y^2)) theta'' = m a y - m g x, I = `pitchInertia` (default 50 kg m^2 about the centre of mass:
judgement), x = b cos(theta) - h sin(theta), y = b sin(theta) + h cos(theta). It lands (its rate lost) or passes the
balance point (x = 0: looped, held at 80 degrees and flagged). A lifted bike still accelerates at what its rear tyre
gives. The wheelie's limit `tel.car.wheelieLimit` = g b / h.

**Wheelie control** (`wheelieControl`): *By the throttle*: the throttle the engine is given is eased on the lift beyond
`wheelieAngle` and its rate (u = theta - target + 0.25 theta'; share 1 - 6u; back up at 2.5 a second), and when the
rear spins past 1.3 x its peak slip (a rider rolls off; so does a traction control). *By the clutch*: the clutch's
capacity is held to the push whose pitch acceleration returns the angle to the target (-25 (theta - target) - 10 theta')
and to 0.97 of the rear's grip. A slipping clutch passes its capacity, not the engine's torque, so only the clutch's
way holds a launch while the lever is still coming out. *Off*: nothing; it can loop.

**Gearbox.** Sequential, a press at a time (one during a shift waits): up from neutral is first, down from first is
neutral. *Quickshifter* (throttle above 0.1): the ignition cut for `quickshiftCut` through the ECU's cut
(`Car::shiftCut`, `cutRequest()`), the dogs out at once and the next in at 40 % of the cut, the clutch closed. Without
it: the rider's clutch (open by 35 ms, gear at 50 ms, out by 160 ms) and throttle shut for 60 ms. *Auto-blipper*: the
throttle the engine is given raised to the lower gear's speed (`blipRequest()` too), the dogs in when the engine is
within 2 % of it (15 ms at the soonest, 100 ms at the latest). Without it: the rider's clutch as above, no blip, let out
over 100 ms: the clumsy downshift. A downshift past 97 % of the redline is refused. `autoGearing` gives a sport bike's
six (2.62, 2.00, 1.68, 1.45, 1.30, 1.18).

**Brakes.** The lever (`ctl.brake`) on the front, `brakeDecel` g at a full pull; the pedal (`ctl.handbrake`) on the
rear, (1 - `brakeBias`) of that. ABS as card I's.

**Not modelled.** Steering and lean; the suspension's travel (only its lag); squat from the chain's pull; a wheelie's
gyroscopic effects; the rider falling off; a shaft drive's jacking (the adventure bike is a chain's numbers).

## The engines

Thirteen custom layouts (`content/builds/engines/bike-*.json`), written by `tests/sim/bike/make_builds.py` from their
firing angles (pins and firing revolutions solved from fire = ((bank - pin) mod 360) + 360 fireRev, then checked
by the same arithmetic): screamer and crossplane litre fours, a 600 four to 16,000 rpm, 90 and 45 degree V-twins,
parallel twins at 180, 270 and 360, a 120-degree triple, a 90-degree V4 screamer (pins 45/135: even) and big bang (pins
offset 70: 0-90-290-380), a 360 boxer, a single. Each takes a bike base layout (its short pipes, idle and light crank)
and its bore, stroke, compression and `redlineSpeed`. Whole bikes (`content/builds/bike-*.json`): superbike,
supersport 600, naked twin, cruiser, adventure twin, small single, each with card E's individual throttle bodies
(intake model Lab, a body a runner) and a short exhaust by the reference's dials (card G's builder is not read by the
engine yet: a 4-2-1 waits for it).

**Limits.** At 96 kHz a 16,000 rpm crank turns 1.7 degrees a sample: the 600 runs to its limiter (16,028 rpm, its sound
finite and within range; `bike.engine.redline`). The custom engine's `redlineSpeed` goes to 2500 rad/s (23,900 rpm); 0
gives 25 m/s of piston speed (13,600 rpm for a 55 mm stroke), capped at 20,000.

**What the engine model gives** (tee-dyno, these builds; a real one of the class in brackets, round figures): superbike
72 kW at 11,500 rpm (140 to 150), 600 52 kW at 13,850 (88), naked twin 37 kW (55), cruiser 55 kW and 146 N m (70, 155),
adventure boxer 62 kW (100), single 19 kW (32). The reference's breathing falls away above some 9,000 rpm (with friction
taken out a litre four still tops out near 102 kW); the ITBs do not change that. So the bike model's validation against
its classes (`ClassTest.cpp`) gives the rig's engine each class's published power curve.

## Tests (`tee-tests-bike`)

Standalone, on `tests/sim/bike/rig.fm`'s made-up engine: `bike.launch` (superbike: lifted throughout, held at 3.1
degrees, mean 10.1 m/s^2 against a wheelie limit of 10.7, the rear short of its grip, front load 0), `bike.launch.cruiser`
(never lifts, traction-limited 98 % of the time, front load at least 799 N), `bike.quickshift` (cut 50.0 ms of 50; drive
gone in the cut; engine to the next gear's speed; no over-rev, no stall), `bike.autoblip` (engine 7854 -> 8920 rpm for
a ratio of 1.159; rear slip -0.016 against -0.100 unblipped), `bike.slipper` (braking into a corner, down a gear:
without a slipper rear slip -0.154 past the peak of 0.111, 529 blocks flagged hopping or locked; with it -0.052, none),
`bike.lash` (an event at each of four throttle changes, sides alternating), `bike.soak` (ten minutes random, every
control mode). Class (`[class]`), against published figures:

| Bike | Class figure (source) | Here |
|---|---|---|
| Litre superbike | 9.9 s, 145 mph (Cycle World, Superbikes 2012, GSX-R1000, 159.8 hp at the wheel); top 180 mph (judgement, between the S1000RR's 185 and the GSX-R's 178) | 9.47 s, 150.5 mph, 191.8 mph |
| 600 | 10.84 s, 127.8 mph (Wikipedia's list of acceleration tests, 2003 R6); top 161 mph (motostatz, slashgear) | 10.79 s, 128.6 mph, 159.7 mph |
| Big-twin tourer (114) | 13.67 s, 99.84 mph (Cycle World, H-D 131 crate engine review, the 114 touring bike); top 115 mph (judgement, 110-125 in secondary sources) | 14.33 s, 95.8 mph, 107.0 mph |

The engines' curves there are round versions of the makers' claims, their shapes judgement. Integrated (`[engine]`,
the real engine): `bike.engine.launch` (cruiser grip-limited, front down; the superbike's front unloaded to within 30 %
but not lifted: the engine above), `bike.engine.shift` (two quickshifts with the ECU's fuel cut on and off, an
auto-blip raising the engine 1.29x, no stall, no over-rev), `bike.engine.redline`, `bike.engine.firing` (all thirteen
engines, running, fire at their arithmetic's angles exactly), `bike.engine.lash`.

**Cost** (`tee-bench --engine`, the V8's engine driven through drive-car's timeline, repeat 5): the reference model
1.036 of R, the car model 1.064, the motorcycle 1.050: the bike's vehicle about 0.014 R (the car's about 0.028).
