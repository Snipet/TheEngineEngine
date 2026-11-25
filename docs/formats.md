# Files: presets and scenarios for the reference

Two JSON formats, both read by `tee_host` (`host/include/tee/Scenario.h`) and used by `tee-render`, `tee-bench` and
the tests. They are the reference player's: a preset is the reference's 32 dials, a scenario what its player does.
The new engine's builds and scenarios (docs/plan/02-contracts.md, section 8) grow from these and get their own
section here when they exist.

A file may hold `//` and `/* */` comments. A key the format does not name is an error in an event (a misspelt
`"thrtle"` should not pass silently) and ignored at the top level.

## A preset: `tee-ref-preset/1`

The dials by name. `content/ref/presets/` holds the reference's own: `defaults` (every dial at its
`fminus::default` in `program.fm`; `tee-render --defaults` writes it), `large-diesel`, `v10` and `v8` (the three
presets of the reference's README), and `bench-v12`, which is not the reference's: its V10's dials with the V12
layout, for the bench's heaviest case.

```json
{
  "format": "tee-ref-preset/1",
  "name": "V8",
  "description": "The reference's preset V8 (from v18), as its README.md lists the dials.",
  "dials": {
    "engine": 12,
    "size": 6.2099,
    "cam": 0.9999
  }
}
```

| Key | |
|---|---|
| `format` | `"tee-ref-preset/1"`. Required |
| `name`, `description` | Text. The name defaults to the file's |
| `dials` | An object: a dial's name to its value, a number (a boolean counts as 0 or 1). Required. A dial left out keeps the program's default; a name the program does not have is an error when the preset is played. Values are the dial's own units, as `program.fm` declares its range: nothing is clamped |

The dials, their ranges and what each does are `program.fm`'s `param` lines and its opening comment
(`tee-render --schema Voice` prints them with their attributes).

## A scenario: `tee-ref-scenario/1`

A timeline: what happens to the dials, the keys, the pedals, the lever and the buttons, at times in seconds from the
start. `content/ref/scenarios/` holds four: `cold-start-idle`, `free-revs`, `drive-gears` and `truck-pull-away`
(`tee-render --list` prints each with its description).

```json
{
  "format": "tee-ref-scenario/1",
  "name": "free-revs",
  "description": "Free revs in neutral: start, idle, blips, flat out to the limiter and held on it, the overrun, and off.",
  "seconds": 24,
  "events": [
    { "t": 0.0, "dials": { "drive": 1, "ambient": 80 }, "input": { "gear": 0, "clutch": 0 } },
    { "t": 0.25, "note": "the start key", "key": 36, "velocity": 1.0 },
    { "t": 0.75, "keyUp": true },
    { "t": 8.0, "input": { "throttle": 1 }, "ramp": 0.15 },
    { "t": 14.0, "input": { "throttle": 0 }, "ramp": 0.05 }
  ]
}
```

| Key | |
|---|---|
| `format` | `"tee-ref-scenario/1"`. Required |
| `name`, `description` | Text. The name defaults to the file's |
| `seconds` | How long the scenario lasts. Required, above 0 |
| `events` | An array of events, in any order: they are sorted by time, and events of one time keep the file's order. Required |

An event is an object with `t`, its time in seconds, and any of:

| Key | What happens at `t` |
|---|---|
| `dials` | An object of dials and values, as a preset's: each is set, over the preset's value |
| `input` | An object of inputs and values: `throttle`, `brake`, `clutch` (the pedals, 0 released to 1), `gear` (the lever: -1 reverse, 0 neutral, 1 to 7), `steering` (-1 to 1; the reference does not read it), `modwheel` (0 to 1: the throttle by hand), `pitchbend` (-1 to 1), `aftertouch` (0 to 1), `sustain` (0 or 1) |
| `ramp` | With `input`: the inputs move to their values in a straight line over this many seconds, from what each was last set to (0 before anything set it), in a step every 5 ms, the last at `t + ramp` |
| `press`, `release` | A button's name, or an array of them: they go down, or come up. `paddleUp` and `paddleDown` are the wheel's right and left paddles, `start` its PS button; the others are `host.fm`'s (`cross`, `square`, `circle`, `triangle`, `r2`, `l2`, `share`, `options`, `r3`, `l3`, `plus`, `minus`, `dialRight`, `dialLeft`, `return`, `up`, `right`, `down`, `left`) |
| `tap` | The same names: down at `t` and up again `hold` seconds later (0.1 when `hold` is not given). A paddle shift is a tap |
| `key`, `velocity` | A key goes down: its MIDI note number and how hard, 0 to 1 (1 when not given). The reference has one engine, played by the key that went down last, so a second `key` replaces the first |
| `keyUp` | `true`: the key comes up |
| `note` | A remark for the reader. Ignored |

What the keys and the wheel mean is the reference's (`program.fm`'s opening comment). In short: with the `drive`
dial at 0 the keys rev the engine (note 36 asks for the idle speed, 84 for the redline; the first key cranks it, and
it idles on for `hold` seconds after the last key comes up); at 1 or 2 it is in a car or a truck, where any key is
the start key (it cranks in neutral or with the clutch down, and held a second while the engine runs switches it
off), the pedals and lever drive it, and with the lever in neutral the paddles shift.

