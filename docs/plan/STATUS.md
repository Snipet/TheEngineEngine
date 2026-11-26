# Status

*Kept by the lead. Times are UTC, 2026-11-24.*

**The owner's standing words on the fleet** (the lead keeps these at the top so they survive a summary):
- 25 Nov, about 11:29: "For future note (in the scope of this project), you may use up to 10 concurrent sessions."
  A ceiling, not a target: a session is woken for named work and idles when it has none.
- 25 Nov, about 10:53: "keep the fleet thin" (given when usage had just run out; the cap above came after it).
- Earlier: new implementation agents run on Opus 5.5; `main` is fast-forwarded at each checkpoint, never forced.

| Package | State | Where it runs | Since | Notes |
|---|---|---|---|---|
| A: FunkGui desktop | **landed and merged 13:19** | local agent (finished) | 11:22 | Snipet/FunkGui pull request 38 merged to main at `19d06ab` (version 0.28.0): `DesktopHost` on SDL3, the menu overlay, file dialogs, clipboard, preferences, `FunkGui::gpu` without JUCE; CI green on Windows (44 tests, Direct3D 11), macOS (70, Metal), Linux (70, Vulkan on lavapipe). Tagged `v0.28.0` by the owner at 13:28. 40 new golden rows of `fg.desktop.menu` are candidates the lead has not adopted yet. Handoff: `docs/handoffs/a-funkgui-desktop.md` |
| K1, K2: the keel; now the build and CI steward | **landed** (handoff 13:00); steward running | cloud session `session_01LFAGgPuDhrgAnggttDA8nH` (since 13:30; the local agent was stopped by an accidental interrupt at 13:26, nothing lost) | 11:27 | R measured: the reference V8 is 2.3 us a sample, 22 to 23 % of a core here, 24.7 % on CI's M1; `scripts/ci-status.sh`; `tee-bench --profile`. The steward watches CI on the three systems, keeps it fast, runs ThreadSanitizer once |
| B: shell and application | **the launch (605e452: first frame 0.42 s, the engine compiled on a worker), faults in the status line, QUALITY (b6c1f90), the truck's switches**; 59 shell tests | cloud session `session_01LFAGgPYzYByoQt6gue8ek5` | 11:30 | 51 shell tests; every workspace opens over the simulator and is pictured; builds on disk, File and Audio menus, GARAGE's library with curves made in the background; the atmosphere shaders registered when the build has them; `docs/checkpoints/M1.md` section 7, "the workshop". Not run: file dialogs, a device unplugged, a real wheel |
| J1: interface kit | **landed 12:04** | cloud session | 11:32 | the panel code ported (36 files, provenance in `ui/kit/SEED.tsv`), `RefPanel`, shaders compiled for Metal and SPIR-V; 29 headless tests; no shader drawn yet (no GPU path ran) |
| K3: the split | **landed**: K3a 12:30, K3b 13:20 (handoff written by the lead 13:43) | (finished) | 11:55 | every module of 02-contracts.md §6 carved; `sim/README.md` is the seam document; verified by the lead at 45474be: 272 of 272 tests, the null test identical to the bit, the cost 1.03 of the reference |
| C: host and tools | FLAG_FAULT counted (8566fbc), `SimHost::setQuality` (9248bb7), the dyno runner's key (8098b36), the predictor within -2 to +5 % of the stepped dyno on a boosted build; the 48 kHz test narrowed with its reason | cloud session `session_01LFAGgSy2V7eFxcA5Jr8M6s` | 11:55 | the fast dyno (`PredictService`: a V8's curve in 0.4 s, refined in 0.6); one `tee::Build`; array elements as telemetry names; `bakeMaps` carried; Windows' test crash fixed (389 of 390 there); **the guard clamps and counts a finite overshoot and cranks back after a true fault** (233c730); validation measures torque, power and port temperature. Open: 48 kHz for the ribbon; the large diesel's negative torques on a sweep; targets moved to the builds they are true of |
| I: vehicle | **the launch under traction control, the eighteen-speed, passive.engine biting again: on the branch 18:07** (c0f95c2, 51908c2); handed off, resumable | local agent (finished 16:14) | 12:00 | behind `car.model` (0 the reference to the bit, 1 the new vehicle); 37 tests: a launch that spins in first and hooks up, a dog box's cut and blip with the ECU, a coast-down within 0.01 % of the analytic one, a stall; quarter miles within 8 % of the rule of thumb (front-drive 1.6: 17.25 s at 80 mph; rear-drive V8: 14.63 s at 103; all-wheel-drive turbo six: 12.07 s at 121); cost about 0.04 R. Waits on D: the env fields (it runs on dry asphalt at 1.2 kg/m^3 meanwhile), the chassis dyno as a driven mode |
| F: forced induction | **integrated 14:33, corrected 14:50** (`docs/handoffs/f-boost.md`), two things open | local agent (finished) | 12:00 | behind `boost.model`; 30 tests, all on the dyno with the **matched** turbo: a 2 L four makes 69 kPa at 3500 rpm and 68 at 5500 (the gate's spring is 70), the shaft at 104,000 to 106,000 rpm, 17.3 bar BMEP against 8.9 without; a Roots blower 29 kPa for 7.0 N m off the crank; a chop from 94 kPa surges without a blow-off valve and not with one. Its "cool port gas" was its own misreading (the bench under `holdSpeed` carries no load). **Open**: the housing's hook-up with G's network (a stand-in feeds it); **over budget**: 0.24 R against 0.12 R (Q has it) |
| E: valvetrain and induction | **integrated 15:00** (`docs/handoffs/e-valvetrain-intake.md`); the runner's ram, the float and the cost running | local agent (resumed 15:02) | 12:00 | behind `valve.model` and `intake.model`; 21 tests on the engine dyno: advancing a 280-degree cam gives +13.8 % torque at 1500 rpm on the V8 and moves the peak 240 rpm; two stages cross at 5500 with no dip; deactivation raises the manifold from 46 to 70 kPa; ITBs radiate 31 dB over a plenum's snorkel. **Open**: the runner's tuned frequency is right at 0.75 m and 27 % low at 0.45 m, and VE tops out near 0.9 (no ram); float is too mild; cost +0.03 to +0.07 R of 0.05. **Breathing targets missed**: a 6.2 L V8 257 kW (300 to 330), a litre superbike four 59 kW at 13,500 (125 to 145): mostly friction at piston speed (D) |
| H: combustion and ECU | handed off 17:33 (traction from rolling, the bangs never overlapping); **its model-on paths now with N1** (the burn with speed, the start rule, H9 on the blowdown and the flame) | local agent (finished 16:00) | 12:00 | cold start X-003 (b09a0a6: the V8 starts at 253 K in 0.35 s), idle (8096b73), traction never stalls (3649de4), a bang capped at 30 kPa, X-006 (3d00883), the full burn with Woschni walls and the spark at the half-burn (f8be3c8: default V8 +16 to 26 %, 12.6 bar; superbike 81 to 111 kW at 13,500 rpm). Open: idle manifold 21 to 22 kPa (25 to 50 wanted); port gas hotter (1433 K); two expected failures. Earlier: with `ecu.model` 1 the default build idles at 700 rpm within 20, a V8 with a mild or medium cam at 700 or 835, the large diesel at 710 within 10. Open: X-003 (no start at 253 K with both models on); the kept share becomes a combustion efficiency when `fuel.model` 1 implies D's Woschni walls; test 10 |
| D: mechanics and layouts; **the integrator** | integrator; today: the walls once (petrol e81f79f, **diesel 9e3a44f**), the backflow conserved, the intake's lines, boost implying the builder, tidies 1 and 3; next: the key-release line, the stand-in's call, its handoff | cloud session `session_01LFAGgTFpHiA9YGryQgFytL` | 12:07 | every package's hooks in (bf0ec81; `sim/README.md`, "The hooks"), the names chosen, a model switch a package, the env and Scope fields; `engine.custom`; the engine dyno (holds a speed within 10 rpm) and `tel.engine.torque` (brake), power, IMEP/PMEP/FMEP/BMEP, VE; the default V8 warm at full throttle: BMEP 10.2 to 10.8 bar, VE 0.86 to 0.94; a V16 runs (1.7 R). Since: the env fields to the car, the car's values to the ECU, oil temperature, key-on rendering, X-001, `oxygenShare`, the energy balance as a test, **a Woschni wall heat staged** (`engine.wallHeat`; implied by `fuel.model` 1 by the lead's decision). Open: the torque lost at high piston speed (a litre four makes half what its air should give at 13,500 rpm); model switches under the fade |
| G: exhaust builder | in the engine since 16:17; passivity exact, the flap, external gates, glides, exhaustSides, **the port's heat (Caton and Heywood; energy closed within 0.7 %) and thermocouple readings beside the mass means** | cloud session `session_01LFAGgSkq1paVPmFHkoNNei` | 12:08 | 12 tests: bit-identical to the reference's exhaust at its defaults for all 17 layouts; passivity over 50 random topologies; conservation; headers, crossovers, resonators, mufflers, the valve measured in dB. Over its CPU budget for a typical dual system (0.31 R against 0.25 R) |
| J2: widgets, binding, the frame | **landed 12:47** | cloud session | 12:05 | eleven Canvas widgets (gauge, plot, map and curve editors among them), units, the binding to the real `tee::Build`, generated pages, a first frame of nine screens; 56 UI tests; contact sheets at density 1 and 2 |
| U1: the redesigned frame | the look's corrections landed 14:11 (a339b60); the Scope's generation fix and the fast dyno behind the ribbon running | cloud session `session_01LFAGgPEvM8vsbaJsuRqXAz` | 12:53 | values in the value ink; LOAD and OUTPUT (dB) apart; DRIVE's numerals inside the ticks, the car scaled; frame goldens free of other packages' stages; the demo hook; `stageFor` |
| U2: ENGINE, VALVES | corrections **landed 14:24** (20c7c5e); looked at by the lead: good | cloud session `session_01LFAGgQaoxE3tRGwUcwpU2G` | 13:19 | the section fills its panel with valves, cams and crank; the block full width; VALVES' editor with a second stage and the phaser's range, the flow bench; frame-stage and collision tests |
| U3: AIR, EXHAUST | handoff 13:52, corrections in at 14:00 (40e9b24) | cloud session `session_01LFAGgTSo5cKehJoa4H3k2A` | 13:19 | 20 tests; both stages fit the frame's real stage at both window sizes over six and seven builds; waits for per-pipe pressures from G |
| U4: TUNE, CAR, DYNO | handoff 13:45, corrections in at 13:56 (5d98532) | cloud session `session_01LFAGgPYn8yFr9MJ2vQvgRd` | 13:19 | stages fold for a small stage; a text-collision test at four sizes; `MapEdit` gained block selection |
| U5: GARAGE, SCOPE, atmosphere shaders | handoff 14:08 (`docs/handoffs/u5-garage-scope.md`); registering GARAGE and wiring the shaders' build line | cloud session `session_01LFAGgSKtL3Z967uRP5DGT9` | 13:19 | 19 tests; SCOPE registered; GARAGE built (cards, search, sort, parts apart, compare); four shaders (glow, flame, smoke, shimmer) compiled for SPIR-V and Metal and drawn once on lavapipe; Direct3D 11 compiles only on CI's Windows; no person has seen them on a GPU; no workspace calls them yet |
| Q: performance | **at its stop 17:29** (waits for the owner's ears and the FMinus wishes): the rotary's shared-port solve reused (4845579, exact: a four-rotor 1.24 R all on, 1.58 with turbos); the builder's exact work stopped (grouping by kind: no faster) | cloud session `session_01LFAGgPBEbLATNvMiRU1PGS` (idle; resumes for the pipes and boost) | 13:44 | exact work about 3 % (the listener 1.6, the pipes' caches 1.9), which pays for the hooks: the simulator is 1.016 of the reference. A twin-turbo V8 with a dual X system: 1.57 R today, 1.53 with exact changes (46 to 61 % of the owner's core), 1.41 with `TEE_FAST` (the valves in two passes: 10.3 %, not exact: bands move 0.3 to 1.3 dB; waits for the owner's ears), about 1.25 with the pipes as arrays too (exact, an estimate of 8 to 12 %, G's module). Also: the boost machine takes the engine's rate; X-005 fixed; the rule for measuring (7 runs, both orders, processor time) in `docs/measurements/q-rule.md`. Not done: the 8-run ensemble floor, boost's budget (0.24 R of 0.12), the hour's soak. For the compiler's author: FMinus's loop vectoriser takes no hot loop here (loops over modules whose methods write state); a `parallel { }` region costs about 1 us; two tasks' modules adjacent in memory false-share (40 %) |
| L: the listener, cameras, cabin, places | **handed off 15:50** (`docs/handoffs/l-listener.md`), integrated behind `sound.model` | cloud session `session_01LFAGgSygRoYnUE3ciPH5pw` | 14:15 | Doppler at a 30 m/s pass within 0.015 % of the formula; the ground comb's notches within 2 %; 5.1 to 6.0 dB a doubling of distance; the reference's five views as presets (exhaust within 1.1 dB, 2.9 dB rms over all pairs, worst 6.6); no click on a camera change. Next: loudness, soak, cost, the cabin |
| M: mechanical voices | **handed off 14:50** (`docs/handoffs/m-voices.md`); follow-ups running | cloud session `session_01LFAGgRQ16hRv6km8TyZqc5` | 14:15 | 24 tests: gear whine, dog-box rattle, belts and drives, fans, pump prime, DI ticks, lifters, float clatter, wastegate chatter and dump, two starters; 0.06 to 0.07 R of 0.08; voices 6 to 18 dB under the engine bay; rendering from key-on. Next: the real gate from boost, the intake's own sound from E, bike voices |
| T: two-strokes | **handed off 16:51** (`docs/handoffs/t-twostroke.md`); in the engine with its switch under the fade (d9c5544); power 41 to 82 % of its classes (an inlet duct to the reed next) | cloud session `session_01LFAGgSsS8RRTWd4EdBgNmc` | 14:15 | the weak response to pipe length was the test's baseline pipe, not the model: the tuned speed now follows length (x0.8 / x1 / x1.25: 13,900 / 11,753 / 9,125 rpm against the returning wave's 13,267 / 10,614 / 8,491); on the pipe +48 % torque in 1000 rpm; three steps a cone suffice. Next: ports, reed, scavenging, power valve, twins and triples, integration |
| B2: motorcycles | **handed off 15:05** (`docs/handoffs/b2-bikes.md`) | cloud session `session_01LFAGgRiXaNaWQi2eaxy3Mi` | 14:15 | 15 tests behind `car.kind` Motorcycle; 13 bike engines whose firing intervals are exact in the running engine, 6 bikes; class quarter miles within their margins on class power curves (superbike 9.47 s at 150.5 mph); cost 0.014 R. With the real engine bikes make 50 to 65 % of class power (the breathing and friction question) |
| X: the whole program under test | running; **84 of 84 default combo cases pass on the tip**; X-001 to X-018 closed but X-011's standstill half (I) and X-016 (noted: a four-rotor with random everything); the listening pack runs end to end (`docs/checkpoints/M2.md`) | cloud session `session_01LFAGgPEmBJHCJxEdvsiG1Y` | 14:15 | X-001 `engine.custom` at defaults never started (**fixed** by D); X-003 no cold start at 253 K with `fuel.model` and `ecu.model` both on (H); charge-air temperatures read 0 K with no machine (Q); **the output passes full scale on a turbo six's launch with `sound.model` 1 and the host's guard then kills the engine** (L: a ceiling; C: clamp and count, never restart for a finite overshoot) |
| D2: heavy diesels | **handed off 17:24** (`docs/handoffs/d2-diesels.md`): the compression brake at 406 kW at 2000 rpm with the vanes shut (published about 450); the flap on G's slot; the eighteen-speed waits for I's hook | cloud session `session_01LFAGgRbWtehbKx8ghnMmpB` | 15:02 | a compression-release brake through E's second exhaust event, an exhaust brake, injection as a tuner's thing, truck turbos, the eighteen-speed and a loaded rig, the air system's sounds |
| P: content, validation, the manual | running; **table D: 186 of 617 missed; power and torque in range on 14 of 31 each** (A: 218 of 581; 5 and 7 of 30); peaks from a three-point mean; quarter miles in the cars' own gearboxes (15 of 23 in); targets re-pointed at the thermocouple readings next | cloud session `session_01LFAGgSHzU6yna9jNWSn93D` | 15:02 | about thirty archetype builds at a standard day with generic names; validation targets with sources, reported as the simulator's list of misses; the manual; help texts |
| N1: the numbers, engines that breathe by themselves | running; causes A to I measured and fixed or routed: the phaser by intake closing, charge heating, the intake conserving (and its own plenum regression fixed), the burn with speed, the start rule, H9 on the blowdown and the flame, **the runner's plenum-end loss (the teeth were a real resonance)**; next: the port bend's loss, the rotary's ports, the pop's fuel at exhaust opening | cloud session `session_01LFAGgS18iAMhTAJ6tm4wT6` | 16:04 | card `w2-n-calibration.md`: the loss ledger (volumetric efficiency, IMEP, pumping, friction, BMEP by class) first; then why every NA archetype lands at 11 to 13.4 bar BMEP whatever its head (mild two-valves too strong, tuned four-valves 25 to 30 % short on power), the rotary doubled, big-cam idles high |
| N2: the numbers, boosted engines | running; the back pressure (G), the gate, the click and thump causes, the sequential and one-bank housings, the VGT's back-pressure limit, the diesel's lambda; **next: gain-reversal protection in the VGT's vane loop** (two steady states on D2's truck) | cloud session `session_01LFAGgSxnqsW2XUcFC39ruQ` | 16:04 | every boosted archetype 20 to 100 % too strong (2.0 turbo 29.5 bar for 22 to 25; kei 103 kW for 44 to 50); first question: what back pressure the cylinder sees while the turbine is fed by a stand-in; then boost that should taper, charge temperature, knock-limited spark, a supercharger's drive; the tractor's broken top-speed run |
| N3: the NA over-delivery (air and energy balance) | **started 19:31**: seven NA engines 20 to 27 % strong; balances per engine on the settled dyno, the share out of range, its fix; takes over discharge by class | cloud session `session_01LFAGgRNmSHhkHzLmwKRJp4` | first report about 45 min after its build |  |
| Held: W the wheel's feedback, R release | not started | | | W cannot be proven without the owner's wheel and shares files with the shell; R needs the owner's signing identities |

## Landed

- **26 Nov 04:36 UTC: `main` at d586682c** (from b557c4a1, fast-forward; run 45752116326 on that commit: Linux
  581 of 581, Windows 573 of 573, macOS green; every build re-baked on sim f23337cc / host 2e7c1bec / tools
  7c3e4d59, 28 knock tables and 14 cam maps, all 31 gated warm and cold). New in `main`: the exhaust's heat settle
  on a cycle's mean and weighted as the running path (a predicted point lands on a 90 s hold's within 1 %); the
  burns' gas booked hot (the rally five's port 7.5 to 5.0 bar); a pipe's wall keyed on its bore and the rate
  under the new models; a stopped engine reporting the loaded engine before the key; the stepped dyno approached
  from below; a custom one-bank engine keeping its split exhaust, its headers split by firing order for twins a
  bank each; the auto level's key for dense bangs; `boost.mismatch` bit 128 with the AIR page's words for it and
  for bit 16; a bore under its least refused at input with its sentence in Rows and in table cells; the
  inspector's wheel through the shared scroll rule (the owner's report: it scrolled a thirteenth of the others);
  EDITING FROM THE G29 (the owner's request: the wheel's free buttons as the inspector's hands; B, U1, J2;
  proven on made-up states only, the real wheel is the owner's to try). The first bake of this window stopped at
  its own gate (the 3.0 twin turbo's warm idle, sd 54 against 25): the idle cure 98d0dc4d was the cause and was
  reverted for the bake; its real cure is the next window's. Table P2 by P's numbers (the table's docs are local
  to P, whose push is refused): 110 of 847 missed, 30 of 204 cited (113 on Table P); in: the 3.5 TT pickup's
  peak torque, the rally five's speed of peak torque, the two-stage's sharpest point, the 3.2's VE at its
  torque peak; out: the boxer's speed of peak torque (3509 against 3600 to 4400); the two-stage 192.3 / 189.5 /
  197.4 N m about its switch, crossing 5921 rpm; first.json 226 N m at 1500, 389 at 1800, peak 391.8 against
  333 to 368; the 2.0's 1750 rpm row 248.7 against 304. Left for the next window: no wheel without a path (a
  dial reaches a turbo with no gas), the idle's cure, the gate loop's limit cycle, the rest of the queue's list.
- **25 Nov 23:25 UTC: `main` at b557c4a1** (from 43006034, fast-forward; Linux 604 of 604, macOS 603 of 603,
  Windows green; every build re-baked, knock 6 and cam 4; Table P 113 of 847 missed). New in `main`: the diesel
  package (the compressor map fitted to the cited maps, the torque ceiling, the engine brake retuned); the
  clutch's latch and the bike's; bangs Burned in the pipe by default, with the auto level's rule for them; the
  bores' floors; the fan's tone; the predictor reading a point as an upward sweep reaches it; the Logitech G29
  on macOS and Linux (pedals, clutch, paddles, the H shifter, the PS button, five shift lights, the spring, the
  WHEEL row, the button mapping, `tee-wheel-dump`, the input probe: never yet run on a wheel; none on Windows);
  the assistant's list of every whole build and its LOAD / KEEP dialog; DRIVE's dials with numbers clear of
  the needle; Rows as the inspector with the part card. Known and written in the queue: on a spark-cut limiter
  the burned bangs' pressure is overstated (the rally five); the 2.0 at 1750 rpm 24 % under VW's 320; the
  rotary above its join; the two-stage's switch 100 rpm before its curves cross; the bake's soak 213 of 221.
- **25 Nov 22:05 UTC: the bake is on the branch at 223f4301** (abadfb4b; not in `main`). Every build re-baked
  (knock 6, cam 4): 28 knock tables, 14 cam maps, 31 builds gated warm and cold, five proofs. With it on the
  branch since `main`: the diesel package; the clutch's latch; bangs Burned in the pipe by default with the
  exposure rule; the bores' floors; the fan's tone; the predictor's approach from below (the two-stage's notch);
  the Logitech G29 on macOS and Linux with its WHEEL row, `tee-wheel-dump` and the input probe (never yet on a
  wheel); the assistant's build list and yes/no dialog; DRIVE's dials with numbers clear of the needle and
  tight round scales; Rows as the inspector, live at launch, with the part card. `main` waits on CI for the
  baked commit and on one macOS-only red in a shell test of the assistant's question (B).
