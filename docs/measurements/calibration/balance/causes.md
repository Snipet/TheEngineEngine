# The over-delivering NA engines: the air and energy balance (card N3)

*Card N3, 2026-11-24, from 19:30 UTC. Measured with the balance probe (`tests/sim/calibration/balance`, hidden tag
`[.balance]`; `scripts/balance.py` writes [balance.md](balance.md)): an archetype started as tee-dyno starts it, full
throttle on the engine dyno, warm, the exhaust's and the intake's heat settled twice in a 2 s hold as `tee-dyno --steps`
settles them, then 1 s measured at the engine's own 96 kHz (one engine step a call: at 48 kHz the resampler runs the
engine in blocks of 16 and the cylinders' events cannot be read). Every cylinder's state is read each sample. The first
law closes within 0.3 % of the fuel's heat at every point (released - walls - p dV - the net enthalpy through the valves
with u(T) and h(T) integrated over the cylinder's own gamma(T) - the gas's change). Figures from 4bf0fe0 unless said.
"Measured" means run here; "proposed" means not committed.*

## N3-0. The air: no mass is made; the two VEs differ by definition (measured)

The seven, each at its target's peak-torque and peak-power speed: through the throttle, in through the intake valves net,
trapped fresh at intake closing, out through the exhaust valves net, and the air the fuel was metered for agree within
0.3 % (balance.md). The flat six at 5199 rpm, g a cycle a litre: throttle 1.1495, valves 1.1496, trapped fresh 1.1498
(and 0.041 burned, 3.5 %), exhaust 1.1495, metered 1.1498. `charge1` meters the fuel from the trapped fresh air itself:
the ECU is perfect, so lambda is exact.

