# TheEngineEngine: abstract plan and research

*Stage 1 of 3, 2026-11-24. Written against the reference project "Engine" (TestingPlaygroundSynth, v19 of
2026-11-15), FMinus at `5b665d7`, FunkGui at `b7bc1cd` (v0.27.0) and TestingPlayground at `47bac40`. Nothing was
built or run for this note; §14 says what was read, by whom, and what is an estimate. Stage 2 turns this into
detailed work-package files for parallel agents; stage 3 is the implementation.*

*Reviewed the same day: your answers to §13 are recorded there, and stage 2 is the rest of this folder
([README](README.md)). Where a later file and this one differ, the later file is right.*

## 0. Summary

**What is being built.** A standalone desktop application in which a car engine is simulated as physics and heard
as the physics' own sound: the gas in each cylinder, the valves, the pipes as waveguides, the turbo, the crank, the
clutch, the tyres. The reference already does this well as a 5,100-line FMinus program played from a synth's keys.
TheEngineEngine keeps that sound engine in FMinus, gives it a window of its own (FunkGui, no JUCE), and grows it
into something an enthusiast can build, tune, dyno and drive: custom valvetrains, turbos and superchargers as real
machines in the gas path, an ECU with editable maps, custom layouts and exhausts, and a straight-line car with
proper tyres, driveline and aero.

**Decisions taken in this session** (yours, 2026-11-24):

| Question | Decision | What it commits us to |
|---|---|---|
| Window, audio, input | An SDL3 `DesktopHost` added to FunkGui; no JUCE anywhere | A FunkGui release (its bgfx layer built without JUCE, the host, menus drawn in the canvas). The real window can only be proven on your Mac (§8.4) |
| Where agents run | Cloud sessions: Linux x86-64, no GPU, display or sound card | Every work package proves itself offline: rendered audio measured, pictures from the software rasteriser (§8) |
| What ships | The FMinus JIT inside the app | No compiler change needed; reload while developing; about 65 MB; an entitlement on macOS (§3.5) |
| Driving | Straight line only | Longitudinal physics done properly; no steering, no circuits (§4.11) |
| Interface code | TestingPlayground's JUCE-free panel code reused, plus new bespoke views | About 11,700 lines copied with provenance; hero views in new C++ and fragment shaders (§6) |
| Feature groups in v1 | All four: valvetrain lab, forced-induction lab, ECU tuning, engine and exhaust builder | v1 is two waves of agents, not one (§10) |
| Sound work | All four: outside the car, the cabin, mechanical detail, the exhaust's tone | A listener model with moving sources and places to listen in (§5) |
| Engine families | Two-strokes, heavy diesels, motorcycles (drag-race exotics not chosen) | A second cylinder model (two-stroke) in the second wave (§4.8) |

**The shape.**

```
 UI thread                               audio thread                         worker threads
 SDL3 events -> DesktopHost (FunkGui)    device callback (SDL3)               JIT compile, reload
   tick / draw / input -> Panels           1 drain commands -> write Spec     dyno sweeps on a 2nd instance
        |                                  2 write Controls (pedals, keys)    offline render, WAV export
   UI model (the build, the tune)          3 Engine::process  (FMinus)        files
        |  commands (lock-free queue) -->  4 read Telemetry and taps
        ^  snapshots (seqlock, rings) <--
```

One FMinus program is the simulator. Everything the interface can change is a declared field of that program, with
its range, default, label and unit as attributes, so the program is the single list of parameters and the shell
builds controls, presets and files from it (§3.3).

**The four things most likely to hurt** (§9 has the rest):

1. *The window.* `DesktopHost` is new, touches macOS, and no cloud session can see it. An early checkpoint on your
   Mac, with FunkGui's gallery and nothing of ours, is the plan's first gate.
2. *Nobody in the cloud can listen.* Agents can measure spectra and levels, not judge a V8. Each wave ends in
   a listening pack for you, and "feature off means the reference's sound, to tolerance" is a test.
3. *One tight per-sample loop, many hands.* The reference is a monolith. Phase 0 splits it into modules with fixed
   seams before anyone works in parallel; that split is the keel and is done by one session.
4. *CPU.* The new physics multiplies pipes and solves. The reference's cost on your Mac is not known to me (§13,
   question 2); Phase 0 measures it and gives each package a budget.

**What I need from you** is in §13: sixteen questions, each with the default I will assume if you say nothing.

## 1. The reference: what exists, and what it leaves simple

The reference is `program.fm` and nine library files (5,134 lines of FMinus), a `.gui` panel (102 lines) and
three fragment shaders (1,383 lines). I read the physics whole and most of the rest (§14 says which parts not).