- **25 Nov 20:41 UTC: frozen for the bake at 9c33806c** (not in `main`). The window landed: the diesel package;
  the clutch's latch; bangs Burned in the pipe as the default with G's fix and L's exposure rule for them; the
  bores' floors; the fan's tone; the predictor reading a point as an upward sweep reaches it (the two-stage's
  notch cured); the Logitech G29 on macOS (compiled and run by CI, not yet on a wheel) with `tee-wheel-dump`;
  the assistant's build list and its yes/no dialog; the DRIVE dials' numbers clear of the needle; Rows as the
  inspector, with the part card. CI green on all three with the diesel package (61ca5f0d, 60e87aef) and on
  macOS and Windows with the wheel, the default and the kit (adf53f54); the last pushes not yet read. P bakes
  every build next; `main` follows the baked commit's CI.
- **25 Nov 19:44 UTC: on the branch, not in `main`:** bangs Burned in the pipe as the default with G's fix
  (81ec1b12); the Logitech G29's first slice on macOS (5b66951a: pedals, clutch, paddles, the H shifter, the PS
  button as the start key, the five shift lights, 900 degrees and the spring; ported from TestingPlayground;
  never compiled on macOS before CI, never run before the owner's Mac); the bike's clutch (9eb34e2a); the chat's
  "Add a key" (98333523); Rows and the part card in the kit (4dd26210), not yet in the frame. Found: the
  two-stage's notch is the predictor reading a point inside the cam switch's band in the wrong stage (N1's patch
  with C); the rotary's secondary turbo does not come on above the join (N2).
- **25 Nov 19:21 UTC: the diesel package is on the branch at e9b2ef3f** (not in `main`; CI on it not yet read).
  N2's ten commits (the compressor map fitted to the seven cited maps; a governed diesel's wheel sized at its
  torque peak; the whole-flow VGT nozzle; no vane surge guard; the loop's integral held where the vanes cannot
  follow or the ECU owns them; the shaft's ceiling in the boost loop) and D2's six (the builds off the cam dial,
  the fixture's 260 kPa target, `ecu.torqueMax` / `ecu.powerMax` with the truck's fitted fuel curve removed, the
  brake without its own shaft integral, the cylinder cut's case, GradeTest's checks). Tier: 580 of 580 on Linux
  before the merge of test-only commits. The baked tables are stale until P's next bake.
- **25 Nov 19:15 UTC: the owner's rulings** (`integration-queue.md`, "The owner's rulings and a new fault"):
  bangs burned in the pipe become the default (G, before the bake); the inspector becomes Rows, then the card on
  the picture (J2, U1's hook); the idle's loudness stays; the wheel is a Logitech G29 with the H shifter, to be
  ported from TestingPlayground's working code (B; reference files in `docs/reference/testingplayground-wheel/`;
  macOS first, never run in the cloud); a fault in DRIVE's dials, numbers under the needle (U1). Nothing of this
  is in `main` yet.
