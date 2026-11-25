# P: content, validation targets, the manual (handoff)

Package P, branch `claude/awesome-gates-ntdb58`, cloud session. Owns `content/builds/**` (new files),
`content/validation/**`, `docs/manual/**`, `docs/measurements/validation/`, this file. Updated 25 Nov, 06:00 UTC.

**Builds.** 31 archetypes and `content/builds/first.json` (the first launch: the 2.0 turbo four, now the EA888 gen 3 of
the Mk7 GTI), written by `content/builds/archetypes/generate.py`. Every model switch on, checked against `tee-schema`
(the generator stops on one left at the reference). **No part comes from its own engine's targets**: NA runners by their
manifold's class, headers by the class the real engine carries, the intercooler from swept air (all judgement, marked in
each build's notes); hardware from a source where one was found (the cited sixteen NA petrols' bands, the 454 as the 1972
LS5, the small block as the L48, the roadster's head, the two-stage's K20A cam, the turbo four's two-stage exhaust).
**The generator writes nothing that does not idle**: warm within 100 rpm of its asked speed (sd < 25), cold running
(sd < 60). Baked intake cam maps (`cams.py`: N1's `tee-dyno --bake-cam --save`, `data/cam-maps.json`) behind
`BAKED_CAMS` (on, the lead's clearance). No sound level or camera per build (L's auto level; Tailpipe the default).

**Validation.** `content/validation/archetypes/<slug>.json` (by its `generate.py`): every target says its **basis**
(cited / recalled / judgement; a fitted part's rows judgement); the turbo four's cited torque curve as rows; hot starts;
idle spread; the sound's floors. `report.py` re-judges re-read rows against today's ranges, counts misses by basis,
lists the worst five, and measures the idle's standing tone itself. Timed runs by `quarter_mile.py` (shifts from the
power curve, revs held at the green, warm tyres), regenerated on each table's pin.

**Tables** (all kept in `docs/measurements/validation/`, named at real UTC): G 132 of 648, H 193 of 741, I 123, J 124 of
772, K (`ab107ca`) 120 of 839, **L (`e43e228b`, content `1efa3282`) 121 of 847 (29 of 203 cited)**, power in range 12 of
30, torque 10 (the tractor's set by its fuel curve); with its twin on K's content (114) to read content from model.

**Sources files:** `docs/measurements/validation/2026-11-24-2314-ea888-gen3-sources.md` (the turbo four: what is known,
what is guessed; gen 3's exhaust cam timing the one figure not found); `docs/measurements/2026-11-25-0032-judgement-sources.md`
(VGT vanes at idle, a diesel's cycle spread, turbine efficiency, fan switches, idle warm-up); the quarter mile one change at
a time (`2026-11-24-2215-quarter-mile-one-change.md`).

**Open for others:** the curves' sharp teeth (N1); the turbo four's low end (the turbine match, N2; gen 3's exhaust timing,
unpublished); the diesels' boost at the check speed in K (N2's correction); the diesels' idle hum (D2, N2).

**Manual** (`docs/manual/`): getting started, controls, glossary, workspaces, models, builds, parameters, help.tsv,
validation (the newest table). **Not run:** macOS and Windows.
