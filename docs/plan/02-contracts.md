# The contracts

*Stage 2. What every package may rely on and none may break. The lead owns this file. Part K3
([p0-split.md](p0-split.md)) makes the simulator's side real and writes the exact signatures it ended with into
`sim/README.md`; where that file and this one differ after K3, `sim/README.md` is right about signatures and this
file about intent.*

## 1. The simulator, seen from outside

One FMinus program, `sim/engine.fm` and what it includes. One main module:

```c
module Engine
{
    Spec      spec;     // what the machine is.   The host writes it, between calls only.
    Controls  ctl;      // what the driver does.  The host writes it before each call.
    Telemetry tel;      // what the machine does. The host reads it after each call.
    Scope     scope;    // the taps: a cycle of pressures, lifts, waves. Read after a call, when someone looks.
    // ... the subsystems' state

    void init(float hostRate);       // takes all memory; not nonblocking; off the audio thread
    void process(float* restrict left, float* restrict right, int n) [[clang::nonblocking]];
    void end();                      // gives the memory back; off the audio thread
};
```

- `process` **writes** `n` samples at the host's rate into both buffers (the model runs at 96 kHz inside and
  resamples, as the reference does). Whatever it computes, what it writes is finite and within ±1.
- The host looks the three methods up by name (`"Engine::process"`), refuses a program whose `process` is not
  nonblocking, and reaches the four blocks by the offsets `fminus::Program::module("Engine")` gives.
- **Plain fields, not `param`s.** Every write to `spec` and `ctl` is made by the host on the audio thread between
  two calls (§5), so there is no race to guard and tables, arrays and nested modules are all allowed. `param` is
  kept for the lead to place later on the few switches worth a specialised copy of the hot loop.
- The program has no other entry point, reads no file, opens no socket, and declares no external function.

## 2. The Spec

`sim/spec/spec.fm`. Nested by subsystem; each section is a module in its own file, owned by one package:

| Section | File | Owner | Holds |
|---|---|---|---|
| `spec.engine` | `spec/engine.fm` | D | layout, cylinders, bore, stroke, rod, compression, masses, flywheel, friction |
| `spec.valve` | `spec/valve.fm` | E | the cams, phasers, stages, lift control, deactivation, springs |
| `spec.intake` | `spec/intake.fm` | E | throttle kind, plenum, runners, air box |
| `spec.boost` | `spec/boost.fm` | F | turbos, wastegate, vanes, blow-off, intercooler, superchargers, anti-lag, nitrous |
| `spec.exhaust` | `spec/exhaust.fm` | G | header assignment and sizes, crossover, converter, resonators, mufflers, valve, tails |
| `spec.fuel` | `spec/fuel.fm` | H | the fuel and its properties |
| `spec.ecu` | `spec/ecu.fm` | H | the maps and the settings |
| `spec.car` | `spec/car.fm` | I | vehicle kind, masses, geometry, tyres, driveline, gearbox, aero, brakes |
| `spec.env` | `spec/env.fm` | lead | air temperature, pressure or altitude, surface, grade |
| `spec.sound` | `spec/sound.fm` | wave 2 (lead until then) | the mix, the listener, the place |

Rules:

