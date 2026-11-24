# Q2: FMinus's own levers

*Package Q, step 2, 2026-11-24. Nothing in `sim/` changed for these: one copy of the simulator (`sim/` as merged at
14:05 UTC, 221 kB of machine code: other packages' modules are arriving, the reference's was 158 kB) compiled twice in
one process, once with the host's options as they are (`host/src/Program.cpp`: -O2, `Math::Vector`,
`Denormals::Flush`, knob versions 8) and once with one option changed, and played on the same builds and scenarios,
the runs alternated. The option is changed by a copy of `Program.cpp` that reads environment variables, linked into
`docs/measurements/tools/qbench` only; `host/` is untouched.*

Machine: this container (Xeon at 2.10 GHz, AVX-512), nothing else running; 4 runs of 12 s of each scenario, the
least time kept. **The run to run spread was 5 to 45 %** (most over least) even so: the cloud host's neighbours, not
ours. A ratio within about 3 % of 1 is no result.

## Each lever alone

"B / A": the lever's cost over the default's, summed over v8 and large-diesel on free-revs and drive-car. Compile:
`EnginePlayer::create` and the machine code, wall time, this machine (the owner measured 2.1 s on Windows for the
reference).

| Lever | B / A | Per pair | Output | Compile, s (default's in the same run) | Code |
|---|---:|---|---|---:|---:|
| -O3 | 0.984 | 0.990, 0.986, 1.022, 0.935 | identical to the bit | 2.05 (1.98) | 247 kB |
| -O1 | 1.016 | 0.986, 0.988, 1.045, 1.077 | identical to the bit | 1.84 (1.92) | 219 kB |
| -O0 | 2.586 | V8 free-revs alone | identical to the bit | **0.13** (1.57) | 201 kB |
| `Math::Fast` | 0.97 to 0.99 (one pair noisy) | | **not** to the bit: see below | 2.07 (1.66) | 167 kB |
| `Math::Libm` | 1.014 | 1.062, 0.938, 1.095, 0.983 | **not** to the bit: the same difference as Fast | 1.38 (1.88) | 154 kB |
| `Denormals::Preserve` | 1.066 | 1.037, 1.082, 1.166, 0.989 | identical to the bit | 1.56 (1.58) | 221 kB |
| knob versions 0 | 0.990 | 0.950, 1.017, 1.006, 0.995 | identical to the bit | 2.18 (1.92) | 221 kB (the same) |
| -O3 and `Math::Fast` | 0.997 | 0.985, 1.052, 0.967, 0.965 | not to the bit (as Fast) | 1.90 (1.71) | 184 kB |

What each permits, and what it did here:

- **The optimisation level.** -O1, -O2 and -O3 play within the noise of each other and give the same samples. -O3
  costs 5 to 10 % more compile time and 12 % more code for nothing measurable. **-O0 compiles in 0.13 s and plays
  at 2.6 times the cost** (4.9 us a sample on the V8: about 47 % of a core here at 48 kHz): fast enough to start the
  application with sound at once while -O2 compiles behind it, then swap (the same samples, so a swap at a part's
  boundary is seamless). That is the one compile-time lever worth having.
- **The math mode.** `Vector` (the default: FMinus's own kernels, 1.56 ulp, the same result in a vector lane as in a
  scalar) against `Libm` (the C library's, a lane at a time) and `Fast` (LLVM's intrinsics: libm's where nothing
  vectorises, which is everywhere hot here, so Fast's output equals Libm's to the bit). Neither is faster by more than
  the noise: the program's transcendental functions are scalar calls either way. Both change the last bits of every
  `expf`, `logf`, `sinf`, so the output is not the reference's; how far it is from it is in "What not to the bit
  means here", below: no further than a nudge of one bit anywhere. **Keep `Vector`**: it is the only mode in which a
  loop that vectorises gives the scalar's samples (the listener's change of step 3 relies on it).
- **Denormals.** The host already flushes them (`Denormals::Flush`, the plugin's mode). Preserving them cost 0 to 17 %
  (6.6 % over the four) and changed no sample: no subnormal reaches the output in these scenarios, but the filters'
  tails produce them while they ring down, and that is where the cost was. Keep Flush.
- **The host's processor.** FMinus targets the host (`JITTargetMachineBuilder::detectHost`: here AVX-512 with FMA).
  **It vectorises almost nothing that matters.** In the optimised IR of the whole program the loop vectoriser
  vectorised loops in `Exhaust::clear`, `Duct::clear`, `Duct::configure`, the layouts' setup, the zeroing of the
  ports' flows in `Engine::sampleTick` and two small loops in `Exhaust::step`: nothing that runs per pipe or per
  cylinder per sample. What vectorises in the hot path is the SLP vectoriser's pairs (`<2 x float>`: `Duct::read`
  filters its forward and backward waves together; 78 pair operations there) and some `<4 x float>` in
  `Hall::process`. `Valve::flow`, `Cylinder::step` and `Exhaust::node` are scalar chains. No FMA is formed from
  the program's own arithmetic (no `contract` flag anywhere: FMinus keeps C's rounding, as `-ffp-contract=off`), so
  results are the same on every machine; FMA appears only inside FMinus's math kernels.
