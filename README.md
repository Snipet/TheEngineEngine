# TheEngineEngine

A standalone application in which a car engine is simulated as physics and heard as the physics' own sound: the gas
in each cylinder, the valves, the pipes as waveguides, the crank, the clutch, the tyres. The simulator is an
[FMinus](https://github.com/Snipet/FMinus) program, compiled when the application starts by the FMinus JIT it carries;
the window is [FunkGui](https://github.com/Snipet/FunkGui)'s. The plan is in [docs/plan/](docs/plan/README.md).

What is here today is the keel: the build, and a C++ host that plays the reference engine program to a file.

| Directory | What it holds |
|---|---|
| `sim/ref/` | The reference: the engine program this project starts from, copied byte for byte (`SEED.tsv` says from where). Never edited: it is the baseline the tests compare with |
| `host/` | `tee_host`, the C++ that compiles and plays the simulator, on FMinus's public header alone: `tee::Program`, `Schema`, `Instance`, `RefPlayer`, the offline renderer |
| `tools/` | `tee-render` (a preset and a scenario to a WAV and a CSV) and `tee-bench` (CPU and compile times) |
| `content/ref/` | The reference's presets and the scenarios ([docs/formats.md](docs/formats.md)) |
| `tests/` | Catch2 tests, run by `ctest` |
| `cmake/`, `scripts/` | The pins, the dependencies, the flags; the cloud bootstrap and CI's helpers |
| `docs/` | The plan, the formats, the measurements, the handoffs |

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

macOS and Windows are built and tested by CI ([.github/workflows/build.yml](.github/workflows/build.yml)) and by
nobody's hands yet: say so before relying on either.

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

## A window without a screen

`scripts/xvfb-run.sh <command>` runs a command on a headless X display with a window manager and Mesa's software
Vulkan, for what needs a window or a GPU in a container that has neither. Pictures made that way are the software
driver's, not a GPU's.

## Licence

GPL-3.0 ([LICENSE](LICENSE)): FMinus and FunkGui are, and linking them makes it this application's. The
dependencies' own licences are listed beside their pins in [cmake/TeePins.cmake](cmake/TeePins.cmake).
