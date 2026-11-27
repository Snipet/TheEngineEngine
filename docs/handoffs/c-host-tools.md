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

## Note, 26 Nov 07:40 UTC: the bake and the predictor against a stated outside pressure (X-046, the lead's question)

Read-only, on the tip f4b03d97+ (sim as d586682c's). The engine is to breathe `spec.env.pressure` (sim/spec/env.fm:16,
default 101325) where today it uses the literal.

**(a) The two baked tables' load axes.**
- **The knock-limit table** (`ecu.maps.knockLimit`) is read by absolute plenum pressure over a fixed 1 atm:
  `knockLimit.at(rpm, pPlenum / 101325)` at sim/combustion/combustion.fm:352. The bake fills that axis the same way:
  cell.load = the predicted point's `tel.engine.manifold` / 101325 (host/src/Bake.cpp:1006), from 0.2 atm up to the
  most a pedal or boost step gave (Bake.cpp:1027-1029). The bake's own reading of the table, its check and report,
  mirrors the simulator's: Bake.cpp:118. Confirmed: absolute pressure over a reference. At 70 kPa outside, the same
  table is read lower down its axis. This 101325 is a reference, not the outside, and must stay a literal on both sides,
  sim and host, together.
- **The intake cam map** (`ecu.maps.camIn`) is not the same kind of axis. It is read at `load` = manifold /
  in->ambient (sim/ecu/ecu1.fm:194, the non-auto branch at :204), and in->ambient is the literal 101325
  (sim/ecu/ecu.fm:317). The bake writes the map's y as "the manifold over the air's" (Bake.cpp:673-678: rows 0, 0.75,
  0.95, 3.0, with the advance only from 0.95 up). So if X-046 makes in->ambient the stated pressure, this axis becomes
  relative to the outside: an engine at full pedal at altitude still reads about 0.95-1.0 and gets the full-load
  advance. That is right for a cam (load relative to what the engine can breathe), but it is a different reference from
  the knock table's. The design should say which it means, and keep the two as they are: knock absolute, cam relative.
  If in->ambient stays 101325, a full-pedal engine at 70 kPa reads about 0.69 and the cam stays parked at full load:
  wrong.
