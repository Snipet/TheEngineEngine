# The interface kit (`ui/`)

*Card J, slices J1 and J2: the `.gui` panel code that came from TestingPlayground, what was cut from it and what stands
in its place, how a panel is bound to an engine, how its shaders are built, and how it is tested. The language
itself is in [gui-language.md](gui-language.md).*

## What is where

| Path | What |
|---|---|
| `ui/kit/gui/` | The `.gui` reader (`GuiSpec`, `PanelSpec`), its layout (`GuiLayout`, `DialLayout`), its controls and looks (`GuiItems`, `Looks`, `Items`, `DisplayItem`, `Dropdown.h`, `Style.h`, `Scroll`, `TabStrip`), its view (`GuiView`), and the Panel around the view (`GuiPanel`, `Ports`). Namespace `tee::ui`. |
| `ui/kit/shaders/` | The text side of a panel's shaders (`ShaderText`), the built-in shader library and its font (`ShaderLib`, `ShaderFont`), the registry of what the build compiled (`Registry`), the build's tool (`shadergen.cpp`), and the stand-in for FunkGui's shader registry in a build with no GPU layer (`ShaderLibraryNone.cpp`). |
| `ui/kit/SEED.tsv` | Every file taken from TestingPlayground: our path, its path there, the commit (`47bac40327ed…`), the sha256 of the file as it was. `ui/kit/tools/import-seed.py` made the copies and the table. |
| `ui/ref/` | `RefPanel`: the reference engine's own panel (`sim/ref/panel.gui`, `header.sc`, `cluster.sc`, `layout.sc`) with the strip that stands where its keyboard was; `makeMainPanel`. |
| `cmake/TeeShaders.cmake` | `tee_add_panel_shaders`: a panel's shaders built when the application is built. |
| `tests/ui/` | `tee-ui-tests`: 29 headless tests, their goldens (`golden/*.txt`) and the test panel (`panels/kinds.gui`). |

`shaders/ref/` (named in the card) is not used: the reference's shader sources stay in `sim/ref/`, which is
never edited, and what is built from them lives in the build tree.

## Building

- **Alone**: `cmake -S ui -B build-ui -G Ninja && cmake --build build-ui && ctest --test-dir build-ui`. It
  needs a C++20 compiler, CMake 3.30 and git (it fetches FunkGui at its pin, JUCE off, no GPU;
  `-DTEE_FUNKGUI_DIR=<checkout>` uses one that is already there). No LLVM, no SDL, no display.
- **In the root**: the root adds `ui/` after `tests/`, whether or not the application is built; `ctest -R '^ui\\.'`
  runs the same tests.
- Targets: `tee_ui_kit`, `tee_ui_ref` (static libraries), `tee_ui` (both: what the application links for
  `makeMainPanel`), `tee-shadergen`, `tee-ui-tests`. FunkGui's core is an INTERFACE library that carries its
  sources, so our libraries take only its headers; whatever is finally linked links FunkGui once.

## From TestingPlayground: what was kept, what was cut

Kept as they were, apart from the namespace (`playground` and `playground::ui` are both `tee::ui`), the include
paths and a first line naming the origin: `GuiLayout.*`, `GuiItems.*`, `Looks.*`, `DialLayout.*`, `Scroll.*`,
`TabStrip.*`, `GuiView.*`, `Dropdown.h`, `Items.cpp`, `ShaderLib.*`, `ShaderFont.*`, `ShaderLibraryNone.cpp`.
None of them names JUCE. Since then `GuiLayout.*` has J2's widget elements (`guiWidgetSize`): one without `width=`
takes at most the width its container offers, so that a generated page never scrolls sideways in the inspector's
column, and an `array` narrower than its 640 px is as tall as the lines its cells wrap onto (`guiTableLines`).

