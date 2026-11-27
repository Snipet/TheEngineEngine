# A bake that finishes while its owner edits (card C's design, 27 Nov; for the bake's window, no code yet)

**The case.** The owner edits while the calibration bakes; they do so all the time. Today (host/src/Bake.cpp,
CalibrationBakeService::consider, :1515-1560):
- an edit that moves neither table's key leaves the bake alone;
- an edit that moves a key cancels it, and a new bake of the edited build starts from nothing after `debounceSeconds`
  (1.5 s);
- each stale table is set aside at once: knockLimitAuto and camInAuto are set to 1, so the engine runs on its knock
  control and the auto cam until the new tables land.

On the owner's slow machine the spark map stood at 44 of its 80 points after 132 s, so a full spark-cam-spark bake is
minutes. An owner who nudges the compression every minute never sees one finish, and hears the engine change twice an
edit: once when the tables are set aside, once when they land.

**What moves what** (Bake.cpp:786-811 and :39-47):
- The spark key (knockLimitKey) holds the program's sources, kKnockBakeVersion, the engine's geometry, the fuel, the
  models, the valve's centres and ranges, every `spec.boost.*`, the ECU's boost, lambda, spark and cam maps, and the
  octane.
- The cam key (intakeCamKey) holds every `spec.valve.*`, `intake.*`, `exhaust.*`, `boost.*` and `engine.*` field, the
  fuel, and the spark's base key.
- So an edit that moves the spark moves the cam as well. An exhaust or intake edit moves the cam alone; an ECU spark,
  lambda or boost map moves both through the base key. No field moves the spark alone.

## The options, each with what it costs and what it risks

