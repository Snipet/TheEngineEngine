# The listener: where the engine is heard from

*Package L. What `sim/listen/` models of the sound between the car and a pair of ears, where each number came from,
and what is judgement. Tests: `tests/sim/listen/` (`tee-tests-listen`, `ctest -R '^listen\.'`).*

`spec.sound.model` 0 (the default) is the reference's listener, untouched: five views from a measured gain table,
the distance as a level and a low-pass, its hall, its compressor, limiter and ceiling. The null test holds it to the
bit. Everything below is model 1.

## The files

| File | Module | What it is |
|---|---|---|
| `sim/listen/listen.fm` | `Listen` | Engine's seam (`block`, `mix`, `finish`); model 0's path; model 1's `block1`, `mix1`, `finish1`, `loudnessOf`; the resampler (package Q's) |
| `sim/listen/scene.fm` | `Scene`, `SceneView` | the sources' places, the listener and its ears, each source's three paths, the cameras, the crossfade, the cabin's ways in, the wind |
| `sim/listen/cabin.fm` | `CabinBoom` | the engine's shaking through the mounts into the cabin's modes |
| `sim/listen/places.fm` | `Place` | a place's early reflections and its hall time and share |
| `sim/listen/hall.fm` | `Hall` | the reference's hall: the diffuse tail of every place |
| `sim/spec/sound.fm` | `SoundSpec` | the fields below |

## Sources and places

The car's frame is the reference's: x across to the right, y forward, z up, the rear bumper's middle on the ground.

