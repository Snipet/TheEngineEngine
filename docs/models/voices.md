# The mechanical voices (package M)

*`sim/sound/` (`Extras` and `Voices`), `sim/spec/voices.fm` (`spec.voices`), tests in `tests/sim/sound/`
(`tee-tests-sound`, ctest `sound.*`).*

`spec.voices.model` 0 (the default) is the reference's engine bay, bit for bit (the null test). With 1, `Extras`
runs `Voices`: eight voices, each driven by what the model does, none a recording or a loop. Each voice is its own
module with its own noise stream, so switching one never moves another's samples (the integrated tests rely on it:
a run with a voice less one without is that voice alone). Inputs come every 16 samples through
`Extras::control16` (called last in `Engine::control16`); events (valves landing, injectors) through the clicks.

## Shared machinery (`osc.fm`)

Every tone is the teeth or blades of a shaft: each shaft keeps a phase in turns; a mesh of z teeth is z times it,
wrapped, so its tones stay locked to the shaft and a speed change never clicks. Sines from a 1024-point table with
linear interpolation (error < 5e-6). Levels are set every 16 samples and ramp over the next 16 (`Glide`): no voice
steps on or off (test `clicks`). A level of -60 dB is off and costs nothing.

## The voices

| Voice | Module | Physics, numbers (source) | Driven by | Output (place) |
|---|---|---|---|---|
| Gear whine | `gears.fm` | Transmission error at the mesh frequency and two harmonics; amplitude k min(fm/1 kHz, 4)(0.12 + load) (Smith, *Gear Noise and Vibration*, 2003: profile error a few um, bending ~10 um at full load). Countershaft box: constant mesh 19 into 19 x 1.55; each pair's teeth sum to 48 (one centre distance); a direct gear (ratio within 2 % of 1) loads no mesh. Straight cut +12 dB and harmonics 0.6/0.35 against helical's 0.3/0.1 (Smith ch. 4; Houser: 8-15 dB). Eccentricity: 12 % AM by each shaft (sidebands). Reverse: straight-cut idler, 13 teeth. | gear, input speed, gearbox ratio, torque / peak, clutch | `vo.gearbox` (gearbox) |
| Final drive | `gears.fm` | Pinion `pinion` teeth or 41 / ratio (>= 7), ring ratio x pinion; helical-like harmonics; load = torque x gearbox ratio / 3 | propshaft speed | `vo.axle` (differential) |
| Neutral rattle | `gears.fm` | Three loose gears within their backlash (4, 5.5, 7 mrad; dog box x2), pulled to the layshaft by oil drag (20, 35, 50 /s), striking at +-b, restitution 0.3 (impact model after Singh, Xie and Comparin 1989); strikes ring the case at 1.9/3.3/5.2 kHz. Dog box 1, manual 0.25, dual clutch 0.5, automatic and CVT 0. | the crank's own speed each sample (its firing ripple) | `vo.gearbox` |
| Accessory belt | `drives.fm` | Alternator 2.8x crank, 6 pole pairs x 6 diodes = 36 a turn (100.8 per crank turn); water pump 1.2x, 6 vanes; steering pump 1.1x, 10 vanes; belt rib hiss near 3 kHz ~ speed^2 (Bosch Automotive Handbook) | crank | `vo.front` (front of engine) |
| Cam drive | `drives.fm` | Chain: the reference's 21-tooth whine, untouched. Toothed belt: 21 a turn, half the chain's level. Gears: 30-tooth straight-cut spur, AM once a cam turn, load 0.5. | crank | `vo.front` |
| Supercharger drive | `drives.fm` | Belt: hiss ~ speed^2 (0.3 + 0.7 boost share). Gears: 40-tooth spur, load = boost share. Only with `asp` 4. | crank, boost | `vo.front` |
| Cooling fan | `fan.fm` | Viscous: 1.1x crank, slip 25 % cold to 90 % locked over the 15 K below `fanOn`, fluid time constant 4 s. Electric: on at `fanOn`, off 5 K lower, 40 turns/s, up in 0.35 s, coasts in 1.2 s. Blade passing + harmonic; broadband ~ tip speed^2.75 in amplitude (sound power ~ U^5.5), band at 4x BPF; tip speed at 0.4 m. | coolant (K), crank, key | `vo.front` |
| Fuel pump | `fuel.fm` | In-tank turbine pump on a DC motor, 110 turns/s free less 15 under pressure (time constant 0.25 s); 41 vanes, 8 commutator segments, once a turn. Prime `prime` s (2) at key-on, then on while the crank turns (Bosch, *Gasoline-engine management*). | ignition, state, crank | `vo.tank` (tank, rear) |
| Direct injectors | `fuel.fm` | Gasoline DI only: a sharp tick at the injector's opening and 1.5 ms later its closing (4.5, 8.2 kHz rings, Q 20-25), and the cam's high-pressure pump (the reference's `pres`) at 0.5; port injection: no HP pump, the reference's soft click. A diesel keeps the reference's. | the cylinders' exhaust closing (the reference's injector event) | `vo.head` (head) |
| Lifters | `valvegear.fm` | Solid: the lash's tap at every valve, strength x lash / 0.25 mm, ringing 4.8 and 7.5 kHz; at intake closing a tap 240 crank degrees later (exhaust opening), at exhaust closing one 1.4x (intake opening merged). Hydraulic: nothing added. | valve events, crank degrees | `vo.head` |
| Valve float | `valvegear.fm` | Card E's `vt.clatter` (0-1): two bounces 0.6 and 1.3 ms after each landing at 0.6 and 0.3 x clatter (card E also makes the landing 1 + 2 clatter harder). | `vt.clatter` | the head's ring (Extras' `vres`) |
| Wastegate | `gate.fm` | Internal flap: knocked each firing by 4 g(1-g) x exhaust flow share, ringing 2.7/4.1 kHz (EA888's known rattle; judgement). External to the air: jet noise ~ gate flow, 1.2 kHz band + low part, pulsed with firings. | gate opening (reference: open as boost reaches its most), exhaust flow, firing rate | `vo.gateOut` (turbo) |
| Starter | `starter.fm` | Pinion 10 on ring 130 (13:1); direct drive armature 13x crank, reduction 3.6 more (47x); pinion mesh, commutator 20 segments, planetary sun 12; housing at 420 Hz (1.2 kW / P)^(1/3); level ~ sqrt(P); P by size 1.2 kW sqrt(V / 2 L), 0.8-3 kW, diesel x1.6. Solenoid knock (180 Hz, 1.1 kHz), pinion strike 3 ms later (2.6 kHz); run-down 0.25 s after the key. | ECU's starterOn, crank | `vo.starter`, replaces Extras' `starter` |
| Chain (motorcycle) | `bike.fm` | Roller chain's chordal action and roller impacts at front sprocket teeth x its turns, gear-mesh level law k min(f/1 kHz, 4)(0.15 + load), straight-cut harmonics, 12 % once a sprocket turn (Binder, *Roller chain drives*) | card B2's `Car::sprocketSpeed()`, `spec.car.frontSprocket`, torque | `vo.chain` (rear sprocket, swingarm) |
| Lash clunk (motorcycle) | `bike.fm` | The drive's play closing on and off the throttle: its closing speed / 3 rad/s rings 140 Hz (swingarm, frame), 650 Hz (sprocket), 2.1 kHz (links) | card B2's `Car::lashStrike()` each sample | `vo.chain` |
| Dry clutch rattle (motorcycle) | `bike.fm` | Two plates in the basket's slots (6, 9 mrad of play, drag 15, 25 /s) driven by the basket (crank / primary ratio), as the gearbox's rattle; quiet under load (gone by a third of the most), with the lever pulled, or wet | crank each sample, `spec.car.clutchDry`, `primaryRatio`, lever (1 - clutch) | `vo.clutch` (clutch cover) |

`vo.all` (every voice but the starter: gearbox, axle, front, head, tank, gateOut, chain, clutch) is added to `Extras::mech`, so today's mix hears them until card L places
each output.

## The intake's voice with card E's induction

With `spec.intake.model` 1 (whatever `spec.voices.model`), `Extras::tick` takes card E's own radiated sound
(`intk.sound`: its air box's snorkel, or individual throttle bodies' trumpets) in place of the reference's box
filter; the throttle's hiss is added as before. `ex.mouth` is the intake's mouth alone, for the listener to place.
Test `intake:` (the default four at 4000 rpm wide open on the dyno): ITBs over a plenum's snorkel +31.7 dB in
`ex.mouth`, as card E's own measure; in the mix, through the listener's compressor and soft ceiling at -24 dB,
+18.2 dB at the firing's first six harmonics.

## Measured

- Lines: every voice's line within 0.5 % of its computed frequency (standalone, all six gears at 150/300/520 rad/s;
  in the engine: the alternator at 100.8 x crank on the bench, the constant mesh at 19 x crank in third, both found
  at the computed frequency). Straight cut over helical 12.0 dB.
- Monotonic: whine with load and speed; fan with coolant and revs; cam gears with revs; supercharger drive and gate
  dump with boost and flow; lash tick with lash; starter with power.
- Silence when the cause is absent: no gearbox on the bench (the whole program's output identical to the bit with
  the whine on or off); final drive at rest; rattle with the clutch out, in gear, a smooth crank, an automatic; no
  turbo, no gate; fan below its temperature or with the key off; pump with the key off after its prime.
- Clicks: each voice's largest step while coming in and going out at most its steady one (x1.05).
- Soak: ten minutes of random inputs, every voice on: finite, peak < 1.
- Level in the mix (V8, drive-car, view Engine, 1 s windows): the reference engine bay 0.0016-0.0028 rms of a mix
  of 0.08-0.23; the voices (all but pump and starter) 0.0002-0.0015, 6 to 18 dB under the reference's bay. In the
  exhaust view both are some 80 dB under the mix: where they are heard from is card L's.
- Cost (`tee-bench --engine`, 5 runs, everything on: gear cam drive, solid lifters, direct injection, fan running):
  V8 drive-car 1.092 against 1.021 off, free-revs 1.087 against 1.027: about 0.06-0.07 R (budget 0.08). Profile:
  GearVoice 46 ns, DriveVoice 38, FanVoice 28, StarterVoice 16, FuelVoice 14, the rest under 5 a sample.

Bike (`bike:`): chain line at the computed frequency for 14/16/18 teeth at 60/150 rad/s; louder with load; none
without a bike; the clunk grows with the closing's speed (4.5 rad/s over twice 1.9's peak); the dry clutch rattles at
idle with the lever out and is silent pulled, under half load, or wet. In the engine (bike-naked-twin): the chain's line
found at 257.2 Hz against 257.5 from `tel.car.sprocketSpeed`; four lash closings heard on and off the throttle. A
bike's own gearbox (card B2's `Car::bikeGearboxInput()`, `bikeGearboxRatio()`, `gearNow()`) feeds the gear whine as a
sequential (straight-cut) box; its chain is its final drive, so the final drive's whine gets no speed: the constant
mesh at 19 x crank / primary found at 792.0 Hz against 796.8.

## Levels at idle

The owner's 21:32 report: first.json (a 2.0 litre turbo four, direct injection, an electric fan) through card L's
Places listener (camera Side) carried a high hum. Measured (p-idle-warm, 8 to 14 s, each voice set to -60 dB in
turn): the 3895 Hz line was the fuel pump's impeller (41 vanes x 95 turns/s), 760 Hz its commutator (8 x 95), 280 and
560 Hz the electric fan's blade passing (7 x 40) and its harmonic. The fan ran because a warm idle holds the coolant
at 368 K, which was `fanOn`'s default: a few hundredths of a kelvin either side of the hold decided whether the fan
came in. `fanOn` is now 373 K (100 C) by default, off 5 K below (a thermo switch's two contacts): the warm hold sits
at the off point, 5 K under the on, so every warm render has the fan off unless its coolant climbs past 373 K by
itself (a long pull) or the build sets `fanOn` lower.

Who carries what (the lead's ruling, 21:47): package M sets each voice's level at its source,
stated against the exhaust's through the reference's listener (`spec.sound.model` 0, which has no places and no
distance); card L carries everything from the scene's inputs to the ear. Neither trims one voice to make up for the
other's path.

Each voice alone, less the same run with every voice silent (an exact cancellation), in dB against that silent run's
mix (the exhaust's above all). first.json, cold idle 3.5 to 5.5 s, the fan switched at 300 K with the air at 301 K so it
runs; test `engine: levels at idle`:

| Voice | dB(A) | 250 Hz | 500 | 1 k | 2 k | 4 k | 8 k | Bound (judgement) |
|---|---|---|---|---|---|---|---|---|
| Fuel pump | -59.9 | -121 | -110 | -49.1 | -99 | -37.4 | -79 | 50 dB(A) and 35 dB in every octave under: an in-tank pump under a running engine is at most barely heard outside the car |
| Electric fan | -23.3 | -21.8 | -12.9 | -15.6 | -14.9 | -17.4 | -25.4 | 15 to 35 dB(A) under, 12 under at 4 and 8 kHz: a fan is heard in front of an idling car, its broadband falling above its peak |
| Direct injectors | -34.3 | -90 | -61 | -53 | -40.1 | -14.2 | -11.0 | 25 to 45 dB(A) under, within 25 dB at 4 kHz: the tick heard beside a DI engine |
| Accessory belt | -60.3 | -101 | -78 | -70 | -46.4 | -50.7 | -46.9 | 45 dB(A) under |

What changed for it: the pump 30 dB down (before: -30 dB(A), -8.2 in the 4 kHz octave; 20 dB for the report, 10 more by
the lead's ruling of 22:19, not scaled by the engine's size); the fan's broadband now falls
18 dB an octave above twice its band (before: -3.4 in the 8 kHz octave). The fan's 280 Hz line is as it was.

The pump's level, 24 November (the owner's words are why it is where it is: do not raise it again from a spectrum
alone; a change goes through the owner's ears). Against the voices-off mix through the reference's listener, first.json
at idle:

| | Gain in `fuel.fm` | dB(A) | 4 kHz octave | Why |
|---|---|---|---|---|
| As shipped to the owner | 0.0035 | -30 | -8.2 | Judgement: "heard through the floor, about the chain's at idle" |
| -20 dB (f074ab47's parent) | 0.00035 | -49.2 | -29.7 | The owner (21:32): "a high pitched hum that almost sounds like the turbo at idle, but far too loud" |
| -30 dB (1a3812e6) | 0.00011 | -59.9 | -37.4 | The lead's ruling (22:19): the owner's "far too loud"; an in-tank pump under a running engine is at most barely heard outside the car; 1.6 to 3.4 dB of margin on the small engines' hum row was one camera from red. Not scaled by the engine's size |

The prime keeps the same level as the pump's hum (the lead, 22:27: a louder prime would be a judgement on a judgement;
it is heard at key-on because nothing else sounds, and from the cabin, which is the listener's business).

The same voices through Places, camera Side (first.json's own listener), measured the same way: the pump -11.7 dB(A)
before the 20 dB, the fan +3.9, the belt -40.4, that is 23 to 35 dB higher against the exhaust than at the source,
with the silent mix itself 15 dB(A) lower than through the reference's. That part is the path's (card L's).

The prime at key-on (first.json, warm, the key on and no start, 0.3 to 1.9 s, rms): through the reference's listener
-105.6 dBFS against -106.3 without the pump (the listener's own floor), some 80 dB under the idle's mix (-25.8):
heard only with the volume far up. Through Places Side -68.4 dBFS against -81.5 without it, 42 dB under the idle's
mix (-26.1): the one sound at key-on there. At idle the pump is -80.6 dBFS through Places Side, 54 dB under the mix.

What a person should compare by ear (the owner's ears are the only measurement these levels will have), each through
the reference's listener (`sound.model` Reference), at a warm idle, with `voices.level` 0, one voice's slider moved:
1. The pump: key on without starting (its 2 s prime, alone), then idling. Beside a real car at idle, is an in-tank
   pump heard at all? If the prime is heard but the idle's whine is not, the level is about right.
2. The fan (`fanOn` 300 to force it on): does it sound like an electric radiator fan in front of an idling car,
   plainly heard but under the exhaust, a hum at 280 Hz with a soft rush, no hiss above it?
3. The direct injectors' tick against `injection` Port: beside a direct-injection engine at idle, is the ticking as
   clear as this, clearer, or less?
4. The belt (`belt` +12 against 0): is the alternator's whine at idle heard at all on a real car?
Each answer moves one number in the table above, at the source; whether the same holds at camera Side is a question for
card L's path.

## Judgement

Every level (the idle bounds above included); the constant ratio 1.55 and the 48-tooth sum; the 12 % eccentricity; the rattle's backlash, drag and
case modes; the resonances of injectors, lifters, gate housing, starter; the 240-degree tap timing; the fan's
clutch curve and band; the pump's speeds and vane count; the gate's chatter law.

## Not done

- A spider-gear voice (the differential's own gears, cornering) waits for steering.
- Done since the first handoff: the key-on prime (card D renders from the ignition's rise; test `engine: the fuel
  pump's prime`), card F's gate (`telGate`, `spec.boost.gate`; test `engine: card F's wastegate`), the intake's own
  sound (`intake:`), the motorcycle's voices and gearbox (`bike:`, `engine: a naked twin's`, `engine: a bike's`).
