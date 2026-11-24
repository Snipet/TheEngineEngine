# F: forced induction — standalone handoff

Card F, branch `claude/awesome-gates-ntdb58`. `boost.model = 1` (compressor map from sizes, inertial duct that surges,
turbine housing and nozzle, shaft, charge path and intercooler, gate/PI/vanes, blow-off, Roots/twin-screw/centrifugal,
sequential/compound/quad, anti-lag, nitrous, sounds) is built in `sim/boost/` (my files beside K3's `boost.fm`; no name
clashes) and proven on a made-up engine (`tests/sim/boost/rig.fm`). Model, numbers and judgement: `docs/models/boost.md`.
**Integration waits for K3b** (`docs/handoffs/k3b.md` and `sim/core/monolith.fm` gone): not started.

**Done, standalone** (`tee-tests-boost`, ctest `boost.*`, 26 cases, all pass):
1 map: choke flow with inducer area (0.2 %), knee tau with U^2, 60 mm at 150 krpm 2.5-3.1, efficiency peak placed.
2 bigger turbine A/R spools and chokes later; bigger compressor lags more; time to boost grows with J (monotonic).
3 steady: turbine = compressor + bearings + rotor energy within 0.3 %; mass kept within 0.5 %.
4 PI holds target within 5 % wherever the turbo can make it (measured 0.6 %); spring holds its own within 5 %.
5 shut throttle, no valve: surge 26 Hz, flow reverses, swing dies to 1/60; valve (air or recirc): never reverses.
6 twin scroll at 2000 rpm (rotor held): 2.2x the single scroll's turbine power from the same mean flow.
7 Roots keeps >0.5 of its peak boost 2000-6500 rpm (centrifugal 0.09), 440 K vs matched twin-screw 394 K at
  145 kPa; centrifugal boost ~ speed^2.1; crank power = air work / efficiency within 3 %.
8 intercooler outlet = Tin - eps(Tin - Tcore) within 1.5 K, core soaks; nitrous step 90/180/270 vs 89/178/267 N m.
9 housing passive over 40 random sizes (worst 0.987 returned); 600 s of random chops on 29 random builds bounded.
Also: at rest every flow dies away (implicit orifices, double-precision volumes); sequential, compound, vanes,
anti-lag, assist each do what they should (`LayoutTest`).

**Cost** (least of five interleaved; machine load 9-18 on 4 cores, ratios only): over the rig, one turbo 289 ns a
96 kHz sample, two 394, two twin-scroll 576, Roots 123; R (tee-bench V8 free-revs, same session) 6162 ns: two turbos
~0.06 R, twin-scroll pair ~0.09 R (share 0.12 R). Kind none, no nitrous: returns at once. Not run idle or Release.

