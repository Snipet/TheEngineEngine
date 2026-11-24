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
