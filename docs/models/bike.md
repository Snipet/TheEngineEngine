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
from just over idle: a rider who never stalls). Once it has locked in a gear the helper is held closed by the gearbox
output's speed through the gear until a shift, neutral, the lever pulled (engagement under 0.5) or that speed near the
idle (the stall band's foot): the integrator's law for the car's robotised clutch, the lead's ruling of 25 Nov. The
output's speed rather than the road's, so a locked rear lets the clutch go before the engine stalls. `bike.lugging`
(the cruiser floored at 1,300 rpm in sixth): slip after the first lock 22.5 rad/s without the latch, 0.00 with it. A wet clutch's oil drag when open: 0.6 % of capacity (judgement).
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

**For the mechanical voices.** `Car::lashStrike()` (above); `Car::sprocketSpeed()` and `tel.car.sprocketSpeed`, the
front sprocket (the gearbox's output), rad/s: the chain's teeth pass at it times `frontSprocket` over 2 pi;
`clutchDry` (off: wet, its oil drags 0.6 %; on: dry, no drag).

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


## The breathing each engine needs (card E's lab, on in every bike build)

Every bike engine and bike build carries card E's hardware (`valve.model` Lab, `intake.model` Lab with a throttle body
a runner, trumpets in the air box) and states its air (`env.temperature` 300 K, `env.pressure` 101,325 Pa: the
reference's presets stand at 233 K, 28 % denser). The heads and cams are judgement within the class's ranges (a sport
four's intake valves 0.38 to 0.40 of its bore, 250 to 270 degrees at 1 mm, 9 to 11 mm of lift on finger followers; a
pushrod cruiser twin's 220 degrees and 12 mm). Intake valve area over bore area, and the runner's tuned speed by the
quarter-wave rule taken at the third harmonic (rpm = 120 c / (4 L) / 3, c = 343 m/s): judgement, as is the rule.

| Engine | Valves | Intake valve area / bore area | Duration at 1 mm | Lift, mm | Runner, m | Tuned near, rpm | Redline |
|---|---|---|---|---|---|---|---|
| bike-boxer-twin | 4 | 0.30 | 244 | 10.5 | 0.34 | 10,088 | 9,000 |
| bike-i4-600 | 4 | 0.32 | 266 | 9.3 | 0.23 | 14,913 | 16,000 |
| bike-i4-crossplane | 4 | 0.31 | 262 | 10.5 | 0.26 | 13,192 | 14,000 |
| bike-i4-screamer | 4 | 0.33 | 262 | 10.5 | 0.26 | 13,192 | 14,500 |
| bike-single | 4 | 0.29 | 244 | 9.5 | 0.32 | 10,719 | 10,500 |
| bike-triple | 4 | 0.32 | 256 | 10.0 | 0.27 | 12,704 | 13,000 |
| bike-twin-180 | 4 | 0.30 | 240 | 9.5 | 0.32 | 10,719 | 10,500 |
| bike-twin-270 | 4 | 0.31 | 236 | 9.0 | 0.34 | 10,088 | 10,000 |
| bike-twin-360 | 2 | 0.25 | 228 | 9.0 | 0.36 | 9,528 | 8,000 |
| bike-v4-bigbang | 4 | 0.35 | 262 | 11.0 | 0.26 | 13,192 | 13,000 |
| bike-v4-screamer | 4 | 0.35 | 264 | 11.0 | 0.25 | 13,720 | 14,500 |
| bike-vtwin-45 | 4 | 0.31 | 220 | 12.0 | 0.42 | 8,167 | 5,500 |
| bike-vtwin-90 | 4 | 0.35 | 250 | 11.5 | 0.32 | 10,719 | 11,000 |

**What they make today with the lab on** (tee-dyno, steady points; `tee-validate`'s sweep reads a little higher):
superbike 77 kW at 12,400 rpm (tee-validate 102 kW), 600 63 kW at 15,300 (82), naked twin 45 kW, cruiser 61 kW and
148 N m (74, 160: within its class), adventure boxer 77 kW, single 22 kW. `content/validation/bike-superbike.json`,
`bike-supersport-600.json`, `bike-cruiser.json`, `bike-vtwin-90.json` hold the class's peak power and torque as targets
reported, not failed (their fast targets, the start, the level and the limiter, pass in ctest). The lead's diagnosis:
at 25 m/s of piston speed the loss is mostly after the breathing (friction, perhaps the burn; the integrator is
splitting it), and the runners do not ram yet (card E). When either lands, `bike.class.*` should be re-run with the
real engine in place of the rig's curves.

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
