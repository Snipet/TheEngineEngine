# Card J: the interface kit

*Wave 1, started early. Two slices that two agents can take: **J1** (the panel code from TestingPlayground, the
shaders at build time, the reference's panel in our window) and **J2** (the new widgets, the binding to the
Schema, the application's frame of screens). Repository: TheEngineEngine, integration branch
`claude/awesome-gates-ntdb58` (fetch and merge before every push). Read [01-rules.md](01-rules.md), then
[02-contracts.md](02-contracts.md) §3, §5, §7, then card B's interfaces ([w1-b-shell.md](w1-b-shell.md): `Player`,
`ParamSource`, `TelemetrySource`, `makeMainPanel`).*

**OWNS**: `ui/**`, `shaders/**`, `tests/ui/**`, `cmake/TeeShaders.cmake`, `docs/gui-language.md`,
`docs/ui-kit.md`; in the root `CMakeLists.txt` only the lines that add `ui/`. J1 owns `ui/kit/gui/`,
`ui/kit/shaders/`, `ui/ref/`, `shaders/ref/`; J2 owns `ui/kit/widgets/`, `ui/kit/bind/`, `ui/app/`.

## What FunkGui gives, and what it does not (00-abstract-plan.md §2.2)

A Canvas of rounded rectangles, capsule segments, polylines, stroked arcs and cubics, discs, area strips, text in
one monospaced face (ASCII and 19 symbols, 10 px at the least), images, clip rectangles, and a rectangle filled
by a custom fragment shader (16 floats and 4 textures a draw). No rotation, no gradients, no filled polygons, no
translucent strokes (blend the colour yourself: `premix`). Widgets: `Dial`, `RuleSlider`, `SegmentedSelector`,
`LatchToggle`, `TextEditor`, `LineEdit`'s state. No button, dropdown, list, table, tooltip or layout. A Panel
takes time only from `tick(dt)`, never reads a clock or the environment, and `draw` changes nothing: keep that,
because it is why a headless frame equals the live one.

## J1: what exists, moved here

1. **Copy, with provenance**, from TestingPlayground at `47bac40` (`/home/user/snipet/testingplayground`, read
   only) into `ui/kit/gui/`: the `.gui` language (`Source/fminus/GuiSpec.*`, and what it needs of
   `PanelSpec.*`), its layout (`Source/editor/GuiLayout.*`), its controls and looks (`GuiItems.*`, `Looks.*`,
   `Items.*`, `DialLayout.*`, `DisplayItem.*`, `Dropdown.h`, `Scroll.*`, `TabStrip.*`), its view (`GuiView.*`).
   Namespace `tee::ui`. Every copied file gets a row in `ui/kit/SEED.tsv` (our path, its path there, the commit,
   the sha256 of the file as it was) and a first-line comment naming its origin. None of these files names JUCE
   (checked); they do name the plugin's own types (`EditorPort`, `Patch`, the meters, the language service): cut
   each such tie and put a small interface of ours in its place, never a copy of the plugin's machinery. What a
   control binds to is card B's `ParamSource`; what a display or a shader input reads is `TelemetrySource`.
   Record in `docs/ui-kit.md` what was cut and what stands in for it.
2. **`docs/gui-language.md`**: the language as we have it (start from `GuiSpec.h`'s header comment), and our
   differences: a control names a parameter by its id (the reference's dial names today; a Spec path such as
   `boost.level` after K3); `keyboard` is not drawn (no MIDI).
3. **Shaders at build time** (`cmake/TeeShaders.cmake`, `ui/kit/shaders/`): TestingPlayground builds a panel's
   `.sc` files with bgfx's shaderc while the plugin runs (`Source/fminus/Shaders.*`, `ShaderLib.*`,
   `Source/editor/ShaderFont.*`). We build them when the application is built: `tee_add_panel_shaders(target
   GUI <file.gui> SHADERS <a.sc> ...)` makes each fragment source as TestingPlayground's `shaderSource` does
   (FunkGui's prelude from `funkgui::shaders::fragmentPrelude()`'s source file, the names `u_time`, `u_size`,
   `u_px`, `in0` to `in15` and `in_<name>` from the element's `inputs=`, string literals turned into glyph codes,
   `#include "funk/text.sh"` and `funk/shapes.sh` resolved), runs FunkGui's shaderc for Metal, SPIR-V and (on a
   Windows host) Direct3D 11 as FunkGui's own rules do (`cmake/FunkGuiTargets.cmake` lines 478 to 544), and embeds
   the binaries with a registry (`tee::ui::shaders::registerAll(std::string& why)`, which calls
   `funkgui::shaders::set`). The text library's font texture as `ShaderFont` makes it. With no GPU layer in the
   build (the headless one), shaders are not built and the rasteriser draws its placeholder.
