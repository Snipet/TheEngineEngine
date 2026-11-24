# Forced induction as a machine (card F, `boost.model = 1`)

*What `sim/boost/` computes, the numbers it uses and where each comes from. "Judgement" marks a number chosen by
reasoning about how such machines behave, not taken from a source; the tests below say what each choice produces.
Nobody here has heard any of it.*

`boost.model = 0` is the reference's behavioural turbo (`Extras::turbo16` and its sounds), unchanged. `model = 1`
replaces it with the model below. Until K3b lands, the model runs standalone on a made-up engine
(`tests/sim/boost/rig.fm`) and in the engine; its Spec fields are `spec.boost`'s from `model` on (`sim/spec/boost.fm`).

## Files

| File | Module | What |
|---|---|---|
| `gas.fm` | `BoostVolume`, `BoostEnd`, `BoostBand`; `boostOrifice`, `boostOrificeStep`, `boostStiff` | gases, the compressible orifice, a volume, a pipe's end against a volume, a band-pass |
| `compressor.fm` | `BoostCompressor` | the map from sizes; the duct's inertial flow (surge) |
| `turbine.fm` | `BoostHousing`, `BoostTurbine` | the housing's volume solved against the exhaust's pipes; the nozzle's work |
| `shaft.fm` | `BoostShaft` | the rotor: inertia, bearings, assist motor |
| `supercharger.fm` | `BoostSupercharger` | Roots and twin-screw |
| `sound.fm` | `BoostSound` | the sources |
| `machine.fm` | `BoostMachine` | the arrangement, the charge path, control, the seams |

## Gases

Air: R 287, gamma 1.4, cp 1004.5 J/kg K. Burned gas: R 287 (the reference's), gamma 4/3, cp 1148, chosen so that the
isentropic expansion's exponent (gamma - 1)/gamma is exactly 1/4: `r^0.25 = sqrt(sqrt(r))`, no power function a
sample. The orifice tables (psi / sqrt(1 - r), 64 steps) are the reference's own (`sim/ref/pipes.fm`, gamma 1.30 for
burned gas, 1.39 for air), copied under new names; the small mismatch between the table's 1.30 and the energy's 4/3
is left (judgement: under 1 % in flow).

**Orifice.** `m = A p0 / sqrt(R T0) psi(r)`, choked below the critical ratio. Between two volumes it is solved
implicitly over the sample (`boostOrificeStep`): with `m = ko sqrt(dp)` and `dp` falling by `kk m` over the sample
(`kk = dt gamma R T / V` for each volume joined), `m = ko sqrt(dp - kk m)` is a quadratic, solved in closed form, the
reference's `lossFlow` idea. An explicit square root of last sample's drop chatters about zero at rest.

**Volume.** Mass and internal energy (in double: a flow of 3e-5 kg/s at rest changes a float mass by less than its
step), `p = m R T / V`, each flow carrying its own gas's enthalpy in, the volume's out. Stepped explicitly; the
inertial flows are updated after, against the new pressures (symplectic Euler: an LC pair neither grows nor decays by
the integration).

## The compressor

Sizes: inducer d_i (its annulus, hub 0.3 of it: `A_i = 0.91 pi d_i^2 / 4`), exducer d_e (tip radius r = d_e / 2),
housing A/R, peak efficiency.

