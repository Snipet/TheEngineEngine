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
