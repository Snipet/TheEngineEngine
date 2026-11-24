# Status

*Kept by the lead. Times are UTC, 2026-11-24.*

| Package | State | Where it runs | Since | Notes |
|---|---|---|---|---|
| A: FunkGui desktop | **landed 13:20; merged 13:26** | local agent (finished) | 11:22 | Snipet/FunkGui pull request 38 merged to main at `19d06ab` (version 0.28.0): `DesktopHost` on SDL3, the menu overlay, file dialogs, clipboard, preferences, `FunkGui::gpu` without JUCE; CI green on Windows (44 tests, Direct3D 11), macOS (70, Metal), Linux (70, Vulkan on lavapipe). **The tag `v0.28.0` waits for the owner**: this session type may not push a tag. 40 new golden rows of `fg.desktop.menu` are candidates the lead has not adopted yet. Handoff: `docs/handoffs/a-funkgui-desktop.md` |
| K1, K2: the keel | **landed** (handoff 13:00) | local agent | 11:27 | R measured: the reference V8 is 2.3 us a sample, 22 to 23 % of a core here, 24.7 % on CI's M1; `scripts/ci-status.sh` |
| B: shell and application | running | local agent, `/home/user/work/shell` | 11:30 | interfaces pushed (`Sources.h`, `MainPanel.h`) |
| J1: interface kit | **landed 12:04** | cloud session | 11:32 | the panel code ported (36 files, provenance in `ui/kit/SEED.tsv`), `RefPanel`, shaders compiled for Metal and SPIR-V; 29 headless tests; no shader drawn yet (no GPU path ran) |
| K3: the split | K3a **landed 12:30**; K3b running | local agent, `/home/user/work/split` | 11:55 | carved so far (13:04): Listen, Extras, Car, Heat, Boost, Ecu, Combustion; signal for integration: `docs/handoffs/k3b.md` and no `sim/core/monolith.fm` |
| C: host and tools | **landed** (first handoff, 13:00) | local agent | 11:55 | 45 tests: Schema and its lint, `SimHost`, `Build`, reload, the dyno runner, the scenario library, replay, validation. ThreadSanitizer never ran here (no runtime in the container). The dyno on the real engine waits for K3b and D |
| I: vehicle | standalone half **landed 12:45**; integration waits for K3b | local agent (finished) | 12:00 | 32 tests: tyres, differentials, loads, five gearboxes, the strip's timers, a chassis dyno; three car builds |
| F: forced induction | standalone half **landed 13:04**; integration waits for K3b | local agent (finished) | 12:00 | 26 tests: the map from sizes, lag and spool trends, energy and mass balance, the gate's loop, surge at 26 Hz with no valve, twin-scroll, three superchargers, nitrous, a passive housing, a ten-minute soak |
| E: valvetrain and induction | standalone half **landed 12:38**; integration waits for K3b | local agent (finished) | 12:00 | 14 tests |
| H: combustion and ECU | standalone half **landed 12:28**; integration waits for K3b | local agent (finished) | 12:00 | 19 tests |
| D: mechanics and layouts | running | cloud session `session_01LFAGgTFpHiA9YGryQgFytL` | 12:07 | standalone first (layouts from geometry, inertia, shaking, friction, work); the integrator's job after K3b |
| G: exhaust builder | standalone half **landed 13:08**; integration waits for K3b | cloud session `session_01LFAGgSkq1paVPmFHkoNNei` | 12:08 | 12 tests: bit-identical to the reference's exhaust at its defaults for all 17 layouts; passivity over 50 random topologies; conservation; headers, crossovers, resonators, mufflers, the valve measured in dB. Over its CPU budget for a typical dual system (0.31 R against 0.25 R) |
| J2: widgets, binding, the frame | **landed 12:47** | cloud session | 12:05 | eleven Canvas widgets (gauge, plot, map and curve editors among them), units, the binding to the real `tee::Build`, generated pages, a first frame of nine screens; 56 UI tests; contact sheets at density 1 and 2 |
| U1: the redesigned frame | interface **landed 13:12**; the frame running | cloud session `session_01LFAGgPEvM8vsbaJsuRqXAz` | 12:53 | [ui-design.md](ui-design.md): the `Workspace` interface and the look first (U2 to U5 wait on it), then the frame, the cockpit, DRIVE |
| U2: ENGINE, VALVES | running | cloud session `session_01LFAGgQaoxE3tRGwUcwpU2G` | 13:19 | the block from above with its firing ring, one cylinder in section, the cam editor and timing ring |
| U3: AIR, EXHAUST | running | cloud session `session_01LFAGgTSo5cKehJoa4H3k2A` | 13:19 | the charge path and the compressor map; the exhaust as built, with heat and travelling pulses |
| U4: TUNE, CAR, DYNO | running | cloud session `session_01LFAGgPYn8yFr9MJ2vQvgRd` | 13:19 | the maps with the live cell; the car from the side, the driveline, the gearing chart; the big dyno plot |
| U5: GARAGE, SCOPE, atmosphere shaders | running | cloud session `session_01LFAGgSKtL3Z967uRP5DGT9` | 13:19 | builds as cards and compared; the instrument bench with a spectrum against speed; the shaders last |
| Wave 2 | after the gate (M2) | | | Q (performance) as soon as K3b lands |

