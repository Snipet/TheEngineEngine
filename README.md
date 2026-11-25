# TheEngineEngine

A standalone application in which a car engine is simulated as physics and heard as the physics' own sound: the gas
in each cylinder, the valves, the pipes as waveguides, the crank, the clutch, the tyres. The simulator is an
[FMinus](https://github.com/Snipet/FMinus) program, compiled when the application starts by the FMinus JIT it carries;
the window is [FunkGui](https://github.com/Snipet/FunkGui)'s. The plan is in [docs/plan/](docs/plan/README.md).

What is here is the keel and what is being built on it. Each directory's own files say more: the table is where to
start.

| Directory | What it holds |
|---|---|
| `sim/ref/` | The reference: the engine program this project starts from, copied byte for byte (`SEED.tsv` says from where). Never edited: it is the baseline the tests compare with |
| `sim/` (the rest) | The simulator being grown from it, by subsystem ([sim/README.md](sim/README.md), [docs/plan/02-contracts.md](docs/plan/02-contracts.md)) |
| `host/` | `tee_host`, the C++ that compiles and plays the simulator, on FMinus's public header alone: `tee::Program`, `Schema`, `Instance`, `RefPlayer`, the offline renderer, and the new engine's players |
| `tools/` | `tee-render` (a preset and a scenario to a WAV and a CSV), `tee-bench` (CPU and compile times), and the tools the other packages add |
| `shell/`, `app/`, `ui/` | The application: the audio device and the inputs, the executables (`tee-headless`, and `TheEngineEngine` with `TEE_BUILD_APP`), the interface ([docs/plan/](docs/plan/README.md)'s cards B and J) |
| `content/` | `content/ref/`: the reference's presets and scenarios ([docs/formats.md](docs/formats.md)); the rest: builds, scenarios and validation files for the new engine |
| `tests/` | Catch2 tests, run by `ctest` (below, "Tests") |
| `cmake/`, `scripts/` | The pins, the dependencies, the flags; the cloud bootstrap, the LLVM fetch, CI's status |
| `docs/` | The plan, the formats, the measurements, the models, the handoffs |

## Building

It needs CMake 3.30 or newer, Ninja, clang, and FMinus's LLVM, which is never built here: it comes prebuilt, as
FMinus's own release archive for the platform. Every other dependency is fetched by git at the commit
[cmake/TeePins.cmake](cmake/TeePins.cmake) pins, into a dependency cache that every build tree and every checkout on
the machine share (`TEE_DEPS_DIR`).

**A cloud container (Ubuntu 24.04, x86-64).** One script, then one command:

```bash
scripts/bootstrap-cloud.sh                    # once a container: apt packages, CMake, LLVM, the sources (about half a minute)
cmake --workflow --preset dev-verify          # configure, build (3 jobs), run every test
```

The script is safe to run again. The cache is `/home/user/.tee-deps`: never delete it, never write into it by hand.
LLVM is FMinus's own release archive (`deps-llvm-23.1.3` of Snipet/FMinus), checked against the release's SHA-256;
a session whose GitHub access does not reach that repository takes the same archive from the mirror, the
pre-release of the same tag on this repository (`TEE_LLVM_MIRROR` names another), checked the same way.

**macOS (Apple silicon).** Xcode's command line tools, and CMake, Ninja and ccache (Homebrew's):

```bash
scripts/fetch-llvm.sh                         # FMinus's prebuilt LLVM, Bison and Flex, into .tee-deps/install
cmake --workflow --preset dev-verify
```

If FMinus is already built on the machine, its own cache serves instead of the download:
`TEE_DEPS_DIR=<FMinus>/.deps cmake --workflow --preset dev-verify`.

**Windows (x64).** From an "x64 Native Tools Command Prompt for VS" with Visual Studio's "C++ Clang tools for
Windows" (FMinus builds with clang-cl, never MSVC's `cl`), in Git Bash for the script:

```bash
scripts/fetch-llvm.sh                         # FMinus's prebuilt LLVM and WinFlexBison
cmake --workflow --preset dev-verify
```

macOS and Windows are built and tested by CI ([.github/workflows/build.yml](.github/workflows/build.yml)). By hand: the
owner runs the app on macOS (Apple silicon), and on 25 Nov 2026 reported that it launches and runs on Windows (`main`
at d0a7330e). Nothing more than that has been checked by a person on Windows (the audio device, input, high-DPI
scaling, the load it runs at): say so before relying on it.

| Preset | Build type | Tree | Workflow (configure, build, test) |
|---|---|---|---|
| `dev` | RelWithDebInfo | `build-dev/` | `cmake --workflow --preset dev-verify` |
| `release` | Release | `build-release/` | `cmake --workflow --preset release-verify` |

Options (`-D`): `TEE_BUILD_TOOLS` (ON), `TEE_BUILD_TESTS` (ON), `TEE_BUILD_APP` (OFF until FunkGui's desktop host is
pinned), `TEE_WERROR` (ON: warnings in our own sources are errors), `TEE_DEPS_DIR`, `TEE_LLVM_DIR` (an LLVM 23.1.3
install of your own), `TEE_COMPILER_CACHE` (ON). A checkout of your own in place of a pin, as it is:
`-DFETCHCONTENT_SOURCE_DIR_FUNKGUI=<dir>` (and `_FMINUS`, `_CATCH2`, `_NLOHMANN_JSON`).

Two things to know about the prebuilt LLVM on Ubuntu 24.04. Its own `opt` wants a newer glibc than Ubuntu 24.04's
(the archive is built on Arch Linux): nothing here runs it; only LLVM's static libraries are linked. And its Bison
wants a library the container lacks, so on Linux the system's Bison 3.8.2 and Flex 2.6.4 are used: the versions FMinus
pins, as Ubuntu packages them.

## Playing the reference

```bash
build-dev/bin/tee-render --list
build-dev/bin/tee-render --preset v8 --scenario free-revs        # out/v8-free-revs.wav and .csv
build-dev/bin/tee-bench                                          # the table of docs/measurements/p0-bench.md
```

`tee-render --help` lists the rest; [docs/formats.md](docs/formats.md) describes presets and scenarios. Nothing a
tool or a test writes is committed: it goes under `out/`.

## Tests

Catch2, run by `ctest` (`ctest --preset dev`; one test by name with `-R`). Adding a test edits no CMake:

- **The host's, the tools' and the reference's**: every `.cpp` in `tests/` is a source of `tee-tests`. Drop a file
  in, with `TEST_CASE`s in it; `tests/support/` has the reference's player, presets and scenarios (`Ref.h`) and the
  measures a test claims (`Audio.h`: `tee::test::nullDepth`, levels, a spectrum's strongest line).
- **A simulator package's** (`sim/<package>/`): its own directory, `tests/sim/<package>/`, with a `CMakeLists.txt`
  of its own, which a glob in `tests/CMakeLists.txt` adds: nobody edits the root's or `tests/`'s CMake to add one.
  That `CMakeLists.txt` begins with a guard, so that a second read does nothing
  (`if(TARGET tee-tests-<package>) return() endif()`); makes one executable, `tee-tests-<package>`, from the
  directory's sources (`tee_target_defaults`, `Tee::host`, `Catch2::Catch2WithMain`); and registers its tests with
  `catch_discover_tests(tee-tests-<package> DISCOVERY_MODE PRE_TEST TEST_PREFIX "<package>." PROPERTIES TIMEOUT 900)`,
  so that `ctest -R '^<package>\.'` runs the package's alone. `tests/CMakeLists.txt` has the rule in full.

Nobody here can listen, and nobody has a screen: a test claims what it measured (samples equal, a difference in dB,
a speed, a spectrum's line), never that something sounds or looks right.

## CI status

Every push to the integration branch builds and tests on Linux, macOS (Apple's clang, arm64) and Windows (clang-cl):
[.github/workflows/build.yml](.github/workflows/build.yml). An agent in a cloud container can build Linux only, so
**after you push, look at macOS and Windows yourself**: a warning that Linux's clang 18 does not give, or a C++
library function Apple's deployment target does not have, shows nowhere else.

```bash
scripts/ci-status.sh            # each platform: the newest run in which its job finished, and its first errors
```

It prints, for each platform, whether the newest finished job passed, the commit it built, its link, and for a
failure the step it failed in and the first error lines of its log. Reading it:

- Pushes to one branch queue behind the run in flight, and **a run that is still waiting is replaced by a newer
  push**: most runs therefore show as "cancelled" without having built anything, and the script skips them. A run
  **in flight is never cancelled** by a push, and its three platforms do not cancel one another. So a commit of
  yours may never be built by itself: it is built with whatever was newest when a run next started. Find your commit
  at or before the commit a result names (`git merge-base --is-ancestor <yours> <theirs> && echo included`).
- A run takes some ten minutes with warm caches. The script lists the run in flight and the one waiting.
- The same by hand, with nothing but `gh`:

  ```bash
  repo=Snipet/TheEngineEngine
  gh api "repos/$repo/actions/runs?branch=claude/awesome-gates-ntdb58&per_page=60" --jq \
    '.workflow_runs[] | select(.conclusion != "cancelled") | "\(.id) \(.status) \(.conclusion // "-") \(.head_sha[0:7])"' | head -5
  gh api "repos/$repo/actions/runs/<run>/jobs" --jq \
    '.jobs[] | "\(.id) \(.name) \(.conclusion // .status) " + ([.steps[]? | select(.conclusion == "failure") | .name] | join("; "))'
  gh api "repos/$repo/actions/jobs/<job>/logs" | grep -nE "error:|FAILED:|\*\*\*Failed|undefined symbol|Undefined symbols" | head -20
  ```

Each run uploads what it built for each platform (the tools, and the application where it builds), also when a test
failed afterwards.

## A window without a screen

`scripts/xvfb-run.sh <command>` runs a command on a headless X display with a window manager and Mesa's software
Vulkan, for what needs a window or a GPU in a container that has neither. Pictures made that way are the software
driver's, not a GPU's.

## Licence

GPL-3.0 ([LICENSE](LICENSE)): FMinus and FunkGui are, and linking them makes it this application's. The
dependencies' own licences are listed beside their pins in [cmake/TeePins.cmake](cmake/TeePins.cmake).
