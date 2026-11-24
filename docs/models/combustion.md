# Combustion and fuels (card H, `fuel.model 1`)

*What `sim/combustion/` models, with each equation, where each number comes from, and which numbers are judgement.
`fuel.model 0` (the default) is the reference's `charge()` and `chargeDiesel()` unchanged; everything here is read
only under `fuel.model 1`. Status: built, tested standalone (H1, H2, the parts) and wired into the simulator's `Combustion`
(`sim/combustion/combustion.fm`) behind `fuel.model`: see "Integrated" at the end.*

## Files

| File | What it holds |
|---|---|
| `fuels.fm` | the fuel table: a `const` row per fuel, and `fuelOctane`, `fuelHeatPerAir` |
| `mixture.fm` | `Mixture`: a charge's fuel, heat, unburned fuel, flame and cooling from its air, lambda and oxygen |
| `knock.fm` | `EndGas`: Livengood and Wu's integral over Douaud and Eyzat's delay, by tables; `knock_tables.py` writes them |
| `diesel.fm` | `DieselFields`: injection timing, pilot, rail pressure, smoke limit over the reference's diesel |
| `watch.fm` | `BurnWatch`, `LambdaSensor`, `InjectorMeter`: what a tuner reads |
| `charge1.fm` | `Charge1`: a gasoline charge from explicit numbers and the ECU's decision for the firing |
| `../spec/fuel.fm` | `spec.fuel`'s fields of model 1: `model`, `grade`, `direct`, `knockDelay`, `injector`, `soiAdd`, `pilot`, `rail`, `overFuel` |

## The fuel table

| Row | AFR | LHV MJ/kg | latent kJ/kg | RON | MON | flame | phi peak | lean, rich lambda | tested |
|---|---:|---:|---:|---:|---:|---:|---:|---|---|
| regular (E10, 87 AKI) | 14.0 | 42.2 | 402 | 91 | 83 | 1.00 | 1.12 | 1.45, 0.60 | yes |
| midgrade (E10, 89) | 14.0 | 42.2 | 402 | 93 | 85 | 1.00 | 1.12 | 1.45, 0.60 | yes |
| premium (E10, 91) | 14.0 | 42.2 | 402 | 96 | 86 | 1.00 | 1.12 | 1.45, 0.60 | yes |
| race (unleaded, E0) | 14.6 | 44.0 | 350 | 104 | 96 | 1.00 | 1.12 | 1.45, 0.60 | yes |
| E85 | 9.80 | 29.3 | 771 | 108 | 89 | 1.12 | 1.10 | 1.55, 0.55 | yes |
| methanol | 6.47 | 19.9 | 1100 | 109 | 89 | 1.25 | 1.11 | 1.65, 0.45 | yes |
| diesel | 14.5 | 42.5 | 250 | - | - | - | - | - | yes (as the reference's) |
| propane | 15.6 | 46.35 | 0 | 112 | 97 | 1.08 | 1.08 | 1.5, 0.6 | **no** |
| hydrogen | 34.3 | 120 | 0 | 130 | 130 | 6.0 | 1.8 | 4.0, 0.4 | **no** |

Sources and judgement:

- **Gasoline (E0)**: stoichiometric ratio 14.6 and lower heating value 44.0 MJ/kg, Heywood, *Internal Combustion Engine
  Fundamentals* (2nd ed.), appendix D. **Ethanol** 9.0 (from C2H5OH + 3 O2 and air's 23.2 % oxygen by mass) and 26.9
  MJ/kg; **methanol** 6.47 (CH3OH + 1.5 O2) and 19.9 MJ/kg; latent heats 840 (ethanol) and 1100 kJ/kg (methanol):
  the standard values Heywood's appendix lists. Gasoline's latent heat, 350 kJ/kg, is a middle value of the 305 to 400
  published (judgement). Diesel 14.5 and 42.5 MJ/kg are the reference's own; its latent heat 250 kJ/kg (Heywood).
- **E10 and E85** are mass-weighted mixtures: 10 % ethanol by volume is 10.6 % by mass, 85 % is 85.8 % (densities 0.789
  and 0.74 kg/L). Pump E85 is often 51 to 83 %: the row is E85 as its name says.
- **Octane numbers**: US pump grades' (R+M)/2 of 87, 89, 91 with typical E10 research and motor numbers (judgement
  within published ranges); E85 108/89 and methanol 109/89 are the commonly published ones; race gasoline 104/96 is a
  generic unleaded race fuel (judgement: brands differ); propane 112/97.
- **The octane index** the knock rule reads is Kalghatgi's `RON - K (RON - MON)` with K = 0 (judgement: modern engines
  at high load sit near 0 or below; the motor method's engines near 0.5).
- **Flame**: the laminar burning velocity's peak over gasoline's, and its equivalence ratio, from published laminar
  flame speed measurements at 1 atm and 300 K (gasoline and iso-octane some 35 cm/s near phi 1.1; ethanol some 40,
  methanol some 45, propane some 38; Metghalchi and Keck's correlation's phi of 1.13 for iso-octane, 1.11 for
  methanol, 1.08 for propane). Read as ratios, rounded: judgement. The curvature (how fast the speed falls either side,
  `S/S_peak = 1 - curv (phi - phi_peak)^2`) is Metghalchi and Keck's `B_phi / B_m` (iso-octane 3.2, methanol 3.8,
  propane 4.0).