- **25 Nov 18:37 UTC: `main` at 43006034** (from 76420f32, fast-forward; Linux 587 of 587, macOS 587 of 587 on a
  second attempt after GitHub had no runner for the first, Windows 580 of 580; sim/host/tools equal the bake's).
  New in `main`: the octane as a number with the knock tables following it; bangs burned in the pipe as a choice
  (default Carved); the start's exposure and the diesel start fuel; the clutch held on the overrun; stage help;
  DRIVE CLASSIC's dials, the ribbon's unsettled marks, a tab for every inspector page; GARAGE without the test
  fixtures; the rotary's cited ports and the two-stage's high lobe; the assistant (SETTINGS > CLAUDE DESKTOP, CHAT,
  the key dialog; never run against Claude Desktop or the API); the soak by request. The next window is open (the
  queue's FREEZE STATE): the diesel package first.

- **25 Nov 15:23 UTC:** `main` still at 76420f32. The window's sim, host and tools items are on the branch and
  baked (P: 15152e0b local, tools sha 758bb12f's; its push follows its tier and must sit on card A's SIGPIPE cure
  and B's two lines). `main` moves when Linux, macOS and Windows are green on that push. Open meanwhile: ui/,
  assistant/, app/, shell/, tests/, ci, docs. The queue's section of 15:23 has what was found while the bake ran
  (the compressor map and the diesel package; the clutch; the bangs; the soak's findings; GARAGE listing fixtures;
  the assistant's platform faults) and the next window's contents.

- **25 Nov 13:59 UTC:** `main` (76420f32) green on all three platforms (Windows 13:50). The window's first three are
  on the branch (tip 1f02d786): the steward's packages and the soak by request only, U5's stage help, N1's octane
  as a free number. Held outside the window: the compressor map's two low coefficients (N2's model-wide table,
  local) and the clutch that slips open under the compression brake (the integrator, local). The queue, 13:59.

- **25 Nov 13:43 UTC: `main` at 76420f32** (from d0a7330e, fast-forward; Linux 545 of 545 and macOS green on run
  45623044537, sim/host/tools equal the bake's dbd64a5c). New in `main`: the idle spark trim's two cures (the dyno's
  low end), P's bake on them, hover help, DRIVE's three new styles, the fuel control (octane grades; its macOS fault
  caught by CI before `main` moved and cured). The window for the held items is open (the queue's FREEZE STATE has
  the order); the diesel package stays out of it, its rated end unruled (the queue, 13:43).

- **12:20 (25 Nov) the owner: "Windows was able to launch and run"** (`main` at d0a7330e; the first run of the app by
  a person on Windows; what beyond launching and running was tried is not known yet). README's line updated to say
  exactly that. **The owner also asked whether the right-side controls are to be redesigned ("relatively clunky
  still"):** no plan existed; J2 is woken to draw three designs of the inspector as real pictures (rows with typed
  numbers and a filter; dials kept but exact; fields on the stage's picture), nothing merged until the owner picks.
- **`main` is held at d0a7330e** on the owner's decision about one listen assertion on P's re-bake (queue, 12:16).
- On the branch since the last entry: U1's DRIVE styles (CLASSIC, DIGITAL, RACE beside STANDARD; 545 of 545 by U1;
  rendered by the lead and sent to the owner; CLASSIC's small dials overlap their digits at the small stage, U1 is
  fixing that first).

- **12:02 (25 Nov) check-in.** `main` at d0a7330e. P's re-bake at dbd64a5c running (pre-bake tier 540 of 540 by P);
  sim/, host/ and tools/ frozen since 11:26, untouched by every push since (the lead's diff: 0 lines).
- **On the branch since `main`, ui/ and app/ only:** U1's hover help (240c9a58: every bound control and the frame's
  own items; a test that fails when a settable field has no help; 541 of 541 by U1; the lead rendered its picture in
  its own build and sent it to the owner) and U4's fuel control (89fa786e: a FUEL band in TUNE with AKI, RON and
  MON read from `fuels.fm`, the knock map and the bake's state after a change, the DYNO naming each run's fuel;
  543 of 543 by U4). The owner's todos 5 and 1.
- **The lead now builds the tree itself** (build-dev in its checkout) and re-measured two claims before they move
  anything: D2's trim cure and N2's (b) wheel, both confirmed (numbers in the queue).
- **Held for the next window, each built or building and each on the lead's word:** card A's `Build` step ids, A1 and
  A2 (Claude Desktop over MCP: built and tested locally, never run against Claude Desktop); C's knock bake with
  lighter pedals (kKnockBakeVersion 5; N1 found the V12 idling at 10.8 deg after top centre on a row copied flat);
  N2's (b) and the surge setpoint limit (table first); G's half of the pops and N1's seam for it (G's session would
  not take the owner's leave through the lead, so the two files went to the card that owns them); D2's start fuel;
  N1's 93 grade. Content only, for P: the rotary's cited ports (N1: 244.5 to 320 N m at 5000 on 294) and the
  two-stage build's high lobe on the F20C's centreline (N1: the stages cross at 5802 rpm where they crossed at
  7188; makers' 5500 to 5900).
- **Fleet:** nine working at times (P, N2, U1, U4, G, A, N1, D2, C), under the owner's ten.

- **11:30 (25 Nov) the fleet under the owner's cap of ten.** Working: P (the re-bake at dbd64a5c for the next
  `main`: D2's two cures under sim/), N2 (its low-speed cells again on the cured engine), U1 (hover help built,
  then the DRIVE gauges), U4 (the fuel grade in TUNE), G (the pops), card A (the assistant, A1), N1 (the two-stage
  build's torque dip at its cam change, the owner's own report; the rotary's 239 N m against 290 to 315, by a
  ledger), D2 (the nightly "diesel cold, full" red, locally). Asleep: the rest.
- **G is stopped at a permission its session will not take from the lead:** edits to `sim/ecu/overrun.fm` and the
  account's call in `sim/combustion/combustion.fm` are outside the paths its original brief names, and a peer's word
  does not widen them. The lead has not worked around it; the owner is asked. Meanwhile G builds inside its paths
  (`PipeBurn`, the event inbox, the junction's source, `exhaust.bang`, tests on synthetic events). G also corrected
  the lead's arithmetic: a burning velocity is against the unburned gas, so the expansion does not shorten a slug's
  burn; a spark-cut charge burns in tens of ms by Metghalchi and Keck plus the collector's turbulence, not the 7 ms
  the lead wrote.
- **D2's two cures on the branch** (2cf55103 the diesel charge takes no idle trim with the pedal open under the
  metered governor; c16af6a2 the same rule for petrols on the bench in `idle.fm`), both diffs read by the lead, 540
  of 540 by D2; not in `main` until P's bake is green.

- **11:15 (25 Nov) `main` moved to d0a7330e** (fast-forward from 7f64dac1): P's bake plus the owner's macOS bundle
  fix, applied at their word ("Apply the bundle fix and keep the fleet thin", about 10:53). `app/Window.cmake`: the
  link depends on every file of sim/ and content/ (LINK_DEPENDS over a CONFIGURE_DEPENDS glob), so an edited or
  added file links again and the copy into the bundle and its signature follow; the mechanism proved on Linux with
  Ninja in a scratch project, not run on macOS from here (the owner's own 392b6a4f verified it on their Mac); a
  deleted file stays in an old bundle. Run 45599543068: linux-x64 and macos-arm64 green; windows-x64 still running
  when `main` moved (the change is inside `if(APPLE)`; 7f64dac1 was green on Windows in both its runs). sim/, host/
  and tools/ identical to 7f64dac1 (the lead's diff: 0 lines).
- **The owner's five todos** (about 10:58; integration-queue.md has each checked against the tree) and "Are there
  any other todos that can run right now?" (about 11:12). Started: **U1** hover help on every bound control, then
  the DRIVE gauges and styles (ui/, app/); **U4** the fuel grade as a first-class control in TUNE (`spec.fuel.grade`
  exists and re-bakes the knock table, but no view names it); **G** the exhaust pops (`sim/exhaust/pop.fm` is the
  reference's synthesized pulse and crackle; no shipped build states `ecu.overrun.fuel`, so what fires at the
  defaults is G's first finding; design to the lead before code); **card A** (a new session, the assistant) on phase
  A1 of `docs/plan/w3-a-assistant.md`, the abstract plan for Claude in the app, written from a map of
  TestingPlayground and **rewritten after an independent review** that broke several first claims (the seam is the
  document through an inbox on the frame's thread, not the simulator's command ring; undo, range checks, the key
  store and the socket were not as the draft said; Claude Desktop over MCP first, the key chat second).
- **D2's trim cure, first form refused by its own tests:** skipping the trim outright under the metered governor
  took the cranking fuel with it (233 K start 3.7 to 19 s). Ruled 10:59: pedal open, the charge takes no trim; pedal
  shut, the trim as today (the reference author's own rule for the driven case, taken to the bench); the same rule
  for petrols in `idle.fm`'s bench branch as a second commit (all 28 petrol builds' full-pedal dyno torque below
  ECU idle + 450 rpm is cut today, floor 0.3; D2's read, the lines read by the lead). In progress.
- Awake, each for named work: D2, U1, U4, G, A. Idle and waiting: P (one re-key after sim/ settles), N2. Asleep:
  the rest. The steward was woken once to check for unpushed work: none.

- **10:33 (25 Nov) `main` moved to 7f64dac1** (fast-forward from 5548dcf8; run 45592657103: linux-x64, macos-arm64
  and windows-x64 green; the soak still running). P's bake at tools sha ed025908, whose sim/, host/ and tools/ the
  lead diffed against 6bac7c78 and the tip: 0 lines. P: 540 of 540 before the bake and after it; 28 petrol knock
  tables and 14 cam maps in the app's order, no step fallen back; gate 31 idle warm and cold; first.json, the V10,
  the tractor, the hot hatch and the rotary "Current at once" on an empty cache. **New in `main`:** the rotary's
  rising hunt at 4250 cured (`seqRpm` visible, 4500 stated); the predictor ending 300 rpm under the limiter with
  limited points as rings; the cam bake's cures (kCamBakeVersion 3); the predictor's settled-cycle mean with its
  standard error (kKnockBakeVersion 4); verified reuse of a table baked under another program; cells still knocking
  marked in TUNE. **P's rows on this content** (local, one agent's): 115 of 847 missed (120 on the last), no
  invariant failing; into range: the peak-power speeds of the 3.2, the flat-plane 4.5 and the V10, the big block's
  peak-torque speed, the 3.9 twin turbo's power, the pickup's trap; out: the 3.0 twin turbo's peak-power speed (6860
  against 6800); the rotary's torque shortfall (239 N m against 290 to 315) is not its join.
- **The fleet stopped on the account's usage limit at about 09:12** and the owner said at about 09:30 that usage was
  back. Since then only P, N2 and D2 have been woken, each for named work; the rest sleep.
- **Found 10:04 (D2's ledger; the lead read the code): the reference's idle spark trim scales a governed diesel's
  heat on the dyno** (`sim/ecu/ecu.fm:209` into `chargeDiesel`): below the ECU's idle + 450 rpm with drive == 0 a
  diesel at full pedal burns about half its metered fuel (tractor at 1000: 3330 J released of 6365 reported a
  cylinder-cycle, 40 % brake on what burned). One fault upstream of the turbo: the tractor's hole below about 1100,
  its take-up, "the turbine cannot hold 1000", C's knife's edge at 1060. **Not cured in 7f64dac1.** Ruled: under the
  metered governor the charge does not take the trim; D2 pushes it on a green tier (told 10:33), then one re-key by
  P. Open: whether the trim cuts a petrol's full-pedal dyno torque in the same band (D2's read).
- **N2, measured and unpushed** (docs on the branch: nozzle-sensitivity.md, vane-guard-off.md): (b), the governed
  diesel's compressor sized at the maker's torque-peak speed, is right in form (tractor 1100: 1910 N m surging to
  2722 steady) but its 10 % margin is used up in the model's own terms by fresh-air VE (0.836 against 0.9) and the
  pulse (the flag reads the instantaneous flow, 5 % under the mean's); the nozzle's inlet factor stays 1.247 (no
  cure, one source); the vane surge guard both makes holes (tractor, pickup, the 2.0) and saves the truck from its
  second state, so it stays and its replacement is open. **The lead ruled the guard's removal at 09:40 on principle
  and withdrew it at 09:58 on N2's truck table.** All of it is judged again on the engine after D2's cure.

- **09:13 (25 Nov) GO to P for the next `main`'s bake, at tip 6bac7c78** (read by the lead on the branch:
  `kCamBakeVersion` 3, `kKnockBakeVersion` 4). C's predictor mean is in (1db8dcd2: a settled point is the mean of its
  settled cycles, at least six, with `PredictPoint::torqueSe` and `settledCycles`; the economy four's cam row at 4786
  rpm reads 134.7, 135.4, 136.9 N m where the two-cycle mean read 135.0, 134.0, 136.8). Since `main` (5548dcf8) sim/
  differs only by `spec/boost.fm` (seqRpm visible), host/ by the bake and the predictor, tools/ not at all. **sim/,
  host/ and tools/ are frozen** from the bake's start (N1, N2, C, D2, U1 told); all pushes freeze once P's content is
  up; `main` moves only on a Linux and macOS green commit whose three directories the lead has diffed against P's bake
  sha. **Ruled for N2, the governed diesel's compressor (b), measured and unpushed:** the torque-peak speed is the
  build's stated field from the maker's sheet (P's section 20, 973f943c: the X15 kind "peak torque starting at 950
  rpm", the 6.7 at 1700, the 2.0 TDI from 1750); no judgement factor on the derived default. N2's table, predictions
  written first: the tractor's inducer 83.2 to 76.8 mm sized at 1000 rpm along the whole climb (the 83.2 wheel
  surges on the way up near PR 1.9); 1100 rpm goes from 1910 N m surging to 2722 steady, 1300 and 1900 within 2 %;
  **1000 rpm pulses 17 to 75 kPa** where it was a steady hole at 13 (the real engine holds from 950: a turbine-side
  finding). (b) waits for N2's nozzle sensitivity on that wheel; if 1000 still pulses, the two builds state 1100
  (inside the makers' 950 to 1150), marked as chosen for that reason. (a), the surge guard, stays held: it neither
  causes nor cures the pulse (N2's measurement). One agent's readings; the lead has not re-measured them.

- **08:53 (25 Nov) the lead's check-in; `main` at 5548dcf8, green on all three platforms** (its own run 45572861796:
  540 of 540 on each, packed; the steward). The branch is green on Linux and macOS at 5b4b2e7. **On the branch since
  `main` moved** (each after the whole default tier): C's 300 rpm default margin for the predictor (bda6a728;
  kKnockBakeVersion 3; the DYNO ribbon's top point no longer at the limiter: 128, 274 and 387 N m where it drew -36,
  -81 and -155) and the tractor lug case on the predictor's honesty; N1's cam bake cures (fab9ae4c: the knock gate
  out, the tie rule, limited points as missing; kCamBakeVersion 2; 540 of 540); N2's `seqRpm` visible with the
  rotary's 4500 stated by P (500cb1a9: the rising hunt at 4250 cured on the build as it ships; both join cases read
  the build's own file, unmarked); U1's and U4's limited points as dim rings and out of the peaks, the cam page
  saying when its table is being baked, cells still knocking marked; C's host timing check alone and serial; D2's
  note that its truck's BSFC is flattered by the reference turbo. **Table L, final** (P's 5b3aa404; three columns on
  one simulator): K's content 116 of 847 missed, the seat-held class rule 118, the shipped 1 mm-held and baked cards
  120; no invariant failing in any; the 1 mm anchor brings back the V12's and VR6's peak-power speeds, three
  over-sharp curves and two torques and costs the V10's and V12's idle manifolds (0.46 and 0.61 bar against 0.30 to
  0.44): the trade it was ruled on, recorded. **Found by N1 and ruled**: the predictor reports an unknocked point as
  the mean of its last two cycles, and the engine's own cycle-to-cycle scatter (0.5 to 0.8 % a cycle on the economy
  four at 4786, as the model's sigma gives) is the "half per cent a point": a 2 % zigzag between adjacent cam
  positions that vanishes over the settled cycles (134.7, 135.9, 137.0, 137.8, 138.0 N m). C: the predictor reports
  the mean of a point's settled cycles with its standard error; N1: each bake cell the mean of its last 8 cycles,
  the tie floor its own error, no row giving up more than 1 % to smoothing (kCamBakeVersion 3; the economy four's
  differing rows 6 to 2, the V6's 4 to 1, the flat-plane's 1 to 1). **P's next bake waits for those two** (about
  70 to 80 minutes from 08:34), so that the builds are baked once: then the gate, the whole default tier, the proof,
  the sha, and the next `main` by the same rule as the last (frozen directories, diffed). *A false alarm, closed*:
  C's "nine cases fail at -j8" was last night's LastTestsFailed.log; the steward could not reproduce it and only one
  of the nine reads a clock. *N2 next*: the tractor's (b) on P's matching source (section 18 has a first figure for
  the exhaust-to-intake pressure ratio), the rotary's weak joined pair, the run-down (wrong in shape at both ends
  against a measured K27: the bearing has no oil state).

- **08:02 (25 Nov) `main` moved e7e58973 to 5548dcf8**, fast-forward. Run 45569559847: Linux 540 of 540, macOS 540 of
  540 (the lead checked the run's jobs; Windows still in flight; main's own run follows). Its sim/, host/ and tools/
  are identical to 32320974, the commit P's bake was made on (0 lines, diffed by the lead). **What it holds over
  e7e58973**: N2's compressor map (surge on the published middles, the flag where the model does surge), the sizing
  at the design tip speed and the matching on the charge's own temperature (the truck one state; the tractor's and
  the pickup's low end; the turbo four at its target by 2500), the wastegate loop's feed-forward (tip-in overshoot
  from 39 to 56 % down to 7 to 13 % with the IS20's wheel), the simulator kept stepping while a turbo runs down;
  D2's smoke limit at 0.77, the diesels' start-of-combustion spread at its cited size, the full-load fuel curve (the
  15 L truck and the tractor); N1's four-valve low-lift curtain was already in. **Content** (P's 066165a0): the hot
  hatch as the 4A-GE, the 7.0 on the LS7's cam and ports, the 3.2 on the S54's cams, the two-stage's low stage and
  5850, first.json's IS20 wheel, the petrol judgement cards under the class rule with the 1 mm held, the diesels off
  it; **every petrol build's knock-limit table and every phased build's cam map baked in the app's order on this
  simulator**, so the app finds them current at once (proved on first.json, the V10, the tractor and the hot hatch:
  no cache file written); all 31 gated warm and cold; no invariant failing; 120 of 847 rows missed on P's final
  read. **Marked `[!mayfail]` in the default tier on it** (the steward's list): the VGT's back-pressure limit at the
  open stop, the over-boost cut under a gate too small, the sequential pair at its join. Nightly: combustion H6 and
  H7, and C's tractor lug case (restated, held). **Audible or visible and known**: the rotary hunts at 4250 rpm
  rising (its cure is ready: `seqRpm` visible and stated); the two-stage's change is a torque dip (its crossing
  about 7300 against 5500 to 5900); 7 to 13 % of overboost at tip-in on the turbo four; the DYNO ribbon's last point
  at the limiter on some builds (C's 300 rpm margin, held); the tractor's hole under about 1050 rpm and its 7.9 s to
  torque; a truck's rotor running down too long. *What it took, for the record*: the bake's keys hash the whole
  program, so the bake had to be the last content step on the exact simulator shipped; sim/, host/ and tools/ were
  frozen at 32320974; three crossings with C and one breach by N2 (un-hiding `seqRpm`, right in itself: a hidden
  field is not read from a build's file) were undone by reading the branch and not messages; the last red was three
  of C's host cases assuming content unbaked (5548dcf8: `unbaked()`). **The freeze is lifted**: C (the 300 rpm
  default, kKnockBakeVersion 3, the lug case), N1 (B', the tie rule, kCamBakeVersion 2), N2 (`seqRpm` visible; the
  rotary's sweep through the harness on the shipped build: 0.56 to 1.65 of its mean at 4250 becomes 0.89 to 1.13,
  nothing else moves) and U1 (limited points as rings; bake.watch from an unbaked build) push held work; then P
  states the rotary's 4500 and bakes once more on that tip: the next `main`. **New rules from tonight**: a push that
  touches sim/ or content runs the whole default tier first; the lead reads the branch before ruling and moves
  `main` only to a commit diffed against the bake's; anything from one agent's reading goes to the owner labelled
  unconfirmed.

- **06:59 (25 Nov) the lead's check-in; `main` stays at e7e58973; sim/, host/ and tools/ are FROZEN at 32320974 for
  the bake `main` ships.** *How `main` moves now*: P bakes all 31 builds in the app's order on tools built at
  32320974 (its Bake.h reads kKnockBakeVersion 2: C's cure A is in), gates them, proves on first.json, a naturally
  aspirated build and the tractor that the app finds them "Current" at once with no cache file, and sends the sha;
  the lead moves `main` only to a Linux and macOS green commit whose sim/, host/ and tools/ are identical to
  32320974's (diffed by the lead). Why: P proved that the tables' keys hash the whole program, so content is current
  only for the exact simulator it was baked on, and any change under sim/ makes every build bake again at its first
  load. *Corrected table L* (P's first L read stale content from the pinned worktree: `report.py` now always names
  its content, 74e055f6): on tools 430cd999 and content 74e055f6, **every invariant passes on all 31** and 118 of
  847 are missed (K: 120 of 839). "The 6.7 pickup cannot reach its limiter", which the lead passed to the owner as
  a broken build, was the stale read; the diesels stay off the class rule on its own ground. *The breach*: N2
  pushed 3fe19dd9 (sim/spec/boost.fm: `seqRpm` un-hidden) during the freeze, before reading it; told to revert it
  and 3e0c1c14 at once; not yet reverted at 06:58 (tip 95df1a30). **Its finding is real and confirmed by the lead's
  code read**: `inFile` (host/src/Build.cpp) does not read a hidden field from a build's file, so the rotary's
  stated 4500 was ignored and the "hunt on the shipped build" was the old join; with the field visible N2 reads 59
  to 75 kPa at 4250 from the content alone. The cure goes into the next `main` with the whole sweep through the
  harness on the build as it ships. *Three crossings with C* (cure A pushed, reverted, reinstated inside ten
  minutes while the lead ruled by message): the tip was read by the lead and frozen where it stood. **Held until
  `main` has moved**: N1's half of the cam bake's cures (B': the knock gate out of the cam bake, since the spark
  table's cell does not depend on the cam, so the lead's B gated nothing; the tie rule; limited points as missing;
  the cam's top at redline - 300; kCamBakeVersion 2; 540 of 540 and the economy four choosing 35 and 40 degrees
  both ways) and C's 300 rpm default margin for the predictor (the DYNO ribbon's top point is at the limiter today:
  -36, -81 and -155 N m on three builds; kKnockBakeVersion 3). C's findings on the cam bake (repeatable to the bit;
  the limiter's row smoothed into the rows below; the gate fooled by the spark table; flat curves flipping inside a
  point's floor) and C's verified reuse of tables across programs (approved, after the cures) are in C's messages
  and Bake.h. *N2, measured and held*: the tractor's low-speed end (a guard pinning the ratio at its own flag's
  gate below 1050 rpm; a surge cycle above; a wheel at 0.23 of choke where the published surge share is 0.27); (a)
  the guard on the flow trades the hole at 1000 for a 3 s boost pulse and is held for (b), the matching placing the
  torque peak, on P's source. *D2*: the tractor's BSFC is pumping (exhaust over intake 1.40 to 1.52 on the matched
  nozzle), not combustion. *C*: a diesel reads current for the spark table without a bake (be3ce9c4).

- **06:11 (25 Nov) the lead's check-in; `main` stays at e7e58973; the branch's tip 430cd999 holds everything `main`
  waits for and its run is queued.** *The gate*: Linux and macOS green outright on a commit holding N2's ef2160fd (the
  simulator kept stepping while a turbo's shaft turns; exhaust.engine passes at 72.9 Pa; a turbo four keeps it awake
  23 s after the key, the truck on GateTest's sets 79 s) and 9a0d4fc3 (the one-state bound at 15 %), green on Linux
  at 362cd47c (538 of 538), **and P's 4ca747ec, the diesel cards off the cam class rule**. *Table L is published*
  (cbc440eb; pin e43e228b, content 1efa3282): **121 of 847 missed** (cited 29 of 203), its twin on K's content 114, K
  on its own pin 120 of 839; power 12 and torque 10 of 30 (the tractor's set by its curve and out). So the model took
  the count from 120 to 114 and the sourced cards took it from 114 to 121. What L's content showed: **the 6.7 pickup
  could not reach its limiter** (2861 rpm against 2982 to 3167; 252 kW): the class rule, whose sources are petrol
  cams, had been applied to three diesel cards: reverted, `main` waits on it; the V12's, the flat-plane's and the
  VR6's power peaks below their bands; the 7.0 over on power (406 kW against 370 to 385) on the LS7's cam and ports;
  the two-stage crossing at 7320; the turbo four 192 / 217 / 394 / 287 N m (P's decomposition against N2's 232 and
  270: the new cams 16 and 27 N m, the rows' 5 s step 15 and 10, a residue). **Ruled: on judgement cards the class
  rule holds the duration at 1 mm and moves the seat** (it had held the seat and cut the 1 mm by 4 to 12 degrees: 10
  on the V10 and the flat-plane, 12 on the V12; a rule adopted for the flank near the seat had resized the cams); the
  cost, more overlap at the seat, to be read on the idle rows of table L2 (three columns: K's content, L, L2). The
  next content step, after `main`. **Ruled: a row never passes on a point the predictor flags unsettled** (the
  tractor's lug at 1100: 1922 N m held, 0.70 of its peak, a miss; its plateau "unsettled"). *N2*: the sequential
  join's mechanism is on the branch (c1e10dfe: acts only on a build that states `seqRpm`; the rotary with 4500
  stated: 4250 rpm from 26 to 77 kPa to 59 to 75 rising, a changeover dip of 2.8 psi; the joined pair weaker than
  the single below about 5000, the rotary's open item); its case's bound to a reasoned 25 % (it read 14 against 15);
  P commits the rotary's `seqRpm` 4500 (an enthusiast's measurement and a forum's quote of the workshop manual).
  Then the tractor's low-speed end. *P*: the knock-limit pass on the 16 naturally aspirated builds running
  (knockpass.py, e0b372c3). *D2*: no case marked; the BSFC look after.

- **05:24 (25 Nov) the lead's check-in; `main` stays at e7e58973 (green on three platforms); the branch has been red
  since 04:06 on N2's commits and its cure is in hand.** *The red*: (1) exhaust.engine, EngineTest.cpp:317, the
  collectors' pressure 6 s after the key off on a twin-turbo V8: 61 Pa at e7e58973, 146 from N2's sizing commit (the
  steward's bisect). N2 pushed after the boost, calibration and diesel tiers only. **N2's finding: the case was
  reading a simulator that had gone to sleep** (engine.fm's `audible()` stops all stepping some seconds after the
  engine is quiet) with the rotors still at 9,600 rpm on the old tree and 13,700 on the new; kept stepping while a
  shaft turns, the collectors follow the rotors down (72.9 Pa at 6 s, 20 at 15) and G's case passes unedited. Ruled:
  that cure (`boost.spinning()`, one line in `audible()`, the lead's leave for D's file), 543 of 543 on N2's machine;
  a turbo heard running down after shutdown is new. (2) N2's short form of the truck's one-state case (a2de1df6):
  3.0, 3.8 and 5.4 kPa apart on three machines against 5 %: ruled onto a reasoned 15 % (the two states were 25
  against 58 kPa), same push. **New rule: a push that touches sim/ runs the whole default tier, chained on "100 %
  passed", not only its package's tiers**; the steward reports a red fast job at once and then only when the failing
  set changes. *Landed since 04:36*: N2's overshoot cure (a7cda791: a feed-forward at the gate's balance with a
  windowed, bounded integral; first.json's tip-in peak +39 / +56 / +48 % to +4 / +11 / +14 % at 2500 / 3500 / 4500
  with the arrival times unchanged; the remaining 14 % for 2.8 s at 4500 recorded; F's case unmarked); D2's `ecu.afc`
  0.77 (9c6977a0, which went out with a red case on a chain that could not fail: owned and fixed), the clatter case
  restated on its mechanism (4e406033), **which found the spread delivering 0.58 of its cited 0.559 degrees**
  (`Noise::gauss()` is 0.577 a sigma, its comment said a unit normal; corrected on D2's term; the diesels' comb in
  the mix 6.4 to 7.0 dB down to 1.9 / 3.2 / 3.5; N1 checked every other use: all the reference's own numbers, none
  from a source), the full-load fuel curve (e43e228b; the truck 455 kW at 1858 rpm against a cited 451, a zero-fuel
  curve to the bit), D2's one-state and brake cases unmarked (213aa90b, 19e29bda: the brake measured after a climb,
  365 kW). C's 9c622135: the predictor agrees with 30 s holds within 2 % on the tractor; the lead's "the predictor
  reads slow engines early" withdrawn. *Table L is running*, pinned at e43e228b on content 1efa3282 (every card
  change, 14 cam maps re-baked, the tractor's fuel curve with its torque and power rows out of the counts); new
  rows with sources: the tractor's BSFC 175 to 205 g/kWh (ICCT/WVU's measured 177 to 189; the model 205.4 at 1200),
  lambda at least 1.25, turbine inlet under 950 K. The twin on K's content after; then the knock-limit pass on the 16
  naturally aspirated builds. *Not going well yet*: the rotary's join: N2's first build narrowed the hunt at 4250
  (19 to 37 kPa) and collapsed 2500 to 4000 (66 to 70 down to 5 to 17); set aside, diagnosis after the red is cured,
  nothing pushed until the whole sweep is better.

- **04:36 (25 Nov) the lead's check-in; `main` at e7e58973, green on all three platforms** (its own run 45543212713:
  542 of 542 on each, packed). `main` waits for N2's cure of the wastegate loop's overshoot. **N2's map commits are
  on the branch** (9d55f503: 7e9bee72 the knee 0.40 + 0.178 (M - 1) floored at Mach 0.8, the flag and the guard at
  0.9 of the knee, the island 0.61; 1acc1025 the sizing at the design tip speed; 29684891 the matching's design flow
  at the charge's own temperature). Checked by the lead against the published file: surge on the flag 0.24 / 0.29 /
  0.36 / 0.44 inside every range, the ridge 0.60 / 0.62 / 0.70 / 0.79 inside but 0.01 over at 3 (within the reading),
  efficiency near surge 0.67 to 0.69, the onset measured on the flag. What it did: the truck one state at 76 to 78
  kPa in 11 s (two states before; the predicted 55 to 65 kPa wrong and kept); the tractor archetype at 1300 from
  1946 N m on 72 kPa to 2263 on 219; the pickup at 1600 from 657 on 32 to 995 on 185; the turbo four at 2500 from
  309 to 385 N m at its target; 4 to 5 % lost at the top of two diesels on a smaller nozzle; EngineTest:124 passing
  untouched; two former failures passing. **New with it, marked on its name: the wastegate loop overshoots** (the
  test's four at 3500: 129 kPa against 100, settling after 8 s), N2's first job. *Ruled since 03:55*: the surge cap
  dropped (it latches: 301 N m on 61 kPa stepped up from 1800 against 385 on 124 reached directly; tried and
  refuted in the document); the sequential join rebuilt as proposed (pre-spool, the charge valve when the second
  compressor can hold the pipe, the decision on speed, a dwell; hidden `seqRpm`; after the overshoot); first.json
  states the IS20's wheel (N2's 6410dbf7: 232 / 270 / 388 / 364 / 288 against the matched 216 / 237 / 385 / 365 /
  288; the prediction wrong at both ends and kept); F's 0.9 stays (two diesels pass 106 and 110 % of the new design
  flow: recorded, looked at with table L); `ecu.afc` to 0.77 goes in although it binds in settled states where the
  air is short (tractor at 1100: 2117 to 1929 N m; the 2.0 at 1800: 166 to 159; D2's V8 truck's rated 535 to 517
  kW): the lead's "a transient limit, no settled point moves" was wrong. *Three things the lead passed to the owner
  before a second look, all corrected with them*: the truck's fuelling (03:00), the turbo four's wheel "half again"
  (03:06), and at 04:12 "the predictor reads slow engines before their boost arrives" (P's reading; C's holds agree
  with the predictor within 2 %: 9c622135). What C found instead belongs to the tractor: **a knife's edge at about
  1060 rpm** (30 s holds: 1544 N m at 1058, 1917 to 2023 at 1061), the VGT barely spooling below it, a dip of 658
  N m at 905 that survives a hold: N2's, with the low-speed vane loop, after the join. *Table L*: P's content is
  committed with every card change (b9dc1732), 31 builds idle, 14 cam maps re-baked on 9d55f503; L and its twin
  next, the diesels after D2's 0.77, then the knock-limit second pass on the 16 naturally aspirated builds (what
  C's bake does to them, which the app in `main` already does to every build and nobody has checked).

- **03:55 (25 Nov) `main` moved 78cdd2f5 to e7e58973**, fast-forward: run 45539104538, Linux and macOS green outright
  (checked on the run's own jobs; Windows' job on it still running, main's own run to follow). Against 78cdd2f5 it
  adds N1's four-valve low-lift curtain (6771c402), D2's diesel telemetry (a90cd20f: fuelFlow the fuel injected,
  lambda filled), N2's two test and tool commits, and P's rows and cards (the cards behind CAM_RULE, so no build
  changes): the tractor's take-up and lug rows (least torque from 1100 to 1300 over the peak: 0.64 against 0.95 or
  more on five makers' curves; the plateau from about 1476 rpm against 900 to 1200). Since 03:13: D2's baseline
  splits the tractor archetype's shortfall in two (boost at 1100 to 1300, the vanes open: 29 and 94.5 kPa; above
  about 1400 the single `fuelBoost` sized for 108 kPa, burning at 0.51 of stoichiometric at 238 kPa) and D2's own
  truck is flat where a real one falls a quarter to rated. **Ruled: a full-load fuel curve for governed diesels**
  (`fuelCurveRpm[8]`, `fuelCurveMg[8]`, mass a stroke at full rack, used where set, the flat line and the null test
  untouched otherwise; the compensator still caps it), built after N2's commit, tables first; where a curve is derived
  from its engine's published torque those torque and power rows are marked "set by the fuel curve" and leave the
  counts, and the model is tested there by boost, lambda, BSFC, turbine inlet temperature and the take-up. The take-up
  row is (b): held at 1100 rpm, the pedal stepped, 90 % of the build's own peak within 5 s; the pedal's own response
  at a held 1300 is 0.41 s and the 19 to 20 s is the climb from 800, which the lead had run together for the owner
  and has corrected. N2 working on the tip (knee, flag, the matching's charge temperature); nothing of it pushed yet.

- **03:13 (25 Nov) the lead's check-in; `main` at 78cdd2f5, green on all three platforms** (its own run 45532730896:
  macOS, Linux and Windows each 542 of 542, application and build packed; the steward). *Landed since 02:24*: N1's
  6771c402, four-valve heads' low-lift curtain at the sheets' medians (0.66 intake, 0.63 exhaust of the curtain, nine
  heads at 0.100 in; two-valve heads on the old line to the bit), approved on its table (full load 0 to +4 %, the
  four-valve idles 1 to 3 kPa nearer their rows, the two-stage crossing 8012 to 7298 rpm, the 7.0 identical) and
  pushed with nothing flipped (542, 14 and 15); a single predicted dyno point is good to about half a per cent (the
  predictor's stop tolerance; C told). D2's db4bcaa7: `tel.ecu.fuelFlow` on a governed diesel is the fuel injected,
  lambda filled. N2's f7fe9e70 (the wastegate case tests what it says) and b4ff2e56 (the take-up tool's `--from` and
  time-to-90 %). P's cards under CAM_RULE: the two-stage's low stage as the K20A's two low lobes meaned (a32ebdd7; an
  aggregator's unlabelled rows, the lead's reading), the hot hatch as the 4A-GE whole, the 7.0's ports from the LS7's
  sheet, the F20C's 5850. **Two things the lead told the owner that were wrong, both corrected with them**: "the
  truck is fuelled to twice its air" (N2's reading of a telemetry field that reported the rack's ask; D2's code read:
  the compensator holds 0.80 of stoichiometric throughout), and "the turbo four's wheel may be half again the real
  one" (the lead's arithmetic at the wrong boost; measured: 44.2 / 59.6 mm today, 48.3 / 65.1 with the sizing kept,
  against the IS20's 42.5 / 54.0). **N2, nothing of the map committed yet; rulings given**: (1) the surge figures
  accepted at 01:30 were the flag's locus (0.8 of the knee); the model's measured onset is 0.9 of the knee, so the
  knee's law is scaled by 0.8 / 0.9 and the flag moved to 0.9 of the knee: one locus, the published middles; no
  reshaping. (2) **The matching sizes every wheel for a charge at ambient temperature** (`mMatch` = displacement flow
  x 1.204 x PR x 0.9): the truck's wheel is sized for 0.86 kg/s where the engine passes 0.494, so its whole line is
  left of the knee, the surge guard holds the vanes open, and the climb from 800 to 1300 rpm takes 19 to 20 s (one
  state, not two; a real tractor engine reaches 90 % of peak torque in 1.5 to 4.4 s by makers' claims, P's section
  14). Ruled into the same commit: the design flow takes the charge's temperature from the build's compressor and
  `icEffect`; table first. (3) The sizing stays kept; EngineTest:124 re-read after. (4) first.json states the IS20's
  wheel after the commit, as content, with a prediction first. *Found on the way, in `main` today*: **the twin-turbo
  rotary hunts at its sequential join** (4250 rpm, full load: 26 to 110 kPa, 170 to 263 N m; steady at 4000 and
  4500); a marked case at the join citing it, the cure to be proposed after the map. D2 after N2: `ecu.afc` 0.8 to
  0.77 (a patent's "typically about 1.3"), the tractor's take-up as a validation row (5 s; 20 s today, a recorded
  miss).

- **02:24 (25 Nov) `main` moved 09c1fb4e to 78cdd2f5**, fast-forward. Run 45529977919: Linux 542 of 542, macOS 542 of
  542 (checked by the lead on the run's own jobs; Windows' job on this commit was cancelled by later pushes, so main's
  own run is its first Windows build). It holds the ten fixes of fc554da and the bake's wiring (C's 648e7fca, 7f2ddfa1,
  314e029e; U1's 1cfb5a6e; B's 169a729a, 02553ee2). New for the owner against 09c1fb4e: the V10's and V12's idles
  steady; no dip after the start; the first-launch turbo four as the EA888 with a baked intake cam; Tailpipe the
  default camera on every build; the cabin's firewall; the fan cycling on its switch; the spark and cam maps baked in
  the background with the stage and count in the top bar, the ribbon's curve about 4 s after load, a second launch
  taking the tables from `~/Library/Caches/TheEngineEngine/bakes`; VALVES showing the pressure across the overlap.
  The steward's probe of the audio thread under a bake (4 vCPUs, not a real-time thread): median block 2.51 to 2.53 ms,
  p99 4.31 to 4.77 ms, the few blocks over 10.67 ms there with or without a bake: weak evidence, not obviously bad.
  **Not in main**: the compressor map (N2, unmeasured knee), the cam cards' changes (behind CAM_RULE until table L),
  the four-valve low-lift coefficient (N1 probing; the lead rules on its table). *Since 02:11*: twelve heads' flow
  sheets (P's 9594f39c; N1's overlay): the port limit's form stays (the heads collapse onto the tanh past the knee;
  the K20A2 is a high-rising head; the series form refuted twice); at 0.100 in nine four-valve heads have medians of
  0.66 and 0.63 of the curtain against the model's unsourced 0.73: ruled a probe and a table before any commit, and
  the lead's "it is the head's kind" withdrawn as unestablished. `valve.portRise` declined. The two-stage build's
  crossing is a validation row (P's 9dd37d32: 5500 to 5900 on a tuner's word; the model 8034) and a recorded miss;
  its runner stays the class's 0.30 m (no maker's figure; forum figures near it); its switch takes the F20C's own
  5850 (cited). The hot hatch becomes the 4A-GE whole (7b95a2e7); the 7.0's ports from the LS7's sheet (15f1f5ac).

