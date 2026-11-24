# N3: the over-delivering NA engines' air and energy balance (card N, third session)

Branch `claude/awesome-gates-ntdb58`. Started 19:30 UTC, 2026-11-24.

**Done**
- Balance probe `tests/sim/calibration/balance` (`[.balance]`, hidden) and `scripts/balance.py` →
  `docs/measurements/calibration/balance/balance.md` (generated). Settled dyno, 96 kHz, every cylinder read each sample;
  first law closes within 0.3 % of the fuel's heat; air within 0.3 % on all seven.
- `calibration-balance.sim` (one entry, 18 s): the air's and energy's books close within 1 % on the small block, the
  VR6 and the roadster (passes); gross indicated efficiency 0.33 to 0.38 on the small block at three speeds (skipped
  on finding N3-1).
- Causes in `docs/measurements/calibration/balance/causes.md`: N3-0 (no mass is made; the two VEs differ by the
  intake's heating), N3-1 (gamma), N3-2 (rich heat, held), N3-3 (the flat six's breathing), the class ranges.

**Not done**
- N3-1's change is D's (`Cylinder::gammaAt` to Gatowski's 1.392 - 8.13e-5 T): sent to D with the 16-engine table
  (-4.8 to -8.6 % torque, a level). Not committed by me.
- N3-2 (Mixture::heatShare rich parabola, H's path): held; needs the fuel-air cycle's figure against λ.
- The validation table was not re-run (no simulator change of mine).
- "Discharge by class": asked N1 what exists (19:47); no answer yet.

**Changes wanted outside my paths**
- D: `sim/core/cylinder.fm:214` → `return fminf(fmaxf(1.392f - 8.13e-5f * tk, 1.15f), 1.40f);` (gated to the physical
  path if model 0 reaches it).
- N1: the class ranges in causes.md for `scripts/ledger.py`, and a gross-indicated-efficiency line.
- P: the flat six's VE 1.00 at 4750 rpm (N3-3).

**Listen** (only if N3-1 lands): any NA petrol preset at full load and on the overrun: the change takes 5 to 9 % of the
torque and raises the exhaust's sensible enthalpy by 0.01 of the fuel's heat (measured; the gas's temperature not read).
