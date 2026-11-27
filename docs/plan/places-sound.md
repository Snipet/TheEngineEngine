# Places against the reference: the owner's three faults, their causes, and what is proposed

Package L, 27 Nov. The owner listened to Places (sound.model 1) in the Listen view on their Mac build of `main` and said:
"the Places mode doesn't sound as good as the reference. The Places mode causes some comb-filter effects, and it brings
out the noise and quiets the engine sound." This page holds the measurements behind each of the three, what is
proposed for the next window under `sim/`, what is not proposed and why, and the clips by name. Nothing here is built:
every change below was tried on a scratch copy of `sim/` (tee-render `--sim`), never pushed.

**A correction stands over the first figures.** The loudness figures first reported (L to the lead, parts 1 and 2,
27 Nov morning) were made by a script that ran the K-weighting along the channels, not along time: every one of them is
void, and with them the claim that a pull rose only 0.8 LU over the idle under Places. The figures below are measured
again with the filter along time (BS.1770 K-weighting, integrated, no gating). The comb's, the octaves' and the
spectra's figures were unaffected.

## How it was measured

first.json (the app's first build) and four builds chosen before any was looked at: the big-block 7.4 V8
(archetypes, automatic), the 15 L truck six, the supersport 600 bike, the twin rotary 1.3 (archetypes). Each at its
shipped Places defaults (camera Tailpipe, place Field, ground Asphalt, auto level on) against its own reference
settings (sound.model 0). Idle: `p-idle-warm`, 6 to 12 s. A steady mid load: the engine dyno at 45 % of the redline
(the truck 1300 rpm), throttle 0.3, 6 to 10 s. A pull: `p-pull-auto` (V8), `p-pull-eighteen` (truck, 9.6 to 13.9 s),
`p-pull-paddles` (the rest), 4.6 to 12.5 s. Measures: integrated loudness against the reference (LU); the idle-to-pull
spread; the comb's ripple (the 95th less the 5th percentile, 300 Hz to 6 kHz, of the Places-over-reference spectrum
less its third-octave smoothing); the top octave (16 kHz) against the body (500 Hz to 4 kHz), relative to the
reference's; the 500 Hz octave against its neighbours, relative to the reference's.

## The three faults

### "Comb-filter effects": the ground under the Tailpipe camera