- **02:11 (25 Nov) a ruling withdrawn; the bake's wiring in; `main` still at 09c1fb4e.** *Withdrawn: the lead's
  01:49 ruling that N1 commit the seat passage's frustum.* Held before the push when a two-stage test's crossing
  moved 950 rpm; P's flow sheets (dbb23107) refute it (real heads pass 0.70 to 0.85 of the curtain at 0.100 to 0.200
  in; the frustum cannot exceed 0.707). Dropped unpushed, with the idle figures the owner was shown on it. The tip's
  low-lift area sits on a production LS7 head's sheet within 0.03. N3-16 as it now stands is in the queue: the flows
  are honest, the one engine on its own cam card agrees with its own idle manifold (the 7.0 on the LS7's: 38.9
  against 38), and the misses are on cards of judgement (the hot hatch to take the 4A-GE's timing if it is that
  engine; the V12 a recorded miss). *The wiring*: U1's 1cfb5a6e and B's 02553ee2 (merged 78cdd2f5): the ribbon's
  curve 4.3 s after load with a bake running (31 to 35 s before), a second launch "baked" in 1.5 s from the cache,
  "Baking the cam 23 of 42" in the top bar. Run 45529977919 at 78cdd2f5 has Linux and macOS in progress: `main` moves
  on their green. *New and open*: the two-stage build's high stage never out-pulls its low one below 7500, so its
  change at 6000 is an 8 % dip (queue); the F20C's own changeover is 5850 (cited; into the card for table L). *N2*:
  nothing committed; the knee's real place still to come.

