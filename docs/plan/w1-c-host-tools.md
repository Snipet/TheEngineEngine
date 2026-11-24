# Card C: the host and the tools

*Wave 1, startable as soon as K2 has landed (and its `SimHost` as soon as K3a has). Repository: TheEngineEngine,
integration branch `claude/awesome-gates-ntdb58`. Read [01-rules.md](01-rules.md), [02-contracts.md](02-contracts.md)
whole (§3, §7 and §8 are yours to make real), the keel's handoff (`docs/handoffs/`), and for precedent
TestingPlayground's `Source/dsp/Engine.h` and `Source/fminus/Patch.h` (read-only: how a host plays an FMinus
program safely: handovers, a stopped program, the output guard, meters through a seqlock) and FMinus's
`docs/research/desktop/experiments/host.cpp` (state carried across a reload by field name).*

**OWNS**: `host/**` (from the keel, once its handoff is in; `EnginePlayer.*` stays K3's until K3 is done),
`tools/**`, `tests/host/**`, `content/scenarios/**`, `content/validation/**`, `docs/formats.md`,
`docs/parameters.md` (generated).

**Budget**: the host's own work in a block at most 20 microseconds besides `Engine::process`; nothing on the audio
thread allocates, locks or logs, and a test proves it.

## 1. The Schema

`tee::Schema` grows from K2's tree into what 02-contracts.md §3 describes: every field's `FieldInfo` (path,
type, extents, offset, range, default, label, quantity or unit, choices, taper, decimals, group, help, hidden,
axes), `Curve16` and `Map16` recognised as tables, sections and their `rev` fields found. **A lint, run as a
test**: every Spec field has a label, a range and a default; every quantity is one of the list; every choice
count matches its range; no path changed type or disappeared since the last committed snapshot of the schema
(`tests/host/schema.snapshot`: append-only, as the contract says). This test is how the contract is enforced on
every sim package. `tee-schema` prints the schema as JSON and writes `docs/parameters.md`.

## 2. `SimHost`

The threaded host of 02-contracts.md §7.
- **Commands**: a single-producer single-consumer ring of writes (an offset, a size, the bytes inline up to 16,
  else a block from a preallocated pool for tables); the audio thread drains it before `process`, bumps the
  section's `rev` once per drain, never waits. A full ring makes `set` return false and the caller retry next
  frame; count it.
- **Controls**: the newest frame wins (a seqlock or a triple buffer).
- **Telemetry**: after each block the `tel` block's bytes through a seqlock; the Scope only while asked, the
  request setting `ctl.scopeOn`.
- **Safety**: flush-to-zero on the audio thread; an output guard (not finite, or beyond full scale: fade out,
  re-initialise the instance off the audio thread, raise a flag: never a blast to the speakers); in a development
  build FMinus's loop budget and bounds checks, and their stops handled the same way with the compiler's message
  shown.
- **A load meter**: the callback's time over the buffer's, mean and peak, in telemetry: the number the user
  quoted for the plugin.
- `prepare` for a new rate or block size; two instances at once (the one that plays, a worker's).

## 3. The document

`tee::Build`: values by path in SI; defaults and clamping from the Schema; JSON as 02-contracts.md §8 (whole, or
the engine, the car or the tune alone; unknown fields kept and written back); a diff of two builds; **undo and
redo** of changes grouped by gesture; a hash of its content (the key of the dyno's cache). It sends each change
to a `SimHost`. `tee::Presets`: the builds under `content/builds/` and the user's folder.

## 4. Reload, for development

Watch `sim/`; on a change compile on a worker; on success allocate the new program's `Engine`, `init` it, carry
the state over by field name and type (nested modules field by field; a new field starts at 0; an array whose
size changed starts afresh), swap under a 20 ms crossfade, free the old one off the audio thread; on failure
keep playing and report clang's words. A switch (`TEE_DEV`), off in release.

## 5. The dyno runner

`tee::DynoRun`, on a worker with an instance of its own: a copy of the build; a plan (engine or chassis; from,
to, a rate, or steps with a settling time; the pedal); warm-up; then the points: speed, torque, power, and every
telemetry field asked for (boost, lambda, volumetric efficiency, knock, exhaust temperature). Progress, cancel,
a cache by the build's hash and the plan. It runs as fast as the machine allows; report how many times real time
that is. The interface asks for a predicted curve a moment after each edit; older requests are dropped.

## 6. Tools

- `tee-render`: a build and a scenario to a WAV (float or 24-bit) and a CSV of chosen telemetry; `--rate`,
  `--block`; later `--hq` (twice the internal rate, wave 2).
- `tee-dyno`: a build and a plan to a CSV and a small text chart.
- `tee-bench`: K2's, kept; adds a breakdown by module when the build is profiled (FMinus gives code ranges by
  function; sample the thread: see how TestingPlayground's `dsp/Profile` does it, or simpler, time builds with
  each subsystem switched to its default).
- `tee-schema`; `tee-validate` (§8).

## 7. Scenarios

The format (`docs/formats.md`), growing K2's: a build or preset, the rate and block, a timeline (controls and
Spec changes at times; ramps), and assertions. And the **test library** every sim package uses, so their tests
read as sentences:

```cpp
tee::test::Run r = tee::test::Scenario("builds/default.json")
                       .at(0.0).ignition(tee::Start).until(1.5).ignition(tee::On)
                       .at(3.0).throttle(1.0).ramp(0.3)
                       .run(8.0);
CHECK(r.tel("engine.speed").during(6.0, 8.0).max() > 0.95 * r.tel("engine.redline").last());
CHECK(r.audio().finite());  CHECK(r.audio().peak() <= 1.0f);
CHECK(r.audio().during(6.0, 8.0).line(r.firingHz()).above(r.audio().during(6.0, 8.0).floor(), 12 /*dB*/));
CHECK(r.audio().largestStep() < tee::test::clickLimit);
```

Audio measures: finite, peak, RMS over a window, a line's level at a frequency (Goertzel), a spectrum (a plain
FFT), the largest step between samples against the local level (a click detector), the null depth between two
runs. Telemetry measures: mean, min, max, last, the time a threshold is crossed, settled-within.

## 8. Validation

`content/validation/<build>.json`: ranges a build must meet, each with its source in words (a textbook's figure,
a rule of thumb, a published specification of the kind of engine it stands for). `tee-validate` runs the dyno and
the scenarios they need and prints a table: target, range, measured, pass. `ctest` runs a fast subset. Wave 2's
content package fills the targets; you build the machine and three examples.

## 9. Replay

A session log: every `ControlsFrame` change and Spec change with its sample time. Rendering a log again gives
the same samples (test it). Wave 2's cameras and exports are built on this.

## Tests

The schema lint and snapshot. A build round-trips through JSON; an unknown field survives; a partial load
touches only its part. `SimHost` under a made-up audio thread that calls `process` in a tight loop while another
thread sets ten thousand fields and tables: under ThreadSanitizer (a `tsan` preset), no report; with an
allocation counter on the audio thread, zero. Commands arrive in order and whole (a table is never seen half
written). The guard catches a planted NaN. A reload carries a running engine's speed across. The dyno runner's
curve equals `tee-dyno`'s. A replayed log's samples equal the original's.

## Not in this card

The audio device, input devices, settings (card B). Widgets and the binding of a field to a control (card J;
you give it `Build` and `FieldInfo`). Any FMinus code.