| Part | What it does today | What it leaves simple |
|---|---|---|
| Cylinder (`cylinder.fm`) | Filling and emptying: mass, energy, burned-gas share; slider-crank volume (rod ratio fixed at 3.3); a Wiebe burn from the spark; a diesel's premixed spike and diffusion flame; wall heat loss; residual gas and reversion tracked, so a big cam's idle lopes by itself | One lift shape from a table; no inertia of the pistons; knock is a threshold on compression, not on the end gas's history |
| Valves (`pipes.fm`, `Valve`) | Compressible flow solved together with the pipe's wave, choked at blowdown, in closed form (three passes) | Valve area a fixed share of the piston's; two valves assumed |
| Cams (`engine.fm`, `valves()`) | One CAM dial: duration 224 to 300 degrees for both valves, lobe separation 112 to 106 | No separate intake and exhaust, no phasing, no lift control, no switching |
| Pipes (`pipes.fm`, `Duct`) | Waveguides with fractional delay; wall loss as fitted shelves; flow loss from the mean Mach number; nonlinear steepening into shocks; junction losses that grow with amplitude; open-end reflection (Silva's fit) | Fixed topology: headers, one or two collectors, pipe, a two-chamber muffler, tail; an H-pipe dial |
| Intake | Runners as waveguides into a plenum the throttle fills; an idle valve | One throttle; fixed runner length; the intake's sound is an air-box resonator on the throttle's flow |
| Forced induction (`sounds.fm`, `turbo16`) | Behavioural: spool chases the square root of exhaust power with a lag; boost is spool squared up to the wastegate; blow-off or flutter as envelopes; a twin-screw supercharger's whine | Not in the gas path: the turbine is a narrower pipe and a low-pass; no compressor or turbine map, no intercooler, no shaft |
| Combustion, ECU | Spark from a formula (speed, load), knock retard, limiter by fuel cut, overrun cut with pops, idle control by valve and spark, a diesel's governor, ignition delay (Wolfer) and premixed share (Watson) | Gasoline at a fixed mixture, or diesel; no maps to edit; one limiter style |
| Heat | Coolant, oil, chamber walls, five exhaust sections with wall temperatures; cold starts; oil pressure | Good as it is; needs an intake-air and exhaust-gas temperature the user can see |
| Vehicle (`car.fm`) | Clutch with stick and slip, six gears (a truck's eighteen), driveline spring with lash and clunk, one lumped tyre, drag, rolling, brakes; paddle shifts with ignition cut and blips | All four tyres as one, grip 1.05 g on the whole weight (so every car is all-wheel drive with no weight transfer); no differential, no tyre falloff past the peak, no downforce |
| Layouts (`layouts.fm`) | Seventeen, each with real firing angles and exhaust grouping; rotary as three faces a rotor | Arrays of 12; no custom firing order, bank angle or crank |
| Listening | Five views by gain tables; an ORTF pair on the tailpipes with directivity and delay; cabin modes driven by torque; distance; an FDN room (`hall.fm`); compressor and limiter; 96 kHz inside, a Kaiser sinc to the host's rate | No moving listener, no ground, no places; gearbox and wind absent |
| Playing it | MIDI keys rev it; a G29's pedals, shifter and paddles drive it (`host.fm`, the plugin's) | The synth's contract (`Voice`, 32 host dials, `host.fm`) goes away |

Two things to keep whatever else changes: **the sound is the simulation's own** (no samples, no clock of its own in
any source: every click and whistle is driven by an event or a flow of the model), and **the stabilisations are
hard-won** (the comments record a Newton iteration that chattered, header echoes that "locked it into chaos", a
runner that squealed). New elements join by the same rules: passive junctions, closed forms, losses that are physics.

## 2. The platform: what was learned

### 2.1 FMinus (the embedding API and the language)

- A host compiles **from a string**, with a callback that resolves `#include` (no include-path option in C++);
  errors come back in clang's wording. `Program::Options` has the optimisation level, the math mode, denormals
  (`Flush`), knob versions, a loop budget and bounds checks (both off by default and free when off; bounds checks
  cost a third or more when on).
- A module is laid out as a C struct and described to the host: `Program::module(name)` lists every field with
  its type, **offset** and **attributes**. The host allocates an instance, calls methods through typed function
  pointers (`"Engine::process"`), and reads or writes fields by offset. There is no getter and no constructor.
- **Attributes the compiler does not know are passed through to the host** (`[[ui::unit("bar")]]`), alongside
  `fminus::range` and `fminus::default`. TestingPlayground already reads `ui::unit`, `ui::label` and
  `ui::decimals` this way.
- A `param` is a `float`, `int` or `bool` only, set atomically from any thread; an `int` param whose range has at
  most 8 values, or a `bool`, gets one specialised copy of the method per value (knob versioning). Arrays and
  tables cannot be params.
- No globals, no function pointers, no `unsigned` or `long`, no growing containers; arrays are sized at compile
  time; `malloc` only outside `[[clang::nonblocking]]` code. So topology is data in fixed-size arrays, dispatch is
  `switch`, and all memory is taken in `init`.
- `[[clang::nonblocking]]` is checked through every call: the audio entry point can be proven free of allocation
  and locks by the compiler.
- `parallel` regions exist and are allowed in nonblocking code, but their pool threads are not real-time threads.
- Ahead of time (`fminus -c`) works but targets the build machine's CPU and gives a host no layout; with the JIT
  inside, neither matters.
- **Nothing here needs a compiler change.** §11 lists what would be nice, for you to weigh.

### 2.2 FunkGui

- With `FUNKGUI_WITH_JUCE=OFF`, `FunkGui::gpu` is not defined: CMake turns bgfx off, and the three platforms'
  surfaces, the frame pump and the editor host are JUCE's. `BgfxContext`, `BgfxSink`, `ShaderLibrary` and the image
  upload name no JUCE symbol. This is the work the research of 23 November called step 1.
- **What the Canvas draws:** rounded rectangles, hairlines, capsule segments at any angle, polylines, stroked
  arcs and cubics, discs, vertical-trapezoid area strips, text, images, clip rectangles, and a rectangle filled
  by a custom fragment shader. **What it does not:** transforms or rotation, gradients, filled polygons or
  sectors, rotated text, translucent strokes. A gauge, a plot, a table and a map are all within reach; a tilted
  piston or a glowing header is a shader's job.
- **Custom shaders** are bgfx `.sc` fragment shaders given 16 floats and up to 4 textures a draw. They must be
  compiled by bgfx's `shaderc`, which FunkGui does not link: we compile ours at build time for Metal, SPIR-V and
  Direct3D 11, as FunkGui compiles its own. The software rasteriser draws a placeholder where a shader goes.
- **Widgets:** dial, rule slider, segmented selector, latch, a code editor. No button, checkbox, dropdown, list,
  table, tooltip or layout helper; menus are a host service, and the only menu drawing is JUCE's.
- **Text** is one monospaced face, ASCII and 19 symbols, 10 px at the smallest. An instrument panel suits it.
- Input: `KeyEvent` has no key-up and no function keys. Holding a key for the throttle needs raw events from the
  host (§3.6).
- Linux's GPU path is Vulkan on X11 only.

### 2.3 TestingPlayground (what can be reused)

All of the following is JUCE-free there (no `juce` in any of these files; lines counted today, headers included):

| What | Files | Lines | Use here |
|---|---|---|---|
| The `.gui` language and its layout (flexbox-like rows, columns, grids, sections, tabs, pages) | `fminus/GuiSpec`, `editor/GuiLayout`, `GuiView` | 3,833 | The control pages |
| Controls in their looks (nine knob styles, sliders, switches, menus, buttons, displays, scroll, tab strip, dropdown) | `editor/GuiItems`, `Looks`, `Items`, `DialLayout`, `DisplayItem`, `Dropdown.h`, `Scroll`, `TabStrip` | 5,518 | The same |
| Shader panels: `.sc` sources given named inputs, a text library, a build cache | `fminus/Shaders`, `ShaderLib`, `editor/ShaderFont` | 2,339 | The pattern; here shaders build at build time |
| A Logitech G29 as data and decisions: its HID table, modes, range, spring, shift lights; Linux's transport | `plugin/WheelMap`, `WheelDriver`, `WheelEvdev` | 1,296 | Beside or under SDL3's own wheel driver (§13, question 6) |
| How a host plays an FMinus program: handovers by atomic pointers, glided dials, meters through a seqlock, an output guard, a stopped program faded out | `dsp/Engine`, `fminus/Patch` | 2,000+ | The pattern for `SimHost`, not copied |

