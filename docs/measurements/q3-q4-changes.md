# Q3 and Q4: changes, made and proposed

> **Corrected at 15:40 UTC.** The first version of this page credited `Valve::flow`'s early exit with 8 % of R. Measured
> again with 7 runs in both A/B orders and by processor time as well as wall time, it saves nothing (1.001 and 0.998):
> a pass almost never returns its inputs to the bit. It was committed and taken back (3a3bcaa). The figures below are
> the re-measured ones; the lesson is in "How to measure here".

*Package Q, steps 3 and 4, 2026-11-24. Each change measured with qbench (`tools/qbench.cpp`): the simulator before and
after, in one process, the runs alternated, the least time of 3 to 6 runs of 8 to 12 s of each scenario. The machine
is this container (Xeon 2.10 GHz), quiet, but its run to run spread was 5 to 45 %: ratios within about 3 % of 1 are
not results. "Exact" means the null test (`tee-tests "[null]"`: 59 pairs, every preset and scenario, 48, 44.1 and
96 kHz) gave every pair identical to the bit with the change applied. For a change that is not exact, the floor it is
judged against is q2-levers.md's ("What not to the bit means here"): the same engine with its air 0.0001 K warmer.*

## Summary

| Change | Module (owner) | Status | Exact | V8 and Large Diesel, cost after / before | R saved |
|---|---|---|---|---:|---:|
| The compressor's `logf`/`expf` in a vectorised loop of their own; the resampler's zero blend skipped | `sim/listen/listen.fm` (now L's) | **committed** (8ade884) | yes | 0.984 (7 runs) | 1.6 % |
| `Duct::set`'s tracker and `Duct::pack`'s shelves designed again only when their inputs move | `exhaust/duct.fm` (G) | **committed** (c2a0e14) | yes | 0.981 (7 runs) | 1.9 % |
| `Valve::flow` stops at a pass that repeats its inputs | `exhaust/duct.fm` (G) | committed, then **taken back** (3a3bcaa) | yes | 1.001, 0.998 (V8, 7 runs, both orders) | none |
| `BoostEnd::flow` stops at a pass that repeats its inputs | `boost/gas.fm` (F) | not proposed | yes | 0.995 (5 runs: noise) | none to speak of |
| `Valve::flow` in two passes instead of three | `exhaust/duct.fm` (G) | **behind `TEE_FAST`** (42c3dd5), off by default | no | **0.897** (7 runs, both orders: 0.896 to 0.899 a pair) | 10.3 % |
| 48 kHz inside (the whole engine) | `engine.fm` (D) | for the dyno ribbon and a quality setting: the design note | no | 0.48 | half |

**Exact work so far takes R down by about 3 %** (the listener 1.6, the caches 1.9; together 0.975 over the V8 and the
Large Diesel, measured on the tree of 15:15 with the cylinder's and the mechanics' new hooks in, which cost about as
much: `tee-bench --engine` reads the simulator at 1.016 of the reference over those four pairs, as at the split). The
card's fifth is not reached by exact work; the two-pass valve gives half of it and waits for the owner's ears; the
pipes as arrays (the design note) is the largest exact change left.

## How to measure here

This container's host is shared: the same render runs 5 to 60 % slower from one run to the next, and a slow stretch
can fall on one side of an A/B pair for several runs. The early exit's "8 %" was that. qbench now alternates A and
B, takes the least of each, and reports the processor-time ratio beside the wall-time one; a saving is believed only
when it holds with 7 runs and with A and B swapped.

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
to the bit, the next pass would repeat it to the bit, so ending the loop there is exact (59 of 59). **It saves
nothing**: a pass almost never returns its inputs to the bit (1.001 and 0.998 with 7 runs, both orders). Taken back.

**Two passes instead of three.** 0.897 with 7 runs (V8 0.896 / 0.897, Large Diesel 0.898 / 0.896; 1.143 with A and
B swapped: the same 10 to 12 %); a wider first look over 8 pairs gave 0.81 to 0.95. Not exact, and outside the floor:
the third-octave bands move 0.7 to 1.6 dB (the floor's 0.14 to 0.77) on nearly every pair, the quarter-second levels
0.6 to 1.9 dB (floor 0.5 to 5), the speed within the floor. A systematic change in the spectrum, not chaos: the
third pass does real work in the blowdown. Which bands (qbench `--bands 1`, both channels, 12 s, two passes over
three): **free-revs** (V8, defaults): every band from 1 kHz to 20 kHz 0.3 to 0.7 dB up, a brighter top end; the floor
(air +0.0001 K) moves those bands -0.2 to +0.25 dB, centred on 0. **drive-car**: the 250 Hz band +1.0 to +1.3 dB,
400 Hz -0.35 to -0.65, 500 to 800 Hz up to +0.8; above 1 kHz within +-0.35 (the floor there: within +-0.25, one band
of the lows +0.42). Now behind the program's one fast switch (`TEE_FAST`, 42c3dd5), off by
default and in the null test. **Listen**: V8 and defaults, free-revs (the blips and the limiter) and drive-car, two
passes against three, loudness-matched (the pairs go to X's listening pack).

**The pipes' coefficients every 16 samples** (G). `Exhaust::pipes` calls `set`, `extra`, `pack`, `back` on every
pipe every 16 samples. `set` computes the tracker's two `expf` of the rate each time: cached by the rate. `pack`
designs two shelves (`expf`, `sinf`, `cosf`, `sqrtf` each) from the muffler dial, which glides and then stands:
cached by its five arguments. Exact (59 of 59); 1.9 % (7 runs). Committed (c2a0e14). `back` and `flow16` follow the gas's temperature and speed,
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

**R**: exact work takes about 3 % off (listen 1.6, caches 1.9). The fifth needs the two-pass valve (10.3 %, behind
`TEE_FAST` until the owner has listened) and the pipes as arrays (exact, estimated 8 to 12 %, G's network), or the
volume table and the junctions' reciprocals (4 to 6 %, not exact). **The turbo V8**: F's twin-turbo V8 measured
1.26 R (drive-car 2489 ns, free-revs 2528, R 1976 in the same run); G's dual system with an X, stubs and valves adds
0.31 R (their measurement, the builder not yet in `engine.fm`): **about 1.57 R today**. The owner's R is 30 to 40 % of
their core (the plugin's meter), so that is 47 to 63 %. With the exact changes about 1.53 R (46 to 61 %): **not
safely under 60 %**. With `TEE_FAST` too about 1.41 R (42 to 56 %); with the pipes as arrays as well, about 1.25 R
(38 to 50 %).

## Not done

The volume table and the junctions' reciprocals not prototyped; the intake (`Intake::plenum`, 3 %) and the vehicle
(1 %) not looked at beyond the profile: too small to matter yet.