The lead read VE 0.970 (the ledger's computed manifold line) against 1.046 (`tel.engine.veManifold`). Read in
`core/work.fm:170` and `engine.fm:1288`: the telemetry divides the trapped mass by the density at `intk.pPlenum` and
`intk.tAir`, and `intk.tAir` is the charge **after** the intake's heating (card E's model 1, N1's F): 288 K outside, +5
for the warm engine bay, +15 of heating, 308 K. The ledger's line takes the air's temperature. 308 / 288 = 1.07 is the
gap. Both are right for what they say; the BMEP adds up with veAir (0.938) and the fuel's real heat (N3-1, N3-2).

## N3-1. Gross indicated efficiency 5 to 10 % high: the cylinder's gamma(T) (measured; D's file, proposed to D)

Every one of the sixteen NA petrol archetypes converts 0.374 to 0.433 of its fuel's heat into gross indicated work at
full load (λ 0.88 to 0.91); at λ 1.0 (a scratch tree with the map's enrichment off) the flat six 0.428 and the small
block 0.401. A real spark-ignition engine at full load does some 0.8 of the fuel-air cycle (Heywood, *Internal
Combustion Engine Fundamentals*, §5.7 and ch. 15), 0.33 to 0.38; its brake efficiency at full load 0.28 to 0.33
(BSFC 260 to 300 g/kWh). The walls take 0.12 to 0.16 of the fuel's heat (the burn 0.07 to 0.09, the gas exchange 0.04
to 0.06), the low end of the in-cylinder share but not far out; the exhaust's sensible enthalpy 0.36 to 0.41, inside
Heywood's table 12.1 (34 to 45 % at maximum power).

`Cylinder::gammaAt` is 1.4125 - 7.5e-5 T (clamped 1.24 to 1.40). The single-zone fit for an engine's cylinder gas
(mixture, residual and equilibrium products with their dissociation) is Gatowski et al., *Heat Release Analysis of
Engine Pressure Data*, SAE 841359 (SAE Trans. 93, 1984, 961-977): **1.392 - 8.13e-5 T** (as used in e.g. Energies 14
(2021) 3117). Ours runs 0.025 to 0.04 higher from 500 to 2500 K (1500 K: 1.300 against 1.270), so the expansion
returns too much work.

The experiment (scratch tree, that one line, clamped 1.15 to 1.40), all sixteen at both speeds: torque -4.8 to
-8.6 %; gross indicated 0.352 to 0.409; brake 0.283 to 0.340; walls -0.01; exhaust +0.01. The over-delivering seven:
flat six 417 → 392 N m (4750 rpm; target 300 to 320), VR6 284 → 267 (230 to 240), roadster 183 → 172 (155 to 172),
small block 486 → 455 (380 to 470), 3.8 V6 366 → 342 (300 to 330), 7.0 V8 748 → 700 (630 to 645), four-cam 594 → 560
(520 to 575). **It is a level**, as N1's A and F were: it also takes the economy four (146 → 137, target 140 to 156) and
the big block (635 → 595, 600 to 690) just under, and the short tuned engines further short (V12 669 → 629, V10 501 →
465): their shortfall is breathing (N1's B). Sensitivities beside it (λ 1, not proposed): gamma 0.02 lower, -3.3 %;
Woschni's combustion term doubled (walls 0.14 → 0.18 of the fuel), -3.3 %.

**The mixture's dependence (the lead's condition, 19:56).** Gatowski's fit is for a charge near stoichiometric; a
diesel burns at φ 0.65 to 0.75. Ceviz and Kaymaz (Energy Conversion and Management 46, 2005) give the direction only
(γ rises as the mixture leans); their coefficients were not reachable here. Sized from first principles instead:
frozen products of CH1.8 with JANAF heat capacities (N2, O2, CO2, H2O; recalled, 1 to 2 %), γ at φ 0.7 against 1.0 is
1.304 / 1.293 at 1000 K, 1.277 / 1.266 at 1500, 1.264 / 1.253 at 2000, 1.257 / 1.246 at 2500: lean 0.011 higher at
every temperature. Gatowski's matches the stoichiometric products at 1500 K (1.270) and falls below them hotter, for
the dissociation. The present fit (1.300 at 1500 K) is above even φ 0.7's products below 2000 K, so it is not "the lean
gas's". Proposed to D: γ = 1.392 - 8.13e-5 T + 0.037 (1 - min(φ, 1)). The three diesels' gross IMEP, bar, at their
target peak-torque and peak-power speeds (4.5 s settle), present → Gatowski → with the lean term: diesel 2.0 22.22 →
21.45 → 21.94 and 23.75 → 22.43 → 22.91; pickup 6.7 21.23 → 20.88 → 20.99 and 21.46 → 20.79 → 21.19; tractor 21.58 →
20.91 → 21.29 and 21.83 → 21.10 → 21.36: -1.1 to -3.5 %, mean -2 %.

**Best torque against lambda, with it (the lead's check).** A flat lambda map (`ecu.maps.lambdaAuto` 0, one point),
full load, torque N m:

| λ | 0.78 | 0.82 | 0.86 | 0.90 | 0.94 | 0.98 | 1.02 | 1.10 |
|---|---|---|---|---|---|---|---|---|
| Flat six 4750 rpm, present γ | 374.6 | 401.6 | 415.1 | **417.6** | 413.1 | 401.4 | 387.6 | 358.8 |
| Flat six, Gatowski | 354.0 | 378.8 | 390.9 | **392.5** | 388.3 | 378.6 | 365.2 | 340.0 |
| Turbo four 2.0, 3050 rpm, present | 341.4 | 366.0 | 380.7 | **380.8** | 378.8 | 362.8 | 352.6 | 323.7 |
| Turbo four, Gatowski | 331.4 | 356.6 | 362.2 | **366.3** | 359.3 | 353.5 | 341.7 | 311.6 |

Best torque stays at λ 0.90, some 5 to 7 % over λ 1.0, as engines show (best power a little rich). Gross indicated
efficiency on the flat six with Gatowski: 0.383 at λ 0.90, 0.407 at 1.02.

Test: `calibration-balance.sim` holds gross efficiency 0.33 to 0.38 on the small block at 2000, 3000 and 4600 rpm,
skipped on this finding until D's change lands.

## N3-2. The rich mixture's heat (measured; H's file, held, not changed)

`Mixture::heatShare` gives a rich charge more heat than a stoichiometric one: a parabola to 1.06 of the stoichiometric
heat at φ 1.12. Chemistry gives less: with the water-gas equilibrium (K 3.5, Heywood §4.9) a CH1.87 fuel at φ 1.104
leaves 21 % of its carbon as CO and 0.066 mol H2 a mol of fuel, releasing 0.876 of its LHV, 0.967 of the
stoichiometric charge's heat (φ 1.05: 0.984; 1.2: 0.938; about 1 - 0.32 (φ - 1)). The probe's released share at λ 0.90
is 0.92, where chemistry and the burn's 0.97 give 0.85. A scratch tree with 1 - 0.32 (φ - 1): released 0.924 → 0.845,
torque -8.3 % (flat six), -8.8 % (small block).

**Settled for now (20:04 by date -u, after N3-1's sweep above):** with Gatowski's γ the rich side still puts best torque at λ 0.90,
5 to 7 % over stoichiometric; the parabola's gain is not visibly too generous against that shape, and without the
fuel-air cycle's figure open there is no sourced curve to replace it with. It stays.

Why it was held: real engines do make their best torque a little rich (H's integrated test 7, λ 0.85 to 0.92), because
of what the cylinder's gas lacks: more product molecules and less dissociation (Heywood §5.5, the fuel-air cycle). The
parabola stands in for those. Gatowski's gamma (N3-1) carries the dissociation's average, so after N3-1 the parabola
over-credits by perhaps the difference between its 1.06 and the fuel-air cycle's rich gain; I have not had that figure
open, so no change. Open: compare the fuel-air cycle's mean pressure against φ (Heywood fig. 5-9) with the model's work
per kg of air against λ, once N3-1 is in.

## N3-3. The flat six's breathing (measured; content, to P with N1)

After N3-1 the flat six is still 392 N m against 300 to 320 (+26 %): its VE is 1.003 at 4750 rpm (49 mm intake valve on a
100 mm bore, a 0.53 m runner tuned near there). Candidates, not isolated: an air-cooled head's walls run hotter than the
368 K coolant the intake's charge heating assumes (more heating, less charge); the cooling fan's power, which a DIN
figure includes. Its peak's target speed and figure (964-type, 184 kW, 310 N m DIN) look right.

## Class ranges for the ledger (proposed to N1)

BMEP at peak torque, from the archetypes' own targets (makers' published figures, P's table, as 4 π T / V):

| Head class | Archetypes | Their targets, bar | Proposed range |
|---|---|---|---|
| Two-valve pushrod | small block, 3.8 V6, 7.0 V8, 7.4 big block | 8.3 to 11.7 | 8.5 to 11.8 |
| Two-valve overhead cam | flat six, VR6 | 10.4 to 11.2 | 10.0 to 11.5 |
| Four-valve | economy four, hot hatch, roadster, four-cam V8, 3.5 V6 | 10.6 to 14.3 | 10.5 to 14.3 |
| Tuned four-valve (redline 8000 rpm and up) | 3.2 six, flat-plane V8, two-stage 2.0, V10, V12 | 12.6 to 15.6 | 12.5 to 15.6 |

Volumetric efficiency against the outside air at peak torque (Heywood §6.2, figs. 6-9 and 6-10; Blair ch. 6), checked
against the BMEPs above at brake efficiency 0.30 and λ 0.88: two-valve pushrod 0.75 to 0.90; two-valve overhead cam 0.82
to 0.92; four-valve 0.85 to 1.00; tuned four-valve 0.95 to 1.10. Brake efficiency at full load, every class: 0.27 to 0.33
(BSFC 260 to 300 g/kWh at full load, rich; Heywood §2.10). Gross indicated efficiency at full load: 0.33 to 0.38.
Against these the seven's BMEP at their target peak-torque speeds: flat six 14.6, VR6 12.8, 3.8 V6 12.1, 7.0 V8 13.4,
four-cam 14.8 bar, all out; the roadster's 12.5 and the small block's 10.7 inside their class (each is over its own
target, which the ledger's per-archetype line already holds). Gross indicated efficiency would have flagged all
sixteen.
