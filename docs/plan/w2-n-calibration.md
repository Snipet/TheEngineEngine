# Card N: the numbers (calibration against the misses table)

*Wave 2, added by the lead 2026-11-24 16:05 UTC, when card P's first full table existed
([../manual/validation.md](../manual/validation.md): 582 targets on 30 archetypes, 190 missed). Two sessions: **N1**
the engines that breathe by themselves, **N2** the boosted ones. Repository TheEngineEngine, branch
`claude/awesome-gates-ntdb58`. Read [01-rules.md](01-rules.md) (§2's push rule above all), `sim/README.md`, then the
model docs of the modules you will open (`docs/models/`) and their handoffs (`docs/handoffs/`).*

## What the table says

The misses are patterns, not scatter. All thirty archetypes run with every model switch on.

1. **Every engine that breathes by itself lands near the same BMEP, whatever its head.** At peak torque
   (BMEP = 4 pi T / V): the 1.5 L economy four 13.4 bar (its kind measures 11.7 to 13.1), the pushrod V6 11.5 (9.9 to
   10.9), the roadster's twin-cam 12.9 (10.8 to 12.0); but the high-revving 3.2 L six 10.8 (13.9 to 14.7), the
   flat-plane 4.5 L V8 12.1 (14.5 to 15.6), the two-stage-cam 2.0 L 12.5 (12.6 to 13.5). A mild two-valve is too
   strong and a tuned four-valve too weak: valve area, cam, runner and exhaust tuning move the result too little.
   Peak power of the high-revving ones is 25 to 30 % short (299 kW for 400 to 445; 201 for 245 to 260; 138 for 165
   to 185).
2. **Every boosted engine is too strong, by 20 to 100 %.** The 2.0 L turbo four 29.5 bar (22 to 25); the kei triple
   103 kW (44 to 50); the supercharged V8 512 kW (395 to 435); the boxer 303 kW (210 to 235). Candidates, none yet
   measured: the turbine is fed by a stand-in (`Boost::exhaustStandIn`) so the engine may see no back pressure for
   its boost (card G wires the real pipes today); boost held flat to the limiter where a small turbo runs out of
   flow; the charge too cool after the intercooler; no knock-limited spark or enrichment at boost; a supercharger's
   drive power.
3. **The rotary's torque is doubled** (582 N m for 290 to 315).
4. **Big-cam engines idle high** (V12 1244 rpm for 850 to 1000; V10 1166; flat-plane V8 1340).
5. Quarter miles and top speeds follow the power misses, except **the tractor's top speed of 13 km/h**, which is a
   broken run, not physics.

Card H's last commit (f8be3c8: the full burn with Woschni's walls, the spark at the half-burn; switched by
`engine.wallHeat` until the integrator ties it to `fuel.model` 1) adds 16 to 26 % on the default V8 (12.6 bar) and
38 % at 13,500 rpm on a superbike. It closes part of the top end's gap and pushes pattern 1's low end further over.
So: measure before changing anything.

## What you own, and what you may change

**OWNS**: N1 `scripts/ledger.py`, `docs/measurements/calibration/na/**`, `tests/sim/calibration/na/**`,
`docs/handoffs/n1-calibration.md`. N2 `docs/measurements/calibration/boost/**`, `tests/sim/calibration/boost/**`,
`docs/handoffs/n2-calibration.md`. N2 uses N1's ledger script when it lands and adds its boosted lines to it by
telling N1 (one owner a file).

**In other packages' modules** (the rule Q works under, widened by the lead for this card): you may commit a change
in a `sim/` module yourself when all of these hold: it is reached only with that package's model switch on (the
null test stays 59 of 59: run it once); it has a physical reason and a source, stated in the module's header comment
in the module's own style; the package's own tests pass or are changed with the reason in the commit; one cause a
commit; and the owner is told, with the numbers before and after. Cards E (valvetrain, intake), F (boost) and H
(combustion, ECU) have handed off and are not running: you act for them on those paths, and report to the lead.
Cards D (the integrator, `sim/engine.fm`, `sim/core/`), G (exhaust), I (vehicle), T, D2, B2, L and M are running:
their files change through them (message the session; the lead for I). The archetype builds
(`content/builds/archetypes/`) are made by card P's generator: a build that is wrong (a kei turbo that holds boost
to the limiter) is a message to P, with the part it needs.

**Never**: tune a constant until a number fits. A change moves every archetype of its kind the right way, for a
reason a textbook gives. If a target looks wrong (the wrong car's figure, a figure at the wheels read as at the
crank), say so to P with the source.

## The work

1. **The loss ledger** (N1 writes the script, first, within the first forty minutes, and pushes it; N2 starts by
   hand with `tee-dyno` meanwhile). For an archetype, on the engine dyno (`MODE_DYNO_ENGINE`, full load, warm) at
   five speeds from 1500 rpm to its limiter: volumetric efficiency against manifold density and against the air
   outside (trapped mass at intake closing); gross IMEP, pumping MEP, friction MEP, BMEP; brake efficiency; lambda;
   the spark and the half-burn angle; charge temperature at intake closing; manifold pressure; pressure at the
   exhaust port, mean; port gas temperature. Boosted: pressure ratio, mass flow, compressor efficiency, shaft speed,
   intercooler out, turbine inlet pressure, the gate's opening, the drive power of a supercharger. Each line beside
   the range a textbook gives for that class of engine, with the source (Heywood, *Internal Combustion Engine
   Fundamentals*; Blair, *Design and Simulation of Four-Stroke Engines*; the Bosch *Automotive Handbook*; Taylor).
   Output `docs/measurements/calibration/<na|boost>/ledger.md`, generated, never edited by hand. Use the telemetry
   that exists (`tee-schema --markdown`, and `tee-dyno --fields` to record any of it at each point); a quantity that is not there is asked of its module's owner (an appended
   `tel.` field), or computed in the script from the Scope's arrays.
2. **The cause of each pattern**: which ledger line is out of its range, on which builds, by how much; and the one
   experiment that isolates it (change that one thing; the line moves into range and the torque with it). A
   paragraph a cause in `docs/measurements/calibration/<na|boost>/causes.md`.
3. **Fix or route** (the rule above). N1: patterns 1, 3 (with D) and 4. N2: pattern 2, and the tractor's run (with
   D2 and P).
4. **The table again**: `content/validation/archetypes/report.py`, before and after, misses counted by pattern.

## Done when

Power and torque are inside their ranges on at least 24 of the 30 archetypes and no archetype is off by more than
15 %; every cause has its paragraph, its experiment and its source; tests in `tests/sim/calibration/` hold what was
fixed (BMEP at peak torque by class of engine; volumetric efficiency by class; friction MEP against Chen and Flynn at
three speeds; back pressure against boost on a matched turbo), each under a minute in the default run (the fast
dyno, `tee::PredictService`, where it is enough), longer ones `[nightly]`; the cost against R is unchanged within
0.01 for every model you touched (`tee-bench`), or the lead is told what it costs. Nobody here can hear: after a
change to the burn, the valves or the pipes, list the scenarios a person should listen to.

## Not in this card

Cost (card Q). New features. The reference path (`*.model` 0). The sound's mix (card L). Tyres and gearing (card I),
except to say that a quarter mile still misses with the power right.
