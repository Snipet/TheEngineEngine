# The turbine match's bake stage: C's design and its tests' shape (26 Nov, before any host code)

Card C's task #39, item 5 of the window opened 26 Nov 04:36 UTC (with N2, step A: `docs/handoffs/patches/n2-match-a.patch`
on 9c5067bc, five commits). Written on paper first: nothing under host/ is pushed until the lead says a bake can follow.
What it builds on: N2's design (turbine-match-design.md, choice (i)(a): match, run a short rated point, re-match), N2's
measured passes (turbine-match-step-a.md), and the eight points N2 and C agreed on 26 Nov 00:11 UTC. "The spark bake" and
"the cam bake" are the two stages host/src/Bake.cpp has today (CalibrationBakeService).

## What the stage does

One stage, `match`, added to CalibrationBakeService in front of the two it runs today.

**The order.**
- A boosted petrol build: match, spark, cam, spark.
- A diesel (no knock-limit table): match, cam.
- A build without a turbine: no match stage. A supercharger is not matched. A build whose program has no
  `spec.boost.ratedP4` skips it too.
- The reason for match first: the match moves the boost at rated, and the boost moves the knock limit. So the spark
  table is baked on the matched machine, never the reverse.

**One pass:**
1. Measure: the predictor at one explicit speed (PredictOptions::speeds, a single rated speed), pedal 1. The hold is
   long enough for the telemetry's 1 s means, so holdSeconds is 2 (both the means and the 0.1 s air are whole there).
   The fields read at the point's end:
   - `tel.boost.turbineOut[i]`: p4, Pa absolute, a 1 s mean;
   - `tel.boost.turbineInTemp[i]`: T3, K;
   - `tel.boost.efficiency[i]`: the compressor's efficiency at its operating point;
   - `tel.boost.engineAir`: kg/s, a 0.1 s mean.
   Which turbines feed the one rated pair, by `spec.boost.layout` (N2, 04:40):
   - one turbine: it;
   - twin and quad (a bank each, four two a bank): the mean of the turbines that are on. They are symmetric and share
     one rated pair;
   - sequential: the primary alone. Its housing is upstream of the transfer valve, and the secondary's sees the valve's
     drop, so a mean would under-read the primary's p4 and T3. Suspected, not measured: N2 checks it on the rotary's
     pair when step A runs;
   - compound: the stage writes nothing and says so (its result: "a compound pair has no one rated state"). Its
     stages' p4 differ by the whole high-stage expansion. The build stays on the stand-ins, bit 32 set.
2. Write: `spec.boost.ratedP4`, `ratedT3`, `ratedEtaC` and `ratedAir` from that point, and then `matchKey`. Writing them
   raises the boost section's rev, so the program re-matches on its next block.
3. Fail the stage (an error naming the reason; the build left on the stand-ins, mismatch bit 32 still set) when the
   point is any of these:
   - limited (FLAG_LIMITER);
   - not settled;
   - approachForced (the approach could not release the latch);
   - boostHunting;
   - faulted.
   None of these is a rated state. A stage that fails leaves the spark and cam stages to run on the stand-ins, as today.

**The passes and when they stop.**
- Pass 1 measures on the stand-ins (whatever the build holds, 0 = not baked) and writes the four fields. Pass 2 measures
  on the pass-1 match and writes again, and so on.
- The stop: every one of the four within its floor of the pass before:
  - p4 within 0.5 %;
  - T3 within 5 K;
  - the efficiency within 0.01;
  - the air within the point's own scatter.
  These are N2's 00:11 numbers. N2 measures the scatter next and may move them; the code reads them from options.
- The torque at the point is the cross-check, not the stop: within twice its torqueSe (the settled cycles' standard
  error, Predict.cpp) of the pass before, else the stage says so in its result (not converged) and still writes the
  last pass.
- At most 4 passes.
- The slow converger: N2's 2.0 moved 6.4 % at pass 2 and 2.0 % at pass 3, its T3 rising with the area. So from pass 3
  on, each written value is damped: x(n+1) = x(n) + 0.5 · (measured − x(n)). Passes 1 and 2 write what they measure.
  The tractor, the truck's sets and the pickup stopped at pass 2 within 0.2 %, and the damping never reaches them.
- Cost: a pass is one predicted point, 2 s of hold plus the approach, about 3 to 5 s. So the stage costs 6 to 20 s
  before the spark stage's minutes.

