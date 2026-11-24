> **K3b in progress**: modules carved so far: Listen (`listen/`), Extras (`sound/extras.fm`), Car (`vehicle/car.fm`), Heat (`core/heat.fm`), Boost (`boost/boost.fm`), Ecu (`ecu/ecu.fm`), Combustion (`combustion/combustion.fm`), Pop (`exhaust/pop.fm`), Valvetrain (`valvetrain/valvetrain.fm`). Do not integrate
> against this file until `docs/handoffs/k3b.md` exists and `sim/core/monolith.fm` is gone.

# sim/: the simulator

One FMinus program: `engine.fm` and what it includes. Its one main module is `Engine`
([docs/plan/02-contracts.md](../docs/plan/02-contracts.md), section 1); `sim/ref/` is the reference it was made
from, never edited, the baseline the null test holds it to.

| Path | What it is |
|---|---|
| `engine.fm` | `module Engine`: `spec`, `ctl`, `tel`, `scope`; `init`, `process`, `end` |
| `spec/` | the contract's blocks: `spec.fm` and a file a section, `controls.fm`, `telemetry.fm`, `scope.fm`, `limits.fm`, `tables.fm` |
| `core/monolith.fm` | **K3a only**: the reference's `engine.fm` (its module `Shared`) byte for byte, with the files it includes copied beside it (`core/*.fm`). K3b carves it into the modules of 02-contracts.md, section 6 |
| `ref/` | the reference (read-only) |

Status: **K3a** (the contract over the unchanged monolith). K3b, the split, follows module by module; this file then
grows each module's responsibility, state, functions with their signatures and when each is called.

## The contract as built

```c
module Engine
{
    Spec      spec;     // spec/spec.fm: what the machine is. The host writes it between calls, and moves a section's rev
    Controls  ctl;      // spec/controls.fm: what the driver does. The host writes it before each call
    Telemetry tel;      // spec/telemetry.fm: what the machine does. Written by process, read after each call
    Scope     scope;    // spec/scope.fm: declared; filled in K3b (ctl.scopeOn)
    void init(float hostRate);                                                    // allocates: off the audio thread
    void process(float* restrict left, float* restrict right, int n) [[clang::nonblocking]];
    void end();                                                                   // frees: off the audio thread
};
```

- `process` writes `n` samples into both buffers (it zeroes them first), finite and within +-1 (the reference's soft
  ceiling). `process(left, right, 0)` applies the Spec, the Controls and the ignition and renders nothing.
- The host looks the methods up by name; `tee::EnginePlayer` (host/include/tee/EnginePlayer.h) refuses a program
  whose `Engine::process` FMinus does not report as `nonblocking` (`fminus::Program::nonBlocking`), and FMinus has then
  proved it reaches no `malloc`, `calloc` or `free`. Nothing had to change for that: the reference's render path was
  already free of allocation.
- FMinus initialises no field: the host writes every Spec and Controls field's `fminus::default` before `init`
  (`EnginePlayer` does, and again on `reset`).