At first.json's mid load the spectrum has notches at 659, 2001, 3272, 4617 and 5892 Hz: a spacing of 1310 Hz, a delay
of 0.763 ms, 11 to 17 dB deep, alike in both channels. It is the ground's image tap (asphalt, groundR 0.9) summed with
the direct path: the Tailpipe camera stands 1.71 m behind the pipe (the distance dial's 0.3) at 0.70 m, the pipe's
mouth at 0.35 m, a path difference of 0.261 m. With ground None the ripple goes from 9.5 to 0.6 dB and the 0.77 ms
cepstral peak (45 times its background) is gone; no other path combs (two pipes on one build, the ears, the intake, the
place, the cabin's and the bay's paths were each checked: the cabin's and the bay's ripples are filter shapes, no echo
peak). The reflected ray already leaves the pipe at its own angle through the piston law (scene.fm: the image path's
unit vector), so its highs are not overstated by reuse. The first notch sits at c / (2 Delta), Delta about
2 h_mic h_pipe / r: the hole at 657 Hz in a four's body is a choice of camera, its height above all.

Shipped ripple: first 9.2, V8 11.2, truck 7.7, bike 14.0, rotary 5.7 dB; the 500 Hz octave 3.6 to 7.8 dB under its
neighbours against the reference.

### "Brings out the noise": the highs above 10 kHz at the outdoor cameras

At Tailpipe nothing but the exhaust is heard (muting the intake moves the level 0.2 dB; mechanical and voices nothing).
Above 10 kHz the reference keeps falling (some 8.5 dB an octave): its distance dial puts its pair at its nearest point
(0.5 m back, 35 degrees off the pipe's axis, where the piston law cuts from about 9.7 kHz) and applies a stylised air,
18 kHz / (1 + r / 5). Places stands at the dial's real 1.71 m, about 12 degrees off the axis, with the physical air
(nothing at 2 m). The top octave against the body, relative to the reference: first +6.1, V8 +7.9, truck +9.3, bike
+4.2, rotary +4.6 dB. Against the reference's matching views the other cameras are within 0.5 to 4 dB, except Engine
bay's top (+4 to +6 dB) and Driver, duller (below).

### "Quiets the engine": the auto level brings every build to one loudness

Places minus the reference, idle / mid / pull (LU), and the idle-to-pull spread (the reference's in brackets):

| build | idle | mid | pull | spread |
|---|---|---|---|---|
| first | -0.8 | -3.9 | -6.0 | 10.0 (15.2) |
| V8 | -7.1 | -19.1 | -15.8 | 1.1 (9.9) |
| truck | -16.1 | -19.5 | -19.1 | 4.2 (7.3) |
| bike | -4.7 | -15.8 | -6.5 | 12.5 (14.3) |
| rotary | +9.9 | +1.5 | -0.2 | 7.0 (17.0) |

At the mid load the reference spans 21.5 LU over the five (the rotary -28.8 to the truck -7.3 LUFS); Places spans 3.4.
The auto level exposes for the peaks (a build's loudest at 0.4, a steady note 12 dB under), alike for every build: a
loud engine is turned far down, a quiet one up, and within a build the range is narrowed by about a third on first.json.
That is the larger part of "the engine quieter".

**On `main` today**, the owner can hear Places without it: Listen view, Listener "Places", Auto level "True levels",
then Level about +9 dB on first.json at Tailpipe to stand beside the reference. With the auto level off each build's
own range comes back (first 14.9 LU against 15.2; V8 9.8 against 9.9; bike 13.7 against 14.3; rotary 15.7 against
17.0; the truck 14.2 against 7.3) and the builds keep their differences, some 5 to 10 LU under their references at
Level 0. The comb and the highs remain.

## Proposed for the window (under `sim/`: each waits for the window and a bake)

The two are independent: the spot cures the comb and the highs; the level is the owner's choice among shipped, half
and true levels.

### 1. The Tailpipe camera at a recordist's spot: a boundary microphone on the ground, 0.5 m behind the mouth

A grid fixed before it was measured (0.25 / 0.35 / 0.5 m back; on the ground, 0.15 m, the pipe's height; each at least
25 degrees off the axis; and the onboard spot, 0.25 m from the mouth at its height): on the ground the image and the
direct path coincide, so there is no comb by the formula. Shipped against the boundary spot at 0.5 m:

| build | ripple | top octave | 500 Hz hole |
|---|---|---|---|
| first | 9.2 to 0.3 | +6.1 to +0.3 | -4.5 to +0.1 |
| V8 | 11.2 to 2.1 (its two pipes) | +7.9 to +1.6 | -7.8 to -0.2 |
| truck | 7.7 to 0.2 | +9.3 to +0.5 | -5.7 to +0.3 |
| bike | 14.0 to 0.5 | +4.2 to -0.9 | -3.6 to +0.1 |
| rotary | 5.7 to 1.0 | +4.6 to -5.8 | -4.8 to +0.6 |

- Constants: none but the microphone's place (on the ground; 0.5 m, the reference's own nearest distance; in line with
  the pipe). The dial moves it straight back along the ground, 0.5 m to 30 m, with no comb at any distance; the level,
  the air and the room fall as now. The standing listener's spot with its comb stays where a real ear hears it: Rear
  (1.2 m high) and Side.
- What it is: a microphone lying on the ground. There the direct and the reflected path are one, so the pressure
  doubles where the two stay coherent: measured +3.5 dB at the mid load (asphalt against ground None at the same spot,
  true levels), less than the +5.6 the ground's 0.9 gives at low frequencies, because the image tap keeps its own
  piston law and low-pass. The auto level absorbs it; under true levels the staging is chosen with the spot.
- Cost: the rotary's top octave 5.8 dB dull: physical, its large bore's piston law 34 degrees off the axis.
- With the car moving it is a camera's place, carried with the car, not a stand one could build: no Doppler (it moves
  with the pipes), the wind's source unchanged. The road and the tyres heard from the tarmac, measured on first.json's
  pull under true levels: the road 13.7 dB under the rest at the boundary spot, against 7.8 dB at the shipped spot
  (the exhaust is nearer, the road relatively quieter). No tyre roar comes forward.
- Twin pipes: in line with the right-hand mouth (x = +0.55 m), 0.5 m behind it; the V8's 2.1 dB ripple is its own two
  pipes, the other 1.1 m across.
- CPU nil.
- The onboard spot (the lead's prediction, about 350 Hz and 5 dB of ripple) measured 3.5 to 4.5 dB on four builds,
  11.6 on the bike (its pipe at 0.45 m).
- Null: model 0 untouched (scene.fm's placeListener is Places' alone); the null (`tee-tests "[null]"`) to be run.
- Bake: every baked table's key hashes the program's sources, so any `sim/` change stales them all; P re-bakes the
  whole set after it lands.
- Cases: "the engine's tilt through Places" (Tailpipe against Exhaust, 7/9 dB) and the defaults' level cases move:
  each re-measured and restated on its property, never loosened. New: Tailpipe's ripple on the single-pipe builds within
  2 dB of the reference, shown to bite on the shipped spot (9.2 dB).

### 2. True levels as the default, with one fixed staging (the owner's choice among three)

The auto level does two jobs at once: it brings near and far cameras together, and it brings every build to one
loudness. The second is the complaint. The plain candidate needs nothing computed per camera, build or source: the
switch the code already has ("True levels", the auto level off), with one staging, a Level chosen openly so the
default camera on first.json stands near its reference, then the same for every build. That is the reference's own
behaviour: one staging, a farther camera quieter, the dial turning the level down, the limiter for a loud build's
peaks. Measured at Tailpipe, the staging as the Level (+9.7 dB; +6.7 one to two dB lower everywhere), Places minus
the reference:

| build | idle | mid | pull | spread (ref) | limiter on the pull |
|---|---|---|---|---|---|
| first | -3.1 | -3.5 | -3.9 | 14.4 (15.2) | never |
| V8 | -1.6 | -4.6 | -3.4 | 8.1 (9.9) | 74 % of reads, 4.3 dB median |
| truck | -5.4 | -2.0 | -1.7 | 11.0 (7.3) | 100 %, 5.0 dB |
| bike | +0.7 | +0.3 | -0.1 | 13.5 (14.3) | 35 %, 3.4 dB |
| rotary | +4.4 | -1.2 | -0.2 | 12.4 (17.0) | never |

Against shipped (every build one loudness, the V8 and the truck 15 to 19 LU under) and half (the gain following half
the exposure's change: the V8's mid -7.9), it keeps each build's loudness within about 5 LU of its reference and each
build's range. Its costs, measured:

- The cameras (first.json, mid, against Tailpipe, the auto level off): Rear -5.6, Side -4.1, Trackside -6.0, Chase
  -7.2, Engine bay +1.6, Driver -3.3. A held mid load switched Tailpipe to Rear to Trackside: shipped within 1.4 LU,
  true levels Rear and Trackside about 4 LU under Tailpipe.
- The dial now falls as the reference's does: Tailpipe at distance 0 / 0.3 / 0.6 / 1.0: -23.7 / -26.9 / -33.0 /
  -44.6 LUFS (21 dB from nearest to farthest).
- Trackside, a pass at a held speed and throttle (first.json, second gear, 21 to 23 m/s, 5700 to 6360 rpm): each pass
  swells about 20 dB, from -53 LUFS at 30 m to -33 abreast and back to -53 at 44 m. A microphone 40 m off is quiet
  (some -52 to -55 LUFS); that is what it hears. Shipped's auto gain swings +16 to +30 dB through the same passes.
- The Level dial counts again (with the auto level on it is undone by the exposure).
- The cost that made the auto level exist: the quietest thing an owner can reach. The kei 660 at idle at the Rear
  camera with the dial at its farthest (30 m): -65.5 LUFS under true levels +9.7, against its reference's -57.1 and
  shipped's -45.2.
- With the boundary spot (proposal 1) the Tailpipe camera is 5.7 dB louder at the same staging (-21.2 against -26.9
  LUFS at mid): the staging is to be chosen with the spot.
- The staging is the owner's ear: clips at +9.7 and +6.7 below, and the earlier +3.7.
- Null and bake: the auto level's default lives in the spec's source (sim/spec/sound.fm, autoLevel 1); its change is
  `sim/` and a bake, the reference untouched (sound.level is the reference's own dial, so the staging cannot be a build
  file's line: it would move the reference too). Cases: "the auto level: cameras near and far" and "the level at the
  defaults" bound today's design and would be restated on the new property (each build's idle-to-pull spread within
  some LU of its reference's, shown to bite on shipped: 10.0 against 15.2 on first.json; the V8's 1.1 against 9.9).

## Not proposed, and why

- The level set by the camera's position (a gain from the camera's geometry, fixed while the camera is fixed): first.json
  met its reference (+0.2 / -0.3 / -0.1 LU, spread 14.9 against 15.2), but the tailpipe-only form was 9.7 dB too loud
  at Engine bay (it made up the bay camera's 4 m to the tailpipe while it hears the bay up close), turned the dial
  upside down (distance 0 / 0.3 / 0.6 / 1.0: -29.0 / -23.7 / -22.4 / -20.0 LUFS: a camera walked back got louder), and,
  frozen where the car was when the camera was chosen, took a pass-by at 40 m as its distance (+37 dB, the limiter
  acting through 91 % of the pass). Weighting each source by the code's stated strengths (sEx, sIn, sMc) was worse
  (Side -9.6, Engine bay -14.6, Trackside -11.5): the strengths are not the signals' levels. A right form needs a
  measured level per source per build. Refuted, and not wanted: true levels (proposal 2) need no per-camera gain.

- The ground tap at gain 0.5 through 1.5 kHz: chosen for the notch depth wanted, built from its own target. Withdrawn.
- A 9 kHz pole at the pipe's mouth: no physical ground for it in the listener; the reference's own high cut is its dial's
  geometry and a stylised air. Withdrawn; the camera's spot is the lever.
- A higher target with a lower floor (0.5 and 3 Q): louder everywhere, the idle 3.6 LU over the reference, the spread
  0.4 LU. Refuted.
- The ground's coherence from the jet's extent (a corner at 0.44 c over the path difference's spread, 5 to 10 pipe
  diameters): withdrawn until its sources can be cited.

## Later: the Driver camera

Driver is 7 to 13 dB duller than the reference's Cabin above 4 kHz (and 4 LU louder). The cause is the cabin model: the
firewall's mass law (a one-pole at 800 Hz), the floor's 260 Hz band and the glass, where the reference's Cabin hears a
share of the bay unfiltered (0.06). A mass law alone overstates the loss high up, where a real cabin leaks through its
seals. The cure wants a measured cabin transfer, from sources, before any constant.

## The clips (shown in L's session, 27 Nov; 16-bit WAV)

Second set, first.json at Tailpipe, Field, asphalt; `idle-` (4 to 12 s of the warm idle), `steady-` (4 to 10 s of the
mid load), `pull-` (2 to 14 s of the paddle pull); "matched" means turned to the reference's integrated loudness:

- `*-1-reference`: the reference listener.
- `*-2-places-as-shipped`, `*-2-places-as-shipped-matched`: Places as on `main`.
- `*-3-PROBE-ground-coherence-matched`: the ground's coherence corner (withdrawn above; kept to be heard).
- `*-4-PROBE-auto-level-half-matched`: the half exposure.
- `*-5-PROBE-camera-distance-0-matched`: Tailpipe at the dial's 0.5 m (chest height).
- `*-6-PROBE-all-three-matched`: 3, 4 and 5 together.
- `steady-7-`, `pull-7-PROBE-camera-level-unmatched` / `-matched`: the level set by the camera (+9.7 dB).
- `steady-8-`, `pull-8-PROBE-camera-level-ground-coherence-distance-0-unmatched` / `-matched`: 7 with 3 and 5.

Staging set, unmatched, first.json and the truck at Tailpipe, steady and pull, of the refuted camera-position level
(at Tailpipe it is a fixed staging after the last stage's compressor): `staging-<build>-<scene>-1-reference`,
`-2-PROBE-camera-level-plus9.7dB`, `-3-...-plus6.7dB`, `-4-...-plus3.7dB`. first.json never reaches the limiter; the
truck sits on it at all three, as its reference does at 0.85.

Third set, unmatched, first.json and the 15 L truck, `idle` / `steady` / `pull`:

- `spot-<build>-<scene>-1-reference`, `-2-places-as-shipped`, `-3-PROBE-boundary-0.5m`, `-4-PROBE-onboard` (each with
  the auto level as shipped, so only the spot changes), `-5-PROBE-boundary-true-levels-plus9.7dB`.
- `level-<build>-<scene>-1-reference`, `-2-places-as-shipped`, `-3-PROBE-half`, `-4-PROBE-true-levels-plus9.7dB`,
  `-5-PROBE-true-levels-plus6.7dB` (at the shipped spot).
- `cameras-1-places-as-shipped-tailpipe-rear-trackside`, `cameras-2-PROBE-true-levels-plus9.7dB-tailpipe-rear-trackside`:
  a held mid load, the camera moved Tailpipe to Rear (at 4 s into the clip) to Trackside (at 7.5 s).

(A first set, `pull-` and `steady-` 1 to 3, made before the correction, is superseded by the second.)
