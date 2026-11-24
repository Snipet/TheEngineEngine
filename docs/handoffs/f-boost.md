# F: forced induction (standalone handoff)

Card F ([w1-f-boost.md](../plan/w1-f-boost.md)), branch `claude/awesome-gates-ntdb58`. The model (`boost.model = 1`)
is built and proven on card F's made-up engine. **Integration waits for K3b** (`docs/handoffs/k3b.md` and
`sim/core/monolith.fm` gone); nothing outside card F's paths was touched. Equations, sources and judgement:
[docs/models/boost.md](../models/boost.md).

**Done, standalone** (`tee-tests-boost`, ctest `boost.*`; the rig is `tests/sim/boost/rig.fm`, a 2 L four as an air
pump with steady or pulsed hot exhaust through waveguide pipes into the turbine's housing):
1. map: choke flow with the inducer's area (to 0.2 %); the knee's tau with tip speed squared; 60 mm wheel at 150 krpm
   makes 2.5 to 3.1; the efficiency's peak where set (`MapTest`).
2. bigger turbine A/R spools later and chokes later; bigger compressor lags more; time to boost grows with J (`TurboTest`).
3. steady state: turbine = compressor + bearings + rotor's energy change within 0.3 %; mass kept within 0.5 %.
4. PI on the target within 5 % at every speed where the turbo can make it (measured within 0.6 %); spring alone
   within 5 % at its actuator (the compressor's outlet).
5. throttle shut, no valve: deep surge at 26 Hz, reverses, swing dies to under a tenth as the rotor slows; with a
   valve (to the air or recirculating) the flow never reverses (`SurgeTest`).
6. pulses into a twin scroll at 2000 rpm, rotor held: 2.2 times the single scroll's turbine power, same mean flow.
7. Roots keeps over half its most boost 2000 to 6500 rpm (centrifugal 0.09); hotter than a matched twin-screw (440 K
   against 394 K at 145 kPa); centrifugal boost goes as speed^2.1; crank power = air's work over efficiency within 3 %.
8. intercooler outlet = T_in - eps (T_in - T_core) within 1.5 K, its core soaking; nitrous's torque step 90, 180, 270
   N m against 89, 178, 267 rated (`ChargerTest`).
9. housing passive for 40 random sizes (worst returned 0.987 of what it took); ten minutes of random chops on random
   builds stay finite and bounded (`SafetyTest`). Also: at rest the flows die away (`RestTest`; orifices between
   volumes solved implicitly, volumes' mass in double); sequential, compound, vanes, anti-lag, assist (`LayoutTest`).

**Cost** (this machine, load average 9 to 18 on 4 cores; ratios measured in the same window): R (tee-bench, V8
free-revs) 6162 ns; the machine with two turbos some 300 to 490 ns (0.05 to 0.08 R), one twin-scroll pair some
540 ns (0.09 R), a Roots 50 to 110 ns; 0 work when the kind is none (it returns at once). Within 0.12 R.

**Seam with card G** (to write into `sim/README.md` with G's agent, once K3b lands), all per sample, `Boost` the
carved module's name for `BoostMachine`:
- `int Boost::housingOf(int bank, int group, int banks)`: the housing a collector ends in, -1 none.
- `float Boost::housing(int h, float drive, float admit, float tGas)`: drive = sum 2 a_i / z_i (m^3/s), admit = sum
  1 / z_i, tGas K; returns the housing's pressure above the outside, Pa; G writes b_i = p - a_i.
- `float Boost::downpipe(int side, float a, float z)`: returns the downpipe's end pressure above the outside (G
  writes p - a); `dpMass[side]` kg/s and `dpTemp[side]` K are what went in.
- `float Boost::gatePipe(int t, float a, float z)`: the same for an external gate's own pipe (`gateMass`, `gateTemp`).
Intake (card E): `float Boost::pressure()`, `temperature()` where the reference read pBox and tAir;
`void Boost::draw(float m)` the throttle's mass flow; `tick(omegaCrank, pPlenum)` after; `control(d16)` every 16.
Combustion (H): `float Boost::oxygen()`. Mechanics (D): `crankTorque` (N m). ECU (H): fields `cmdTarget` (Pa, below 0:
the spec's), `cmdVanes`, `cmdAntiLag` (kg/s), `cmdNitrous`, `cmdPurge`; the PI loop is card F's.

**What waits for K3b**: append `machinespec.fm`'s fields to `sim/spec/boost.fm` (K3's `kind` and `size` exist: drop
mine), the carved `Boost` calling `BoostMachine` when `model = 1` (0 stays the reference bit for bit, null test), the
seams above with G and E, `tel.boost` and its Scope arrays (the map's lines, contours and trail are already fields),
integrated tests (2 L turbo four's boost and BMEP, drive pressure 1 to 2 x boost, shaft speeds, bench).

**Outside my paths**: none needed yet. Card E's throttle should be solved implicitly against the boost volume and the
plenum (`boostOrificeStep`, as the rig does). FMinus issues: none met.
