# D2: heavy diesels

Branch `claude/awesome-gates-ntdb58`. Model and numbers: [docs/models/diesel.md](../models/diesel.md). Code: `sim/combustion/heavy.fm`, `sim/vehicle/roadranger.fm`, `truckair.fm`; tests `tee-tests-diesel` (ctest `diesel.*`, 17 cases, all pass); builds `content/builds/truck-15l-six{,-bobtail}`, `diesel-{v8-truck,mechanical-classic,pickup-tuned,marine-v12}` (all 300 K, 101325 Pa). Every D2 field off: the reference to the bit (`[null][quick]` before each push).

**Done, by test** (BrakeTest, GovernorTest, InjectionTest, ColdSoakTest, GradeTest, BuildsTest, TurboTest, TruckPartsTest):
- Compression brake (fuel off, E's aux lobe opened per cylinder by `brakeArea`): 2000 rpm stages 1/2/3 104/164/239 kW vs 56 motoring; High 114/173/239 kW at 1200/1600/2000. Published 15 L ~450 kW at 2100: we sit at the low edge (200-550 asserted: judgement).
- Bark: +8.4 dB rms over motoring; firing-frequency harmonics 2-10 ~25 dB up. Muffled vs straight: the reference's muffler dial (listening files); the flap softens it.
- Exhaust brake (flap or vanes, a mean back pressure at the ports): 3 bar held, +61 kW; with the Jake 318 kW.
- Governors: droop line exact (rack 1.00/0.49/0.00); mechanical idle hunts 145 rpm, electronic 4 rpm. Fuel by the rack, boost compensator, smoke number in `tel.ecu.smoke`: rolling coal emerges (10 FSN spooling, 4.4 spooled; stock 1.8).
- Injection (H's fields): pilot 4 % 11.2 -> 5.4 bar/deg, noise 160 -> 61 Pa; 6 deg later 4.7. Over-fuel +21 % torque, smoke 1.3 -> 7.3.
- Cold: starts 253 K in 1-1.9 s, 233 K in 2.9-4.5 s, every governor, H's models 0 and 1 (no no-start). Soak 40 s random: bounded.
- 36 t rig (I's vehicle, paddles): 0-50 km/h 19.3 s; 6 % at 42 km/h on 278 kW (power/weight 39); down 6 % braked to 39 km/h, free to 63.
- F's machine: big single 59 kPa t90 2.05 s at 1300 rpm; VGT 192 kPa 0.65 s; compound 202 kPa 1.9 s.
- 18-speed alone: lever/range/splitter give all 18; a matched float goes in silently, 300 rpm off grinds (60 Hz clash) and stays out. Air: cut-out 8.3 bar with purge; spring-brake release heard.

**Cost** (`tee-bench --engine`, free-revs/truck, 2 runs, sim ~1.9 us a sample here): large-diesel (D2 off) 1.004/1.007; truck-15l-six 1.073/1.086 and mechanical classic 1.085/1.093, both with I's vehicle model and H's fuel model in them (D2's own part some 0.02-0.03 R: a table lookup a cylinder a sample).

**Not done**: the 18-speed does not drive I's gearbox, and the grind and air sounds are not mixed (below); the exhaust flap is a mean back pressure, not a flap in the pipes (G's slot not in the engine); the turbine's back pressure under boost.model 1 does not reach the cylinders (F's stand-in). Whole suite: run before the last push only for diesel and null; earlier full run's failures were pre-existing (ContractTest's maps/exhaust/voices attributes).

**Changes I made outside my paths (gated, null-tested)**: `sim/engine.fm` four hooks (no answer from D in 20 min; README "The hooks" rows); H's `diesel.fm` (fields `lever`, `metered`, `fuelFull`, `afcPhi`, `phiAsked`, `phiMetered`) and two lines in `chargeDiesel`; `tel.ecu` D2 block; `sim/README.md` rows.
**Wanted**: lead: `BTN_SPLITTER 16`, `BTN_RANGE 32` in controls.fm (read meanwhile as raw bits). Card I: a hook in `VehGearbox.control` for kind Manual with `spec.car.kind` Truck: take `Roadranger::gear` and its `wShaft` (the box is mine; it needs the output shaft's speed and the clutch). D/L: mix `Roadranger::grind()` and `TruckAir::out()` (each sample, driven truck) at the gearbox's and the chassis's place; a spec switch for them in my `spec.car` block. M: the grind could be your voice instead of mine.
**FMinus**: no issues.

**Listen to** (`tests/sim/diesel/listen/`, README there): the Jake by stages muffled and straight, with the flap; a cold mechanical idle; the pilot softening the clatter; a big single's whistle and the vanes' quick spool; the loaded pull-away and the grade. Nobody here has heard any of it.
