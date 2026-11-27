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
- Cost: the rotary's top octave 5.8 dB dull (its large bore's piston law, 34 degrees off the axis). CPU nil.
- The onboard spot (the lead's prediction, about 350 Hz and 5 dB of ripple) measured 3.5 to 4.5 dB on four builds,
  11.6 on the bike (its pipe at 0.45 m).
- Null: model 0 untouched (scene.fm's placeListener is Places' alone); the null (`tee-tests "[null]"`) to be run.
- Bake: every baked table's key hashes the program's sources, so any `sim/` change stales them all; P re-bakes the
  whole set after it lands.
- Cases: "the engine's tilt through Places" (Tailpipe against Exhaust, 7/9 dB) and the defaults' level cases move:
  each re-measured and restated on its property, never loosened. New: Tailpipe's ripple on the single-pipe builds within
  2 dB of the reference, shown to bite on the shipped spot (9.2 dB).

### 2. The level set by the camera's position, not by the signal (the owner's choice)

The auto level does two jobs at once: it brings near and far cameras together, and it brings every build to one
loudness. The second is the complaint. The candidate that separates them: the auto level off, a gain the camera's place
sets once (fixed while the camera is fixed), the limiter for the peaks. Measured as a probe (the gain from the camera's
direct distance to the tailpipe, normalised to Tailpipe at the default dial, staged +9.7 dB, a level match chosen on
first.json's mid load, nothing more):

| build | idle | mid | pull | spread |
|---|---|---|---|---|
| first | +0.2 | -0.3 | -0.1 | 14.9 (15.2) |
| V8 | +2.7 | -1.9 | -1.5 | 5.7 (9.9) |
| truck | +0.1 | -1.5 | -1.2 | 6.0 (7.3) |
| bike | +7.0 | +1.9 | +1.7 | 8.9 (14.3) |
| rotary | +4.9 | +0.3 | +3.2 | 15.3 (17.0) |

- What it costs: the loud builds sit on the limiter as their references do on theirs (the truck's pull: acting in 98 to
  100 % of reads, 10.5 dB median at +9.7, 4.5 at +3.7); that is what eats their spread.
- Near against far (first.json, mid, against Tailpipe): Rear +1.2, Side -0.3, Trackside +2.5, Chase +1.7, Driver -2.1,
  **Engine bay +9.7**: the gain makes up the bay camera's 4 m to the tailpipe while it hears the bay up close.
- Weighting each source by the code's stated strengths (sEx, sIn, sMc) was tried and is refuted (Side -9.6, Engine bay
  -14.6): the strengths are not the sources' signal levels. A right gain needs each source's level against the
  exhaust's, measured per build at staging, as the exhaust's own loudness is computed (loudnessOf); that is design
  work, not a constant.
- The dial: the gain more than undoes it (Tailpipe at distance 0 / 0.3 / 0.6 / 1.0: -29.0 / -23.7 / -22.4 / -20.0
  LUFS): a camera walked back gets louder, the room's share growing while the gain makes up the full 1/r. The owner
  should know it before choosing it; a cure takes the room's share into the gain.
- Trackside: a gain frozen where the car is when the camera is chosen fails (+37 dB at 40 m; the limiter takes the
  pass); the camera's nominal geometry (the closest approach) is the right distance. A pass at a held speed and throttle
  is still to be measured.
- The owner chooses among three, with clips: as shipped (every build one loudness), half (the gain following half of the
  exposure's change: spread restored on all five, the V8's mid -7.9), and by position. The owner ruled on 25 Nov that
  the idle's loudness stays "as designed"; this reopens it on new evidence.
- Null and bake: as in 1. Cases: "the auto level: cameras near and far brought together" and "the level at the
  defaults" bound today's design and would be restated on the new property (the idle-to-pull spread within some LU of
  the reference's, shown to bite on shipped: 10.0 against 15.2 on first.json).

## Not proposed, and why

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

Staging set, unmatched (the level is the question), first.json and the truck at Tailpipe, steady and pull:
`staging-<build>-<scene>-1-reference`, `-2-PROBE-camera-level-plus9.7dB`, `-3-...-plus6.7dB`, `-4-...-plus3.7dB`.
first.json never reaches the limiter; the truck sits on it at all three, as its reference does at 0.85.

(A first set, `pull-` and `steady-` 1 to 3, made before the correction, is superseded by the second.)
