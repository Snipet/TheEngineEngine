# Q8: shipping the engine compiled ahead of time (feasibility, one page)

*Package Q, 2026-11-24, about 17:00 UTC (`date -u`). FMinus read, not edited: `docs/architecture.md` ("Ahead of time",
"The runtime"), `docs/programs.md`, `src/support/Aot.h`, `HostTarget.cpp`, `tools/fminus/main.cpp`. Measured with the
optimised IR FMinus keeps for a program (`KeepIr::Optimised`: "the IR the JIT generates the machine code from", what
`fminus --emit ll` prints) and LLVM's own code generator in a 40-line tool (`tools/aot/emitobj.cpp`), because
FMinus's driver is not built in our tree (`FMINUS_BUILD_TOOLS` OFF; it needs CLI11).*

**The idea**: CI compiles `sim/` with `fminus -c` for each define set we ship (Full; Economy:
`TEE_ENGINE_RATE_48000`), one object per platform, linked as a shared library; at launch the host hashes the `sim/`
files it would compile with the defines and FMinus's version, and when the hash matches the library's, it runs FMinus's
**front end only** (`Program::compile`, which does not run LLVM's pipeline until a function is looked up: 0.12 to
0.26 s here) for the Schema, the module's layout and the `nonblocking` proof, and takes `Engine::init`, `process` and
`end` from the library instead of the JIT. Anything else (an edited `sim/`, a define set not shipped) compiles in
the JIT as now.

## What is true already

- **The driver writes objects with the host's options**: `fminus -c engine.fm -o engine.o -O 2 --math vector
  --denormals flush --knob-versions 8 -D TEE_ENGINE_RATE_48000 -I sim/` (its flags: `-O`, `--math`, `--denormals`,
  `--knob-versions`, `-D`, `-I`, `-c`). Same compiler, same options, same pipeline (`optimiseModule`, then
  `emitFile`): the same code as the JIT's.
- **The object exports the entry points by name and needs almost nothing**: from today's `sim/` (all packages but
  none of the tests' appended functions): 800 functions, among them `Engine::init`, `Engine::process`, `Engine::end`
  (the names `fminus::Program::function` uses); undefined symbols only `fminus_rt_calloc`, `fminus_rt_free`,
  `fminus_rt_sinf`, `fminus_rt_cosf`, `memcpy`, `memset`. FMinus's runtime is a plain C static library with no LLVM
  (`libfminus_runtime.a`), built on all three systems already (our CI links it): link it into each engine library.
- **The layout is the front end's**: the module's struct layout is decided by FMinus's lowering for the target's data
  layout, the same on the JIT's side and the object's for the same triple; the host's Schema already comes from the
  front end.
- **The samples would be the same**: FMinus contracts no arithmetic into FMA and its `Math::Vector` kernels give the
  same bits in any lane on any target (FMinus.h), so an object built for a lower CPU than the JIT's gives the JIT's
  samples.
- **Size and time** (this container, today's `sim/` at 597 kB of JIT code): the optimiser about 2.9 s and the code
  generator 2.7 to 3.2 s (FMinus's own split when it keeps the IR); the object **1.38 MB** (655 kB of code). In CI that
  is some 6 s per define set per platform; at launch it would be 0.12 to 0.26 s (the front end) plus a `dlopen`.

## What is missing (for FMinus's owner, smaller than an object cache)

1. **A target for objects**: FMinus's optimiser and code generator always target the machine they run on
   (`JITTargetMachineBuilder::detectHost`, for the JIT and for files alike; the driver has no `-mcpu`, `-mattr` or
   `--target`). An object made on CI's runner carries that runner's features (here: AVX-512, "graniterapids"), and a
   user's older CPU would stop on an illegal instruction. Retargeting the optimised IR afterwards is not enough: the
   IR is already specialised (to `x86-64-v3` it compiled; to `x86-64-v2` LLVM stopped, "Cannot select
   llvm.x86.avx.vzeroupper"). **The ask: `-mcpu`/`--target-cpu` (and features) on `fminus -c`, applied to the
   optimiser as well as the code generator** (x86-64-v3 for Windows and Linux, apple-m1 for macOS).
2. (Ours, not FMinus's) the driver in our build: `FMINUS_BUILD_TOOLS` ON in CI only (with CLI11 fetched by git), or
   FMinus's `support/Aot.h` made public as `Program::emitObject(path, cpu)`; either is fine.

## What our side needs (card C, the host; card B, CI and packaging)

- `tee::Program`: a mode that compiles the front end only and binds the three entry points from a loaded library;
  `EnginePlayer` takes the Schema and the `nonblocking` report from the front end as now.
- Two engine libraries per platform (Full, Economy), each linked with the runtime so their symbols do not collide;
  each with its key (FMinus's version, the define set, the SHA-256 of every file the front end read, in order) in a
  sidecar file or an exported string.
- **Proof that it is the same code**: a CI test on each platform that plays every pair of the null test (and X's
  combinations) through the JIT and through the library and finds them identical to the bit, for each shipped define
  set; and the key check at launch, so an edited `sim/` can never run the old library.

## Verdict

**Feasible, with one small ask of FMinus** (a target CPU for objects); without it, it is feasible only by building on
the oldest CPU we support, which CI's hosted runners cannot promise. With it, every ordinary launch skips 5 to 6 s of
compile; the JIT stays for edited programs and unshipped define sets. Not yet measured: the library's speed against the
JIT's (an x86-64-v3 object loses AVX-512, which the hot path barely uses: q2 found almost nothing vectorised).
