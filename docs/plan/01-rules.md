# Rules for every work package

*Stage 2. Read this, then your card, then [02-contracts.md](02-contracts.md) if your card touches `sim/`, `host/` or
`ui/`. [00-abstract-plan.md](00-abstract-plan.md) is the why; a card is the what. Where they differ, the card wins,
and the lead (the session that wrote these files) settles what a card leaves open.*

## 1. What you may change

- **Your card's `OWNS` paths, and nothing else.** Everything else is read-only to you. If your work needs a change
  outside them, do not make it: say so in your handoff, with the exact change you want, and work around it meanwhile.
- **The contracts** (`sim/spec/*.fm`, `host/include/tee/*.h` named in 02-contracts.md) are shared. You may **append**
  fields to the Spec, Controls and Telemetry sections your card names; you never rename, reorder, retype or delete a
  field, and never change a seam's signature. A change of that kind is the lead's.
- **FMinus** (the compiler): never edit `src/`, `include/` or `tools/fminus/` there. The user writes the compiler. A
  bug or a missing feature is a report to the lead: the smallest program that shows it, what you expected, what
  happened, and how you worked around it.
- **FunkGui**: only the FunkGui package (card A) and packages the lead names edit it, on a branch, through a pull
  request the lead merges. Its existing goldens are never re-blessed to make a test pass.
- **TestingPlayground**: read-only reference. Code copied from it is listed in `ui/kit/SEED.tsv` (file, source
  path, source commit, sha256 of the source file).
- **The reference** (`sim/ref/`): never edited after Phase 0 copies it. It is the baseline the tests compare with.

## 2. Repositories, branches, commits

- TheEngineEngine's integration branch is `claude/awesome-gates-ntdb58`. Your card names your branch. If it is the
  integration branch itself, commit small and often, and before every push run
  `git fetch origin claude/awesome-gates-ntdb58 && git merge --no-edit FETCH_HEAD`; never force-push, never rebase
  what you have pushed.
- Commit messages: `area: what changed` in the imperative, a body when the why is not obvious. End every message
  with the trailers your own session's instructions give. No model names anywhere else in the repository.
- Push after every step that builds and passes its tests. Unpushed work is lost when a container is reclaimed.
- Never commit build trees, dependency caches, rendered audio or pictures. `.gitignore` covers `build-*/`, `out/`
  and `.tee-deps/`; test artefacts go under `out/`.

## 3. Building in a cloud container

- `scripts/bootstrap-cloud.sh` (Phase 0 writes it) makes a fresh container able to build: a CMake of 3.30 or newer,
  the system packages, FMinus's prebuilt LLVM, the dependency sources by git. Run it once; it is idempotent.
- The dependency cache is `TEE_DEPS_DIR` (default `/home/user/.tee-deps`), shared by every checkout on a machine:
  never delete it, and never write into it by hand.
- This machine has 4 cores and other agents may be building. Build with `-j3` unless your card says otherwise, and
  use the compiler cache the presets configure (ccache).
- Release archives fetched by URL are refused here (HTTP 403). Fetch by git at a pinned commit. GitHub Actions
  runners and the user's machines have no such limit; do not weaken a pin to suit the cloud: add a git route.
