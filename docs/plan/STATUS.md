# Status

*Kept by the lead. Times are UTC, 2026-11-24.*

| Package | State | Where it runs | Since | Notes |
|---|---|---|---|---|
| A: FunkGui desktop | **landed and merged 13:19** | local agent (finished) | 11:22 | Snipet/FunkGui pull request 38 merged to main at `19d06ab` (version 0.28.0): `DesktopHost` on SDL3, the menu overlay, file dialogs, clipboard, preferences, `FunkGui::gpu` without JUCE; CI green on Windows (44 tests, Direct3D 11), macOS (70, Metal), Linux (70, Vulkan on lavapipe). Tagged `v0.28.0` by the owner at 13:28. 40 new golden rows of `fg.desktop.menu` are candidates the lead has not adopted yet. Handoff: `docs/handoffs/a-funkgui-desktop.md` |
| K1, K2: the keel; now the build and CI steward | **landed** (handoff 13:00); steward running | cloud session `session_01LFAGgPuDhrgAnggttDA8nH` (since 13:30; the local agent was stopped by an accidental interrupt at 13:26, nothing lost) | 11:27 | R measured: the reference V8 is 2.3 us a sample, 22 to 23 % of a core here, 24.7 % on CI's M1; `scripts/ci-status.sh`; `tee-bench --profile`. The steward watches CI on the three systems, keeps it fast, runs ThreadSanitizer once |
| B: shell and application | handoff 14:12 (`docs/handoffs/w1-b-shell.md`): the frame over the simulator is the application | cloud session `session_01LFAGgPYzYByoQt6gue8ek5` | 11:30 | 51 shell tests; every workspace opens over the simulator and is pictured; builds on disk, File and Audio menus, GARAGE's library with curves made in the background; the atmosphere shaders registered when the build has them; `docs/checkpoints/M1.md` section 7, "the workshop". Not run: file dialogs, a device unplugged, a real wheel |
| J1: interface kit | **landed 12:04** | cloud session | 11:32 | the panel code ported (36 files, provenance in `ui/kit/SEED.tsv`), `RefPanel`, shaders compiled for Metal and SPIR-V; 29 headless tests; no shader drawn yet (no GPU path ran) |
| K3: the split | **landed**: K3a 12:30, K3b 13:20 (handoff written by the lead 13:43) | (finished) | 11:55 | every module of 02-contracts.md §6 carved; `sim/README.md` is the seam document; verified by the lead at 45474be: 272 of 272 tests, the null test identical to the bit, the cost 1.03 of the reference |
| C: host and tools | second slice landed; follow-ups as they come | cloud session `session_01LFAGgSy2V7eFxcA5Jr8M6s` | 11:55 | the fast dyno (`PredictService`: a V8's curve in 0.4 s, refined in 0.6); one `tee::Build`; array elements as telemetry names; `bakeMaps` carried; Windows' test crash fixed (389 of 390 there); **the guard clamps and counts a finite overshoot and cranks back after a true fault** (233c730); validation measures torque, power and port temperature. Open: 48 kHz for the ribbon; the large diesel's negative torques on a sweep; targets moved to the builds they are true of |
| I: vehicle | **integrated 14:06**; **X-007 fixed at its cause 16:14** (f9e26f1; `docs/handoffs/i-vehicle.md`) | local agent (finished 16:14) | 12:00 | behind `car.model` (0 the reference to the bit, 1 the new vehicle); 37 tests: a launch that spins in first and hooks up, a dog box's cut and blip with the ECU, a coast-down within 0.01 % of the analytic one, a stall; quarter miles within 8 % of the rule of thumb (front-drive 1.6: 17.25 s at 80 mph; rear-drive V8: 14.63 s at 103; all-wheel-drive turbo six: 12.07 s at 121); cost about 0.04 R. Waits on D: the env fields (it runs on dry asphalt at 1.2 kg/m^3 meanwhile), the chassis dyno as a driven mode |
| F: forced induction | **integrated 14:33, corrected 14:50** (`docs/handoffs/f-boost.md`), two things open | local agent (finished) | 12:00 | behind `boost.model`; 30 tests, all on the dyno with the **matched** turbo: a 2 L four makes 69 kPa at 3500 rpm and 68 at 5500 (the gate's spring is 70), the shaft at 104,000 to 106,000 rpm, 17.3 bar BMEP against 8.9 without; a Roots blower 29 kPa for 7.0 N m off the crank; a chop from 94 kPa surges without a blow-off valve and not with one. Its "cool port gas" was its own misreading (the bench under `holdSpeed` carries no load). **Open**: the housing's hook-up with G's network (a stand-in feeds it); **over budget**: 0.24 R against 0.12 R (Q has it) |
| E: valvetrain and induction | **integrated 15:00** (`docs/handoffs/e-valvetrain-intake.md`); the runner's ram, the float and the cost running | local agent (resumed 15:02) | 12:00 | behind `valve.model` and `intake.model`; 21 tests on the engine dyno: advancing a 280-degree cam gives +13.8 % torque at 1500 rpm on the V8 and moves the peak 240 rpm; two stages cross at 5500 with no dip; deactivation raises the manifold from 46 to 70 kPa; ITBs radiate 31 dB over a plenum's snorkel. **Open**: the runner's tuned frequency is right at 0.75 m and 27 % low at 0.45 m, and VE tops out near 0.9 (no ram); float is too mild; cost +0.03 to +0.07 R of 0.05. **Breathing targets missed**: a 6.2 L V8 257 kW (300 to 330), a litre superbike four 59 kW at 13,500 (125 to 145): mostly friction at piston speed (D) |
| H: combustion and ECU | **on the branch 15:55** (five fixes that had sat unpushed for an hour; verified on origin by the lead); handed off, resumable | local agent (finished 16:00) | 12:00 | cold start X-003 (b09a0a6: the V8 starts at 253 K in 0.35 s), idle (8096b73), traction never stalls (3649de4), a bang capped at 30 kPa, X-006 (3d00883), the full burn with Woschni walls and the spark at the half-burn (f8be3c8: default V8 +16 to 26 %, 12.6 bar; superbike 81 to 111 kW at 13,500 rpm). Open: idle manifold 21 to 22 kPa (25 to 50 wanted); port gas hotter (1433 K); two expected failures. Earlier: with `ecu.model` 1 the default build idles at 700 rpm within 20, a V8 with a mild or medium cam at 700 or 835, the large diesel at 710 within 10. Open: X-003 (no start at 253 K with both models on); the kept share becomes a combustion efficiency when `fuel.model` 1 implies D's Woschni walls; test 10 |
| D: mechanics and layouts; **the integrator** | **integrated 14:15** (`docs/handoffs/d-mechanics.md`); stays as integrator | cloud session `session_01LFAGgTFpHiA9YGryQgFytL` | 12:07 | every package's hooks in (bf0ec81; `sim/README.md`, "The hooks"), the names chosen, a model switch a package, the env and Scope fields; `engine.custom`; the engine dyno (holds a speed within 10 rpm) and `tel.engine.torque` (brake), power, IMEP/PMEP/FMEP/BMEP, VE; the default V8 warm at full throttle: BMEP 10.2 to 10.8 bar, VE 0.86 to 0.94; a V16 runs (1.7 R). Since: the env fields to the car, the car's values to the ECU, oil temperature, key-on rendering, X-001, `oxygenShare`, the energy balance as a test, **a Woschni wall heat staged** (`engine.wallHeat`; implied by `fuel.model` 1 by the lead's decision). Open: the torque lost at high piston speed (a litre four makes half what its air should give at 13,500 rpm); model switches under the fade |
| G: exhaust builder | standalone half **landed 13:08**; the wiring into the engine **built and green in its clone, not yet pushed** (15:55: an hour lost re-running the null test on every merge; the push rule changed, 01-rules.md §2) | cloud session `session_01LFAGgSkq1paVPmFHkoNNei` | 12:08 | 12 tests: bit-identical to the reference's exhaust at its defaults for all 17 layouts; passivity over 50 random topologies; conservation; headers, crossovers, resonators, mufflers, the valve measured in dB. Over its CPU budget for a typical dual system (0.31 R against 0.25 R) |
| J2: widgets, binding, the frame | **landed 12:47** | cloud session | 12:05 | eleven Canvas widgets (gauge, plot, map and curve editors among them), units, the binding to the real `tee::Build`, generated pages, a first frame of nine screens; 56 UI tests; contact sheets at density 1 and 2 |
| U1: the redesigned frame | the look's corrections landed 14:11 (a339b60); the Scope's generation fix and the fast dyno behind the ribbon running | cloud session `session_01LFAGgPEvM8vsbaJsuRqXAz` | 12:53 | values in the value ink; LOAD and OUTPUT (dB) apart; DRIVE's numerals inside the ticks, the car scaled; frame goldens free of other packages' stages; the demo hook; `stageFor` |
| U2: ENGINE, VALVES | corrections **landed 14:24** (20c7c5e); looked at by the lead: good | cloud session `session_01LFAGgQaoxE3tRGwUcwpU2G` | 13:19 | the section fills its panel with valves, cams and crank; the block full width; VALVES' editor with a second stage and the phaser's range, the flow bench; frame-stage and collision tests |
| U3: AIR, EXHAUST | handoff 13:52, corrections in at 14:00 (40e9b24) | cloud session `session_01LFAGgTSo5cKehJoa4H3k2A` | 13:19 | 20 tests; both stages fit the frame's real stage at both window sizes over six and seven builds; waits for per-pipe pressures from G |
| U4: TUNE, CAR, DYNO | handoff 13:45, corrections in at 13:56 (5d98532) | cloud session `session_01LFAGgPYn8yFr9MJ2vQvgRd` | 13:19 | stages fold for a small stage; a text-collision test at four sizes; `MapEdit` gained block selection |
| U5: GARAGE, SCOPE, atmosphere shaders | handoff 14:08 (`docs/handoffs/u5-garage-scope.md`); registering GARAGE and wiring the shaders' build line | cloud session `session_01LFAGgSKtL3Z967uRP5DGT9` | 13:19 | 19 tests; SCOPE registered; GARAGE built (cards, search, sort, parts apart, compare); four shaders (glow, flame, smoke, shimmer) compiled for SPIR-V and Metal and drawn once on lavapipe; Direct3D 11 compiles only on CI's Windows; no person has seen them on a GPU; no workspace calls them yet |
| Q: performance | **handed off 15:44** (`docs/handoffs/q-performance.md`); **the target is not met** | cloud session `session_01LFAGgPBEbLATNvMiRU1PGS` (idle; resumes for the pipes and boost) | 13:44 | exact work about 3 % (the listener 1.6, the pipes' caches 1.9), which pays for the hooks: the simulator is 1.016 of the reference. A twin-turbo V8 with a dual X system: 1.57 R today, 1.53 with exact changes (46 to 61 % of the owner's core), 1.41 with `TEE_FAST` (the valves in two passes: 10.3 %, not exact: bands move 0.3 to 1.3 dB; waits for the owner's ears), about 1.25 with the pipes as arrays too (exact, an estimate of 8 to 12 %, G's module). Also: the boost machine takes the engine's rate; X-005 fixed; the rule for measuring (7 runs, both orders, processor time) in `docs/measurements/q-rule.md`. Not done: the 8-run ensemble floor, boost's budget (0.24 R of 0.12), the hour's soak. For the compiler's author: FMinus's loop vectoriser takes no hot loop here (loops over modules whose methods write state); a `parallel { }` region costs about 1 us; two tasks' modules adjacent in memory false-share (40 %) |
| L: the listener, cameras, cabin, places | **handed off 15:50** (`docs/handoffs/l-listener.md`), integrated behind `sound.model` | cloud session `session_01LFAGgSygRoYnUE3ciPH5pw` | 14:15 | Doppler at a 30 m/s pass within 0.015 % of the formula; the ground comb's notches within 2 %; 5.1 to 6.0 dB a doubling of distance; the reference's five views as presets (exhaust within 1.1 dB, 2.9 dB rms over all pairs, worst 6.6); no click on a camera change. Next: loudness, soak, cost, the cabin |
| M: mechanical voices | **handed off 14:50** (`docs/handoffs/m-voices.md`); follow-ups running | cloud session `session_01LFAGgRQ16hRv6km8TyZqc5` | 14:15 | 24 tests: gear whine, dog-box rattle, belts and drives, fans, pump prime, DI ticks, lifters, float clatter, wastegate chatter and dump, two starters; 0.06 to 0.07 R of 0.08; voices 6 to 18 dB under the engine bay; rendering from key-on. Next: the real gate from boost, the intake's own sound from E, bike voices |
| T: two-strokes | package pushed; **its hook in `sim/engine.fm` not yet in** (parked as a patch since 15:33; the lead said yes at 16:00: T commits it itself) | cloud session `session_01LFAGgSsS8RRTWd4EdBgNmc` | 14:15 | the weak response to pipe length was the test's baseline pipe, not the model: the tuned speed now follows length (x0.8 / x1 / x1.25: 13,900 / 11,753 / 9,125 rpm against the returning wave's 13,267 / 10,614 / 8,491); on the pipe +48 % torque in 1000 rpm; three steps a cone suffice. Next: ports, reed, scavenging, power valve, twins and triples, integration |
| B2: motorcycles | **handed off 15:05** (`docs/handoffs/b2-bikes.md`) | cloud session `session_01LFAGgRiXaNaWQi2eaxy3Mi` | 14:15 | 15 tests behind `car.kind` Motorcycle; 13 bike engines whose firing intervals are exact in the running engine, 6 bikes; class quarter miles within their margins on class power curves (superbike 9.47 s at 150.5 mph); cost 0.014 R. With the real engine bikes make 50 to 65 % of class power (the breathing and friction question) |
| X: the whole program under test | running; five findings so far | cloud session `session_01LFAGgPEmBJHCJxEdvsiG1Y` | 14:15 | X-001 `engine.custom` at defaults never started (**fixed** by D); X-003 no cold start at 253 K with `fuel.model` and `ecu.model` both on (H); charge-air temperatures read 0 K with no machine (Q); **the output passes full scale on a turbo six's launch with `sound.model` 1 and the host's guard then kills the engine** (L: a ceiling; C: clamp and count, never restart for a finite overshoot) |
| D2: heavy diesels | running | cloud session `session_01LFAGgRbWtehbKx8ghnMmpB` | 15:02 | a compression-release brake through E's second exhaust event, an exhaust brake, injection as a tuner's thing, truck turbos, the eighteen-speed and a loaded rig, the air system's sounds |
| P: content, validation, the manual | running | cloud session `session_01LFAGgSHzU6yna9jNWSn93D` | 15:02 | about thirty archetype builds at a standard day with generic names; validation targets with sources, reported as the simulator's list of misses; the manual; help texts |
| N1: the numbers, engines that breathe by themselves | **started 16:04** | cloud session `session_01LFAGgS18iAMhTAJ6tm4wT6` | 16:04 | card `w2-n-calibration.md`: the loss ledger (volumetric efficiency, IMEP, pumping, friction, BMEP by class) first; then why every NA archetype lands at 11 to 13.4 bar BMEP whatever its head (mild two-valves too strong, tuned four-valves 25 to 30 % short on power), the rotary doubled, big-cam idles high |
| N2: the numbers, boosted engines | **started 16:04** | cloud session `session_01LFAGgSxnqsW2XUcFC39ruQ` | 16:04 | every boosted archetype 20 to 100 % too strong (2.0 turbo 29.5 bar for 22 to 25; kei 103 kW for 44 to 50); first question: what back pressure the cylinder sees while the turbine is fed by a stand-in; then boost that should taper, charge temperature, knock-limited spark, a supercharger's drive; the tractor's broken top-speed run |
| Held: W the wheel's feedback, R release | not started | | | W cannot be proven without the owner's wheel and shares files with the shell; R needs the owner's signing identities |

## Landed

- **16:14 X-007 fixed at its cause** (card I, f9e26f1, checked on origin): the car stepped the crank with the
  flywheel dial's inertia while the engine's torque carried the moving masses' inertia torque, which made energy
  (a heavy piston on a light flywheel ran away to 8809 rad/s); and a converter's torque could overshoot in one
  sample. Property tests hold both (`vehicle.passive`, `vehicle.passive.engine`); the automatic no longer hunts
  1-2-1 on spinning tyres. D's guard stays as the net. Open ends: X closes it on its own reduction; the motorcycle's
  crank step takes no `jCrank` yet (B2).
- **16:11 to 16:13 calibration's first measurements** (both on the branch): N1's ledger (`scripts/ledger.py`,
  3fa490a): the tuned four-valves trap 0.71 to 0.90 of their air where their kind traps 1.0 to 1.1, the mild
  economy four 0.98 to 1.06 and still rising at its limiter; gross IMEP per unit of air is the same on all three:
  pattern 1 is in the breathing. N2 (`docs/measurements/calibration/boost/backpressure.md`, 8575568): the 2.0 L
  turbo four's cylinder exhausts at 0.61 to 0.87 of its intake port's pressure while its turbine's housing stands
  at 0.98 to 1.32 of the plenum (the stand-in hands no pressure back): PMEP negative to 5000 rpm. G's housing
  wiring is the fix; G was interrupted at 16:10 and told to push at once. With e9a9748 that engine's torque is
  already in range (383 to 387 N m for 350 to 400); its power is not (247 kW for 155 to 230).
- 16:11 D: `tel.engine.veManifold` fixed (a4ca362); the ITB intake's manifold reading goes below zero absolute
  after a limiter lift (E's module: N1 has it). 16:12 U1: settings labels in full, the status line cut at words,
  LSN, one bike outline for CAR and DRIVE (b743f1a). 16:13 U2: amber geometry warnings in ENGINE's inspector
  (d2500aa).
- **16:09 decided: a QUALITY setting** (Full, 96 kHz inside, the default; Economy, 48 kHz inside: 0.49 of the cost,
  torque within 1.2 %, the top octave lost). Q's ceiling for a twin-turbo V8 with a dual X: about 1.49 R today, 1.40
  with every exact change (56 % of the owner's core if R is 40 % there, 42 % if 30 %: under 60 with no margin), 1.27
  with the valves in two passes (waits for the owner's ears). Exact work cannot make the target certain; Economy
  can (about 0.7 R). C builds it into `SimHost`, B stores it, U1 shows it; it ships once C has explained why the
  48 kHz comparison fails on macOS alone (0.116 against 0.05). Q also: the pipes as arrays are exact and save 3 to
  8 % on 8 to 12 cylinders (not the 8 to 12 estimated), waiting for G's wiring; the turbo's machine costs about
  0.17 R (not 0.24).
- 16:09 decided: the rotary gets its own losses under `fuel.model` 1 (D): the reference's rotary is about 1.3
  times strong before any boost (18 bar BMEP at 70 kPa for a real 14), and the turbo is matched on a four-stroke's
  air, half a rotary's (N2). Three separate terms with sources, checked at two points (boosted and not), no
  factor fitted to a number. D's guard: a diverged engine is put at rest, restarted and flags FLAG_FAULT (C and B
  show it); ENGINE's inspector will warn of geometry no engine has (U2). The steward's help sweep: 243 help texts.
- **16:06 the first finished CI job on every platform**: Linux 593 of 596 and Windows 593 of 596 (both at 08ae4f8),
  macOS 594 of 598 (6bbc346). The same three whole-program tests fail on all three, with the same numbers to the
  digit on Linux and Windows (small-four and turbo-six with every switch on: no cold start, X-003, and steps of 18
  to 23, X-006; `sound.model` flipped under load: a step of 23.2): all three predate H's fixes reaching the branch
  at 15:55, and X is re-running them. macOS alone fails the fast dyno's 48 kHz comparison (C's). With every switch
  on the turbo six costs 1.04 R there: 25 % of the Linux runner's core, 38 % of the Windows runner's.
- 15:58 D: `fuel.model` 1 implies the physical cylinder (Chen-Flynn friction and Woschni walls), and the 48 kHz
  define the fast dyno waited for (e9a9748). 16:02 U1's frame goldens green again with LISTEN's slot (67aadbb);
  16:04 LISTEN's label off the wheel (0edf438).
- **16:04 calibration started** (card N, two sessions): P's first full validation table (`docs/manual/validation.md`:
  582 targets on 30 archetypes, 190 missed) misses in patterns, not scatter: every engine that breathes by itself
  near the same BMEP whatever its head, every boosted engine 20 to 100 % too strong, the rotary doubled.
- **15:55 the push rule changed** (01-rules.md §2, "a long test never holds a push back twice"): the null test takes
  eight minutes and the branch moves forty commits in that time, so G's wiring, H's five fixes and T's hook had
  each sat unpushed for up to an hour. H's are on the branch (verified commit by commit on origin); G and T are
  pushing. D pushed the X-007 hardening (ae5c99b: no hang on an angle, no read off a table, a diverged engine put at
  rest and says so) and every model switch under the fade (02bf4db).
- 15:50 to 16:00: the lead's third look at the interface (rendered at 529af2a, software rasteriser): LISTEN, the
  two-stroke ENGINE, port timing and expansion chamber, the motorcycle on CAR and DRIVE all read well; faults sent to
  their owners: the settings page cuts its unit labels to three letters and three frame goldens are red since the
  eleventh rail slot (U1), a label on a wheel (U5), a label under a trace (U3). Six pictures sent to the owner.
- 15:53 CI: each platform queues on its own since 14:24, so a run reads "cancelled" when any platform's job was
  replaced; the first macOS job (6bbc346) and Windows job (08ae4f8) to reach their tests were in them at 15:53. Linux's
  newest finished job (e6eba87) failed in a reload test of the host and five of SCOPE's.

- **15:33 the performance figures of 15:03 are withdrawn**: Q re-measured and the valves' early exit saves nothing
  (the 8 % was the cloud host's noise); exact changes total about 3 %, not 11. The 60 % target for a twin-turbo V8
  with a dual exhaust is therefore not met by exact work (46 to 61 % of the owner's core); it is met with the
  valves in two passes (42 to 56 %), which waits for the owner's ears. The lead had told the owner the target was
  met on paper.
- 15:25 to 15:33: the listener integrated with its ceiling; the engine brake retards and barks (D2); H closed the
  cold start, the cuts and the idle, and ran traction; thirty archetype builds run (P); X-007 reduced (a converter
  running the crank away: a hang or a crash in the shipped build: with I and D); the steward's 79 failures withdrawn
  (a stale configure); CI has no finished macOS or Windows job among its last 60 runs.
- **15:14 builds for the owner's tests**: macOS run 45419507610 (8b4eeca), Windows run 45420506181 (5f79fe0): the
  start-key fix, the live Scope, the fast ribbon, ten workspaces. They lack the telemetry-array lookup (3661785) and
  the guard's clamp (233c730).
- 15:07 the owner's presets checked against the originals: `v8`, `v10` and `large-diesel` carry the cam dial at
  0.9999 and the air at 233 K in the reference project itself: the conversion is faithful, and the V8's lumpy
  1050 rpm idle is its race cam's. Stock targets move to builds with stock hardware. The lead had called the idle
  a fault and the 700 N m a settled figure before checking the build.
- 15:05 to 15:16: bikes handed off; the shell offers telemetry array elements by name (every per-wheel, per-shaft
  and per-cylinder value was a dash in the application until then); the guard's clamp; D's Woschni walls;
  the contract test green in D's tree at e6eba87 (CI has not caught up: its newest finished jobs are of 14:47).
- **15:00 the valvetrain and induction integrated** (card E): five of the six physics packages are in the one
  program; the exhaust builder (G) is the one outstanding. 15:02 D2 (heavy diesels) and P (content, validation, the
  manual) started. 14:50 mechanical voices handed off whole. 14:52 the first bike launches.
- 14:55 to 15:00 the whole-program tester's findings: no cold start at 253 K with both of H's models on; the
  output passing full scale with `sound.model` 1 and the host's guard then stopping the engine. Both routed.
- 15:00 the breathing miss: a litre superbike four makes 59 kW at 13,500 rpm (125 to 145 expected), a 6.2 L V8
  257 kW (300 to 330): at the bike's VE of 0.75 the torque is half what the air should give: the integrator is
  splitting it into indicated, pumping and friction work; E is on the runner's ram.
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
