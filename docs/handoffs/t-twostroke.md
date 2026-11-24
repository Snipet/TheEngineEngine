# T: two-strokes (handoff)

Card T, branch `claude/awesome-gates-ntdb58`. Model, sources and measurements: [docs/models/twostroke.md](../models/twostroke.md).
**Done** (`tee-tests-twostroke`, 17 cases, all pass in this container; nothing run on macOS or Windows):
- Tuned length (`PipeTest`): the weak response was the measurement (straight-pipe baseline cut at 0.98 m, so x1 and x1.25
  were divided by the same pipe). Against an endless pipe: x0.8/x1/x1.25 centred -1.8/-1.4/-5.3 % from the returning
  wave (before: 11 548/9 627/9 320 rpm, now 13 033/10 466/8 044). On the pipe: +42 % torque in 1000 rpm. Cones: the
  tuned speed settles from 3 steps (moves under 1 %).
- Cylinder (`CylinderTest`, `FeatureTest`): ports as rounded windows from widths and durations; sprung reed with its own
  frequency, rotary disc, piston port; displacement/mixing/short-circuit scavenging and purity; power valve (+37 % at
  4000 rpm, none in the band); at rest nothing flows; mass kept to 1e-7, energy to 0.01 % of the burn (closed-form T
  from energy with gamma(T); the old update lost 4 %); 60 s random soak finite.
- 1 to 4 cylinders, even or together, a chamber each; builds `content/builds/engines/two-stroke-{mx125,road-twin,triple,
  kart,utility}.json`. `EngineTest`: each starts, idles 2280-2930 rpm, misfires on
  80-84 % of idle cycles unscripted and never on the pipe, peaks within 10 % of its class's speed; peak sample <= 0.92.
- Spec: `spec.engine.cycle` + `ts*` fields, one marked block in sim/spec/engine.fm (D's later `wallHeat` follows it).
- In the engine (988b442, allowed by the lead, for D's review): `cycle` 1 on a custom engine runs `twoStrokeTick` in
  place of cylinders/exhaust/intake, lay.cyc 360, Scope 360, tel.engine.ts* (purity, delivery, trapping, power valve,
  pipe gas, crankcase, reed). At `cycle` 0: one integer test a sample; whole null test green.
**Not done / honest gaps**:
- Output is 41 % (125 MX), 72 % (250 twin), 65 % (500 triple), 46 % (kart), 82 % (utility) of the classes' published
  powers: the crankcase pump delivers ~0.88 with no inlet-tract or transfer-duct inertia. Next: an inlet Duct to the reed.
- Spectrum: lines sit on multiples of the firing frequency (twins/triples cancel the rest by 100 dB), but the strongest
  radiated line on a chamber is the header's ~870 Hz ring, the firing line 7-27 dB under it (monopole +6 dB/octave,
  chamber cones pass highs). Physics, not the card's wording; flagged.
- Not fed for a two-stroke: work.sample (IMEP, PMEP read 0), exh.mdot/tPort (the pipe's gas is tsPipeTemperature).
**Cost** (`tee-bench --engine`, free-revs, share of that run's reference V8): single with chamber 0.32 R (card: 0.35 R),
twin 0.59 R, triple 0.87 R, utility 0.24 R. Rig alone: 0.28 R. Fewer cone steps (3) would roughly halve the ducts.
**Wanted outside my paths**:
- sim/engine.fm, sim/spec/telemetry.fm, sim/README.md (D's): committed by me with the lead's leave; D to review.
- sim/exhaust/duct.fm (G): `pack()` and `extra(lowpass)` pass up to 4.4 % more than they take near 25 Hz around the
  steady-flow tracker (|H(1-LP)+LP| > 1); a stopped pipe closed at one end rings for ever. Sent to G with numbers; I use
  a broadband gain meanwhile.
- ECU (H): leaves cranking when the sensed speed passes 550 rpm; a light 50 cc engine spikes past it on one weak fire
  and the starter stops. The utility build has `ecu.starter` off (a pull start) instead.
- spec.engine's accessory defaults (300 W alternator, a valvetrain's friction) suit cars; each two-stroke build sets its own.
**FMinus**: no issues met (module fields cannot be `const`: used an enum-level `const float` instead, as documented).
**Listen to**: idle on every build (irregular firing, the pipe's ring between firings); the 125 and twin pulling
8000-11 000 rpm (coming on the pipe, the power valve opening); overrun from redline (four-stroking returning); the
triple's 120-degree beat, the kart at 14 000, the utility behind its can.
