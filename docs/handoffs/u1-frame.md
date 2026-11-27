# Handoff: package U1 (the frame, DRIVE) — branch `claude/awesome-gates-ntdb58`

**Done** (`ctest -R '^ui\.'`, root `dev` preset, Linux; pictures by FunkGui's software rasteriser, no GPU run):
- Step 1 (bd34d6b): `Workspace`, `WorkspaceContext`, `Selection`, `ScopeSource`, `DynoSource`, `Live`, `look::Palette` and
  helpers, `DemoEngine`/`DemoRig`, `ui/views/_example`, the generated registry, `tee-view-tests` (`ui.example.select`,
  `ui.look.palette`, `ui.demo.source`). How-to: `docs/handoffs/u1-interface.md`. U2-U5 have since registered engine,
  exhaust, tune, dyno, scope on it unchanged.
- Step 2, the frame: `ui.frame.workspaces` (ten in order, every inspector page note-free, every non-hidden field on a
  page), `ui.frame.select`, `ui.frame.set` (drag, Cmd-Z, menu, unsaved star), `ui.frame.cockpit` (START, pedals drag and
  spring back, clutch, mode, readouts and dashes, shift lights), `ui.frame.garage` (J2's garage kept; units page),
  `ui.frame.ribbon` (edit -> new curve, old ghosted; folds), `ui.frame.sizes` (1100x700 to 2560x1440; rail narrows
  under 1280), `ui.frame.idle` (no frame wanted at rest), `ui.frame.sheet`; real Spec: `ui.build.frame`, `ui.build.set`.
- Step 3, DRIVE (`ui/views/drive`): `ui.drive.instruments`, `ui.drive.small` (goldens at density 2 and 1).
- Step 4: `ServiceDyno`/`SimScope` (`ui/app/host`, in `tee_ui_build`): `ui.build.dyno` on the real simulator.
- **Contact sheet: `out/ui/sheet.frame.png`** (density 2) and `sheet.frame.dpi1.png`: each workspace at 1100x700 and
  1600x1000; `out/ui/frame.<id>[.dpi1].png`, `frame.engine.paper.png` (the paper theme).

**What the shell must do** (card B; `app/Offline.cpp` already shows the frame for `--engine sim`, through J2's names,
which `AppFrame` keeps): pass the dyno and the Scope, else the ribbon says "no dyno runner":
`ServiceDyno dyno(sim->host().sim(), sim->build()); SimScope scope(sim->host());`
`makeAppFrame(doc, units, player, services, &dyno, &scope)` (both kept alive with the frame, in `Shown::keep`). Fill
the new `FrameServices` members: `unsaved`, `simLoad`, `mode`/`setMode` (ControlsFrame::mode), `clutch` (the mouse's
clutch pedal, -1 when let go). Load the units from the settings (`Units::load`). `--ref-panel` keeps `RefPanel`.

**Not done / waits**: the dyno curve on the real simulator waits for `tel.engine.torque` (the runner says the program
lacks it; the ribbon shows that sentence). Lambda and oil temperature read `engine.lambda`, `engine.oilTemperature`
(not in telemetry.fm yet: dashes). DRIVE's slip, tree and slip times read card I's `car.wheelSlip[i]`, `car.stripState`,
`car.t60`... (waiting for I's integration). No settings button (only units) and no rail pictograms yet. No GPU or Xvfb
run; macOS/Windows: CI only.

**Wanted outside my paths**: (1) Lead: `PanelControls` has no clutch; the frame reports it through
`FrameServices::clutch` until it does. (2) Lead/H: `tel.engine.lambda`, `tel.engine.oilTemperature`. (3) U5:
`ScopeWorkspace::inspectorPage()` starts with `panel "SCOPE"` without braces: a .gui note that fails
`ui.frame.workspaces` and `ui.build.frame` (drop the line). **Additive kit change**: `GuiPanel::setInsets` (J's tests
green). `ui/CMakeLists.txt` and `tests/ui/CMakeLists.txt` gained the view glob and `tee-view-tests` blocks.

**Changed after step 1** (compatible): `WorkspaceContext::services`, `Workspace::shownChanged`, `Live` re-asks absent
names, `DemoEngine::hold`. **FunkGui**: no issue met.
**For a person to look at**: `out/ui/sheet.frame.png`, `frame.drive.png`, `frame.air.png` (placeholder + inspector +
ribbon), `frame.engine.paper.png`; on a Mac: the application, DRIVE while revving (the rpm bar's shift lights, the
pedals dragged), an edit on AIR and the ribbon answering.

**27 Nov 2026, the bake across edits (card C's options 1 and 2, the owner's yes; the words U1's, the lead's ruling):**
`ui/app/BakeStatus.h` is the bake as fields (phase; stage and count; the old tables still playing, spark and cam; set
aside; the field that restarted it, by its label; the stages kept), from `ServiceBake::status()` (C's `tee::BakeStatus`
mapped) or a state string parsed. Words: `bakeLine` (the status line: "Baking the spark map 12 of 80", "Bakes once
your edits rest" with no number, the lead's ruling: the rest is a debounce, not counted down; then a reason as a field
dropped whole when short of room: "restarted: compression changed", "spark map kept, unchanged by the edit"),
`bakeStaleLine` (a second small line under it only while old tables play: "Old tables in use until baked"), all in
the label's ink. While either is said the status line may take up to 420 px (the name keeps 200); old states keep
their words and place. The ribbon's caption while old tables play: "ON THE OLD TABLES UNTIL BAKED" (the predictor
runs on the build as it stands: C). TUNE says the same (U4's handoff). Tests: `frame.bake.stale` (made-up states),
`build.bake.stale` (the real service on first.json: an edit keeps the old tables, an edit of the cylinders sets them
aside), and the nightly `build.bake.pictures` (an edit's whole course on the bake's state).
The lead's look (27 Nov, 12:42: the patch's text applied, built, the pictures drawn, the bite repeated) was on the text
before two mends the lead then asked for, which the lead took as described and did not draw again: tune.bake.stale's rig
given a baked table, and TUNE's "old table in use" said from the service's own word when no telemetry says it (U4's
handoff). J2's tier covers them.

**27 Nov 2026, SETTINGS' CLUTCH row and the page's scroll (the lead's rulings):** CLUTCH under WHEEL: three buttons
across the column (Shifter's, Automatic, Yours; card B's three-way `input::ClutchMode` through
FrameServices::clutchMode/setClutchMode, wired in app/Offline.cpp as coldStart is), the mode lit and following a change
made elsewhere (B's F9 item), B's line for the gearbox loaded under them (`autoClutchSays`, two lines at most). With it
the page no longer fits 1100 x 700 when every row is served, so the settings page SCROLLS where its rows do not fit
(`placeSettings`, `settingsScroll_`): every row in its own place and size, the page moved up; a widget, heading or line
not wholly in the room is not shown (nothing over the cockpit or the ribbon); the wheel by the inspector's own rule (over
a stepper's value box its unit, the page still; elsewhere the page, by funkgui::scroll::wheelPx with the Rows' row height,
the trackpad speed mended on 26 Nov); Tab and Shift-Tab bring a stop scrolled out into view before focusing it; a thin bar
at the column's right edge while it scrolls; at the top when SETTINGS is opened or the window changes. Where the rows fit,
nothing changes (offset 0; no golden moved). Test `frame.clutch`; pictures out/ui/frame.clutch[.bottom][.small][.dpi1].png.
Against a press and a focus (the lead's question): while a press holds, the wheel does not move the page (the press
ends as a click where it began; before, the page moved under it and the release landed on nothing); a stop focused and
then wheeled out of the room keeps the focus, unseen, and the first key it takes scrolls it in whole before acting (before,
an arrow changed a unit out of sight). A press on a stepper steps on the press, so a resize under it (the page back at its
top) leaves its release nothing to do. Test `frame.settings.held`.

**27 Nov 2026, a question withdrawn (card A's ask, the lead's review of the Claude integration, item 3):**
`AppFrame::ask` takes a fourth, defaulted argument, `std::shared_ptr<const std::atomic<bool>> withdrawn`, which the asker
raises from its own thread when it no longer waits (load_build's askSeconds run out, a cancel, the session stopping). A
queued question so raised is never shown, and its future is answered false. An open one closes on the frame's next tick
as a no would, answered false (nobody reads it), and the next question opens in its place, laid out once. The flag is read
with acquire on the frame's thread; no lock. Every caller compiles unchanged. Test `frame.ask.withdrawn`. Card A's half
(SessionParts::ask, Tools, app/Offline.cpp's askOwner) follows on this.
