# N3: the NA engines' air and energy balance, and where the misses are (card N, third session)

Branch `claude/awesome-gates-ntdb58`, 19:30 to 21:15 UTC 2026-11-24 (`date -u`). Every number and its run is in
`docs/measurements/calibration/balance/causes.md` (N3-0 to N3-14); `gamma.md` and `discharge.md` beside it.

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

8. *The turbo four's low end* (B-3, 23:00 to 23:16): the build's long exhaust lobe back-fills the overlap N1's phaser
   asks at boosted low speed (residual 0.16 to 0.22); P's cited AVS short lobe (180 degrees at the seat) scavenges in
   the model: 316 N m at 1800 on a 0.40 match (from 181). Left: the compressor's map width (N2: the low end wants a
   small wheel, the top a large one), the turbine's size by the cited top (81 kPa real boost: 0.85 turbine 276 N m,
   0.40 233, cited 250), knock (C; about 0 N m once the lobe is in). Knock and DI cooling sized; G's heat loss third.

**Tests**: `calibration-balance.sim` (one entry, 18 s): air and first law within 1 % on three engines; gross indicated
efficiency 0.33 to 0.38 on the small block at three speeds. Passes. The probe (`[.balance]`, BAL_SET / RAW / READ /
TRACE) and `scripts/balance.py` (balance.md, generated).

**Not done / open**: the overrun burn (N3-14: position and dilution terms worked, waiting on the overrun's burned share
from N1 and a source for the dilution form); the accessories (no source better than forums); the ledger's class ranges
(proposed to N1, causes.md); discharge by class beyond the LS heads (forum figures, weak); the header thermocouple's gas
(no cited full-load figure found).

**Changes I made outside my paths**: none. Proposed: D's γ (taken); P's runners, primaries, L48 parts (taken in part).

**Listen**: any NA petrol preset at full load and on the overrun, and a diesel at full load (the γ change: 5 to 9 % less
torque on petrol, the exhaust's sensible heat +0.01 of the fuel's). Nothing of the sound measured by me.