- **Before every push**: `scripts/lint.sh` (a second: the platform rules Linux cannot see). **Before a push that
  touches a header others include** (`host/include/`, `sim/`'s shared modules, a test support header such as
  `tests/support/` or a package's `Rig.h`), build every target, not only yours:
  `cmake --build --preset dev -j3 && scripts/lint.sh`. A rename that breaks another package's build breaks every
  session's next run (the build of 6651274). CI's Linux job builds everything first and fails within about three
  minutes; look with `scripts/ci-status.sh` after you push.
- Long soak tests (minutes) carry the Catch tag `[nightly]` and are left out of the default runs and CI; `ctest
  --preset nightly` runs them (the rule in `tests/CMakeLists.txt`).

## 4. Code

**C++** (`host/`, `shell/`, `ui/`, `tools/`): C++20; namespace `tee`; warnings are errors in our code (`-Wall
-Wextra -Wshadow -Wpedantic`), never in dependencies; `-ffp-contract=off`, never `-ffast-math`. No exceptions, locks,
allocation or logging on the audio thread. No JUCE, anywhere. Third-party code only through CMake pins in
`cmake/TeePins.cmake` (name, repository, commit, licence).

**FMinus** (`sim/`):
- Everything `Engine::process` reaches is `[[clang::nonblocking]]`. Memory is taken in `init` and given back in
  `end`, never in between. Arrays are fixed-size; the maxima are in `sim/spec/limits.fm`.
- State lives in modules; a module file starts with a header comment in the reference's style: what the thing is
  physically, what it models and what it leaves out, with the numbers and the sources.
- **Off means the reference.** Your subsystem has a setting (its Spec defaults) at which the simulator behaves as it
  did before your package. Your tests prove it.
- **Stability is a requirement, not a tuning matter.** A new element is passive at its junction (it never gives a
  wave more energy than it took), is solved in closed form or by a fixed number of passes, and has a soak test.
  Read the reference's comments where it explains why something was done the hard way before replacing it.
- **Found the hard way, 2026-11-24** (add to this list when you find another): an explicit step of a
  square-root-of-pressure-drop orifice between two volumes never settles: at rest it chatters about zero for ever
  (the two-stroke's transfer ports sloshed 4.6 g/s with the engine stopped). Solve the flow together with the drop
  it changes, in closed form, as the reference's `lossFlow` does; and test "at rest, nothing flows". With an
  inertia that varies round the turn, an explicit `dw/dt` step of the crank loses energy (15 % in 20 s on a free
  single): step the kinetic energy instead (`sim/core/inertia.fm`).
- **Found on macOS, 2026-11-24**: never keep a `tee::Program`, or anything that owns one, in a static with a
  destructor. At exit it is destroyed after FMinus's and LLVM's own statics and its destructor runs on a JIT that is
  gone (38 tests crashed at exit on macOS; Linux's order hides it). Free it before `main` returns.
- No transcendental function per sample where a table or a control-rate update will do. Your card gives a CPU
  budget; `tee-bench` measures it.
- Every Spec field has `[[fminus::range]]`, `[[fminus::default]]`, `[[ui::label]]` and says what its number is:
  one of `[[ui::quantity]]`, `[[ui::unit]]` or `[[ui::choices]]` (a `bool` needs none). SI units inside; the
  interface converts. By kind:
  - a physical quantity: `[[ui::quantity("length")]]` (one of 02-contracts.md §3's list: the user picks its unit);
  - a fraction or a ratio: `[[ui::quantity("fraction")]]`, `[[ui::quantity("ratio")]]`;
  - a level or anything else with a fixed unit: `[[ui::unit("dB")]]`, `[[ui::unit("deg")]]`;
  - a count: its unit, plural: `int gears [[fminus::range(1, 18)]] [[fminus::default(6)]] [[ui::label("Forward
    gears")]] [[ui::unit("gears")]];` (as `engine.cylinders` has "cylinders");
  - a kind, or a switch of more than two ways: `[[ui::choices("Layout's", "Single", "Dual")]]`, one name for each
    value of the range from its minimum (a special value such as -1 "the layout's" is a choice too);
  - a table: the table field carries `[[ui::label]]` and `[[ui::axes("rotation", "ratio")]]`; a `Map16` also its z's
    `[[ui::unit]]` or `[[ui::quantity]]`; the table's own x, y, z, n need nothing.
  `scripts/lint.sh` runs `tee-schema --lint` on `sim/engine.fm` when a build tree has it and names each field and
  what it lacks; the contract test fails on the same.

**Comments and docs**: say what and why in plain words, as the reference and FMinus's manual do. No decoration.

## 5. Proof

- A package is done when its card's tests pass in a fresh container after `bootstrap-cloud.sh`, the whole test
  suite still passes, and `tee-bench` is within budget.
- Nobody here can hear. Do not claim a sound is good; claim what you measured (a spectrum's lines, a level, an
  envelope, a number against a published range) and list the scenarios a person should listen to.
- Nobody here has a GPU or a display. Pictures come from the software rasteriser, or from the real GPU path on
  Xvfb with Mesa's software Vulkan (`scripts/xvfb-run.sh`); say which.
- Say plainly what was not run, on which system. macOS and Windows are proven by CI builds and by the user.

## 6. Handoff

End with a message of at most 40 lines: the package and branch; what is done, by test name; what is not done and
why; measurements (CPU, compile time, sizes); every change you want outside your paths; every FMinus or FunkGui
issue met, with its workaround; what a person should look at or listen to. Put the same text in
`docs/handoffs/<package>.md`.
