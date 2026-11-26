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

**Fixed since** (acting for E and H; each measured before and after; tests pass; the null test passed)
- `Phaser::schedule` by target intake closing, capped by overlap (712b599). Test `calibration-na.the intake
  phaser's schedule cuts no hole...` (29 s). The six at 2700 rpm: 206 → 290 N m. The V12 at 5000: 495 → 582.
- Charge heating in `Intake::control`, model 1. It waits for D's `intk.tCoolant = heat.tCool;` and then costs 6 to
  11 % on all engines alike.
- The ITB manifold floored at 8 kPa. Test `calibration-na.individual throttles...`.
- A rotary's burn efficiency of 0.92 (D's number): -4 to -6 %.
- Pattern 4 is closed by D's conserving cylinder: V12 882, V10 952, flat-plane V8 998 rpm, on target. The balance test
  is unskipped (1.048).
- The ledger: boosted settle, boost over target, the commit in its header.

**Since 19:14 (the lead's queue; causes.md L and G's correction)**
- P's two warm no-starts (flat six, 7.0) fixed by measurement, one cause a commit:
  - the intake's plenum books, per sample (c40f041): the Valve's stiffening rectifying the back and forth;
  - the ECU's after-start air (ca76cd6);
  - the idle spark's look-ahead = the manifold's filling time (14890c0, with D's 3930eadb).
- The 7.0 idles 750 rpm sd 5; every archetype starts warm and runs (WarmStartTest; the steadiness check in the fast tier).
- C's spec.intake.heatSettle (876aa34). X-022 held in the fast tier again (4 s). calibration-na's fast tier is 57 s locally.
- The springs' known offset in docs/models/valvetrain.md; the ledger's header says walls settled (its rows predate 1220ca3: to re-run).
- The pop's store on exUnburnedJ: measured, held as a diff (the overrun's burn ends before its exhaust opens; the lead
  kept the spark-time estimate; the overrun burn's duration is N3's).
- Moved to N3 (the lead, 20:25): full-load breathing (the runner's ram, two-stage stage B, the short engines' top end,
  the side-port rotary at speed).

**Not done**
- Pattern 1 is not closed. The tuned engines need content (runner, exhaust, port quality: values given to P). The
  mild ones need the charge heating (D's line) and less efficiency at speed (proposed to the lead and D).
- The NA spread (tuned about 0.9 of target, mild about 1.25, with every lever measured) waits for G's builder in the
  engine. Without it, moving a constant would be fitting.
- The class tests (BMEP, VE, friction) wait for the fixes they would hold.
- The table again (`report.py`) has not been rerun: nothing it measures has changed.

**Wanted outside my paths**
- D: `tel.engine.chargeTemperature`, `tel.engine.trappedMass`; the intake round-trip fix, gated.
- G: `tel.exhaust.portPressure` (mean); the builder in the engine; pipes by flow.
- P: port quality by class of head; `ecu.maps.camIn` from `cam-maps.md`.

**FMinus**: no issue. **Cost**: nothing in `sim/` changed (the null test was not needed).

**To listen to**
- The 7.0 (big-bore-70-pushrod-v8) and the flat six on p-idle-warm: the flare falls to a steady idle (750 and 850 rpm),
  no lope, no stall.
- M2's pops groups, 09 (the reference's dial: nothing at 0, crackle at 50, bangs at 100) and 10 (the same with card H's
  ECU, the store's bangs: fewer and louder). 10's fuelled overrun rests on the spark-time estimate.
- The V12, V10 and flat-plane V8 at a warm idle; they now lope at 880 to 1000 rpm, where they held
1100 to 1300. The 3.2 six and the V12 on a full-throttle sweep through 2500 to 5500 rpm; the phaser's holes are gone,
and the note should carry no dip there. The 3.2 six's lift after the limiter (the ITB reading floored).

## 25 Nov, the window before the bake: the two-stage's stage latch, the rotary, X-032 / X-034 / X-037 / X-040

**The two-stage's 13 N m notch and P's start overshoot are one fault** (measured, tip 60e87aef). two-stage-cam-20's
switch is speed-only: up over 612.6 rad/s (5850 rpm), down under 581.2 (the 31.4 rad/s hysteresis). A held point
inside the band reads the stage of the way it came.
- The predictor: its 5740 rpm point read stage B at 181.3 N m (neighbours 194.5 at 5197, A; 202.5 at 6283, B). The
  worker loop, instrumented, showed both paths: from 6826.8 rpm with B held, and a step up from a lower point that the
  absorber's loop carried over the switch. Approached from below: A, 192.1 to 192.6 N m.
- The stepped dyno, upward steps 5197 / 5740 / 6283: 193.9 / 190.3 (A) / 212.1 N m.
- **The stepped dyno is exposed too (measured):** a single held step at 597 rad/s reads stage B at 191.6 N m from the
  default `from` (150 rad/s), and at 184.7 N m from `from` 590. The jump 150 to 597 is estimated, not traced, to
  overshoot about 13 % of the step, to some 655 rad/s (6250 rpm; P saw 6190). The lead gave the stepped dyno to C with
  the predictor's ramp, and the absorber loop's overshoot (core/dyno.fm) to the integrator.
- The cure: `tel.engine.hysteresis` and `tel.engine.latched` (the valvetrain's `hysteresis()`, `latched(omega)`; D's
  fields), read by C's predictor, which lets a latched point down and approaches it from below; kCamBakeVersion 4,
  kKnockBakeVersion 6 (every table re-bakes).

**The rotary's low end** (rotary-twin-13, the new compressor map, P's steps): peak 312.7 N m at 2294 rpm on one turbo at
the 0.70 bar target; joined above 4500 rpm the boost falls to 0.45 to 0.53 bar with the gate shut (the secondary
compressor a tenth of the air, its shaft at some 10.7 k, the primary choking, efficiency 0.72 to 0.50). N2's machine:
N2 found the transfer's width evens the pair but gains under 2 %; the pair's match itself (the next window).

