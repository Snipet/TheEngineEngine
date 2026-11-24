# C: the host and the tools (wave 1, first handoff)

Card [w1-c-host-tools.md](../plan/w1-c-host-tools.md), branch `claude/awesome-gates-ntdb58`. Tests: `tee-host-tests`
(45 cases, `tests/host/`), all passing here; the whole suite passed (219 ctest tests) on the merged tree.

**Done, by test.** Schema (`tee/Fields.h`: FieldInfo for every field, sections and revs, Curve16/Map16 as tables;
`lint()`, `compareSnapshot()`): `[schema]`; lint and `tests/host/schema.snapshot` hold on K3a's `sim/engine.fm`.
`tee-schema` (JSON, `--snapshot`, `--markdown` wrote `docs/parameters.md`, `--lint`). SimHost (`tee/SimHost.h`): command
ring with a pool, rev bumped once a drain, seqlock controls and telemetry, scope only while asked, guard and checks'
stops with a fresh instance off the audio thread, load meter, prepare, two hosts on one Sim: `[simhost]`, `[commands]`
(in order and whole), `[guard]` (planted NaN), `[checks]` (bounds stop with FMinus's words), `[realtime]` (0 allocations
on the audio thread in 2000 busy blocks), `[threads]` (audio loop vs 10 000 fields and tables, a reader). SimHost plays
v8 and large-diesel through free-revs and drive-car bit-equal to EnginePlayer: `[engine]`. Build (`tee/Build.h`): `[build]`
(round trip, unknown fields kept, parts, undo by gesture, diff, hash, Presets). Test library (`Tee::test`,
`tee/test/Scenario.h`, the card's example verbatim): `[scenario]`. Dyno (`tee/Dyno.h`, `tee-dyno`): `[dyno]`, its curve
equal to tee-dyno's. Replay (`tee/Replay.h`): `[replay]`, bit-equal at another block size. Reload (`tee/Reload.h`):
`[reload]`, speed carried across, by the audio thread's snapshot too. Validation (`tee-validate`,
`content/validation/` x3, `content/scenarios/` x2): `[validation]`. Formats: `docs/formats.md` (appended).

**Measured** (this container, dev build, shared 4 cores). Host's own work a 512-sample block: 1.03 us mean (6 us peak)
on the V8, 1.05 us on the stand-in (budget 20). Dyno runner: 209x real time on the stand-in; the V8 through the test
library 1.9 to 4.2x real time. Real-engine dyno: not measurable yet (below).

**Not done / not run.** ThreadSanitizer: the `tsan` preset is in, but never ran here: the container's clang has no
`libclang_rt.tsan` and installing it was refused. tee-render's build-and-scenario path (K3 owns `--engine`), tee-bench's
breakdown by module (the keel's file; no keel handoff yet). `--hq` is wave 2. CI (run 45399202005): Windows compiled
all of card C's sources clean (it stopped on D's `M_PI`); macOS ran tee-host-tests, and 38 passed every assertion then
crashed at exit: Tee::test's static cache of Sims outlived LLVM's statics. Fixed (never-destroyed cache, e69ea09);
the next macOS result is not seen yet.

**Waits for K3b/D.** The dyno on the real engine: `ctl.mode` MODE_DYNO_ENGINE and a brake torque in
`tel.engine.torque` (N m); the validation's torque and power targets read `n/a` until then.

**Changes I want outside my paths.**
1. `host/include/tee/EnginePlayer.h` (K3) declares `struct tee::Build`, the contract's name for the document: two
   classes of one mangled name in `tee_host`. Mine is in `inline namespace document` meanwhile; please rename K3's.
2. Contract 7 names `setControls(const ControlsFrame&)`, but shell's `Sources.h` already defines `tee::ControlsFrame`
   (fewer fields). Mine is `tee::SimControls` (contract 4 complete); the shell adapter converts.
3. `scripts/bootstrap-cloud.sh`: add `libclang-rt-18-dev` so the tsan preset can run.
4. Telemetry: `tel.engine.firing` (Hz) would let tests skip a cylinder count (`Run::firingHz` falls back to speed).
5. Additive edits I made to shared lists: `tests/CMakeLists.txt`, `host/CMakeLists.txt` (one `add_subdirectory`
   each), `CMakePresets.json` (tsan).

**Look at / listen to.** Validation fails against its sources, as the reference has it: V8 warm idle 1221 rpm, diesel
idle 1129 rpm and governed speed 2600 rpm. The V8's first second has a step of 30x the local level (the starter's onset
out of silence); once running it reads 5 to 10, under `clickLimit` 12. Listen to `idle` and `rev-limit`.
