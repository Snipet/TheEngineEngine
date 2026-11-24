# Status

*Kept by the lead. Times are UTC, 2026-11-24.*

| Package | State | Where it runs | Since | Notes |
|---|---|---|---|---|
| A: FunkGui desktop | running (stage 2) | local agent, `/home/user/FunkGui` | 11:22 | **gallery artefacts exist (12:15)**: Windows and Linux jobs green in run 45389650505, macOS built; draft pull request Snipet/FunkGui#38 carries the run instructions; nothing has drawn on Metal or on a real Windows GPU yet |
| K1, K2: the keel | running | local agent, `/home/user/work/keel` | 11:27 | pushed by 11:52: build, bootstrap, `sim/ref/`, `tee_host`, `tee-render`, CI; finishing the bench and tests |
| B: shell and application | running | local agent, `/home/user/work/shell` | 11:30 | interfaces pushed (`Sources.h`, `MainPanel.h`) |
| J1: interface kit | **landed 12:04** | cloud session | 11:32 | the panel code ported (36 files, provenance in `ui/kit/SEED.tsv`), `RefPanel`, shaders compiled for Metal and SPIR-V; 29 headless tests; no shader drawn yet (no GPU path ran) |
| K3: the split | K3a **landed 12:30**; K3b running | local agent, `/home/user/work/split` | 11:55 | carved so far (13:04): Listen, Extras, Car, Heat, Boost, Ecu, Combustion; signal for integration: `docs/handoffs/k3b.md` and no `sim/core/monolith.fm` |
| C: host and tools | running | local agent, `/home/user/work/hosttools` | 11:55 | Schema lint and the scenario library first |
| I: vehicle | standalone half **landed 12:45**; integration waits for K3b | local agent (finished) | 12:00 | 32 tests: tyres, differentials, loads, five gearboxes, the strip's timers, a chassis dyno; three car builds |
| F: forced induction | standalone half **landed 13:04**; integration waits for K3b | local agent (finished) | 12:00 | 26 tests: the map from sizes, lag and spool trends, energy and mass balance, the gate's loop, surge at 26 Hz with no valve, twin-scroll, three superchargers, nitrous, a passive housing, a ten-minute soak |
| E: valvetrain and induction | standalone half **landed 12:38**; integration waits for K3b | local agent (finished) | 12:00 | 14 tests |
| H: combustion and ECU | standalone half **landed 12:28**; integration waits for K3b | local agent (finished) | 12:00 | 19 tests |
| D: mechanics and layouts | running | cloud session `session_01LFAGgTFpHiA9YGryQgFytL` | 12:07 | standalone first (layouts from geometry, inertia, shaking, friction, work); the integrator's job after K3b |
| G: exhaust builder | running | cloud session `session_01LFAGgSkq1paVPmFHkoNNei` | 12:08 | standalone first (the network on the reference's `Duct`); integrated after K3b |
| J2: widgets, binding, the frame | **landed 12:47** | cloud session | 12:05 | eleven Canvas widgets (gauge, plot, map and curve editors among them), units, the binding to the real `tee::Build`, generated pages, a first frame of nine screens; 56 UI tests; contact sheets at density 1 and 2 |
| U1: the redesigned frame | running | cloud session `session_01LFAGgPEvM8vsbaJsuRqXAz` | 12:53 | [ui-design.md](ui-design.md): the `Workspace` interface and the look first (U2 to U5 wait on it), then the frame, the cockpit, DRIVE |
| Wave 2 | after the gate (M2) | | | Q (performance) as soon as K3b lands |

## Landed

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
