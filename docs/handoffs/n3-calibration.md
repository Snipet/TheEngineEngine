# N3: the engines' air and energy balance, and where the misses are (card N, third session)

Branch `claude/awesome-gates-ntdb58`, 19:30 to 23:31 UTC 2026-11-24 (`date -u`). Every number and its run is in
`docs/measurements/calibration/balance/causes.md` (N3-0 to N3-15, B-1 to B-3); `gamma.md` and `discharge.md` beside it.

**Found, in order**
1. *No air is made* (N3-0): throttle, valves, trapped fresh, exhaust and the metered fuel's air agree within 0.3 % on all
   seven over-strong engines. The two VEs differ by definition (tel veManifold is at the charge after the intake's heating).
2. *The cycle was too efficient* (N3-1): gross indicated 0.374 to 0.433 of the fuel against 0.33 to 0.38; the cylinder's
   γ(T) ran 0.025 to 0.04 over Gatowski's single-zone fit (SAE 841359). D took it (3990ee47, a845b07f, b551cd05) with a
   lean term from frozen products (lean +0.011 at every T: gamma.md) and a diesel's φ by its burned share: the 16 NA petrol
   -4.8 to -8.6 % torque, diesels within -4 to +1 %. Best torque stays at λ 0.90. The rich heat parabola (N3-2) kept.
3. *No common level remains* (the 16 against their cited bands: mean -2 to -7 %, spread 16 %). The rest is content.
4. *Runners* (N3-8): the generator tuned each runner to the target's own peak-torque speed, so each engine was read on
   its own 12 to 18 % ram tooth. On class runners the cited seven sit at -1 to +10 % but the small block and the four-cam.
5. *The ram's height* (N3-6, open): losses in at textbook size, levers measured; published gains 6 to 9 %, the model's
   12 to 18 %; the tuned engines need it at about this height on real-like hardware. Not changed.
6. *Long cams live on waves* (N3-4, N3-10, N3-12): the two-stage's stage B and the V12's middle fill only on a runner's
   tooth or a header's suction at overlap; both work in the model (primaries swing VE 20 to 30 %). The tuned four-valves'
   shortfall is their runners, second runners and primaries (N3-9, N3-11, N3-13): content, given to P with the method.
7. *The small block* (N3-7, open): VE right, efficiency high; P's L48 parts took 5 %; ~14 % unexplained (sized: burn 1 %,
   fan 1 to 3 % on forum figures only, spark 1.5 % without a source).

8. *The boosted fourteen* (B-1, B-2): torque mean -5.5 %, spread 7 %; boost short at the top on the matching (N2), knock
   (C); the housings' mass gap closed by G (0.998). The ram's damping (N3-15): intake bound 6 to 9 % against the model's
   12 to 18 %, to N1; no exhaust bound found.
9. *The turbo four against the EA888 gen 3* (B-3, ends in one table): the intake phase at boosted low speed (N1's rule
   against a baked map, +44 to +71 % at 1800), then the exhaust lobe and an exhaust that closes earlier (P: AVS lobe
   ruled at 1 mm; the parked end unknown), the spool and compressor map width (N2), the turbine's back pressure at the
   top (6200 at the real 81 kPa: 272 on the 0.85 turbine, 233 on a 0.40, cited 250), then the knock-limited burn (C;
   DI cooling twice the measured), lambda 1 (-3 to -4 %, a tuner's source), the exhaust's heat (G, third order). Best
   with cited or ruled hardware and a fixed-phase bake: about 303 / 310 to 330 / 233 to 272 N m at 1500 / 1800 / 6200
   against 333 to 368 / 333 to 368 / 237 to 262. The grid's cells scatter up to 7 %: read them to about 20 N m.

**Tests**: `calibration-balance.sim` (one entry, 18 s): air and first law within 1 % on three engines; gross indicated
efficiency 0.33 to 0.38 on the small block at three speeds. Passes. The probe (`[.balance]`, BAL_SET / RAW / READ /
TRACE / MEAN / HOLD: a throttle step) and `scripts/balance.py` (balance.md, generated). Boost energy terms (the turbine's
isentropic power, housing enthalpy) were read from a scratch copy of sim/boost, nothing committed there: a probe of them
in sim would need N2's sums.

**Not done / open**: the overrun burn (N3-14: position and dilution terms worked, waiting on the overrun's burned share
from N1 and a source for the dilution form); the accessories (no source better than forums); the ledger's class ranges
(proposed to N1, causes.md); discharge by class beyond the LS heads (forum figures, weak); the header thermocouple's gas
(no cited full-load figure found); the turbo four's exhaust cam parked end (P) and a baked map to run (N1); the DI
cooling's owner (asked of the lead).

**Changes I made outside my paths**: none. Proposed: D's γ (taken); P's runners, primaries, L48 parts (taken in part).

**Listen**: nothing of B-3 changed the sim. From N3-1: any NA petrol preset at full load and on the overrun, and a diesel at full load (the γ change: 5 to 9 % less
torque on petrol, the exhaust's sensible heat +0.01 of the fuel's). Nothing of the sound measured by me.
