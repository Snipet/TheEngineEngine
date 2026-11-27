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

The two are independent: the spot cures the comb and the highs; the level is set by the owner's ruling (true levels,
one fixed staging). Both land together, the staging taken at the new spot.

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
  the pipe). The dial moves it straight back along the ground in proportion (0.5 m at the default dial's 1.71 m, so
  0.29 of the dial's distance; at least 0.25 m), with no comb at any distance; the level, the air and the room fall as
  now. The standing listener's spot with its comb stays where a real ear hears it: Rear
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
- The ground folded into each capsule's own path on the ground (found 27 Nov): the image had been heard at the pair's
  middle, which on a boundary microphone made a false notch from the far pipe (-6.6 dB at 2160 Hz, the Tailpipe case
  failing without the fold). The fold acts below 5 cm only: Rear, Side, Engine bay, Driver, Trackside, Chase and a Free
  camera 0.15 m up rendered with and without it are identical byte for byte. What it gives up: at the camera's 1 cm the
  true image path is 14 mm longer, a real notch near 12 kHz, which the fold drops; a real boundary microphone's capsule
  sits a millimetre or two over its plate (a notch near 60 kHz), so the fold is that microphone's physics.
- G's three caveats on what the spot hears (read from sim/exhaust/ends.fm, not measured): the jet's mixing noise is
  beamed by the listener's piston law like the monopole, where a real jet is loudest some 30 to 45 degrees off its
  axis; the open end's reflection is a fit good to ka = 2 (about 6.4 kHz for a 60 mm tail in hot gas), extrapolated
  above, which is the spot's top octave; and the monopole's near field below about 110 Hz at 0.5 m, which neither
  listener applies (the reference's nearest view is the same 0.5 m). A probe of the jet's share of the top octave is
  asked of G for the window after item 20.
- CPU nil.
- The onboard spot (the lead's prediction, about 350 Hz and 5 dB of ripple) measured 3.5 to 4.5 dB on four builds,
  11.6 on the bike (its pipe at 0.45 m).
- Null: model 0 untouched (scene.fm's placeListener is Places' alone); the null (`tee-tests "[null]"`) to be run.
- Bake: every baked table's key hashes the program's sources, so any `sim/` change stales them all; P re-bakes the
  whole set after it lands.
- Cases: "the engine's tilt through Places" (Tailpipe against Exhaust, 7/9 dB) and the defaults' level cases move:
  each re-measured and restated on its property, never loosened. New: Tailpipe's ripple on the single-pipe builds within
  2 dB of the reference, shown to bite on the shipped spot (9.2 dB).

### 2. True levels as the default, with one fixed staging (the owner's ruling, 27 Nov)

The owner, offered three defaults (as shipped: every build, camera and distance at about one loudness; true levels with
a fixed gain; half), ruled: **"Go with true levels with a fixed gain"**. It supersedes their 25 Nov ruling that the
idle's loudness stays as designed.

- The default: true levels (sim/spec/sound.fm autoLevel's default 1 -> 0), and ONE fixed staging gain after the last
  stage's compressor, where "True levels" is a gain of 1 today: the same for every build, camera and distance. No
  per-camera, per-build or per-source gain. "Auto" stays in the Listen view, unchanged, for whoever wants it.
- The staging is a level match at one named point, and says so in the code's comment: first.json at the default
  camera (the boundary microphone, proposal 1), its steady mid load (the dyno at 45 % of the redline, throttle 0.3,
  6 to 10 s), standing beside its reference. Without staging it sits +3.5 (idle), +3.7 (mid), +3.6 (pull) dB under: the
  three agree within 0.2, so the point hardly matters; the mid load's +3.7 dB (a gain of 1.531) is taken.
- Checked, not tuned, on the other four builds (Places minus the reference, idle / mid / pull; the idle-to-pull spread,
  the reference's in brackets; the limiter on the pull):

| build | idle | mid | pull | spread | limiter on the pull |
|---|---|---|---|---|---|
| first | +0.2 | +0.0 | +0.1 | 15.1 (15.2) | never |
| V8 | -0.4 | +0.0 | -4.3 | 6.0 (9.9) | 98 % of reads, 9.5 dB median |
| truck | -0.4 | -1.6 | -1.5 | 6.1 (7.3) | 100 %, 10.4 dB |
| bike | +2.4 | +0.5 | +0.5 | 12.4 (14.3) | 84 %, 3.5 dB |
| rotary | +7.9 | +5.2 | +4.1 | 13.2 (17.0) | 22 %, 2.4 dB |

- The cost said plainly: at the boundary spot the exhaust is nearer, so with a staging matched on first.json the loud
  builds' pulls drive the limiter some 10 dB deep (the V8, the truck), deeper than at the shipped spot (4 to 5 dB), and
  the V8 loses 4 dB of its spread. Their references sit on their own 0.85 limiter (model 0 reports no limiter's depth,
  so how deep is not known). A staging matched over the loud builds would leave first.json 3 to 4 dB under its
  reference.
- What the owner gives up, measured (true levels at the shipped spot, +9.7 dB Level; the boundary spot adds some 6 dB
  at Tailpipe and nothing at the other cameras): the cameras keep their true differences (first.json, mid, against
  Tailpipe: Rear -5.6, Side -4.1, Trackside -6.0, Chase -7.2, Engine bay +1.6, Driver -3.3; walking Tailpipe to Rear to
  Trackside on a held load, shipped stays within 1.4 LU, true levels puts Rear and Trackside about 4 LU under). The dial
  falls as the reference's does (distance 0 / 0.3 / 0.6 / 1.0: -23.7 / -26.9 / -33.0 / -44.6 LUFS). Trackside with the
  car far off is quiet: on a pass at a held speed and throttle (first.json in second, 21 to 23 m/s) each pass swells
  about 20 dB, from -53 LUFS at 30 m to -33 abreast and back to -53 at 44 m. The quietest reachable case: the kei 660
  at idle at the Rear camera with the dial at its farthest (30 m), -65.5 LUFS, against its reference's -57.1 and
  shipped's -45.2.
- The Level dial counts again (with the auto level on it was undone by the exposure); it acts before the compressor,
  as the reference's does.
- The limiter's depth against the yardstick's, measured by rendering each with its limiter and ceiling lifted (a probe
  on a scratch tree, the free output scaled under the resampler's clamp) and differencing the 10 ms peaks: on the V8's
  pull the reference is limited in every window, 4.1 dB in median (7.6 deepest), the patch 4.8 (9.7); on the truck's,
  the reference 7.6 (10.0), the patch 8.7 (11.1). The +3.7 staging goes 0.7 to 1.1 dB deeper than the reference does:
  faithful to it. (The telemetry's 9.5 and 10.4 dB are each 0.4 s read's deepest, not the median.)
- The rotary at +7.9 at idle: not the spot. Under true levels at the shipped spot it already sat 4.6 dB louder at idle,
  against its reference, than first.json does against its own; and its reference puts its idle 17.0 LU under its pull,
  where first.json's is 15.2. Places' computed exhaust loudness for a rotary (its pulses' strength against the V8's,
  loudnessOf) and the reference's own table for it disagree, and the reference's rotary idle is quiet. Not tuned:
  explained.
- The default's change: the spec's append-only rule is its layout (fields, order, enum values); a default changes on
  the owner's word, and this one has it. No shipped build or listener part states autoLevel (none to keep, none to
  remove). An owner's own saved build that states nothing follows the new default: intended. The manual's parameter
  page (generated by tee-schema from the Spec) is regenerated with the patch.
- Null: model 0 and the reference listener untouched. sound.level and sound.distance are read by the reference exactly
  as today (listen.fm block for model 0); the staging and the spot live in model 1's paths (finish1's autoLevel,
  scene.fm's placeListener). The null (`tee-tests "[null]"`) to be run with the patch.
- Bake: every baked table's key hashes the program's sources: P re-bakes after it lands.
- Cases restated on their new property, each shown to bite on today's code: "the level at the defaults" (a free rev
  exposed near its reference's level at the default camera, and each build's idle-to-pull spread within some LU of its
  reference's: bites on shipped's 10.0 against 15.2 on first.json, the V8's 1.1 against 9.9); "the auto level: cameras
  near and far" (on Auto, as now; and under true levels the far camera quieter by the geometry: bites on shipped, where
  they come together). The auto level's own cases (the start, the bangs, the dense burns) stay, run on the Auto setting.

## The evidence for item 20, to repeat in ten minutes

With the patch applied and the build made (`cmake --build build-dev`):

    build-dev/bin/tee-tests-listen "true levels at the defaults*,the Tailpipe camera*,the level falls 6 dB*,the reference's five views*"

- "true levels at the defaults": first.json idle peak -15.7 / -15.8 dBFS, the rev's typical peak -6.5 / -6.5 (reference
  / Places), the rise 9.2 / 9.3 dB; the V8's idle over first.json's +8.9 (reference) / +8.4 (Places). With
  `TEE_LISTEN_PROOF=1` (Auto, the old default) the V8's reads -1.0 and the case fails: it bites. (These are what the
  patch prints, the lead's build and L's alike; an earlier draft of this page gave -15.7, 9.2, +8.1 and -0.9, measured
  before the ground's fold was added, which moves what the Tailpipe camera hears by some 0.1 to 0.3 dB. The case's
  print says "under" where it means "over"; its figures are right.)
- "the Tailpipe camera, a boundary microphone": the least of asphalt over no ground, 300 Hz to 6 kHz, +5.6 dB. On the
  old spot (scene.fm before the patch) -9.2 dB at 620 Hz: it bites.
- "the level falls 6 dB a doubling": -4.54, -5.52, -5.74, -5.99 dB a doubling against the geometry's -4.45, -5.49,
  -5.84, -5.95.
- "the reference's five views are presets": 36 source-view pairs (Rear, Side, Engine, Cabin), 2.97 dB rms, net of the
  stated staging.
- The patch: `git apply docs/handoffs/patches/l-places-true-levels.patch`. Its whole default tier (27 Nov, 12:43 to 12:55
  UTC): 582 of 583 green, ref.null among them; the one red, host.sim.bakes, is the shipped V12's knock table no longer
  current on the changed program (the keys hash the sources), cleared by the bake that follows any sim/ item.

## Not proposed, and why

- Half (the auto level's gain following half the exposure's change, scaled to meet first.json's mid load): it restored
  each build's spread only in part (the V8 5.0 against 9.9) and left the V8's mid 7.9 LU under; the owner chose true
  levels.

- The level set by the camera's position (a gain from the camera's geometry, fixed while the camera is fixed): first.json
  met its reference (+0.2 / -0.3 / -0.1 LU, spread 14.9 against 15.2), but the tailpipe-only form was 9.7 dB too loud
  at Engine bay (it made up the bay camera's 4 m to the tailpipe while it hears the bay up close), turned the dial
  upside down (distance 0 / 0.3 / 0.6 / 1.0: -29.0 / -23.7 / -22.4 / -20.0 LUFS: a camera walked back got louder), and,
  frozen where the car was when the camera was chosen, took a pass-by at 40 m as its distance (+37 dB, the limiter
  acting through 91 % of the pass). Weighting each source by the code's stated strengths (sEx, sIn, sMc) was worse
  (Side -9.6, Engine bay -14.6, Trackside -11.5): the strengths are not the signals' levels. A right form needs a
  measured level per source per build. Refuted, and not wanted: true levels (the owner's ruling) need no per-camera gain.

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

The ruled design, unmatched: `demo-<first|truck>-<idle|steady|pull>-1-reference`, `-2-places-as-shipped`,
`-3-PATCH-true-levels-boundary-microphone`; `demo-cameras-1-places-as-shipped-tailpipe-rear-trackside` and
`demo-cameras-2-PATCH-...` (a held load, Tailpipe to Rear to Trackside). The second staging, the pull only, the big-block
V8, the truck and first.json: `staging2-<build>-pull-1-reference`, `-2-places-as-shipped`,
`-3-PATCH-boundary-staging-plus3.7dB`, `-4-PROBE-boundary-staging-0dB`. The patch carries +3.7 dB unless the owner says
otherwise after hearing them.

(A first set, `pull-` and `steady-` 1 to 3, made before the correction, is superseded by the second.)