1. **Every field is declared with its attributes** (§3). SI units inside (K, Pa, m, kg, s, rad/s; angles in crank
   degrees where the reference uses them, and say so in the label's unit). A field with no `fminus::default`
   defaults to 0.
2. **Append only.** A package adds fields to its own section, at the end. Nobody renames, reorders, retypes or
   removes a field: files, tests and other packages name fields by path. A field that turned out wrong is left,
   marked `[[ui::hidden]]`, and replaced by a new one; the lead prunes at a version boundary.
3. **Defaults are the reference.** With every field at its default the simulator is the reference's V8 as its
   Init program has it. A package's new fields default to "off, as before".
4. **Each section starts with `int rev;`.** The host adds one to a section's `rev` after it has written any of its
   fields (it knows the section from the offset). The simulator compares `rev` with the one it last applied, every
   16 samples, and only then looks at the fields. Nothing else tells the simulator that something changed.
5. **The simulator smooths; the host does not.** A change the sound can follow (a length, a level, a timing) is
   glided inside over about 80 ms, as the reference glides its dials. A change it cannot (the layout, the
   displacement, the fuel, the topology of the pipes) is made under the reference's short fade. A package says in
   its section's header comment which of its fields are which.
6. **Tables** use the shared modules of `spec/tables.fm`: `Curve16` (`x[16]`, `y[16]`, `n`) and `Map16` (`x[16]`,
   `y[16]`, `z[16][16]`, `nx`, `ny`), each with its interpolating read. A package does not invent another table
   shape without the lead.
7. **Fixed maxima** are enumerators in `spec/limits.fm` (`MAX_CYL` 16, `MAX_PORTS` 16, `MAX_COLLECTORS` 8,
   `MAX_SIDES` 2, `MAX_TURBOS` 4, `MAX_GEARS` 18, `MAX_WHEELS` 4). Arrays are sized by them; a count field says how
   many are in use.

The first Spec (K3 writes it) is the reference's 32 dials, moved to their sections with their meanings kept:

| Reference dial | Spec field | Note |
|---|---|---|
| `engine`, `size`, `borestroke`, `compression`, `inertia`, `idle`, `redline`, `rough`, `load` | `engine.layout`, `.displacement` (m^3), `.boreStroke`, `.compression`, `.flywheel`, `.idleTrim`, `.redlineTrim`, `.rough`, `.benchLoad` | |
| `cam` | `valve.cam` | the one dial, until E replaces it |
| `aspiration`, `turbo`, `turbosize`, `bov` | `boost.kind`, `.level`, `.size`, `.blowOff` | |
| `headers`, `pipe`, `xpipe`, `muffler` | `exhaust.headers`, `.pipe`, `.crossover`, `.muffler` | |
| `fuel` | `fuel.kind` | |
| `timing`, `pops`, `starter` | `ecu.timing`, `.pops`, `.starter` | |
| `mass`, `drive` (CAR, TRUCK) | `car.mass`, `car.kind` | KEYS and PITCH are gone: §4 |
| `ambient` | `env.temperature` (K) | |
| `view`, `distance`, `room`, `level`, `intake`, `mech` | `sound.view`, `.distance`, `.room`, `.level`, `.intake`, `.mechanical` | |
| `hold` | none | the engine no longer switches itself off |

## 3. Attributes: how the program describes itself

FMinus passes any attribute in a namespace of the host's own through to `fminus::Program::Module::Field`. Ours:

| Attribute | On | Meaning |
|---|---|---|
| `fminus::range(min, max)` | every number | what a control spans; the host clamps to it |
| `fminus::default(v)` | every number and bool | the value a new build has, and a file that lacks the field |
| `ui::label("Bore")` | every field | its name for a person |
| `ui::quantity("pressure")` | a physical quantity | lets the interface show it in the user's unit. One of: `pressure`, `temperature`, `length`, `area`, `volume`, `mass`, `inertia`, `speed`, `rotation` (rad/s, shown as rpm), `torque`, `power`, `angle` (crank degrees), `time`, `frequency`, `flow`, `ratio`, `fraction` (0 to 1, shown as %) |
| `ui::unit("deg")` | a number with a fixed unit | when no quantity fits |
| `ui::choices("NA", "Turbo", ...)` | an `int` that is a choice | its options' names, from the range's minimum up |
| `ui::taper("log")` | a control | its travel is logarithmic (the range must be above 0) |
| `ui::decimals(n)` | a number | digits shown |
| `ui::group("Turbine")` | a field | the heading it sits under on a generated page; holds until the next one |
| `ui::help("...")` | a field | one sentence a person can read |
| `ui::hidden` | a field | not shown, not saved |
| `ui::axes("rotation", "fraction")` | a `Curve16` or `Map16` field | its axes' quantities |

`fminus::range` and `fminus::default` are FMinus's own names; FMinus neither clamps nor initialises with them (it
runs no constructor), so the host does both. A bool is a switch; an `int` without `ui::choices` is a number.

## 4. Controls

`sim/spec/controls.fm`. The lead owns it; packages ask for additions.