The reference's own three shaders (the dash, the engine from above firing in its order, the header) come too, as
the first versions of those views.

### 2.4 The cloud container

Checked in this session: `apt` works (Xvfb and Mesa's software Vulkan are installable); `pip` has CMake 4.4 (FMinus
needs 3.30, the container has 3.28); public GitHub repositories clone through git, but **release archives by URL
are refused** (HTTP 403), which is how FMinus pins its dependencies; FMinus's own prebuilt LLVM for Linux (137 MB)
is a release asset of your repository and is reachable. So a cloud agent can build everything, given a bootstrap
script that fetches by git and unpacks LLVM (Phase 0 writes it; the research of 23 November did this by hand).
What it cannot do is compile Objective-C++ for macOS, open a window on a GPU, or play sound.

## 3. Architecture

### 3.1 The repository

```
TheEngineEngine/
  sim/         the sound engine: FMinus only
    engine.fm    the entry module and the per-sample order of work
    spec/        Spec, Controls, Telemetry: the three contracts (§3.3)
    core/        crank, cylinder, mechanics, heat
    valvetrain/  intake/  boost/  exhaust/  combustion/  ecu/  vehicle/  sound/  listen/
  host/        C++: SimHost (compile, schema, commands, snapshots, reload), the offline renderer, scenarios
  shell/       C++: main, the audio device, input devices, settings, files
  ui/kit/      the panel code from TestingPlayground, the binding to the schema, plots, gauges, tables, editors
  ui/views/    the screens
  shaders/     .sc fragment shaders, built by shaderc at build time
  content/     builds (engine, car, tune) and scenarios, as JSON
  tools/       tee-render, tee-dyno, tee-bench, tee-shot
  tests/       unit, scenario, validation, ui
  docs/        plan/, the manual
  scripts/     the cloud bootstrap, CI helpers
```

One CMake build with FMinus, FunkGui and SDL3 fetched at pins (git commits, since archives by URL do not reach
the cloud). The paths above are also the ownership boundaries of the parallel stage.

### 3.2 Threads and handovers

- **The audio thread** owns the simulator's instance. Each callback: apply the commands waiting in a lock-free
  queue (each a write of some bytes at an offset of the Spec), write the Controls, call `Engine::process`, then
  copy the Telemetry out through a seqlock and push the scope taps into rings. It never waits and never allocates.
- **The UI thread** owns the build as the user sees it, sends commands, and reads snapshots at frame rate.
- **Workers** compile (a reload is a new program compiled aside, its state carried over by field name, swapped
  under a short fade: the experiment of 23 November did this by hand), run dyno sweeps and offline renders on
  instances of their own faster than real time, and touch the disk.

Because every write to the Spec happens on the audio thread between two calls, Spec fields are plain fields:
tables, arrays and nested modules included, with no protocol inside the FMinus code. `param` is kept for the
few switches worth a specialised copy of the hot loop (fuel kind, stroke kind).

### 3.3 The contracts, and why the program describes itself

Three modules are the whole surface between the simulator and everything else:

- **`Spec`**: what the machine is. Nested by subsystem (`spec.valvetrain.intake.duration`), every field
  declared with `fminus::range`, `fminus::default`, `ui::label`, `ui::unit` and, where it applies, `ui::taper`
  and `ui::choices`.
- **`Controls`**: what the driver is doing this block: pedals, lever, buttons, the key, the mode (bench, dyno,
  strip), and the dyno's own commands.
- **`Telemetry`**: what the machine is doing: speeds, torques, pressures, temperatures, the turbo's operating
  point, per-wheel slip, timers; and the taps: a cycle of cylinder pressure against crank angle, valve lifts,
  pressures along the pipes.

At start the shell walks `Program::module("Spec")` and builds from it: a port for every field (what a widget
binds to), the reader and writer of a build's JSON (by path, so old files load into a grown Spec), an inspector
page for any field no `.gui` file has placed, and the manual's parameter tables. **Adding a physical parameter is
one declared line of FMinus**, and it appears in the app, in files and in the docs without a line of C++. That is
what lets six simulation agents work without queueing behind one interface agent.

### 3.4 The simulator's structure

- **Rates** as the reference has them: 96 kHz inside whatever the device's rate (its sinc resampler stays); the
  gas, the crank, the clutch and the junctions every sample; the ECU, the dials' smoothing, the pipes' retuning
  and the turbo's slow states every 16 samples; heat every 20 ms.
- **Seams between modules**, most of them already implicit in the reference: the valvetrain gives each cylinder
  its two effective areas at its angle; a cylinder and its two pipe ends exchange a wave and a flow (`Valve`); the
  intake system is the boundary at the runners' far ends; the boost module is a boundary on the intake side and a
  junction element on the exhaust side; combustion is called when an intake valve closes (`charge()`, as now); the
  ECU's outputs are a handful of numbers every 16 samples; mechanics sums torques into the crank's speed; the
  vehicle takes the crank's speed and torque and returns its speed (`car.step`, as now); sound sources take
  events and flows; the listener takes sources and gives two channels.
- **Topology as data.** Cylinders to 16. The exhaust is primaries, two levels of merging by an assignment table
  (which covers 4-1, 4-2-1, banks, 180-degree headers, unequal lengths, a log manifold, open stacks), then on
  each side a chain of slots (turbine, converter, crossover, resonator, muffler, valve, tail), each slot empty or
  one of a few kinds. Junctions go through the reference's `node()`.
- **A change the sound can follow is smoothed** (80 ms, as now); **one it cannot is made under the short fade**
  the reference already has for a new layout.
- **Off means the reference.** Every new subsystem has a setting at which the model is the reference's, so the
  port can be held to the original while the features grow.

### 3.5 The host and the JIT