| Source (`LS_*`) | Signal (Engine's, through `Listen::mix`) | Place (front engine; mid; rear) | Pattern |
|---|---|---|---|
| `TAIL0`, `TAIL1` | `exh.outL`, `exh.outR` | (-0.55 or 0.4, 0, 0.35); a motorcycle's 0.45 up | the reference's piston law for the highs (2.5 c / (pi D sin)), and the body shading a forward listener: 1 - 0.67 u_y |
| `INTAKE` | `ex.mouth` (the mouth alone; the throttle's hiss goes to `BAY`) | (0.35, 3.9, 0.75); (0, 1.4, 1.05); (0, 0.65, 0.9) | the bay's, floor 0.3 |
| `BAY` | `ex.mech` (less M's `vo.all`), M's `vo.front` and `vo.head`, `ex.block`, the headers' walls, the hiss | (0, 3.4, 0.6); (0, 1.1, 0.5); (0, 0.55, 0.5) | open forward-up and sideways, floor 0.1 |
| `BAYOPEN` | `boost.turbo`, `ex.starter`, `ex.comb`, M's `vo.gateOut` | as `BAY` | the same, floor 0.7 (these carry round the car) |
| `TYRES` | `car.road(...)` (driven only) | (0, 2.2, 0.1) | none |
| `WIND` | its own noise, 250 Hz to 2.5 kHz, as the cube of the speed (driven only) | (0, 2.4, 1.2) | none |
| `DRIVE` | M's `vo.gearbox` (spec.voices.model 1) | (0, 2.4; 0.9; 0.9, 0.35) | the floor pan above: 1 - 0.8 u_z |
| `AXLE` | M's `vo.axle` | (0, 0.9; 0.6; 0.6, 0.3) | as `DRIVE` |
| `TANK` | M's `vo.tank` | (0.3, 0.8; 3.6; 3.6, 0.35) | as `DRIVE` |

M's voices come summed into `ex.mech` (`vo.all`) for model 0's mix; model 1 takes the sum out and hears each from its
place (every voice at the valvetrain's strength). `DRIVE`, `AXLE`, `TANK` are read only while their voice has sounded
in the last second.

The bay's pattern: `a + (1 - a) max(c1^2, |u_x|^3)`, `c1 = (1 + u . axis) / 2`, axis forward-up for a front engine
(0, 0.6, 0.8), up for a mid (0, 0, 1), up-back for a rear (0, -0.3, 0.95); a shaded path is also duller (a low-pass at
2 + 18 g^2 kHz, diffraction taking the highs first); the ENGINE camera's open bonnet removes the shading from above.
These shapes are judgement; the floors `a` were fitted (below).

## The listener

A place and a facing, and two ears:

- **Outside: the reference's ORTF pair** (cardioids 17 cm apart, each 55 degrees out). A recording of a car is made
  with a pair of microphones, and the reference's views have always been heard as one: keeping it makes model 1's
  presets comparable with them, sample for sample in the pattern.
- **In the cabin: a head.** Ears 16 cm apart, omnidirectional, the far side of the head duller (a one-pole falling to
  1.5 kHz for a source straight across, as Brown and Duda's spherical head's shadow does at that size) and 3 dB weaker.

Each source reaches each ear by its direct path and both ears by its image in the ground: three taps a source. A tap
is a read of the source's delay line (32768 samples, 117 m of path at 96 kHz) at a fractional delay, third-order
Lagrange, gliding each sample to where the geometry puts it every 16. The delay is the **retarded time**: the sound
arriving now left the source when it was `V tau` further back, `|u + V tau| = c tau`, solved exactly for a steady
speed (c = 343.2 m/s, still air). So a car going by bends in pitch by exactly the rate its path shortens: Doppler,
with no term of its own. Along each tap:

- the level, 1/r;
- the air's loss of highs: ISO 9613-1 at 20 C and 50 % humidity gives about 0.1 dB a metre at 8 kHz, a one-pole whose
  loss at 8 kHz is that (`fc = 8000 / sqrt(10^(0.01 r) - 1)`);
- the source's pattern and the microphone's (or the head's shadow);
- the ground's reflection on the image tap: asphalt 0.9, grass 0.6 and a 1.8 kHz low-pass, or none. The two paths
  add into a comb whose notches sit at odd multiples of `c / (2 (r_image - r_direct))`, moving as the car does.

The taps' filters are combined into one one-pole each (`1/fc^2` summed). Cost: about 7 ns a tap, 25 ns a source.

## Cameras (`spec.sound.camera`)

| Camera | Where (r: DISTANCE's 0.5 to 30 m) | Ears |
|---|---|---|
| Tailpipe | (0 or 0.4, -r, 0.7), facing the car | ORTF |
| Rear | (0, -2 - r, 1.2) | ORTF |
| Side | (0.9 + r, 1.3, 1.2), facing across | ORTF |
| Engine bay | (0, bay + r/2, 1.4), facing back, the bonnet open | ORTF |
| Driver | (-0.35, 1.7 (2.1 mid, 2.0 rear), 1.1) | head, in the cabin |
| Trackside | 0.9 + 2r out from the car's line, 1.5 m up, standing still; 40 m ahead of a moving car (beside a stopped one); when the car is 60 m past, it moves on 40 m ahead | ORTF |
| Chase | (0, -3 - r, 1.3), moving with the car | ORTF |

The first five are the reference's views as places. A change of camera, or trackside moving on, sets up a second
listener at the new place and crossfades the two at equal power over 250 ms (the scene is crossfaded, not the
samples; a change asked during a fade waits for it). DISTANCE moves the listener at a walking pace at most (8 m/s:
the pitch moves at most 2.3 % while the dial does). The reference instead kept its microphones where they were and
took the distance as a level, because moving them "would move the sound's colour and its picture too": model 1 moves
them on purpose, since the ground's comb moving with the distance is most of what an outdoor recording sounds like.

## The cabin

In the DRIVER camera the outside sources come through the body: the exhaust's, the tyres' and the driveline's
through the floor (the reference's 260 Hz, Q 0.8 low-pass), the intake's and the bay's through the firewall (its
800 Hz, Q 0.7), the wind's through the glass (a 1.2 kHz one-pole: 6 dB an octave, the mass law's slope), each with a
transmission fitted to the reference's CABIN view (exhaust +2.6 dB, intake +4.3 (the air box against the firewall),
the shaded bay -6.5, its carrying part -4.7, tyres -0.7). An open window lets each in past its filter at half its
level, `(1 - w) filtered + 0.5 w direct`: measured in the engine, the highs' share at 2860 rpm goes from -30.8 dB
(shut) to -18.6 dB (open).

**The boom** (`CabinBoom`): the gas's torque reaction (`crank.tqNow`, the reference's whole source) and, for a custom
engine, card D's free forces and moments (`shake.fx`, `fy`, `mx`, `my`) act at the mounts (the roll over a 0.3 m lever,
the moments over 1.2 m), which pass `T(s) = (2 z w s + w^2) / (s^2 + 2 z w s + w^2)`, f0 12 Hz, z 0.15 (a rubber mount
tuned under the idle's lowest order); then the reference's six cabin modes. Matched to the reference's at 100 Hz;
measured +15 dB at 30 Hz, +8 at 50, -6.5 at 200, -12.4 at 400 against it: lugging booms more, a high speed less.

## Places (`spec.sound.place`)

| Place | Early reflections (ms, level) | Hall's RT, share at ROOM 0.15 |
|---|---|---|
| Hall | none of its own: the reference's room | 0.4 + 2.2 ROOM s, ROOM |
| Field | one slap off a tree line 34 m away, 200 ms, 0.05, below 1.8 kHz | 0.3 s, 0.03 |
| Street (14 m wide, 4 m from a facade) | 23, 58, 81, 105, 140; 0.5 to 0.12 | 1.4 s, 0.25 |
| Tunnel (9 by 6 m, concrete) | eight from 23 to 94; 0.6 to 0.25 | 3 s, 0.8 |
| Garage (6 by 7 by 3 m) | eight from 5 to 40; 0.55 to 0.25 | 0.8 s, 0.5 |

Levels are the image sources' spreading against a direct path of some 4 m, times the surface's reflection (brick and
concrete 0.9, a tree line 0.3), rounded: judgement in the details. In the cabin a place is a third as loud with the
windows shut. A new place fades its reflections out and in again over 40 ms time constants.

## The last stage: headroom, limiter, ceiling

The reference lives on its limiter: the V8 build peaks at 7.3 before it at full load and takes 24 dB of reduction on
a free rev. Model 1 is staged so that a build's loudest ordinary moment (the V8 build at full load, 0.75 of its
redline, the default camera and distance) peaks at 0.5, 6 dB under full scale: a gain of 0.116 (-18.7 dB) after the
compressor. Then a limiter at 0.7 whose attack is the sample (no sample leaves it above 0.7) and whose release is
250 ms; a soft ceiling past 0.75 toward 0.95 (`0.75 + 0.2 tanh((|x| - 0.75) / 0.2)`), a guard; and a clamp at 0.99
after the resampler, whose ripple on a limited peak was measured at 0.733 (48 kHz). `tel.sound.limiter` is the
deepest reduction over the last 0.4 s; `tel.sound.ceilingParts` counts the parts the ceiling bent. X-004 (a turbo six's
launch with eight switches on, past 1.0 before): largest sample 0.795, no guard trip.

## Level

The reference keeps two tables by layout (`exTrim`, `gainNorm`), measured. Model 1 computes the exhaust's level from
the engine (`Listen::loudnessOf`): n_eff, the lowest order of the cycle at which the firings' comb
`|sum exp(i 2 pi k fire_c / cycle)|` reaches half its whole (n for an even-firing engine, fewer when firings bunch: a
cross-plane four's is 3). Measured at full load, 0.75 of the redline, on the reference's sixteen piston layouts at
their own sizes and pipes, the exhaust's radiation falls 26.96 dB a decade of n_eff (least squares; residuals -5.3 to
+6.2 dB, the cross-plane four the most); 26 is used, and a rotary's pulses are 16 dB stronger (its two-rotor, fitted).
The gain brings each to the V8's level; the whole is then the V8's (`gainNorm` 0.6645). Result
(`loudness: ...` test): the exhaust's spread at full load across the sixteen is 11.3 dB, against 17.1 dB through the
reference's tables. It depends on the engine only, not the exhaust: a muffler still makes a car quieter. Left: the
bay's sources are not normalised (their spread is Extras'), and the size within a layout is kept (a bigger engine is
louder, as in the reference).

## The fit to the reference's views

Each source's strength (its level as it leaves its place) was fitted so that the five cameras that are the
reference's views hear the sources in the reference's balance, band-limited noise (100 Hz to 1.5 kHz) into each
source on the rig (`tests/sim/listen/ExploreTest.cpp` prints the table). Strengths (dB, on the reference's k's):
exhaust -4.85, intake +0.8, valvetrain +0.3, block -1.6, headers' walls +0.9, turbo +2, starter -0.4, combustion
+2.4, tyres -8.2. Residuals at the default distance: **the exhaust within 1.1 dB in every view**; the rest within
6.6 dB (the headers' walls in REAR the worst: the reference's REAR hears the bay better than its EXHAUST, 2 m nearer,
which no geometry gives); 2.86 dB rms over the 45 source-view pairs.

## What it leaves out

Diffraction round the body (the shading is a pattern); the ground's impedance by angle (a constant reflection);
reflections off the car's own panels; wind at the microphones; the turbo and its valves as places of their own (they
are in the bay); each tyre on its own (one source at the wheelbase's middle); each of card G's radiating ends by its
tag (the exhaust's two channels are the reference's two tails until G's network lands in the engine: `LS_TAIL0/1`
are where its tags `XTAG_REAR_L/R` will go, and `XTAG_SIDE_*`, `FRONT`, `UP` need a source each).

## Cost

`tee-bench --engine`, drive-car and free-revs, 3 runs, a busy machine (R about 2.5 us then): the V8 at model 0 1.045
R, model 1 1.112 to 1.131 R by camera: model 1 adds 0.07 to 0.09 R over the reference's own listening path (budget
0.15 R). `tee-bench --profile` (drive-car, trackside): `Scene::hear` 153 ns, `Scene::aim` 33, `Listen` 100, `Hall` 67,
`Place` 25, against model 0's `Listen` 108 and `Hall` 53; in the cabin `Scene` 152 and `CabinBoom` 14.

## What a person should listen to

- The V8 driven past trackside on the field (`content/builds/listeners/v8-trackside.json`): the pitch's fall at the
  pass, the comb sweeping as the car comes and goes.
- Every camera in turn while revving: no click, the balance moving.
- The driver's seat with the windows shut and open; the boom while lugging at 1200 rpm in a high gear.
- The chase through the tunnel (`v8-tunnel-chase.json`); the street and the garage at a revving idle.
- DISTANCE swept slowly at the tailpipe on the field: the comb's notches moving.
- A single, a flat-plane four and a V12 at full load, one after the other: one loudness?
- Model 0 against model 1 on the same build: model 1 is quieter by design (its headroom) and far less limited; turn
  the host's volume up rather than LEVEL.
