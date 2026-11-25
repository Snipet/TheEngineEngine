# P: content, validation targets, the manual (handoff)

Package P, branch `claude/awesome-gates-ntdb58`, cloud session. Owns `content/builds/**` (new files),
`content/validation/**`, `docs/manual/**`, `docs/measurements/validation/`, this file. Updated 25 Nov, 23:45 UTC.

**Builds.** 31 archetypes and `content/builds/first.json` (the first launch: the 2.0 turbo four, now the EA888 gen 3 of
the Mk7 GTI), written by `content/builds/archetypes/generate.py`. Every model switch on, checked against `tee-schema`
(the generator stops on one left at the reference). **No part comes from its own engine's targets**: NA runners by their
manifold's class, headers by the class the real engine carries, the intercooler from swept air (all judgement, marked in
each build's notes); hardware from a source where one was found (the cited sixteen NA petrols' bands, the 454 as the 1972
LS5, the small block as the L48, the roadster's head, the two-stage's K20A cam, the turbo four's two-stage exhaust).
**The generator writes nothing that does not idle**: warm within 100 rpm of its asked speed (sd < 25), cold running
(sd < 60). The calibration is baked in the app's order (`cams.py`: spark, the cam on it, spark again;
`data/bake-maps.json`) on the simulator `main` ships, its keys with it: Current at launch. **Re-bake on every simulator
change; a content push runs the whole default tier first.** No sound level or camera per build (Tailpipe the default).

**Validation.** `content/validation/archetypes/<slug>.json` (by its `generate.py`): every target says its **basis**
(cited / recalled / judgement; a fitted part's rows judgement); the turbo four's cited torque curve as rows; hot starts;
idle spread; the sound's floors. `report.py` re-judges re-read rows against today's ranges, counts misses by basis,
lists the worst five, and measures the idle's standing tone itself. Timed runs by `quarter_mile.py` (shifts from the
power curve, revs held at the green, warm tyres), regenerated on each table's pin.

**Tables** (all kept in `docs/measurements/validation/`, named at real UTC): G 132 of 648, H 193 of 741, I 123, J 124 of
772, K (`ab107ca`) 120 of 839, L final (`066165a0`) 120 of 847 (its columns on K's content 116, L's seat-held cards 118),
M (`7f64dac1`) 115; N and O (114, 112) read locally and sent to the lead; **P (`abadfb4b`, pushed in `223f4301`) 113 of
847 (30 of 204 cited)**: rows held by calibration out of the counts, N2's ten builds re-timed (old beside new kept).

**Sources files:** `docs/measurements/validation/2026-11-24-2314-ea888-gen3-sources.md` (the turbo four: what is known,
what is guessed; gen 3's exhaust cam timing the one figure not found); `docs/measurements/2026-11-25-0032-judgement-sources.md`
(VGT vanes at idle, a diesel's cycle spread, turbine efficiency, fan switches, idle warm-up); the quarter mile one change at
a time (`2026-11-24-2215-quarter-mile-one-change.md`).

**Next:** G's six pops scenarios (content/scenarios); N1's two-stage switch against its crossing; the 2.0's air at 1750. **Open for others:** the long-seat
cards' idle vacuum (V10, V12: N1's overlap); the curves' sharp teeth (N1); the turbo four's low end (the turbine match, N2; gen 3's exhaust timing,
unpublished); the diesels' boost at the check speed in K (N2's correction); the diesels' idle hum (D2, N2).

**Manual** (`docs/manual/`): getting started, controls, glossary, workspaces, models, builds, parameters, help.tsv,
validation (the newest table). **Not run:** macOS and Windows.