- **Lean and rich limits** are an engine's misfire limits with a homogeneous charge (judgement after Heywood §9.4's
  lean limits near lambda 1.4 to 1.5 for gasoline; the alcohols reach a little further).
- **Propane and hydrogen** are rows for later, marked untested; hydrogen's numbers above (flame 6, a lean limit of
  lambda 4) are rough.

## The mixture (`Mixture.set`)

With `air` kg trapped, its oxygen `oxy` times air's (1; more with nitrous: card F's share), lambda asked `lambda`:

- fuel: `m_f = air oxy / (lambda AFR)`; the fuel the oxygen burns completely: `m_s = air oxy / AFR`
- heat into the gas: `Q = m_s LHV H(phi) c`, phi = 1 / lambda, where `H(phi) = phi` lean and, rich, `H = 1 + x - x^2 /
  (2 d)` with `x = phi - 1`, `d = phi_peak - 1`: slope 1 at phi 1, its top `1 + d/2` (1.06 for gasoline) at the fastest
  flame's phi, falling to 0 near the rich limit. This is judgement shaped after the fuel-air cycle's mean pressure,
  which peaks some 10 % rich (Heywood §5.5: more product molecules and less dissociation there). Tested: the heat's
  top falls between lambda 0.86 and 0.92 (H1).
- completeness near the limits: `c = 1 - 0.15` over the last 0.1 of lambda inside each limit (linear); misfire
  probability rising from 0 at each limit to 1 at 0.2 past it (judgement).
- unburned fuel (what the bangs burn): `m_f - m_s H c`.
- the burn's length: `burnK = (S(phi)/S_peak * flame)^-0.5` (judgement: an engine's turbulent flame depends on the
  laminar speed more weakly than linearly).
- evaporation: `dT_full = latent m_f / (air 1005 J/kg K)`; the share in the cylinder `0.3 + 0.55 direct` (port 0.3,
  direct 0.85; judgement); the cylinder's cooling capped at 80 K, the port's (half of what evaporates there, the rest
  from the walls; judgement) at 40 K (both judgement: where the charge would saturate).

Test H1: a kilogram of air at lambda 1 releases each row's `LHV / AFR`, within 2.9 to 3.1 MJ for every tested row
(regular 3.01, race 3.01, E85 2.99, methanol 3.08, diesel 2.93 MJ); hydrogen 3.50 against its own row.

## Knock (`EndGas`)

From the intake valve's closing to 98 % burned, every few samples:

    I += dt / tau,   tau = k_d 17.68 ms (ON/100)^3.402 (p / 1 atm)^-1.7 exp(3800 K / T_u),   T_u = T_0 (p / p_0)^0.25

Livengood and Wu (1955); tau is Douaud and Eyzat's fit (SAE 780080, 1978), from knocking engines on primary reference
fuels of octane 80 to 100 (above 100 it is extrapolated). `T_0`, `p_0`: the charge at the valve's closing less the
evaporation's cooling. The exponent 0.25 is gamma 4/3 for the unburned mixture (judgement). `k_d` is
`fuel.knockDelay`, the calibration (1: the published fit; 3 by default, below). `I` reaching 1 after the spark and before 98 % burned is a
knock of intensity `1 - x_b` (the unburned share that lights); before the spark, pre-ignition (intensity 1).

The tables (`knock_tables.py`): `exp(-3800/T)` every 5 K from 250 K, `p^1.7` every 0.5 atm to 256 atm, `r^0.25` every
0.25 to 128; read linearly. Measured (H2): within 1 % of the closed form over 5 to 150 atm and 550 to 1300 K.

Test H2 on a made-up trace (`tests/sim/combustion/trace.fm`: single zone, slider crank, Wiebe burn, the reference's
wall loss): more boost (1.3 atm at closing), a hotter charge (360 K), a lower octane (87) and a higher compression
(11) each lower the knock-limited advance; more advance brings the integral further, then the knock earlier. The known
operating point: Douaud and Eyzat's delay for PRF 90 at 40 atm and 800 K is 2.7 ms, within the 1 to 5 ms of shock-tube
delays near those conditions (Fieweger, Blumenthal and Adomeit, Combust. Flame 109, 1997: read from their figures, from
memory; not re-checked here). On the trace, a 10:1 engine at full load and 2000 rpm on regular (1 atm, 330 K at
closing, 50-degree burn) has its best torque at 24 degrees of advance and its knock limit at 8; at octane 108, 16. Real
engines of that compression on regular are knock-limited there by a few degrees, not 16: the single-zone trace (no gas
exchange, one gamma) runs its end gas hotter than an engine does, and Douaud and Eyzat's fit is known to need a
calibration per engine. `k_d` is set against the simulator's own cylinder at integration (test 8), not against this
trace.