| File here | From | What was cut | What stands in |
|---|---|---|---|
| `gui/Controls.h` | `dsp/Controls.h` | The host's 32 parameters against the panel's own 32 (`kHostControls`, `isPanelDial`, slot masks); the carry of a dial's value from one compiled program to the next (`DialCarry`, `carryValue`, `carryWhy`, `clampedPlain`, `optionAt`, `optionValue`, `sameName`). | A `Control` is made from a `tee::ParamInfo` (`controlOf`, `gui/Ports.h`). `kMaxControls` is 64, a plain limit. |
| `gui/Sources.h` | `dsp/Sources.h` | The editor's tabs: file-name rules, kinds of file, the folder sync's manifest, circuit pins, fresh names, `tidyFiles`. | `SourceFile`, `Sources`, `findFile`: a panel's files by name. |
| `gui/Meters.h` | `dsp/Meters.h` | Everything the plugin's audio engine published besides what a display reads: probes, voices, held keys, the host's transport, the shift lights, which program and tap table a frame is of. | A `Meters` of the output's levels and the displays' values, filled each frame by `GuiPanel` from a `tee::TelemetrySource`. |
| `gui/Style.h` | `editor/ChatStyle.h` | The chat column's look: bubbles, series inks, the git mark, thirty of its icons. | The palette, the text styles and eleven line icons (a menu control's chevron among them). Namespace `tee::ui::style` (was `chat`). |
| `gui/Items.h` | `editor/Items.h` | The code editor's keyword rules for FMinus, `.gui` and netlists; the kinds of file. | Nothing: no editor here. |
| `gui/PanelSpec.*` | `fminus/PanelSpec.*` | `readPanelSpec`, the reader of `#pragma panel` lines in a program's source, and its helpers. | Nothing: a panel is a `.gui` file. The types, `displaySource`, `readFieldAttribute`, `cutText` stay. |
| `gui/GuiSpec.cpp` | `fminus/GuiSpec.cpp` | The rule that `hover=` needs a parameter no host sees. | Any parameter. |
| `gui/DisplayItem.*` | `editor/DisplayItem.*` | `TapsShown` (holding a display's values across a program change). | Nothing: one source, one panel. |
| `shaders/ShaderText.*` | `fminus/Shaders.*` | Running shaderc while the plugin plays: the process spawn, the time-out, the cache in memory and on disk, the renderer's check, the citing of shaderc's errors to a line. | `shaderFiles()`: what shaderc is handed, as text. shaderc runs in the build (below). |
| `gui/GuiPanel.*` | `editor/PanelView.*` | The sandbox's mock ports and demo meters; a fixed size; no input. | See the next section. Most of this file is new. |

Not copied at all: the plugin's `Panel.cpp` (its editor, chat and cards), `EditorPort`, `Patch`, the language
service, MIDI learn, the GUI sandbox. `GuiView`'s hooks for MIDI (`setMidi`, `dialMenu`) are still there and
nothing sets them.

`DialLayout.*` (the layout of plain rows of dials and of `#pragma panel` sections) is compiled and nothing
draws from it yet; `DisplayItem.h` uses its `ItemPlace`. `TabStrip.*` likewise is compiled and unused.

## A panel bound to an engine

`tee::ui::GuiPanel` (`ui/kit/gui/GuiPanel.h`) is a `funkgui::Panel` over a `GuiView`:

```cpp
tee::ui::Sources files = { {"panel.gui", text}, {"cluster.sc", text}, ... };
tee::ui::GuiPanel panel(files, paramSource, telemetrySource, options);
```

- **Parameters.** Parameter `i` of the `tee::ParamSource` is slot `i`. Each slot has a `SourcePort`
  (`gui/Ports.h`), FunkGui's `ParamPort` over that parameter: a control turns it 0..1 and the port calls
  `ParamSource::set` in the parameter's own units, by the range and taper the panel ended with. Every gesture is
  bracketed (`beginGesture`, `endGesture`) through FunkGui's `GestureController`; card J2's `FieldPort` is
  another port of the same kind.
- **Telemetry.** Each tick the panel reads `TelemetrySource::value` for every field a display names, and
  `outputPeak`, into a `Meters`; the displays, `live=` marks and shader inputs read that.
- **Input** is the panel's own, modelled on what the plugin's Panel did for its view: the item under the pointer
  takes a press and its drag; a click gives a control the focus, Tab and Shift-Tab move it (with a ring), the
  focused item takes keys; the wheel turns the control under it, else scrolls an interface larger than its
  window; scroll bars drag; a menu control opens the host's menu (`HostServices::showMenu`) and writes the
  choice as one gesture; accessibility lists every item shown and takes its actions.
- **Size.** It opens at the interface's natural size (or `Options::width`, `height`), is resizable down to
  `Options::minWidth` by `minHeight`, lays out again on `resize`, and scrolls when smaller than natural.
- **A subclass** may add items of its own (`addItem`), keep room under the interface (`setBottomReserve`) and
  place its items after every layout (`placed()`): `RefPanel` does.
- As every FunkGui Panel: time comes only from `tick(dt)`, and `draw` changes nothing. `needsDraw()` is
  FunkGui's default (true): a still panel is drawn again each frame at the host's calm rate; mixing the items'
  `drawKey()`s to skip unchanged frames is left for J2.

`RefPanel` (`ui/ref/RefPanel.h`) is a `GuiPanel` over the reference's files, embedded when the build is made
(`ui/ref/CMakeLists.txt` writes them into a source file, byte for byte; a test compares them with `sim/ref/`).
Its strip (`ui/ref/Strip.h`) sits in the cell of the file's `keyboard`: START (held), a throttle bar (drag; the
wheel sets a rest position), the gear between `-` and `+`. The strip never drives the engine: it reports a
`tee::PanelControls` through `AppServices::panelControls`, shows the pedal the application last sent
(`AppServices::controls`) and the gear the telemetry gives (`car.gear`), lights START while `engine.state` is
cranking or running, and shows the first line of `AppServices::status` across the top when it starts with
`error:`.

