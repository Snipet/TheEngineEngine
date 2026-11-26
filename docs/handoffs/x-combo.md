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

**Findings** (X-001 to X-049; `tests/sim/combo/Known.h` holds the open ones with owner and reason; a closed one leaves
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