**The rated speed: the match's own design speed.** No single field holds it today (N2, 26 Nov 04:40). The rule is
the program's, in `sim/boost/machine.fm` (nDesign, ~464), fed by `sim/engine.fm` (~526, boost.designSpeed):
- spark ignition: `spec.boost.designShare` × the redline (`tel.engine.redline`, rad/s; designShare 0.85 by default);
- a diesel (the engine's fuelUse not 0): `spec.ecu.ratedSpeed` (rad/s) when above 0, else the redline.
The stage copies that rule exactly, with a comment pointing at both lines, until N2's proposed `tel.boost.designRpm`
(the machine's resolved nDesign) lands when sim/ opens. Then the stage reads that field and the copy goes, so the rule
lives in one place. The first sketch's "redline less 300 rpm" was wrong for both kinds and is not used. N2's hand-picked
cells (2.0 4000, pickup 2800, truck's sets 2100, tractor 1900) are the rule's speeds on those builds; the nightly test
checks that the stage's speed is each of them.

## The key, the cache, the reuse

**`matchKey`** (an int, unit "hash", the knock and cam keys' form: FNV-1a, 31 bits):
- Hashed: `Sim::sourceHash`, `kMatchBakeVersion` (1 at first), and every spec field that moves the match.
- Left out:
  - the four rated fields and matchKey itself (a key may not hash what it writes);
  - every baked table and its Auto switch and key: the knock-limit table, `camIn` and `camEx`, as knockLimitBaseKey
    leaves them out. So the spark and cam stages after it do not make the match stale. N2's point (1).
- What moves the match: today's `movesKnock` set less the ecu's maps, plus the whole boost section. Written as its own
  `movesMatch(FieldInfo)` beside movesKnock and movesCam, so a later field is placed once.
- A 0 field is left out as zeroOptional leaves it today, so a build that never had the field keys as it did.

**The spark key moves with the match.** The four rated fields are spec fields that move the boost, so they are in the
knock key's set as written. A new match makes the spark table stale, and the order above re-bakes it. This needs no
special case, and the test below asserts it.

**`matchCurrent(build, sim)`:** matchKey equals the key, and ratedP4 and ratedT3 are both above 0. There is no Auto
switch: the match has no hand-set form to keep. A person who wants the stand-ins sets the four to 0, as the patch
reads it.

**The cache.**
- The entry: `match-<key>.json`, the four values and the pass count, the form spark and cam entries have today.
- The program-free key `matchFieldsKey(build)`: the same fields without the program's hash, for the verified reuse.
- The check set for a candidate baked under another program: one predicted point at the rated speed with the four
  applied. p4 within 0.5 %, T3 within 5 K and the torque within twice its torqueSe of the stored ones (N2's point (5)).
  Otherwise the stage is baked.
- One point, so the check is 3 to 5 s against the stage's 6 to 20.

**Versions.** `kMatchBakeVersion` 1. The spark's kKnockBakeVersion does not change for this: the rated fields enter its
key as fields, not as a rule.

## What it costs elsewhere

- **The progress:** BakeProgress gets the stage "match", with passes done and passes most (4) as its points.
- **The interface:** knockBakeStateName needs nothing new. The stage name shows through the existing stage() call.
  U1 learns of "match" by the progress record, as of "cam" today.
- **tee-dyno:** `--bake-match` (the stage alone, as `--bake-spark` and `--bake-cam`), printing each pass's four values,
  the torque and its torqueSe.
- **docs/formats.md:** the cache entry `match-<key>.json`, and the key's field set in the bake section.

## The tests' shape (host, written with the code, not before)

The default tier (each case under its 30 s; the fast tier's cap counted):

1. **"the match key moves with a field that moves the match and not with what it writes"** (no simulation, under 1 s
   beyond the shared compile):
   - a boost field edited moves it; an engine field (bore) moves it;
   - the four rated fields, matchKey, the knock table and its Auto, and camIn and its Auto do not;
   - a build without the boost section keys as before.
   - Grouped with the octane-keys case (one build, one compile).
2. **"the match stage on the turbo four: pass 1 writes the four, mismatch bit 32 clears, current until a boost edit"**
   (one boosted build, 2 passes, about 10 s):
   - each value above 0;
   - `tel.boost.mismatch & 32` clear after the apply;
   - matchCurrent, then stale after a boost edit, then current again after undo;
   - the spark key changed by the apply.
   Grouped onto the background bake's case: it already bakes the turbo four, and the match runs in front of its spark
   stage there. This adds the stage's seconds, not a build's.
3. **"a point the match cannot use fails the stage and leaves the stand-ins"** (planted: a rated speed over the
   redline, so the point is limited):
   - the stage's error names "limited";
   - the four still 0, bit 32 still set;
   - the spark stage still runs after it.
   1 point, about 4 s.

The nightly tier:

4. **"the match converges on N2's four builds and the rated rows stay within 1 %"**: the 2.0, the pickup, the truck's
   sets and the tractor:
   - the stop reached within 4 passes;
   - the written ratedP4 and ratedT3 within 1 % and 10 K of N2's pass 3 (turbine-match-step-a.md);
   - the rated torque within 1 % of today's.
   Minutes, so nightly. Its default guard is case 2.
5. **"the match's cache across launches and its verified reuse"**: the same build baked twice. The second is taken by
   key with no measure. A planted program change (PlantedSim, as the spark's reuse test plants one) is checked and
   taken. A planted change that moves p4 is baked again. Its default guard is the default tier's
   "the calibration bake's cache …" case, which covers the shared cache code.
6. **"a stage that converges slowly is damped"**: the 2.0 at its rated cell, pass 3 onward written as
   x + 0.5 · (measured − x), stopping by pass 4. Its default guard is case 2, the pass loop.

Each new case's bound gets its measured number and its reason in its comment, and each is shown to fail once by hand
(the damping off, the key hashing a rated field, the fail rule off).

## Answered by N2 (26 Nov 04:40) and still open

- **Answered:** the design speed (the rule above, copied until `tel.boost.designRpm`); two turbines (the mean, the
  primary alone for a sequential pair, nothing for a compound); the damping from pass 3 and the 4 passes; the spark key
  moving with the rated fields.
- **Open:** the reuse floors and the stop (0.5 % p4, 5 K T3, 0.01 efficiency, the air's scatter) stay options until
  N2's scatter measure, N2's next window. The sequential pair's primary-alone rule is to be checked by N2 on the rotary.
- **Tests added by the answers:** case 3 also plants a compound layout (the stage writes nothing, says why); the nightly
  case 4 asserts the stage's speed on each of N2's four builds.
