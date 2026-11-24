# X: the whole program under test (handoff, 2026-11-24 17:40 UTC)

Package X, branch `claude/awesome-gates-ntdb58`, cloud session. Owns `tests/sim/combo/**`, `content/builds/combo/**`,
`content/scenarios/combo/**`, `tools/tee-fuzz*`, `scripts/listening-pack.*`, `docs/checkpoints/M2.md`, this file.

**Done, by test** (`tee-tests-combo`; ctest `-R '^combo\.'`; 84 default cases, each under 30 s; 32 `[nightly]`). The
harness (`tests/sim/combo/Combo.h`) finds the switches in the Schema (every `spec.<section>.model`, `engine.custom`,
`engine.wallHeat`: 11 now), so a package's switch is covered the day it lands. Judged in every run: finite, within
full scale, no guard trip nor FLAG_FAULT, DC under 0.005 FS, largest step within twice the same build's own reference
(a bang: within the largest of three reference copies with pops on), starts, idles 450 to 1800 rpm, nears its redline,
moves, stops; every telemetry column finite, in its declared range, else physical by its quantity.
- archetypes (`content/builds/combo/`: small four, V8, turbo six, large diesel, turbo V8 dual) through the tour with
  nothing and everything on; nightly: through all four scenarios (tour, pull, launch, cold; `content/scenarios/combo/`)
- every switch alone (V8 tour, six pull); nightly: every pair, every triple
- every switch flipped under load, singly and all in one block; every choice of 10 Spec sections stepped while running;
  builds loaded over each other, with nothing and everything on
- targeted: turbo with both ECUs (anti-lag, nitrous switched), launch control, dog box, traction on wet (standstill,
  rolling), the four limiter kinds, the engine dyno; the 30-build archetype corpus (6 by default, all nightly)
- fuzz: random and corner Specs (2 seeds by default, 1-6 and 100-299 nightly, with bounds checks); nothing allocates
  in process with every switch on (operator new; not on Windows); the 10-minute soak (nightly) and an hour by hand
- fixed findings held as tests: X-001, X-003, X-007 (`[fixed]`)
- `tee-fuzz`: `--seeds`, `--reduce` (ddmin), `--play <archetype> <scenario> <switches>`, `--checks`, `--match`

**Findings**: 18 filed today. Closed: X-001 (D), X-002 (not a defect), X-003 (H), X-004 (L, C), X-005 (Q), X-006 (H),
X-007 (D, I), X-008, X-009 (D), X-010 and X-014 and X-015 (N2, G), X-012 (T), X-013 (G), X-018 (H). Open (`Known.h`):
X-011 standstill (I), X-016 (the four-rotor rotary, the contract's reported heavy case, Q), X-017 (N1, D: charge
temperature 0 K before the first charge).

**Measured** (this container, shared 4 cores; R from the same process; docs/measurements/combo/2026-11-24.md): tour,
off / every switch on: small four 0.52 / 0.98 R; V8 1.00 / 1.57; turbo six 0.80 / 1.32; large diesel 0.77 / 1.19; the
typical turbo V8 dual 1.04 / 1.81 R, 39 % of a core at 48 kHz/512 (under 60 %, over the contract's 1.6 R). Corpus as
built 0.59 to 1.81 R. Soak: 600 s of everything on, 236 s wall, 0 failures, 1.71 R. Default tier: 510 s at -j3 here.

**The listening pack** (M2): `scripts/listening-pack.sh [--quick]` and `.ps1` over `scripts/listening-pack.tsv` (41
groups, an optional define column); `--quick` rendered 10 files in 90 s here. `docs/checkpoints/M2.md`: what to
hear in each group and the yes-or-no lines.

**Not run**: the .ps1 (no Windows here), the whole pack (only --quick), the hour soak, the nightly triples; nothing
on macOS or Windows by me (CI only). Nobody has listened to anything. **Outside my paths**: none. **FMinus**: no issue.
