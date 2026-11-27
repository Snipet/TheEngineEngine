# X: the whole program under test (handoff, 2026-11-25 23:50 UTC)

Package X, branch `claude/awesome-gates-ntdb58`, cloud session. Owns `tests/sim/combo/**`, `content/builds/combo/**`,
`content/scenarios/combo/**`, `tools/tee-fuzz*`, `scripts/listening-pack.*`, `docs/checkpoints/M2.md`, this file.

**Done, by test** (`tee-tests-combo`; ctest `-R '^combo\.'`; the fast tier is combo.fast-1 to fast-3 (combo's CI share: 165 s scaled) and combo.fast-tags; 84 default cases, each under 30 s; 32 `[nightly]`). The
harness (`tests/sim/combo/Combo.h`) finds the switches in the Schema (every `spec.<section>.model`, `engine.custom`,
`engine.wallHeat`: 11 now), so a package's switch is covered the day it lands. Judged in every run: finite, within
full scale, no guard trip nor FLAG_FAULT, DC under 0.005 FS, no click, starts, idles 450 to 1800 rpm, nears its
redline, moves, stops; every telemetry column finite, in its declared range, else physical by its quantity.
A click (the lead's ruling, 25 Nov 17:24) is a step its neighbouring firing periods lack: where the engine fires, the
worst step's excess over the median of the same measure in the periods t +- kT (k = 1-4) against twice the reference's
own (a bang: the largest of three reference copies with pops on); where no period exists, the step over the local
level against twice the reference's step. Both are printed in a click's detail. Proof it bites: X-015 and X-014 on
their old tree (62a6a7fd) 7.33/3.19, 4.07/3.64 (its weakest), 7.77/4.60. An upshift's cut with ecu.model 1 under
fuel.model 0 is bounded at a step of 42 (the lead's ruling, 24 Nov 23:15; "click-shift-cut"). A click row is a SOMETIMES row
(the lead, 26 Nov 07:30): one run is one draw (a millikelvin of air moves a row's excess by 2), so Known.h gives each
row n of 17 copies over at 293.15 +- 0.15 K, and a cure is accepted only as 0 of 17 (tests/sim/combo/spread.sh, in no
tier). The nightly's bound is twice the reference's LOWEST excess over 5 fixed copies (Combo.h kBoundNudges; never above
the default tier's single draw); it brought in X-048 and X-049 and two pull rows of X-043 (the turbo six's pull bound
3.37 to 2.67). Next (the lead's order): per-event windows (part B; the rows "at its shifts" may then split), then the
judge at a cut (part C, measured first behind a local variant).
- archetypes (`content/builds/combo/`: small four, V8, turbo six, large diesel, turbo V8 dual) through the tour with
  nothing and everything on; nightly: through all four scenarios (tour, pull, launch, cold; `content/scenarios/combo/`)
- every switch alone (V8 tour, six pull); nightly: every pair, every triple (16 ctest shards by subset index mod 16;
  the soak 66 min on CI's 4 cores with everything in, every shard every night)
- every switch flipped under load, singly and all in one block; every choice of 10 Spec sections stepped while running;
  builds loaded over each other, with nothing and everything on
- targeted: turbo with both ECUs (anti-lag, nitrous switched), launch control, dog box, traction on wet (standstill,
  rolling), the four limiter kinds, the engine dyno; the 30-build archetype corpus (6 by default, all nightly)
- fuzz: random and corner Specs (2 seeds by default with bounds checks; 1-6 and 100-299 nightly without, in 4 ctest
  shards of 50 seeds); a seed draws each field from the seed and its path, so it names the same Spec across commits;
  nothing allocates
  in process with every switch on (operator new; not on Windows); the 10-minute soak (nightly) and an hour by hand
- decisions (`DecisionsTest.cpp`, fast): each switch written while stopped (D's stopped-switch sequence) reaches the
  engine the build with it on reaches; every switch loaded over every switch off reaches the fresh build; on the
  default block (nightly: V8, turbo six). the routes agree before f89d40c too (444 N m), so the size line holds the fix: with fuel.model or wallHeat on,
  the default block's torque within 3 % of the line between 5.0 and 6.2 L (old tree -17 %, tip -0.3 %)
- decisions, field by field (fast-1, 13 s for the 11 switches): each switch written while stopped against the program
  initialised over the build (#337's reference, the integrator's FieldDiff.h), every field that holds still before the
  key; the leftovers named with their reasons (revision counts, a configure's draws, its pipes' delay history). Bites on
  393519ea's parent (engine.custom: customUse, layoutWhy, ecu.redRpm, the redline, dyno.j; boost and exhaust:
  exh.b.eng.tPort); X-045 (the exhaust built at the key) under its own check. The guard of each switch's 12 s runs on
  the default block, nightly since the bake reached main (07a08dc0; fast-3 77 s to 22 s here)
- X-020 closed (N2 896c4b3) and out of Known.h; X-021 closed by the build: large-diesel.json carries D2's truck boost
  (gateSpring 220000, icFlow 0.8, chargeVolume 0.02): boost.model on, 2000 rpm, 1865 N m against the reference's 1933
- fixed findings held as tests: X-001, X-003, X-007 (`[fixed]`)
- `tee-fuzz`: `--seeds`, `--reduce` (ddmin), `--play <archetype|build.json> <scenario> <switches> [--set path=value]`
  (prints the step and the excess against the reference's), `--checks`, `--match`

**Findings** (X-001 to X-053; `tests/sim/combo/Known.h` holds the open ones with owner and reason; a closed one leaves
it so it fails again if it returns). Open: X-016 (the heavy case's cost, Q), X-030 (the new ECU's shift cut under
fuel.model 1: a firing unlike its neighbours as the spark returns; the steward), X-044 (the small four's downshift
with engine.custom, fuel and ecu: the fuel returning whole after the overrun cut; the steward's, kept this window), X-045 (boost.model or exhaust.model written while stopped, and the application's load: the
last exhaust reported until the key; the integrator, the next window), X-046 (env.pressure acts on the car's drag only, the engine breathes
101325 Pa; the integrator), X-047 (the new ECU's shift cut re-armed while the vehicle asks; a fault, the steward),
X-048 (the turbo six's pull at its shifts with boost.model on; N2, N1 on the valve rows), X-049 (valve, exhaust and
sound on the turbo six's pull; N1 and G), X-036 (a DC with
car.model and a turbo: the turbo V8 dual, random 103, and the large diesel on the dyno with everything on; D, the
steward and the integrator), X-039 (a supercharged 50 cc single with a recirculating blow-off, N2: cured in its held
package), X-041 (the turbo six's pull lift with valve, boost and ecu: the blow-off opening at a compressor ratio of 1.94
since G's 5941904e spooled the turbo harder; N2, the next window; its other three rows retired by 5941904e), X-042 (the large diesel's dpWave under
engine.custom, valve and boost: one collector feeds one of its two turbos; N2 with the integrator), X-043 (the turbo
six's start with intake, exhaust and car on: the reference idle's hunt on the fixture's cam lope; kept for good by
the lead's ruling, a pairing nothing ships). Closed on 26 Nov: X-031
(the V8's downshift, by the steward's 98d0dc4d; triple shards 3 and 4 pass here). Closed on the
bake's soak (223f4301, 213 of 221): X-029, X-038, X-040 (G's and N1's bore floors), X-032 (excess 1.84), X-037 (N1's
b07522fc). The turbo six's remaining hunt with intake.model on (intake+ecu idle sd 227 rpm, 307 with every switch off)
is its cam's own lope (cam dial 0.9999, a dilution recursion near 0.4 s; the steward, 25 Nov), not the idle valve's:
nobody chases it as X-032 again. Closed on 25 Nov before it: X-027 (a bang, under the ruled 42), X-028 (D's 1842d7cd: voices leave the engine
bit-equal), X-033, X-034, X-035 (the large diesel's firing train, not a click, under the restated check). The reduced
builds: `content/builds/combo/findings/`.

**Measured** (this container, shared 4 cores; R from the same process; docs/measurements/combo/2026-11-24.md): tour,
off / every switch on: small four 0.52 / 0.98 R; V8 1.00 / 1.57; turbo six 0.80 / 1.32; large diesel 0.77 / 1.19; the
typical turbo V8 dual 1.04 / 1.81 R, 39 % of a core at 48 kHz/512 (under 60 %, over the contract's 1.6 R). Corpus as
built 0.59 to 1.81 R. Soak: 600 s of everything on, 236 s wall, 0 failures, 1.71 R. Default tier: 510 s at -j3 here.

**The listening pack** (M2): `scripts/listening-pack.sh [--quick]` and `.ps1` over `scripts/listening-pack.tsv` (41
groups, an optional define column); `--quick` rendered 10 files in 90 s here. `docs/checkpoints/M2.md`: what to
hear in each group and the yes-or-no lines.

**Not run**: the .ps1 (no Windows here), the whole pack (only --quick), the hour soak; nothing
on macOS or Windows by me (CI only). Nobody has listened to anything. **Outside my paths**: none. **FMinus**: no issue.

**Moving the fixtures out of content/** (the lead's ruling, 25 Nov 15:09; one coordinated step after P's bake, the
lead calls the order). `content/builds/combo/findings/` (x001 to x040), `pack/` and the five combo archetypes go to
`tests/sim/combo/builds/`. U5's GarageLibrary already lists nothing under `content/builds/combo/`. Every path that
reads them today (grep of the tip at 15:08):
- mine: `tests/sim/combo/Combo.cpp` (buildPath: `content/builds/combo/<name>.json`), `DecisionsTest.cpp` (buildWith),
  `tools/tee-fuzz/main.cc` (--play archetypes), `scripts/listening-pack.tsv` (the pack's and archetypes' builds);
- N1: `tests/sim/calibration/na/BoostInletTest.cpp:86, 92-93` (x022, x024, x026-redline, x026-single-big-blower,
  x026-single-small-pipe, x026-tiny-engine-supercharger, x026-seed298-leak-back), `Probe.cpp:516, 782, 841`,
  `X022Test.cpp:32` (x022, x026-single-big-blower);
- the integrator: `tests/sim/core/EngineTest.cpp:566` (Bench("combo/findings/x025-two-stroke-rotary-silent"));
- P: `docs/manual/builds.md:149-161` (the manual's table of content builds lists x001 to x026).

**26 Nov, the combo builds' cams** (the lead's ruling, 11:46 and 13:14). valve.model Lab never reads the reference's cam
dial (X-050), so on the combo builds (dial 0.9999, a 300 degree event) every valve row compared the lab's default 231 degree
cam with the dial's, not a model with a model. The five builds now state lab cams equal to their dials by N1's lift-matched
law (seat 276.8, 1 mm 254.2, centres 106: the reference's 1.5 % and 10 % lift points); with valve.model off each renders
bit-identical, with it on its events read 104.03 / 615.97 against the reference's 104 / 616. A fair twin,
turbo-six-mild (dial 0.0921053, a 231 degree event; lab seat 213.1, 1 mm 195.7, centres 111.447), judges the mild cam model
against model, nightly only. The turbo six's valve rows, 17 copies at 293.15 +- 0.15 K, copies over the nightly bound,
dial build -> lift-matched -> the twin: valve 0 -> 0 -> 0; valve+sound 0 -> 0 -> 0; valve+exhaust+sound (X-049) 2 -> 0
-> 0; valve+boost (X-048) 4 -> 2 -> 0; wallHeat+valve+boost (X-048) 14 -> 0 -> 2; valve+boost+fuel (X-048) 8 -> 0 -> 3;
the tour's valve+boost+ecu (X-041's lift) 3 -> 0 -> 0. So X-049's row and X-041's tour lift left Known.h (they measured
the cam swap); X-048's valve+boost rows stay on the lift-matched turbo six (2 of 17, the model's); its wallHeat and fuel
rows moved to the twin (2 and 3 of 17). X-049's part (1), the reference pinned on its limiter, stays a question in part C.
The twin's own nightly (26 Nov) gave the corpus eight rows a wild cam did not show (X-044's 3-2 downshift on two switch
sets, X-030's 1-2 upshift on two, X-048's boost rows at the 1-2 and 2-3 upshifts on four; most 1 of 17). A row at 1 of 17
on one build is the weakest kind of entry there is, kept because the rule is that none is waved off. The twin costs about
7 minutes of the local nightly (3630 s against 3187 s), all in its triples; if the CI soak's share passes 10 minutes, its
triples stay in the nightly and come out of the soak (the lead's ruling, 14:25).

**26 Nov, shipped builds on the pull** (the lead's ruling, 15:00). The corpus's tour read clean; the pull is where the
upshifts are. Three of the corpus as built (the two boosted small builds and a boxer four), each against its own
every-switch-off reference's nightly bound, 17 copies at 293.15 +- 0.15 K per event, on 8ec06fd1: kei-660-triple-turbo
(bound 2.77) over at its 1-2 upshift in 2 of 17; three-cylinder-10-turbo (2.84) at its 1-2 in 6 of 17 and its 2-3 in 8
of 17; boxer-four-turbo-unequal (3.52) 0 of 17 everywhere; nothing over anywhere but at upshifts. The first two are X-048
rows (N2, the steward named: as built they run the map ECU, so the upshift's cut is X-047's over-long one and item 16 is
expected to move them). The nightly case "shipped builds on the pull" plays the three (a few minutes; the boxer stays
as the build that reads clean); it leaves the CI soak with the twin's triples if the soak's added share passes 10
minutes. Reproduce one build: `BOUNDPROOF="corpus <stem> pull" BOUNDPROOF_DT="<17 nudges>" tee-tests-combo
"[.boundproof]"` (per-window lines "window | dT | event | excess | at | over|under").
Proposal, not done: the whole corpus (31 builds) on the pull as a weekly entry, judged per event the same way: about 40
minutes on this container's 4 shared cores, single draw at 293.15 K; a newly red row then gets its 17-copy spread (about
4 minutes a build) before its entry.

**26 Nov, X-052's valve+intake+car row left Known.h** (the lead's ruling, 15:48). The turbo six's tour with
valve.model+intake.model+car.model, 17 copies at 293.15 +- 0.15 K per event, on the tip (8ec06fd1's sim, the
lift-matched builds): 0 of 17 on every event, the limiter's worst 2.51 against the nightly bound 2.97 (start 2.87, the
rest 2.70, cut 2.17). G's corrected-corpus table (tip and G's secant tree) read the same row 0 of 17 on every event on
both. It was 2 of 17 at the limiter on the dial build: the cam swap, as X-049's row and X-041's tour lift. X-052's
intake+exhaust+voices row stays (1 of 17 at the limiter, 3.03 over 2.97, on the tip; 0 on G's tree against its own 3.46).

**26 Nov, part C: the reference's output limiter. RULED no change** (the lead, 15:48; form (i), the reference rendered
off its limiter, NOT taken; the corpus's views and levels stay). Read per event, sound.model against the every-switch-off
reference, 17 copies, on the tip and G's stand-in (kValvePasses 30), (a) own settings against (b) the reference off its
0.85 output limiter (small four sound.level -24, peak 0.610; turbo six -24 alone still pins it at 0.863, so -24 and
sound.distance 0.6, peak 0.475), the case given the same values; `BOUNDPROOF_SET` / `BOUNDPROOF_REF` of [.boundproof]:
(1) neither pull has a "limiter" window: that event is the rev limiter (ecu/engine flags), which the pull never reaches;
the output's limiter is no event the judge names, so it can only move a bound. (2) Small four: the case the same to the
digit in (a) and (b) on both trees (sound.level does not reach sound.model's path), 0 of 17 everywhere; the bound
2.82 -> 2.82 (tip), 2.94 -> 2.96 (stand-in). (3) Turbo six, tip: bound 2.67 -> 2.88 (+0.21: the reference's -0.03 K copy
1.34 -> 2.72); case 1 of 17 -> 0 of 17. (4) Turbo six, stand-in: bound 2.92 -> 2.76 (-0.16); 0 of 17 both. Not one
direction, and about the reference's own copy-to-copy spread, which the lowest-of-five exists to cut. What the numbers
do not show: that a reference pinned on its limiter makes the bound too tight (the tip's reference top ROSE off it, 2.06
-> 2.72 over the run; the stand-in's fell, 2.72 -> 1.85).

**The lowest-of-five bound's known blind spot** (the lead's ruling, 15:48: no entry). A reference copy that clicks
raises nothing, since the bound is the reference's LOWEST excess over its copies. Met on sound.model / turbo-six / pull
on the tip at dT -0.07 K: the case 3.56 at 4.612 s against the bound 2.67, and the reference's own copy at that nudge
3.29 at 4.607 s (over its own bound): both sides click at the same moment, the engine's own event on the frozen
reference. Never red on a draw (the 293.15 K copy reads 1.75); 1 of 17.

**26 Nov, part C NOT TAKEN** (the lead's ruling, 17:22): the cut-aware neighbour variant lowers the shift rows' counts
and cures none, tightens the limiter rows, and on one reference leaves no excess; the events it concerns change under
items 16 and 17. The judge stays as parts A and B left it; the tables as measured in
docs/measurements/combo/2026-11-26-part-c.md; the variant on a local branch, never pushed. Re-opened after the window
as one question: with the cut at 60 ms and the valve converged, does dropping a shift's silent periods move a verdict.

**26 Nov, the shipped corpus on the pull** (the lead's survey; docs/measurements/combo/2026-11-26-shipped-pull.md; no
entries, as ruled). Item 16's probe brings every upshift over on the tip to 0 of 17 (the three-cylinder's 6 and 8, the
kei's 2) and adds no copy anywhere. Not cured by it, for whoever reads them: flat-plane-v8-39-twin-turbo's 1-2 upshift,
1 of 17 on both trees (a dual-clutch box: no shift cut); the same build at the LIMITER on the pull, 7 of 17 over its
bound 2.33 (top 2.63), and in the rest 1 of 17 (2.76), both trees: beside X-052's limiter rows, and on no nightly path
(the corpus nightly plays the tour); the rally five's cut 1 of 17 (3.85 over 3.51) on the tip only. Eight builds have no
shift event on the pull (five automatics, the CVT, the eighteen-speed: the paddles do not apply). Proposal (not done):
the whole corpus on the pull as a weekly entry, judged per event; this survey took about 12 minutes for the single
draws and 2 hours for the 17-copy spreads of all 31 on two trees, on 4 shared cores.

**X-054 and X-055 (26 Nov evening).** X-055: the flat-plane twin-turbo V8 joins the nightly's shipped-pull case as
its fourth build (the lead's ruling, 18:11); its rows (the limiter 7 of 17, the 1-2 upshift on its dual clutch 1 of 17,
the rest 1 of 17) are in Known.h. X-054 (the judge's, mine): a single-sample click saturates largestStep's measure (each
edge of an impulse sits in the other's local level, so the measure tends to sqrt(2 x 480) = 31), so on a steady
full-throttle stretch a click about 11 dB over the firing's sharpest edge passes in 7 of 17 copies, and at the limiter
in every copy (the small four's tour); the plant table, the guard's widths and its list go with its entry.
A question for after the window, beside X-054 (the steward's read of X-055, the lead's 18:16): whether a limiter's row
should be read per EPISODE (how many of its episodes are over, and the reference's the same way) instead of the worst of
all of them, since a run with forty episodes and a run with four are not judged alike today. No code for it now.

**X-054 (26 Nov evening), measured, no judge change**: docs/measurements/combo/2026-11-26-x054.md. The saturation
of largestStep's measure (each edge of an impulse sits in the other's level) makes the judge blind to a single-sample
click where its neighbourhood is loud: on the reference sound path, 7 of 17 copies pass at ~11 dB over the firing's
sharpest edge on a steady full-throttle stretch and 17 of 17 at the limiter; with sound.model on, 17 of 17 are caught.
A guard of g samples (left out of the level) closes clicks up to ~g samples wide, but g4 over all 149 visible combo cases
turns 72 rows over and 20 cases red, raises 1661 bounds (to +2.95) and lowers 1910 measures (to -1.9): not a tightening.
RULED no judge change (the lead, 26 Nov 20:59); nothing enters Known.h. OPEN FOR AFTER THE WINDOW, three questions
and no more: (1) a measure whose numerator unsaturates without moving its neighbours' and the references' (the robust
family, e.g. the click's level from its neighbouring periods only: it moves every reference and needs its own list);
(2) a limiter's rows read per episode (how many of its episodes are over, and the reference's the same way), not the
worst of all of them; (3) part C's one question on the fixed cut: with item 16's cut at 60 ms and item 17's valve
converged, does dropping a shift's silent periods move any row's verdict.

## 27 Nov: the pull in D for an automatic and a CVT (the lead's ruling, 11:37)

The integrator's reading confirmed by car.gear: the combo pull left the lever in N, so an automatic, a CVT and the
eighteen-speed ran it in neutral (gear 0, speed 0, the engine free). The survey of 26 Nov said "in one gear, against
the limiter" for those eight builds: corrected in its file. No nightly or default case was affected: none plays those
boxes through the pull or the launch (the generator's are manuals and dual clutches; the shipped pull's four are three
manuals and a dual clutch), and the tour moves no lever for any box.

Done: the pull and the launch put the lever in D at 3.0 s and back in N as the car stops, for a build whose "gearbox"
is Automatic or CVT (leverDrives); no other box's lever moves; the reference key carries the lever. Held by a fast-3
case that fails with the lever left in N. Measured (docs/measurements/combo/2026-11-27-automatic-pull.md): the six
automatics and the CVT of the corpus on the real pull, 17 copies, 0 of 17 at every event; no new row. Weak where the
every-switch-off reference's excess is high (8 to 12 in most copies on the real pull; bounds up to 17.02): which event
raises those references is open.

The eighteen-speed (the tractor; four more at the top level, in no case): no driver. Today it judges, on the tour, a free
rev in neutral like every other box, and on the pull (the survey only) neutral with the engine governed at 210 rad/s.
Proposed for the lead's ruling, not done: (a) name its pull rows for what they are, "in neutral (no driver)", and keep
the truck off the pull; or (b) the smallest honest scenario, a pull-away in one gear: the clutch floored from rest (the
clutch brake stops the input shaft), the lever to position 1 (low range, gear 3: roadranger.fm), the clutch let up as
the throttle opens, held in that gear to the stop, no shifts (a shift needs a float or a double clutch: a driver, not
a timeline). (b) judges the pull-away and the governed run in gear, not the shifts.

27 Nov 12:07, the lead's ruling, done: a reference never moves the lever (referenceScenario), since a reference's own car
(car.model 0) is a manual and lever 1 dropped it into first with no clutch: it stalled, and the automatics' wide bounds
were a stalled engine's. Every played reference that stalls while driven throws (requireReferenceRuns). X-058 (the
automatic put into gear at rest, v6-35-twin-turbo-pickup's start, 1 of 17) numbered and recorded in
docs/measurements/combo/2026-11-27-automatic-pull.md; no case's path, not in Known.h. Next: the tractor's one nightly
pull-away (the lead's 3(b)): 17 nudges without a stall, no shifts, its cost stated before it lands with [soak].