- Tip speed `U = omega r`, tip Mach number `M = U / a01`.
- Knee (the speed line's highest ratio): ideal rise `tauK = 0.45 U^2 / (cp T1)`; the map is written in
  `tau = PR^(1/3.5) - 1`, so `PR = (1 + tau)^3.5` and `T2 = T1 (1 + tau / eta)`. 0.45 is judgement: a backswept wheel's
  work coefficient times its efficiency, set so a 60 mm wheel at 150,000 rpm makes 2.6 to 3.0 (test checks it; Garrett's
  GT28-class maps are in that range).
- Choke flow `mc = 0.45 rho01 a01 A_i (A/R / 0.6)^0.25 (0.3 + 0.7 min(M / 1.2, 1))`. 0.45 is judgement (an inducer's
  relative flow chokes near an axial Mach number of 0.5 to 0.6, less blockage); a 45 mm inducer gets 0.26 kg/s, about
  a GT28's published choke. The speed term and the A/R widening are judgement.
- Speed line in `phi = mdot / mc`: from shut-off `tau0 = 0.88 tauK` (flat toward surge) to the knee at `phiK =
  0.5 (A/R / 0.6)^-0.3` (clamped 0.3 to 0.7) by `3x^2 - 2x^3`; a quarter ellipse from the knee down to `tauC = 0.35
  tauK` at choke (vertical there); past choke `tau` falls at `8 (tauK + 0.1)` per unit phi; below zero flow the reversed
  branch `tau0 + 1.2 (tauK + 0.03) (phi / phiK)^2` (Moore and Greitzer's shape). All shape constants judgement.
- Efficiency island: `eta = etaMax - 0.9 (phi - phiB)^2 - 0.25 (M - 1.1)^2`, `phiB = phiK + 0.45 (1 - phiK)`, at least
  0.3 (judgement). Past choke the work a kilogram stays the choke point's (Euler's work does not fall with the ratio):
  without this an over-fed turbine ran its shaft away (the soak test found it).
- Power `|m| cp T1 tau / eta` plus windage `0.002 rho U^3 r^2` (some hundreds of watts at full speed; judgement).
- **Surge is the duct's dynamics**, not an effect: `dm/dt = (A_i / L) (p1 PR(m, U) - p2 - 4000 m)`, `L = 10 d_i +
  0.2 m` (judgement), solved semi-implicitly in m (the map's slope taken at the last flow) so the steep choke side is
  stable. The linear 4 kPa per kg/s is the duct's friction (judgement), which lets its Helmholtz ring die at rest
  (Q some 20). Greitzer's B is above 1 at full speed for these sizes, so a shut throttle surges by itself.

## The turbine

- Nozzle area `A = A/R x 0.0254 x R_c`, `R_c = 0.6 d_t` (A/R quoted in inches as housings are; R_c judgement), times a
  variable nozzle's vanes. Matched (A/R 0): the area that passes 65 % of the engine's matched flow at 2.4 bar, 1150 K,
  choked (judgement: the gate passes the rest at full power).
- Housing volume `4 pi R_c A` (judgement: the volute and its inlet), halved per scroll.
- Housing (`BoostHousing.solve`): `dp/dt = gamma p (D - Y (p - p_out)) / V + (gamma - 1)(H_in - cp m_out T) / V`
  with the pipes' drive `D = sum 2 a_i / z_i` and admittance `Y = sum 1 / z_i`, backward Euler in the pipes' part
  (passive at any step). The pipes' enthalpy flow is `q p cp / R` either way (its temperature cancels).
- Nozzle to downpipe (`BoostEnd`): the reference's `Valve.flow` (three closed-form passes, then choked) without its
  finite-wave factor, plus the housing's own give over the sample (`zc = dt gamma p / V`) so the two settle together.
  Several turbines on one downpipe: two Gauss-Seidel sweeps. A twin scroll: one solve at the scrolls' mean state, the
  flow shared by each scroll's own orifice law at the solved end pressure, the residual spread by area (so what leaves
  the housings is what the pipe takes).
- Work `P = m cp T (1 - r^0.25) eta`, `eta = etaMax x (2 - x)`, `x = (U_t / c_s) / 0.7`, `c_s = sqrt(2 cp T (1 -
  r^0.25))`: the textbook parabola peaking at a blade speed ratio of 0.7 (Watson and Janota, *Turbocharging the
  Internal Combustion Engine*, ch. 5). The torque `m c_s r etaMax (2 - x) / 1.4` is finite at rest (the stall torque),
  so the shaft starts from rest. Vanes nearly shut cost efficiency to 0.85 of it (judgement).

## The shaft

`J dw/dt = T_t - P_c / w - T_f + T_m`. `J = 0.0043 (8200 d_t^5 + 2700 d_c^5)` or given; the constant gives 2.5e-5 kg
m^2 for a 54 mm Inconel turbine and a 60 mm aluminium compressor (judgement; published rotors of that class are 1.5
to 4e-5). Bearings `T_f = c w + T0`: journal c = 1.6e-6 N m s (some 400 W at 150,000 rpm), ball 0.65e-6 (40 %:
manufacturers' comparisons put ball bearings at a third to a half), both scaled by (d_c / 60 mm)^2 (judgement).
Assist motor: torque `min(P/3000, P/w)`, commanded below the target with the pedal down.

## The charge path and control

Filter: `p1 = p_out - k m|m|`, 2.5 kPa at the engine's matched flow (judgement). Charge pipe v1 (0.4 of the charge
volume), intercooler duct (`A/L = 0.01`, loss `icDrop` at `icFlow` squared plus 4 kPa per kg/s, effectiveness
against a core of `icMass x 900 J/K`, cooled at 200 W/K plus 30 W/K per m/s of air; judgement), v2 (0.6) before the
throttle. The core is integrated every 16 samples (its warming a sample is under a float's step).

Wastegate: lift `(s - spring) / 10 kPa`, `s = (p_v1 - p_out)(1 - duty) + 0.03 (p_housing - p_out)` (judgement: stiff
actuator, small poppet push), 40 ms lag. PI on the boost before the throttle: `duty = 2.5 e + integral(4 e)`, e the
error over the target (here: card F owns the loop; card H gives the target). Vanes take the loop's output instead.
Blow-off valve: lift `(p_v2 - p_plenum - spring) / 10 kPa`, 3 ms lag; default spring 15 kPa (a vacuum-referenced
valve's few inches of mercury). Sequential: the second joins above 40 % of the matched flow (0.15 s, 80 % hysteresis).

## Superchargers

Positive displacement: delivered `rho1 D n`, leak back `boostOrificeStep` through `0.005 D^(2/3)` (judgement: about
75 % volumetric efficiency at a third of top speed, 90 % near it). Work a turn `p1 D 3.5 (Vi^0.4 - 1) + (D/Vi)(p2 -
p_i)` (under-compression; over-compressed, the first term), `p_i = p1 Vi^1.4`: a Roots is Vi = 1. Crank power: work /
0.9. Centrifugal: the compressor above at `crank x pulley x step-up`; crank power its power / 0.95. Matched sizes: D =
0.27 Vd (about PR 1.6 at a 2.4 pulley); a centrifugal's tip speed 300 m/s at the redline. Bypass: open with the
plenum's vacuum (5 to 30 kPa).

## Anti-lag and nitrous

Anti-lag fuel pools in turbo 0's housing and lights at random while the housing is above 750 K (about every 3 ms),
each event releasing the pool's `43 MJ/kg x 0.7` over half a millisecond (judgement throughout). Nitrous: mass flow =
rating / 2.13 MJ/kg: N2O is 0.364 oxygen; 3.41 kg of oxygen burns a kilogram of gasoline (43 MJ/kg), plus N2O's own
decomposition (1.86 MJ/kg), at a brake efficiency of 0.33 (judgement); evaporation 300 kJ/kg (judgement, between its
latent heat at 20 C and at its boiling point); bottle pressure falls to 65 % as it empties (judgement).

## Sounds

Inlet (volume flow's rate of change x 0.02, as the reference's air box; rush; whistle once a turn and blade passing
twelve a turn below 18 kHz, as phasors turned every 16 samples), vent (blow-off to air, purge), case (supercharger
whine from pockets: 6 a Roots rotor turn, 4 a twin-screw male turn, 31 a centrifugal's gear; harmonics as the
reference's, their fall-off from the pocket's pressure step), bang (anti-lag). Levels set beside the reference's.

Card N2's additions: a variable nozzle's back-pressure limit (`backPressure`, p3 over p2 absolute, 1.4: the vanes
opened by an integral past it) and its surge guard (the vanes opened at 1/s while the compressor surges with the pedal
down and the boost short, 0.3/s given back: gain-reversal protection, `docs/measurements/calibration/boost/causes.md`
5). The core in an engine test cell (MODE_DYNO_ENGINE, the absorber on, `Boost::testCell`) is held at the air's
temperature: SAE J1349's laboratory charge-air cooling, so the cooler gives its stated effectiveness; chassis dyno and
road: the car's air speed; standing: still air. `spec.boost.heatSettle` (a press counter) puts the core where the
present flow holds it.

### The reference's dials, and matching the charge system

Model 1 does **not** read the reference's `level` dial: the boost is the spring's, or the target's (the spec's or the
ECU's). `size` scales the matched machine. Every other field means what it says, in absolute units: a 15 L engine left
with F's defaults (a 70 kPa spring, a cooler rated 0.25 kg/s, 5 L of pipes: a 2 to 3 L engine's) makes little boost,
as such an engine would. `tel.boost.mismatch` says so, with each pair (`coolerHas`/`Wants`, `springHas`/`Wants`,
`volumeHas`/`Wants`); its thresholds are warnings, not physics (`sim/boost/boost.fm`, telemetry).

The rule for matching them, for the host to apply when the person asks (an action beside the warning, never a silent
default in apply):

- **Cooler rated flow** (`icFlow`): the engine's air at rated power. That is the brake power times the specific air
  consumption, the specific fuel consumption times the air-fuel ratio: a petrol engine at full load near 0.30 kg/kWh
  at lambda 0.85 (14.7 x 0.85 = 12.5) takes some 3.7 kg/kWh, 1.0 to 1.1 g/s a kW; a diesel at 0.21 kg/kWh and lambda
  1.6 (14.5 x 1.6 = 23.2) some 4.9 kg/kWh, 1.35 g/s a kW, to 1.7 with the leaner calibrations (fuel consumptions:
  Heywood, Internal Combustion Engine Fundamentals, ch. 2 and 15). Without a rated power: the machine's matched flow,
  `mMatch`, the four-stroke-equivalent displacement times the redline over 120 times 1.204 kg/m^3 times 1.9 times 0.9.
- **Charge pipes' volume** (`chargeVolume`): 2.5 displacements (F's defaults' ratio on 2 L), within 1 to 20 L.
  Judgement: no published rule; the volume sets the lag before the throttle and the surge's frequency, not the boost.
- **Gate spring** (`gateSpring`): the target with no electronic control; half the target under a duty (the ECU's
  solenoid bleeds the signal to hold up to twice the spring, the common "base boost" practice). Judgement.

P's archetypes are sized so by hand (10e71bd: 1.1 g/s a kW petrol, 1.7 diesel; pipes twice the swept volume).

## In the engine

`Boost` (`sim/boost/boost.fm`) holds the machine; `spec.boost.model` 1 switches to it (README, Boost), and card G's
exhaust network (implied by boost.model 1) ends its collectors in the housings (`housingOf`, `housing`, `downpipe`,
`gatePipe`). On the engine dyno at full load the 2 L
four's port gas reaches 1190 to 1330 K (`exh.tPort`, settled after some 4 s), and the matched turbo holds its 70 kPa
spring from 3500 rpm (EngineTest.cpp). Test at full load: the bench's `holdSpeed` driver holds a speed with almost no
brake torque, and its gas stays near 560 K (an earlier reading of this file's author, now corrected).

## Drawing the map elsewhere (the interface)

The map is a function of the compressor's sizes alone; the Scope carries it as the simulator computes it
(`scope.boostLine*`, `boostSurge*`, `boostContour*`, rebuilt when `boostMapRev` moves). To draw it without the Scope,
mirror `BoostCompressor` (`sim/boost/compressor.fm`): `size(dInd, dEx, ar, etaPeak)` (matched sizes as `BoostMachine::apply`
makes them when a size is 0), `speed(omega, 101325, 293.15)`, then `rise(phi)` (PR = (1 + tau)^3.5, flow = phi x mc)
and `efficiency(phi)` for phi from `phiK` to 1 on each line; `BoostMachine::mapLines` is the loop, with the lines' tip
Mach numbers 0.4 + 0.15 l and the contours' levels etaMax less 0.02, 0.05, 0.08, 0.12, 0.18.

## What the tests measured (standalone, `tee-tests-boost`)

See the test files' printed lines for the numbers; the handoff `docs/handoffs/f-boost.md` lists them. Not yet
measured: anything on the real engine (integration waits for K3b).