1. **Run on the last good tables while the new ones bake (keep, don't set aside).**
   - What changes: the stale tables stay applied, marked stale. The service still bakes, and swaps the new tables in
     when they land.
   - What it costs: small. consider() stops writing the Auto switches, and the status line says "playing on the tables
     from before your edit; baking".
   - What it risks:
     - A stale spark table after an edit that raises knock (more compression, more boost, less octane) under-retards.
       Knock control is on in every build (spec/ecu.fm:54) and hears it, as on a car run on a map made for other fuel:
       the engine knocks lightly and retards closed-loop until the new table lands.
     - A stale cam map is only less than best.
     - Neither is unsafe in the simulator. But what the owner hears between edits is the old build's calibration on
       the new hardware, and the status line must say so.
   - My recommendation: yes. It removes the double change of sound and the "never finished" feeling costs nothing.
2. **A debounce against the bake's length.**
   - What changes: the bake waits for the edits to rest. Not 1.5 s but a rest that grows with what a restart throws
     away: max(1.5 s, 5 % of the expected bake), the expected bake from the last one's points per second on this
     machine. On the owner's machine (about 0.33 points a second, some 200 points spark-cam-spark) that is about 30 s.
   - What it costs: small. The service keeps a points-per-second figure.
   - What it risks: a bake starts later after the last edit. With option 1 the engine plays on the old tables
     meanwhile, so the wait is quiet.
   - My recommendation: yes, with 1.
3. **Reuse an unchanged stage (keyed stages).** The cache already does this, by key: an edit undone, or a value
   tried before, is taken from it at once (the app's bakes folder, app/Offline.cpp:195). Across a new value nothing is
   reusable, because the cam key holds the spark's base key, and every spark field is in the cam's key.
   - The one honest case is an exhaust or intake edit: it moves the cam only, so the spark map is reusable. But the
     spark's key holds the cam maps (knockLimitKey with them), so after a new cam is baked the spark map is stale again
     and "spark again" runs anyway. What is saved is the first spark stage: about a third of the bake. Already true
     today: the first spark stage is current and skipped.
   - My recommendation: nothing new to build. Say it in the status ("the spark map kept; baking the cam").
4. **Reuse a spark map's untouched cells.**
   - What changes: after an edit, re-measure only the cells the edit moves.
   - What it risks: a wrong table. No field I know moves only some cells. Compression, boost, octane, cams and
     geometry move every cell's knock limit through the end gas's temperature and pressure (combustion/knock.fm), and
     a rule that guessed which cells were untouched would be one a reading could not check.
   - My recommendation: no.
5. **A coarse table first, refined after (as the predictor's curve is).**
   - What changes: the spark bake measures a coarse grid (2 speeds × the full-load pedal and boost steps) first, applies
     it, then measures the rest.
   - What it costs: a stage and a "coarse" state for the interface.
   - What it risks: a coarse table is a real table between its cells, interpolated across 3000 rpm. The owner hears a
     calibration that is neither the old nor the final one, and it moves again. With option 1 the old tables already
     cover the wait.
   - My recommendation: not now. Measure first how far a coarse table is from the final one on the 28 builds, if 1
     and 2 are not enough.
6. **Warm-start the new bake from the old table.**
   - What changes: the corrections pass starts from the stale table's cells, not from none.
   - What it costs: one option in bakeKnockLimit.
   - What it risks: it saves only the corrections (at most two passes, re-measuring the cells over correctAbove), not
     the grid, and a correction converged from a stale start must still be measured. A little speed, some risk of a
     table that depends on the edit history (the same build baked two ways reads two tables). That is against "a
     table is its key's".
   - My recommendation: no.

## What the interface shows (U1's, proposed)

- While it waits for the edits to rest: "your edit will be baked once you pause (N s)".
- While it bakes: "baking the spark map 12 / 80 (playing on the tables from before your edit)". The progress counter
  restarts on a new edit, and the words say why: "restarted: compression changed".
- When a stage is kept: "spark map kept (unchanged by this edit); baking the cam".
- When the new tables land: "calibration baked", as today.

## What I would build, if the owner chooses

1 and 2 together:
- consider() keeps the stale tables applied and marked stale;
- the debounce is the larger of 1.5 s and 5 % of the expected bake (that machine's measured rate);
- the status line carries the words above.

The tests:
- an edit mid-bake leaves both Auto switches at 0 and the old values in place;
- three edits a second apart start one bake, not three;
- a stale table is replaced by the new one when it lands.

No table changes, so the bake's own proofs (three builds byte for byte) carry over. Option 5 only after a measurement
says 1 and 2 are not enough. Options 4 and 6 not at all.

## As built (27 Nov, the owner's yes to 1 and 2; host/src/Bake.cpp, CalibrationBakeService)

- **What may play stale (provenance, no flag from the interface).** A stale table keeps playing only if this service
  saw those exact values current on this program (consider() with the whole build current, or takeAndApply landing
  them), and the edits since have not made it unfit (`unfitBy`, the list in Bake.h). A loaded build's tables (a
  shipped build after a simulator change, a file baked elsewhere) were never seen current here, so they are set aside
  as before, with the reason "not seen current here". Undo puts the kept tables back under their own keys: current at
  once, nothing marked stale.
- **A restart of the application** remembers nothing of this: the record of what was seen current lives in the
  service, in memory. A build left stale when the owner quit is set aside on the next launch ("not seen current
  here") and baked as before. A build left current is current at once, and is the new baseline.
- **The bake measures what it measured before:** its own copy of the build has every stale table set aside, whatever
  plays live. Shown byte for byte on three shipped builds, edited, baked through the service with their old tables
  playing, against cams.py's bake of the same edited file.
- **The wait:** max(debounceSeconds 1.5 s, debounceShare 5 % × the expected bake's points × this machine's seconds a
  point), at most debounceMostSeconds 15 s. The rate is measured by each bake that finishes; none yet, the floor. Each
  edit starts it again. A load (nothing plays meanwhile) waits the floor.
- **BakeStatus**, plain fields for U1: which table plays stale; which was set aside and by which field; the wait and
  its seconds left; the stage and its points; a restart and the field that caused it; stages kept; bakes started; the
  rate.
- **The predicted curve** runs on the build as it stands: on the stale tables while they play. Whether it is marked is
  U1's call from the stale flags (U1: a quiet note under it, not a warning).