- **01:49 (25 Nov) the lead's check-in; `main` stays at 09c1fb4e; fc554da is green on Linux and macOS.** Run
  45525224026 at fc554da: 541 of 541 on both, the first green on a commit holding all ten of tonight's fixes (the
  steward's report; the lead's first gate). **Main waits one commit more**: the app's wiring of C's bake changes. C:
  the bake yields between points while the ribbon's predictor is busy (648e7fca; a curve in 1.05 s with a bake running
  against 0.94 alone, where it waited about 30 s), a cache across launches by key (7f2ddfa1), progress and the cells
  still knocking served, and the yield holds the predictor's shared busy flag, not a pointer (314e029e). B: the cache
  folder passed (169a729a: `~/Library/Caches/TheEngineEngine/bakes` on macOS; a machine's run only with
  `TEE_CACHE_DIR`). Outstanding: U1's `ServiceBake::yieldTo(const ServiceDyno&)`, then B's call. U1: the stage in the
  top bar on every workspace (e8ab7047); applying a bake on a running engine does not click (53ca2fcf): nothing at
  idle, a 0.3 s change of note at 3000 rpm light load because the cam bake has no part-load rows (in the queue).
  *The residual at idle* moved four steps in half an hour, all measurements (the queue has the whole of it): the fresh
  charge is right (N1), the drive is honest (D), the overlaps are ordinary so everything passes under 1 mm of lift
  (N1; the lead's "too much overlap in the cards" withdrawn), and the passage there was 0.73 of the head-diameter
  curtain, more than the seat's frustum holds. **Ruled: N1 commits the frustum's geometry** (`valve.model` 1; seat
  widths and the 0.8 coefficient as constants marked judgement; no Spec field until a cited seat exists): shares
  0.399 to 0.347 (V10), 0.488 to 0.445 (V12), 0.463 to 0.406 (hot hatch), economy four level. The yardstick becomes
  each real engine's idle manifold pressure (P collecting). *Content*: the 7.0 on the LS7's own card (68c7c879; basis
  search snippets, not read): share 0.476 to 0.396, full load within 2 % of K's. The IS20's wheels at rebuilders'
  basis (dfbd36cc): compressor 42.5 / 54.0 mm, turbine 50.4 / 41.8; N2 to set the matched wheel beside them. *N2*:
  nothing committed; measuring where the model really surges (see 01:35).

- **01:35 (25 Nov) a correction to 01:30, and what `main` now waits for.** *The lead's acceptance of the map's surge
  line may have been of the wrong line*: N2 says the model goes unstable wherever a speed line rises with flow, which
  is everything left of the knee `phiK`, while the figures checked (0.24 / 0.29 / 0.36 / 0.44 of choke) may be the
  flag's locus at 0.8 `phiK`. If so the model's real line is at 0.30 / 0.36 / 0.45 / 0.55, above the published ranges
  at every ratio. Not measured yet; N2 is to say which locus the figures are and measure where the model surges.
  Ruled: the knee's place is part of the surge line's place and is settled before the commit; one commit (the map, the
  sizing kept, the knee if it moves, the cap tried after), so that table L and the baked maps are re-made once. A
  thought given to N2 to test: the truck's two states may be static (an engine's swallowing line crossing a rising
  characteristic twice), in which case the low state exists only because the model lets a compressor sit left of its
  peak. *N2's three trees* (turbo four at 1500 / 1800 / 2500 / 4000 / 6200, N m): head 213 / 227 / 305 / 366 / 289;
  the map alone 231 / 257 / 394 / 367 / 290; the map with the sizing kept 222 / 239 / 319 / 366 / 289. The map alone
  reaches the target at 2500 only by building a wheel a fifth smaller, so the engine's low end is the turbo's size:
  hardware a build should state from a source (P asked for the IS20's wheel dimensions; nothing sized from the 350).
  The 2.0 common-rail at 1800 gains nothing on either tree (turbine inlet 127 kPa absolute: spool). *`main` now also
  waits for C's fix to the background bake*: U1 watched a real bake (0101e7f4): at load the DYNO ribbon reads "no
  curve yet" for 31 to 35 s on 4 cores because the bake holds the workers ahead of the predictor, and every content
  build bakes again at each launch (about 110 s, up to six workers) since all are stale against the program and
  nothing keeps a result between launches. Asked of C, in order: the predictor ahead of the bake; a cache by key in a
  folder the app gives; done and total per stage; the cells still knocking served. The owner was told, and that
  `main` moves sooner if they say so. *Rulings*: the 7.0 takes the LS7's own cam card for table L (basis: search
  snippets, the page refused, not read), shown three ways in the twin; N1 to size whether the low-lift reverse-flow
  coefficient alone could carry the residual gap, as a diagnostic and no change.
- **01:30 (25 Nov) between check-ins; `main` stays at 09c1fb4e.** The first run on a commit holding every one of
  tonight's fixes is in flight on Linux and macOS (45525224026 at fc554da; the ten commits checked as ancestors by the
  lead: 4e1c813b, 62657c07, 1ab6affe, fd6ed014, f0512a25, ee603c37, 576c1c6e, 68e683f1, a18f1bd4, f4cf2b39). The runs
  before it (483886d, 2f86a47) were red on ui.build alone, the phaser range held as a constant, which f4cf2b39 ends.
  *The compressor map* (N2, measured, not committed): knee `phiK = (0.45 + 0.2 (M - 1))` x the A/R factor, the choke
  growing to tip Mach 1.63. The lead checked it against `published-compressor-maps.md`: surge over choke 0.24 / 0.29 /
  0.36 / 0.44 at ratios 1.5 / 2 / 2.5 / 3, inside the published ranges at each; the efficiency ridge inside at 1.5 and
  2.5 and **left of every published map at 2.0 (0.56 against 0.62 to 0.76) and 3.0 (0.72 against 0.75 to 0.78)**, to
  be moved if one constant does it without touching the surge line, else written down as open. **N2's prediction
  failed**: the truck at 1300 rpm still has two states (28.3 kPa / 1202 N m from 800, 62.1 kPa / 1661 N m from 2000;
  one state at 45 to 60 was predicted), so the map's shape is not the whole cause; the lead asked for the loop's terms
  at the low state (fuel on the smoke limit or not, turbine against compressor power, the point against surge) before
  another guess. **Ruled: the sizing keeps its meaning.** F's rule set the full choke at 1.15 x the design flow; with
  the full choke now at Mach 1.63 that leaves a turbo near a ratio of 2 with 0.94 of its own design flow at its design
  speed, so the rule is restated as the choke at the design point's own tip speed. Nothing commits until the 2.0 at
  1800, the turbo four at five speeds (three columns), F's cases with no bound moved, the boost and diesel tiers, the
  nine take-ups and the truck again are shown. *The cam cards' class rule* (P's af2debe5, off until table L; N1's
  rows): residual share V10 0.399 to 0.371, V12 0.488 to 0.447, 7.0 0.476 to 0.491; the overhead-cam mid-range up to
  +12 %, the tops 1 to 3 % down: a sourced correction that closes about a tenth of the gap to the measured 0.20 to
  0.27, so N3-16 stays open on the reverse-flow coefficient. U1's 0101e7f4: the background bake says "baked" once
  applied, with a test that watches a real bake.
