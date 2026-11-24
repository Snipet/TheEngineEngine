# The interface kit (`ui/`)

*Card J, slice J1: the `.gui` panel code that came from TestingPlayground, what was cut from it and what stands
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
None of them names JUCE.

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
- Spec paths as names, generated pages, the new widgets: J2.