```c
module Controls
{
    int   ignition;     // 0 off, 1 on, 2 start (held): a key, as a car's
    float throttle;     // the pedal, 0 up to 1 down
    float brake;
    float clutch;       // 1 is the pedal on the floor
    float handbrake;
    float steer;        // -1 left to 1 right; not driven by yet
    int   lever;        // an H-pattern's position: -1 R, 0 N, 1 up; ignored by a gearbox that has none
    int   shiftUp;      // counts presses: the simulator acts on each increase it sees
    int   shiftDown;
    int   buttons;      // held switches, a bit each: BTN_LINE_LOCK, BTN_LAUNCH, BTN_NITROUS, BTN_HORN, ...
    int   mode;         // MODE_BENCH (no vehicle), MODE_DRIVE, MODE_DYNO_ENGINE, MODE_DYNO_CHASSIS
    float dynoSpeed;    // a dyno's set speed (rad/s) ...
    float dynoRate;     // ... and its sweep's rate (rad/s^2; 0 holds)
    float holdSpeed;    // MODE_BENCH: above 0, a driver holds this crank speed (the reference's key governor)
    int   strip;        // counts requests to stage a drag run
    int   scopeOn;      // 0: the Scope block need not be filled
};
```

A press that must not be lost between two blocks is a **counter**; a state that is held is a **level**. The
reference's MIDI keys and pitch mode are gone; its key governor survives as `holdSpeed`, because a dyno and a test
both want it.

## 5. Telemetry and Scope

`sim/spec/telemetry.fm`, nested as the Spec is (`tel.engine`, `tel.boost`, `tel.exhaust`, `tel.ecu`, `tel.car`,
`tel.sound`), append-only by the same owners, with the same attributes (a label and a quantity make a gauge).
Values are instantaneous at the block's end unless the label says mean or peak. `tel.engine` starts as the
reference's `show*` fields unpacked into SI: `state`, `speed`, `redline`, `idle`, `throttle`, `manifold` (Pa
absolute), `coolant`, `oilPressure`, `flags` (limiter, fuel cut), `cycle` (the slowed phase the pictures turn by),
`crank` (degrees); `tel.car`: `speed`, `gear`, `brake`, `clutch`; `tel.boost`: `pressure`.

`sim/spec/scope.fm` holds what is too big to copy every block: cylinder 1's pressure, volume and both valve lifts
for each degree of the last whole cycle; pressures along the pipes; and what packages add. The host copies it only
while a view asks.

## 6. Inside the simulator: who does what, in what order

K3 splits the reference's `Shared` (2,600 lines in one module) into these, each a module in its own directory, with
the reference's behaviour unchanged:

| Module | Directory | Owner after K3 | Responsibility |
|---|---|---|---|
| `Engine` | `sim/engine.fm` | lead and D | the rates, the order of calls below, the fade, nothing physical |
| `Crank`, `Cylinder`, `Mechanics`, `Heat` | `core/` | D | angle and speed; the gas in a cylinder; friction, inertia, the starter; coolant, oil, walls |
| `Valvetrain` | `valvetrain/` | E | each cylinder's valve events and area tables |
| `Intake` | `intake/` | E | runners, plenum, throttle, air box; the intake's sound |
| `Boost` | `boost/` | F | compressor, turbine, shaft, gates, coolers, superchargers; their sounds |
| `Exhaust`, `Duct`, `Valve`, the junction helpers | `exhaust/` | G | every pipe after a port; junctions; pops; what leaves each tail |
| `Combustion` | `combustion/` | H | the charge at intake closing: heat, spark, burn, knock; fuels |
| `Ecu` | `ecu/` | H | the engine's state machine, the driver in `holdSpeed`, idle, limiter, cuts, maps |
| `Car` | `vehicle/` | I | everything from the flywheel to the road; road noise |
| `Extras` | `sound/` | wave 2 | the mechanical sources |
| `Listen` | `listen/` | wave 2 | sources to two channels: views, distance, room, level, the resampler |

**One sample** (`Engine::sampleTick`), in the reference's order:

1. Pipes read: `exhaust.read()`, `intake.read()`: the waves arriving at every end.
2. The crank turns: `crank.advance(dt)`.
3. Each cylinder: its areas from `valvetrain`, its two pipe ends from `exhaust` and `intake` (the arriving wave,
   the impedance, the gas's temperature), `cylinder.step(...)`; its flows are summed per port, its torque into the
   total; at intake closing `combustion.charge(c)`; its events go to `extras`.
4. `exhaust.step(flows)`: the ports' waves in, every junction solved (`boost`'s turbine among them), the pipes
   written, each tail's radiation out.
