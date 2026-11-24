# X: the whole program under test (first handoff, 2026-11-24)

Package X, branch `claude/awesome-gates-ntdb58`, cloud session. Owns `tests/sim/combo/**`, `content/builds/combo/**`,
`tools/tee-fuzz*`, `scripts/listening-pack.*`, `docs/checkpoints/M2.md`, `docs/measurements/combo/**`.

**Done, by test** (`tee-tests-combo`, ctest `-L combo`; the harness `tests/sim/combo/Combo.h` finds the model switches
in the program's Schema, so a package's switch is covered the moment it lands):
- `archetype <a> with nothing switched on / with every switch on through every scenario`: five archetypes
  (`content/builds/combo/`: small four, V8, turbo six, large diesel, turbo V8 with a dual exhaust) x four scenarios
  (tour, pull, launch, cold; `tests/sim/combo/scenarios/`). Judged: finite, within full scale, no guard trip, DC under
  0.005 FS, largest step within max(12, 2x the reference's own on the same build and scenario), starts, idles 450 to
  1800 rpm (diesel 350 to 1500), reaches 85 % of its redline in neutral, the car moves, stops after the key; telemetry
  finite and within its declared range (else bounds by quantity).
- `switch <i> alone and with each switch after it`: every single and pair, V8 tour and turbo six pull.
- `switch <i> flipped on and off while the V8 / turbo six pulls in gear`; `every choice and switch of spec.<section>
  stepped while the V8 revs`; `a build loaded over another while it runs` (each with every switch on too).
- `fuzz: random Specs inside the ranges` and `the ranges' corners` (seeds 1 to 6, 6 s each); `nothing on the audio
  thread allocates with every switch on` (operator new counted around SimHost::process; not built on Windows).
- Nightly (label `nightly`, skipped unless `TEE_NIGHTLY` is set): every triple x every archetype x tour and pull; fuzz
  seeds 100 to 299 random and corners; the ten-minute soak of everything on (an hour by hand: `TEE_SOAK_SECONDS=3600`;
  the run keeps its audio in memory, about 2 GB for an hour).
- `tee-fuzz`: `--seeds A-B [--corners]`, `--seed N --reduce` (ddmin to the fewest fields), `--play <archetype>
  <scenario> <switches>` (one combination, one line), `--match` (the pack's loudness), `--write-scenarios`.
- The listening pack: `scripts/listening-pack.sh` and `.ps1` over `scripts/listening-pack.tsv`; `docs/checkpoints/M2.md`.

**Findings** (`tests/sim/combo/Known.h` lists the open ones):
- X-001 (D, open): `engine.custom` on with the Spec's default geometry (8 cylinders, every pin 0) never starts.

**Asked outside my paths**: card C: `tests/host/ValidationTest.cpp` should skip anything in `content/scenarios/` that is
not a `.json` file (my scenario files wait in `tests/sim/combo/scenarios/` until then).

**Not run**: the nightly tier and the soak (not yet in this session); Windows and macOS (CI only); the listening pack on
a person's machine.
