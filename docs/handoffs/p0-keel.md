# P0, K1 and K2: the keel

Phase 0, parts K1 and K2, on `claude/awesome-gates-ntdb58` (397078c onwards). A local agent on the shared cloud
machine. The numbers: [docs/measurements/p0-bench.md](../measurements/p0-bench.md).

**A fresh container**: clone the branch, then `scripts/bootstrap-cloud.sh` (apt, CMake, FMinus's LLVM checked by
SHA-256, every source by git: 24 s; 2 s run again), `cmake --workflow --preset dev-verify` (configure, build with 3
jobs, every test), `build-dev/bin/tee-render --preset v8 --scenario free-revs` (`out/v8-free-revs.wav`, `.csv`),
`build-dev/bin/tee-bench --quick`, `scripts/ci-status.sh`. A new test is a `.cpp` dropped in `tests/` (README, "Tests").

**Done.** K1: layout, README, LICENSE; CMake (`cmake/TeePins.cmake`, `TeeDeps.cmake`, `TeeFlags.cmake`, presets `dev`
and `release` with `*-verify`): FMinus, FunkGui's core, Catch2 and nlohmann/json by git at pinned commits into
`TEE_DEPS_DIR`, LLVM found and never built; `bootstrap-cloud.sh`, `fetch-llvm.sh`, `xvfb-run.sh`, `ci-status.sh`; CI on
three platforms, sources and compiler cached. K2: `sim/ref/` with `SEED.tsv`; `tee_host` (`Program`, `Schema`,
`Instance`, `RefPlayer`, `Scenario`, `Render`, `Wav`, `Clock`); `tee-render`, `tee-bench`; five presets, four
scenarios; `docs/formats.md`. Tests (`tee-tests`: 20 of mine pass here, 19 on CI's three systems so far): `the
reference compiles with no error and no warning`; `start, process and stop are proven nonblocking; init and end are
not, and need not be`; `with the checks the reference plays the same samples ...`; each scenario `renders finite and
within full scale`; `the same samples whatever the block size` (bit for bit); `the same sound at 44.1, 48 and 96 kHz,
resampled, to a stated tolerance` (start and idle 30 dB under: measured 41 and 55); `the engine starts, idles near its
idle speed, reaches its limiter flat out in neutral, and stops` (880 of 880 rpm; 6557 against 6500); `the strongest
low line at a held speed sits where the layout's firing predicts` (nine layouts within 0.01 %); the host's own, and
`tee::test::nullDepth` for K3.

**Measured** (a Xeon at 2.1 GHz, shared with other sessions' builds: processor time, the least of three): R, the V8,
2.3 us a 96 kHz sample: 22 to 23 % of a core at 48 kHz in blocks of 512. The V12 (ours: the V10's dials, twelve
cylinders) 3.1 to 3.2 us, 30 to 31 %; the V10 27 %, the Large Diesel 16 to 17 %. The checks cost nothing measurable.
CI's virtual M1: V8 24.7 %, V12 28.6 %. Compile 0.10, 1.24, 1.38 s at -O0, -O1, -O2. Cold build of the keel 2 min
42 s; a fresh checkout with the cache warm 54 s to configure, 30 s to build. `tee-render` is 61 MB stripped.
**CI**: github.com/Snipet/TheEngineEngine/actions/runs/45387990976 was green on all three (the keel alone: 3 min 17 s
Linux, 2 min 4 s macOS, 4 min 8 s Windows, no caches). Since then Linux stays green; macOS and Windows have failed on
other packages' code. `scripts/ci-status.sh` says how it stands now.

## `tee-bench` today, and how a package measures its cost against R

R is the reference V8's cost a sample: nanoseconds of the rendering thread's processor time for one sample of the
engine's own 96 kHz rate, release options (`docs/plan/02-contracts.md` §9). Here (a shared Xeon at 2.1 GHz) R is
about 2.3 to 2.4 us; budgets are stated in R, so a figure means something only beside an R taken on the same
machine in the same minute. Build `tee-bench` in the `release` preset for a figure to quote (`dev` is
RelWithDebInfo: close, not the same). Every mode prints a Markdown table; `--out <file>` keeps it.

| Command | What it prints | Time here |
|---|---|---|
| `tee-bench --quick` | the reference: V8 and V12 on free revs, ns a sample, share of a core at 48 kHz in blocks of 512, slowest call, with and without the checks; and compile times at -O0/-O1/-O2. The V8's "ns" **is R**. | 35 s |
| `tee-bench` | the same for every preset and scenario, least of 3 | minutes |
| `tee-bench --engine [--builds a,b] [--scenarios a,b] [--repeat n]` | the simulator (`sim/engine.fm`, each build of `content/builds`) and the reference played side by side from the same inputs (`tee::playTwin`): ns a sample each, and **simulator / reference, the figure in R** | 20 s a build and scenario at `--repeat 1` |
| `tee-bench --profile [--preset v8] [--scenario free-revs]` | where the reference's time goes: by module and by function, ns a sample each (a sampler at 4 kHz on the rendering thread; Linux and macOS) | 3 s with `--seconds 5` |
| `tee-bench --profile --engine [--build default] [--scenario free-revs] [--seconds s] [--all]` | the same for the simulator. A function the optimiser inlined has no row of its own: its time is its caller's, listed in the "inlined into it" column | 3 s with `--seconds 5` |

**A package's own cost**, in R:
1. `tee-bench --engine --builds <a build that turns your feature on>,default --scenarios <yours>,free-revs --repeat 3`:
   the ratio column is the build's cost in R; the difference between your build's row and `default`'s is what your
   feature adds (a quiet machine; repeat until two runs agree to 2 %).
2. `tee-bench --profile --engine --build <yours> --seconds 10`: your module's row in ns a sample, divided by the
   R of step 1's reference column, is your module's share in R. Look in "inlined into it" before concluding your
   module is cheap: a small function of yours may be inside another module's row.
3. Put the command, both numbers and the machine (`/proc/cpuinfo`'s model, `nproc`) in your handoff.

Today (default build, free revs, here): the simulator is 1.02 R. Its biggest modules: Duct 30 %, Valve 22 %, Exhaust
15 %, Cylinder 14 %. CI's Linux, macOS and Windows jobs print R (`--quick`) and the simulator against it (`--engine`,
default build, free revs) in each run's summary: a number for each runner, not a gate (a hosted runner is shared).

**Not done.** macOS and Windows have only CI behind them. No test holds the single's, the inline five's, the
flat-plane V8's or the rotary's strongest line (seen once: 2, 3 and 1.5 times the firing frequency; the rotary's 0.93
of it, unexplained). The plugin's way of playing (smoothed dials, calls of 16 samples, its frozen copy) is not
reproduced: the user's 30 to 40 % is bracketed, not explained. The application's CI steps are card B's.

**FMinus, FunkGui.** (1) FMinus's four runtime dependencies are archives by URL, refused here: the bootstrap clones
them where FMinus unpacks and points it there; FMinus then writes its `.fminus-populated` marker with a hash nobody
checked. (2) `FMINUS_PARSER_TOOLS_MODE` is a cache variable defaulting to `fetch`: a consumer's normal variable wins
on the first configure only (8a62eb8). (3) The LLVM archive's Bison needs `libtextstyle.so.0`, its `opt` glibc 2.44:
Ubuntu's Bison and Flex are used. (4) Catch2 must be C++20 on macOS and Windows (a19fbc1). FunkGui: nothing.
**Wanted outside my paths**: nothing. **For a person**: listen to `out/v8-free-revs.wav`, `v8-drive-gears.wav` and
`large-diesel-truck-pull-away.wav` against the plugin; run `./tee-bench --quick` from CI's `tee-macos-arm64` on the Mac.
