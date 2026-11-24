# D2: heavy diesels

Branch `claude/awesome-gates-ntdb58`. Model and numbers: [docs/models/diesel.md](../models/diesel.md). Code: `sim/combustion/heavy.fm`, `sim/vehicle/roadranger.fm`, `truckair.fm`; tests `tee-tests-diesel` (ctest `diesel.*`: 18 quick cases in the default run, 7 full forms `[nightly]`; all pass); builds `content/builds/truck-15l-six{,-bobtail}`, `diesel-{v8-truck,mechanical-classic,pickup-tuned,marine-v12}` (all 300 K, 101325 Pa). Every D2 field off: the reference to the bit (`[null][quick]` before each push).

**Done, by test** (BrakeTest, GovernorTest, InjectionTest, ColdSoakTest, GradeTest, BuildsTest, TurboTest, TruckPartsTest):
- Compression brake (fuel off, E's aux lobe opened per cylinder by `brakeArea`): reference turbo and exhaust, 2000 rpm stages 1/2/3 104/164/239-252 kW vs 56 motoring; High 114/173/239 kW at 1200/1600/2000. With F's VGT shut by the brake (`vaneCommand` -> `Boost::vanesAndPurge`) and G's network (exhaust.model 1): manifold 3.41 bar, **406 kW** at 2000 rpm: published 15 L ~450 kW at 2100. The missing half was the vanes and the turbine's back pressure. The button BTN_ENGINE_BRAKE (64) gives High with the dash switch Off.
- Bark: +8.4 dB rms over motoring; firing-frequency harmonics 2-10 ~25 dB up. Muffled vs straight: the reference's muffler dial (listening files); the flap softens it.
- Exhaust brake: with G's butterfly (exhaust.model 1, flapKind Butterfly; I drive `exh.setFlap`) 362 kPa before it, 130 kW vs 54 open; without it my stand-in (a mean back pressure at the ports) 3 bar, +61 kW; with the Jake 318 kW.
- Governors: droop line exact (rack 1.00/0.49/0.00); mechanical idle hunts 145 rpm, electronic 4 rpm. Fuel by the rack, boost compensator, smoke number in `tel.ecu.smoke`: rolling coal emerges (10 FSN spooling, 4.4 spooled; stock 1.8).
- Injection (H's fields): pilot 4 % 11.2 -> 5.4 bar/deg, noise 160 -> 61 Pa; 6 deg later 4.7. Over-fuel +21 % torque, smoke 1.3 -> 7.3.
- Cold: starts 253 K in 1-1.9 s, 233 K in 2.9-4.5 s, every governor, H's models 0 and 1 (no no-start). Soak 40 s random: bounded.
- 36 t rig (I's vehicle, paddles): 0-50 km/h 19.3 s; 6 % at 42 km/h on 278 kW (power/weight 39); down 6 % braked to 39 km/h, free to 63.
- F's machine: big single 59 kPa t90 2.05 s at 1300 rpm; VGT 192 kPa 0.65 s; compound 202 kPa 1.9 s.
- 18-speed through card I's vehicle (gearbox "Eighteen-speed") in the whole engine: all 18 gears at rest; rolling, a float into 5 (revs within 25 rpm) silent; 300 rpm over 7's grinds and is refused (EighteenTest). Its grind at L's gearbox place. Air system (spec.car.truckAir) at L's places: purge at 8.3 bar, spring-brake release heard (sound.model 1).

**Cost** (`tee-bench --engine`, free-revs/truck, 2 runs, sim ~1.9 us a sample here): large-diesel (D2 off) 1.004/1.007; truck-15l-six 1.073/1.086 and mechanical classic 1.085/1.093, both with I's vehicle model and H's fuel model in them (D2's own part some 0.02-0.03 R: a table lookup a cylinder a sample).

**Not done**: the PV-loop picture; a 30 deg / 3.5 mm lobe loses the bark's lines (not understood). Whole suite on the tip: combo archetype/step tests fail on card T's `engine.tsPipeTemperature`=0 (not mine, told the steward); my tests and the null test pass.

**Changes I made outside my paths (gated, null-tested)**: `controls.fm` BTN_SPLITTER 16, BTN_RANGE 32, BTN_ENGINE_BRAKE 64 (lead; shell bound Z, X, J); `sim/engine.fm` five hooks (the fifth: `boost.vanesAndPurge(hd.vaneCommand(), 0)`, agreed with N2/F) (no answer from D in 20 min; README "The hooks" rows); H's `diesel.fm` (fields `lever`, `metered`, `fuelFull`, `afcPhi`, `phiAsked`, `phiMetered`) and two lines in `chargeDiesel`; `tel.ecu` D2 block; `sim/README.md` rows.
**Done with others**: I's "Eighteen-speed" gearbox (51908c2), L's places, U1's brake lamps, G's flap, N2's vane brake position. Nothing wanted now but the kept-share change (D/H), after which the brake, cold starts and rig are to be re-checked.
**FMinus**: no issues.

**Listen to** (`tests/sim/diesel/listen/`, README there): the Jake by stages muffled and straight, with the flap; a cold mechanical idle; the pilot softening the clatter; a big single's whistle and the vanes' quick spool; the loaded pull-away and the grade. Nobody here has heard any of it.