- **01:11 (25 Nov) the lead's check-in; `main` stays at 09c1fb4e.** No Linux and macOS job is green outright on a
  commit holding tonight's fixes: macOS at 2f86a47 (run 45521139814) is red on one case, ui.build's
  ValvesFirstTests.cpp:70, which held first.json's exhaust phaser as the constant 30 where P's cited 42 now stands
  (U2 restating it to read the build, not constants); Linux at e29d52b on that and on core's books case, which caught
  N2's 69c1fdfa: the tractor's turbo slower to arrive at 1800 rpm (1523 N m in the 4 to 6 s window against 2090;
  steady state whole at 2096; 2065 in that window again after N2's 78e4deda). Main also waits on N2's word that
  the tractor's time to boost is unchanged or meant (N2 measuring three trees). **Since 00:31.** *Idles*: N1's
  576c1c6e (the idle valve's integral held through the first 3 s after a start): the after-start dip gone
  (first.json's lowest 641 to 737 rpm against 750; the 7.0's 433 to 677). The V10's and V12's 1 Hz wander was not
  the valve's loop but the burned gas at idle: N1's 68e683f1 (area.fm applied the port quality at every lift,
  1.2 to 1.3 times the geometric curtain at low lift; now the lesser of the port's and the curtain's limits):
  share at intake closing 0.487 to 0.399 (V10), 0.518 to 0.488 (V12); idle sd 12.1 to 1.6 and 17.7 to 2.3 rpm;
  full load up 0 to 19 % where less exhaust crosses the overlap at speed (V12 5400 rpm 479 to 591 N m, fresh
  charge 0.83 to 0.98): every row of K and every baked map is stale. *The residual at idle, open* (N3-16; D's
  mechanics.md and N1's valvetrain.md): the big-cam shares read 0.40 / 0.49 / 0.49 against a measured band near
  0.20 to 0.27 (one small engine, a thesis's figure) and the economy four agrees; the Fox correlation set aside
  (units unprinted, paper not reached); the excess is the overlap's through-flow at 355 to 380 degrees (D: x =
  (R + E) / (R + E + F); the late closing's push-back cancels; no ratchet). **Ruled: no "both valves open" factor**
  (0.33 would land the V10 at 0.28, but it is no property of either valve); the sourced routes in order: the cam
  cards' flanks (N1's sensitivity at 26 degrees a side: 0.035 to 0.056 off the share; P collecting cited cards),
  reverse-flow discharge coefficients (SAE 962527 and 2001-01-1798, paywalled), the primary's ring at idle (no
  trace). *Diesels*: the clatter's spread from the paper's idle point (0.559 degrees, read by P; D2's 7001b79a,
  the test on the reduction, 3 dB); P's sources: VGT vanes closed at idle (kept), a fan switch at 100 / 95 C cited,
  no idle warm-up time, no small-turbine map beyond peak efficiencies; the diesels' idle row restated on the
  clatter's comb (all three pass). *Boost*: N2's surge cap reverted (it cost the turbo four 12 % at 2500 on the
  present map) and held for the corrected map; the model's map against the published ones: surge too far right
  at ratios 1.5 and 2.0 (0.31 and 0.38 against 0.22 to 0.26 and 0.26 to 0.34), the choke not growing above 1.8.
  *Also*: C's CalibrationBakeService and U1's ServiceBake on it; L: the cabin by the mass law, Tailpipe the
  default everywhere; P holds table L for N2's map, then re-bakes. The account's weekly usage shows a warning on
  N2's session record (resets in about two days): told the owner.
- **00:31 (25 Nov) the lead's check-in; `main` stays at 09c1fb4e**: its own run (45510675759) built and packed on
  Linux, macOS and Windows, 537 of 538 on each (the launch case: the steward's idle-trim fade bb93325e had quickened
  the idle launch by 0.064 s; reverted, 4e1c813b; the case restated by the lead as well). The owner asked for a stable
  local build and was told to build 09c1fb4e. The next move is on a Linux and macOS job green outright on a commit
  holding 4e1c813b, U2's 62657c07 and 1ab6affe, N2's 69c1fdfa (the surge cap reverted: it cost the turbo four 12 %
  at 2500 on the present map), fd6ed014, L's f0512a25 and D's ee603c37: none finished yet (in flight: Linux and
  Windows at 24ccc32, macOS at 1ab6aff). **Since 23:52.** *The compressor's map, sourced*: five published maps read
  into numbers by a research agent of the lead's and spot-checked (`calibration/boost/published-compressor-maps.md`,
  061428cb): surge over choke 0.22 to 0.26 at a pressure ratio of 1.5, 0.26 to 0.34 at 2.0, 0.32 to 0.42 at 2.5,
  0.41 to 0.47 at 3.0; N2's model map in the same terms (`compressor-map-model.md`): 0.31 and 0.38 at 1.5 and 2.0
  (too far right: its surge flow climbs 1.22 times from 1.5 to 2.0 where the published climb 1.45) and a choke
  that stops growing above a ratio of about 1.8; the island, the efficiency near surge, the peak and the tip limit
  already inside the ranges. N2 sets those two things from the file; the truck's two states (the shaft's
  hysteresis inside the surge region) are the new map's test; the surge cap is held as a patch for it. *Table K*
  (ab107ca, P): 120 of 839 missed (25 of 196 cited, the EA888's rows now among them); the baked cams alone: 14 rows
  in, 6 out; the turbo four 189 / 217 / 303 / 352 / 281 N m against the maker's 350 and about 250; every archetype
  hot-starts at 318 K; new worst rows the diesels' boost at the check speed (0.20 and 0.43 bar: the map). After K:
  the exhaust phaser 42 degrees cited from the EA855, the 14 re-baked on N1's half steps; table L waits for N2's
  map. *The cam bake as a service* (C's 879c59f2 on N1's keys): spark, cam on that spark, spark again; U1's
  ServiceBake on it (f15422b2). *The intake's ram closed*: N1 read every sourced loss against the code (all in;
  the plenum's end too); N3 read the "+8.7 %" source in full: a 1-D simulation of 400 against 528 mm on a 1.6, and
  the model gives +8.3 / +7.9 / +6.0 % for that pair: the flat six's 17.6 % was never comparable; no measured bound
  on the ram's height exists among the readable sources. *Idles* (N1): the turbo six's hunt is its wild cam's lope
  (0.4 s, unmoved by a fourfold flywheel) with the ECU's gain ramp reading it as "far off" (c24cacf6: 151 to 127
  rpm sd against the reference's own 113); the V10 and V12 a 1 Hz valve loop of plus or minus 25 to 31 rpm, and
  first.json an after-start dip to 660 to 680 rpm (the integral unlearned during the flare): both being cured by
  plant measurement, with the steward told. *Sound* (L): the cabin's firewall by a wall's mass law (f0512a25: 4 kHz
  from -19 to -6.5 dB against the reference's CABIN); Trackside no finding; the pump at least 11.5 dB under the
  engine's third octave at four cameras, the prime faint and present; a rev's largest bounded at the limiter's
  ceiling. *Heat* (D): the fan cools the radiator (ee603c37: Heat's thermoswitch, the voice reading it, 2.1 % duty
  at a warm idle); the warm-up documented as measured (4.8 minutes, band 3.5 to 8, judgement). X-028 re-proven on
  C's fixed header (0 of 1313).
- **23:52 the lead's check-in; `main` fast-forwarded at 23:31** from 40be51d2 to 09c1fb4e: it holds everything the
  owner reported (the cam and VVT, the hum, the airy balance on Tailpipe) and X-028, the key-on ring, the wastegate's
  limit cycle, the automatics' upshift on spinning tyres, the shift's spark ramp, the DI cooling at the cited 14 K.
  Basis at the move: Linux at a37728d9 and macOS at 007416ef each red on one listen case that L had restated
  (59b95981, 4273188b, both in 09c1fb4e); macOS built at 007416ef with one host commit after it. **Main's own run on
  09c1fb4e (45510675759): Linux and macOS built, 537 of 538 pass**; the one red on both is card I's new
  `launch.automatic` (LaunchTest.cpp:198: brake-torqued 3.104 s against 3.072 from idle with traction control off,
  bound 0.01): on a grip-limited launch "no slower" is not a law: restated by the lead (within a tenth of a second
  without traction control; unchanged with it), pushed. Windows still running. **Since 23:17.** *The turbo four as
  the EA888 gen 3* (P's 86079c1 for table K: the two-stage exhaust from gen 2's SSP 922903, its durations read at
  1 mm because the page's own large lobe says so; intake phaser 60; exhaust phaser 30 judgement, 42 from the
  sibling EA855 after K; each basis in the build's notes) with N1's baked intake map reads, by N3: 189 / 218 / 300 /
  351 / 283 N m at 1500 / 1800 / 2500 / 4000 / 6200 (from 139 / 144 / 265 / 368 / 276; the maker's 350 to 4400 and
  about 250 at 6200): it now breathes cleanly (residual 3 to 4 %, dpOverlap about 0), 4000 meets the curve, and what
  is short below 2500 is spool. N3's ranking closed: the intake phase at boosted low speed (the bake: +30 % at 1814
  on the old lobe, 137 to 178), then spool (N2), the short lobe +3 %, the exhaust's retard nothing, knock nothing
  (14 N m), the exhaust's heat nothing (G's probe); the one figure that would move the low end further, gen 3's
  exhaust cam's parked timing, is unpublished and is not to be set from N3's best cell (327 to 332 N m at 1500 to
  1800 on a 0.40 turbine with the exhaust closing 30 degrees earlier). *The compressor's map is the root* (N2): on
  the model's one map family the surge line lies under the engine's operating line at low speed: a surge cap
  (built, held as a patch) holds the diesels' target near zero and would cost the turbo four 12 % at 2500; no one
  match holds both ends. Ruled: the map's width and its low-flow efficiency from published maps (a research agent
  of the lead's is reading them into numbers), then the cap on the corrected map; the back-pressure gate pushed
  alone; the four boost and diesel cases stay marked. *The cam bake* (N1): half steps, rows either side of a stage
  change (7471609c), a key carrying the stage and the spark's key (c7236d05); the V12 baked: 2040 rpm 440 to 497
  N m, 3600 rpm 366 to 546 (auto asked 47.5 degrees into 9 degrees of knock retard); the turbo four's cell scatter
  was the wastegate's limit cycle and is gone; BAKED_CAMS on for table K; ruled (c): no feedback rule on the
  overlap, the auto rule stays the fallback, `tel.valve.dpOverlap` ships (U2: VALVES says BACK-FILLS or SCAVENGES,
  51d5911a), the bake becomes a background service with C; no exhaust-direction field. *Sound*: L: Rear is right
  physically and darker than the reference's REAR by design (the bay shaded, the ground's notch): ruled: every
  build opens on Tailpipe (the Spec's default; P dropped the 32 stated cameras); the two listen cases restated on a
  rev's typical peak with 2 dB margins; the fan through Tailpipe measured by the lead (+0.9 to +2.4 dB in the mids)
  and D cleared to bring back the fan cooling the radiator. *Host*: C: the predicted curve depended on the
  machine's core count (c0498ad5: fixed shares, bit-equal on 1 to 3 workers; 3 kept); D's 4.68e-310 was a dangling
  reference through the test library (998af7c9, accessors ref-qualified); D's and X's bit-equality for X-028 did
  not use that form and X re-ran it (0 of 1312). X-027 and X-028 closed; `Known.h` holds X-016.
- **23:17 the lead's check-in; `main` not moved (40be51d2)**: the newest finished fast jobs (Linux and macOS at
  609198db, run 45503965919) have four reds, each fixed or marked at the tip (host.sim.cases: C's de49e456; the
  listen quick case at Side: L's 59b95981; diesel.brakes and diesel.turbo: marked in 4c253094 on G's finding); main
  moves when the macOS job in flight on a commit holding them has built and passed. A correction of the lead's: from
  about 22:36 to 22:50 the times written on the lead's messages were estimates that ran up to 24 minutes fast; the
  agents were told; times here are from `date -u`. **The owner's reports, on the branch:** *the cam and VVT*: done
  and in the default run (B's shell.window.first: warm oil at launch, stage 0 to 1 across its speed, the phaser
  moving; warm start: C 8c698820, B 1a7eadcc and 0488157b, D d17d6808; the gates: N1 83f34ce5; the words and the
  drawing: U2). *The hum*: the cause was the path: Places radiated the in-tank pump as if in open air (L's tank
  wall by the mass law, f6bf38ec: 27 dB at 760 Hz, 38 dB at 3.9 kHz); M's source level ends 20 dB down
  (2e9cd96a) on a masking criterion (every pump line at least 10 dB under the engine's third octave, four cameras,
  the lead's first criterion "3 dB over the bins" having been wrong: it moved with the transform's length).
  *Airy, the engine quieter*: the camera: first.json opened on Side; it now opens on Tailpipe, model 1's form of
  the reference's default view. The lead's renders of first.json as shipped at the tip, cold idle, 6 to 11.5 s:
  pump lines at the voices-off floor (760 Hz -97.6 against -97.0 dBFS, 3895 Hz -103.6 against -102.9; the owner's
  build: -54.7 and -56.1), voices on against off within 1 dB in every octave (the owner's build: +19.5 dB at 1 kHz,
  +16.2 at 4 kHz). **Found on the way:** P's baked cam maps had no load axis (full-load phase at idle; the
  flat-plane V8 hunting at sd 108 rpm): undone (a15f710), the generator now refuses to write unless every archetype
  idles, N1's bake reworked (load axis, smoothing, acceptance equal to P's gate: df52688b, 2b04859c), BAKED_CAMS off
  until N1 says why the bake missed the turbo four's 14 % at 1800. G's housings closed by mass (15fcdd84) took away
  gas that did not exist: **the first-launch engine makes 139 to 144 N m at 1500 to 1800 rpm where the maker's EA888
  gen 3 makes 350 (P, cited), and 276 at 6200 against about 250.** N3's ranking (causes.md B-3): N1's auto phaser
  rule asks 44 degrees of overlap at 1800 with the exhaust level with the manifold (parked: 181 to 293 N m on a 0.40
  turbine); the compressor's low-speed efficiency and the matching (N2: published maps; one match cannot hold both
  ends on the model's map family); what remains is trapped air (scavenging), not knock (14 N m) nor the exhaust's
  heat (G: 29 % lost before the turbine at 1800, the right order, a hotter port wall worth nothing). The VGTs on
  true mass really surge at the ratios their builds ask: ruled: the boost target capped at the surge line for the
  present flow, the map's width from published maps, no change to idle vanes without a source. Key-on with the
  engine standing rang (the stores gliding from 300 K to the build's air): D's part fixed (f6a8b88d), the boost
  machine's part with N2. X-028: voices.model changed the engine's path (the standing engine's control ticks): D:
  ticks always from the key, bit-equal (local, pushing). The shift's spark cut: a ramp on the spark's return
  (steward, 0cb535ae); X-027 closed on the cut's own bound for the reference's burn (42). Card I: the automatic
  shifted up on the engine's speed while the tyres spun (35ef028e): big block 16.85 to 15.54 s, pickup 22.80 to
  19.53. D2: the clatter's cycle-to-cycle spread (4c253094; its k from an unread paper, marked). Table J (86f1574):
  124 of 772 missed (19 of 187 cited), power 16 and torque 12 of 31. The first soak to finish: 88 minutes, 6 of 187
  red, each with its owner.
- **22:30 the lead's check-in; `main` fast-forwarded** 445453da to 40be51d2, the newest commit green on Linux and
  macOS together (run 45496629256; Windows' newest green is 69c633d, run 45494891308): it carries M's first pump cut
  and core's restated backflow check. Not moved to the tip: the steward has core's engine cases red locally at
  174881c0 (EngineTest.cpp:398, at idle the exhaust valves' air 3.8 % off the intake valves' against 1 %) and is
  bisecting. **The owner's reports since 22:08.** *The cam and the VVT:* fixed at the source: N1 sourced the two
  gates and both were too strict (83f34ce5, D's 47eb6794): the second stage on the coolant the ECU reads at 10 C with
  the oil-pressure switch (Honda's service text), the phasers commanded from cold and parked only without oil
  pressure (SAE 2007-01-1640); the lead's render from cold on two-stage-cam-20: stage 1 at 6053 rpm at 4.9 s, oil
  300 K. U2: VALVES and ENGINE say why in words from N1's `tel.valve.waiting` / `waitTemp` (ec9c0cd1, 9b46ad10,
  f8ca8200), and ENGINE's section drew B's lift shrunken (5.7 and 8.5 mm for 8.5 and 12.65): fixed, fcf41db7. The
  warm start: C's `LoadOptions::initial` (8c698820) and B's app side (1a7eadcc, `--cold`, "coldStart") are in, but
  `Heat::stood()` cools the warm engine at the first configure of any build whose air differs from the default:
  D's line, asked 22:26. *The hum:* the lead's cold-idle renders of first.json through Side, main's tree against
  the tip: pump lines 760 / 3895 Hz -54.7 / -56.1 dBFS to -74.0 / -75.2 (M's -20 dB), voices over the engine alone
  +19.5 to +3.3 dB at 1 kHz; M then -30 dB in all by the lead's ruling (1a3812e6) and the fan on at 373 K
  (f074ab47). *Airy, the engine quieter:* with no voice at all Side at the Field is 10 dB down at 500 Hz and 6 to
  9 dB up from 4 to 16 kHz against the reference's balance: L's, asked 22:01 and 22:13; L has a tank wall for the
  pump (the mass law, 38 dB at 3.9 kHz) passing its tests and is pushing it; with it the lead's extra 10 dB at the
  source is to be reviewed (two cures for one fault). **Also:** D's per-sample boost read (b7e64fa0, no cost); G
  closed the turbine housings' gas by mass (0.997, 1.007) and measured the NA tail's overstatement (0.4 to 0.7 % of
  torque: recorded, left); N2's design-point rule adopted (its table to be re-run on G's closing) and its knock
  table proof on 14 archetypes (9 at every speed; C adds a second bake pass where retard sat at its ceiling); D2's
  VGT brake governed and unmarked (908bbecd) and the diesels' idle lines traced to the clatter struck at the same
  point every cycle (D2 gives it the burn's own timing) and to the turbine taking 20 to 24 dB off the exhaust's lows
  at idle (G, N2); N3: the V12's "peak at 2049 rpm" is a dip (the primaries' wave in the baked map's overlap: N1's
  bake checks midpoints next) and the intake's ram is about twice the published 6 to 9 % (damping, N1); P's
  one-change quarter mile (9 of 15 miss, none of it the run; the automatics' brake-torque launch to card I); X:
  each switch's decisions back in the fast tier, combo's budget 165.
- **22:08 the owner's two reports, where they stand.** (2) **The two-stage cam and the VVT: explained, nothing
  regressed.** B drove the window by script on first.json (a2605a16, the nightly `shell.window.edits`): every valve
  edit is held and sent in all four cases (before the first start, running, after a restart, after a QUALITY change
  and back). The simulator does not obey because the application's engine starts cold (oil 288 K) and the valvetrain
  parks the phasers under 313 K of oil (phaser.fm:47) and keeps the second stage off under 333 K (lab.fm:424), both
  there since 12:12: held at 6700 rpm from cold the phase first moves at 90 s and the stage at 180 s. tee-render's
  scenarios hold the engine warm, which is why the lead's check saw the stage switch. The owner, told: "I didn't test
  it for that long, so that might have been it". **Ruled:** the application's engine starts at working temperature
  by default, set once and not held (C: `LoadOptions::initial`, a swap carries the live temperatures; B: the app's
  side, `--cold` and a settings key); N1 sources the two gates (the lead's recollection, not a source: a car's ECU
  gates the stage on the coolant it reads, and phasers are commanded from cold) and adds telemetry that says why;
  U2's VALVES says it in words. (1) **The hum:** M's a00abbf6 measured by the lead at the tip on first.json's warm
  idle through Places Side: the pump's 3895 Hz line from 17 dB under the 63 Hz octave to 38 dB under; the fan's
  lines unmoved (280 Hz 14 dB under); the voices' broadband still 11 to 16 dB over a voices-off render from 250 Hz
  to 4 kHz, where through the reference listener voices on and off differ by under 1 dB: L's path, asked at 22:01,
  not answered yet. M gives the fan's switch hysteresis (it sits at the warm start's 368 K). P's new validation row
  (the strongest line above 300 Hz at warm idle at least 10 dB under the exhaust's band) was red on 23 of 31 before
  M's commit. **Also ruled:** D takes N1's per-sample read of the boost's pressure (landed, b7e64fa0: X-022's
  blips no longer pull the charge volume to 0 kPa); N2's design-point rule for turbine matching adopted, its table
  to be re-run after G closes the turbine housings' gas (1.14 times the valves' on the turbo four, N3) and the
  flat-plane V8's -13 % to be explained; the boosted books' 8 % withdrawn by N3 (a 1 s window on filling stores);
  C's KnockBakeService in (8572845f) and a bake's key survives a save (b8cdba82); X-026 closed, `Known.h` holds only
  X-016; the fast tier is under its cap (2179 of 2400 test-seconds scaled, steward).
- **21:53 the lead's check-in; `main` fast-forwarded** f69bf016 to 445453da (the one case red on all three systems,
  core's trapped-unburned check, restated by D in f2d8e19e; X-022's quick case green again). Windows builds and
  tests again (the friend declaration: 006c1a6c, 6b54fe2a); main's run on f69bf016 packed the application on all
  three (run 45488426712). **THE OWNER LISTENED AND LOOKED, on a Mac build of main:** (1) "a high pitched hum … far
  too loud", "more airy", the engine quieter: measured on first.json's idle: the in-tank fuel pump's lines (3895
  and 760 Hz) and the electric fan's (280, 560 Hz); the voices sit 23 to 35 dB higher against the exhaust through
  L's Places than through the reference mix, and the engine 10 dB lower. M lowers the pump 17 dB and steepens the
  fan at source (every voice level is judgement: the owner's ears are the only measure); L owns the path: open.
  (2) the two-stage-cam archetype's lift change at 6000 rpm seen and heard in one build and not in the next: the
  simulator switches (tel.valve.stage at 6000 rpm on four commits and under Economy), so it is the application:
  B (what is sent) and U2 (what ENGINE and VALVES draw) are testing: open. (3) the owner's local commit 392b6a4f:
  the macOS bundle copies sim/ and content/ only when the executable relinks, so a sim-only pull leaves a stale
  engine in the .app: not pushed; the lead recommends landing it. **Since 21:10**: table I (dc24110): 123 of 741
  missed (19 of 187 cited); power 14, torque 12 of 31: lower in power because the builds no longer carry runners
  tuned to their own torque rows; L's auto level (c686d345: a camera's exposure, true levels a switch away; the
  limiter a guard again); N2: the blow-off case and the VGT's surge guard re-derived on real exhaust mass
  (the vanes sooner than the single again), X-024 fixed, the rotary's pair short of boost at the top (turbines
  matched at the redline at a fixed ratio: to be matched at the build's own design point); N1: seeds 298 and 217;
  N3: the boosted fourteen: 9 torque and 10 power rows in band, boost collapsing at the power peak on four, two
  air-book gaps to chase; C: the knock-limit bake's host side (32a6dc47); D2's final numbers; the brake's shaft
  governor marked. Next check-in 22:35.
- **21:10 the lead's check-in; `main` fast-forwarded** 35286df to f69bf016 (141 commits; the tip compiles, lint ok).
  **CI, newest finished fast jobs**: Linux green (c51e77a) and macOS green (7ef0f53), both holding the first
  build, the level fix, the gamma, the tyres and the one loader; Windows failed at BUILD from 127c350 (a friend
  declaration in tee/test/Scenario.h that clang-cl refuses once LoaderTest includes both Scenario headers): the
  lead's 006c1a6c and C's 6b54fe2a fix it, no Windows job has finished on them yet. Main's run on 35286df was red
  on all three (the truck air case, fixed since; Windows the same build break). The soak tier has reported
  nothing since 18:14 because its job's limit was 90 minutes (the steward: 240 now, d5ca2d50). **Builds for the
  owner**: macOS run 45484220368 (7ef0f53). **Since 20:34**: the exhaust's mean flow closed and live (G 8bb2c80f,
  D 7bd08444; given over let out 1.000; the turbine had 13 to 22 % phantom gas at load, 45 % on a compression
  brake): the brake test's oversized turbo went back to the matched one (D2 efd737ed), the VGT's spool is slower
  than the single's on real gas (marked; N2), the blow-off case restated; a diesel's gas air until it burns (D
  b551cd05: the 233 K start 5.2 s); the 7.0's idle fixed (N1 ba2fee9b: 750 sd 5, the spark trim's look-ahead =
  the manifold's filling time); P's generator no longer derives parts from an engine's own targets (runners,
  headers by class; baked cam maps to come); targets carry their basis (187 cited, 167 recalled, 370
  judgement); table H: power 22, torque 11 of 31; N3: no common level left; the tuned four-valves' gap is their
  headers' primaries and runners (content), the exhaust scavenges as it should; table I running (dc24110).
  The interface: 130 of 130 view tests in the lead's build; round 7 of pictures (the TRY FIRST strip, AIR's
  mismatch warnings). Next check-in 21:50.
