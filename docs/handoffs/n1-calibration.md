# N1: calibration, the engines that breathe by themselves (handoff)

Card N1 ([w2-n-calibration.md](../plan/w2-n-calibration.md)), branch `claude/awesome-gates-ntdb58`. Measurements:
[docs/measurements/calibration/na/](../measurements/calibration/na/) (`ledger.md`, `causes.md`, `cam-maps.md`).

**Done**
- `scripts/ledger.py`: the loss ledger on the engine dyno, with the line ranges and their sources. NA and boosted
  lines. `--set` for experiments, `--sim` for another tree. Sixteen NA builds take 40 s.
- `ledger.md` for the 16 NA archetypes, as built.
- The probe (`tests/sim/calibration/na/Probe.cpp`, hidden `[.probe]`, `[.probe-idle]`): VE split at intake closing
  (matches tel within 0.005), the strokes' pressures, the walls' share, throttle air against trapped air.
- Test `calibration-na.the fresh air trapped at a warm idle is the air through the throttle`: skipped on cause D.
  It measures 1.58x today and holds a fix within 15 %. Runs in 6 s.

**Causes** (each with its experiment, in `causes.md`)
- A. Woschni: +17 to 25 % uniform, walls 15 to 19 % either way. Recommended on for the archetypes.
- B. Pattern 1 is in VE. The exhaust builder is not wired; the reference's pipes are sized by displacement, not
  flow (mean port pressure 154 kPa at 7600 rpm). Routed to G. Pipes x1.3: tuned +5 to 11 %, mild 0 to 4 %.
- C. The phaser placeholder over-advances long cams (V12 at 5000 rpm: 457 against 554 N m parked). Full-load maps
  for P in `cam-maps.md`: +2.5 to 5 % mean.
- D. **Mass created at the intake valve** (`core/cylinder.fm:290`, the reference's too): a V12 at idle traps 20 g/s
  on 0.7 through the throttle. This is pattern 4. The fix (`intake-roundtrip.diff`, tried in a scratch tree): the V12
  idles 810 to 930 on a working valve. Routed to D, to gate behind engine.custom.
- E. Pattern 3: the rotary is over-boosted (147 kPa for 70, gates wide open). For N2. NA it is within ~20 %.
- F. The intake's charge heating, left out: 25 K costs 9 % on all alike.

**Not done**
- Patterns 1 and 4 are not closed: no module changed. Each fix is in another owner's file and routed (D, G, P, E on
  the lead's word).
- The NA spread (tuned about 0.9 of target, mild about 1.25, with every lever measured) waits for G's builder in the
  engine. Without it, moving a constant would be fitting.
- The class tests (BMEP, VE, friction) wait for the fixes they would hold.
- The table again (`report.py`) has not been rerun: nothing it measures has changed.

**Wanted outside my paths**
- D: `tel.engine.chargeTemperature`, `tel.engine.trappedMass`; the intake round-trip fix, gated.
- G: `tel.exhaust.portPressure` (mean); the builder in the engine; pipes by flow.
- P: port quality by class of head; `ecu.maps.camIn` from `cam-maps.md`.

**FMinus**: no issue. **Cost**: nothing in `sim/` changed (the null test was not needed).

**To listen to** once D's fix lands: the V12, V10 and flat-plane V8 at a warm idle. They should lope at 850 to 950,
not hold 1100 to 1300 rpm.
