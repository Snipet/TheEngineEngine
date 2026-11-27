# X-065: the output limiter under true levels (package L)

The lead's ruling on X-065 (27 Nov, 19:01 UTC): the cure is an attack that does not cut the fronts. It is designed
now and built in the next window. Nothing in sim/ changes before then. This page records the evidence, the design,
its sources, and the predictions, written before any code.

## What was found (measured on frozen copies of the sim tree; 17 draws a row, 293.15 ± 0.15 K)

- **Old and new trees.** Old is the tree just before 2f6bbbeb; new is 3ba103f4.
  - The seven triple rows are alike on both trees (5 of 119 red on old, 6 of 119 on new). They are sometimes-rows
    of the click families already numbered, and they leave X-065.
  - The two archetype rows were made red by item 20. The rotary twin's tour went from 0 to 3 of 17, its median
    excess from 1.52 to 2.10 (bound 2.35). The supercharged V8's tour went from 0 to 3 of 17, its median from
    2.15 to 3.09 (bound 3.35).
- **Item 20's parts, each turned back alone** (count over 17, then median excess):

  | tree | rotary | V8 |
  |---|---|---|
  | old | 0, 1.52 | 0, 2.15 |
  | new | 3, 2.10 | 3, 3.09 |
  | true levels back to Auto | 0, 1.75 | 0, 2.63 |
  | the Tailpipe camera at its old spot | 2, 2.09 | 0, 2.28 |
  | no fold | 6, 2.20 | 0, 3.01 |
  | PROBE "quiet": new, staging × 0.125 (limiter out of play) | 0, 1.58 | 0, 2.63 |

  - Predictions for the quiet probe: 0 of 17 on both rows (right); rotary median about 1.75 (it came in lower, at
    1.58); V8 median about 2.6 (right).
  - The staging is a constant gain, and a constant gain cannot move a relative judge. It moves the judge only
    through the output limiter: take the limiter out of play and both rows pass, with new's spot and fold kept.
- **The limiter's depth on the tour while the rev limiter acts** (deepest / median):

  | tree | rotary | V8 |
  |---|---|---|
  | old | −1.3 / −0.2 dB | 0 / 0 |
  | new | −19.6 / −18.6 dB | −13.7 / −11.9 dB |

  The old auto level had ducked these builds 7 to 12 dB before the limiter. On first.json's tour the limiter does
  not act on either tree (deepest 0.0 dB).