`SimHost` compiles the program at launch (the files beside the executable, resolved by its include callback),
checks that `Engine::process` is nonblocking, builds the schema, and owns the handovers of §3.2. Development
builds compile with the loop budget and bounds checks on and watch the files; release builds turn both off. On
macOS the signed app needs `allow-unsigned-executable-memory`, as TestingPlayground's packaging already does.
Compile time for a program this size is not known (the largest in FMinus's tree, 1,431 lines, takes 0.16 s);
Phase 0 measures it, and if a cold start is slow the answer is a splash, then perhaps an object cache (§11).

### 3.6 The shell: FunkGui without JUCE

In FunkGui, as a release for you to merge and tag (v0.28.0, say):

1. **`FunkGui::gpu` without JUCE**: the bgfx context, the sink, the shader library and the image upload as a
   target that builds with JUCE off; the JUCE-bound files (editor host, frame pump, surfaces, GPU pictures,
   accessibility bridge) in a part of their own.
2. **`FunkGui::desktop`**: `DesktopHost`, the third of the family beside `EditorHost` and `WebHost`: an SDL3
   window; the frame clock (the display's rate while anything moves, sleeping in `SDL_WaitEvent` at rest); the
   tick, record, compare and submit sequence with the unchanged-frame rule; input converted by `WebInput.h`'s
   rules; resizing and DPI; `HostServices` (the clipboard, the system's file dialogs, cursors, unbounded drags).
3. **Menus drawn by the Canvas**, since the only menu look is JUCE's.
4. **A raw-event hook**, so an application can take key-up, gamepad and joystick events without a change to
   `Panel`'s frozen headers.
5. A PNG writer and a file backend for preferences without JUCE.
6. FunkGui's gallery in that window: the bring-up test, with nothing of TheEngineEngine in it.

bgfx stays at its pin (one window is all we need; the second-window work the research tied to a newer bgfx is not
on this path). In TheEngineEngine the shell adds the audio device (SDL3's stream callback first, behind a small
seam, in case its latency on macOS disappoints: not measured), the input devices (keys, a gamepad's triggers, a
wheel and pedals with force feedback through SDL3's joystick and haptic interfaces), settings and files.

### 3.7 Data

A **build** is three JSON documents that can be saved together or apart: the engine, the car, the tune (the ECU's
maps and settings). The schema of §3.3 is their format. Presets are builds that ship with the app. Units are a
display matter: the Spec is SI, and each quantity shows in the unit the user picks (kW, hp or PS; N m or lb ft;
bar or psi; km/h or mph).

## 4. Feature research

For each group: what is modelled and how, and what the user hears or sees. "W1" and "W2" are the waves of §10.

### 4.1 Engine architecture (W1)

| Feature | How | Hear / see |
|---|---|---|
| Any layout to 16 cylinders | Each cylinder's bank and crank pin; bank angles; a firing order. The host derives firing angles and refuses an order the crank cannot fire | A 90-degree V6 with common pins fires unevenly because its geometry says so; V4, VR6, straight eight, W12, V16; three- and four-rotor rotaries; peripheral or side ports |
| Bore, stroke, rod length, compression, valves a cylinder | The reference's bore and stroke logic, with the rod ratio and valve count freed | Rev limit by piston speed; breathing by valve area |
| Moving masses | Piston and rod inertia as a torque on the crank each sample; flywheel mass; shaking forces and moments by order from the layout; balance shafts | The torque's ripple at high revs; what shakes the cabin, the picture of the engine and the wheel's rim |
| Friction by part | The reference's rubbing, split by rings, bearings, valvetrain and accessories | Why a pushrod V8 and a four-valve four differ at 7,000 |

### 4.2 Valvetrain lab (W1)

| Feature | How | Hear / see |
|---|---|---|
| A cam for each valve group | Duration (at the seat and at 1 mm), lift, centreline, ramp sharpness; a lift curve generated from them; area from the valve's curtain and throat. Tables rebuilt at control rate | The cam editor: both lifts against crank angle, the overlap shaded, handles to drag |
| Variable timing | Phasers on either cam: a target from an ECU map, a slew rate that depends on oil pressure and temperature | Torque filled in low down; the idle smoothing out as overlap closes |
| Two-stage lift | Two lobe sets and a changeover with hysteresis and conditions, crossed over a cycle | The changeover: the induction note hardens and the torque steps |
| Continuously variable lift | Lift and duration as the load control, the throttle left open | Less pumping work; a different induction sound |
| Cylinder deactivation | Valves held shut and fuel off on a pattern | A V8 on four: the drone |
| Free valves | Opening and closing given per cycle: late or early intake closing, a second exhaust event | Miller and Atkinson cycles; the diesel's compression brake (§4.9) |
| Valve float | Spring rate, valve mass and ramp against speed: past the limit the valve leaves the cam and bounces | Power falling over and a clatter, where a real valvetrain would |
| The flow bench | Flow against lift computed for the head as specified | The number tuners quote for a head |

A big cam's lope, the reversion into the runners and the pull of a tuned length already emerge from the
reference's residual-gas and waveguide model. The lab mostly gives that model honest inputs.

### 4.3 Induction (W1)

A single throttle with a plenum (as now), or a throttle a cylinder with open trumpets, each radiating as a pipe
end (the bark of individual throttle bodies); runner length and bore as parameters (the tuning peaks are the
waveguides' own); a two-length runner with a changeover; plenum volume; an air box as a volume and a snorkel
rather than a resonator on the flow. The intake's sound becomes what the runners and the box actually let out.

### 4.4 Forced-induction lab (W1)

| Feature | How | Hear / see |
|---|---|---|
| A turbo as a machine | A compressor map (speed lines and efficiency as a fitted family, sized by wheel diameters and trim); a turbine as a nozzle with an efficiency against blade speed ratio, sized by wheel and A/R; a shaft with inertia. The turbine is a junction element between the collector and the downpipe, the compressor a boundary on the charge pipe | Lag, boost threshold and choke that follow from the sizes chosen; the exhaust muffled by the turbine because it is one; the operating point moving on the map |
| Wastegate | A parallel orifice: internal (back into the downpipe) or external to its own pipe | An external gate's own open pipe |
| Variable geometry | Vane position from an ECU map changes the nozzle's area | Early spool without late choke; a diesel's exhaust brake |
| Twin-scroll, twins, sequential, compound | Collector groups kept apart into two turbine entries; two units side by side, in sequence or in series | Why pairing the right cylinders spools sooner |
| Surge | The compressor's characteristic with its reversed-flow branch, the pipe's inertia and the plenum's volume (Greitzer's model): flutter is then the system's own oscillation | Flutter with the blow-off valve shut, at the rate the plumbing gives |
| Blow-off valve | Venting to air, recirculating, or none; spring preload | Its sound from the vented flow |
| Intercooler | Effectiveness, pressure drop, volume, heat soak | Intake temperature, and the knock that follows it |
| Superchargers | Roots (a volume a turn, no internal compression), twin-screw (internal compression), centrifugal (a compressor on a geared belt); pulley ratio; bypass; the drive's torque from the air's work over the efficiency | Three different whines and three different boost curves |
| Anti-lag | Late spark and fuel into the manifold off-throttle | The bangs, and boost held through a lift |
| Nitrous | A shot by size and window; its oxygen and its cooling; the bottle's pressure falling | A step in torque; a purge |
| Electric assist | A motor's torque on the shaft | Boost before the exhaust can make it |

### 4.5 Exhaust builder (W1)

Header kinds by the assignment table of §3.4, with primary length and bore, stepped primaries, and collector
kinds; a crossover as an H or an X; a converter present or not; resonators (a quarter-wave side branch, a
Helmholtz chamber) against the drone at a chosen speed; mufflers by kind (straight-through, chambered,
three-pass) with size; a valve that bypasses the muffler by speed and load; tails rear, side or up, single to
quad. The walls' temperatures the reference already tracks drive the picture: headers that glow.

### 4.6 Combustion and fuels (W1)

- **Fuels as a table**: stoichiometric ratio, heating value, latent heat (charge cooling), knock resistance, flame
  speed, lean and rich limits. Pump grades, race fuel, E85, methanol, diesel; propane and hydrogen fit the table
  later.
- **Mixture as a variable**: lambda changes the heat released, the burn's speed, the exhaust's temperature, the
  knock margin, the misfire limit and what burns in the pipe.
- **Knock from the end gas**: the unburned gas's temperature and pressure history integrated to autoignition
  (Livengood and Wu's integral over a delay correlation), in place of the threshold on compression. Boost,
  intake temperature, spark and fuel then act through one mechanism.
- Peak pressure, exhaust temperature and mean effective pressures become telemetry: what a dyno operator reads.

### 4.7 ECU and tuning (W1)

Maps, each a table with axes of speed and load, interpolated at control rate, edited in the app with the live
operating point on it: spark, lambda, boost target (by gear too), each cam's phase, the pedal's map. Settings:
the limiter's speed and style (fuel cut, spark cut, soft), launch control and two-step, flat-shift, overrun
fuelling and retard (how much it pops and bangs), idle, knock control on or off, traction control by slip, the
shift light. A base-map generator gives a safe starting tune for any build, so nothing the user builds refuses
to run. The driving aids for keyboard players (an automatic clutch, automatic shifts) are settings here too.

### 4.8 Two-strokes (W2)

A second cylinder model: ports opened by the piston's position, a crankcase as a second volume fed through a reed
or a rotary valve, scavenging between perfect displacement and perfect mixing with a share short-circuited. The
expansion chamber is the pipe: a header, a diffuser, a belly, a baffle cone and a stinger, as a run of short
ducts of stepped area, so its returning wave plugs the port and makes the power band and the sound together. A
power valve (the exhaust port's height against speed) is this family's variable valve timing. Singles, twins and
triples; karts and dirt bikes.

### 4.9 Heavy diesels (W2)

On top of the reference's diesel: a compression-release brake (the exhaust valves cracked open at the top of
compression with the fuel off, a free-valve event of §4.2: the bark is the cylinder model's own), an exhaust
brake (a flap, or the turbo's vanes), compound and variable-geometry turbos (§4.4), injection timing, pressure
and a pilot injection (which softens the clatter the reference already makes), over-fuelling past the smoke
limit, the eighteen-speed with its splitter, trailer weight, grades, and the truck's own noises (the air
system's dryer and brakes, the fan's clutch).

### 4.10 Motorcycles (W2)

Layouts added (V4s and their firing orders, a boxer twin); very light cranks; the engine's sound dominated by the
air box; a sequential gearbox with a quickshifter and an auto-blipper; a slipper clutch; chain drive with its
sprockets and lash; and the vehicle a bike: one driven tyre, a high centre of mass on a short wheelbase, so the
launch is limited by the front wheel lifting before the tyre lets go.

### 4.11 The vehicle, in a straight line (W1)

| Part | Model |
|---|---|
| Tyres | A slip curve with a peak and a falloff (Pacejka's form) for each driven wheel; grip that depends on load; compounds (economy to slick, drag radial, winter); a temperature state (a burnout warms a slick); surfaces (dry, damp, wet, snow, a prepared strip) |
| Weight transfer | Mass, wheelbase, centre of mass height and distribution; a first-order squat; grip moves to the rear on launch |
| Drive layout | Front, rear or all-wheel drive; a centre split (fixed, viscous, active) |
| Differentials | Open, limited-slip (preload and ramp), locked, for each driven axle: with two wheels an axle and a small asymmetry, an open differential spins one tyre |
| Gearbox | Manual with a clutch pedal, sequential dog box (clutchless upshifts on an ignition cut), dual-clutch, torque-converter automatic (stall, multiplication, lock-up), CVT; any ratios and final drive; one to ten gears |
| Aero | Drag area and lift or downforce front and rear; air density from temperature and altitude |
| Brakes | The reference's, with a line lock and a handbrake |

The modes are instruments more than games: **free-rev**; an **engine dyno** (an absorber holds a speed or sweeps
at a set rate, logging torque and power); a **chassis dyno** (the same at the wheels, with the driveline's loss
and the tyres' slip); the **drag strip** (the tree, reaction time, 60 ft, eighth, quarter, trap speed, a time
slip); a **roll** or **top-speed run**; a **cruise** at a held speed and grade, where a drone is found. Because
the dyno is the simulation, a worker can run a sweep on a second instance faster than real time (how much faster
is one of Phase 0's measurements): change the cam, and see the predicted curve move before driving anything.

### 4.12 Not chosen for v1

Drag-race exotics (nitromethane, the big Roots blower, open zoomies, the trans-brake) were not picked. The fuel
table, the Roots model and open stacks leave the door open. §12 lists the rest of what waits.

## 5. Sound

All four directions were chosen; they are one design.

- **Sources with places.** Each thing that sounds is a signal with a position on the car: each tailpipe, the
  intake's mouth or trumpets, the block, the turbo, the wastegate's pipe, the blow-off valve, the supercharger,
  the gearbox, the differential, each tyre, the wind, the brakes. Where the engine sits (front, mid, rear) is a
  parameter, as it is of a car.
- **A listener.** A place and a facing, on or off the car. For each source: the path's delay (so a moving car
  bends in pitch: Doppler from fractional delay lines, which the reference already runs for its tailpipes), the
  level with distance, the air's loss of highs, the source's directivity, and a second path off the ground, whose
  comb moves with the distance and is most of what an outdoor recording of a car sounds like.
- **Cameras**: at the tailpipes; trackside as the car passes; a chase; over the engine; the driver's seat; and a
  replay: since the simulation is deterministic, a run's inputs can be rendered again from another place, or to
  a file.
- **The cabin**: structure-borne sound from the shaking forces of §4.1 through the mounts, the exhaust's drone
  through the floor, the induction through the firewall, wind and tyre noise with speed, the cabin's modes
  (which the reference has), windows up or down.
- **Places**: an open field (nothing but the ground and a distant slap), a tunnel, a garage, a street between
  buildings: presets of the reverb the reference carries, with early reflections that fit each.
- **Mechanical detail**: gear whine by tooth count and shaft speed (straight-cut or helical), the differential,
  a belt, the fan, the fuel pump priming at key-on, port or direct injectors, a wastegate's chatter, the
  valvetrain's character by type. Each driven by the model's events and speeds, as the reference's are.
- **The exhaust's tone**: the muffler kinds and resonators of §4.5 as real elements; small differences between
  cylinders; and an offline render at twice the internal rate for exports.
- **Output**: stereo to the device at its rate; a recorder; a level that holds across builds (the reference
  normalises by layout from measurements; here it has to be computed for any build).

## 6. Graphics and interface

**The rule that follows from §2.2:** instruments and plots are drawn with the Canvas, so the software rasteriser
can picture them in a cloud session; the hero pictures are fragment shaders, fed a few numbers and a texture or
two, and are judged on a GPU.

| Screen | What is on it | Drawn by |
|---|---|---|
| **Build** | The engine as a cutaway that runs: pistons, valves lifting by the real cam, gas coloured by pressure and heat, the turbo turning, flame in the pipe on a pop, headers glowing. Beside it, the pages of controls by subsystem | A shader for the cutaway (the reference's `layout.sc` is its seed); the `.gui` pages for controls |
| **Valvetrain** | The cam editor; the valve timing diagram; the flow bench's curve | Canvas |
| **Boost** | The compressor map with the operating point's trail and the surge line; shaft speed, pressures and temperatures along the charge path | Canvas |
| **Exhaust** | The system laid out as built, pressure waves moving along it | A shader with a texture of pressures along the pipes |
| **Tune** | The maps as tables and heat maps, the live cell lit; limiter, launch and overrun settings | Canvas, pages |
| **Dyno** | Torque and power against speed, runs overlaid, boost and lambda under them; the predicted curve from the background sweep | Canvas |
| **Drive** | The dash (the reference's `cluster.sc` grown: tachometer, speed, boost, lambda, temperatures, shift lights); a side view of the car that squats, spins and smokes; the strip's tree and time slip | A shader for the dash, Canvas for the rest |
| **Scope** | Cylinder pressure against crank angle and against volume; pressures at the ports; the output's wave; a spectrum against speed, where the firing orders stand as lines | Canvas |
| **Garage** | Builds and presets; compare two | Pages |

Text is FunkGui's one face; the look is an instrument's, dark by default. **Input**: the keyboard (a key held is
a pedal, eased), the mouse, a gamepad's analogue triggers, a wheel with pedals and a shifter. Force feedback
(the engine's shake in the rim, the limiter's buzz, the tyres letting go) is a late item of the second wave.

## 7. Performance

**Measured by you, not by me:** TestingPlayground's load meter reads 30 to 40 % for the reference at 512 samples
on your Mac. So the reference alone takes a third of one core, and the headroom on that core is about two and a
half times. Wave 1 adds a turbo solved every sample, more pipes and more cylinders: **the budget is tight from the
start**, and performance is a package of wave 1, not a late polish. Phase 0 measures the port on this container
(release options: no bounds checks, no loop budget, which the plugin's playing copy has while a dial moves) and
gives each package a budget in nanoseconds a sample for a reference build.

The levers, in the order I would pull them: work that can run every 16 samples does; tables for anything
transcendental (the reference does this for cam areas and orifice flow); `param` switches so the compiler
specialises the hot loop; quality tiers (shelves a pipe, noise bands); and last, because it is real engineering:
any two parts joined only by a pipe longer than 16 samples of delay can be advanced in parallel for 16 samples
(the two banks, the intake side, the exhaust after the collectors).

## 8. Verification, from a container with no screen and no speakers

### 8.1 The simulator

- **The port equals the original.** The first test: the reference, moved to the new contract, renders the same
  audio as the reference's own files for its three presets, to a tolerance.
- **Scenarios.** A script of inputs over time, a build, and assertions on telemetry and on the audio: finite,
  bounded, no steps, the same result whatever the block size and the device's rate, the spectrum's lines at the
  firing orders the layout predicts.
- **Physics against the world.** Each archetype build is held to ranges a textbook or a dyno sheet gives: mean
  effective pressures, volumetric efficiency's shape, peak cylinder pressure, idle vacuum, exhaust temperature,
  turbo shaft speeds and spool times, and for cars the quarter mile and trap speed that weight and power predict.
- **Conservation and passivity.** Mass in equals mass out over a run; a system with the fuel off comes to rest;
  an hour of random inputs never leaves bounds.
- **Real-time safety.** The entry point is declared nonblocking, which the compiler checks; test builds run with
  the loop budget and bounds checks on.

### 8.2 The interface

Each screen runs in FunkGui's `HeadlessHost` with scripted input; frames are fingerprinted and written as PNGs
from the software rasteriser (with a small encoder of our own, since FunkGui's needs JUCE). An agent looks at its
own pictures; you look at a contact sheet for each wave. Shader panels show as placeholders there. The real GPU
path can run in the cloud on Xvfb with Mesa's software Vulkan: TestingPlayground's CI already runs its GUI probes
that way (its `build.yml`), so agents can see shaders and a window's whole frame; Phase 0 sets it up here.

### 8.3 Builds on the systems agents cannot touch

A CI workflow builds the app on macOS and Windows runners on every push, so Objective-C++ and Direct3D code at
least compiles (§13, question 3).

### 8.4 What only you can do

| Checkpoint | When | What you run on your Mac |
|---|---|---|
| M1 | Today: Windows about 11:00 your time, macOS in the afternoon | FunkGui's gallery in the SDL3 window: it opens, resizes, runs at the display's rate, takes input. Then our shell: the ported engine heard through the device, a key as the throttle |
| M2 | End of wave 1 | The app with its control pages and wave 1's physics. A listening pack: rendered files of each archetype, each feature on and off. Your wheel |
| M3 | End of wave 2 | Every screen, the shaders, the cameras and places; a signed build |

## 9. Risks

| Risk | Why it is real | What the plan does about it |
|---|---|---|
| The window fails on macOS | New host code, a resize path the research flagged, no way to run it in the cloud | M1 early and alone; SDL3 hands bgfx the window through its own interfaces, so little new platform code; the shell sits behind a seam |
| Sound judged by numbers | Agents cannot hear | "Off means the reference"; listening packs; an offline renderer you can run yourself |
| Parallel work in one loop | Six agents, one `sampleTick` | Phase 0 fixes the seams and the call order; only the integrator edits `engine.fm`; each module has a no-op default |
| A new element destabilises the system | Surge, turbines, stiff clutches and tyres, explicit steps at 96 kHz | Elements are passive at their junctions by construction; soak tests; the reference's own cautions kept as rules |
| CPU | More cylinders, more ducts, a turbo each sample | Measured budgets from Phase 0; the levers of §7 |
| FMinus surprises at this size | A program several times the largest yet; compile time; no function pointers | A Phase 0 spike; agents never patch the compiler: they report, you decide |
| Shaders unseen in the cloud | The rasteriser draws a placeholder | The Xvfb experiment; Canvas first; M checkpoints |
| Scope | Everything was chosen | Two waves with a gate between; each family of §4.8 to §4.10 is separable and can slip without blocking the rest |
| FunkGui's process | Its rules are written for agents on your Mac with a card and a lead | §13, question 4, before stage 2 writes any FunkGui card |

## 10. Work packages

**Phase 0, one session, serial (the keel).** The repository, the build and the cloud bootstrap; the reference
ported to the new entry module and split into the modules of §3.1 with their seams, sounding as it did; `SimHost`
and `tee-render` in their first form; the three contracts and the schema reader; the scenario harness; the
measurements (CPU, compile time); the experiments (Xvfb, FunkGui's GPU layer compiled without JUCE on Linux). It
ends when a second session could add a parameter and a test without asking anyone anything.

**Wave 1, ten agents in parallel**, each owning the paths named:

| | Package | Owns | Delivers |
|---|---|---|---|
| A | FunkGui desktop | FunkGui: the items of §3.6 | The gallery in a window; M1 |
| B | Shell | `shell/` | Device, inputs, settings, files; the app's loop over `DesktopHost` and over `HeadlessHost` |
| C | Host and tools | `host/`, `tools/` | Commands, snapshots, reload, the dyno runner, the bench, scenario and validation runners |
| D | Mechanics and layouts | `sim/core/` | §4.1: layouts as data to 16, moving masses, friction; the integrator of `engine.fm` |
| E | Valvetrain and induction | `sim/valvetrain/`, `sim/intake/` | §4.2, §4.3 |
| F | Forced induction | `sim/boost/` | §4.4 |
| G | Exhaust | `sim/exhaust/` | §4.5, and the turbine's junction with F |
| H | Combustion and ECU | `sim/combustion/`, `sim/ecu/` | §4.6, §4.7 |
| I | Vehicle | `sim/vehicle/` | §4.11 and its modes |
| J | UI kit | `ui/kit/` | The ported panel code, the schema binding, plots, gauges, tables, the map and curve editors, navigation; first pages for every subsystem |

**Gate: M2**, an integration pass, your listening and your corrections.

**Wave 2, eight to ten agents:** the listener, cameras, cabin and places (§5); mechanical sources; two-strokes
(§4.8); heavy diesels (§4.9); motorcycles (§4.10); the views in two packages (Build, Exhaust and Drive with
their shaders; Valvetrain, Boost, Tune, Dyno, Scope); content and validation (archetype builds held to §8.1's
ranges, the manual); wheel and force feedback; packaging and performance.

Each package works on a branch of its own and opens a pull request against an integration branch; packages touch
only their paths and the contracts' own files by appending. Stage 2 writes, for each: its goal, its paths, the
seams it implements with their signatures, the tests that define done, its budget, and what it must not touch.

## 11. What changes in the other repositories

**FunkGui**: §3.6's six items, as one release. Its public headers gain a `desktop/` family and nothing existing
changes. By your word at the review, FunkGui's rules for FCompressor's agents do not bind this work: pull requests
in FunkGui and FMinus are ours to open, merge and tag unless you say otherwise. Existing goldens are still never
re-blessed to make a test pass; a new test's goldens are adopted with the release that adds it.

**FMinus**: nothing is required. What would help, for you to take or leave as the compiler's author: an object
cache so a second launch skips the compile; a note in the docs on the prebuilt LLVM archives (they exist and the
build guide does not mention them); pins that can be fetched by git as well as by archive (the cloud refuses the
archives); and later, if a small release build is ever wanted, a CPU baseline flag and a layout export for
ahead-of-time hosts.

**TestingPlayground**: read, not changed. Its code is copied at `47bac40` with a table of each file's origin and
hash, as FunkGui's `SEED.tsv` does.

## 12. Not in v1, and what would bring it in

- **Cornering and circuits**: your decision; a lateral tyre model and a track are a second simulator.
- **Drag-race exotics**: not chosen; the pieces they need are mostly in place.
- **Engine damage** (a lean piston, a missed shift's bent valves): fun, and a design question of its own (§13).
- **Carburettors** and their transient fuelling; **hybrids**; **radials**.
- **Ahead-of-time release builds**: when size matters more than reload.
- **MIDI**: SDL3 has none; a small library when the keys are wanted again (§13).
- **A second window, a system menu bar, accessibility, text beyond ASCII**: the desktop research's later steps;
  this app does not wait for them.
- **Users' own FMinus mods**: the JIT makes it possible; an interface for it is its own piece of work.

## 13. Questions for the review

Each with what I will assume otherwise.

**Answered at the review (2026-11-24):** (2) the load meter reads 30 to 40 % at 512 samples (§7). (3) GitHub
Actions: yes, with caching so runs stay short. (4) FunkGui's FCompressor-era rules are set aside; full permission
to open and merge pull requests and to tag in FunkGui and FMinus. (6) No opinion: SDL3's driver, TestingPlayground's
logic for the G29's lights and range where SDL lacks them. (7) No MIDI; keyboard and mouse controls are required,
for players with no wheel. (13) Checkpoints start today: Windows about 11:00, macOS in the afternoon, with as much
done before them as can be. The other questions stand at their assumptions.

1. **Platforms.** macOS on Apple silicon is the product; Linux x86-64 is kept working because agents live there;
   Windows is kept compiling in CI and otherwise untested. *Assumed: yes.*
2. **CPU today.** What does the plugin's load meter read for the V8 and the V12 on your Mac, at which buffer
   size? *Assumed: unknown until Phase 0 measures.*
3. **GitHub Actions** for macOS and Windows builds of TheEngineEngine and of the FunkGui branch. *Assumed: yes.*
4. **FunkGui's process.** Its rules forbid an agent to push, and scope it to a card written in FCompressor's
   docs. For this work: cards written in this repository's plan, the work pushed to the FunkGui branch this
   session was given, a pull request for you to merge and tag, goldens left as candidates? *Assumed: yes.*
5. **Copying from TestingPlayground**, which has no licence file: copied into this repository under GPL-3.0 (which
   FMinus and FunkGui make the app's licence anyway), namespace renamed, free to diverge from the original.
   *Assumed: yes.*
6. **Your wheel.** SDL 3.4 has its own driver for some Logitech wheels (the G25, G27 and G29 with force feedback,
   by its pull request's description); TestingPlayground has one written for your G29, lights included. *Assumed:
   SDL3 for every device, TestingPlayground's logic kept for the G29's lights and range if SDL lacks them.*
7. **MIDI and the keys.** Is playing the engine from a keyboard (the reference's KEYS and PITCH modes) still
   wanted? *Assumed: not in v1.*
8. **Units**: both systems, chosen per quantity. *Assumed: yes; first run follows the system's locale.*
9. **Names.** Presets as generic archetypes ("3.0 straight six, twin turbo"), no makes or models. *Assumed: yes.*
10. **Old presets.** The reference's three (Large Diesel, V10, V8) converted into builds. *Assumed: yes.*
11. **Damage.** Should a build be able to hurt itself? *Assumed: no; knock and heat cost power, never the
    engine.*
12. **Mods.** Reload from edited `.fm` files as a developer's switch only. *Assumed: yes.*
13. **Checkpoints.** Three sessions of yours at M1, M2, M3 (§8.4), M1 within days of wave 1 starting. *Assumed:
    yes.*
14. **Listening packs.** How should rendered audio reach you: files attached to CI runs, or rendered on your Mac
    by `tee-render` from committed scenarios? *Assumed: the second; nothing binary in git.*
15. **Waves.** Two waves with a gate, as §10, or one long wave? *Assumed: two.*
16. **Identity.** The app's display name, bundle id and icon; FunkGui's seams would be `TheEngineEngine`, `Tee`,
    `TEE_`. *Assumed: "The Engine Engine", yours to finalise before M3.*

## 14. What was read, what was checked, and what is neither

**Read by me, in full:** the reference's `program.fm`, `cylinder.fm`, `pipes.fm`, `car.fm`, `layouts.fm`,
`sounds.fm`, `panel.gui`, and of `engine.fm` its state, `configure`, the heat, the valves and pipes, the control
tick, the drive logic, the per-sample tick, both charge functions and the render (about 2,200 of 2,621 lines; not
the key handling, the listener's geometry or `init`); of `hall.fm` its header only, and the filters and the three
shaders not at all beyond their opening comments. FMinus's desktop research of 23 November, whole.
TestingPlayground's `HostFile.cpp`, the headers of its shader, GUI and wheel code, and the first 800 lines of its
CLAUDE.md.

**Read by two surveys run for this note, which I spot-checked at five places** (the attribute structure, the
rules for `param`, `#include`, FunkGui's JUCE gate, `KeyEvent`): FMinus's public header, library, program and
building guides and its plugin example; FunkGui's CMake, panel, canvas, shader, widget, host and test headers.
Every statement in §2.1 and §2.2 rests on those.

**Checked today by running a command:** which hosts the container reaches, the packages `apt` offers, the LLVM
release asset, SDL's tags (3.4.20 is the newest), line counts and the absence of JUCE in the files of §2.3.
**Read on the web:** SDL's page for its Logitech wheel driver hint (available since 3.4.0); the list of wheels is
from a pull request's description in a search result, not from the source.

**Neither run nor read at a primary source:**

- Anything built: FMinus, FunkGui, SDL3, the reference outside TestingPlayground. No sound was rendered.
- §7's cost estimate, and any compile time for a program of this size.
- That FunkGui's JUCE-free GPU files compile without JUCE (the research said the same of itself).
- That bgfx's Vulkan back end runs on Mesa's software driver under Xvfb.
- SDL3's audio latency on macOS, and its haptic support for a G29 there.
- Whether an `#include` may sit inside a module's body (the language guide says a file is read "where the line
  is, as in C"; not tried).
- The models named in §4 are from the literature as I know it (Appendix A), not from a source opened today.

## Appendix A: the models this plan leans on

From memory; stage 2 cites each where it is used. Heywood, *Internal Combustion Engine Fundamentals* (the Wiebe
burn, heat transfer, friction, knock); Livengood and Wu (1955) and Douaud and Eyzat (1978) for autoignition;
Chen and Flynn (1965) and Patton, Nitschke and Heywood (1989) for friction; Benson, and Winterbone and Pearson,
for wave action in manifolds; Blair, *Design and Simulation of Four-Stroke Engines* and *of Two-Stroke Engines*
(ports, scavenging, expansion chambers); Watson and Janota, *Turbocharging the Internal Combustion Engine*;
Greitzer (1976) for surge; Guzzella and Onder, and Eriksson and Nielsen, for map-based compressor and turbine
models and engine control; Munjal, *Acoustics of Ducts and Mufflers*; Pacejka, *Tire and Vehicle Dynamics*;
Gillespie, *Fundamentals of Vehicle Dynamics*; Smith, *Physical Audio Signal Processing* (waveguides). The
reference already cites Wolfer, Watson, Daw, Silva, Kirchhoff, Lyon, Dittus and Boelter in its comments. As
prior art, Ange Yaghi's open-source engine-sim also simulates the gas and synthesises its sound, by a different
route (secondary sources say it shapes its output by convolution with impulse responses; I did not read its
code).
