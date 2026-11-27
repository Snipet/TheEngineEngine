n2-match-a.patch: N2's step A of the turbine's match (turbine-match-design.md), five code commits (format-patch) on base 9c5067bc, for C's bake stage; NOT for the branch until the stage lands with it (the lead's ruling). Adds spec.boost.ratedP4, ratedT3, ratedEtaC, ratedAir (hidden) and matchKey (hidden, unit hash); the rated balance in the match when they are baked; tel.boost.mismatch 32 (turbine on stand-ins) and 64 (a governed diesel's compressor failing rule (c)'s rated check); the mismatch case restated on its bits; tel.boost.turbineOut[] and engineAir for the bake to read. Measured: turbine-match-step-a.md. Apply with `git am`.

n2-package.patch: the boost package for the diesel window, eleven code commits (format-patch). Base: 1fa216c0, as n2-b.patch's; it replaces n2-b.patch (its first five commits are n2-b's). Apply with `git am docs/handoffs/patches/n2-package.patch`.
Adds: the compressor map fitted to every cited map (rise, knee, fall, choke scale; one floor at tip Mach 0.67) and MapTest's two knee cases restated on the cited lines; the VGT loop's integral held while the vanes cannot follow (conditional integration, vgt-windup.md), and held while the ECU owns the vanes (an engine brake); the inlet's pressure solved in the sample while a recirculating blow-off is open (X-039, with its case InletTest.cpp); the shaft's ceiling in the boost loop at 0.948 of omegaMax (shaft-ceiling.md). sim/engine.fm carries two call lines (D's file) for the integrator. The taper (the target from the fuel curve) is a build's data, not in it. On this series alone TurboTest :88/:108/:109 and BrakeTest :226/:234 are red: they need D2's fixture and brake.

n2-b.patch (superseded by n2-package.patch): N2's (b) series as five code commits (format-patch). Base: 1fa216c0. Apply with `git am docs/handoffs/patches/n2-b.patch` on 1fa216c0 or a later branch tip.
What it carries: (b), the torque-peak compressor sizing with its charge terms; the vane surge guard removed (and its LayoutTest case); a VGT's matched nozzle passing the whole design flow, its stop scaled to the same shut area.
Not final at the rated end (the lead's ruling pending); for D2's brake retune. Pushed as docs because code is frozen; it goes in through the package window, not from here.

n1-pops-seam.patch: N1's half of the pops' seam (sim/ecu/overrun.fm's FuelAccount under Pop::burnPipe, combustion.fm's account calls, and the quick test and probe in tests/sim/ecu/IntegratedTest.cpp), a plain diff against 856a4ee4. For G's review before the code push (docs/handoffs/g-exhaust.md, "The pops' seam"); not applied from here.

d-clutch-synced.patch: card D's (the integrator's) "a clutch that has synchronised stays closed", for the vehicle model's
paddles (sim/vehicle/gearbox.fm, `synced`: one latch for both directions, replacing c1ab538a's overrun hold) and its two
GradeTest cases (tests/sim/diesel/GradeTest.cpp). A plain diff; base f3639cb9 (applies there with `git apply`); the
lead's ruling and acceptance of 25 Nov, for the next code window (local commit 87629688 on D's side). Once locked in a
gear (the slip under 0.5 rad/s, engaged) the clutch is held closed by the road's speed through the gear, opening only as
that nears the idle (50 to 250 rpm over it), until a shift begins, neutral, the clutch pedal, or the road at the stall
band's foot. Before/after: all 46 vehicle cases and combo's fast tier identical (prints and assertion counts); combo's
dog box and GradeTest's overrun case move as the law says. Given to D2 for the diesel package's GradeTest :169.

d-bike-synced.patch: card D's patch for B2, the lead's ruling of 25 Nov: bike.fm's launch helper gets the same law (once
locked in a gear, the slip under 0.5 rad/s and the lever out, the clutch is held closed by the output's speed through
the gear until a shift, neutral, the lever pulled or a stall). Box::control takes the lever's engagement as a new last
argument. Measured with it applied (bike suite, non-nightly): one print moves, "engine lash ... last closing 4.62 -> 4.66
rad/s" (the overrun half of the on-off cycle no longer slips the clutch at low revs); every other print and every case's
assertion count identical. The output's speed carries the rear tyre's slip, where the vehicle model reads the road's:
B2's choice. A plain diff; base f3639cb9. Not applied.
n1-predict-stage.patch: N1's cure for P's two-stage notch (host/src/Predict.cpp: a point under the stage switch approached from below, the latched stage let down and the speed ramped at 100 rad/s^2; a host test that fails without it), a git format-patch against 60e87aef. For C's review (the predictor is C's); not applied from here.
n1-latched.diff: N1's FLAG_LATCHED (tel.engine.flags 8: sim/engine.fm one line for D, sim/spec/telemetry.fm, the valvetrain's latched()) and the predictor's let-down on it with PredictPoint::latched and the bake versions (cam 4, knock 6), a plain diff against 60e87aef, superseding n1-predict-stage.patch. For C's and D's review; not applied from here.
g-bang-hot-gas.patch: G's next-window item 1 (a), held by the lead's ruling (26 Nov) until L's exposure rule takes dense bangs. Under Burned in the pipe a turbine housing books the burns' gas at its hot temperature (Slots::houseT: T (1 + q_burns / q_ports), 20 ms means a bank, at most 2600 K), not the collector's: the rally five on its limiter booked 3.1 to 3.5 times the valves' mass and read 7.54 bar at the port, 8.94 at the turbine and dpWave -350 kPa on the tour; with it 4.98, 6.84 and -155 (Carved 3.84, 4.72, -92), the click at 8.74 s gone (0.088 against Carved's 0.098). It also holds BangTest's two measure cases (the limiter's pressures and books; the largest output steps) and the doc's section. Its one red on the tip: L's "a burn in the pipe does not hold the body down", Burned 3.2 dB under Carved against 2 dB: the raw body unchanged (-14.4 dBFS before and after), the auto gain 2.6 dB lower (-13.3 dB against -10.7) because the limiter's bangs are denser (rms +2.2 dB). A git show of commit 301ba06c (git apply works on the tip of 26 Nov); not applied from here.

l-places-true-levels.patch: L's window item 20 (the owner's ruling of 27 Nov, "true levels with a fixed gain";
docs/plan/places-sound.md). A plain diff; base 6ca1e80e (applies there with `git apply`). sim/spec/sound.fm autoLevel's
default 1 -> 0; sim/listen/listen.fm one fixed staging under true levels (+3.7 dB, a level match at first.json's mid
load at the default camera, stated in the comment); sim/listen/scene.fm the Tailpipe camera a boundary microphone on the
ground (in line with the right-hand mouth, 0.29 of the dial's distance back, at least 0.25 m) and the ground folded into
each capsule's own path there (below 5 cm only: every other camera and height bit-identical, shown); the cases restated
(tests/sim/listen) and two new ("true levels at the defaults", "the Tailpipe camera, a boundary microphone"); listen.md
and the parameter page's autoLevel line. Its whole default tier: 582 of 583 green; the one red is host.sim.bakes (the
shipped V12's knock table no longer current on the changed program), which the bake after any sim/ item clears.
Model 0 untouched (ref.null green). The evidence to repeat: docs/plan/places-sound.md, "The evidence for item 20".