### How a scenario is played

`tee::render` (`host/include/tee/Render.h`) resets the player, sets the preset's dials, and plays the timeline.
Each event takes effect at its own sample, its time times the sample rate, rounded, whatever the block size: a
call of `process` is cut short where the next event falls, so the block size is the longest call, not every
call's length. That is why two renders of one scenario at different block sizes are the same, sample for sample
(`tests/ReferenceTest.cpp` holds them to it).

Across sample rates the same scenario is the same sound, but not the same samples for long: at 48 and 44.1 kHz the
engine, which always runs at 96 kHz, runs some 50 of its samples ahead of the output, so an input reaches it half a
millisecond earlier in its own time than at 96 kHz, and the gas is chaotic enough to turn that into different
waveforms within seconds. The tests state how alike the renders stay.

## What `tee-render` writes

- **The WAV**: stereo, 32-bit floats (format 3), at `--rate`. The samples as the program made them: no dither, no
  gain, no conversion. `out/<preset>-<scenario>.wav` unless `--out` says otherwise.
- **The CSV**: a row after every call of `process`: `time` (seconds from the start, at the call's end), then every
  field of the reference's `Shared` whose name begins with `show`, in declaration order: `showRpm`, `showRed`,
  `showGear`, `showKmh`, `showThr`, `showMap`, `showLamps`, `showPedals`, `showCycle`, `showBoost`, `showAir`,
  `showPipes`. `engine.fm` says what each holds where it declares them; some are packed for the panel's shader
  (`showLamps` is the engine's state, 0 off, 1 cranking, 2 running, 3 running down, plus 4 on the limiter, plus 8
  with the fuel cut, plus 16 when driven, plus the dash's temperature and oil pressure in its higher bits).

Nothing either tool writes is committed: both go under `out/`, which git ignores.

# Files for the simulator (card C)

The reference's files above are K2's and stay as they are. What follows is the simulator's (`sim/engine.fm`), read
and written by `tee_host` (`tee::Build`, `tee::Timeline`, `tee::Validation`, `tee::SessionLog`) and by the tools on
it. Every one is JSON with a `"format"` and a `"version"`; a reader refuses another format and reads a newer version
as far as it goes.

## A build: `tee-build`, version 1

What the machine is: the Spec's values (02-contracts.md, section 8). `content/builds/` holds the shipped ones; the
user's are in their own folder (`tee::Presets::userDirectory()`). `tee::Build` reads and writes them.

```json
{"format": "tee-build", "version": 1, "name": "V8", "notes": "...",
 "spec": {"engine": {"layout": "V8 crossplane", "displacement": 0.0062099, "torqueShape": {"x": [0, 400], "y": [0.8, 1]}},
          "ecu": {"starter": true, "spark": {"x": [100, 400], "y": [0, 1], "z": [[0, 0.01], [0.02, 0.03]]}},
          "boost": {"turbo": [{"size": 0.5}, {"size": 0.6}]}}}
```

- Values by field name under their section, in SI; a choice by its name (a number is read too), a switch as `true` or
  `false`, an array of numbers as an array, an array of modules as an array of objects.
- A table: a `Curve16` as `{"x": [...], "y": [...]}`, its points in use only (`n` is their count); a `Map16` as
  `{"x": [...], "y": [...], "z": [[...], ...]}`, `z[i]` the row at `x[i]`, one value for each `y`.
- A field the file lacks takes its default; a value outside its range is clamped, with one warning naming them all. A
  field the program lacks is kept and written back where it was (a newer build opened by an older application loses
  nothing), with one warning. A field marked `ui::hidden` is neither read nor written.
- **A part** saved alone carries `"part"`: `"engine"` (the sections engine, valve, intake, boost, exhaust, fuel), `"car"`
  or `"tune"` (the ecu section). Loading a part touches only its sections; a part's file is refused as another part.
- The build's hash (`Build::hash`, the dyno's cache key) is of the Spec's values alone, not the name or the notes.

## A scenario: `tee-scenario`, version 1

A build, a rate and a block size, a timeline of the driver's controls and Spec changes, and assertions. `content/
scenarios/` holds the shipped ones. `tee::parseTimeline` also reads K3a's `tee-engine-scenario/1`
(`tests/sim/scenarios/`), which is the same with `"ctl"`.

```json
{"format": "tee-scenario", "version": 1, "name": "rev-limit", "description": "...",
 "build": "v8", "rate": 48000, "block": 512, "seconds": 14,
 "events": [
   {"t": 0.0, "spec": {"env.temperature": 300}, "ctl": {"mode": 1, "ignition": 1}},
   {"t": 0.25, "ctl": {"ignition": 2}},
   {"t": 4.0, "ctl": {"throttle": 1}, "ramp": 0.15},
   {"t": 6.0, "press": ["shiftUp"]},
   {"t": 8.0, "spec": {"engine.layout": "V12", "sound.harmonics[2]": 0.4}}],
 "assertions": [
   {"tel": "engine.speed", "measure": "max", "during": [6, 10], ">": 600, "source": "..."},
   {"audio": "finite"},
   {"audio": "peak", "<=": 1},
   {"audio": "step", "during": [1, 14], "<": 12}]}
```

- `"warm": true`: the engine at its working temperatures from the start and held there (`tee/Warm.h`: the coolant
  at its thermostat, the oil and the chamber's walls as a running engine's, written before every block, as a dyno
  cell's conditioning holds an engine): a warm idle without minutes of warming. `content/scenarios/idle-warm.json`.
- `"warmTyres": true`: the tyres warm, apart from the engine (`tee/Warm.h`'s `warmTyres()`: the vehicle model's
  `car.m1.warmTyres` held at 1 puts each tyre in the middle of its compound's working window once after every reset of
  the car, then lets it warm and cool by itself): a timed run after its burnout. Default false: a warm engine on cold
  tyres is every drive's first mile. State, not Spec: a scenario's spec event cannot set it. Inert under `car.model` 0.
- `build`: a name under `content/builds/`, or a path; none: every field at its default. `rate` (48000), `block` (512,
  the longest call: a block is cut where the next event falls, so the result does not depend on it).
- An event at `t` seconds: `"ctl"` (or `"controls"`) sets controls by their names in 02-contracts.md, section 4;
  `"press"` raises a counter (`shiftUp`, `shiftDown`, `strip`) by one; `"spec"` sets Spec fields by path (under spec;
  an element as `name[i]`), a number, a switch or a choice's name. `"ramp"`: the event's numbers move there in a
  straight line over that many seconds from the value before, a step every 5 ms. Events of one time apply in the
  file's order.
- A telemetry name, wherever one is taken (an assertion's `"tel"`, a validation target's `"tel"` and `"relativeTo"`,
  `tee-render --tel`, `tee-dyno --fields` and `--torque`, the test library's `Run::tel`): a field's path under tel
  (`engine.speed`; `tel.` optional), or an element of an array field by its index (`car.wheelSlip[2]`,
  `engine.cylPeak[3]`, `a.b[1][2]` for two dimensions). An array named without an index, an index outside its extent
  ("car.wheelSlip[7]: car.wheelSlip has 4 elements, 0 to 3") or an index on a field that is no array is an error that
  says so. A run that names no telemetry keeps every number and each element of every array of at most 64, a column
  each (`car.wheelSlip[2]`).
- An assertion: `"tel"` a telemetry field with a `"measure"` (`max`, `min`, `mean`, `last`), or `"audio"` (`finite`;
  `peak`; `rms` in dB; `step`, the largest step between samples over the local level, `tee::measure::largestStep`),
  over `"during"` (seconds; the whole run by default), compared by `"<"`, `"<="`, `">"`, `">="` or `"within": [a, b]`;
  `"source"` says why. `tee::checkAssertions` gives a line each.

`tee-render --build <b> --scenario <s>` plays one through `SimHost` (the host the application uses): a WAV of 32-bit
floats (`--pcm24`: 24-bit integers), a CSV of the telemetry (`--tel a,b`: those fields and what the assertions read;
none: every number), a row a block; it prints each assertion's line and exits 3 when one fails. `--build defaults`
takes every field's default whatever the scenario names; `--rate`, `--block` and `--seconds` override the file's;
`--define NAME[=V]` (repeatable) compiles the program with a define, a compile-time switch such as `TEE_FAST`.

## A validation: `tee-validation`, version 1

What a build should measure as, each target with its source (card C, section 8). `content/validation/<build>.json`;
`tee-validate` runs them and prints the table (target, range, measured, pass, source); ctest runs the `"fast"` ones.

```json
{"format": "tee-validation", "version": 1, "build": "v8", "description": "...",
 "targets": [
   {"name": "Warm idle", "scenario": "idle", "tel": "engine.speed", "measure": "mean", "during": [8, 12],
    "range": [62.8, 94.2], "unit": "rad/s", "source": "a warm passenger-car V8 idles at 600 to 900 rpm"},
   {"name": "Holds the limiter", "scenario": "rev-limit", "tel": "engine.speed", "measure": "max", "during": [6, 10],
    "relativeTo": "engine.redline", "range": [0.97, 1.03], "fast": true, "source": "..."},
   {"name": "Peak torque", "dyno": {"from": 150, "to": 680, "rate": 60}, "measure": "maxTorque",
    "range": [490, 690], "unit": "N m", "source": "bmep 1.0 to 1.4 MPa (Heywood, ch. 2)"}]}
```

- A scenario's target: a telemetry field (`max`, `min`, `mean`, `last`) or the audio (`"audio"`: `peak`, `rms`,
  `step`) over `"during"`; `"relativeTo"` a telemetry field divides the measure by that field's last value in the run.
- A dyno's target: `"dyno"` a plan (`from`, `to`, `rate` in rad/s and rad/s^2, or `steps` with `settle` and `measure`;
  `pedal`, `warmup`, `torque`, `chassis`), measured by `maxTorque`, `maxPower`, `speedAtMaxTorque`,
  `speedAtMaxPower` or `torqueAt` (with `"at"`).
- Every target says its `"source"` in words; a target the program cannot be measured for yet (a field it lacks) is
  `n/a`, no failure.
- A dyno target is measured on the predicted curve's steady points (`tee/Predict.h`: the engine warm, the exhaust's
  temperatures settled, every 15 rad/s over the plan's `from` to `to`), the curve the interface's ribbon shows; with
  `"sweep": true` (or `steps`) on the dyno runner instead (`tee::runDyno`, the program's MODE_DYNO_ENGINE), whose
  curve depends on how long the sweep took (the exhaust's walls warm over minutes).
- `tee-validate` runs each file in three columns: every model off (the reference: Spec switches named `model` or
  `custom`, and those whose help starts "Off: the reference": `tee::referenceSettings`), the build as it is, and every
  new model on (each `model` at its range's top: `tee::modelsOnSettings`); a column whose build is one already run is
  merged into it (its title says `=`). It prints each column and which targets each misses. A miss is reported, not
  failed; the exit status is 1 only when a fast target fails, or a column misses a target the reference meets.
- Each target belongs to the build it is true of: `default.json` and the content package's archetypes
  (`builds/archetypes/...`, given as a path) carry a stock engine's figures; the owner's presets (`v8`, `v10`,
  `large-diesel`: converted faithfully, a race cam, 233 K air, an open muffler) carry the contract's targets and what
  the preset is meant to do.

## A dyno curve: CSV

`tee-dyno` (and `DynoCurve::csv`): `speed_rad_s,rpm,torque_N_m,power_kW`, then each telemetry field asked for
(`--fields`), a row a point, numbers with nine significant digits. A sweep's point is the mean over its span of speed
(`spacing`, 10 rad/s); a step's the mean over its measuring window after it settled.

Which kind of run it is matters, and a run says so (`DynoCurve::settled`; `tee-dyno` prints it). A settled run
(`DynoPlan::settleExhaust`, on by default; `--no-settle` off) raises `spec.exhaust.heatSettle` at the sweep's start and
twice in each step, so its exhaust walls are where they head: a step is then steady from 2 s on and the same from
either side, and the predicted curve (`tee/Predict.h`, held 1 s at least a point) reads within 2 % of it (the V12,
the economy four). An unsettled step with cold walls held a few seconds reads a few percent low, and how low depends on
the path to it (the V12 at 6673 rpm: 769 N m after a short hold against 779 after a minute). A run on the DYNO
workspace, or a table quoted from one, should say which it is ("walls settled" or not).

Why the predicted points are held 1 s (`PredictOptions::minHoldSeconds`). The slow state is the charge's heating in
the intake (`sim/intake/intake.fm`, `heatRise`). It is a lag of 0.5 s towards a rise that the flow sets. A fresh
instance starts it at 0, so its first point reads a few percent high: the V12 at 6673 rpm reads 706 N m at 0.3 s and
692 at 1 s, with the charge going from 443 to 451 K and the trapped mass falling 1.5 %. It is not knock control (the
same with knock control off), the phaser (still at 15 degrees) or the ECU's trims (lambda still). Its end value is
known, as the exhaust walls' is: with the lag made instant (a scratch copy of the program) the same point reads 691.6
with no hold and 690.6 to 691.6 out to 2 s, against the settled step's 689. When the program can settle it, as
`spec.exhaust.heatSettle` settles the exhaust, the hold goes back to 0 and the ribbon gets its time back.

Whether a predicted point depends on the way the rig came to it (measured 25 Nov on 1db8dcd2's mean of settled cycles,
by a probe case not kept: one share, so each point's path is fixed; six points on three builds without stages). Each
point reached from below (`{low, T}`), from above (`{low, T, high}`: after 640 rad/s), and both again held 3 s at
least. Torque in N m; `se` is the point's own standard error (`PredictPoint::torqueSe`):

| build | rpm | below | above | below, 3 s | above, 3 s | spread |
|---|---|---|---|---|---|---|
| economy-four-15 | 4786 | 136.03 (se 0.65) | 136.22 | 136.41 | 137.35 | 0.97 % |
| economy-four-15 | 3275 | 145.73 (se 0.23) | 145.37 | 145.31 | 145.99 | 0.46 % |
| v6-35-na | 4086 | 312.80 (se 0.59) | 311.81 | 312.60 | 313.32 | 0.48 % |
| v6-35-na | 3314 | 331.18 (se 0.92) | 332.33 | 333.55 | 332.01 | 0.72 % |
| turbo-four-20-gdi (knock-limited) | 4047 | 357.24 (se 0.08) | 357.55 | 357.04 | 357.82 | 0.22 % |
| turbo-four-20-gdi (knock-limited, hunting) | 3247 | 343.90 (se 1.69) | 340.33 | 343.90 | 343.67 | 1.04 % |

On these six points the spread is one to two of the point's standard errors, and no direction repeats (neither from
above nor the longer hold reads consistently higher or lower). That is what was measured; it is not a proof for every
build and speed. A part with hysteresis is different, and is handled: the two-stage cam's switch holds its stage
across a band, so a point in the band read the stage of the way it came (13 N m at 5740 rpm, card N1's find). The
predictor now reaches every point as an upward sweep does where the program says a part can latch
(`tel.engine.hysteresis`, `tel.engine.latched`; `PredictPoint::approachForced` when it would not let go).

## A session log: `tee-session`, version 1

Every control change and every Spec write that reached the simulator, each at the sample it took effect (card C,
section 9): what `SimHost` keeps when `LoadOptions::log` is on, and what `tee::replay` plays again to the same samples.

```json
{"format": "tee-session", "version": 1, "rate": 48000, "samples": 144000, "start": "<the module's bytes, hex>",
 "events": [{"sample": 0, "controls": {"ignition": 2, "throttle": 0, ...}},
            {"sample": 12288, "offset": 40, "section": 0, "bytes": "0000a042"}]}
```

A Spec write is the bytes at an offset of the main module, so a log belongs to the program it was made with (a log of
another layout is refused when an offset fits no field).

## A bake cache entry: `tee-bake-cache`

`CalibrationBakeOptions::cacheDirectory` holds one file per table baked (host/include/tee/Bake.h):
`spark-<key>.json` for a knock-limit table and `cam-<key>.json` for an intake cam map. `<key>` is the table's exact
key: `knockLimitKey` or `intakeCamKey`, made from the program's sources and the fields that move the table. The newest
256 files are kept, by their last use. Each file is one line of JSON:

```json
{"format": "tee-bake-cache", "key": 1208261038, "fieldsKey": 377154212,
 "fields": [["ecu.maps.knockLimit", [...]], ["ecu.maps.knockLimitAuto", [0]], ["ecu.maps.knockLimitKey", [1208261038]]],
 "stillKnocking": [[false, false], [true, false]],
 "check": {"speeds": [104.7, 209.4], "torque": [68.0, 123.1], "retard": [0.0, 3.9], "valid": [true, true]}}
```

- `fields`: the values the table's apply wrote, field by field.
- `stillKnocking`: a spark table's cells still knocking after its corrections, by speed and load.
- `fieldsKey`: the same key without the program (`knockLimitFieldsKey` or `intakeCamFieldsKey`).
- `check`: the full-load row measured with the table applied: speeds in rad/s, torque in N m, and knock control's
  mean retard in degrees. `valid` is false where the point faulted or the limiter cut it.

A stage whose exact key has a file takes it at once. When the exact key misses, the newest file with the same
`fieldsKey` and a `check` is a candidate, baked under another program. It is checked on today's program and taken only
if every point agrees (Bake.h says how); then it is written again under today's key. A file without `fieldsKey` or
`check` (written before the verified reuse) is never a candidate. A damaged file counts as absent.
