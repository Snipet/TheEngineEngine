# N2: the boosted engines (card N, pattern 2) — handoff

Branch `claude/awesome-gates-ntdb58`. Measurements in `docs/measurements/calibration/boost/` (README, backpressure,
by-hand, gate, causes, gamma, design-point); tool `tee-calib-boost`; tests `tee-tests-calib-boost` (9 cases) and
F's `tee-tests-boost` (34; the 10-minute chop soak `[nightly]`, a one-minute form in the default run).

**Causes found and fixed** (causes.md): no back pressure at the ports (G's housings); the gate's runaway (aGate =
aNoz/0.65); a rotary matched to half its air (D); clicks (X-010/014/015); a one-bank twin's second turbo stood
(Boost::exhaustSides); the VGT's two states (surge guard: vanes open at 1/s while the loop's own compressor surges,
give back 0.1 of it; on the closed exhaust D2's truck spools to the single's 90 % in 2.60 s against 2.80); the
back-pressure limit 1.4 (re-measured on real exhaust mass: tractor 1000 rpm 1159 against 790 N m without it).

**Since the exhaust's mean flow closed** (G 8bb2c80f): the over-boost test's small gate is 18 mm (26.3 held 105 kPa);
the blow-off chop's check is "no surge cycle" (no reverse flow, no run over 2 blocks; the lead's ruling); the kei's
26 mm gate was dropped (P: the matched one holds its taper); the rotary's 5200 rpm hysteresis is gone.

**Built for others**: the over-boost cut (ecu.overBoost); Boost::testCell (the engine dyno's cooler held at the air,
SAE J1349: the four +4 to +5 %, up against down within 0.6 %); spec.boost.heatSettle; tel.boost.mismatch with has/
wants pairs (cooler, spring, pipes, compressor; U3 draws them); the inlet's stiffness in the orifices that end in it
(X-024) and the filter's loss bounded (X-026); the knock-limit table's ECU side (ecu.maps.knockLimit, knockLimitAuto,
knockLimitKey; tel.ecu.knockTable; advanceAsked per charge). C bakes the table (tee-dyno --bake-spark).

**The compressor's map** (compressor-map-model.md): from published maps (knee, choke to M 1.63, island 0.61); flag
and guard at 0.9 of the knee (the model's measured onset); sizing at the design tip speed; design flow at the charge's
temperature. Marked findings: the VGT limit at its open stop, the wastegate overshoot at 3500, the sequential join.
**Open**: (1) the surge cap, held, to try on this map. (2) The rotary: its join cycles at
4250 (26 to 106 kPa); its top now 68 to 74 kPa. (3) The knock table's proof, when C's boost-sweep bake exists. (4) The pickup's top end (vanes fully open at p3/p2 1.41). (5) The boost package's CPU budget (80 s).

**Outside my paths, done by owners**: D (overBoostRef, rotary x2 vd, stand-in dropped, testCell's line), T, D2
(vanesAndPurge, its brake governor), G (externalGates, exhaustSides), N1 (the blower's leak inflow, X-026's seeds).

**Cost**: sizing at apply time; cut, limit, guard, warnings: compares every 16 samples (tee-bench not run).

**Listen** (nobody here can hear): the tractor at 1300 and the pickup at 1600, full load (far more boost); the
rotary at 4250 (its join's surge); the 2 L four at full load 5000-6500 rpm; a tip-in at 5000 rpm; the over-boost cut
(gateDiameter 0.018, ecu.overBoost 150000, full load 5300 rpm): a rhythmic fuel cut; the kei's taper at 6000-7600;
the twin six: two whines at one pitch; the truck's VGT tip-in at 1300 rpm: no surge chuff once spooled; a throttle
chop with the blow-off valve: one soft puff, no flutter.