The reference's shaders read the reference's own `Shared` fields, raw: `showRpm`, `showRed`, `showGear`,
`showKmh`, `showThr`, `showMap`, `showLamps`, `showPedals`, `showCycle`, `showAir`, `showPipes`, `showBoost`.
The Player must answer `findValue` for those names; one it does not answer reads 0 and is a note.

## Shaders, built with the application

`tee_add_panel_shaders(<target> GUI <file.gui> SHADERS <a.sc> ... [HEADERS <x.sh> ...] [NAME <panel>])`
(`cmake/TeeShaders.cmake`):

1. `tee-shadergen` writes, for each shader, the fragment source bgfx's shaderc is handed, as
   TestingPlayground's `shaderSource` made it: FunkGui's prelude (`shaders/fs_custom_prelude.sc`), then
   `u_time`, `u_size`, `u_px`, `in0` to `in15`, and `in_<name>` for each name in the element's `inputs=`, then
   the file with its string literals turned into glyph codes; and beside it the files it includes
   (`funk/text.sh`, `funk/shapes.sh`, the panel's own `.sh` files).
2. FunkGui's shaderc compiles it for Metal and SPIR-V on every host and for Direct3D 11 on a Windows host, with
   FunkGui's varyings, embedded with `--bin2c` (the rules of FunkGui's `cmake/FunkGuiTargets.cmake`).
3. The target gets `tee::ui::shaders::<panel>Shaders()`, a table of file names and the platform's binaries.
   The panel's owner adds the table (`shaders::add`); `GuiPanel` finds each shader by the panel's name and the
   file's, and registers it with FunkGui under its bytes' hash, with the text library's font bound
   (`ShaderFont`). `tee::ui::shaders::registerAll(why)` registers every table at once, for an application that
   wants no compile at its first frame.

Shaders are built when a GPU layer of FunkGui's is in the build (`FunkGui::desktop` or `FunkGui::gpu`), or when
`-DTEE_UI_BUILD_SHADERS=ON -DTEE_SHADERC=<shaderc> -DTEE_BGFX_SRC=<bgfx.cmake>` is given, which proves they
compile where there is no GPU layer. Otherwise the tables are empty and `ShaderLibraryNone.cpp` answers for
FunkGui's registry: a shader element draws a placeholder that says so.

## Tests

`tee-ui-tests <name>` (each a CTest test `ui.<name>`); `--list` names them, `all` runs them.

- A made-up `tee::Player` (`tests/ui/MockPlayer.h`) and `tests/ui/panels/kinds.gui`, which has every element in
  every look. Input is scripted through FunkGui's `HeadlessHost`: a drag turns a knob and the parameter moves; a
  menu picks; tabs switch; a resize lays out again; and so on, a test each.
- `ui.ref.panel` is the reference's own panel over a Player with the reference's 32 dials (read from
  `sim/ref/program.fm`): no notes, every dial on its page turned and put back, the menus, buttons and switch,
  the three shaders fed, the strip in the keyboard's cell, at its natural size (840 by 764), at 640 by 400
  (it scrolls) and at 1920 by 1200.