- A section's `rev`: the host adds one after writing any of the section's fields. `Engine` compares each section's
  `rev` with the one it last applied at the start of each call (the reference's Voice read its dials at each call too),
  and only then copies it into the monolith.

### The first Spec

The reference's 32 dials, in their sections (02-contracts.md, section 2), their ranges and defaults kept. SI where
the table says: the displacement in m^3 (the dial's litres / 1000) and the air in K (the dial's F: the reference took
80.33 F as 300 K, and so does the conversion, `F = (K - 300) * 1.8 + 80.33`). The trims stay in rpm (`ui::unit("rpm")`),
the timing in crank degrees, the level in dB, and every 0-to-1 dial a fraction with its meaning unchanged.

| Spec field | Reference dial (Set field) | Conversion back (in `Engine::readSpec`) |
|---|---|---|
| `engine.layout` (choices: the 17 layouts) | `engine` (`layout`) | as is |
| `engine.displacement` m^3 | `size` L (`size`) | x 1000 |
| `engine.boreStroke`, `.compression`, `.flywheel`, `.idleTrim`, `.redlineTrim`, `.rough`, `.benchLoad` | `borestroke`, `compression`, `inertia`, `idle`, `redline`, `rough`, `load` | as is |
| `valve.cam` | `cam` | as is |
| `boost.kind`, `.level`, `.size`, `.blowOff` | `aspiration`, `turbo`, `turbosize`, `bov` | as is |
| `exhaust.headers`, `.pipe`, `.crossover`, `.muffler` | `headers`, `pipe`, `xpipe`, `muffler` | as is |
| `fuel.kind` | `fuel` | as is |
| `ecu.timing`, `.pops`, `.starter` (bool) | `timing`, `pops`, `starter` | as is; the bool 0 or 1 |
| `car.kind` (Car, Truck), `car.mass` | `drive` 1 or 2 (with `ctl.mode`), `mass` | below |
| `env.temperature` K | `ambient` F | (K - 300) x 1.8 + 80.33 |
| `sound.view`, `.distance`, `.room`, `.level`, `.intake`, `.mechanical` | `view`, `distance`, `room`, `level`, `intake`, `mech` | as is |
| none | `hold` | 1e9 s: the engine no longer switches itself off |

### What replaces the keys (Controls into the reference's Set, every call)

| Control | The reference, as `Engine` drives it |
|---|---|
| `mode` `MODE_BENCH` (and the dynos, which do nothing yet) | DRIVE KEYS: `set.drive = 0`; `ctl.throttle` is the mod wheel (`set.modwheel`), `set.gas = 0` |
| `mode` `MODE_DRIVE` | DRIVE CAR (`spec.car.kind` Car) or TRUCK: `set.drive` 1 or 2; `ctl.throttle` the pedal (`set.gas`), `set.modwheel = 0` |
| `brake`, `clutch`, `lever` | `set.brake`, `set.clutch`, `set.lever` as they are |
| `shiftUp`, `shiftDown` (counters) | each increase is a press of the right (bit 16) or left (bit 32) paddle in `set.buttons`: down until Shared's shift logic has seen it (`lastButtons` holds the bit), then up until it has seen that; a further press waits its turn |
| `ignition` 2, start | while the engine is stopped or running down (state 0 or 3): Shared's `keyDown(36, 1)`, the reference's start key (C2, whose key driver asks for the idle speed, so for nothing); it cranks (without a starter, it runs at once at its idle). The key stays down while start is held; started again if the engine stops meanwhile |
| `ignition` from 2 to 1 or 0 | Shared's `keyUp(36)`; if it still cranks (state 1), state 0: the starter stops. (The reference's key was a tap: it cranked on until it caught) |
| `ignition` 0, off | a cranking or running engine (state 1 or 2) is put in state 3, running down, with its stall timer and its armed key cleared. (The reference ran down after HOLD's seconds of idling, or, driven, a second of the start key held while it ran; neither happens now) |
| `holdSpeed` > 0 (`MODE_BENCH`, running) | the reference's key driver aimed at it: a key of the pitch whose firing that speed is (DRIVE PITCH, `set.keys = 1`), held as the newest key, velocity 1, written into Shared's key list; removed when `holdSpeed` is 0, the mode changes or the engine stops |
| `bend`, `pressure`, `bendRange` | 0, 0, 12: no pitch wheel, no aftertouch |
| `handbrake`, `steer`, `buttons`, `dynoSpeed`, `dynoRate`, `strip`, `scopeOn` | not read yet |

`Engine` renders Shared only while it sounds, as the reference's Voice did: from a start until Shared says it is silent
(`audible()`: stopped, the room rung out, the car at rest) with the key up. Until the first start, and after that,
`process` writes silence and the Telemetry stands still.

### Telemetry (written after each call, from Shared's state)

`engine.state` (0 off, 1 cranking, 2 running, 3 running down), `.speed`, `.redline`, `.idle` (rad/s), `.throttle` (the
plate, 0 to 1), `.manifold` (Pa absolute), `.coolant` (K), `.oilPressure` (Pa over the air), `.flags` (`FLAG_LIMITER`
1: the limiter or a diesel's governor; `FLAG_FUEL_CUT` 2: the fuel cut while it runs), `.cycle` (the panel's slowed
cycle, 0 to 1), `.crank` (degrees, 0 to 2160); `boost.pressure` (Pa over the air); `exhaust.portTemperature` (K);
`ecu.knockRetard` (degrees); `car.speed` (m/s, 0 on the bench), `.gear` (-1 R, 0 N, 1 up; 0 on the bench), `.brake`,
`.clutch`; `sound.peak` (the call's largest sample).

## The null test, both ways

`tests/sim/NullTest.cpp`: each build of `content/builds` (the reference's Defaults, V8, V10 and Large Diesel presets,
converted) on each engine scenario of `tests/sim/scenarios` (`free-revs`: neutral, blips, the limiter, the overrun,
off; `drive-car`: a car up the gears on the paddles and down one; `truck`: a 30 t truck pulling away; `bench`: no
vehicle, the throttle by hand), at 48 kHz in blocks of 512, and two more at 44.1 kHz / 333 and 96 kHz / 64. Both
programs are played in one process by `tee::playTwin` (EnginePlayer.h):

- **The simulator** takes the build and the scenario as they are.
- **The reference** is given what its Voice would have been: before each call the simulator is called for no samples,
  its adapter's Set is read back, and the reference's dials (by the table above, read from that Set) and its Host's
  pedals (`throttle` from `set.gas`, `modwheel` from `set.modwheel`), lever and buttons (the paddles' pulses) are set
  from it: both Sets hold the same bits. The ignition's start is its start key, note 36 at velocity 1, going down
  (`RefPlayer::start`), and coming up when the start is let go (`stop`). Off, which the reference has no key for, is
  written into its Shared as the adapter writes it (state 3, `stallT` 0, `offArmed` false), at the same sample.
- Then both are called for the same samples.

Result (K3a, this container): every pair identical to the bit (`nullDepth` infinite, the largest difference 0).

## Left as the reference has it

(K3b fills this in as it carves; nothing has been changed so far.)
