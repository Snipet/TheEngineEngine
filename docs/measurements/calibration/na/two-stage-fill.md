# The two-stage build at 7500 rpm: where stage A's fill comes from

Card N1, 25 Nov, 02:20 UTC (by `date -u`). The lead's question: why does the two-stage build's stage A hold its
volumetric efficiency to 7500 rpm, so that stage B only overtakes it at about 6900 rpm (≥ 7500 on the tip)? The real
pair crosses at 5500 to 5900 (Hondata's tuning help, an S2000 owner: P's sources §12; a tuner's and a forum's word,
not a curve).

## How it was measured

- The build is `two-stage-cam-20`, on the tip after 57189404, at full load and warm.
- The probe is `[.probe-fill]` (tests/sim/calibration/na/Probe.cpp). For cylinder 1, every 10 degrees, it reads the
  pressure in the cylinder and at its intake port, the valve's flow, and the jet's Mach number. Then it reads the state
  at the intake's nominal seat closing, and the mass going in and back out between bottom centre and that closing.
- Each stage is held through `valve.stageSpeed`: A at 1100, B at 100.
- "Won't ring" means `intake.runnerLength` 0.03 m, a runner too short for its wave to arrive in the stroke.

All figures are **measured**.

## Results

| case | torque N m | VE (tel) | jet Mach, peak | after bottom centre, a cycle: in / back out | T at bottom centre | T at seat closing |
|---|---|---|---|---|---|---|
| A, the card's 0.30 m runner | 189.1 | 0.945 | 0.50 | 5.9 / 15.4 mg | 362 K | 395 K |
| A, 0.03 m (won't ring) | 167.9 | 0.849 | 0.41 | 4.5 / 12.6 mg | 347 K | 381 K |
| B, 0.30 m | 184.9 | 0.903 | 0.47 | 0 / **60.8 mg** | 351 K | 409 K |
| B, 0.03 m | 170.9 | 0.841 | 0.37 | 2.2 / **49.4 mg** | 336 K | 394 K |
| A at 5000 rpm, 0.30 m | 188.4 | 0.850 | 0.36 | 0 / 34.0 mg | 346 K | 376 K |

The trapped charge on stage A at 7500 is 593 mg. The jet's Mach number is the valve's flow over the upstream density
and the effective area, divided by the upstream sound speed. The Mach number from the pressure ratio alone (isentropic)
peaks at 0.60 to 0.64.

## The temperature at closing

The intake's seat closing on stage A is at 606° of cylinder 1's cycle (B: 628°). That is 66° (B: 88°) past bottom
centre, and the valve's flow has fallen to zero by about 590°. So the gas temperature read at the seat closing
includes up to 66° of compression with the valve all but shut.

The charge's own temperature is the one at bottom centre: 336 to 362 K, 50 to 75 K over the air. That comes from the
residual (burned share 0.013 to 0.032) and the walls.

The 377 to 417 K in the earlier two-stage ledger were read at the seat closing. They are a reading's angle, not
heating. Any ledger or probe column of "T at closing" must say which angle it was read at.

## Reading

1. **Stage A's late, hard fill is the runner's wave.** On stage A at 7500 rpm the port runs 19 % over the plenum
   (116.7 against 98.2 kPa) through 480 to 540°. That is worth about 10 points of VE (0.945 against 0.849 without the
   wave). Without the wave, stage A at 7500 holds what it does at 5000 (0.849 against 0.850). The valve does not
   throttle it: the jet peaks at Mach 0.41 to 0.50. The 0.30 m runner is the card's judgement, a class default, and
   no source was found for it.
2. **Stage B pushes back a tenth of its charge at 7500:** 49 to 61 mg a cycle, through its later closing (628
   against 606°). Stage A pushes back 13 to 15 mg. Whether a real long cam does this at 7500 depends on the same
   runner. This is the first thing to look at again once the real runner's length is in.
3. **Without the wave the stages are level at 7500:** A 0.849, B 0.841. B's 49 % more lift buys +5.6 % of area under
   the tanh port limit. The K20A2's sheet gives +12.6 % (flow-sheets-vs-seat.md). The port limit's form is open: a
   series form was tested against the sheets and does not follow them, and more four-valve sheets are asked of P.

## What stage A's card rests on (the lead's question, 02:24)

Stage A is 236 / 204° (seat / 1 mm) on 8.5 mm intake, and 232 / 200° on 8.0 mm exhaust, centred at 110 / 112. It
carries **no source**. In `content/builds/archetypes/generate.py` the cam tuple stands with no comment, while the high
stage beside it cites the K20A Type R's figures (an aggregator) and the switch cites Honda's fact book. The same 236 /
204 on 110 recurs on other archetypes' generic cards. Its 32° between seat and 1 mm is shorter than CAM_RULE's 38° for
a rocker. So stage A is **judgement**, a generic card.

A low stage a little milder than this one would fall away earlier, and the model's crossing (8034 rpm on P's row,
against the real 5500 to 5900) is then partly a card nobody has sourced. That sits beside the two measured parts above:
the runner's wave, which favours this stage A at 7500, and stage B's head-specific area under-read. P is asked whether
Honda's fact book that gave the high lobe gives the low one.

## Next

1. P finds the engine's real runner: the length from the valve to the plenum or bellmouth, its diameter, and whether
   its length changes.
2. The port limit's form, once more four-valve sheets are in.
3. The crossing again, with each stage held, beside the real 5500 to 5900.
