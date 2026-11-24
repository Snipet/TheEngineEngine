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