- `maps.spark` under sparkAuto reads `loadRef` = manifold / 101325 (ecu1.fm:195, "the reference's formulas go by
  1 atm"), and the other maps read `load` (ecu1.fm:194). Neither is the bake's, but they have the same split.

**(b) What in host/ and tools/ assumes sea level.**
- **No dyno correction.** Neither the predictor nor the stepped dyno applies SAE J1349 or any density correction. Both
  report the absorbed torque raw (host/src/Predict.cpp reads `tel.engine.torque` and `dyno.tqAbs`; host/src/Dyno.cpp
  DynoPlan::torque "engine.torque"). At a stated 70 kPa, the predicted curve and DYNO read the altitude's uncorrected
  torque, which is right for "what this engine makes here". Nothing in host/ compares it with a sea-level figure, but
  a person reading a brochure number may: that is U1's caption to decide ("uncorrected, at the stated air").
- **No density** anywhere in Predict.cpp or Dyno.cpp; the one boost read is `tel.boost.pressure` (Predict.cpp:71).
- **The idle gate** (Bake.cpp:692-702, checkIdle) compares with `ecu.idle.speed` in rpm (±100 rpm, sd under 25): no
  pressure in it.
- **The boost steps** (Bake.cpp:896-939) set `spec.boost.target` in Pa over the air (gauge), so they follow the
  outside as the ECU's target does.
- **The keys.** No `spec.env.*` field is in the knock, cam or match keys (Bake.cpp:786-793 kKnockFields and
  :802-811; :39-47 movesCam). So a change of the stated pressure leaves every baked table current. That is right only
  if the tables stay valid across pressures; (c) says when they don't.

**(c) Do the baked tables stay valid at another pressure?** It depends on the direction, because the bake measures at
the build's own stated air.
- **Baked at sea level, used at altitude:** valid. The knock table spans 0.2 atm to the most measured. At altitude
  every point reads lower down the same axis, inside what was measured, and knock there depends on the absolute charge
  (pressure and temperature), which the axis already is. The cam map, relative, reads its full-load row at full pedal,
  as it should. No pressure axis is needed.
- **Baked at altitude, used at sea level:** not valid at the top. A build stated at 70 kPa bakes a naturally aspirated
  knock table only up to about 0.7 atm. At sea level its full-load row reads above the measured range and is taken flat
  from the top cell (tee-dyno's "* extrapolated flat"). That under-retards exactly where knock is worst; knock control
  catches it, but the table no longer does its job. And the key does not go stale, because env is not in it.
- **What I recommend instead of a pressure axis:** the bake measures at the reference air. It runs its copies with
  `env.pressure` at 101325 (and `env.temperature` at its default) whatever the build states, and env stays out of the
  keys. One table then serves every stated air, sea level down, read by absolute pressure as the simulator reads it.
  The same holds for the turbine match's rated state (my stage, turbine-match-bake-stage.md): measure it at the
  reference air, so `ratedP4` is a sea-level design point, as a maker's is. That is a host change for the bake's
  window, not X-046's. X-046 needs only (a)'s cam reference decided, and `in->ambient` made the stated pressure where
  the design means the outside.

**Addendum, 07:55 UTC: the temperature half (the lead's three reads).**
- **(a) Today the bake runs at the build's stated air.** Every bake and predictor copy loads the build whole
  (`build.save()`: Bake.cpp:256, 283, 399, 624, 628, 725, 954, 1736; Predict.cpp:1203). Nothing in host/ sets
  `spec.env.*`, so a copy runs at the stated `env.temperature` and `env.pressure`. Yet no `env.*` field is in a key,
  so two builds that differ only in their stated air share the same key while their measured tables differ.
- **(b) Every shipped build with a baked table states 288.15 K,** not the field's default of 300 K (sim/spec/env.fm:11):
  - all 31 archetypes and first.json;
  - of those, 28 have a baked knock table (the three diesels have none) and 15 a baked cam map;
  - all state the pressure at 101325.
  The other off-default temperatures are on builds with no baked table: v8.json, v10.json, large-diesel.json and the
  listeners at 233.15; the combo packs at 293.15; one finding at 322.04.
  So "bake at the field's default" would re-bake every shipped table 12 K hotter and move them all. "Bake at
  288.15 K and 101325 Pa" moves none: every shipped table was baked there.
- **(c) What corrects for a hot day at run time:** knock control, not the table.
  - The knock model (fuel.model 1, sim/combustion/knock.fm:1-20) is Livengood and Wu's integral over Douaud and
    Eyzat's delay, the end gas's temperature T0 · (p / p0)^0.25 from the charge's at the valve's closing. Hotter air
    brings the knock sooner, through the model itself.
  - The baked table is a fixed retard below best torque by speed and absolute load (combustion.fm:351-352). It holds no
    temperature, so on a 49 °C day it under-retards against that day.
  - Knock control (ecu.knock.on, default on: sim/spec/ecu.fm:54; on in all 32 builds) then hears the knocks and
    retards by its steps, giving back about 2° a second: the sawtooth a knock-limited point shows.
  - So a hot day is caught, as a real engine's knock sensor catches it: lightly knocking and retarded closed-loop, not
    pre-retarded by the table. A cold day: the table over-retards a little, which costs a little torque and is safe.
- **So:** the reference air is 288.15 K and 101325 Pa (the standard day the shipped builds already state), not the
  field's default. With that, the recommendation changes no shipped table. A build that states another air gets a
  table baked at the standard day, with knock control covering the difference, as for a person's own build today. If a
  temperature axis is ever wanted, the evidence for it would be a hot-day knock-limited point whose closed-loop retard
  costs more than the table's margin. I have no such reading.

## Note, 27 Nov 16:05 UTC: X-062, the dyno service's superseded curve

**What the owner could see.** DYNO could show the curve of the build before their last edit as the current one, for
as long as the newer run took. The dyno service (host/src/Dyno.cpp, DynoService) published a run that ended
uncancelled while a newer request was already waiting, and a new request did not set aside a result not yet taken.
Cured in 36362b62 (Dyno.cpp alone, +8/−1): a new request sets an untaken result aside, and a run that ends while a
newer one waits is cached under its own key but not handed back. Not in the bake's path: by nm, the bake's objects
reference none of Dyno.cpp's symbols, and tee-dyno's --bake-spark and --bake-cam do not use DynoService.

**How it was found, and the lesson.** One red on a loaded macOS runner (efec3052's run 46019040854, "the service runs
the newest request only"). On this box the case failed 0 times in 400 runs with six cores busy: the race needs the test
thread descheduled for a whole run between two back-to-back calls. Waiting for such a draw finds nothing. The case
written for it, "a curve finished before a newer request is never taken as the newest" (tests/host/DynoTest.cpp),
makes the race certain instead: request A is run to its end first, then B is asked. It fails every time on the old
code. A service with a request-supersedes-request rule should be tested that way, by putting its events in the bad
order on purpose, not by hoping a runner does.
