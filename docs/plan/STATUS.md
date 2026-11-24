# Status

*Kept by the lead. Times are UTC, 2026-11-24.*

| Package | State | Where it runs | Since | Notes |
|---|---|---|---|---|
| A: FunkGui desktop | **landed and merged 13:19** | local agent (finished) | 11:22 | Snipet/FunkGui pull request 38 merged to main at `19d06ab` (version 0.28.0): `DesktopHost` on SDL3, the menu overlay, file dialogs, clipboard, preferences, `FunkGui::gpu` without JUCE; CI green on Windows (44 tests, Direct3D 11), macOS (70, Metal), Linux (70, Vulkan on lavapipe). Tagged `v0.28.0` by the owner at 13:28. 40 new golden rows of `fg.desktop.menu` are candidates the lead has not adopted yet. Handoff: `docs/handoffs/a-funkgui-desktop.md` |
| K1, K2: the keel; now the build and CI steward | **landed** (handoff 13:00); steward running | cloud session `session_01LFAGgPuDhrgAnggttDA8nH` (since 13:30; the local agent was stopped by an accidental interrupt at 13:26, nothing lost) | 11:27 | R measured: the reference V8 is 2.3 us a sample, 22 to 23 % of a core here, 24.7 % on CI's M1; `scripts/ci-status.sh`; `tee-bench --profile`. The steward watches CI on the three systems, keeps it fast, runs ThreadSanitizer once |
| B: shell and application | handoff 14:12 (`docs/handoffs/w1-b-shell.md`): the frame over the simulator is the application | cloud session `session_01LFAGgPYzYByoQt6gue8ek5` | 11:30 | 51 shell tests; every workspace opens over the simulator and is pictured; builds on disk, File and Audio menus, GARAGE's library with curves made in the background; the atmosphere shaders registered when the build has them; `docs/checkpoints/M1.md` section 7, "the workshop". Not run: file dialogs, a device unplugged, a real wheel |
| J1: interface kit | **landed 12:04** | cloud session | 11:32 | the panel code ported (36 files, provenance in `ui/kit/SEED.tsv`), `RefPanel`, shaders compiled for Metal and SPIR-V; 29 headless tests; no shader drawn yet (no GPU path ran) |
| K3: the split | **landed**: K3a 12:30, K3b 13:20 (handoff written by the lead 13:43) | (finished) | 11:55 | every module of 02-contracts.md §6 carved; `sim/README.md` is the seam document; verified by the lead at 45474be: 272 of 272 tests, the null test identical to the bit, the cost 1.03 of the reference |
| C: host and tools | second slice: **the fast dyno landed 14:15** (`tee::PredictService`); Windows' test crash open | cloud session `session_01LFAGgSy2V7eFxcA5Jr8M6s` | 11:55 | the ribbon's curve of the real V8: first in 0.33 to 0.41 s, refined in 0.61 to 0.64 s on four cores, within 0.8 % mean (4.4 % worst) of points held five minutes; the engine held warm and the exhaust's temperatures settled (without that the curve moves 27 % at 6000 rpm cold against hot); one `tee::Build` (the split's is `EngineBuild`); `tee-render --build --scenario`; validation measures torque and power (V8: 700 N m, 346 kW). Windows' test crash **fixed** (fb69de2): 388 of 390 tests pass there at 7893861; one of its Predict tests is timing-dependent on Windows (open) |
| I: vehicle | **integrated 14:06** (`docs/handoffs/i-vehicle.md`) | local agent (finished) | 12:00 | behind `car.model` (0 the reference to the bit, 1 the new vehicle); 37 tests: a launch that spins in first and hooks up, a dog box's cut and blip with the ECU, a coast-down within 0.01 % of the analytic one, a stall; quarter miles within 8 % of the rule of thumb (front-drive 1.6: 17.25 s at 80 mph; rear-drive V8: 14.63 s at 103; all-wheel-drive turbo six: 12.07 s at 121); cost about 0.04 R. Waits on D: the env fields (it runs on dry asphalt at 1.2 kg/m^3 meanwhile), the chassis dyno as a driven mode |
| F: forced induction | **integrated 14:33, corrected 14:50** (`docs/handoffs/f-boost.md`), two things open | local agent (finished) | 12:00 | behind `boost.model`; 30 tests, all on the dyno with the **matched** turbo: a 2 L four makes 69 kPa at 3500 rpm and 68 at 5500 (the gate's spring is 70), the shaft at 104,000 to 106,000 rpm, 17.3 bar BMEP against 8.9 without; a Roots blower 29 kPa for 7.0 N m off the crank; a chop from 94 kPa surges without a blow-off valve and not with one. Its "cool port gas" was its own misreading (the bench under `holdSpeed` carries no load). **Open**: the housing's hook-up with G's network (a stand-in feeds it); **over budget**: 0.24 R against 0.12 R (Q has it) |
| E: valvetrain and induction | standalone half **landed 12:38**; **integrating since 13:33** | local agent (resumed 13:33) | 12:00 | 14 tests |
| H: combustion and ECU | **integrated 14:50** (`docs/handoffs/h-combustion-ecu.md`); the idle and the traction test running | local agent (resumed 14:55) | 12:00 | behind `fuel.model` and `ecu.model`; the null test 59 of 59; torque peaks at lambda 0.90; regular knocks from 27.5 degrees of advance and E85 not up to 40; a spark-cut limiter makes 638 bangs in 3 s and a clean overrun none; cost 0.02 R of 0.06. **Open**: the warm idle lopes 900 to 1500 rpm and stalls at 800 (the validation's loud miss); half the charge burned at 0.3 degrees after top at best torque (the cylinder's wall heat, with D); test 10 (traction) not yet run; the 10-minute soak not run |
| D: mechanics and layouts; **the integrator** | **integrated 14:15** (`docs/handoffs/d-mechanics.md`); stays as integrator | cloud session `session_01LFAGgTFpHiA9YGryQgFytL` | 12:07 | every package's hooks in (bf0ec81; `sim/README.md`, "The hooks"), the names chosen, a model switch a package, the env and Scope fields; `engine.custom`; the engine dyno (holds a speed within 10 rpm) and `tel.engine.torque` (brake), power, IMEP/PMEP/FMEP/BMEP, VE; the default V8 warm at full throttle: BMEP 10.2 to 10.8 bar, VE 0.86 to 0.94; a V16 runs (1.7 R); default engines with all hooks 1.05 R |
| G: exhaust builder | standalone half **landed 13:08**; **integrating since 13:33** | cloud session `session_01LFAGgSkq1paVPmFHkoNNei` | 12:08 | 12 tests: bit-identical to the reference's exhaust at its defaults for all 17 layouts; passivity over 50 random topologies; conservation; headers, crossovers, resonators, mufflers, the valve measured in dB. Over its CPU budget for a typical dual system (0.31 R against 0.25 R) |
| J2: widgets, binding, the frame | **landed 12:47** | cloud session | 12:05 | eleven Canvas widgets (gauge, plot, map and curve editors among them), units, the binding to the real `tee::Build`, generated pages, a first frame of nine screens; 56 UI tests; contact sheets at density 1 and 2 |
| U1: the redesigned frame | the look's corrections landed 14:11 (a339b60); the Scope's generation fix and the fast dyno behind the ribbon running | cloud session `session_01LFAGgPEvM8vsbaJsuRqXAz` | 12:53 | values in the value ink; LOAD and OUTPUT (dB) apart; DRIVE's numerals inside the ticks, the car scaled; frame goldens free of other packages' stages; the demo hook; `stageFor` |
| U2: ENGINE, VALVES | corrections **landed 14:24** (20c7c5e); looked at by the lead: good | cloud session `session_01LFAGgQaoxE3tRGwUcwpU2G` | 13:19 | the section fills its panel with valves, cams and crank; the block full width; VALVES' editor with a second stage and the phaser's range, the flow bench; frame-stage and collision tests |
| U3: AIR, EXHAUST | handoff 13:52, corrections in at 14:00 (40e9b24) | cloud session `session_01LFAGgTSo5cKehJoa4H3k2A` | 13:19 | 20 tests; both stages fit the frame's real stage at both window sizes over six and seven builds; waits for per-pipe pressures from G |
| U4: TUNE, CAR, DYNO | handoff 13:45, corrections in at 13:56 (5d98532) | cloud session `session_01LFAGgPYn8yFr9MJ2vQvgRd` | 13:19 | stages fold for a small stage; a text-collision test at four sizes; `MapEdit` gained block selection |
| U5: GARAGE, SCOPE, atmosphere shaders | handoff 14:08 (`docs/handoffs/u5-garage-scope.md`); registering GARAGE and wiring the shaders' build line | cloud session `session_01LFAGgSKtL3Z967uRP5DGT9` | 13:19 | 19 tests; SCOPE registered; GARAGE built (cards, search, sort, parts apart, compare); four shaders (glow, flame, smoke, shimmer) compiled for SPIR-V and Metal and drawn once on lavapipe; Direct3D 11 compiles only on CI's Windows; no person has seen them on a GPU; no workspace calls them yet |
| Q: performance | running; the profile (14:04) and the lever table (14:29) landed | cloud session `session_01LFAGgPBEbLATNvMiRU1PGS` | 13:44 | R is 1.79 us a sample on a quiet core; FMinus's own levers give nothing (-O2 stays); the listener's compressor and resampler 2.5 % of R faster, to the bit; **the engine is chaotic**, so non-exact changes are judged against an ensemble of perturbed runs and live behind one switch (the lead's rule, integration-queue.md); the valves' solve in two passes would give 10 to 14 % |
| L: the listener, cameras, cabin, places | running; first tests **14:45**, standalone | cloud session `session_01LFAGgSygRoYnUE3ciPH5pw` | 14:15 | Doppler at a 30 m/s pass within 0.015 % of the formula; the ground comb's notches within 2 %; 5.1 to 6.0 dB a doubling of distance; the reference's five views as presets (exhaust within 1.1 dB, 2.9 dB rms over all pairs, worst 6.6); no click on a camera change. Next: loudness, soak, cost, the cabin |
| M: mechanical voices | running; first voices **in the engine 14:37** behind `voices.model` | cloud session `session_01LFAGgRQ16hRv6km8TyZqc5` | 14:15 | gear whine within 0.5 % of each pair's tooth frequency in six gears, straight-cut 12 dB over helical, silent on the bench; a dog box's rattle in neutral from the crank's ripple; alternator, pumps, cam drives and blower gears at their orders; electric and viscous fans. Wants from D: rendering at key-on, so the pump's prime is heard |
| T: two-strokes | running; the tuned length **answered 14:37** | cloud session `session_01LFAGgSsS8RRTWd4EdBgNmc` | 14:15 | the weak response to pipe length was the test's baseline pipe, not the model: the tuned speed now follows length (x0.8 / x1 / x1.25: 13,900 / 11,753 / 9,125 rpm against the returning wave's 13,267 / 10,614 / 8,491); on the pipe +48 % torque in 1000 rpm; three steps a cone suffice. Next: ports, reed, scavenging, power valve, twins and triples, integration |
| B2: motorcycles | running | cloud session `session_01LFAGgRiXaNaWQi2eaxy3Mi` | 14:15 | the bike behind `Car`'s calls (wheelie-limited launch, chain and its lash, slipper clutch, quickshifter and auto-blipper); bike engines as builds through layouts-as-data |
| X: the whole program under test | running; first finding 14:36 | cloud session `session_01LFAGgPEmBJHCJxEdvsiG1Y` | 14:15 | X-001 (to D): `engine.custom` on with the Spec's default geometry never starts. Harness and fuzzer being pushed |
| Held: D2 heavy diesels, P content and manual, W the wheel's feedback, R release | not started | | | D2 needs E's auxiliary exhaust event, G's flap slot, F's vanes and H's injection fields, all mid-integration; P needs the Spec's names to stop moving; W cannot be proven without the owner's wheel and shares files with the shell; R needs the owner's signing identities |

## Landed

- **14:50 combustion and the ECU integrated** (card H). **14:37 Windows ran tests for the first time**: 388 of 390
  at 7893861 (card C's fix of its tests' allocator replacement); the two failures: the contract test (E's five new
  fields, every platform) and one timing-dependent Predict test (C). Linux and macOS at 6c190f9: 389 of 390, the
  contract test alone. So the branch builds and runs tests on all three systems, one lint fix from green.
- 14:45 the shell: a tap of Return did not start the simulator (the start key was held 0.15 s; enough for the
  reference only): fixed, 09553a5. The artefact of run 45410462346 has the fault: hold Return there.
- 14:45 three of the four shipped builds stand in air at 233 K (the reference's presets): figures quoted from them
  are cold-day figures (the V8's 700 N m); the owner was asked whether -40 is meant.
- 14:50 F confirmed its error and corrected its tests: the matched turbo makes its spring on the dyno (69 and 68
  kPa); it had read the port gas on the bench under `holdSpeed`, which reports a full throttle and carries almost
  no load (brake torque -2 to 2 N m). **For everyone: full load is the engine dyno (`MODE_DYNO_ENGINE`), not
  `holdSpeed` on the bench.**
- 14:38 Q: `Valve::flow` leaves its three-pass loops when a pass changes nothing: to the bit, about 8 % of R. The
  lead's rule: Q commits what it has proved exact to the bit in any module itself, telling the owner.
- **14:45 the "exhaust gas too cool" alarm of 14:33 is withdrawn**: the integrator measured `exh.tPort` at 1100 to
  1330 K at full load (work 36 % of the fuel, walls 11 % plus the reference's 20 % cut, exhaust 38 %). The boost
  package's 450 to 560 K was not reproduced; it is checking what it read and re-running its matched turbine. The
  lead had passed the claim to the owner as a finding before it was checked.
- 14:31 GARAGE registered: all ten workspaces are real. The atmosphere shaders compile into the application
  (Direct3D 11 for the first time on CI's next Windows job). 14:29 Q: FMinus's own levers give nothing; the engine
  is chaotic; the lead's rule for non-exact optimisations (integration-queue.md).
- **14:33 the boost package integrated** (card F). It found that the reference's port gas is 450 to 560 K at full
  throttle where a real engine's is 900 to 1100 K, so a matched turbo makes little boost: the integrator and the
  combustion package are doing the cylinder's energy balance to find where the heat goes. The lead's decision in
  principle: with the new models on the gas temperature must be physical and validated; model 0 stays the reference.
- 14:30 the lead rendered the interface again at 20c7c5e (89 of 89 view tests): ENGINE, VALVES and CAR are fixed;
  six more pictures to the owner. GARAGE is built and pictured alone; not registered in the frame yet.
- 14:24 card C: `tests/host/Alloc.cpp`'s replaced `new` and `delete` compiled on Linux and macOS only (fb69de2), the
  suspected cause of Windows' test crash; unconfirmed until a Windows job runs.
- **14:11 to 14:15: the integrator's hooks for every package landed** (bf0ec81) with the names chosen (`drivenSlip`,
  `speedNow`, `gearNow`, `upshifts`, `cutRequest`, `blipRequest`; `oxygenShare`; `Boost::control16`; `Exhaust::popAt`),
  the mechanics integrated (the engine dyno and brake torque among them), and card C's fast dyno for the ribbon
  measured (first curve 0.4 s, refined 0.6 s). E, H, F and G were told to fill their stubs. The shell handed off.
  CI run 45410462346 (6651274) has the application with the frame's Scope and dyno on all three systems; its
  Linux and macOS test builds failed on a renamed type in one test header (fixed since).
- 14:15 the owner asked whether more could run: five packages started (L, M, T, B2, X), all cloud sessions on
  Opus 5.5; four held with reasons (the table). 14:10 macOS: CI's first complete run with the fixes (45407345444 at
  9f23b8b) builds, the exit crashes are gone, 351 of 353 pass; its `TheEngineEngine-macos-arm64` is the artefact
  for the owner's Mac test; the commands were given.
- **14:06 the vehicle integrated** (card I), the first physics package in the one program: `car.model` 1; the null
  test green; its Spec and telemetry names are the ones the CAR and DRIVE workspaces already read.
- 14:05 at 4c11901 the lead re-ran the null test in a full tree: all ten groups pass (the reference to the bit after
  the integrator's first commits).
- 13:56 to 14:02: U3, U4 and U5 pushed the review's corrections (stages laid out and tested at the frame's real
  stage rects, collision tests, labels); U2 handed off and has the same corrections to do; the shell made the frame
  the application and can picture any workspace. Open: `ui.build.frame` fails since the mechanics fields landed (the
  kit's pages do not take array elements such as `engine.bankAngle[0]`: J2's session has it); `ui.frame.ribbon`'s
  golden (U1). CI: run 45409354272 was replaced before it ran; 45407345444 (9f23b8b) is the first macOS run with
  the exit-crash fix and was still running at 14:04.
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
