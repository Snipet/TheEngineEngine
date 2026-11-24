# U1, first step: the `Workspace` interface (for U2 to U5)

*Package U1, branch `claude/awesome-gates-ntdb58`. The frame itself (top bar, rail, inspector, ribbon, cockpit) comes
next; this is what a workspace is written against, and it will not change shape under you. The design is
[../plan/ui-design.md](../plan/ui-design.md).*

## What is on the branch

| File | What |
|---|---|
| `ui/app/Workspace.h` | `Workspace` (a `Board` whose items and drawing keep to its stage), `WorkspaceContext`, `Selection`, `ScopeSource`, `DynoSource` (+ `DynoTrace`, `DynoRequest`), `Live` (telemetry by name, NaN when absent), `registeredWorkspaces()`. Each member documented there. |
| `ui/app/Look.h` | `look::Palette::of(theme)`: ground, panel, raised, edge, grid; inks value, text, label, dim, faint; by meaning `selected`, `cold`, `hot`, `hotter`, `ok`, `warning` (graphite and paper); `heat(K)`, `pressure(Pa)` tints. Type (`kNumberL/M/S`, `kUnit`, `kCaps`, `kTitle`, `kBody`), lines (`kHair`, `kTrace`, `kNeedle`), `kGauge`, `kPlot`, `kDiagram`. Helpers: `num` (a dash for NaN), `region`, `readout`, `level`, `pipe`, `selectRing`, `lamp`, `placeholder`. |
| `ui/app/Demo.h` | `demoFields()` (a made-up Spec with the real sections), `DemoEngine` (telemetry with `sim/spec/telemetry.fm`'s names, a Scope of 720 degrees and 64 pipe points, a dyno that answers edits of `engine.compression` and `boost.level`; `set`/`forget` a name), `DemoRig` (a whole `WorkspaceContext`). Fixed script, no clock. |
| `ui/views/_example/` | The smallest whole workspace (57 lines): a `Gauge` made on the board, a drawn part whose press selects `boost`, a live value in the user's units. |
| `tests/ui/views/` | `tee-view-tests`: `ViewKit.h` (`VIEW_TEST`, `CHECK`, `StageRig`, `golden`, `picture`, `typeKeys`), `Main.cpp`, `ExampleTests.cpp` (`ui.example.select`, `ui.look.palette`, `ui.demo.source`), goldens in `tests/ui/views/golden/`. |

`WorkspaceContext::doc` is a `FieldDoc&` (J2's interface; in the application a `BuildDoc` over card C's `tee::Build`),
not `BuildDoc&` as ui-design.md §5 sketched: `ui/` alone has no `tee::Build`, and a workspace needs only the fields.

## Writing one (U2: `engine`, `valves`; U3: `air`, `exhaust`; U4: `tune`, `car`, `dyno`; U5: `garage`, `scope`)

1. `ui/views/<id>/<Name>Workspace.cpp` (and `.h` if you like): a class deriving `tee::ui::Workspace`; implement
   `id()` (the directory's name), `title()` ("ENGINE"), `sections()` (the Spec sections its inspector shows when
   nothing is selected), `layout(stage)` (place your widgets inside `stage`), and draw the rest in
   `drawStage(canvas, theme)` with `look::Palette::of(theme)`. Optional: `inspectorPage()` (a `.gui` text for the
   selection; default: the frame's generated pages of the selected prefix), `ribbon()` (false on DYNO), `moving()`,
   `drawKey()` (mix what `drawStage` reads, so a still frame is not drawn again), `step(dt)`.
2. Clicking a part: override `pointerDown` (and `pointerMove` for `selection.hover`), call
   `ctx_.selection.select("exhaust.header")` for what was hit, `Board::pointerDown(e)` otherwise. Mark the selected
   part with `look::selectRing` or `region(..., selected)`: never colour alone.
3. Values: `ctx_.live.get("exhaust.portTemperature")` (SI, NaN when absent: draw `look::num(v)`, which is "-");
   fields: `ctx_.doc.get(ctx_.doc.find("valve.intakeLift"))`; Scope: `int h = ctx_.scope.find("pressure");
   ctx_.scope.read(h, buf, n)` only while shown; dyno: `ctx_.dyno.predicted()`, `before()`, runs. Units:
   `ctx_.units.format(Quantity::pressure, pa)`.
4. Register: `ui/views/<id>/register.h`, one line and no include guard (it is included twice):
   `TEE_UI_WORKSPACE(<id>, make<Name>Workspace)`, where `std::unique_ptr<Workspace> make<Name>Workspace(WorkspaceContext&)`
   is in namespace `tee::ui`. Re-run CMake; the build lists it (generated `WorkspaceRegistry.cpp`) and the frame puts it
   on the rail in place of that id's placeholder. Directories starting with `_` are skipped.
5. Rules: Canvas primitives only, opaque strokes (`p.over(c, a)`), text >= 10 px, no rotation, gradients or filled
   polygons; time only from `step(dt)`/`tick`; `drawStage` changes nothing; words, not icons.

## Testing and picturing one

`tests/ui/views/<Name>Tests.cpp`, nothing else to edit (CMake reads the `VIEW_TEST(ident, "name")` lines; one per
line). `StageRig rig(make<Name>Workspace, 1100, 700, /*dpi*/ 2.0f, /*t*/ 4.6)` hosts the workspace alone over the demo;
`rig.host->click(x, y)`, `rig.demo.selection.path()`, `rig.at(t)` (another moment of the script),
`rig.demo.engine.set/forget(name)`. `golden("air.stage", *rig.host)` compares a fingerprint and writes
`out/ui/air.stage.png`; make one at dpi 1 too (`golden("air.stage.dpi1", ...)`). Bless only after looking:
`build-dev/bin/tee-view-tests air.stage --bless`. Run: `ctest --test-dir build-dev -R '^ui\.'`. The frame's contact
sheet will include every registered workspace.