5. `intake.step(flows)`: the runners' far ends, the plenum, the throttle (`boost` gives the pressure upstream).
6. `mechanics` and `boost` give their torques; the crank's speed from the sum, through `car.step` when driven.
7. `extras.tick(...)`, then `listen` mixes.

**Every 16 samples** (`Engine::control16`): `heat`; the Spec's `rev`s and the gliding; `ecu` (state, driver, idle,
cuts, the maps' outputs); `valvetrain.control`; `boost.control`; `exhaust.control` (the gas's temperatures, the
pipes' lengths, the flow losses); `intake.control`; `car.control`.

Rules of the seams:

- A module reads another's state only through the functions `sim/README.md` lists for it. It never writes
  another's fields.
- Each module has `setup(...)` (in `init`, may allocate), `reset()` (the state at rest), `control(...)` (every 16
  samples) and its per-sample functions; and `apply(spec)` when its section's `rev` moved.
- Only the integrator (D, with the lead) edits `sim/engine.fm`. A package that needs a new call in the order of
  work asks; the hook is added with a default that does nothing.
- A package's new state goes into its own module. Nothing is added to another package's module.

## 7. The host's side

`host/include/tee/` (K2 starts it, package C finishes it). What the other packages may use:

```cpp
namespace tee
{
    class Schema;        // modules, fields by path, offsets, types, attributes (§3) parsed into FieldInfo
    struct FieldRef;     // a resolved field: use on any thread, no strings

    class SimHost
    {
    public:
        bool load(const std::filesystem::path& simDir, const LoadOptions&, std::string& errors);
        const Schema& schema() const;

        // any thread but the audio thread
        void set(FieldRef specField, double value);                          // queued; applied before the next block
        void setBytes(FieldRef specField, const void* data, std::size_t n);  // an array, a curve, a map
        void setControls(const ControlsFrame&);                              // the newest wins
        bool telemetry(TelemetryFrame& out) const;                           // the newest whole block's
        bool scope(ScopeFrame& out);                                         // asks for it, then gives the newest

        // the audio thread
        void prepare(double sampleRate, int maxBlock);                       // audio stopped
        void process(float* left, float* right, int frames) noexcept;        // drain, controls, Engine::process, publish
    };
}
```

- The UI's document is `tee::Build` (values by path, in SI). It is the truth on the UI thread; the simulator's
  Spec is its copy. `Build` loads and saves `content/` files (§8), applies defaults and ranges from the Schema,
  and sends `SimHost::set` for each change.
- A widget binds to a field through `tee::ui::FieldPort`, FunkGui's `ParamPort` over a `Build` and a `FieldRef`
  (normalised by the range and taper; steps for choices and switches; text by the quantity and the user's units).
- Until K3 lands, the shell and the interface run on `tee::RefPlayer` and the reference's 32 dials through the
  same two interfaces (`ParamSource`: a list of named parameters with ranges; `AudioSource`: `prepare`,
  `process`), so nothing in `shell/` or `ui/` names the reference or the new engine directly.

## 8. Files

- A **build**: `{"format": "tee-build", "version": 1, "name": ..., "notes": ..., "spec": { "engine": {...}, ... }}`.
  Values by field name, in SI, numbers as numbers, choices by their names, tables as arrays. A field the file
  lacks takes its default; a field the Schema lacks is kept in memory and written back (a newer build opened by an
  older app loses nothing) with one warning. `engine`, `car` and `tune` (the `ecu` section) can each be saved and
  loaded alone.
- A **scenario**: a build (or a preset's name), a timeline of `Controls` and Spec changes by time in seconds, and
  assertions on telemetry and audio. Package C defines it in `docs/formats.md`, growing K2's.
- Nothing binary in git: no audio, no pictures. Tests write under `out/`.

## 9. Budgets

K2 measures the reference V8 in this container: call that cost **R** (nanoseconds a 96 kHz sample, release
options). On the user's Mac R is 30 to 40 % of a core.

| State of the build | Budget |
|---|---|
| Every new feature at its default ("off") | at most 1.10 R |
| A typical modified engine (a turbo V8 with variable timing and maps in use), driven | at most 1.6 R |
| The heaviest v1 build (sixteen cylinders, four turbos) | reported, with what tier it needs; not required in real time |

Each card names its share. A package over its share says so and proposes what to move to the 16-sample rate, to a
table, or to a quality tier.
