# The interface, redesigned

*Stage 2, added 2026-11-24 12:50 UTC on the owner's word: "The simple engine sim from TestingPlayground had to use
complex shaders to get a detailed interface, but this limitation should not apply here. Feel free to completely
redesign the GUI away from the original." This file replaces 00-abstract-plan.md §6 and the views of
[w2-cards.md](w2-cards.md) (V1, V2), and it moves the interface's design out of wave 2: it starts now. The kit it is
built with exists (cards J1 and J2: `docs/ui-kit.md`, `docs/gui-language.md`).*

## 1. What changes

The reference drew its dash, its engine and its header inside three fragment shaders because a plugin's generated
panel gave it nothing else to draw with. Here we have a Canvas, widgets, and a window of our own. So:

1. **Nothing a person reads is drawn by a shader.** Dials, numerals, curves, tables, diagrams and labels are
   widgets on the Canvas: sharp at any pixel density (the reference's dash loses the "1" of "100" at density 1),
   pictured by the software rasteriser in a test, listed for a screen reader. A shader is light and atmosphere
   only: a glow on a hot header, flame at a tail, tyre smoke, heat shimmer. Take every shader away and no fact is
   lost.
2. **The application is a workshop, not a faceplate.** The reference's panel is three tabs of knobs under a
   picture. This is a place to build an engine, see what the change did, and drive it: the machine drawn as it
   is built, the part you click is the part you edit, and a dyno curve that moves when you change a cam.
3. **The reference's panel is retired from the application.** `RefPanel` stays as a developer's view
   (`--ref-panel`) for comparing with the plugin, and as the test that panel shaders build. It is what checkpoint
   M1 shows only because M1 came before this.

## 2. The window

```
+--------------------------------------------------------------------------------------------------+
| THE ENGINE ENGINE   build name *   3.0 L straight six, twin turbo, E85      load  out  units  ...  |  top bar
+------+----------------------------------------------------------+--------------------------------+
| GAR  |                                                          |  INSPECTOR                     |
| ENG  |   STAGE                                                  |  the selected part's           |
| VLV  |   the machine as built, alive: drawn from the build,     |  parameters, grouped;          |
| AIR  |   moving with the telemetry; click a part to select it   |  generated from the Spec,      |
| EXH  |                                                          |  arranged where it matters     |
| TUNE |                                                          |--------------------------------|
| CAR  |                                                          |  DYNO RIBBON                   |
| DYNO |                                                          |  torque and power, predicted   |
| DRV  |                                                          |  for the build as it is now;   |
| SCP  |                                                          |  the last run ghosted          |
+------+----------------------------------------------------------+--------------------------------+
| START  mode | rpm bar with numerals and redline | gear | speed | boost | lambda | temps | pedals  |  cockpit
+--------------------------------------------------------------------------------------------------+
```

- **Top bar**: the build's name and whether it has unsaved changes; the engine in a sentence (made from the Spec);
  the simulator's load; the output's level; units; settings; undo and redo.
- **Rail**: the workspaces (§3). Words, not icons (the face has no icon set; a small pictogram drawn with
  primitives beside each word is welcome). It narrows to the words' first letters in a small window.
- **Stage and inspector**: every workspace is these two. The stage is a picture of that part of the machine,
  built from the Spec (so it changes when the build does) and moved by telemetry. Selecting something on the stage
  shows its parameters in the inspector; the inspector's pages are J2's generated pages, arranged by a `.gui` file
  where order and grouping matter. Nothing is editable only by a picture: every field is in an inspector.
- **Dyno ribbon**: under the inspector in every workspace but DYNO and DRIVE: the predicted torque and power of the
  build as it stands (card C's dyno runner, in the background, a moment after each edit), with the curve before
  the edit ghosted behind it, and the peak numbers. This is the application's signature: change something, watch
  the curve answer. It folds away.
- **Cockpit**: always there, so the engine can be started and revved from anywhere: a start button; the mode
  (bench, dyno, drive); the tachometer as a wide bar with numerals, a redline zone and shift lights, and the speed
  in digits; the gear; road speed; boost or vacuum; lambda; coolant and oil; the pedals as three bars that the
  mouse can drag and that show what the keyboard, a gamepad or a wheel is doing. The keys of `docs/controls.md`
  work in every workspace unless a text field has the caret.

Smallest window 1100 by 700 logical px; resizable; it must read at density 1 (text at 10 px or more, lines on
pixels) and at 2.

## 3. The workspaces

Each: what its stage shows, and what its inspector holds. "Live" means from telemetry or the Scope.

| Workspace | Stage | Inspector |
|---|---|---|
| **GARAGE** | Builds as cards: name, a sentence, peak torque and power, a small power curve. Two builds side by side: curves overlaid, the fields that differ | The build's notes; load, save, duplicate; the three parts (engine, car, tune) loadable apart |
| **ENGINE** | The block from above, drawn from the layout: banks, cylinders numbered, the crank's pins; each cylinder lights as it fires, live, in its order; the firing intervals as a ring. Beside it one cylinder in section: the piston on its rod and crank, both valves lifting by the real cam, the gas tinted by pressure, live or slowed | Layout and its presets; bore, stroke, rod, compression; masses; friction; a pressure-volume loop |
| **VALVES** | The cam editor: both lifts against crank angle, overlap shaded, handles to drag for centreline, duration and lift; the classic timing ring (arcs for each valve's open period round the cycle); phaser ranges as bands; the flow bench's curve | Cams, phasers, stages, lift control, deactivation, springs |
| **AIR** | The charge path as a row of parts, left to right, each with its live pressure and temperature: filter, compressor, intercooler, throttle, plenum, runners. The compressor's map with the operating point's trail, the surge line and the islands; the shaft's speed; the gate and the blow-off valve as they open | Induction; turbos and superchargers; intercooler; anti-lag; nitrous |
| **EXHAUST** | The system as built, drawn from the assignment tables and the slots: primaries into collectors into pipes, each element a shape; walls tinted by temperature; pulses travelling, live from the Scope; each radiating end marked. Click an element to select it | Headers; crossover; converter; resonators; mufflers; valve; tails |
| **TUNE** | The selected map as a heat map and a table, the live cell lit, the knock and lambda traces under it; slices through the map as plots | The maps and their `auto` switches; bake; limiter, launch, overrun, traction, idle; fuel |
| **CAR** | The car from the side: wheelbase, centre of mass, the load on each axle as bars, live; the driveline as a diagram for the layout chosen; the tyre's grip curve with each wheel's live point; the gearing chart (road speed against engine speed in each gear) | Mass and geometry; tyres; differentials; gearbox and ratios; aero; brakes |
| **DYNO** | The big plot: torque and power (and anything else from telemetry) against speed; runs listed, overlaid, named, exported; an engine or chassis run started and watched as it draws | The run's plan: from, to, rate or steps, pedal |
| **DRIVE** | Round tachometer and speedometer (the `Gauge` widget: arcs, ticks, numerals and a needle, all primitives), boost, lambda, temperatures; the car from the side on its road, squatting and spinning its tyres; the strip's tree and the time slip; the wheels' slip as bars | Mode, surface, grade; aids (automatic clutch, traction control); the strip |
| **SCOPE** | Cylinder pressure against crank angle and against volume; the ports' pressures; the output's wave; a spectrum, and a spectrum against engine speed where the firing orders stand as lines | What is plotted; the listener (the camera, the place, the mix) until the listener has a workspace of its own |

## 4. The look

- **Ground and ink**: FunkGui's graphite theme and its ink steps; a paper theme must work too.
- **Colour means something, and is never the only sign**: the accent (the reference's orange) is the thing
  selected or active; cool blue is intake air and anything cold; orange to red is exhaust and heat; green is within
  limits; amber is a warning (knock, the limiter, slip). Each also has a label, a shape or a position.
- **Type**: FunkGui's scale, as it is. Numbers large and tabular; units small beside them; labels in the small
  spaced capitals the theme has. No text under 10 px.
- **Line**: hairlines for grids and frames; 1.5 to 2 px for traces and needles; no gradients (there are none) and
  no chrome. A gauge is an arc, ticks, numerals and a needle; a plot is axes, a grid and traces.
- **Motion**: eased, never stepped; a needle has a little inertia; nothing moves that carries no information. A
  panel at rest draws nothing (`needsDraw`), so the application idles near 0 % of a core, as FunkGui's gallery does.
- **Density**: designed at 1, checked at 2. Every golden and contact sheet at both.

## 5. How it is built

`ui/app/` (the frame) and `ui/views/<workspace>/` (one directory a workspace). A workspace is a class the frame
owns and lays out:

```cpp
namespace tee::ui
{
    struct WorkspaceContext            // what every workspace is given; owned by the frame
    {
        BuildDoc&         build;       // the document: fields by path, ports for controls, undo (J2's)
        TelemetrySource&  live;        // the newest block's values by name (card B's interface)
        ScopeSource&      scope;       // the Scope's arrays by name, when a workspace asks (new: U1 defines it)
        DynoSource&       dyno;        // the predicted curve and the runs (new: U1 defines it over card C's runner)
        Units&            units;
        Selection&        selection;   // what is selected on the stage: a path prefix into the Spec
        const AppServices& app;
    };

    class Workspace
    {
    public:
        virtual ~Workspace() = default;
        virtual const char* id() const = 0;            // "engine"
        virtual const char* title() const = 0;         // "ENGINE"
        virtual void place(Rect stage) = 0;            // the frame gives the stage; lay out for it
        virtual void tick(float dt) = 0;
        virtual void draw(funkgui::Canvas&, const funkgui::Theme&) = 0;   // changes nothing
        virtual bool moving() const = 0;               // wants full rate
        virtual const char* inspectorPage() const = 0; // the `.gui` text for what is selected (may use `auto`)
        // pointer, wheel, key and accessibility calls as J2's Board forwards them
    };
}
```

A workspace reads fields by path and telemetry by name, so it compiles and runs before the physics that feeds it
has landed: a name that is not there yet reads as absent and draws a dash. Each has a demo source for tests and
pictures (a made-up engine turning over), so every workspace has goldens and a contact sheet from the day it
exists.

## 6. The packages

| | Package | Owns | Depends on |
|---|---|---|---|
| U1 | The frame: top bar, rail, stage and inspector with selection, the dyno ribbon, the cockpit; `Workspace`, `ScopeSource`, `DynoSource`; the look as tokens and helpers every workspace draws with (`ui/app/Look.h`: colours by meaning, the gauge, plot and diagram styles); DRIVE's instruments as the first workspace, to prove the pattern; the application switched from `RefPanel` to the frame (with card B) | `ui/app/**`, `ui/views/drive/**` | J2's kit; card B's shell; card C's `Build`, `SimHost`, dyno runner |
| U2 | ENGINE and VALVES | `ui/views/engine/**`, `ui/views/valves/**` | U1's interface; D's and E's Spec fields and Scope arrays (their standalone specs name them) |
| U3 | AIR and EXHAUST | `ui/views/air/**`, `ui/views/exhaust/**` | U1; F's and G's fields and Scope arrays |
| U4 | TUNE, CAR and DYNO | `ui/views/tune/**`, `ui/views/car/**`, `ui/views/dyno/**` | U1; H's and I's fields; C's runner |
| U5 | GARAGE and SCOPE; the atmosphere shaders (glow, flame, smoke), last | `ui/views/garage/**`, `ui/views/scope/**`, `shaders/fx/**` | U1; everything, lightly |

U1 starts now and pushes the `Workspace` interface and the look first; U2 to U5 start when that is on the branch.
Each package's tests are J's kind: scripted input through `HeadlessHost`, fingerprints, PNGs at density 1 and 2,
a contact sheet; and once, when it can, a run on the GPU path under Xvfb.

## 7. What a person will be asked

At the next checkpoint: the contact sheet of every workspace, and the application on a Mac. The questions will be
of taste as much as of fault: is this the workshop you had in mind; what is missing from a stage; which workspace
you would open first.
