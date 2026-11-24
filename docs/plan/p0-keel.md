# Phase 0, parts K1 and K2: the keel

*Serial, before anything else in TheEngineEngine. One agent. Repository: **TheEngineEngine**, working copy
`/home/user/work/keel`, branch `claude/awesome-gates-ntdb58` (the integration branch: the lead pushes `docs/plan/`
to it from another checkout, so fetch and merge before every push). Read [01-rules.md](01-rules.md) first.
Part K3, the split of the reference into the new contract, is its own card ([p0-split.md](p0-split.md)) and starts
when the lead says.*

**OWNS**: everything in the repository except `docs/plan/**`.

**Ends when** a second agent, in a fresh container, can run one script, build, play the reference engine to a WAV
file, and add a test, without asking anyone anything; and the numbers the plan needs (CPU, compile time) are on a
page.

## K1: the repository, the build, the bootstrap, CI

1. **Layout** as 00-abstract-plan.md §3.1: `sim/`, `host/`, `shell/`, `ui/`, `shaders/`, `content/`, `tools/`,
   `tests/`, `docs/`, `scripts/`, `cmake/`. Create only what K1 and K2 fill; a directory nobody has filled yet does
   not exist. Add `README.md` (what this is, how to build on each system, in a page), `LICENSE` (GPL-3.0, the text
   FMinus and FunkGui carry: linking them makes it this application's licence), `.gitignore` (`build-*/`, `out/`,
   `.tee-deps/`, editor files).
2. **CMake** (3.30 or newer; Ninja): `CMakeLists.txt`, `CMakePresets.json`, `cmake/TeePins.cmake` (every
   dependency: name, repository, commit, version, licence, in one place), `cmake/TeeDeps.cmake`,
   `cmake/TeeFlags.cmake` (`tee_target_defaults(target)`: C++20, the warnings of 01-rules.md §4 as errors unless
   `TEE_WERROR=OFF`, `-ffp-contract=off`; never applied to a dependency). Presets `dev` (RelWithDebInfo,
   `build-dev`), `release` (`build-release`), each with a `*-verify` workflow preset that configures, builds and
   runs every test. Options: `TEE_BUILD_TOOLS` (ON), `TEE_BUILD_TESTS` (ON), `TEE_BUILD_APP` (OFF until FunkGui's
   desktop host is pinned; then ON), `TEE_WERROR` (ON), `TEE_DEPS_DIR`.
3. **Dependencies, all by git at a pinned commit** (archives by URL are refused in the cloud):
   - **FMinus** at `5b665d7` (write the full SHA), as a subproject: its tests, tools and examples off, its
     `-Werror` off, its JSON on (the default). Its LLVM: when `TEE_DEPS_DIR` (or `FMINUS_DEPS_DIR`) holds an
     install, use it (`FMINUS_LLVM_MODE=system`, `LLVM_DIR`; FMinus's `docs/library.md` lines 178 to 190 and
     `docs/building.md` say how); never start a build of LLVM by surprise: if no install is found, stop at configure
     with a message that says what to run. FMinus pins four runtime dependencies by URL (PFFFT, fast_float, yyjson,
     pugixml: `cmake/FMinusPins.cmake`): find how its CMake names them and give each a source directory the
     bootstrap cloned at the same version (`FETCHCONTENT_SOURCE_DIR_<NAME>`, or the mechanism FMinus offers), so
     the cloud never asks for an archive; on a machine where archives work, FMinus's own pins are used untouched.
     Bison and Flex from the system on Linux (`FMINUS_PARSER_TOOLS_MODE=system`), as FMinus's research note did.
   - **FunkGui** at `b7bc1cd` (v0.27.0) for now, JUCE off, tools and tests off: the core only, to prove the two
     configure in one tree. The pin moves to the desktop release when card A lands; leave the option names ready.
   - **Catch2** v3 (tests) and **nlohmann/json** (tools and files), each by git tag with its commit asserted.
4. **`scripts/bootstrap-cloud.sh`**: idempotent; for a fresh Ubuntu 24.04 container as these are. A CMake of 3.30
   or newer (pip); `apt` packages (ninja, clang, lld, flex, bison, ccache, and what FunkGui's GPU path and SDL3
   will need later: X11 and Vulkan development packages, xvfb, mesa-vulkan-drivers, openbox); FMinus's prebuilt
   LLVM for Linux from the release `deps-llvm-23.1.3` of `Snipet/FMinus` (through `gh api`, its SHA-256 checked
   against the release's `.sha256` asset), unpacked into `$TEE_DEPS_DIR/install/`; the git clones of step 3. It
   ends by printing the configure line to use. Note in the README what FMinus's research note found: the archive's
   `opt` wants a newer glibc than Ubuntu 24.04's; nothing we build runs it.
   `scripts/xvfb-run.sh <command>`: runs a command on a headless X display with a window manager and Mesa's
   software Vulkan (TestingPlayground's `.github/workflows/build.yml` does the same for its GUI tests).
5. **CI**, `.github/workflows/build.yml`, modelled on TestingPlayground's (`/home/user/snipet/testingplayground`,
   read-only): on push to `claude/**` and `main`, on pull requests, and by hand; macOS arm64, Windows x64
   (clang-cl), Linux x86-64. LLVM from FMinus's release archive for the platform, checked. **Caches, so a warm run
   is minutes**: the compiler's objects (ccache; sccache on Windows), the fetched dependency sources, and later
   shaderc; keys from `cmake/TeePins.cmake`'s hash. Build, run the tests, upload `tee-render` and `tee-bench` (and
   later the application) as artefacts per platform. Every action pinned to a commit SHA. A change under `docs/`
   alone builds nothing.