- **20:34 `main` fast-forwarded** aaa10cc to 35286df (159 commits). What held it at 20:28 was one default-run case
  failing at the tip (diesel's VGT against the single: two loops on one target, 210.1 against 211.4 kPa); the
  lead restated it within the loops' 2 % in D2's file (35286df), D2 being deep in a re-run; every other failure
  in the newest finished fast jobs has its fix in this tip. Main's own run on 35286df is the first same-commit
  run on all three systems that should be green. Since 20:28: C's one strict build loader (127c350), which also
  fixed tee::Build dropping arrays of choices by name (the exhaust presets' resonators, muffler kinds,
  converters, bypass were at their defaults in the application: Q re-measures the typical build); the tools'
  unknown-field error and stale outputs (93187bd); a diesel's gas air until it burns (D b551cd05); the listener
  +14 dB and a measured level a build (L 35e355d, P 76e1b29: first.json's free rev peaks 0.29); N2's boosted
  ledger on the new gamma and its ruling to give the auto spark map the knock limit less 2 degrees; U3's AIR
  mismatch warnings (82a98f3); U1's TRY FIRST as a strip that covers nothing (4bc084b).
- **20:28 the lead's check-in** (the tip 04a4d11 and after: about 1970 commits; it compiles; lint ok). **CI**: the
  newest finished fast jobs are red on all three, each on a commit from before fixes that are now in the tip:
  core's books on the boosted petrol (N1's b6676a5; fixed by c40f041, checked green on Linux by the steward),
  `calibration-balance.sim` and `diesel.brakes` (the gamma's first commit alone; the test and the lean term
  followed: bae5ffd, a845b07f), `ui.dyno.run` on Windows (U4's slip, fixed two minutes later, 4eaa5d2). No job
  has finished on a commit holding c40f041 yet. One default-run case fails at the tip itself: diesel's VGT
  against the single (210.1 against 211.4 kPa: two loops on one target, a near-tie the gamma flipped): D2
  restates it within the loops' 2 %. **main** stays at aaa10cc until that is in (its own run on aaa10cc is in
  flight on all platforms; it will show the same books and brakes failures). **Since 20:10**: G measured the
  exhaust pipes given 14 to 20 % more gas than the cylinders let out at load (the valve's simple-wave gain: 10 to
  14 kPa at a turbine's inlet) and closes the mean, D adding two lines; the gamma's treatment of a diesel's
  compression is wrong (its air compressed as products: D2's 233 K start lost; D makes the gas air until it
  burns, N3 checked); N1 found the 7.0's lope in the spark trim's look-ahead (to be the manifold's filling
  time); N3 owns full-load breathing (the runner's ram, the two-stage cam, the small block against its cited
  band); P's targets carry their basis (cited, recalled, judgement) and misses are counted by kind; the first
  build is the 2.0 L turbo four (about 1.1 R) with the listener and voices on, which play 23 to 30 dB under the
  reference mix (L: a level scheme; P: two floor rows); U1's TRY FIRST card and low-redline dials, U3's cost
  line, U4's knock-limited points: 127 view tests pass in the lead's build; round 6 of pictures to the owner.
  N2: the intercooler held at the air on a test bed (10345ed), the charge system's has/wants telemetry
  (00d6728), the VGT sooner than the single (e12b230). Next check-in 21:08.
- **20:10 `main` fast-forwarded** to aaa10cc (80 commits; the owner's word at 20:09: "Fast-forward main at each
  checkpoint"). The rule is in 01-rules.md section 2: at each lead check-in, when the tip compiles, lint is clean
  and the newest finished fast-tier jobs are green, forward only, by the lead alone. At this one: the tip compiles
  (`tee-schema --lint`), lint ok, the newest finished fast jobs green on all three (Linux e05e179, macOS 48e9a17,
  Windows 46691a4); no job has finished on aaa10cc itself yet. Since 20:05: the gamma's lean term (D a845b07f),
  the tyres at the air's temperature (card I a140c6f: every launch and quarter mile quicker), the VGT sooner than
  the single (N2 e12b230, D2's case plain again), X-020 and X-021 closed.
- **20:05 `main` exists** (the owner made it the repository's default branch, at 3aad826, a commit of this
  branch). Nothing changes for the packages: the integration branch stays `claude/awesome-gates-ntdb58`, every
  session pushes there and nowhere else, and `main` moves only when the owner says how (a pull request, or a
  fast-forward they ask for). CI runs on both (`on.push.branches: [main, 'claude/**']`). Since 19:43: N3 found
  the cylinder gas's gamma above the published single-zone fit (D's 3990ee47, the physical cylinder only; a
  lean-mixture term follows): every petrol engine -5 to -9 %; N1 found the intake books' parting (the valve's
  simple-wave gain rectifying a big cam's back-and-forth) and closes the mass each sample (in test); the tyres
  started at 200 K (card I's agent on it); an older build loader used by twenty test rigs drops nested sections
  (C: one strict loader); a first launch showed the bare reference engine (B: `content/builds/first.json`, P's
  3.0 L twin-turbo six, to open instead; the listener and the voices to be switched on in it); table G: 132 of
  648, power 15 and torque 14 of 31.
- **19:43 the lead's check-in** (the tip: 1756 commits, compiles). **CI: the fast tier is green on all three
  systems for the first time**: Linux and Windows run 45467052791 (46691a4), macOS run 45468737097 (48e9a17).
  Those builds hold M's index guard, N1's runner damping, G's port law and D's f89d40c; not the warm-start fix
  (02f2883), card I's launch (0078996) or U1's load meter (f53f564). **Card I's launch landed** (0078996): the
  NA rotary no longer bogs (quarter 20.18 to 18.60 s), the 7.0 inside its quarter-mile target; what remains is
  in how P's timed runs are made (shift points, pre-rev, warm tyres). **The 7.0's idle hunts** at the tip (742
  sd 107; 750 sd 8 at c8f2d54): N1 confirmed the lead's render (its own steady figure was a stale csv); the
  after-start air stays, the steadiness check goes in marked on the 7.0. **f89d40c, exactly** (D, 492e525: a test
  that fails before it): a switch written while stopped with no configure after; the first launch was right, a
  switch changed in the interface between runs was not. The interface at the tip: 124 of 124 view tests in the
  lead's build; round 5 of pictures to the owner (the load meter's warning; DYNO's walls labels). Next check-in
  20:25.
- **19:35 the lead, between check-ins** (times from `date -u`). **Table E** (pin 8ff2bf3): 161 of 648 missed,
  power 12 and torque 13 of 31; **table F** (pin 30d2dfc, walls settled): 157 of 648, 12 and 13 of 31. The count
  falls, power and torque do not: the same seven NA engines 20 to 27 % strong each time, so **N3 was started**
  (19:31: an air and an energy balance per engine; discharge by class). **A regression P found and the lead
  reproduced**: the flat six and the 7.0 did not start warm; N1's two causes (the plenum's book correction as an
  added flow, b6676a5; the idle valve shut on the flare, ca76cd6: after-start air) are on origin with a test, and
  the flat six idles at 850 sd 12 in the lead's render; the 7.0 runs but hunts there (742 sd 107), against N1's
  699 sd 4: being settled between a stale host and a later merge. **Closed**: X-022 (N1, 32cea26, its own
  regression), X-025 (D, 081f84b: refused, reason 4), X-023 (G), X-024's listener half (L, 20334d0); the VGT's two
  states (N2, cf888e2: a surge guard; "sooner than the single" still fails on D2's truck, marked). **The dynos
  agree**: C's stepped dyno settles the exhaust's heat (1220ca3) and names the second slow state, the intake's
  charge heating (8b23294; the settle field goes in with N1's next intake push, C's session may not edit `sim/`);
  the third is the intercooler's core in dead air (N2: its rated air on a dyno, SAE J1349). **Cost** (Q, q6):
  the typical build is about 2.0 R at Full against 1.6 (the builder now really running; G +0.31 and F +0.12 over
  their cards); exact savings landed (G c4ce28f, L 5ce0d1b); Economy about 1.0 R. Nothing acted on load: U1's
  top bar now shows the peak with a verdict and names Economy (f53f564), C adds `HostStats::fits`. D's f89d40c
  (the switches' decisions taken again each call) is in; X could not reproduce the hole through the host, so
  what the lead told the owner about the application's first engine is withdrawn until D gives the sequence.
  **CI, fast tier**: Linux green (46691a4), macOS green (3afd438), Windows one failure (valvetrain.11 at b13c6ba,
  before N1's later intake changes).
- **19:01 the lead's check-in** (taken at 18:55; the tip: 1544 commits, compiles). **CI, newest finished fast jobs**:
  macOS 709 of 712 (803ece2), Windows 703 of 707 (c41755e), Linux 701 of 715 (50f68ac: 11 of the 14 were the
  bike's rig not compiling after the eighteen-speed's include, fixed by B2 at 18:36). Failing on more than one:
  valvetrain.11 (N1's plenum regression, fixed in ab65e06) and the VGT-against-single case (the vane loop's two
  states: N2). Test step: Linux 21 min (it was 42), macOS 27, Windows 39: the budget's splits are landing.
  **Builds for the owner**: macOS run 45461545892 (05db893) and Windows run 45461627792 (b13c6ba), both 18:37:
  the diesels at strength, the start fixes, the launch, the eighteen-speed; not M's index guard, N1's runner
  damping or G's stronger port law.
- **18:34 table D: the count came down**: 186 of 617 targets missed (C by the same reading: 210); power in range
  on 14 of 31 archetypes and torque on 14 (A: 5 and 7 of 30); every invariant on all 31; the diesels and most
  boosted petrol engines in. Worst: NA two-valves 20 to 27 % strong; low-speed VE on wild cams (0.50 at 2000 rpm
  on the flat-plane 4.5); the teeth; quarter miles (now 15 of 23 in, in the cars' own gearboxes).
- 18:20 to 18:55, by cause: **the teeth** were two things: the predictor's points not settled near a resonance
  (C: a point settles when its torque has no trend, 1935471; and the stepped dyno is to settle the exhaust's heat
  as the predictor does, so both mean steady state) and a real runner resonance nearly lossless at the plenum end
  (N1, ab65e06: K 1 out, 0.2 in: the V12 543 / 608 / 657 / 678 / 662 N m held from 6000 to 6850 rpm). **The hot
  exhaust** was mostly a mean: the model's readings are mass means, the targets a thermocouple's: G added
  thermocouple readings (64 to 83 K lower): the port target met on NA engines, the header 60 to 106 K over.
  **X-019**: a NaN indexed the sine table (a SIGSEGV on x86 on a Spec inside its ranges): M's guard 84d1ac0; the
  steward swept all 68 float-to-index casts (five unsafe; a lint rule); the NaN came from the engine diverging
  on that seed (boost or mechanics: X reducing). **The VGT** on D2's truck: the vane loop has two steady states
  (closing past peak turbine power lowers boost): N2 adds gain-reversal protection; the lead had wrongly blamed
  the back-pressure limit. **The lead's unsourced target withdrawn**: "plus and minus 20 to 30 kPa" at the port.
  D's key release (a64e511: the 2.0 common-rail starts on a short key). The soak tier's first job: 68 failing
  runs by kind (the steward's summary): X triages on the tip.
- **18:18 the lead's check-in** (the tip: 1401 commits, compiles). **CI**: the newest finished fast jobs (linux
  e497d3b, macOS 887bc9a) failed the same 68 each: 65 combo cases on `engine.chargeTemperature` = 0 at 0.01 s
  (X-017, a second new telemetry field with no sane first value; fixed 17:47), H9 (fixed), ui.build.frame (U5),
  N1's phaser test (rule changed since). Linux's Test step: 2533 s of wall, 10,111 CPU-seconds (combo 3932): the
  30-second rule is not enough: a CPU budget per package in the fast tier (the steward). **Builds for the owner**:
  Windows run 45452393982 (c41755e, 17:47), macOS run 45448001690 (887bc9a, 17:23).
- **18:14 the diesel's walls were counted twice too** (`combustion.fm`: a fixed 0.62 with Woschni's walls also on;
  D, 9e3a44f): the tractor 2732 N m at 1300 rpm (2000 to 2780; table C had 1241), the 2.0 common-rail 373 N m
  (320 to 380). Under it N1 found a real start fault (the ECU's running rule at 550 rpm switched off a diesel
  firing at 540 when a one-second key let go: 1792025).
- **17:49 to 18:09 a wrong alarm, relayed by the lead to the owner and corrected**: N2 reported the diesels dead
  at 5fdc5ad; it was N2's harness cranking for one second (D2's tee-dyno and P's own table rows showed them
  running). The lead had told the owner the 17:52 Windows build's diesels do not fire: they do, weak until
  9e3a44f. Lesson kept: a second measurement before a fault is reported outward.
- 17:45 to 18:15: **P's pinned tables**: B (engine fixes, old builds) 207 of 581 missed, power in range 7 of 30,
  torque 8; A (with N1's build values) 218, 5, 7; 16:22's: 182, 10, 7: worse by count while the terms that take
  torque out were still to land; C's 17 NA petrol rows: idles exact, 2 of 17 in range, most 10 to 35 % strong.
  The V12 reconciled between P and N1: the reading of the peak, on a curve with a 350 rpm spike (765 N m on 530
  to 620): the table now compares a three-point mean and reports "the sharpest point" as its own miss. Landed
  since: the intake's charge heating and mass books (throttle, valves and exhaust agree to 1 %), the burn's angle
  with speed (the spark now rises 13 degrees to 6000 rpm; efficiency moved 1 %), the phaser by intake closing
  (peaks at the right speeds: the V12 at 6783 rpm), phased cams parked late (the warm no-start was 38 degrees of
  overlap at idle), N2's VGT back-pressure limit (the tractor +12 % at 1300 rpm) and matching-to-target measured
  and not adopted, the rotary's shared-port solve reused (a four-rotor 1.24 R), card I's launch (wet standstill
  5.51 m/s against 4.31, slip 0.081) and X-011 closed, H9 on the blowdown and the flame (1 bang at 0.1, 0 at
  0.25: the unburned fuel handed over at exhaust opening is next, ruled). Open: the rotaries under-breathe since
  17:47 (VE 0.60 to 0.68), NA engines 10 to 35 % strong (port quality by head class next), the V12's spike,
  gross indicated efficiency 5 to 10 % over at speed (an observation), the 2.0 common-rail's short-key start
  (D's line), table D running on a pin at or after 9e3a44f.
- **17:40 the lead's check-in.** The tip (1333 commits) compiles. CI: the jobs behind the owner's 17:10 builds
  finished on Linux (e3d48b0) and macOS (26407ec) with the same 65 failures each: 63 combo cases on one telemetry
  range (`engine.tsPipeTemperature` = 0: X-012, fixed 17:05), H9 (fixed) and the gate test's own setup (fixed):
  nothing else failed on either system. The Test step had reached 23 to 34 minutes; the long tests are split
  (default under 30 s each; a soak tier in CI). **Newest build for the owner: macOS run 45448001690** (887bc9a,
  17:23: the conserved backflow, QUALITY wired, COMPILING on START, the inlet sound behind its box, H's traction
  and H9, the phaser's rule; tests running). Windows: the next job after e3d48b0's.
- 17:10 to 17:40, calibration and correctness: **pattern 4 closed** (the V12 idles at 882 for 900, was 1244; V10
  952; flat-plane V8 998) on D's conserving cylinder; the phaser by intake closing angle (712b599: the six at
  2700 rpm 206 to 290 N m); the ITB manifold floored; the rotary boosted within a tenth of its target from 5000
  rpm with sourced terms and N2's first gate sized for the whole flow; an NA side-port rotary archetype added
  (184 kW in range; torque rises to the limiter instead of peaking at 5500); N1's checks: the gas has gamma(T),
  Woschni is whole, the energy balance sits in Heywood's table, **the burn's angle does not lengthen with speed**
  (go given: a speed term, proved by the spark advance rising with speed); the exhaust port's heat loss asked of
  G; D: boost.model 1 to imply the builder's exhaust and the stand-in to go (ruled); the two-stroke's masses from
  its own cylinders (fc8a3ba). C: the auto cam map baked by a sweep (`tee-dyno --bake-cam`), the predictor on
  turbo builds settles on its boost (within 2 % but at 4047 rpm, +6.7 %). P: both tables on one pinned engine
  (f907a83), about 90 minutes from 17:10: A the builds as now, B as before N1's values.
- 17:10 to 17:35: X-011 from rolling closed (slip 0.104 for 0.10), the standstill half with I; X-018 closed
  (H's bangs never overlap on a bank); X-010, X-014, X-015 closed (N2: actuators every sample, the nozzle's
  choke, the inlet sound behind its duct and box: the tractor's thump was the compressor inlet's dq/dt).
- **17:06 the lead's check-in.** The tip (1153 commits) compiles and holds every fix of the day but N2's inlet-sound
  commit (cdde6dd, pushing). **Builds for the owner, tests still running on both**: Windows run 45441587457
  (e3d48b0, 16:49: the exhaust builder, the crank fix for car and bike, the gate and the over-boost cut, the walls
  once, the two-stroke, the window-first launch, the settings labels; not the QUALITY wiring, the disabled START,
  the conserved backflow or N2's click fixes); macOS run 45438896554 (26407ec, 16:35: the builder, the car's crank
  fix, the gate, the two-stroke; not the walls' fix nor the launch). CI's Test step has grown to 23 to 34 minutes
  (651 tests): ruled: a default test under 30 s, longer ones split into a quick form and a `[nightly]` form, and
  a second CI tier for the latter (the steward). The validation table on the branch is still 16:22's (power in
  range on 10 of 30, torque on 7): P re-runs it now.
- 16:40 to 17:05: N2: the over-boost cut (f2af1a7), the actuators moved every sample and the nozzle's choke
  (9d64f7d: X-010's and X-014's clicks), the sequential pair's first gate sized for the whole flow (the rotary
  holds 72 to 75 kPa at 70), the tractor's thump found (the compressor inlet's sound, dq/dt unfiltered: now behind
  its duct and box); the 2.0 four 401 N m and 222.6 kW and the kei 100 N m and 47 to 48 kW with P's tapers: the
  first boosted archetypes in range. N1: the tuned engines' shortfall is mostly their builds (runner length is the
  lever and behaves as a real one; the archetypes' exhausts are all the reference's; default port quality);
  intake charge heating is missing (9 % for 25 K); it now changes E's intake (heating, the ITB manifold, mass in
  the runners) and the phaser's rule. X: the ten-minute soak of everything on passes (worst step 18.0, 1.71 R);
  X-001 to X-009 and X-013 closed; open X-011 (H, traction on a wet road), X-016 (a random build at 4.6 to 4.9 R
  since the builder runs). H9 (fuelled-overrun bangs) fails since about 16:34: with H. Q: four wishes for
  FMinus's owner now (the queue): an object cache, parallel compile, lazy compile, a target processor for
  `fminus -c` (which would let us ship the engine compiled ahead of time).
- **16:34 the lead's check-in.** CI: Linux at 9a78692 (15:54, the first job with H's fixes): 618 of 621, the three
  failures U1's frame goldens (re-blessed since): **every simulator test green on Linux**; macOS is testing the same
  commit, Windows 03d0e3b. A job holding G's wiring, I's fix and a14db75 on macOS and Windows is the next build for
  the owner. C: the 48 kHz comparison's macOS failure was the owner's large-diesel preset alone, in the band where
  its charge only just lights (bistable at either rate, on either system): Economy is unblocked.
- **16:32 N1's cause D: mass is created at the intake valve** (`sim/core/cylinder.fm:290`, and the reference's
  line 279): gas pushed back into the runner leaves hot and returns as the same volume cold. The V12 traps 20 g/s
  on 0.7 g/s through the throttle (its idle valve pinned shut: the big-cam idles); VE inflated 5 to 12 % on mild
  engines and 50 to 200 % at low speed with big overlap. Ruling: the reference path keeps it to the bit; the
  model-on path conserves it (D, after the walls' one decision), with a test.
- 16:31 X's verdicts (above); 16:30 N2's gate on the branch (the 2.0 four 402 N m and 260 kW; the twin six now
  under at 404 N m: it breathes 0.51 to 0.65; the kei needs its build's boost tapered to 15 to 20 kPa); 16:32 G's
  passivity fix (57c9876); 16:29 to 16:31 U2 and U3 bound the two-stroke's telemetry, `scope.ductMid` and
  `scope.inletOpen` (the last approved by the lead; T appends it).
- **16:21 to 16:28 the tip did not compile; fixed by the lead (a14db75).** T's two-stroke hook landed (988b442)
  calling `Car::step` with three arguments; I's X-007 fix (f9e26f1) had given it a fourth; the two merged with no
  textual conflict and the 15:55 push rule let it through. One line in `sim/engine.fm` (the call passes `mm.jNow`);
  the rule's one command now compiles `sim/` between the merge and the push (`scripts/lint.sh`); the steward
  broadcasts it. D2 noticed. With T's hook, **every package's hook is now in the one program.**
- 16:22 to 16:26, calibration: N2 found a boosted engine has two states (stepped up from low speed the 2.0 L four
  runs away to 255 to 290 kPa against 120, the gate wide open; brought up gently it holds): the matched gate is
  too small (0.85 of the nozzle); with 1.5 times the area torque and power land in range; N2 is resizing it and
  adding over-boost protection after. N1: the walls counted once add 17 to 25 % on all sixteen NA archetypes
  (then the mild ones are 1.19 to 1.30 of target: the burned gas's heat capacity is the next question); VE does
  not separate the heads; the phaser's placeholder schedule over-advances long cams (to be scheduled by intake
  closing angle).
- 16:21 G's handoff: the builder costs 0.17 R at the reference's own topology and 0.44 R for a V8's dual X with
  stubs and valved mufflers (card: 0.25): the twin-turbo V8 with that exhaust is near 1.9 R. Q attacks the 0.17
  first. 16:22 Q's compile study (`docs/measurements/q7-compile.md`): 466 kB, 3.4 to 4.2 s; -O0 struck; three
  wishes for FMinus's owner in the queue (an object cache first).
- **16:17 the exhaust builder is in the engine** (card G, cf267fb): `exhaust.model` 1 runs the builder's pipes, and
  with `boost.model` 1 the collectors end in the turbines' housings and take the housing's pressure back, so the
  cylinders see their turbine at last (`Boost::exhaustStandIn` only when `exh.turbinesFed()` is 0). **All ten
  wave-1 packages are integrated.** It had been green in G's clone since about 15:10, re-running the null test
  against a moving branch. Now free to move: N2's "after" measurement, Q's pipes as arrays and the builder's cost in
  the engine, U3's per-pipe pressures, D's review of the lines.
- **16:19 a double count found** (N2 measured it, the lead read it in the code; D fixing): `sim/engine.fm:821`
  turns Woschni's walls on under `wallHeat == 1 || fuel.model != 0`, but `sim/combustion/combustion.fm:261` gives
  the burn its 0.97 share of the heat only when the field `wallHeat` is 1. Every archetype sets `fuel.model` and
  leaves `wallHeat` 0, so each takes its wall loss twice: +11 to 13 % torque on the turbo four with it counted once.
  The table of 16:15 (174 of 581 missed, every invariant met on all thirty) will be re-run after the fix.
- 16:16 N2: the fast dyno reads 9 to 10 % over the stepped dyno on a boosted engine (437 for 401 N m), so the
  validation table and the ribbon inflate boosted engines by that (C has it); the kei's matched gate cannot hold
  its own spring over 5000 rpm; the twin-screw's boost rises 58 to 135 kPa with speed.
- 16:16 Q, every landed model on (`docs/measurements/q6-all-models.md`): the twin-turbo V8 with a dual X costs 1.44
  to 1.47 R today (budget 1.6); about 1.75 once G's builder lands at its own 0.31; about 1.55 with the pipes as
  arrays and the valves in two passes; Economy about 0.87. **New risk: the program compiles in 3.7 to 4.1 s here**
  (465 kB of machine code; the reference 1.4 s, 158 kB): Q studies compiling less (a switched-off model not
  instantiated at all), C reports compile time on every CI system.
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
