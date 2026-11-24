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
