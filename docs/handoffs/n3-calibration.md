# N3: the over-delivering NA engines' air and energy balance (card N, third session)

Branch `claude/awesome-gates-ntdb58`. Started 19:30 UTC, 2026-11-24; this note 20:18 by `date -u`.

**Done**
- N3-1 landed (D's 3990ee47, a845b07f: γ = 1.392 - 8.13e-5 T + 0.037 (1 - min(φ, 1)) under the physical cylinder);
  re-measured: 16 NA petrol -4.8 to -8.6 % torque, diesels -3.9 to +1.0 % of gross IMEP (causes.md, gamma.md).
- Balance probe `tests/sim/calibration/balance` (`[.balance]`, hidden; builds loaded whole through tee::Build, as
  tee-dyno; `BAL_SET`, `BAL_RAW`, `BAL_READ`) and `scripts/balance.py` → `docs/measurements/calibration/balance/
  balance.md` (generated). Settled dyno, 96 kHz, every cylinder read each sample; first law closes within 0.3 % of the
  fuel's heat, the air within 0.3 % on all seven.
- `calibration-balance.sim` (one entry, 18 s): air and energy books close within 1 % on the small block, VR6 and
  roadster (passes); gross indicated efficiency 0.33 to 0.38 on the small block at three speeds (passes since N3-1 landed).
- `causes.md`: N3-0 no mass made (the VEs differ by the intake's heating); N3-1 the cylinder's γ 0.025 to 0.04 over
  Gatowski's (SAE 841359), with its mixture term from frozen products (lean +0.011) and the diesels' -2 %; the λ sweep
  (best torque stays at 0.90); N3-2 the rich parabola kept; N3-3 the flat six's VE 1.00 is its runner's ram; N3-4 the
  two-stage cam's stage B loses; N3-5 the 3.5 V6 peaks at 2778; class ranges for the ledger.

**Not done**
- Breathing (N3-3, N3-4, N3-5) is routed to N1 (runner damping, exhaust back pressure, phaser) and P (runner, cams).
- "Discharge by class" (taken from N1, who had none): discharge.md has the LS family's heads; the overhead-cam and
  four-valve rows and the exhaust's are still to do.
- The validation table was not re-run (no simulator change of mine).

**Changes wanted outside my paths**
- N1: the class ranges in causes.md for `scripts/ledger.py`, and a gross-indicated-efficiency line.
- P: roadster valves and cams, small block's cam, the flat six's and 3.5 V6's runners, the two-stage's stage B; portIn
  1.15 to 1.2 on the small block and 3.8 V6, 1.6 on the 7.0 (discharge.md).

**Listen** (N3-1 is in): any NA petrol preset at full load and on the overrun; a diesel at full load. The change
takes 5 to 9 % of a petrol engine's torque, about 2 % of a diesel's, and raises the exhaust's sensible enthalpy by
0.01 of the fuel's heat (measured; the gas's temperature not read).