- **`param`, `restrict`, `const`, the knob versions.** The simulator has no `param` at all (`grep -n "param " sim`):
  every switch is a field, so knob versioning has nothing to version (0 and 8 compile to the same 221 kB). To see
  what `param` specialisation of the fuel and cycle switches could buy at most, a copy with them hard-wired for a
  four-stroke gasoline V8 (`rotary`, `diesel`, the rotary's shared ports and the diesel's clatter branch made
  constant false): identical to the bit, and **no faster** (1.002 and 1.056 on the V8's two scenarios, noise). The
  branches are perfectly predicted; there is no hot loop for a constant to unlock. Not worth a contract change.
  `restrict` and loop shapes: the hot loops are loops over modules whose bodies call methods that write the module
  (a pipe's delay line and its filters' states), which no qualifier makes vectorisable; see step 4 and the design
  note for the structure that would (the pipes' state as arrays across pipes).

## Together

-O3 with `Math::Fast`: 0.997, not to the bit. Nothing to combine: no lever gives more than the noise alone.

## What not to the bit means here

The engine is a chaotic system: two renders whose arithmetic differs in one last bit anywhere part company within the
first sound (the difference sample for sample reads about 0 dB under the signal by the second second) and sound the
same. So a "difference under -80 dB" cannot be the test for any change that is not exact: none passes it, however
small. qbench therefore also reports how long the two stay within -80 dB of each other, the long-term spectrum's
third-octave bands (25 Hz to 20 kHz, those within 50 dB of the loudest), each quarter second's level (within 40 dB of
the loudest) and the engine's speed apart. **The floor**: the simulator against itself with the outside air 0.0001 K
warmer (300 K: one or two bits), "the same engine on another day":

| build | scenario | together for | bands, worst | quarter-second level, worst | speed apart, worst / mean |
|---|---|---:|---:|---:|---:|
| v8 | free-revs | 0.25 s (the first sound) | 0.77 dB | 4.98 dB | 404 / 42.7 rpm |
| v8 | drive-car | 0.25 s | 0.42 dB | 0.72 dB | 46 / 9.1 rpm |
| large-diesel | free-revs | 0.25 s | 0.14 dB | 0.58 dB | 9 / 2.7 rpm |
| large-diesel | drive-car | 0.25 s | 0.18 dB | 0.70 dB | 21 / 5.5 rpm |

`Math::Libm` (and Fast) against the default: bands 0.49, 0.37, 0.27, 0.13 dB; quarter seconds 4.01, 0.63, 0.71,
0.50 dB; speed 372/38, 80/8, 8/2, 8/1.4 rpm: inside the floor everywhere. (The V8's lope at idle on its big cam is
where the floor is widest: which beat of the lope a blip lands on decides the blip's peak.)

**Proposal to the lead for the tolerance** (instead of -80 dB): a change that is not exact is accepted for a module
when, on every build and scenario of the null test, its band, level and speed differences are within the floor's
above (measured in the same run, as qbench does), and a person has listened to the pairs named in the change's note.
The bit test stays for every change that claims to be exact.

## The host change proposed

None for speed: the host's options are already the fastest that keep the output (-O2, `Vector`, `Flush`). Two for the
application:

1. **Start at -O0, swap to -O2.** Compile at -O0 (0.13 s here) to make the first sound, compile -O2 on a worker
   thread (1.5 to 2 s here), and swap the instance at a part's boundary: the state is the same module layout, the
   samples identical. Saves the owner's 2.1 s at launch and at every reload of a sim file. (Card B or C: the swap
   is theirs; `tee::Program::Options::optimisation` is all FMinus needs.)
2. **Watch the compile time as packages land**: the program has grown from 158 to 221 kB of code since K2 and its
   compile from 1.4 to 1.5-2.0 s here. At this rate the 4 s limit holds through wave 2; tee-bench's compile table
   (or qbench's first two lines) is the check.

## Reproduce

```
docs/measurements/tools/build.sh
build-dev/q/qbench --a <sim copy> --b <sim copy> --benv TEE_Q_MATH=libm --builds v8,large-diesel \
    --scenarios free-revs,drive-car --repeat 4 --seconds 12
build-dev/q/qbench --a <sim copy> --aenv TEE_Q_IR=out/ir.ll --builds v8 --scenarios free-revs --repeat 1 --seconds 1
```
