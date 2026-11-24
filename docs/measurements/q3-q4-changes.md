# Q3 and Q4: changes, made and proposed

*Package Q, steps 3 and 4, 2026-11-24. Each change measured with qbench (`tools/qbench.cpp`): the simulator before and
after, in one process, the runs alternated, the least time of 3 to 6 runs of 8 to 12 s of each scenario. The machine
is this container (Xeon 2.10 GHz), quiet, but its run to run spread was 5 to 45 %: ratios within about 3 % of 1 are
not results. "Exact" means the null test (`tee-tests "[null]"`: 59 pairs, every preset and scenario, 48, 44.1 and
96 kHz) gave every pair identical to the bit with the change applied. For a change that is not exact, the floor it is
judged against is q2-levers.md's ("What not to the bit means here"): the same engine with its air 0.0001 K warmer.*

## Summary

| Change | Module (owner) | Status | Exact | V8 and Large Diesel, cost after / before | R saved |
|---|---|---|---|---:|---:|
| The compressor's `logf`/`expf` in a vectorised loop of their own; the resampler's zero blend skipped | `sim/listen/listen.fm` (Q today; L's card) | **pushed** (7f0aff5) | yes | 0.974 (0.957, 0.995, 0.950, 0.994) | about 2.5 % |
| `Valve::flow` stops at a pass that repeats its inputs | `exhaust/duct.fm` (G) | patch: `proto/valve-flow-exit.patch` | yes | **0.917** (0.923, 0.914, 0.892, 0.938) | **about 8 %** |
| `Duct::set`'s tracker and `Duct::pack`'s shelves designed again only when their inputs move | `exhaust/duct.fm` (G) | patch: `proto/duct-coefficient-cache.patch` | yes | 0.992 (0.980, 0.992, 1.014, 0.986) | about 1 % (noise-level) |
| `BoostEnd::flow` stops at a pass that repeats its inputs (as `Valve::flow`) | `boost/gas.fm` (F) | measured, not proposed | yes (twin-turbo V8, turbo four: identical) | 0.995 | under 1 %: not worth F's time |
| `Valve::flow` in two passes instead of three (on top of the exit) | `exhaust/duct.fm` (G) | measured; **not** proposed as "no audible change" | no | **0.870** over 8 pairs | 10 to 13 % more |
| 48 kHz inside (the whole engine) | `engine.fm` (D) | for the dyno ribbon only: see the design note | no | 0.48 | half |

With the three exact ones (listen, valve exit, the caches) **R comes down by about 11 %**, bit for bit. The card's
fifth needs the two-pass valve or a change of the same size elsewhere; the two-pass valve alone gives it, and is the
first thing a person should listen to (below).

## Q3: the modules nobody else is editing today

**`sim/listen/listen.fm`** (pushed). `Listen::finish` ran the compressor's `logf` and `expf` a sample, scalar, inside
the loop that also carries the envelope, the limiter and the ceiling (each sample's state depends on the last). Split
into three passes over the 16-sample part: the levels and the envelope in order; the gain (`logf`, `expf`) in a loop
of its own, which LLVM vectorises (FMinus's `Math::Vector` gives a vector lane the scalar's result, to the bit); the
limiter and the ceiling in order. `Listen::resampled`: at a host rate that divides the engine's (48 kHz: every output
sits on a phase of the kernel's table) the blend between two phases multiplies by 0 and adds; skipped then. The
profile: `Listen::finish` 26-35 ns to 14 ns a sample, `Listen::resampled` 45-49 to 32-33 ns.

What was not changed, and why:
- **The resampler's 96 taps.** The sum is one dependent chain of 96 adds each channel (C's order; FMinus contracts
  nothing). Fewer taps or a polyphase or halfband form changes the sum's order and its last bits: not exact, and
  worth 15 to 25 ns (about 1 % of R) at most. A halfband pair needs a different kernel (this one cuts at 0.92 of the
  host's Nyquist, which is not a halfband). Left for a quality tier.
- **The room** (`Hall::process`, 40 ns, 2 %): an eight-line network already SLP-vectorised in fours.
- **`sim/sound/extras.fm`** (27 ns, 1.4 %): eleven resonators a sample, a `sinf` for the chain's whine. A phasor for
  the whine saves some 5 ns and is not exact. Left.
- **`sim/lib/`**: `OnePole::set` already takes its sine and cosine in one call; the filters' coefficients are
  recomputed by their callers (the pipes', below).

## Q4: the modules other packages own

**`Valve::flow`** (`exhaust/duct.fm`, G; 24 % of the V8's time, the largest single function). Each solve runs three
passes of a closed form (a table look-up, a `sqrtf`, two divisions, one dependent chain), from the pipe at rest,
whatever happens. A pass is a pure function of three numbers (`gg`, `pp`, `rr`); when a pass returns them unchanged
to the bit, the next pass would repeat it to the bit. The patch ends the loop there. Exact by construction and by the
null test (59 of 59), and **8 % of R** on the V8 and the Large Diesel: most solves converge to the bit in two passes.
`git apply docs/measurements/proto/valve-flow-exit.patch`.

**Two passes instead of three.** On top of the exit, 0.870 (V8 0.920 / 0.886, Large Diesel 0.811 / 0.893, defaults
0.814 / 0.869, V10 0.948 / 0.818). Not exact, and outside the floor: the third-octave bands move 1.0 to 1.6 dB
(the floor's 0.14 to 0.77) on every pair, the quarter-second levels 0.8 to 1.9 dB (floor 0.5 to 5), the speed
within the floor. A systematic change in the spectrum, not chaos: the third pass does real work in the blowdown.
**Listen**: V8 and defaults, free-revs (the blips and the limiter) and drive-car, against the three-pass build. If
a person hears nothing, it is the fifth on its own.

**The pipes' coefficients every 16 samples** (G). `Exhaust::pipes` calls `set`, `extra`, `pack`, `back` on every
pipe every 16 samples. `set` computes the tracker's two `expf` of the rate each time: cached by the rate. `pack`
designs two shelves (`expf`, `sinf`, `cosf`, `sqrtf` each) from the muffler dial, which glides and then stands:
cached by its five arguments. Exact (59 of 59); about 1 %. `back` and `flow16` follow the gas's temperature and speed,
which move every time: not cacheable exactly; a cheaper exponential or every 64 samples would be the -80 dB kind.
`git apply docs/measurements/proto/duct-coefficient-cache.patch`.

**`Duct::read` and `write`** (G; 23 %, 12 to 14 ns a pipe a sample, 29 pipes on the V8). Already SLP-vectorised in
pairs (forward and backward waves). What would help is structural (the pipes' states as arrays across pipes, so one
loop runs the wall's shelves of eight pipes in a vector): the design note. G's own plan, skipping the filters' loop
and the tracker on a bare pipe, saves the most on the builder's extra pipes (stubs, bypasses), which is where G's
+0.31 R goes: each of the 11 extra pipes costs the reference's 54-62 ns.

**`Cylinder`** (D; 15 %). `volumeSlope` (62-73 ns: `sinf`, `cosf`, `sqrtf` a cylinder a sample) could read a table by
the crank's angle (0.1 degree steps hold the volume to some 1e-7, near float's own rounding): not exact. Not measured
against the floor yet. The `expf` of the burn runs only while a cylinder burns.

**`Engine::sampleTick`** (D): the valves' click level (`expf(0.8 logf(rpm))`) was a per-sample cost when profiled at
c5636c5; D has since moved it to the 16-sample rate (`clickAmpC`): done.

**Boost** (F's machine, integrated): on the twin-turbo V8 it adds about 470 ns (0.24 R): `BoostMachine` 224,
`BoostEnd::flow` 123, the compressor 35, the rest 60. `BoostEnd::flow` has the same three passes; the same exit is
exact but saves under 1 % (its passes rarely converge early). Nothing proposed.

**Exhaust::step** (G; 12 %): each junction computes `1 / z` for every branch twice a sample (once in `step` for the
pop's source, once in `node`), and divides by `z` twice more; `z` changes only every 16 samples. Reciprocals kept at
the 16-sample rate are exact for the sums of `1 / z` and not exact for `2 a / z` (a product by a reciprocal rounds
differently from a division). Worth 2 to 3 %; not prototyped.

## Where the target stands

**R**: the exact changes take about 11 % off (listen 2.5, valve exit 8, caches 1). The fifth needs the two-pass valve
(a further 10 to 13 %, if a person hears no difference) or the volume table and the junctions' reciprocals together
(4 to 6 %, both not exact). **The turbo V8**: F's twin-turbo V8 measured 1.26 R (drive-car 2489 ns, free-revs 2528, R
1976 in the same run); G's dual system with an X, stubs and valves adds 0.31 R (their measurement, the builder not yet
in `engine.fm`): **about 1.57 R today**. The owner's R is 30 to 40 % of their core (the plugin's meter), so that is
47 to 63 % of it: at the edge of 60 %. With the exact changes (the valve's share scales with the cylinders, not the
pipes: about -0.19 R on this engine) **about 1.38 R, 41 to 55 %: under 60 % across the owner's whole range**. With the
two-pass valve too, about 1.2 R.

## Not done

The volume table and the junctions' reciprocals not prototyped; the intake (`Intake::plenum`, 3 %) and the vehicle
(1 %) not looked at beyond the profile: too small to matter yet.
