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

# C: the host and the tools (wave 1, second slice)

Same card and branch. `tee-host-tests`: 51 cases, all passing here. The full suite passed except ContractTest's
attribute check on G's new `spec.exhaust.*` fields (not mine).

**Done, by test.**
- macOS: the run of 9f23b8b passed every host test (353 tests ran; 2 UI failures). host/ and tools/ hold no static that owns a Program.
- One `tee::Build`: K3's struct is now `tee::EngineBuild`; the document is out of its inline namespace. One line in `shell/` (SimAdapter.h) and one in `tests/sim/core/Engine.h`.
- The ribbon's curve: `tee/Predict.h` (`PredictService`: request, take coarse then refined, progress, cancel, cache by hash; `predictCurve`, `predictRamp`): `[predict]`, three on the stand-in and one on the real V8.
- `tee-render --build --scenario` through SimHost, 24-bit with `--pcm24`, exit 3 on a failed assertion: `[render]`, sample for sample against `runTimeline`.
- `LoadOptions::hold` and `tee/Warm.h` (a warm engine held, offline only), and a scenario's `"warm": true`: `[simhost]`.
- tee-validate: torque and power measured on the steady points; the reference's column beside the simulator's, with what each misses; a reference miss is reported, not failed.

**Measured** (this container, 3 workers, idle machine).
- V8 predicted curve: first 0.33 to 0.41 s, refined 0.59 to 0.65 s (16 speeds, 7 to 9 s of engine time). Large diesel: 1.2 and 1.5 s, because its turbo has to spool at the start.
- Accuracy against points held 300 s each: 0.8 % mean, 4.4 % worst of the peak torque. Against a 10 rad/s^2 sweep: 1.3 % and 6.2 %.
- Host brake against D's absorber: within 2.5 %.
- Without settling the exhaust: twice as fast, but 4.3 % mean and 10.3 % worst; even 9 plain cycles leave 2.5 % and 4.3 %.
- Cycles a point: 9 (1 for the speed change, 6 settling exhaust and turbo, 2 within 1 %).
- Validation, warm: V8 idle 1240 rpm (target 600 to 900), peak torque 696 N m (490 to 690), 347 kW. Diesel idle 1121 rpm (600 to 800), governed 2600 rpm (1800 to 2400), peak 2164 N m (in range). Default build: all pass.

**How the curve is made, and why** (Predict.h says it at length).
- Steady points at held speeds, torque averaged over whole cycles.
- The host's brake (crank.omega held, torque = J dω/dt) where engine.custom is off; D's dyno otherwise. D's 6 Hz loop needs 5 to 10 more cycles a point and overshoots into the limiter.
- The engine held warm: `Heat::stood` resets a one-off write to ambient, and at full load the coolant overheats over minutes.
- The exhaust's slow temperatures (exh.tPort 0.8 s; exh.tWall minutes) and the turbo's spool settled by a two-drift secant: without it the curve depends on how long the engine ran (−27 % at 6000 rpm cold against hot).

**Not done / not run.**
- ThreadSanitizer: nothing received from the steward yet.
- The interface's DynoSource over PredictService is the shell's (mapping sent to the lead).
- `--hq` is wave 2.

**Wanted outside my paths.**
1. G or D: a sim-side settle (a ctl level under which Exhaust puts tPort and each tWall at their equilibrium, Boost its spool), to replace the host's secant on those paths.
2. G: attributes for `spec.exhaust.primaryTo`, `collectorTo`, `sides` and the rest (ContractTest fails).
3. Packages whose tests keep a Program in a static with a destructor: exhaust, boost, core, two-stroke and valvetrain rigs, ContractTest, NullTest (01-rules §4).
4. Still open from the first slice: `libclang-rt-18-dev` in the bootstrap.

**Look at.** `tee-dyno --predict --build content/builds/v8.json [--ramp 10 | --hold 300]`; `tee-validate`.