## K2: the reference playing in our own host

6. **`sim/ref/`**: the reference's ten `.fm` files, its `panel.gui` and its three `.sc` shaders, copied byte for
   byte from `/tmp/claude-0/-home-user/0aba2e4b-258a-5dbd-b216-ae73f908e80b/scratchpad/engine-ref/Engine/` (the
   newest version: the folder's own files, not `versions/`); `host.fm`, whose text is the raw string in
   TestingPlayground's `Source/fminus/HostFile.cpp`; and `SEED.tsv` (file, where it came from, sha256). These files
   are never edited afterwards.
7. **`host/`**, a static library `tee_host` on FMinus's public header alone (`fminus/FMinus.h`; no LLVM header):
   - `tee::Program`: compiles a program from a directory (an include resolver over its files; the entry file
     named), with options (optimisation level; `checks`: loop budget and bounds checks on; denormals always
     `Flush`); keeps errors and warnings as FMinus words them, and the seconds the compile took.
   - `tee::Schema`: what `fminus::Program::module()` says, as a tree: every field of a module and of the modules
     it holds, with a path (`cyl[3].p`, `set.size`), its type, offset, size, array extents and attributes.
   - `tee::Instance`: zeroed, aligned memory for a module; typed read and write of a field by path or by a
     resolved handle (resolve once, use on the audio thread: no string work there).
   - `tee::RefPlayer`: plays `sim/ref/program.fm` exactly as TestingPlayground's synth does (its
     `Source/fminus/Patch.h`, lines 1 to 45, is the contract): one `Shared` (its `init(rate)` before the voice's,
     its `end()` after), one `Host`, one `Voice` whose `Shared*` and `Host*` fields point at them; the 32 dials
     set by name through `fminus::Program::setValue`; `start(note, velocity)`, `stop()`, `process(left, right, n)`;
     the wheel's pedals, lever and buttons written into `Host` before each call; `Shared`'s `show*` fields read
     after it.
8. **`tools/tee-render`**: a preset (the dials by name), a scenario (a timeline: dials, keys, pedals, lever,
   buttons at times in seconds), a rate and a block size in; a float WAV and a CSV of the `show*` fields a block
   out. `content/ref/presets/` holds the reference's defaults and its three presets (Large Diesel, V10, V8: the
   values are in the reference folder's `README.md`); `content/ref/scenarios/` holds at least: a cold start to
   idle; free revs in neutral to the limiter and off; a drive in CAR mode through the gears with paddle shifts; a
   truck pulling away. Document both formats in `docs/formats.md`.
9. **`tools/tee-bench`**: for each preset and scenario: nanoseconds a sample of the engine's own 96 kHz rate, the
   share of real time at 48 kHz in blocks of 512, with and without the checks; and the compile's seconds at -O0,
   -O1 and -O2. It writes a table. Commit the table from this container as `docs/measurements/p0-bench.md`, with
   the machine it ran on (the user's plugin reads 30 to 40 % of a core for the same program on an Apple silicon
   Mac: say how this container compares, without pretending the two are the same machine).
10. **Tests** (`tests/`, Catch2, run by `ctest`): the reference compiles with no error (list its warnings in the
    measurements page); `Voice::process` and what it reaches are nonblocking, or say exactly what is not; each
    scenario renders finite and within full scale; the same samples whatever the block size (64, 333, 512) and,
    resampled, the same sound at 44.1, 48 and 96 kHz to a stated tolerance; the engine starts, idles near its idle
    speed, reaches its limiter at full throttle in neutral, and stops; the output's strongest low line at a held
    speed sits at the firing frequency the layout predicts. A helper compares two renders (difference in dB under
    the signal) for K3's use: `tee::test::nullDepth(a, b)`.
11. **The handoff** (01-rules.md §6), plus: the exact commands a new agent runs from a fresh container to build and
    test; the cold and warm build times; anything in FMinus or FunkGui that got in the way.

## Order, and what to push when

K1 steps 1 to 4, then K2 steps 6 to 8 far enough that `tee-render` writes a WAV: push that at once (the lead is
writing the cards that depend on `tee::Program` and `tee::Schema` existing). Then CI (step 5), the bench, the
tests. Build with `-j3`: another agent is building FunkGui, bgfx and SDL3 on the same four cores.

## Not in this card

The new entry module, the Spec, Controls and Telemetry, and the split of `engine.fm` (K3); the audio device and the
window (cards A and B); the interface (card J). Do not design them here; `RefPlayer` is deliberately the old
contract, so that the port can be held to it.
