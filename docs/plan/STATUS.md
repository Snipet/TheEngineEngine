# Status

*Kept by the lead. Times are UTC, 2026-11-24.*

| Package | State | Where it runs | Since | Notes |
|---|---|---|---|---|
| A: FunkGui desktop | **landed and merged 13:19** | local agent (finished) | 11:22 | Snipet/FunkGui pull request 38 merged to main at `19d06ab` (version 0.28.0): `DesktopHost` on SDL3, the menu overlay, file dialogs, clipboard, preferences, `FunkGui::gpu` without JUCE; CI green on Windows (44 tests, Direct3D 11), macOS (70, Metal), Linux (70, Vulkan on lavapipe). Tagged `v0.28.0` by the owner at 13:28. 40 new golden rows of `fg.desktop.menu` are candidates the lead has not adopted yet. Handoff: `docs/handoffs/a-funkgui-desktop.md` |
| K1, K2: the keel; now the build and CI steward | **landed** (handoff 13:00); steward running | cloud session `session_01LFAGgPuDhrgAnggttDA8nH` (since 13:30; the local agent was stopped by an accidental interrupt at 13:26, nothing lost) | 11:27 | R measured: the reference V8 is 2.3 us a sample, 22 to 23 % of a core here, 24.7 % on CI's M1; `scripts/ci-status.sh`; `tee-bench --profile`. The steward watches CI on the three systems, keeps it fast, runs ThreadSanitizer once |
| B: shell and application | running (slice 2; the frame as the application) | cloud session `session_01LFAGgPYzYByoQt6gue8ek5` (since 13:30; the local agent was stopped at 13:26, nothing lost) | 11:30 | slice 1 landed: the application on three systems, artefacts in CI, `tee-headless`; the simulator in the interface's frame (`--engine sim`). Next: FunkGui pinned at v0.28.0, the frame by default, any workspace pictured with `--workspace`, builds on disk |
| J1: interface kit | **landed 12:04** | cloud session | 11:32 | the panel code ported (36 files, provenance in `ui/kit/SEED.tsv`), `RefPanel`, shaders compiled for Metal and SPIR-V; 29 headless tests; no shader drawn yet (no GPU path ran) |
| K3: the split | **landed**: K3a 12:30, K3b 13:20 (handoff written by the lead 13:43) | (finished) | 11:55 | every module of 02-contracts.md §6 carved; `sim/README.md` is the seam document; verified by the lead at 45474be: 272 of 272 tests, the null test identical to the bit, the cost 1.03 of the reference |
| C: host and tools | first slice **landed 13:24**; second slice running | cloud session `session_01LFAGgSy2V7eFxcA5Jr8M6s` (since 13:30; the local agent was stopped at 13:26, nothing lost) | 11:55 | 45 tests: Schema and its lint, `SimHost`, `Build`, reload, the dyno runner, the scenario library, replay, validation. Second slice: a dyno fast enough for the ribbon (a first curve within a second of an edit), one `tee::Build`, `tee-render` with builds and scenarios |
| I: vehicle | standalone half **landed 12:45**; **integrating since 13:33** | local agent (resumed 13:33) | 12:00 | 32 tests: tyres, differentials, loads, five gearboxes, the strip's timers, a chassis dyno; three car builds |
| F: forced induction | standalone half **landed 13:04**; **integrating since 13:33** | local agent (resumed 13:33) | 12:00 | 26 tests: the map from sizes, lag and spool trends, energy and mass balance, the gate's loop, surge at 26 Hz with no valve, twin-scroll, three superchargers, nitrous, a passive housing, a ten-minute soak |
| E: valvetrain and induction | standalone half **landed 12:38**; **integrating since 13:33** | local agent (resumed 13:33) | 12:00 | 14 tests |
| H: combustion and ECU | standalone half **landed 12:28**; **integrating since 13:33** | local agent (resumed 13:33) | 12:00 | 19 tests |
| D: mechanics and layouts; **the integrator** | integrating (since 13:33) | cloud session `session_01LFAGgTFpHiA9YGryQgFytL` | 12:07 | first the hooks for every package as no-ops, the fields asked of the lead, one model switch a package; then its own (layouts as data, moving masses, friction, the engine dyno). Two-strokes (standalone, 7 tests) parked |
| G: exhaust builder | standalone half **landed 13:08**; **integrating since 13:33** | cloud session `session_01LFAGgSkq1paVPmFHkoNNei` | 12:08 | 12 tests: bit-identical to the reference's exhaust at its defaults for all 17 layouts; passivity over 50 random topologies; conservation; headers, crossovers, resonators, mufflers, the valve measured in dB. Over its CPU budget for a typical dual system (0.31 R against 0.25 R) |
| J2: widgets, binding, the frame | **landed 12:47** | cloud session | 12:05 | eleven Canvas widgets (gauge, plot, map and curve editors among them), units, the binding to the real `tee::Build`, generated pages, a first frame of nine screens; 56 UI tests; contact sheets at density 1 and 2 |
| U1: the redesigned frame | frame, cockpit, ribbon and DRIVE **landed 13:44** (`docs/handoffs/u1-frame.md`); corrections running | cloud session `session_01LFAGgPEvM8vsbaJsuRqXAz` | 12:53 | next: the frame's goldens free of other packages' stages, the stage rects for the workspaces' tests, the interface additions U4 and U5 asked for (a run's partial trace, steps and gear, a save-file service, the garage's library), the demo hook |
| U2: ENGINE, VALVES | both stages on the branch (13:32, 13:48); corrections running | cloud session `session_01LFAGgQaoxE3tRGwUcwpU2G` | 13:19 | the block from above with its firing ring, one cylinder in section, the P-V loop; the cam editor with overlap and the piston ghosted, the timing ring |
| U3: AIR, EXHAUST | both stages on the branch (13:34, 13:48); corrections running | cloud session `session_01LFAGgTSo5cKehJoa4H3k2A` | 13:19 | the exhaust drawn from the tables with a pulse riding it; the charge path as a row of parts, the compressor map |
| U4: TUNE, CAR, DYNO | three stages on the branch, handoff 13:45 (`docs/handoffs/u4-tune-car-dyno.md`); corrections running (text collisions in the frame's stage, CAR worst) | cloud session `session_01LFAGgPYn8yFr9MJ2vQvgRd` | 13:19 | DYNO is right as it is; `MapEdit` gained block selection |
| U5: GARAGE, SCOPE, atmosphere shaders | SCOPE on the branch (13:33); GARAGE running; its library seam defined (`ui/views/garage/Library.h`) | cloud session `session_01LFAGgSKtL3Z967uRP5DGT9` | 13:19 | the instrument bench with a spectrum against speed; builds as cards and compared; the shaders last |
| Q: performance | running | cloud session `session_01LFAGgPBEbLATNvMiRU1PGS` | 13:44 | where R goes by module; FMinus's own levers; the resampler and the unowned modules first; patches for the pipes and the cylinder through the lead. Target: R down by a fifth, a turbo V8 with a dual exhaust under 60 % of a core |
| Wave 2, the rest | after the gate (M2) | | | the listener, mechanical voices, two-strokes (started), diesels, bikes, content, the wheel, release |

## Landed

- **13:48 CI, the first run with the three fixes** (45404725485 at 7c4871f; the build steward's report):
  Linux builds, 290 of 297 pass, the 7 failures all interface tests (the frame's goldens against stages that have
  since landed, SCOPE's page, four `rest`/`slow`/`freeze` tests: their owners are on them). Windows builds and its
  application artefacts uploaded, but **no test runs there**: `tee-host-tests.exe --list-tests` exits with heap
  corruption (0xc0000374) at discovery, which stops ctest; card C has it (its tests replace the global `new` and
  `delete`). macOS: the job was still waiting for a runner at 13:48, so the exit-crash fix is not yet shown there.
- 13:49 ThreadSanitizer ran for the first time (the steward, Linux, clang 18): `tee-host-tests` 45 of 45, 6696
  assertions, zero reports, the command queue and the seqlock among them (with the tests' allocator replacement
  compiled out, which the preset needs: card C guards it).
- 13:45 to 13:50: nine of the ten workspaces have real stages (GARAGE is the placeholder left). The lead rendered
  the frame in each at both densities at 2743869 (64 view tests, 2 failing: the frame's golden and SCOPE's page),
  sent the owner eleven pictures in two batches, and sent each session its corrections. The common fault: stages
  tested alone at 1100x700 collide or are cut off in the frame's smaller stage.
- **13:33 integration began**: K3b verified by the lead (272 of 272 tests at 45474be; `docs/handoffs/k3b.md`); D is
  the integrator; E, H, F, I (resumed local agents) and G (cloud) wire their models in behind switches.
- 13:32 to 13:34 the first three workspace stages landed: ENGINE (U2), SCOPE (U5), EXHAUST (U3). The lead rendered
  the frame in every workspace at both densities, sent five pictures to the owner, and sent each session its
  corrections (lay out for the frame's stage, text collisions, four red view tests, the frame's goldens).
- 13:26 the owner pressed ESC by accident, which stopped the four agents still running on the lead's machine (the
  keel, the shell, the split, the host and tools). All four working copies were clean and pushed: nothing was lost.
  They cannot be woken; at 13:30 the shell, the keel (as build and CI steward) and the host and tools were started
  again as cloud sessions on Opus 5.5, which an interrupt here cannot stop. The split was finished but for its
  handoff; the lead verifies it and writes that.
- 13:28 the owner pushed FunkGui's tag `v0.28.0` (on 19d06ab).
- 13:32 the branch did not build for about ten minutes (the frame's new interface against `app/Offline.cpp`); fixed
  by the lead, 45474be.
- **13:15 checkpoint M1, step 2, Windows, first part: passed.** The owner ran the application (run 45399202005) on a
  real Windows machine: the reference engine compiles in 2.11 s; WASAPI at 48 kHz with a 480-frame (10 ms) buffer;
  Direct3D 11 at 150 % scale; `--engine tone` held 60.0 frames a second for 789 frames with 1381 audio callbacks and
  exited cleanly. Printed, as expected: `ref/header.sc` refused (it reads a uniform of its own). Not said yet:
  whether the tone and the engine were heard, and how they sound.
- 13:19 FunkGui pull request 38 merged (card A, both stages). 13:19 U2 to U5 started (the workspaces). 13:12 U1: the
  `Workspace` interface, the look and the demo engine (`docs/handoffs/u1-interface.md`).
- 13:14 to 13:20 CI: Windows did not build at 0ee89f4 (`M_PI` in card D's tests: the keel defines
  `_USE_MATH_DEFINES`, 3cb314c); the kit's two golden tests failed where shaders were compiled (fixed, 9f8bb0d).
  On macOS 38 of card C's host tests crashed at exit (a static map of compiled programs destroyed after the JIT's own
  statics; fixed, e69ea09). The first run with all three fixes is 45402917543: not finished yet.
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