**Seam I need from card G** (per sample; `Boost` = the carved module holding a `BoostMachine`):
`int Boost::housingOf(int bank, int group, int banks)` (-1: no turbine);
`float Boost::housing(int h, float drive, float admit, float tGas)` — drive = sum 2a_i/z_i, admit = sum 1/z_i;
returns housing pressure above outside, G writes b_i = p - a_i;
`float Boost::downpipe(int side, float a, float z)` — returns end pressure, G writes p - a; `dpMass[side]`, `dpTemp[side]`;
`float Boost::gatePipe(int t, float a, float z)` — external gate's own pipe; `gateMass[t]`, `gateTemp[t]`.
Intake (E): `pressure()`, `temperature()` for pBox/tAir, `draw(float m)`, then `tick(omegaCrank, pPlenum)`;
`control(d16)` every 16. Combustion (H): `oxygen()`. Mechanics (D): `crankTorque`. ECU (H): `cmdTarget` (Pa, <0 =
spec's), `cmdVanes`, `cmdAntiLag` (kg/s), `cmdNitrous`, `cmdPurge`; the PI loop is F's.

**Remaining after K3b**: append `machinespec.fm`'s fields to `sim/spec/boost.fm` (use K3's `kind`, `size`; drop mine);
`Boost` calls `BoostMachine` when `model = 1`, 0 stays the reference (null test); agree and write the seams above in
`sim/README.md` with G and E; `tel.boost` and Scope arrays (map lines, contours, trail already exist as fields);
integrated tests (2 L turbo four's boost and BMEP, drive pressure 1-2x boost, shaft speeds, bench).

**Outside my paths**: none needed. Suggest card E solve its throttle implicitly (`boostOrificeStep`, as the rig).
FMinus issues: none. Listen to: surge after a chop with no valve; blow-off; Roots vs twin-screw whine; anti-lag.

## F: integrated (after K3b), 14:30

Branch `claude/awesome-gates-ntdb58`; pushed with the null test identical (10 cases, every pair) before each push.
**Done**: my Spec fields appended to `spec.boost` (`sim/spec/boost.fm`; `machinespec.fm` gone); `tel.boost` and the
Scope's map fields; `Boost` (`sim/boost/boost.fm`) holds the machine and fills the integrator's hooks (`pressure()`,
`temperature()`, `draw`, `tick`, `control16`, `crankTorque`, `oxygenShare()`, `housingOf/housing/downpipe/gatePipe`),
model 0 the reference's code untouched; my calls added to `engine.fm` at my hooks only (`applyMachine` in configure and
on rev, `airSpeed`, the exhaust stand-in, `telemetry`, `scope`, model 1's restriction off); `sim/README.md`'s Boost
section and hook table; tests to the glob's rule (`tee-tests-boost`, 30 cases, all pass); builds
`content/builds/boost/{turbo-four,twin-turbo-v8,roots-v8}.json`.
**Integrated tests** (`EngineTest.cpp`, 2 L inline four, bench and dyno): matched turbo 19 kPa at 5500 rpm, 63,000 rpm;
size 0.55: 59 kPa (charge pipe 65, spring 70), 168,000 rpm, drive ratio 1.46; dyno 5500 rpm: 17.3 bar BMEP at 72
kPa against 9.0 natural (production 2 L turbo fours 15-25; makers' torques); Roots: 32 kPa, 8.7 N m off the crank;
a chop without a blow-off valve flags surge in 82 of 100 blocks, with one in none.
**Finding for D/G/H**: the engine's port gas is cool, `exh.tPort` 450-560 K at full throttle (real: 900-1100 K), so
turbos matched to the engine make little; the turbine takes what the engine gives.
**Cost** (`tee-bench --engine`, free-revs, load ~4.5): V8 1.062 of the reference, twin-turbo V8 1.198, Roots V8 1.102,
turbo four 1.203. `--profile` (twin-turbo V8): BoostMachine 297 ns, BoostEnd 152, BoostCompressor 54, Boost 47,
BoostSound 12: about 560 ns, **0.24 R against the 0.12 R share: over**. Levers: warm-start BoostEnd's pipe-end solve
from last sample's ratio (one pass, not three: about -100 ns); the charge path's volumes in float with a double
residual; the sounds' noise at 48 kHz. For package Q.
**Not done, with card G**: the housing's per-sample sequence in `sampleTick` step 4 (README, Boost and the hook table:
my side written; G's network not wired yet). Until then `Boost::exhaustStandIn` feeds the housings with the cylinders'
gas; remove it when G's calls go in. Nothing else outstanding with G: its handoff accepted my signatures.
**For the interface (U3)**: Spec paths `boost.model, layout, compInducer, compExducer, compAR, compEta, turbWheel,
turbAR, turbEta, scrolls, housingVolume, vanes, vaneMin, inertia, bearing, assist, maxSpeed, gate, gateDiameter,
gateSpring, target, overBoost, blowOffKind, blowOffSpring, blowOffDiameter, icEffect, icDrop, icFlow, icMass,
chargeVolume, charger, scDisplacement, scRatio, scVolumeRatio, scStepUp, scBypass, antiLag, nitrousShot,
nitrousBottle, seqFlow` (plus K3's kind, level, size, blowOff). Telemetry `tel.boost.pressure, turbos,
shaftSpeed[4], ratio[4], flow[4], efficiency[4], compressorOut[4], gate[4], vanes[4], turbineIn[4],
turbineInTemp[4], drive[4], chargePipe, intercoolerOut, throttleIn, blowOffValve, superchargerPower, oxygen, flags`.
Scope `boostMapRev, boostLineSpeed[8], boostLineFlow[8][16], boostLineRatio[8][16], boostSurgeFlow[8],
boostSurgeRatio[8], boostContourEta[5], boostContourFlow[5][16], boostContourRatio[5][16], boostTrailAt,
boostTrailFlow[256], boostTrailRatio[256]` (while ctl.scopeOn). The same map without the Scope: mirror
`BoostCompressor::size/speed/rise/efficiency` as `BoostMachine::mapLines` loops them (docs/models/boost.md).
**Not run**: the whole suite after the last merges (my targets, `tee-tests [null]` and the engine tests only;
`tests/sim/core/Engine.h` did not compile at one merge: D's, from the Build rename).
