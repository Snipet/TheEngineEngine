# Handoff: card B, the shell and the application (slice 1, and part of slice 2)

*Branch `claude/awesome-gates-ntdb58`; paths `shell/`, `app/`, `tests/shell/`, `docs/controls.md`,
`docs/checkpoints/M1.md`. 2026-11-24. Nobody has run the application on a real GPU or heard it through a sound card
yet: everything here was measured in cloud containers and on CI's runners.*

**Done, by test** (`ctest -R shell`: 27 tests in a default tree, 30 with `TEE_BUILD_APP=ON`; all pass here, and on
CI's Linux, macOS and Windows runners):
- controls (ControlsTest.cpp): bindings, eased pedals, the start key's rule, a tap shorter than a frame, shifts as
  counted presses, a gear key that shifts until the gearbox is there, the lever, a Panel's own controls;
- the reference as a `Player` (RefAdapterTest.cpp): 32 dials, telemetry in SI and as the twelve raw `show*` fields,
  `outputTail`, a start, an idle, a rev, a shift, a stop;
- the audio device on SDL's dummy driver; the recorder, sample for sample; the settings file; PNG, WAV, script, options;
- `tee-headless` on scripts: `shell.headless.ref` (start, rev, stop: a WAV and PNGs), `.drive`, `.tone`, `.error`, `.refuses`;
- the window under `scripts/xvfb-run.sh` (Mesa's software Vulkan, dummy audio): `shell.window.gpu`, `.plain`, `.info`.

**CI artefacts** (workflow Build; run 45399202005 was the first with all three): `TheEngineEngine-windows-x64`,
`TheEngineEngine-macos-arm64` (an `.app` signed ad hoc with the entitlement; `codesign --verify` passes),
`TheEngineEngine-linux-x64`, and `TheEngineEngine-listen-<platform>` (a WAV and pictures). On each runner the window
opened (Direct3D 11; Metal; Vulkan), the reference compiled in 1.3 to 1.9 s, started and revved to about 6,350 rpm
from scripted keys, and a frame was read back from the GPU.

**Measured.** The scripted render (14 s at 48 kHz): finite, peak 0.84318, rms 0.11550, the same on all three systems;
strongest low line 59 Hz at the fast idle and 429 Hz at the limiter; silence after the stop; it renders in 4.4 s here
with its pictures. SDL's dummy driver: 44,100 Hz, a buffer of 256 frames (5.8 ms); the macOS runner's virtual
CoreAudio device: 48,000 Hz, 256 frames (5.3 ms). Devices that play to nowhere: nothing about a real one's latency.

**Not done.** Builds on disk (waits for card C's Schema); a wheel (only the seam, `shell/input/Wheel.h`); units; a
device opened by its name from settings (always the default); nothing in the window changes a setting yet. The
window shows `RefPanel` over the reference: `tee::ui::AppFrame`, `tee::Build` and `SimHost` are the next step. The
gamepad code has never met a gamepad. A person's answers to `docs/checkpoints/M1.md` are still to come.

**Changed outside my paths** (for the keel's handoff): `cmake/TeePins.cmake` (FunkGui at 15e3775, SDL3, bgfx.cmake,
bgfx, bx, bimg); `cmake/TeeDeps.cmake` (two `include` lines before FunkGui); root `CMakeLists.txt` (`shell/` and
`app/` always added); `.github/workflows/build.yml` (the shader compiler's cache; the application in `build-app`,
built first, packed, uploaded, run). **Wanted:** `TEE_BUILD_APP` is OFF by default because a fresh container would
build bgfx's shader compiler (about 2,000 CPU-seconds); if `scripts/bootstrap-cloud.sh` put one in
`$TEE_DEPS_DIR/tools/shaderc-v1.153.9385-561/` (with `shaderc.stamp`), the default could be ON.
`TelemetrySource::outputTail` has a body (returns 0) so older Players built: make it pure when all override it.

**Met on the way.** A start-key tap shorter than a slow frame was lost (fixed in the Driver). At scale 1 the
speedometer's "1" of 100 to 180 is under a pixel wide (right at scale 2). On the Windows runner's Direct3D 11,
`header.sc` is refused ("reads the uniform ORANGE") and the shaders' dot-font words do not draw: card J's;
`--panel plain` needs no shader. FunkGui's sources compile inside our targets: with our `-ffp-contract=off`
(checked on every one), without our `-Werror`.

**For a person:** `docs/checkpoints/M1.md`; if the application will not run, the listen artefact's WAV.

## Second handoff (2026-11-24, afternoon): the frame is the application; slice 2

*Card B, branch `claude/awesome-gates-ntdb58`; a second session continuing the first. Run here: Ubuntu 24.04 x86-64,
4 cores, no GPU, display or sound, a `dev` tree with `TEE_BUILD_APP=ON`.*

**Done, by test** (`ctest -R '^shell'`: 51 of 51 pass here; the window's under `scripts/xvfb-run.sh`, Mesa's software
Vulkan, SDL's dummy audio):
- The frame over the simulator is the default (`--engine ref`/`--ref-panel` keep the reference and `RefPanel`). It is
  given U1's `ServiceDyno` and `SimScope`, the shell's services (`unsaved`, `simLoad`, `mode`/`setMode`, `clutch`,
  the build's name and files) and GARAGE's library (`AppFrame::setLibrary`). `shell.workspace.<id>` for each of the
  ten: opens over the simulator, starts the engine, 120 frames, a picture; `shell.workspace.none`: an unknown id exits 2.
- `--size WxH`, `--scale S`, `--dyno-wait S` (tee-headless; returns at the dyno's error with its words). Every workspace:
  `for w in garage engine valves air exhaust tune car dyno drive scope; do build-dev/bin/tee-headless --workspace $w
  --size 1280x800 --scale 2 --frames 120 --script tests/shell/scripts/workspace.txt --shot out/sheet/$w.png; done`.
- `PanelControls` gained `clutch` and `handbrake` (the lead's decision), merged as throttle and brake are (ControlsTest).
- Builds on disk (`shell/files/BuildFiles`, BuildFilesTest): new, open, save, save as, a part apart, recent files, the
  dirty flag, a library by name. In the window: Ctrl/Cmd+N/O/S/Shift+S, F2 the File menu, F9 the Audio menu (device
  by name, rate, buffer, recorder), FunkGui's menus and SDL's file dialogs; the last build opens at start; a build
  unsaved at close is kept as "Unsaved changes". Not tried by a person; the dialogs never opened here (no desktop).
- GARAGE's `Library` (U5's header) in `app/GarageLibrary` (`tee-shell-frame-tests`, 66 assertions): shipped and user
  builds and parts, values, current, load (a part loads only the fields its file sets: `Build::loadFile` with a part
  puts the rest of the part's sections back to their defaults), save, save as, duplicate, delete, revert, notes,
  curves by card C's dyno runner in the background cached by hash.
- The audio device by name from the settings, taken back when it returns (not tested: the dummy driver has one device).
- The wheel: `shell/input/WheelMap` (a table a device, a learn mode) tested with a made-up wheel (WheelTest); no device.
- `-ffp-contract=off` on all 112 compiles of `tee_app`, `tee-headless` and `TheEngineEngine` (100 FunkGui's). No static
  in `shell/` or `app/` owns a program or a thread.

**Measured.** The ribbon's predicted curve arrives 2.7 s after the frame opens (card D's `tel.engine.torque`; default
V8: 505 N m at 1,439 rpm, 224 kW at 5,254 rpm). The window on Xvfb: 70 fps, LOAD 14 %. tee-headless's LOAD reads high
(91 %): it renders faster than real time, so a callback's time over its buffer's means nothing there.

**Not done.** GARAGE uses the shell's library only once U5 reads `ctx.library` (asked). A wheel opened through SDL, its
table in settings, a window to learn in. A settings page in the window (the frame has none yet: the keys stand in).
Robustness was not exercised on a real device (unplug, rate change); the paths are SDL's and the code's own.

**CI.** Run 45407345444 built, packed, uploaded and ran the application on Linux and Windows (macOS was still running);
its Test steps fail outside my paths (`ui.frame.ribbon`, `ui.build.frame`; `tee-host-tests.exe` 0xc0000374 on Windows).

**Outside my paths, wanted.** U5: `makeGarageWorkspace` to take `ctx.library`; `scope.gui`'s brace-less panel. The
`cams/*.json` set the lab valvetrain's fields, which `sim/engine.fm` does not have: they load nothing yet.

**For a person.** On the Mac: the frame, every workspace; F2 and F9; Ctrl/Cmd+S on an untitled build (the save dialog);
an edit on ENGINE and the ribbon answering; the audio device menu with headphones plugged and unplugged.

**Launch (the lead's 20:06 ask).** With no `--preset` and nothing remembered the window opens the first build,
`content/builds/first.json` (P's file; until it exists, the simulator's defaults as before), as a remembered build is
opened (`app/Launch.h`): clean, its name in the top bar, its values sent as Spec writes after init. `--reference` opens
none, every model at Reference; so does a scripted window (--frames, --shot) unless it names one (--first-build,
--preset), by the lead's ruling: CI's pictures and timings do not move with a content file. A shipped build is never written over by Ctrl+S (it asks for a file). tee-headless is
unchanged. Tests: `LaunchTest.cpp` (the choice), `shell.window.first` (first.json named; its "Try first" line, its name, and
every model switch the Schema has on, in the document and as the engine was sent them: one at 0 fails it), `shell.window.last` (nightly: the remembered
build wins).

**What a person should look at.** A first launch shows the first build, not the bare reference engine. GARAGE is where
the other archetypes are (the bikes, the diesels, the V8s and V10s, the truck); `--reference` shows the bare engine
for a comparison.


**The calibration bakes' cache (C's 7f2ddfa1; 169a729a).** A person's window passes `CalibrationBakeOptions::
cacheDirectory` = `settings::cacheDirectory()/bakes`: `TEE_CACHE_DIR` when set, else
`%LOCALAPPDATA%\TheEngineEngine\bakes` (Windows), `~/Library/Caches/TheEngineEngine/bakes` (macOS),
`$XDG_CACHE_HOME/TheEngineEngine/bakes` or `~/.cache/TheEngineEngine/bakes` (Linux). A machine's run (`--frames`,
`--shot`, `--seconds`) uses none unless `TEE_CACHE_DIR` names one, so no test writes outside a folder it made. The bake
yields to the ribbon's PredictService (ServiceDyno's) once U1's ServiceBake takes it; GARAGE's library curves are a
DynoService (no PredictService) and simply share the cores.
