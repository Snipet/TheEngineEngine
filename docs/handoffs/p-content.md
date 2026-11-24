# P: content, validation targets, the manual (handoff)

Package P, branch `claude/awesome-gates-ntdb58`, cloud session. Owns `content/builds/**` (new files),
`content/validation/**` (new files), `docs/manual/**`, this file.

**Done.** 30 archetypes, `content/builds/archetypes/`, written by `generate.py`. Each is a whole build with every model on
(custom engine with real bore, stroke and rod; valve Lab with cams by duration, lift and springs; intake runners of their
kind's length; boost Machine; exhaust Builder; Fuels; ECU Maps; Vehicle) at 288.15 K and 101325 Pa. Dimensions are
published figures, cited in each build's notes. **All 30 start warm and cold, idle, hold the limiter (1.005 to 1.021 of the
redline), pull through the gears and stop at the key**: every invariant passes on the tip (`tee-render` and the `p-*`
scenarios in `content/validation/scenarios/`). Parts in `content/builds/parts/`: 30 engines, 30 tunes, 28 cars (declared
parts); loose cams (29), turbos (13), a supercharger and 6 exhausts (no "part" key: GARAGE applies only their fields).
Valve springs are set so that each engine floats 10 % past its limiter (measured from `valve.floatSpeed`).
`docs/manual/builds.md` lists every shipped build.

**Validation.** `content/validation/archetypes/<slug>.json` holds 581 targets, each with its source and how firm it is.
`report.py` writes `docs/manual/validation.md`, the scoreboard: **174 missed**, all invariants met. Measured peaks against
targets are in that table; a sample (kW / N m against the published figures):
- six TT 369/630 (250-340/450-600)
- flat-plane 4.5 305/411 (400-445/520-560)
- big block 303/596 (240-310/600-690)
- 7.0 pushrod 373/618 (370-385/630-645)
- V12 526/586 (500-590/680-730)
- tractor 391/2344 (300-450/2000-2780)
- 2.0 diesel 104/271 (100-130/320-380)
- hot hatch 102/151 (85-120/140-165): passes

**Largest misses:**
- Kei 97 kW against 47: boost now cut to a production 0.65 bar, not yet re-measured.
- Rotary torque +92 % (605 N m), its boost 1.3 bar against 0.7 (sequential turbos ignore the target?).
- Torque peaks 25 to 41 % too low on high-revving NA engines (V10, two-stage, flat-plane 4.5).
- NA engines' torque level about 11-13 bar BMEP whatever the head.
- Boosted engines 20-100 % strong.
- Big-cam idles lumpy (the V12 wanders 709 to 1405 rpm).
- Port gas meets 1100-1300 K on most engines; header gas is n/a (nothing reports it).

N1 and N2 own the patterns.

**Could not build as intended.**
- The compression brake (`valve.auxOn`) is always on, so the tractor and the 6.7 diesel would not start with it. It is
  left off; D2 is gating it. Both diesels have D2's exhaust brake on.
- The supercharged V8 overran a fuel-cut limiter by 3.2 % and a throttle limiter by 5.6 %. It runs the soft limiter
  (1.4 %). The throttle limiter's overrun with a blower is for card H.
- Pushrod engines need 2 to 4 times a real spring to float past their limiter, so the float model reads pessimistic for
  pushrod trains (card E).
- The quarter-mile and top-speed runs swap a manual for an automatic (an open-loop timeline cannot shift one). The
  tractor has no top-speed run.

**Help texts:** 243 of 420 visible Spec fields had no `ui::help`. `docs/manual/help.tsv` has a sentence for every one.

**Manual** (`docs/manual/`): getting started, controls, glossary, a page for each workspace and each model, builds,
`parameters.md` (from `tee-schema --markdown`), validation.

**Wanted:**
- An audio "line" measure (the strongest spectral line), so the firing-order predictions in each validation's
  description can be measured (card C).
- `tel.exhaust.headerTemperature` (card G).

**Not run:** macOS and Windows; nothing listened to. To listen: the flat-plane 4.5 V8 against the big block on
free-revs (`p-free-rev`), the boxer's idle, the rotary.
