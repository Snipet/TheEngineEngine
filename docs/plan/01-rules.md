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
- **`main`** is the repository's default branch (the owner's, since 2026-11-24 20:05 UTC). Only the lead moves it,
  and only forward: at each lead check-in, when the integration branch's tip compiles, `scripts/lint.sh` is clean and
  the newest finished fast-tier job on each platform is green, the lead fast-forwards `main` to that tip
  (`git push origin <sha>:refs/heads/main`, never forced); otherwise `main` stays and the status file says why. No
  package pushes to `main`, opens a pull request to it, or bases work on it: the integration branch is ahead of it.
- Commit messages: `area: what changed` in the imperative, a body when the why is not obvious. End every message
  with the trailers your own session's instructions give. No model names anywhere else in the repository.
- Push after every step that builds and passes its tests. Unpushed work is lost when a container is reclaimed.
- **Green before a push** (the lead's rule, policed by the build steward): on a freshly merged tree, your own
  package's tests and every test of a package whose files you touched pass (`ctest --preset dev -R '^<package>\.'`),
  and `scripts/lint.sh` is clean. A test that fails for a filed finding is skipped with the finding's number in its
  reason (`SKIP("finding F-12: ...")`), never left red. A soak or a fuzz is tagged `[nightly]` (§3). The steward
  messages each owner its own failing tests after every full run and every finished CI job.
- **A long test never holds a push back twice** (the lead's rule, 2026-11-24 15:55 UTC: the null test takes eight
  minutes and the branch moves forty commits in that time, so "green on a freshly merged tree" never came true for
  the packages that edit `sim/engine.fm`, and finished work sat unpushed for an hour). Run your tests once on a tree
  merged at that moment. When they pass, fetch and merge again, check that the program still compiles (five seconds:
  `scripts/lint.sh` compiles `sim/` with `tee-schema --lint` when a build tree has it) and push at once, in one
  command, whatever the branch did meanwhile:
  `git fetch origin <branch> && git merge --no-edit FETCH_HEAD && scripts/lint.sh && git push -u origin <branch>`.
  (The compile check was added at 16:28, half an hour into the rule: two changes that merged without a conflict,
  a function given a fourth argument and a new call written with three, left the tip not compiling for six
  minutes. A merge can break what neither side broke; the check catches the kind that stops everyone.) Only a
  merge that conflicts in a file your change touches earns a second look: resolve it, build (the program must
  compile), run the one test nearest the conflict, push. The sum is the steward's to check (its full runs and CI),
  and it tells the owner whose change broke it.
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
- **Test time** (the lead's rule): a test in the default run takes under 30 s on a CI runner, and CI's whole Test
  step under 10 minutes on Linux. A longer property test or soak is **split**: a quick form stays in the default
  run (the same property on a small fixed sample, the corners that have failed before first), and the full form
  carries the Catch tag `[nightly]` (registered with `ADD_TAGS_AS_LABELS`, the rule in `tests/CMakeLists.txt`), which
  the dev and release presets leave out and `ctest --preset nightly` runs. CI's soak tier runs the `[nightly]` tests
  on Linux at every push, and on macOS and Windows when the head commit's message contains `[soak]`. Each CI job's
  summary lists the default-run tests over 30 s with their package; the build steward tells their owners.
- **A budget a package** (the rule's second half: tests under 30 s each do not by themselves bound the whole): the
  fast tier's CPU-seconds on CI's Linux runner, the sum of a package's test times. The whole fast tier is capped at
  2400 (ten minutes of wall on four cores), and that cap binds: it is shared out by the lead's weights scaled by
  2400 / 4500: `null` 600 (the guardian, kept whole), `diesel`, `exhaust` and the host 135, every other package 80,
  and `combo` what is left, 120. What does not fit goes to the soak tier, by the owner's choice of what
  guards most per second. `scripts/test-budget.sh <ctest log>` prints the table (each fast CI job's summary has it);
  run it on your own `ctest --preset dev | tee ctest.log` before a push that adds tests.
- **One compile a process** (the lead's decision, 2026-11-24): compiling sim/ costs about 6.6 CPU-s, and
  catch_discover_tests runs each case in a process of its own, so a package with ten simulator cases pays it ten
  times. Group a package's simulator-heavy cases into one ctest entry (e.g. `add_test(NAME <pkg>.sim COMMAND
  tee-tests-<pkg> "[sim]")`, the light ones discovered as before), with one kept player or Sim a process, as the
  shell and the host do (D2's "one kept player a process, reset per test" took diesel from 321 to 202 CPU-s). The
  caution: a kept player is **reset** between cases to the state a fresh one has, so that no case passes because of
  what the one before it left behind; prove it once by running the cases in reverse order (`--order rand` or the
  list reversed) and say so in the commit. The 30 s limit is then each case's; a grouped entry may take up to 90 s
  (longer, and it is split into two entries, so that ctest's four jobs still pack the tier); the summary's list of
  default-run tests over 30 s names grouped entries too: read them against 90.

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
- **Found the hard way, 2026-11-24** (X-007, X-019): a float is never an index (or a loop's bound) without a clamp at
  both ends and a non-finite check. A NaN's cast to int is INT_MIN on x86 and 0 on arm64, so macOS hides the read
  off a table that Linux and Windows crash on, in a shipped build, on a Spec inside its ranges. `if (i > last) i =
  last;` alone does not stop it (a NaN compares false with everything). Call `tableIndex(x, last)` or write
  `(int)fminf(fmaxf(x, 0.0f), (float)last)` (FMinus's fminf and fmaxf return the other operand for a NaN); wrap an
  angle with `wrapCycle` (sim/core/mathx.fm), never `while (a >= cyc) a -= cyc;`. `scripts/lint.sh` (rule 8)
  fails on a new one. CI's soak tier will run the fuzz at the corners without bounds checks (X's next push), so a crash is a crash there.
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
