# docs/measurements: how a figure here is measured

**The method, for every figure from 15:40 UTC on 2026-11-24** (the lead's rule; [q-rule.md](q-rule.md) has the rest):
`docs/measurements/tools/qbench` (built by `tools/build.sh`), A and B in one process, **7 runs a side**, alternated, the
least of each kept, the **spread** stated (most over least); **both orders** (A against B, then B against A); and the
**processor-time ratio** beside the wall-time one (a shared host's stolen time lengthens the wall, not the processor
time). This container's host is shared: spreads of 5 to 60 % are normal, so a saving under about 3 % is not
believed until it holds that way. A figure measured otherwise is marked **"one run, shared host"** or with its count.

## The figures, restated

| Figure | As first given | Measured by the method | Where |
|---|---|---|---|
| R, the reference V8 on a quiet core | 1.79 us (`tee-bench --engine`, 3 runs) | 1.79 to 2.07 us in later 3-run tee-bench runs: the host drifts; ratios, not ns, are the result | q1 |
| The listener's change (8ade884) | 2.5 % (6 runs) | 1.6 % (0.984, 7 runs, one order) | q3-q4 |
| The pipes' coefficient caches (c2a0e14) | 1 % (5 runs) | 1.9 % (0.981, 7 runs, one order) | q3-q4 |
| `Valve::flow`'s early exit | 8 % (5 runs) | **none** (1.001, 0.998, 7 runs, both orders): taken back | q3-q4 |
| The valves' two passes (`TEE_FAST`) | 10 to 14 % (3 runs) | 10.3 % (0.897; 1.143 swapped; 7 runs) | q3-q4 |
| The pipes as arrays (proto, G's) | 8 to 12 % (an estimate) | V8, V10, defaults 3 to 8 % (0.921 to 0.971; swapped 1.001 to 1.067); Large Diesel none (1.007, 1.012); 7 runs | q3-q4 |
| 48 kHz inside | 0.48 (3 runs) | 0.488 (0.472 to 0.492 a pair; swapped 2.010 to 2.092, i.e. 0.48 to 0.50); the twin-turbo V8 too (0.491, 0.492) | q5 |
| The twin-turbo V8 (F's machine) over R | 1.26 R (one run, shared host) | 1.16 to 1.19 times the simulator's own V8 (2206 to 2215 ns against 1862 to 1946, 7 runs): **about 1.18 R** | q3-q4 |
| The boost machine's cost | 0.24 R (F's, on the old R) | about 0.16 to 0.18 R (the twin-turbo V8 over the V8, above) | q3-q4 |
| G's dual system with an X, stubs and valves | +0.31 R (G's, dev build, shared host) | not re-measured: G's builder is not in `engine.fm` yet | g-exhaust handoff |
| FMinus's levers (q2) | 4 runs | not re-measured: each within the noise, so none is claimed | q2 |

## The pages

- [q1-profile.md](q1-profile.md): where R goes, by module and function.
- [q2-levers.md](q2-levers.md): FMinus's own options; the chaos floor.
- [q3-q4-changes.md](q3-q4-changes.md): the changes made and proposed, with their numbers.
- [q5-design-note.md](q5-design-note.md): parallel parts, the pipes as arrays, 48 kHz inside.
- [q-rule.md](q-rule.md): the rule for changes made for speed.
- [q6-all-models.md](q6-all-models.md): every landed model on, the typical build against the 1.6 R budget.
- [q7-compile.md](q7-compile.md): where the compile goes, by package; defining models out; what FMinus could do.
- `proto/`: patches for modules whose owners have not landed (`duct-lanes.patch`: the pipes as arrays, G's).
- `tools/`: qbench, the region benchmark (`par/`).