Cost (H2's last test): one tick, 9.7 ns on this container (4 cores shared with other builds; the reference V8 measured
2722 ns a sample of 96 kHz in the same hour, `tee-bench --presets v8 --scenarios free-revs`). Ticking every 4 samples,
a V8's eight cylinders cost at most 8 x 9.7 / 4 = 19 ns a sample (0.7 % of R), and some 5 ns on average (each
cylinder integrates over some 170 of its 720 degrees). The card's budget is 0.06 R.

What knock leaves out: two-stage (cool-flame) ignition, the end gas's own heat loss, hot spots, residual gas's effect
on the delay, where the knock starts.

## The diesel (`DieselFields`)

The reference's `chargeDiesel` stays; under `fuel.model 1` four fields move its numbers, each identity at its default
(tested):

- `soiAdd`: degrees added to its start of injection.
- `pilot` (0 to 0.1 of the fuel): the premixed share `beta (1 - 0.7 min(pilot / 0.03, 1))` (judgement after common-rail
  heat-release curves, where a pilot all but removes the premixed spike).
- `rail` (bar): the diffusion burn's degrees times `(1000 / rail)^0.5` (mixing-controlled burn, the spray's speed as the
  pressure's square root; 1000 bar the reference's, judgement).
- `overFuel`: the smoke limit's equivalence ratio `0.75 (1 + overFuel)`; a smoke number `4 ((phi - 0.6) / 0.3)^2`
  (judgement, the shape of published filter smoke numbers against the mixture).

## What a tuner reads (`watch.fm`)

Peak pressure and its angle and the half-burn angle per cylinder per cycle (a compare a sample; published at the
cycle's end); a lambda sensor two crank revolutions behind the port with its own 80 ms (judgement: a wide-band sensor
answers in under 100 ms); injector duty `(m_f / injector + 0.5 ms) / cycle` and the fuel's flow smoothed over 0.2 s.

## Integrated

`Combustion::charge` takes the model-1 path (`charge1`) for a gasoline charge when `fuel.model` or `ecu.model` is 1;
`Combustion::sample` ticks each cylinder's end gas every 4 samples (fuel.model 1, from the cylinder's own pressure and
burned share, only while the engine runs: cranking's slow squeeze loses heat the isentropic end gas does not know). A
knock lights the rest of the charge: `knock` for the ping, a tenth of the remaining heat per unit scrubbed, the ECU's
retard; before the spark, pre-ignition (the burn starts there, 20 degrees long). Both models 0: the null test
(`tests/sim/NullTest.cpp`, 59 pairs) identical to the bit.

**The knock delay's calibration** (`fuel.knockDelay`, default 3): on the reference's V8 (10:1) through free revs with
ecu.model 1: `k_d` 1 knocks everywhere (691 knocks in 3.5 s on the limiter, the retard at its 15-degree most); 2
still 34 on regular; **3**: 14 knocks on regular and none on premium (no retard left standing); 4 and more, none on
regular. 3 puts the reference's V8 on regular at its knock edge at full load and premium clean, where a 10:1 engine
sits: judgement against that one engine.

Integrated tests (`tests/sim/combustion/IntegratedTest.cpp`; the V8 on the bench, its load rising with the speed's
square, so the settled speed stands for the torque at a fixed throttle of 0.35; race fuel, knock control off):

- **6**: spark advance 0 to 40 in 2.5-degree steps: one maximum, at 20 degrees (5572 rpm). There the half-burn angle is
  **0.3 degrees after the top**, not 6 to 12 (a `[!mayfail]` test records it): the cylinder (card D's, the
  reference's single zone with little wall loss) has its best torque with half burned at the top, as the made-up trace
  did. Engines show 6 to 12 because of the heat the walls take near the top; that is the cylinder's to model.
- **7**: lambda 0.70 to 1.20: torque peaks at **0.90** (inside 0.85 to 0.92). The exhaust temperature peaks at 0.90
  too, not 1.0 to 1.05 (`[!mayfail]`): the heat's share peaks rich and the cylinder's gas has air's properties
  whatever burned in it.
- **8**: at 0.6 throttle with the spark held flat, regular starts knocking at 27.5 degrees, E85 not by 40 (model 1's
  most); a supercharged V8 in neutral on the limiter at a flat 22 degrees knocks at a boost level of 0.1 (0.32 bar) on
  regular and 0.9 (0.92 bar) on methanol.
