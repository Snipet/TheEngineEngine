# Q: performance (handoff, 2026-11-24)

**Package Q, `claude/awesome-gates-ntdb58`.** Owns `docs/measurements/**`; exact changes in others' modules under
the lead's rule. Machine: this container (Xeon 2.10 GHz, quiet locally, noisy host).
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
- Pipes as arrays LANDED for model 0 (cf569cc): exact, 3-8 % on 8-12 cylinders, nothing on the six; inside G's
  builder (XNet::read) exact but no gain: not landed. Builder in the engine: +0.13-0.14 R at the reference topology,
  +0.37-0.44 R dual X (q6); its overhead is the generic junction path (next exact target, needs G). README.md: the method, every earlier figure restated.
- q-rule.md (exact first; 7 runs both orders; ensemble floor; one switch); q5 design note (parallel parts: region
  ~1 us, 2.3x less latency, +5-8 % CPU; 48 kHz inside). Tools: qbench (A/B, bit test, bands, CPU ratio), par.

**Not done**: the 8-run ensemble floor (one perturbed run measured); boost's budget (now ~0.17 R by the method, not
0.24: no exact win found; F's suggestions are not exact); Cylinder::volumeSlope table (not exact); the hour soak.
**Measurements** (7 runs, both orders): exact work committed ~3 % of R; twin-turbo V8 ~1.18 R; with G's dual X
system (+0.31 R, G's figure, not re-measured) **~1.49 R today**; 48 kHz inside 0.49 of the cost. Compile 1.5-2.0 s.
**The realistic ceiling** (lead's question): every exact thing I can see (the lanes patch ~0.07 R on this engine and
G's pipes, the junctions' 1/z sums ~1 %) lands the twin-turbo V8 with a dual exhaust at **~1.40 R: 56 % of the
owner's core on the slower figure (R = 40 %), 42 % on the faster** - under 60 % but with no margin, and before
variable timing, maps and G's builder add their shares (the card allows 1.6 R = 64 %). Exact work cannot make 60 %
certain. Non-exact: TEE_FAST (-10 %) ~1.27 R = 51 %; plus a volume table and the pipes' back/flow16 every 64 samples
~1.2 R = 48 %, each behind the switch and the owner's ears. **The honest answer for slow machines is a quality
setting**: 48 kHz inside costs 0.49, so ~0.7 R = 28 % (even a 1.6 R build: 31 %); it loses the top octave (bands from
about 10 kHz 13-24 dB down: the rasp and hiss on top of the note thin out), keeps levels and speed within the floor
and the torque within 1.2 %.

**Compile**: 466 kB / 3.4-4.2 s at 16:20, 597 kB / 4.8-6.4 s with G's builder: past 4 s (q7: object cache and
parallel compile asked of FMinus's owner). **Wanted outside my paths**: host passes `defines["TEE_FAST"]` only after the owner listens; -O0-then-swap later;
card C: the 48 kHz numbers; tee-dyno gives the Large Diesel negative torques. **FMinus**: no bug; the loop vectoriser
takes no hot loop (module-method loops); a `parallel { }` region ~1 us; adjacent tasks' modules false-share (40 %).

**Listen to**: TEE_FAST off vs on, loudness-matched: v8 and default, free-revs and drive-car (pairs sent to X).