## Landed

- **13:15 checkpoint M1, step 2, Windows, first part: passed.** The owner ran the application (run 45399202005) on a
  real Windows machine: the reference engine compiles in 2.11 s; WASAPI at 48 kHz with a 480-frame (10 ms) buffer;
  Direct3D 11 at 150 % scale; `--engine tone` held 60.0 frames a second for 789 frames with 1381 audio callbacks and
  exited cleanly. Printed, as expected: `ref/header.sc` refused (it reads a uniform of its own). Not said yet:
  whether the tone and the engine were heard, and how they sound.
- 13:26 FunkGui pull request 38 merged (card A, both stages). 13:19 U2 to U5 started (the workspaces). 13:12 U1: the
  `Workspace` interface, the look and the demo engine (`docs/handoffs/u1-interface.md`).
- 13:14 to 13:20 CI: Windows did not build at 0ee89f4 (`M_PI` in card D's tests: the keel defines
  `_USE_MATH_DEFINES`, 3cb314c); the kit's two golden tests failed where shaders were compiled (fixed, 9f8bb0d).
  **Open: on macOS 38 of card C's host tests segfault at start** (they pass on Linux); C is on it.
- 12:50 the owner asked for the interface to be redesigned from scratch: [ui-design.md](ui-design.md); package U1 started; U2 to U5 follow when its interface is on the branch.
- 12:39 two-strokes, standalone (wave 2's card T, started early by D's session while it waits for K3b): the cylinder,
  the crankcase, the expansion chamber from the reference's ducts; 7 tests; four-stroking at a closed throttle
  appears unscripted.
- 12:38 E (valvetrain and induction) and 12:28 H (combustion and ECU): standalone halves done, 14 and 19 tests;
  integration waits for K3b. 12:27 D (mechanics, layouts): standalone half done, 16 tests, 33 layout presets.
- 12:36 B: the application runs on Linux's GPU path under Xvfb with the reference engine and J1's panel; the three
  reference shaders were drawn for the first time. The speedometer's numerals look wrong from 100 up (being checked).
- 12:30 K3a: the new contract (`Engine`, Spec, Controls, Telemetry) over the unchanged reference.
- **12:25 to 12:45 checkpoint M1, step 1, Windows: passed in full.** The owner ran FunkGui's gallery artefacts on a
  real Windows machine (3840 x 2160 at 150 %, Direct3D 11): the primitives drew correctly and sharp; near 0 % CPU
  at rest; the services section's Canvas-drawn menu, the three file dialogs and the clipboard all work; `--resize`
  (run 45394491459) resizes live by dragging and maximises ("works great"). A section's window is fixed-size by
  design. macOS: the same run's artefact drew on Metal on the CI runner; no person has run it yet.
- 11:19 stage 1: the abstract plan, reviewed; the answers recorded.
- 11:45 stage 2: every file of this folder.
- 11:52 the keel's first slice: the build, the bootstrap, `sim/ref/`, `tee_host`, `tee-render`, CI (first run green).
- 12:15 card A, stage 1: FunkGui's gallery in an SDL3 window builds on all three systems; on Linux its GPU path (Vulkan on Xvfb) draws the expected picture in CI.
- 12:04 J1: the interface kit's first slice (`docs/handoffs/j1-ui-kit.md`).

## Blocked, or wanting a decision

- Resolved 12:10: cloud sessions could not fetch FMinus's prebuilt LLVM (their GitHub access covers only this
  repository). The owner published the Linux archive as the pre-release `deps-llvm-23.1.3` here, and
  `scripts/fetch-llvm.sh` falls back to it. A cloud session then bootstrapped in 25 s and built in 3.5 min
  (12:06). It fetched the archive by plain URL from FMinus's own release: release assets of a public repository
  are reachable after all (only the API and source archives are refused), so the mirror is a fallback that has
  not yet been needed. Packages started from now on run in their own cloud sessions; the nine already running
  locally stay where they are.