- **Goldens** are frame fingerprints (FunkGui's `canvas/Fingerprint.h`) as text under `tests/ui/golden/`. They
  are written only by `tee-ui-tests <name> --bless`, after looking at the PNG the test wrote under `out/ui/`.
- **Pictures** are drawn by FunkGui's software rasteriser and written by a small encoder of ours
  (`tests/ui/Png.h`: FunkGui's own is JUCE's, so its JUCE-free build writes none). They are uncompressed, a few
  megabytes each, and never committed. The rasteriser draws no shader: in every picture a shader is its
  placeholder.
- `ui.seed` holds `SEED.tsv` to the files: every row is a file whose first line names its origin, and every
  file that names one has its row.

## What is not here yet

- The shaders seen: nothing in this slice ran the GPU path. The reference's three shaders were compiled by
  shaderc where that was possible (the handoff says where) and never drawn. The 120-frame run of `RefPanel` on
  the GPU under Xvfb waits for FunkGui's desktop host (card A).
- The output's samples for a scope or a spectrum (`TelemetrySource` has none).
- Skipping unchanged frames (`needsDraw`).
- Spec paths as names, generated pages, the new widgets: J2 (below).

# J2: widgets, units, the binding, the frame

*Card J, slice J2 (items 6 to 10). Everything drawn here is drawn with the Canvas alone, so the software rasteriser
pictures it; none of it reads a clock or the environment, and every motion goes through `funkgui::ease`.*

| Path | What |
|---|---|
| `ui/kit/widgets/Widget.h` | `Widget`: an `Item` (J1's) its owner places (`setPlace`), with a hover eased, a focus ring and the theme's colours (`Look`: dark, one accent). |
| `ui/kit/widgets/Board.*` | `Board`: a `funkgui::Panel` of items its owner places, with an optional inner Panel under them (hover, press and drag, focus and Tab, keys, wheel, accessibility, one `GestureController`). |
| `ui/kit/widgets/Basic.*` | `Button` (press, hold, lit), `Toggle`, `Stepper`, `TextField` (over `LineEdit`), `List`, `Bar`, `Lamp`, `Label`. |
| `ui/kit/widgets/Gauge.*` | `Gauge`: a 270-degree arc, ticks, numerals, a red band, a needle eased, the value in digits. |
| `ui/kit/widgets/Plot.*` | `Plot`: linear and log axes with round ticks, a grid, series as polylines and area strips, a cursor with a readout, pan (drag), zoom (wheel, pinch), double-click back. |
| `ui/kit/widgets/Tables.*` | `Curve16`, `Map16` as the editors hold them; `CurveEdit` (a `Plot` with points to drag), `MapEdit` (table and heat map, typed or dragged cells, typed axes, the live cell). Each writes through a `CurveSource` or `MapSource`, an edit one gesture. |
| `ui/kit/bind/Units.*` | `Units`: a quantity's SI value to the chosen unit's text and back (offsets too: C, F), the choices as settings lines. |
| `ui/kit/bind/Fields.*` | `FieldInfo`, `FieldDoc` (what the binding needs of a Spec and its document), `MemoryDoc` (one in memory, undo by gesture). |
| `ui/kit/bind/Binding.*` | `FieldPort` (a `ParamPort` over a field), `FieldParams` (a page's fields as a `ParamSource` in the user's units, and a `GestureSink`), `pageGui` (a generated page), `fieldWidget` (the `.gui` widget elements), `FieldPage` (a `GuiPanel` bound to a document). |
| `ui/kit/bind/host/BuildDoc.*` | `BuildDoc`: card C's `tee::Build` as a `FieldDoc`; `buildServices`: the frame's garage, undo and name over a `Build` and `tee::Presets`. Target `tee_ui_build`, in the root's build only. |
| `ui/app/AppFrame.*` | `AppFrame`: the application's frame of screens. Target `tee_ui_app`. |

## The binding

A Spec field reaches a control in one of two ways, both bracketing every gesture in the document (one undo step a
drag, a click, a typed value):

- **A `.gui` page** (`FieldPage`): its `FieldParams` offers the fields the file names (and every field of its `auto`
  sections) as a `tee::ParamSource` whose values are in the user's units: J1's `GuiPanel` binds to it unchanged, a
  control naming its field by path. A temperature in C has an offset, which a panel's `scale` cannot say, so the
  values are converted, not scaled. `FieldParams` is also a `GestureSink` (`ui/kit/gui/Ports.h`), so J1's
  `SourcePort` brackets each gesture in the document. A change of units makes the pages again.
- **A widget placed by C++**: `FieldPort`, a `funkgui::ParamPort` over the field (0..1 by its range and taper, steps
  for a choice, an integer and a switch, `text()` through `Units`).

A generated page is `.gui` text (`pageGui`), so it lays out, draws and is tested as any hand-written page; a section
with more than 60 controls is cut into pages (`splitPages`, a group kept whole where it fits), since one panel shows
64 parameters at most (J1's `kMaxControls`).

**Arrays.** An array of numbers in the Spec (`float pin[MAX_CYL]`) is a field an element: `cyl.pin[2]`, labelled from
the array's label and its index counted from 1 ("Crank pin 3"), with the array's range, unit and choices
(`FieldInfo::array`, `arrayLabel`, `element`, `extent`). A `.gui` page names an element as `knob engine.pin[2]`; an
index past the array is a note that gives its extent ("engine.pin[20] is past the array: it has 16 elements,
engine.pin[0] to engine.pin[15]"). A generated page shows an array of up to 8 elements a control each, in its group's
row (`kArrayKnobs`); of 9 to 16 elements one row of cells, the `array PATH` element (a `MapEdit` of one row: a cell
typed, a choice by its name or index, or dragged, one undo step each; the header of element numbers is not
editable; in a column too narrow for the row, the inspector's, the cells wrap onto lines of as many as keep 36 px
each, the element numbers over each line, `MapEdit::lines`; a value longer than its cell, a choice's name, is cut
with an ellipsis); of more than 16 a control each again (and `splitPages` counts them). An array of arrays is a field an
element too, `engine.gain[1][2]`, labelled "Gain 2.3" (`FieldInfo::shape`), a control each on a generated page; an
element outside it is a note that gives its shape ("cyl.gain[2][0] is not an element: cyl.gain has 2 x 3 elements"). The Spec's per-cylinder arrays are
`MAX_CYL` (16) long, so they come as rows of cells.

**Until and after card C.** `FieldDoc` is the small interface the binding needs (fields with their attributes,
values in SI by index, whole tables, gestures, a revision). `MemoryDoc` implements it for `ui/` alone and its tests,
with a made-up Spec of every field kind (`tests/ui/MockSpec.h`); `BuildDoc` implements it over the real `tee::Build`
(an array of numbers is a field an element; a map's z is transposed from the Build's z[i][j] at x[i], y[j]; a table's
axis ranges, which the Spec does not give, are the quantity's usual span).

## The application's frame

`AppFrame` (`ui/app/AppFrame.h`) is a `Board`: a rail of the nine screens at the left (Build, Valvetrain, Boost,
Exhaust, Tune, Dyno, Drive, Scope, Garage), a top bar (the build's name and the status line, the engine's state,
the output's level, undo, redo, START held), a strip of the screen's pages when it has more than one, and a bottom
strip (the pedals as bars, the gear, the speed, the engine's speed). A screen is a list of Spec sections (a generated
page each) or a `.gui` text (Dyno, Drive and Scope: gauges, bars, a plot and `auto` sections until wave 2 gives them
their views). A section no screen names goes to Build, so every field that is not hidden is on a page; the tests hold
that for the made-up Spec (`ui.app.frame`) and for the real one (`ui.build.frame`). The Garage lists the builds
(shipped and the user's), loads one, renames this one, saves it, and chooses the units. Cmd (or Ctrl) Z undoes,
with Shift redoes.

The frame needs a document, the units, the telemetry and `FrameServices` (`buildServices` makes them over a
`Build`). The shell does not show it yet: `makeMainPanel` is still J1's `RefPanel` over the reference's dials, and
the frame replaces it once the shell plays the new simulator with a `Build` (the handoff says what the shell does).

## Tests and pictures

`tee-ui-tests` holds J2's tests beside J1's (`WidgetTests.inc`, `BindTests.inc`, `AppTests.inc`); the root's build
adds `tee-ui-build-tests` (`BuildTests.cpp`: the real `sim/`). Goldens: `widgets.controls`, `widgets.instruments`,
`widgets.tables` (the gallery) and `app.<screen>` (each screen at 1280 x 800). `ui.app.sheet` writes the contact
sheets, `out/ui/sheet.png` (pixel density 2) and `out/ui/sheet.dpi1.png` (density 1, as many Windows displays at
100 %): every screen at 1024 x 640 and at 1600 x 1000, three screens a row; and each screen at 1280 x 800 and density
1 (`app.<screen>.dpi1.png`), as the gallery's pages are (`widgets.<page>.dpi1.png`). J2's text is at FunkGui's 10 px
floor or above in logical px (its smallest styles are 10 px; a gauge's numerals are clamped to 10 to 16). Every
picture is drawn by FunkGui's software rasteriser.

**The same picture on every platform.** The goldens are fingerprints of the geometry, so a float computed
differently changes them. FunkGui's sources are compiled inside whatever links `FunkGui::core` (an INTERFACE
library), with that target's flags: the tests' copy (`tee_ui_test_funkgui`) is built with `-ffp-contract=off`, as
our own code is, because Apple's clang on arm64 otherwise fuses multiply-adds in FunkGui's geometry (its default
is `-ffp-contract=on`, and arm64 has FMA) and the six J1 goldens failed there alone (run 45390597717).

# U1: the frame and the workspaces

*Package U1 (docs/plan/ui-design.md). How a workspace is written, registered, tested and pictured:
[handoffs/u1-interface.md](handoffs/u1-interface.md).*

| Path | What |
|---|---|
| `ui/app/Workspace.h` | `Workspace` (a `Board` keeping to its stage), `WorkspaceContext`, `Selection`, `ScopeSource`, `DynoSource`, `Live`, `FrameServices`, `registeredWorkspaces()`. |
| `ui/app/Look.h` | `look::Palette` (colours by meaning for graphite and paper), the type and line styles, `region`, `readout`, `level`, `pipe`, `selectRing`, `lamp`, `placeholder`, `num`. |
| `ui/app/AppFrame.*` | The window: top bar, rail, stage, inspector (selection, tabs, units page), dyno ribbon, cockpit; `needsDraw`. J2's screen names kept for the shell's `--workspace`. |
| `ui/app/Panes.*` | The stage's workspace and the inspector's page side by side as one inner Panel (input routed by place, ids tagged). |
| `ui/app/Cockpit.*` | `RpmBar`, `Readout`, `PedalBar`, `DynoRibbon`. |
| `ui/app/Placeholders.*` | A workspace's stand-in until its package lands; the garage's (J2's list, load, rename, save); `NoDyno`, `NoScope`. |
| `ui/app/Demo.*` | `demoFields()`, `DemoEngine` (telemetry, Scope, dyno; `set`, `forget`, `hold`), `DemoRig`. |
| `ui/app/host/HostSources.*` | `ServiceDyno` (over `tee::DynoService`), `SimScope` (over `SimHost::scope`): in `tee_ui_build`, the root's build only. |
| `ui/views/<id>/` | A workspace each; `register.h` puts it on the rail (generated `WorkspaceRegistry.cpp`). `drive/` is U1's. |
| `tests/ui/views/` | `tee-view-tests`: `ViewKit.h`, the frame's tests (`frame.*`), DRIVE's (`drive.*`), each package's own file. |

**The inspector.** With nothing selected (or a selection with no fields), a workspace's `sections()` (and, on ENGINE,
every section no workspace names): a section's scalar fields on a generated page (cut at 60 controls), each table on a
page of its own. A selection shows the fields under its path. A workspace's `inspectorPage()` replaces both. A
workspace with no section and no page shows the units. The pages are J2's `FieldPage`s, laid into the inspector's room
by `GuiPanel::setInsets` (added here).

**Drawing only when something changed.** `AppFrame::needsDraw()` mixes every shown item's `drawKey()` (the frame's,
the workspace's, the inspector page's), the workspace's own `drawKey()`, the document's, the selection's, the units'
and the dyno's revisions, and is true after any input or while anything eases (`ui.frame.idle`). It runs at full rate
while the engine cranks or runs (live values), while the dyno works, or while the workspace says `moving()`.

### Hover help (U1, the owner's ask of 25 Nov)

`kit/gui/Tooltip.h`: after a short rest of the pointer (0.55 s) or of the keyboard's focus reached by Tab (0.35 s), a
box beside the thing it describes (below it, else above, else to a side; never over it, never past the panel's edges)
with its title, its sentence of help and a line of facts. It is laid out once when it appears; `draw()` allocates
nothing. A press hides it, and it stays away until the pointer has left what was pressed.

- `GuiPanel`: every bound control. `Options::help(param)` gives the sentence (`FieldPage` fills it from the field's
  `ui::help`); the facts are the parameter's range and default with its unit, or its choices. `GuiPanel::tooltip()`.
- `Board`: any item whose `Item::help(title, body, anchor)` says something; a `Widget` says what `setHelp("...")`
  gave it. `Board::tooltip()`.
- An owner that draws only when its key moves mixes `tooltip().drawKey()` into it (the frame does, for itself, its
  inspector page and the workspace shown).
- `ui.build` checks that every Spec field a person sets has its `ui::help`; `ui.frame.help` hovers a dial, START and a
  control reached by Tab.

# Inspector designs (J2, 25 Nov)

*The owner, 25 Nov: "Are there any plans for redesigning the right-side controls on the views? I feel like they are
relatively clunky still." Three designs, built and pictured; none is wired into the frame: the shipped inspector is
J1's ported knobs on FieldPages until the owner picks one.*

`ui/kit/inspect/`: `Outline` reads any inspector page the frame makes (a workspace's own `.gui` or a generated page)
back into its headings, fields, tables and notes, in order, so every design keeps each workspace's arrangement with
no change to the workspace. `FieldInspector` draws an outline in one of three styles, with the same behaviour:

- **A value is typed.** A click on it types it; Enter keeps it when it is within the field's range (in the user's
  unit, or any unit of its quantity named: "86", "3.39 in", "1 bar"; a choice by its name or number; an integer only
  whole) and otherwise refuses it, saying the range ("Out of range: 0.05 to 30.00 L"), never clamping. A drag sideways
  scrubs it (240 px the travel, Shift ten times finer), the wheel nudges it (a hundredth, Shift a thousandth; a burst
  one undo step), a double-click is its default. Each edit is one undo step.
- **Keyboard**: one Tab stop; Up and Down between fields, a digit or Enter types, Left and Right nudge, Space flips a
  switch, Delete is the default, / is the filter, Escape leaves typing or clears the filter.
- **Hover help** (U1's tooltips): each row's own (`Item::helpPart`, added to the kit for this), its `ui::help` and
  its range and default.

| Design | Style | What it is |
|---|---|---|
| 1, Rows | `Style::rows` | A field a row: label, typed value with its unit, a thin bar of where it sits in its range with a tick at its default; a mark on each field off its default; groups fold; a filter box finds across the workspace; CHANGED shows only what moved. |
| 2, Dials made exact | `Style::dials` | A dial for each number (larger, 64 px), its value typed under it; choices and switches as cells; a steady grid of two columns; headings stay at the top while their group scrolls under them. |
| 3, On the picture | `Style::list` + `setCard` | A part pressed on the stage brings a card beside it with the first six fields of its page; the column is the compact, searchable list of everything in the workspace. |

**Pictures**: `tee-ui-build-tests build.inspect.pictures` (root build; the real Spec in the real frame, the design
laid over the frame's column by a test-only Panel) writes `out/ui/inspect/`: `<design>.<page>.<small|large>.png`
for ENGINE 1, TUNE's FUEL, EXHAUST 1 (and `.scrolled`), TUNE's ECU 1 (the choices and switches), at windows of
1100 x 700 and 1600 x 1000; `rows.engine.*.refused`, `rows.filter.*`, `rows.changed.*`; `picture.<workspace>.*`
(ENGINE, EXHAUST, TUNE, with a part chosen). Drawn by FunkGui's software rasteriser on Linux. Behaviour:
`ui.inspect.type`, `.scrub`, `.filter`, `.keys`, `.dials` (tee-ui-tests, the made-up Spec).

**What each costs across the eleven workspaces**
- *Rows.* The widget is built and reads every workspace's page as it is, so the workspaces change nothing. The frame
  needs one hook (U1's): show a `FieldInspector` over the page's outline in place of the `FieldPage`, and send a
  table's row to the table's own page as now. What a workspace's `.gui` page does beyond fields is reduced to
  headings and notes (no workspace's inspector page carries a plot or a gauge today). Keyboard use gains most: one
  stop, arrows, typing. Hover help is per row.
- *Dials.* The same plumbing. Long pages grow: EXHAUST 1's 43 numbers are about 22 rows of 136 px. Keyboard and help
  as Rows; a dial can still be dragged, so muscle memory from the synthesiser look is kept.
- *On the picture.* The column is design 1's list (free once Rows is in). The card anchors at the press, which works
  today with no workspace change; a part chosen from the list or by the keyboard has no point to stand beside, so
  each workspace would add `partRect(path)` (about twenty lines each, eleven workspaces, their packages'). Its fields
  are the first six of the selection's page, so a workspace chooses them by ordering its page. The card is a second
  Tab stop. Help as Rows.