- **The mechanism.** My first prediction was a gain step at each cut onset. It was wrong: at telemetry rate the
  limiter's gain is flat to within 1.1 dB.
  - Sample by sample, the judge's worst steps are the rotary's port-opening fronts, which rise from 0.5 to 6–8
    times the local RMS within two samples.
  - On new, each front meets the limiter's sample-instant attack (`lim *= 0.7 / pk`) and is flattened at 0.70.
  - 40 % of the new rotary's steps over 6 sit on clamped samples, against 4 % on old.
  - The worst step from 6.5 to 7.6 s: rotary 13.4 old, 17.8 new; V8 13.4 old, 17.3 new.
  - The hard knee adds the edge. It is a real change in the sound, not a judge artefact (the lead's ruling), so
    nothing is restated.

## The design: a look-ahead limiter in finish1 (model 1 only)

- **Delay.** Delay the signal after the compressor and staging by D = 2 ms (192 samples at the engine's 96 kHz) in a
  stereo ring.
- **Target.** On the undelayed signal, take the target gain t[n] = min(1, 0.7 / max|x| over the next D samples). A
  sliding-window maximum (a monotonic deque) does this in O(1) amortised per sample.
- **Attack.** The gain falls along a box average, over D, of the held minimum of t. Because of the delay, the gain
  has reached the needed value when the peak arrives at the output: no sample leaves above 0.7, as now, but the
  gain falls along a 2 ms ramp ahead of the front instead of at the front's own sample. This is the look-ahead
  limiter of Zölzer and of Giannoulis et al., cited below.
- **Unchanged.** The release (one pole up, 250 ms), the 0.75 soft ceiling and the staging of 1.531 stay as they are.
  Model 0's limiter is untouched and stays bit-exact.
- **Why 2 ms.**
  - The ramp's energy then lies below about 500 Hz (1 / 2 ms). The front's own content is far above that, so the
    front keeps its shape; the judge's step is a sample difference over a ±10 ms level.
  - Under about 0.5 ms the ramp moves into the band the step lives in.
  - Over about 5 ms a ramp starts to be heard as a duck before each pulse.
  - The build scans 1, 2 and 4 ms and keeps the shortest that meets prediction 1.

## Predictions (written before any code)

1. **The two rows.**
   - Rotary: 0 of 17, median 1.5 to 1.8 (quiet: 1.58).
   - V8: 0 of 17, median 2.5 to 2.8 (quiet: 2.63).
   - Two or more of 17 on either row means the design is wrong.
   - The seven triples are unchanged within their rates (each 0 to 2 of 17).
2. **The limiter's depth is unchanged within 1 dB.** Rotary about −18.6 dB median on the rev limiter, V8 about −12 dB.
   Look-ahead changes when the gain falls, not how far.
   - What a person should hear, comparing against the clips below (level-matched):
     - The hard, papery edge on each pulse goes. The pulses' attacks should sound like the quiet PROBE clip, not like
       the new tree's.
     - What stays is the density. On the limiter the rotary and the V8 are as loud as on the new tree now (RMS over
       the old tree: rotary +2.5 dB, V8 +6.7 dB, unmatched).
     - The pulses stand less far above the gaps than on the old tree. Crest factor: rotary 17.6 dB against 19.7 old;
       V8 13.3 dB against 17.5 old.
     - The fuel cut's stutter is flatter: more a wall than a stutter.
     - Bringing the stutter back is a question of level, not of this cure (the lead's ruling: that is the owner's
       question).
3. **first.json and the level match.**
   - The limiter does not act on first.json's tour (measured: 0.0 dB). Where no sample exceeds 0.7, the look-ahead
     limiter is a pure delay.
   - first.json's output is therefore new's delayed by 2 ms. The true-levels case (first.json against its reference
     within 2 dB at idle, mid load and rise) moves by 0.0 dB. The staging stays at 1.531.
4. **The delay.**
   - 2 ms in finish1, the last stage of model 1, so every camera has it, the Driver camera included, and the cameras
     stay in step with each other.
   - The reference (model 0) needs no matching delay. A change of sound.model starts the other listener from rest
     (listen.fm, `mWant`), and the two never sound together.
   - Against the picture, 2 ms is under one host block (10.7 ms at 512 / 48 kHz). It is far under the threshold of
     detectability for sound against picture (ITU-R BT.1359: about 45 ms with sound ahead, 125 ms with sound behind).
   - tel.sound.limiter will lead what is heard by 2 ms. This is documented in docs/models/listen.md.
5. **Effects on bakes and tests.**
   - Model 1's output moves on every build (by the delay, and where the limiter acts), so host.sim.bakes is red until
     the re-bake, as with any sim change.
   - Model 0 stays bit-exact.
   - The listen.sim tests that time events (the start case from 3 s) move by 2 ms and keep their margins.
6. **Cost.** A 192-sample stereo ring plus a deque: under 1 % of a core at 96 kHz, measured as the case's R.

## The question for the owner (not in this design)

A per-build ceiling that only falls would take the hot builds out of the limiter's depth. It is levelling by another
name and would change the owner's ruling ("true levels with a fixed gain"). It goes to the owner as a question with
the clips. It is not part of this cure.

## Clips (scratch-tree renders; 5.5 to 8.5 s of the rotary's tour, 5.5 to 9.0 s of the V8's)

They are in the scratchpad of session 01LFAGgSygRoYnUE3ciPH5pw, `clips7/`, in two forms:
- **Unmatched:** as rendered.
- **Matched:** RMS matched to the old tree's clip, peak kept under 0.98.

| clip | rotary | V8 |
|---|---|---|
| `1-old-tree` | Auto level, old spot: limiter at most −1.3 dB | Limiter idle |
| `2-new-tree` (item 20 as pushed) | Limiter −18.6 dB median, fronts clamped at 0.70 | Limiter −11.9 dB |
| `3-PROBE-quiet` (new with the staging × 0.125) | Limiter at most −1.6 dB | Limiter idle |

The quiet probe sounds like new without the limiter's knee, so it is the sound the cure aims at on the pulses'
attacks.

## Sources

- U. Zölzer (ed.), *DAFX: Digital Audio Effects*, 2nd ed., Wiley 2011, chapter 4 (nonlinear processing: the limiter
  with a look-ahead delay; peak measurement and the attack and release smoothing).
- D. Giannoulis, M. Massberg, J. D. Reiss, "Digital Dynamic Range Compressor Design—A Tutorial and Analysis",
  J. Audio Eng. Soc. 60(6), 2012, pp. 399–408 (an instant attack as a cause of distortion; smoothing the gain, not
  the signal).
- ITU-R BT.1359-1 (1998), relative timing of sound and vision for broadcasting (the detectability thresholds).
