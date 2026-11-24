# Q: performance (handoff, 2026-11-24)

**Package Q, branch `claude/awesome-gates-ntdb58`.** Owns `docs/measurements/**`; exact changes committed in other
modules under the lead's 14:58 rule. Machine: this container (Xeon 2.10 GHz, AVX-512, quiet locally, noisy host).

**Done** (each with numbers in `docs/measurements/`):
- q1-profile.md: where R goes (V8, V12, Large Diesel; free-revs, drive-car): Valve::flow 24 %, Duct::read+write 23 %,
  Cylinder 15 %, Exhaust::step 12 %, the pipes' per-16 coefficients 5-6 %. R on a quiet core: **1.79 us**, not 2.3.
- q2-levers.md: FMinus's levers: none faster beyond noise (-O3 0.98, Fast/Libm not exact, Preserve 1.07, knob versions
  moot: no `param` in the sim; hard-wired switches no faster). Keep -O2/Vector/Flush. -O0 compiles in 0.13 s at 2.6x.
- Exact commits (null test 59/59 identical each time, owners' tests green): 8ade884 listen (compressor's log/exp
  vectorised, resampler's zero blend; 1.6 %); c2a0e14 duct caches (1.9 %); b9fb66b boost machine takes the engine's
  rate (exact at 96 kHz); dd8ec5f X-005 (boost charge temperatures not 0 K). 3082af9 (valve early exit) was taken
  back in 3a3bcaa: it saves nothing; my first "8 %" was host noise (q3-q4-changes.md, corrected).
- 42c3dd5: `TEE_FAST`, the program's one fast switch (`#ifdef` in exhaust/duct.fm): valves in 2 passes, **10.3 % of
  R** (7 runs, both orders), not exact: bands 1-20 kHz +0.3-0.7 dB on free-revs, 250 Hz +1.0-1.3 dB on drive-car.
- q-rule.md (the rule: exact first; 7 runs both orders; ensemble floor; one switch). q5-design-note.md (parallel
  parts: FMinus region ~1 us, 2.3x latency, +5-8 % CPU; pipes as arrays; 48 kHz inside: 0.48 cost, torque within 1.2 %).
- Tools: docs/measurements/tools/qbench (A/B, bit test, bands, levels, speed, CPU ratio, defines), par (regions).

**Not done**: the ensemble floor of 8+ perturbed runs (one perturbed run measured; qbench does pairs, the ensemble is
a loop of them); boost's budget (0.24 R vs 0.12: F's own three suggestions stand; its pipe-end exit saves <1 %); the
pipes as arrays (G's network, exact, est. 8-12 %); Cylinder::volumeSlope table (not exact); the hour soak.

**Measurements**: sim/ref 1.016 over V8 + Large Diesel (free-revs, drive-car) with D's hooks in: exact work paid for
them. Twin-turbo V8 1.26 R; + G's dual X system 0.31 R = ~1.57 R (47-63 % of the owner's core at R = 30-40 %); with
exact changes ~1.53 R; with TEE_FAST ~1.41 R (42-56 %). **The card's target is not met**: exact work gave ~3 %, not a
fifth; TEE_FAST + pipes as arrays would reach it. Compile 1.5-2.0 s here at -O2 (221 kB of code).

**Wanted outside my paths**: host (B/C) to pass `defines["TEE_FAST"]` only after the owner's listening; -O0 first
sound then -O2 swap (lead: later); card C: 48 kHz numbers relayed; tee-dyno on large-diesel gives negative torques.

**FMinus**: no bug. Observations for the owner: the loop vectoriser vectorises no hot loop (loops over modules whose
methods write state); a `parallel { }` region costs ~1 us; two tasks' modules adjacent in memory false-share (40 %).

**Listen to**: TEE_FAST off vs on, loudness-matched: v8 and default, free-revs and drive-car (pairs sent to X).