4. **The reference's panel** (`ui/ref/`): `RefPanel`, a `funkgui::Panel` that is a `GuiView` over
   `sim/ref/panel.gui` with `sim/ref/{header,cluster,layout}.sc`, bound to a `tee::Player`: knobs, menus and
   switches to the parameters by name; `shared->showRpm` and its kind to telemetry named `showRpm`; `out` to the
   output's peak. Where the reference had its keyboard, a strip of ours: a start button, a throttle strip to drag,
   the gear. `tee::ui::makeMainPanel` returns it. This is the picture checkpoint M1 hopes for; card B has a plain
   placeholder if it is late, so correctness comes before the deadline.
5. **Tests** (`tests/ui/`): every control kind in every look through `HeadlessHost` with scripted input (a drag
   turns a knob and the parameter moves; a menu picks; tabs switch; a resize lays out again), fingerprints as text
   goldens under `tests/ui/golden/` (written only by an explicit `--bless` of yours, reviewed from the PNGs in
   `out/`), PNGs from the software rasteriser through an encoder of your own until FunkGui's JUCE-free one lands
   (card A, stage 2). When card A's host is pinned: `RefPanel` for 120 frames on the GPU path under
   `scripts/xvfb-run.sh`, its picture kept, so the shaders are seen at least once outside a Mac.

**J1 is done when** the reference's whole panel lays out and works headless at its natural size and at a small
and a large one, every copied file is in `SEED.tsv`, the shaders build for all three renderers in CI, and the
handoff shows the pictures.

## J2: what is new

6. **Widgets on the Canvas** (`ui/kit/widgets/`), each a small class in FunkGui's manner (owned by a Panel, given
   `tick`, `draw`, pointer, wheel, key and accessibility calls; state eased, never stepped), each with a gallery
   page and tests: `Button`, `Toggle`, `Stepper`, `TextField` (over FunkGui's `LineEdit`), `List`; **`Plot`**
   (linear and logarithmic axes with ticks and labels, a grid, several series as polylines and area strips, a
   cursor with a readout, pan and zoom); **`Gauge`** (an arc, ticks, numerals, a needle as a segment, a redline
   band, a digital value); `Bar` and `Lamp`; **`MapEdit`** (a `Map16` as a table and a heat map, cells edited by
   typing or dragging, the live cell lit, axes edited); **`CurveEdit`** (a `Curve16` as points to drag on a
   `Plot`). The valvetrain's cam editor, the compressor map and the scope are wave 2's views, built from `Plot`.
7. **Units** (`ui/kit/bind/Units.*`): a quantity (02-contracts.md §3) and a value in SI to text in the user's
   unit and back; the user's choice per quantity from settings.
8. **The binding** (`ui/kit/bind/`): `FieldPort`, FunkGui's `ParamPort` over a `tee::Build` and a `FieldRef`
   (range, taper, steps, default, text through `Units`); gestures bracketed for undo; a **generated page** for a
   Spec section (headings from `ui::group`, a control chosen by the field's type and attributes: a knob, a menu,
   a switch, a `MapEdit`, a `CurveEdit`), so a field no `.gui` file places still appears. The `.gui` language
   gains: a control named by a Spec path, `auto "boost"` (the generated page of a section), `plot`, `gauge`,
   `map`, `curve`.
9. **The application's frame** (`ui/app/`): the screens of 00-abstract-plan.md §6 as a rail at the left (Build,
   Valvetrain, Boost, Exhaust, Tune, Dyno, Drive, Scope, Garage), a top bar (the build's name, its state, the
   output's level, the start button), a bottom strip (the pedals as bars, the gear, speed), each screen a `.gui`
   page of generated sections until wave 2 gives it its view. Dark, an instrument's look; one accent.
10. **Tests** as J1's, and a contact sheet (`out/ui/sheet.png`) of every screen at two sizes for a person to look
    at in one glance.

**J2 is done when** every Spec field of every section can be seen and set from the application's frame with no
hand-written C++ for it, files round-trip, and the contact sheet exists.

## Not in this card

The hero views and their shaders (wave 2: the cutaway, the exhaust's waves, the dash grown, the cam editor, the
compressor map, the dyno and scope screens' content). Menus, dialogs and the window (card A). Audio and input
devices (card B).

## Building alone

`ui/` must configure and test with nothing but FunkGui's core (JUCE off) and a made-up `Player`, so its tests run
in a container that has no LLVM: give `ui/CMakeLists.txt` a top-level mode for that (`cmake -S ui -B build-ui`),
besides being added by the root.