**The soak's findings:** X-034 closed (a firing train, X's restated check). X-037 (the pickup's dpOverlap -394 kPa)
is the vane brake's back pressure (tip: -298 kPa, the brake 359 kPa); restated as two checks, `tel.valve.dpWave` (the
waves' part, +-300 kPa) and the brake's own limit; suspected: D2's retune moved it. X-032 (the turbo six's lift, 3.05
against 2.97) stands: the lift is the same with or without the idle valve's start hold (576c1c6e, sourced, kept); the
hold changes the start's history (that combination's idle never settles before the throttle: 100 to 190 rad/s against
134), so the lift's firings fall at another phase; the steward's. X-040: the intake's least bores (runner 8 mm,
snorkel 10 mm, G's design): the finding runs finite.

**H9 and the bangs' default:** H9 runs `Sim("v8")` under exhaust.model 0, where the pipe is cleared; bang 0 and 1 read
the same. Under exhaust.model 1 its fuelled-overrun part would rest on one 0.04 mg burn (not its configuration).

## 26 Nov: the cam dial under the Lab (X-048 and X-049's read; the lead's ruling, 10:08)

**What an owner meets** (read at e4c668d3): all 32 shipped builds (31 archetypes and first.json) run `valve.model` Lab
with stated lab cams and `ecu.model` Maps with `ecu.idle.speed` stated; 30 leave `valve.cam` at 0.35 and the two rotaries
state it. Under the Lab, `spec.valve.cam` reaches three places: the valvetrain, whose events `place()` overwrites from the
lab's tables on a piston engine (a rotary keeps the dial's ports: the lab is not built for one); the idle target of
`ecu.model` 0 (ecu.fm:154, engine.fm:439: + 350 (cam - 0.35) / 0.65 rpm); Ecu1's idle, only when no idle speed is stated
on a petrol build (ecu1.fm:165). So on the 30 piston builds the dial does nothing. The fault (a dial that promises and
does nothing) is the interface's, cured by U1 in `ui/`. The corpus's valve rows on a dial-only combo build compare the
lab's default cam (231 deg at the seat, centres 110) with the dial's, not one model with the other; X measures the turbo
six with lab cams equal to its dial (300 / 281 / 106, the law at valvetrain.fm:116-122) before (a) against (c).

**A design sketch, not in a window (a feature for the owner to want): a one-dial cam for a Lab build.**
- `valve.camFrom` (int, 0 or 1, default 0; group Model; label "Cams from"; choices "The cam fields", "The cam dial"):
  stated by the builder, never inferred. A hidden "the build states no lab cam" rule cannot work: the Spec keeps no
  record of what a file stated, so a build that states the defaults on purpose would read as one that states nothing.
- 1, on a piston engine: stage A's two cams are requested from the dial's law, as the reference makes its events: both
  seat durations 224 + 76c, both centres 112 - 6c (`lsa`), so the seat events are the reference's to the degree; the
  1 mm durations the seat less the lab's own default step (19 deg in all: 231 -> 212, judgement); lifts, fatness, the
  head and every switch as their fields say. Glided as the dial is (`smCam`), requested through `cam[].request` as an
  edit is, so a turn of the dial rebuilds the tables one a tick with each valve shut (no click, as now).
- 0 (every shipped build and the null): untouched, bit for bit; a rotary ignores the flag (its dial already acts).
- Acceptance, when it is wanted: the null and the 32 to the bit at 0; at 1, the events read back (`valve.evo`, the seat
  event) equal the reference's at c = 0, 0.35, 0.9999 within a degree; X's valve rows on the turbo six at 1 against
  its stated-equal copy.

**A small item of mine for a window, a design line first: what the idle follows on a Lab piston build** with
`ecu.model` 0, or with no idle speed stated: today the dial raises the idle target by up to 350 rpm for an overlap the
lab's cams do not have. No shipped build is in that state (all 32 state an idle speed under Maps). Two choices: the
lab's own overlap (`overlap1mm`, which the lab already computes), through the same law re-expressed in overlap; or
nothing (the cam term held at 0.35 under the Lab, the idle the build's own). I'd take nothing: the overlap-to-idle law
would be a new judgement with no source behind it, a builder who wants a lumpy cam's idle states its speed (as all 32
do), and under `camFrom` 1 the dial would act again, through the events, where the dilution is the model's own.
