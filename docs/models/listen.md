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

The default is Tailpipe (camera 0; lead, 23:34): an engine-sound program opens where the exhaust note is, and it is
the reference's default view (EXHAUST), which every earlier build played: the engine alone through it is within 2 to
5 dB of the reference's in every octave. Rear, Side and the rest are a person's choice from there; Rear is right as
what a person behind the car hears (the bay shaded by the body: some 11 dB darker at 8 kHz than the reference's REAR,
whose bay no geometry gives; the ground's notch near 1 kHz). A build states a camera only when it means one.

| Camera | Where (r: DISTANCE's 0.5 to 30 m) | Ears |
|---|---|---|
| Tailpipe | (0 or 0.4, -r, 0.7), facing the car | ORTF |
| Rear | (0, -2 - r, 1.2) | ORTF |
| Side | (0.9 + r, 1.3, 1.2), facing across | ORTF |
| Engine bay | (0, bay + r/2, 1.4), facing back, the bonnet open | ORTF |
| Driver | (-0.35, 1.7 (2.1 mid, 2.0 rear), 1.1) | head, in the cabin |
| Trackside | 0.9 + 2r out from the car's line, 1.5 m up, standing still; 40 m ahead of a moving car (beside a stopped one); when the car is 60 m past, it moves on 40 m ahead | ORTF |
| Chase | (0, -3 - r - 0.25 v, 1.3), moving with the car, a quarter of a second back | ORTF |
| Free | spec.sound.listenerX, Y, Z, facing (degrees from forward toward the right), walked to at 8 m/s | ORTF |

**A motorcycle** (spec.car.kind Motorcycle, car.model 1; package B2's geometry, a superbike's scaled by the
wheelbase over 1.41 m, B2's x from the rear contact = y - 0.3 here): the can at (0.2, 0.3 + 0.30 k, 0.45), the
ram-air mouth at (0, 0.3 + 1.45 k, 0.75), the engine under the tank at (0, 0.3 + 0.75 k, 0.5) in the open (no
shading), the rider's head at (0, 0.3 + (0.65 + 0.2 tuck) k, 1.40 - 0.25 tuck), the wind at the helmet; every point
turned nose-up about the rear contact by `tel.car.liftAngle`. The Driver camera is the rider's head (two ears, no
cabin). Measured on bike-superbike at full throttle: the intake 13 dB under the exhaust at the head (B2 expects the
intake to lead: for the owner's ears before any retune).

The first five are the reference's views as places. A change of camera, or trackside moving on, sets up a second
listener at the new place and crossfades the two at equal power over 250 ms (the scene is crossfaded, not the
samples; a change asked during a fade waits for it). DISTANCE moves the listener at a walking pace at most (8 m/s:
the pitch moves at most 2.3 % while the dial does). The reference instead kept its microphones where they were and
took the distance as a level, because moving them "would move the sound's colour and its picture too": model 1 moves
them on purpose, since the ground's comb moving with the distance is most of what an outdoor recording sounds like.

## The cabin

In the DRIVER camera the outside sources come through the body: the exhaust's, the tyres' and the driveline's
through the floor (the reference's 260 Hz, Q 0.8 low-pass: the exhaust's band, where the reference's CABIN is matched),
the intake's and the bay's through the firewall (an 800 Hz one-pole: a wall's mass law, 6 dB an octave; it was the
reference's second order, which left the cabin's 4 and 8 kHz octaves 19 and 32 dB under the reference's CABIN on the
engine alone, now 6.5 and 12.4, the rest from 250 Hz to 2 kHz within 4 dB; the fit unchanged, 2.86 dB rms), the
wind's through the glass (a 1.2 kHz one-pole, the mass law's slope too), each with a
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
a free rev. Model 1 is staged on the archetypes (content/builds/archetypes, Rear camera on the field, the validation
free rev p-free-rev): a gain of 0.58 (-4.7 dB) after the compressor puts the middle of them near 0.3 (0.08 for the
quietest, a kei triple turbo; 0.25 to 0.35 for the turbo sixes, the V6s, the fours; the dozen loudest, the big V8s,
the V12, the high-revving six, reach the limiter on their peaks). It was first staged on the reference's wild-cam V8
build alone (0.116, -18.7 dB): that build is some 20 dB louder than a typical one, which left the archetypes 23 to 30
dB under the reference's mix (P's measure, 20:10). Then a limiter at 0.7 whose attack is the sample (no sample leaves
it above 0.7) and whose release is 250 ms; a soft ceiling past 0.75 toward 0.95, a guard; and a clamp at 0.99 after
the resampler, whose ripple on a limited peak measures 0.73 to 0.75 (48 kHz). `tel.sound.limiter` is the deepest
reduction over the last 0.4 s; `tel.sound.ceilingParts` counts the parts the ceiling bent. X-004 (a turbo six's
launch with eight switches on): no guard trip. Between the compressor and the limiter sits the auto level (below), on
by default; with it off (`spec.sound.autoLevel` 0, True levels) the stage is as described here and a build's own
level (spec.sound.level, -24 to +12 dB) is where it is brought to the others.

## The ground's comb (the "fixed mode near 2.3 kHz")

The colour heard near 2.3 to 2.6 kHz on first.json at the opening camera, the same at every engine speed, is the
ground's reflection, not a pipe mode, the body, or a filter (asked with G, 25 Nov). At the Tailpipe camera's default
place the listener is 1.71 m behind the pipe and 0.7 m up, the pipe 0.35 m up: the direct path is 1.745 m, the ground
image's 2.007 m, 0.262 m longer. The two add in phase every c / 0.262 = 1325 Hz and cancel half-way between, from
c / (2 x 0.262) = 662 Hz. Measured (p-full-load-4000, Asphalt over None, the engine alone): notches at 650, 1957, 3270,
4582, 5865 Hz, 1308 Hz apart; between the second and third, the lobe the ear takes for a mode, with -11 dB at 2 kHz and
-6 dB at 3.15 kHz on its sides. It is gone with the ground None, gentler on grass (0.6 and a 1.8 kHz low-pass on the
reflection), at other places in the band at other cameras (Rear, 1.2 m up and 2.3 m back: the first notch near 1 kHz),
and it moves with DISTANCE. The engine's speed does not move it.

Physical as far as it goes: hard ground near a source gives this comb, which is why microphones are laid on the ground
to avoid it. What the model leaves out is what blurs it in life: the pipe's mouth is not a point, the ground is not
flat to a centimetre, the air moves, and a car moves; each smears the notches over a few per cent, and the real comb
is softer above 2 kHz. Not changed (no code in this window); if it reads as a fault, the candidates for a later one are
a reflection that falls with frequency on asphalt as on grass (some 0.95 to 0.8 over 1 to 8 kHz), or a small, slow
jitter of the image's path (a few millimetres) that softens the deep notches above 2 kHz.

## Auto level (`spec.sound.autoLevel`)

The ruling (lead, 20:13): at the defaults a free rev peaks at 0.3 to 0.6 of full scale and the idle is plainly
heard; a far camera or the cabin is not left 20 to 30 dB down; within a setting, the levels stay true. Of the two
schemes offered, a fixed make-up gain with trims, or an exposure, this is the exposure, for three reasons. The
spread to close is not one number: the archetypes' free revs span 0.076 to 0.74 (20 dB) at one camera, and the
cameras add 6 to 15 dB more (the cabin, trackside at 40 m), so fixed trims would be a table of builds by cameras that
every new build and camera must extend, and any user's change of muffler or camera puts the result outside it. A trim
is a guess made before the sound exists; the exposure measures the sound it is given. And it can be switched off for
anyone comparing true levels (a muffler against a straight pipe, the cabin against the street), which a trim baked
into every build cannot.

How it works (`Listen::autoLevel`, `finish1`): P, the loudness exposed for, is taken from the peaks after the
compressor. P rises to any peak at once and the gain follows within 1 ms, measured on each 16-sample part before it is heard, so a rev's onset is caught by the gain, not
the limiter. P holds 1.5 s, then falls at 4 dB a second. It never falls below 4 times Q, the level of what is heard
lately (0.1 s windows' peaks: Q falls over 1 s and rises at 0.3 dB a second, so a 4 s rev moves it 1.2 dB). The gain is
0.4 / P, within -30 to +30 dB of the staged level, and rises over 0.5 s. In a start (the starter turning and 1.5 s
after; and the first sound after silence, for 3 s) P falls at 12 dB a second without the hold and Q follows what
comes over some 0.3 s, so the crank and the catch do not set the exposure and the catch's flare stands over the idle. A cut (another camera, place, or the windows past half)
starts afresh; silence holds it. (Until the first sound counted as a start, a start whose starter was not heard, a
voice muted or none, left Q at the first faint sound, rising 0.3 dB a second, and P on the catch's peak: the idle
5.5 dB over where a heard start leaves it, +15.6 dB against +10.1 on first.json. M measured the two apart, 25 Nov;
`the auto level after a start` holds them within 1 dB from 2 s on.)

A burst does not set the exposure: a part over four times the parts' running 80th percentile (log domain, some
50 ms) does not raise P, and goes to the limiter, as a real limiter takes a crack. A bang burned in the pipe (exhaust.bang
1, the default since 25 Nov) is a burst of 1 to 4 ms; before, each one raised P and held it 1.5 s, the body falling with
it. Card G's clips (25 Nov, 81ec1b12): the rally five's lift, Burned against Carved, from 6.4 dB under to 0.8 over; its
anti-lag, from 3.5 dB under to 2.1; the limiter then acts in 22 to 31 % of the 0.4 s reads, 3 dB when it does. The
flat-plane V8's lift stayed 4.5 dB under (5.0 before): its bangs come close together and stand less than four times over
the parts round them, so the level's percentile alone took them for its body. At twice, not four times, a free rev's own
pulses counted as bursts (the kei's and the V12's revs to 0.65 and 0.68, typical): four keeps the revs where they were.
So a burn is keyed by what it is, not by its level alone (26 Nov): while a burn in the pipe is alight (the exhaust's
bangHeat, held while its sound is on its way: the tailpipes' longest tap and 40 ms for the pipe's run and the ring), a
part over twice the parts' running median is a burst too. A window whose every part is a burst leaves Q where it was.
The V8's lift (card G's clip): 4.9 dB under, then 0.3; the rally's +0.8, then +1.5; the free revs the same to two
decimals (they burn nothing). Its price: on the V8's spark cut, the limiter acts in 74 % of the 0.4 s reads, not
45 %, and the body there is 4.4 dB over Carved's: those bangs are now limited, not exposed for. Carved bangs, and a
model 0 exhaust's pops, have no bangHeat: for them only the level's rule acts.
`the auto level and the bangs` holds the rally's lift within 2 dB (5.8 dB under without the burst rule); its nightly
form holds the V8's (-0.2 dB; -5.1 without the key).

Its known limit: a sound whose loudest state is its steady one is exposed about 12 dB under the target, by design,
for the headroom a rev would need. The tractor's cabin is the case: its governed rev rises 1.4 dB over its idle there,
so the floor sets its exposure, and it sits 5 to 6 dB under the Rear where it is truly 0.8 dB over. Measured and set
aside (25 Nov): a floor giving way with the throttle either lets every rising rev come in loud and settle (3 to 7 dB,
first.json) or swells that cabin's rev; an exposure on the 0.1 s windows' 90th percentile sees a rev late (3.8 dB).
The Rear's typical level under the cabin's (1.5 to 2.6 dB on first.json) is the exhaust's own crest, not stray cracks.

What that means for a listener: a steady sound (an idle, a cruise) sits near 0.1 (12 dB under the target), which
leaves room for a rev. A rev peaks near 0.4 to 0.55, and the gain moves only when the rev outgrows the room. A range
wider than 12 dB within a setting is closed slowly: a V12's idle comes up some 3 to 6 dB a few seconds after a rev.
Faster changes are kept exactly: the gain moves at most 4 dB a second upward. Whatever passes the target is caught
by the limiter, which reports in `tel.sound.limiter` as before. `tel.sound.autoGain` is the gain now (dB).

Measured (the `the level at the defaults` test at e1692f91's content: P's builds without trims, M's quieter pump and
fan; each build's own sound settings; true levels at 35e355de, Rear). A rev's "typical" peak is the 90th percentile
of its 0.1 s windows' peaks; its largest includes the cracks on the rev limiter, each faster than the 16-sample part
the auto level looks ahead by, which meet the limiter (0.7) instead:

| build | true levels: idle / free rev peak | auto level: idle / rev typical / rev largest | auto gain |
|---|---|---|---|
| first.json (2.0 L turbo four, Tailpipe) | 0.02 / 0.17 | 0.12 / 0.35 / 0.41 | +6.3 dB |
| kei 660 triple turbo (Tailpipe) | 0.01 / 0.08 | 0.10 / 0.40 / 0.49 | +12.2 dB |
| V12 6.5 (Tailpipe) | (limited) / 0.72 | 0.11 / 0.42 / 0.56 | -21.4 dB |
| tractor 15 L six (Tailpipe) | 0.01 / 0.06 | 0.14 / 0.29 / 0.70 | -2.8 dB |

(At the opening camera, Tailpipe, since P's builds stopped stating Rear; the true levels are Rear's at 35e355de.)
The tractor's onset into the pipe meets the limiter (0.70; its limiter reads -2 dB at most, 6 % of them, over a rev).
The tractor's governed rev (2100 rpm) is only some 3 dB over its idle once its onset is past; the auto level keeps
that, as it should. The test holds the rev's largest to 0.28 to 0.76 (the limiter's ceiling) and its typical peaks to 0.15 to 0.6.

The limiter on the loud builds, at first.json's sound settings (Side, Field, +7 dB), a free rev (p-free-rev, 5 to 9 s)
and a launch (p-quarter-mile, 10.5 to 20 s); the share is of `tel.sound.limiter`'s 0.4 s reads under -0.5 dB:

| build | auto: free rev | auto: launch | true levels: free rev | true levels: launch |
|---|---|---|---|---|
| truck 15 L six | never | never | -10.6 dB, 39 % | -10.3 dB, 12 % |
| V12 6.5 | never | never | -19.9 dB, 91 % | -18.5 dB, 94 % |
| big-block 7.4 | -1.2 dB once (the rev limiter's crack) | never | -4.2 dB, 72 % | never |
| flat-plane V8 4.5 | never | never | -18.2 dB, 82 % | -18.1 dB, 94 % |

With the auto level the limiter is a guard again: the one case is a single crack that jumps 9 dB inside a block,
caught with 1.2 dB for 250 ms. With true levels at a staged +7 dB, the loud builds live on it, as the reference does.
No straight-piped build is in content/builds with a sound of its own (exhausts/straight-pipes.json is an exhaust part).

On first.json, by camera (free rev peak, true / auto): Rear 0.17 / 0.55, Tailpipe 0.24 / 0.59, Side 0.18 / 0.50,
Engine bay 0.14 / 0.53, Driver 0.09 / 0.53, Trackside 0.11 / 0.53, Chase 0.16 / 0.53 (at 35e355de). The whole run's
peak stays under 0.7 everywhere. The rig's tests (ListenerTest) run with true levels, so the
distance law, the Doppler and the balance are measured unaltered; `the auto level` test checks that the Driver is
3 dB or more under the Rear with it off (3.4, by the rev's typical peaks) and within 2 dB with it on (0.3).

Cost: some five operations a sample in the last loop and one window's arithmetic every 0.1 s. tee-render on first.json
does not resolve it (3.82 to 4.19 s with it off, 3.83 to 4.01 s on, for 14 s of audio).

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

## Package M's voices through Places, and the fuel tank's wall

The voices come apart into their places (`mix1`): the front's belts and fan, the head's lifters and the clutch in
the bay; the gearbox; the axle and the chain; the tank's pump. Each is heard at the reference's mechanical strength
(`sMc`), so M's levels, staged in the reference's mix, keep their meaning. The pump is the one source inside a box:
a moulded HDPE tank, 5 mm, m = 4.75 kg/m2, whose wall by the mass law at field incidence (TL = 20 log10(m f) - 47 dB,
Bies and Hansen, Engineering Noise Control) is a one-pole low-pass at 47 Hz: 27 dB at 760 Hz, 38 dB at 3.9 kHz (the
fuel, the floor and the leaks left out). Without it the pump was heard as if in the open under the car, and on
first.json at Side its 3.9 kHz and 760 Hz tones stood 46 and 38 dB over their neighbours: the owner's "high pitched
hum" (lead, 21:32). D2's extras at that place (a truck's air dryer) are not behind it.

Held by `the voices through Places` (EngineTest; quick at Side, nightly at Side, Rear, Driver): first.json held warm
as p-idle-warm holds it (the fan runs), voices on against off, through Places and through the reference's matching
view (Side, Rear, Cabin). Places / the reference, the voices' share above 250 Hz: Side +18.6 / +17.7 dB, Rear
+8.6 / +4.4, Driver +19.6 / +11.8; the fan's 560 Hz against the engine's own highs: 8.3 / 7.0 dB over, 5.6 / 8.1
under, 9.4 / 1.4 over; on a free rev no voice's tone stands out. Tolerances 3, 6 (the fit's worst residual), 9 dB.
(With the fan running, the cabin passed its 560 Hz some 8 dB more than the reference's CABIN; M has since moved the
fan's switch to 373 K and its level.)
The voices' own levels are M's (the fan's 560 Hz stands 7 dB over the engine's highs even in the reference's Side).

The camera matters more than any of this: the reference's Side view hears the bay 26 dB louder against the exhaust
than its default view (Exhaust) does, and model 1 is fitted to the reference's views. A build meant to be heard
first sits at Rear (the default), where Places matches the reference's mix within 4 dB in every band from 250 Hz.

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

Compare these three first (marked first in the handoff). Each answers a question no test can:

1. **Model 0 against model 1** on first.json, Rear, Field, a free rev: does model 1 sound at least as real as the
   reference? Also: is the idle plainly heard, and does the level breathe after the rev? It should not.
2. **Rear against Driver** (windows shut) on first.json, cruising in third: one loudness, with the seat duller and
   boomier. Does the cabin sound like a cabin, or like a muffled microphone?
3. **The trackside pass** (`content/builds/listeners/v8-trackside.json`): the fall in pitch, the comb sweeping as the
   car comes and goes. This is model 1's headline effect; if it sounds wrong, nothing else matters much.

Then:

- The V8 driven past trackside on the field (`content/builds/listeners/v8-trackside.json`): the pitch's fall at the
  pass, the comb sweeping as the car comes and goes.
- Every camera in turn while revving: no click, the balance moving.
- The driver's seat with the windows shut and open; the boom while lugging at 1200 rpm in a high gear.
- The chase through the tunnel (`v8-tunnel-chase.json`); the street and the garage at a revving idle.
- DISTANCE swept slowly at the tailpipe on the field: the comb's notches moving.
- A single, a flat-plane four and a V12 at full load, one after the other: one loudness?
- Model 0 against model 1 on the same build: model 1 is far less limited. With the auto level on, it is as loud as
  the reference's mix; with True levels it is quieter by design (its headroom).
- The auto level against True levels on one build, Rear then Driver: with it on they should be one loudness, the
  driver's seat duller; with it off the seat some 7 dB down. Listen for the gain breathing after a rev (it should
  not: the idle under a rev keeps its distance).

## Savings that change the sound (for an Economy tier, never in Full silently)

Measured, not adopted: each renders the V8 on the listening scenarios (tests/sim/listen/scenarios/) against today's
model 1, and gives the difference's level against the signal and the change of the bands' levels (left channel).

| Candidate | Where it changes the sound | Measured |
|---|---|---|
| Linear taps (two reads, not four) for every path | everywhere a path moves or sits between samples | difference -37.6 dB (trackside pass), -39.0 (tailpipe pull), -49.2 (street, revving), -86.1 (driver's seat); highs 10-20 kHz -0.50 to -0.56 dB, 5-10 kHz -0.15 dB, below 5 kHz under 0.03 dB |
| Linear taps beyond 20 m only (5598 samples of delay) | far sources only: trackside before and after the pass | -59.7 dB on the pass; the other three scenarios unchanged to the bit |

What a person should listen for: a duller top on a close pass, and a faint roughness while the pitch bends
(linear interpolation's ripple moves with the fraction). The saving was not measured; the read is about half a
tap's cost (about 3 to 4 ns a tap). Already taken, unchanged to the bit: a still tap's Lagrange weights computed once
a part (Scene::hear 167 -> 96 ns on the V8's free revs at the tailpipe).
