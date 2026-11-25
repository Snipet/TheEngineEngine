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

Test: `calibration-balance.sim` holds gross efficiency 0.33 to 0.38 on the small block at 2000, 3000 and 4600 rpm.


**Landed (D's 3990ee47 and a845b07f; measured on the tip by 20:16 by `date -u`, tee::Build path).** Torque, N m, at each
archetype's target peak-torque and peak-power speeds, before (4bf0fe0) and after; gross indicated efficiency beside.
Other commits landed between the two runs (N1's, G's); the probe's scratch run with the γ line alone gave the same
within 0.3 %, so the change is γ's.

| Engine | rpm | Torque before → after | Gross before → after |
|---|---|---|---|
| air-cooled-flat-six-36 | 4750 | 417.0 → 392.6 (-5.9 %) | 0.408 → 0.385 |
| air-cooled-flat-six-36 | 6050 | 381.6 → 354.2 (-7.2 %) | 0.412 → 0.386 |
| vr6-28 | 4200 | 284.4 → 266.6 (-6.3 %) | 0.395 → 0.372 |
| vr6-28 | 5800 | 219.5 → 202.5 (-7.7 %) | 0.399 → 0.375 |
| twin-cam-roadster-18 | 5000 | 183.1 → 171.6 (-6.3 %) | 0.385 → 0.361 |
| twin-cam-roadster-18 | 6650 | 180.2 → 167.1 (-7.3 %) | 0.399 → 0.374 |
| small-block-57-carb | 3000 | 486.1 → 455.6 (-6.3 %) | 0.374 → 0.352 |
| small-block-57-carb | 4600 | 462.1 → 429.4 (-7.1 %) | 0.383 → 0.358 |
| pushrod-v6-38 | 4000 | 365.5 → 341.2 (-6.6 %) | 0.388 → 0.364 |
| pushrod-v6-38 | 5200 | 251.0 → 231.1 (-7.9 %) | 0.389 → 0.366 |
| big-bore-70-pushrod-v8 | 4800 | 748.0 → 699.5 (-6.5 %) | 0.407 → 0.383 |
| big-bore-70-pushrod-v8 | 6300 | 605.1 → 558.8 (-7.7 %) | 0.412 → 0.387 |
| four-cam-v8-50 | 4500 | 594.4 → 560.0 (-5.8 %) | 0.414 → 0.390 |
| four-cam-v8-50 | 6850 | 485.1 → 452.5 (-6.7 %) | 0.422 → 0.397 |
| economy-four-15 | 4400 | 146.1 → 136.9 (-6.3 %) | 0.393 → 0.370 |
| economy-four-15 | 6300 | 142.5 → 133.7 (-6.2 %) | 0.406 → 0.382 |
| hot-hatch-16-16v | 5250 | 154.5 → 145.3 (-6.0 %) | 0.389 → 0.366 |
| hot-hatch-16-16v | 6850 | 173.2 → 158.3 (-8.6 %) | 0.402 → 0.377 |
| straight-six-32-high-rev | 4900 | 375.1 → 351.4 (-6.3 %) | 0.408 → 0.384 |
| straight-six-32-high-rev | 7800 | 289.5 → 270.4 (-6.6 %) | 0.416 → 0.390 |
| flat-plane-v8-45 | 6000 | 392.9 → 365.7 (-6.9 %) | 0.418 → 0.395 |
| flat-plane-v8-45 | 8500 | 407.1 → 375.0 (-7.9 %) | 0.425 → 0.401 |
| two-stage-cam-20 | 7500 | 143.1 → 136.3 (-4.8 %) | 0.398 → 0.375 |
| two-stage-cam-20 | 8300 | 161.5 → 151.2 (-6.4 %) | 0.397 → 0.373 |
| v10-52 | 6500 | 495.1 → 465.8 (-5.9 %) | 0.420 → 0.395 |
| v10-52 | 8350 | 501.5 → 465.1 (-7.3 %) | 0.430 → 0.404 |
| v12-65 | 6500 | 668.8 → 635.1 (-5.0 %) | 0.425 → 0.402 |
| v12-65 | 8575 | 666.9 → 628.9 (-5.7 %) | 0.433 → 0.409 |
| v6-35-na | 4750 | 310.2 → 291.9 (-5.9 %) | 0.401 → 0.378 |
| v6-35-na | 6350 | 319.3 → 298.8 (-6.4 %) | 0.403 → 0.379 |
| big-block-74-pushrod-v8 | 3500 | 634.7 → 593.6 (-6.5 %) | 0.392 → 0.368 |
| big-block-74-pushrod-v8 | 5000 | 570.3 → 527.2 (-7.6 %) | 0.399 → 0.374 |

Diesels, gross IMEP, bar (4.5 s settle): diesel 2.0 2350 rpm 22.22 → 22.44, 3850 23.75 → 22.83; pickup 6.7 1700 21.23 →
21.16, 2887 21.46 → 21.28; tractor 1100 21.58 → 21.40, 1800 21.83 → 21.70: -3.9 to +1.0 %.

`calibration-balance.sim`'s efficiency case now passes unskipped (small block 0.345, 0.352, 0.358) and holds it.

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

## N3-4. The two-stage cam's switch: it switches, and the high cam loses (measured; content or N1's B, routed)

The lead's row (the switch not showing in the curve). Present tip, present γ, tee::Build path, full load:

| rpm | 4000 | 5500 | 6500 | 7500 | 8500 |
|---|---|---|---|---|---|
| Torque, as built (stage B from 6000 rpm), N m | 202.8 | 193.0 | 141.7 | 136.3 | 155.5 |
| VE | 0.907 | 0.885 | 0.714 | 0.718 | 0.831 |
| Burned / fresh trapped | 0.027 | 0.024 | 0.078 | 0.078 | 0.053 |
| Torque, stage A held (`valve.stages` 0) | 202.8 | 192.9 | 202.6 | 190.3 | 191.3 |
| VE | 0.907 | 0.885 | 0.945 | 0.924 | 0.968 |

The stage does switch (`vt.lab.stageWant` 1 above `stageSpeed`, 6000 rpm); no float (`vt.lab.floating` 0; float
speeds 12 100 and 12 800 rpm). Stage B (290 / 284 degrees at the seat, 258 / 252 at 1 mm, 11.8 mm, centres 110 / 112)
loses a quarter of the low cam's filling at 6500 and 14 % at 8500, where a real engine's high cam gains. The residual
only triples, so the loss is mostly the late closing's push-back (the events the telemetry reports: intake 53 before
the top to 93 after the bottom) with nothing in the pipes to hold the charge in. Candidates, not isolated: the
exhaust's back pressure through the overlap (N1's B: 154 kPa mean at the port on the 3.2 six); the stage B cam larger
than the real head's (P to check against the source engine). Routed to N1 and P.

## N3-5. The 3.5 V6 peaks at 2778 rpm (measured; content, routed to P and N1)

| rpm | 2000 | 2778 | 3500 | 4200 | 4750 | 5500 | 6350 |
|---|---|---|---|---|---|---|---|
| Torque, N m | 335.9 | 338.5 | 310.8 | 310.7 | 291.9 | 285.8 | 298.8 |
| VE | 0.891 | 0.917 | 0.833 | 0.837 | 0.805 | 0.811 | 0.869 |
| Intake phase, deg | 40 | 34 | 24 | 15 | 7 | 3 | 3 |

The 0.50 m runner tunes near 2800 rpm; the switch to 0.30 m at 4000 rpm (`intake.runnerSwitch` 419 rad/s) does not
lift 4200 to 5500, and only 6350 recovers. The target's peak torque is 335 to 365 N m near 4750. The runners' lengths
and switch speed are the build's (P); the phaser's schedule is N1's rule (its advance falls to 3 degrees by 5500).

## Since N3-1 (20:24 by `date -u`, tee::Build path)

- **P's trims (2a4fb34).** Roadster (31 / 26 mm valves, 236 / 232 degree cams): 161 / 176 / 155 N m at 3000 / 5000 / 6650
  rpm, target 155 to 172. Small block (the L48's cam): 482 / 478 / 424 N m at 2000 / 3000 / 4600 rpm, VE 0.82 to 0.83. Its
  target is now the L48 and L82's SAE net ratings, 340 to 385 N m (P, from looked-up figures): **25 % over again**.
- **The two-stage cam's stage B as the K20A's** (280 / 278 at the seat, 12.65 / 12.14 mm, switching at 6000 rpm: Honda's
  factbook for the switch): B still loses. VE with A held 0.946 / 0.925 / 0.971 against 0.773 / 0.759 / 0.881 with B at
  6500 / 7500 / 8500 rpm; torque 206 / 193 / 195 against 160 / 149 / 171. N3-4 is the model's (N1's path).
- **The 7.0's ports at 1.6 / 1.8** (P, from discharge.md's LS7 row, which is one posting): 723 and 630 N m at 4800 and
  6300 rpm against 700 and 560 at 1.35 / 1.5 (target 630 to 645). Asked P to put the defaults back until a second
  figure agrees (the lead's rule).

## N3-6. The runner's ram: its height measured, no missing loss found yet (20:31 by `date -u`; the lead's item 1)

The flat six at full load, VE against the outside air, a 0.53 m runner (as built) against 0.30 m, every 250 rpm (tip,
tee::Build path):

| rpm | 0.30 m | 0.53 m | Ratio |
|---|---|---|---|
| 2500 | 0.829 | 0.824 | 0.994 |
| 2750 | 0.837 | 0.839 | 1.003 |
| 3000 | 0.833 | 0.824 | 0.988 |
| 3250 | 0.835 | 0.817 | 0.979 |
| 3500 | 0.849 | 0.854 | 1.006 |
| 3750 | 0.856 | 0.893 | 1.043 |
| 4000 | 0.864 | 0.882 | 1.022 |
| 4250 | 0.853 | 0.888 | 1.041 |
| 4500 | 0.876 | 0.979 | 1.117 |
| 4750 | 0.857 | 1.009 | 1.177 |
| 5000 | 0.849 | 0.953 | 1.123 |
| 5250 | 0.846 | 0.944 | 1.115 |
| 5500 | 0.846 | 0.957 | 1.132 |
| 5750 | 0.846 | 0.978 | 1.156 |
| 6000 | 0.851 | 0.976 | 1.148 |
| 6250 | 0.852 | 0.897 | 1.053 |
| 6500 | 0.854 | 0.836 | 0.978 |

The 0.30 m runner is flat (0.83 to 0.88); the 0.53 m one rises from 4500 rpm to 1.009 at 4750 and holds 0.94 to 0.98 to
6000: a gain of 12 to 18 % over a broad band.

Its ringing, read sample by sample at the valve end of cylinder 1's runner with the valve shut (4750 rpm): about 161 Hz
(a quarter wave of 0.53 m at 350 m/s with its end correction), swings of -24.2, +25.3, -16.3, +22.5, -16.2 kPa about a
mean of -5.8 kPa: some 10 % lost in the first period, almost none in the second. Lightly damped, but of the size a
smooth straight duct's own losses give (by hand: Kirchhoff's wall loss about 1.3 % a period at 161 Hz in a 63 mm bore;
the turbulent friction on the oscillation's some 48 m/s about 3 %; the jet's head at the plenum end, K 1 out, some 7 %
a period): the losses the lead listed (the walls' viscous and thermal layer, the flow's friction, the end's jet) are all
in, and of their textbook size.

Experiments at 3500 / 4750 / 6000 rpm (0.53 m; VE):
- the plenum end's inward K 0.2 → 0.5 (a square edge, Idelchik): 0.858 / 0.995 / 0.953 against 0.854 / 1.009 / 0.976:
  1.4 % off the peak;
- the valve's wave stiffening off for the intake (g 1, a scratch Valve): 0.863 / 1.015 / 0.974: none;
- the runner's bore 63 → 50 → 45 mm: VE at 4750 1.008 → 1.036 → 1.050: a narrower runner rams harder.

Published gains found so far: a petrol engine's VE up to 8.7 % higher with a 528 mm runner, a turbocharged diesel's
6 % (single studies on ResearchGate, geometry not compared); Ohata and Ishida (SAE 820407) and Heywood's figures not
reachable here. Not enough to call 12 to 18 % wrong against a known geometry, and no sourced loss is missing from the
duct: nothing changed. What would settle it: a measured intake-port pressure trace after closing (its decay a period)
or a measured VE against runner length for a known runner, matched in a build.

## N3-7. The small block against its SAE-net band (20:33 by `date -u`; the lead's item 3)

Target 340 to 385 N m (the L48 and L82's SAE net ratings, P). On the tip, full load, tee::Build path:

| | 2000 rpm | 3600 rpm |
|---|---|---|
| Torque, N m | 481.8 | 466.9 |
| VE (outside air) | 0.821 | 0.827 |
| Lambda | 0.887 | 0.894 |
| Spark, deg before top | 13.5 | 18.2 |
| Released, of the fuel's heat | 0.909 | 0.918 |
| Walls | 0.167 | 0.140 |
| Gross indicated | 0.348 | 0.359 |
| Pumping | 0.006 | 0.019 |
| Friction (FMEP 1.11 and 1.38 bar) | 0.033 | 0.041 |
| Brake | 0.310 | 0.300 |
| Exhaust, sensible | 0.399 | 0.437 |

The air is about right: at brake efficiency 0.26 and this lambda, 385 N m wants VE 0.78. The excess is the efficiency
(0.31 against some 0.25 to 0.28: a carburetted engine of the 1970s at full load, BSFC 300 to 335 g/kWh).

What the build is not (measured one at a time, 2000 / 3600 rpm, torque):
- compression 9.0 → 8.5 (the L48's, as recalled: P to source): -1.2 / -2.1 %;
- dual exhaust → single (`exhaust.sides` 1): -3.0 / -4.3 %;
- the ECU's best-torque spark 6 degrees later (`ecu.timing` -6, a distributor's curve held back for emissions: P to
  source): -1.3 / -1.8 %.
Together some -6 to -8 %: to P as parts, each with its source to find.

What may be the model's (not measured, each to isolate before changing):
- the accessories: friction.fm counts the alternator (and an air conditioner) only; an SAE-net rating of the 1970s took
  the water pump, the fan and the air pump off the crank too (a few kW at speed; the rating's own rules to be read);
- the burn: one base angle (44 degrees at 3000 rpm, combustion.fm) for every head; an open-chamber two-valve wedge
  burns slower than a modern four-valve pent roof (Heywood ch. 9), which costs a few per cent at the best spark;
- friction's level: 1.11 bar at 2000 rpm is at the low end of Heywood's eq. 13.15 band (friction with pumping 1.17
  against 1.47 ± 25 %).

**What an SAE net rating carries** (enthusiast write-ups of J1349; the standard itself not opened): the engine as
installed, with its alternator and water pump, the factory's ignition timing and carburettor (and air cleaner), and the
whole exhaust with its manifolds and mufflers (a converter from 1975). The fan is not confirmed either way. So the
build-side list above is the larger share: the exhaust as fitted, the factory's timing, the air cleaner's and the
carburettor's restriction (not yet measured: the build has a single throttle and an air box). The model's accessories
(the water pump is in friction.fm's constant) are a smaller question than first thought.

N1's suggestion (the ram adding to the exhaust's tuning), tested: with `exhaust.model` 0 the flat six reads the same to
the bit at both runner lengths (4750 and 5500 rpm), and the burned share trapped is the same at 0.30 and 0.53 m (0.030
and 0.030 at 4750). The 0.53 m runner's gain is the intake's own ram, not better scavenging.

**On P's 9c71eb1** (the L48's 8.5:1, one source; its installed exhaust, both banks into one converter, two sources; 20:40 by
`date -u`): 460 / 457 / 446 / 436 / 410 N m at 2000 / 2400 / 3000 / 3600 / 4400 rpm, VE 0.80, gross 0.342 to 0.353,
brake 0.301 to 0.274: still some 20 % over 340 to 385. Candidates sized since (scratch trees; none isolated as the
cause):
- the chamber's burn 30 % longer at the best spark (`combustion.fm` burnBase x 1.3): 455 and 432 N m at 2000 and 3600
  rpm (-1.1 %);
- an engine-driven fan: the figures found are forum postings (a clutched V8 fan about 10 hp at speed; big fans 10 to
  15 hp on a stationary dyno) and a patent for the cube law in the fan's speed; 10 hp at 4000 rpm would be about 1 % at
  2000 rpm and 3 % at 3600. A build property worth adding (the lead's design: era defaults by P, model-on), not the 20 %;
- the factory spark: no source (P).
Unmeasured: a carburettor's wet manifold's spread of mixture between cylinders, and an older engine's higher friction.

**Open, the integration queue's way:**
- *N3-6, the runner's ram (open).* What would settle it: a measured intake-port pressure trace after closing, or a
  measured VE against runner length for a known runner, built and compared. Until then the model's 12 to 18 % stands.
- *The flat six's intake (content, P).* The 964's induction is a resonance system with a flap between two plenum
  halves; the build's single 0.53 m runner is P's rule, not the engine's. Asked P to mark it as judgement and to build
  the real geometry if a source turns up.

## The sixteen on the tip's gas (20:43 by `date -u`; the lead's table)

Each NA petrol archetype's torque at its target's peak-torque speed over the torque band's middle, and its power at the
target's peak-power speed over the power band's middle (settled dyno, tee::Build path), with P's basis for the rows:

```
torque: mean -1.7 %, sd 16.2 %, median -3.2 %
power: mean -0.2 %, sd 14.1 %, median -1.0 %

sorted by torque over the band's middle:
  air-cooled-flat-six-36     cited      +27.7 %   (torque 396 for [300, 320], power 226 kW for [180, 195])
  small-block-57-carb        cited      +23.1 %   (torque 446 for [340, 385], power 192 kW for [120, 160])
  vr6-28                     cited      +14.3 %   (torque 269 for [230, 240], power 124 kW for [125, 132])
  big-bore-70-pushrod-v8     cited       +9.9 %   (torque 700 for [630, 645], power 369 kW for [370, 385])
  pushrod-v6-38              cited       +8.8 %   (torque 343 for [300, 330], power 127 kW for [140, 160])
  twin-cam-roadster-18       cited       +7.3 %   (torque 176 for [155, 172], power 108 kW for [95, 112])
  four-cam-v8-50             cited       +2.4 %   (torque 560 for [520, 575], power 325 kW for [300, 345])
  straight-six-32-high-rev   recalled    -2.5 %   (torque 356 for [355, 375], power 223 kW for [245, 260])
  hot-hatch-16-16v           recalled    -3.8 %   (torque 147 for [140, 165], power 114 kW for [85, 120])
  economy-four-15            recalled    -7.2 %   (torque 137 for [140, 156], power 89 kW for [80, 97])
  big-block-74-pushrod-v8    recalled    -7.6 %   (torque 596 for [600, 690], power 277 kW for [240, 310])
  v12-65                     recalled    -9.4 %   (torque 639 for [680, 730], power 567 kW for [500, 590])
  v10-52                     recalled   -14.6 %   (torque 466 for [520, 570], power 408 kW for [385, 475])
  v6-35-na                   recalled   -16.1 %   (torque 294 for [335, 365], power 200 kW for [195, 225])
  two-stage-cam-20           recalled   -28.3 %   (torque 149 for [200, 215], power 143 kW for [165, 185])
  flat-plane-v8-45           recalled   -31.9 %   (torque 368 for [520, 560], power 335 kW for [400, 445])

sorted by power over the band's middle:
  small-block-57-carb        cited      +36.9 %   (torque 446 for [340, 385], power 192 kW for [120, 160])
  air-cooled-flat-six-36     cited      +20.7 %   (torque 396 for [300, 320], power 226 kW for [180, 195])
  hot-hatch-16-16v           recalled   +11.5 %   (torque 147 for [140, 165], power 114 kW for [85, 120])
  twin-cam-roadster-18       cited       +4.2 %   (torque 176 for [155, 172], power 108 kW for [95, 112])
  v12-65                     recalled    +4.0 %   (torque 639 for [680, 730], power 567 kW for [500, 590])
  big-block-74-pushrod-v8    recalled    +0.8 %   (torque 596 for [600, 690], power 277 kW for [240, 310])
  four-cam-v8-50             cited       +0.8 %   (torque 560 for [520, 575], power 325 kW for [300, 345])
  economy-four-15            recalled    +0.2 %   (torque 137 for [140, 156], power 89 kW for [80, 97])
  big-bore-70-pushrod-v8     cited       -2.2 %   (torque 700 for [630, 645], power 369 kW for [370, 385])
  vr6-28                     cited       -3.6 %   (torque 269 for [230, 240], power 124 kW for [125, 132])
  v6-35-na                   recalled    -4.9 %   (torque 294 for [335, 365], power 200 kW for [195, 225])
  v10-52                     recalled    -5.1 %   (torque 466 for [520, 570], power 408 kW for [385, 475])
  straight-six-32-high-rev   recalled   -11.9 %   (torque 356 for [355, 375], power 223 kW for [245, 260])
  pushrod-v6-38              cited      -15.6 %   (torque 343 for [300, 330], power 127 kW for [140, 160])
  two-stage-cam-20           recalled   -18.1 %   (torque 149 for [200, 215], power 143 kW for [165, 185])
  flat-plane-v8-45           recalled   -20.7 %   (torque 368 for [520, 560], power 335 kW for [400, 445])
```

The mean is near nothing and the spread wide: no common level is left in the model. What remains is each build's
content and the breathing, and the class pattern still shows (the cited mild two-valve engines over, the recalled tuned
four-valve ones under).

**The small block, stopped (the lead's call).** Open: some 14 % unexplained after P's L48 parts. Sized: accessories 1 to
3 % (forum figures only: no change to friction.fm until a better source), the chamber's burn about 1 %, the spark about
1.5 % (no source); unmeasured: the carburettor's spread of mixture between cylinders, an older engine's friction.

## N3-8. The runners are tuned to the targets' own peak speeds (20:45 by `date -u`)

The 7.0 (cited +9.9 %) across its range, full load (tip, tee::Build path):

| rpm | 2000 | 3000 | 3600 | 4200 | 4800 | 5400 | 6000 | 6300 | 6800 |
|---|---|---|---|---|---|---|---|---|---|
| Torque, N m | 564 | 573 | 659 | 642 | 701 | 656 | 628 | 560 | 510 |
| Power, kW | 118 | 180 | 249 | 282 | 352 | 371 | 395 | 369 | 363 |
| VE | 0.748 | 0.758 | 0.871 | 0.869 | 0.955 | 0.933 | 0.940 | 0.855 | 0.808 |
| Brake efficiency | 0.325 | 0.328 | 0.329 | 0.322 | 0.322 | 0.310 | 0.297 | 0.289 | 0.279 |

The torque comes in teeth (VE 0.76 → 0.87 → 0.955 → 0.85), its top at 4800 rpm: the target's peak-torque speed. Its
power peaks at 395 kW at 6000 rpm (377 cited, at 6300). Every cited engine's runner but the small block's is P's rule,
0.5 (5000 / the target's peak-torque rpm)^1.3 m, which puts the runner's tune exactly at the speed the torque is
measured at:

| Archetype | Runner | Target's torque peak | The rule's length |
|---|---|---|---|
| Flat six | 0.53 m | 4750 rpm | 0.53 |
| VR6 | 0.63 | 4200 | 0.63 |
| 7.0 V8 | 0.53 | 4800 | 0.53 |
| 3.8 V6 | 0.67 | 4000 | 0.67 |
| Four-cam V8 | 0.57 | 4500 | 0.57 |
| Roadster | 0.50 | 5000 | 0.50 |
| Small block | 0.18 | 3000 | (0.97: not the rule) |

So each cited engine is measured on the top of its own ram (N3-6: 12 to 18 % on the flat six). Whether the ram's height
or the runner is wrong, the fix is a cited runner: each real engine's port-plus-runner length (the LS7's, the VR6's, the
3800's, the Coyote's, the MX-5's are documented engines), so the model's tooth falls where the real one does. Asked of P.

## N3-4, settled: stage B needs its runner's ram (20:47 by `date -u`; before G's flowClose is live in the engine)

Cylinder 1's intake valve over a cycle at 6500 rpm (g a cycle): the low cam held draws in 0.583 and pushes back 0.024
(0.021 of it after the bottom); stage B draws in 0.535 and pushes back 0.081 (0.070 after the bottom). The long cam's
late closing loses the charge back up a runner that does nothing to hold it. Stage B against the runner's length (VE,
torque N m):

| Runner | 6500 rpm | 8000 rpm |
|---|---|---|
| 0.20 m | 0.769, 160 | 0.780, 154 |
| 0.30 m (the build: P's rule for a 7500 rpm torque peak) | 0.773, 160 | 0.816, 160 |
| 0.40 m | 0.826, 169 | **1.031, 211** |
| 0.50 m | **1.034, 219** | 0.883, 178 |
| Stage A held, 0.20 m | 0.873, 187 | 0.873, 177 |

The long cam lives on the ram at its closing, the short one hardly cares: a real high-lift stage is matched to its
runner. So N3-4 is not the model's: it is the build's runner (P's rule, not the K20A's intake). Routed to P for the
K20A's runner, cited; my note to N1 that it was the model's is withdrawn.

## N3-9. Where the tuned four-valves lose at speed: their runners again (20:50 by `date -u`)

The balance at speed (tip, tee::Build path; before G's flowClose is live):

| Engine, rpm | Torque, N m | kW | VE | Released | Walls | Gross | Pumping | Friction | Brake |
|---|---|---|---|---|---|---|---|---|---|
| Flat-plane 4.5, 7000 | 389.6 | 286 | 0.809 | 0.924 | 0.113 | 0.396 | 0.004 | 0.063 | 0.329 |
| Flat-plane 4.5, 8500 | 376.2 | 335 | 0.825 | 0.937 | 0.110 | 0.403 | 0.009 | 0.077 | 0.316 |
| Two-stage 2.0, 7500 | 148.8 | 117 | 0.759 | 0.913 | 0.120 | 0.380 | 0.011 | 0.073 | 0.296 |
| Two-stage 2.0, 8800 | 177.9 | 164 | 0.916 | 0.925 | 0.113 | 0.386 | 0.015 | 0.074 | 0.297 |

Every share but the air is a real engine's; the VE is the short one (95 to 100 kW a litre wants VE above 1.0 at the
power peak). Not the valves: the flat-plane's Mach index (Heywood §7.3) is 0.23, 0.31, 0.35 at 6000, 8000, 9000 rpm,
far under the 0.6 where VE falls away, and its ports at 2.0 (from 1.65) do not help (VE 0.785 / 0.813 at 7000 / 8500).
Its runner does: the build switches to a second length of 0.18 m above 6200 rpm (P's rule):

| Second runner | 7000 rpm | 8000 rpm | 8500 rpm |
|---|---|---|---|
| 0.18 m (as built) | 390 N m, VE 0.809 | 378, 0.812 | 376, 0.825 |
| 0.25 m | 399, 0.831 | 383, 0.827 | 394, 0.867 |
| 0.30 m | 383, 0.807 | **469, 0.985** (392 kW) | 433, 0.933 |
| 0.35 m | **480, 0.979** | 441, 0.943 | **460, 1.003** (410 kW) |

The target is 400 to 445 kW at 8000 to 9000 rpm. The two-stage's stage B is the same (N3-4: 211 N m and VE 1.03 at 8000
on a 0.40 m runner). So the tuned engines' shortfall at speed is their runners too, not a missing flow or an exhaust's
scavenging (G's closing moves NA engines under 1 %).

## Open: the runner's ram, its height (the lead's frame, both sides)

- *The studies:* published runner-length gains of 6 % (a turbocharged diesel), 8.7 % (a petrol engine, a 528 mm runner)
  and a 5.4 % average over a speed range from two lengths (single studies, geometry not matched to ours); Ohata and
  Ishida (SAE 820407) and Heywood §7.6's figures were not opened.
- *The model:* 12 to 18 % at the tune on the flat six; VE 1.03 on the two-stage's stage B and 1.00 on the flat-plane on
  matched runners. The losses in it are of textbook size (N3-6).
- *What the short engines need:* VE 1.0 and over at their power peaks, which the model reaches only with its ram as
  strong as it is (on matched runners). Halving it would trade the seven for the nine.
- *To decide:* re-measure the seven on the real engines' runners (P); if a tooth of this height at the right speed still
  leaves them over, and the tuned ones are met with real runners, a sourced damping has its case. Neither yet.

## The sixteen on P's class runners and cited bands (20:56 by `date -u`; P's b24fbca and after, G's flowClose live)

Every NA petrol runner now P's class default of 0.40 m (judgement) or a hand-set length (judgement); every band cited.
Torque at the target's peak-torque speed, power at its peak-power speed, against the band's middle:

```
torque: mean -7.2 %, sd 16.3 %, in band 4 of 16
power: mean +2.4 %, sd 14.0 %, in band 6 of 16

  small-block-57-carb        cited    torque  +23.7 % (448 at 3000 for [340, 385], VE 0.80)  power  +38.4 % (194 kW at 4600 for [120, 160], VE 0.79)
  twin-cam-roadster-18       cited    torque  +10.3 % (180 at 5000 for [155, 172], VE 0.94)  power  +15.2 % (119 kW at 6650 for [95, 112], VE 0.95)
  air-cooled-flat-six-36     cited    torque   +7.4 % (333 at 4750 for [300, 320], VE 0.86)  power   +9.4 % (205 kW at 6050 for [180, 195], VE 0.87)
  big-block-74-pushrod-v8    cited    torque   +6.8 % (561 at 3000 for [510, 540], VE 0.78)  power  +17.9 % (239 kW at 4200 for [195, 210], VE 0.79)
  vr6-28                     cited    torque   +5.8 % (249 at 4200 for [230, 240], VE 0.87)  power   +6.4 % (137 kW at 5800 for [125, 132], VE 0.85)
  pushrod-v6-38              cited    torque   +1.8 % (321 at 4000 for [300, 330], VE 0.85)  power   -1.8 % (147 kW at 5200 for [140, 160], VE 0.79)
  big-bore-70-pushrod-v8     cited    torque   -0.9 % (632 at 4800 for [630, 645], VE 0.87)  power   +4.9 % (396 kW at 6300 for [370, 385], VE 0.90)
  economy-four-15            cited    torque   -1.9 % (143 at 4400 for [135, 156], VE 0.88)  power   +4.8 % (93 kW at 6300 for [80, 97], VE 0.93)
  hot-hatch-16-16v           cited    torque   -2.6 % (144 at 6050 for [140, 155], VE 0.87)  power   -6.1 % (96 kW at 6850 for [85, 120], VE 0.84)
  straight-six-32-high-rev   cited    torque  -14.5 % (312 at 4900 for [355, 375], VE 0.89)  power   -6.6 % (236 kW at 7800 for [245, 260], VE 0.90)
  v6-35-na                   cited    torque  -15.9 % (294 at 4750 for [335, 365], VE 0.81)  power   -4.4 % (201 kW at 6350 for [195, 225], VE 0.87)
  four-cam-v8-50             cited    torque  -16.4 % (458 at 4500 for [520, 575], VE 0.84)  power   +8.1 % (349 kW at 6850 for [300, 345], VE 0.94)
  v10-52                     cited    torque  -27.1 % (397 at 6500 for [520, 570], VE 0.76)  power   -7.8 % (397 kW at 8350 for [385, 475], VE 0.90)
  v12-65                     cited    torque  -28.1 % (507 at 6250 for [680, 730], VE 0.72)  power   +0.4 % (547 kW at 8575 for [500, 590], VE 0.90)
  two-stage-cam-20           cited    torque  -30.0 % (149 at 7500 for [205, 220], VE 0.76)  power  -18.5 % (145 kW at 8300 for [170, 185], VE 0.85)
  flat-plane-v8-45           cited    torque  -33.4 % (360 at 6000 for [520, 560], VE 0.74)  power  -21.1 % (333 kW at 8500 for [400, 445], VE 0.82)
```

**The lead's question answered:** with the runners no longer tuned to the measured speed, the ram's tooth at the wrong
speed leaves the cited seven at -1 to +10 % in torque (7.0 -0.9, 3.8 V6 +1.8, VR6 +5.8, flat six +7.4, roadster +10.3)
but for the small block (+23.7, N3-7, open) and the four-cam, now short (-16.4). The tooth no longer inflates them: no
case for a damping from the seven's side.

**The tuned four-valves are the miss now:** torque 27 to 33 % short (V10, V12, two-stage, flat-plane) with VE 0.72 to
0.76 at their torque peaks; power closer (-21 to +0.4 %). N3-9 showed the flat-plane reaching VE 1.0 and 410 kW on a
0.35 m second runner and the two-stage's stage B VE 1.03 on 0.40 m: their runners remain judgement, and no real engine's
runner was found. Stage B re-measured on G's closing (live in the engine): 159 / 149 / 173 N m at 6500 / 7500 / 8500 rpm,
VE 0.769 / 0.758 / 0.887 (stage A held: 206 / 194 / 196, VE 0.946 / 0.927 / 0.974): unchanged by it.

## N3-10. A long-cam engine fills only on its runner's tooth (20:57 by `date -u`)

The V12 (cited 680 to 730 N m; its runner 0.30 m, judgement; cams 272 / 264 degrees at the seat, the intake parked at
126), full load:

| rpm | 3000 | 4500 | 5500 | 6250 | 7000 | 8000 | 8575 |
|---|---|---|---|---|---|---|---|
| Torque, phaser scheduled (N1's rule), N m | 504 | 645 | 548 | 548 | 490 | 626 | 579 |
| VE | 0.696 | 0.866 | 0.755 | 0.771 | 0.719 | 0.905 | 0.861 |
| Intake advance, deg | 50 | 38 | 27 | 20 | 15 | 15 | 15 |
| Torque, both phasers parked | 527 | 523 | 512 | 531 | 467 | 595 | 554 |
| VE | 0.721 | 0.718 | 0.712 | 0.746 | 0.688 | 0.861 | 0.824 |

Between its runner's teeth the long-cam engine fills only 0.70 to 0.75; the phaser lifts the 4500 tooth and a little
elsewhere, and is not what holds the middle down. A long cam's late closing lives on a ram at its closing (N3-4), so
where such an engine makes its torque is where its runner is tuned: real ones are tuned at their torque peak by design.
With P's rule gone (no part from the engine's own targets) and no real runner found, a class default that does not look
at the targets would have to come from something else the build holds: for example a runner tuned at a fixed share of
the engine's own redline. That is P's and the lead's decision; proposed, not done.

## N3-11. The tuned four-valves: three sensitivities side by side (21:04 by `date -u`; the lead's three)

Torque, N m (VE) at each engine's target peak-torque and peak-power speeds; one change at a time from the build as it
stands on the tip (tee::Build path, G's closing live). Fixed advance: the phaser's range 0 and the intake's centreline
moved earlier by that many degrees (what a baked map holds at that speed).

| Change | Flat-plane 4.5, 6000 / 8500 | V10, 6500 / 8350 | Two-stage 2.0, 7500 / 8300 |
|---|---|---|---|
| As built (phaser by N1's rule) | 403 (0.814) / 395 (0.862) | 410 (0.776) / 453 (0.895) | 149 (0.758) / 166 (0.855) |
| The rule's advance, deg | 25 / 17 | 15 / 15 | (no phaser) |
| Best fixed advance | 400 at 30 / 436 at 50 | 411 at 10 / 453 at 20 | 153 at 40 / 168 at 30 |
| Runner switch: never | 406 / 433 | | |
| Runner switch at 7000 rpm (from 5850) | 406 / 396 | | |
| Headers' length dial 0 (0.61 x the layout's) | 366 / 383 | 428 / 456 | 150 / 167 |
| Headers' length dial 1 (1.63 x the layout's) | **452 (0.897)** / 401 | **478 (0.867)** / **487 (0.957)** | **179 (0.866)** / **186 (0.925)** |
| A log manifold (every primary 0.05 m) | 291 (0.637) / 279 (0.675) | 423 / 432 | 127 (0.689) / 135 (0.747) |
| Target band | 520 to 560 / 400 to 445 kW | 520 to 570 / 385 to 475 kW | 205 to 220 / 170 to 185 kW |

(C's `tee-dyno --bake-cam` on the flat-plane, predicted points, agrees: best 404 N m at 20 degrees near 5950 rpm, and
the most advance there is (50) wanted from 7400 rpm up.)

What each says:
1. *The phaser.* At the torque peaks the rule is at or near the best fixed advance (flat-plane +0, V10 +0, two-stage has
   none). At the flat-plane's power peak it is 33 degrees short of best (+10 %). A baked map is worth a few per cent at
   the top, not the torque rows.
2. *The runner's switch.* The flat-plane's switch to its short runner (at 5850 rpm) costs 10 % at 8500 rpm (433 never
   switched against 395) and nothing at 6000: the second length (P's judgement) is too short for its top end.
3. *The exhaust.* The wave arrives: a log manifold costs the flat-plane 28 %, the two-stage 14 to 19 %, and longer
   primaries (the dial's top, 1.63 x the layout's) lift every one, +7 to +20 %, the residual falling from 4 to 8 % to 1
   to 3 %. G's model scavenges as a tuned header should; the primaries' lengths are the builds' (the layout's default
   scaled by the dial: P's content). The V10 is the odd one: a log is 3 % better than its headers at 6500, so its
   primaries as laid out arrive out of phase there.

Whose it is: mostly content (the primaries' lengths, the second runner), some the phaser's top end (a baked map). Even
the best of each alone leaves the flat-plane's torque 13 % and the V10's 8 % under their bands; together not measured.

## N3-12. The exhaust's help at overlap: present, and as large as the intake's ram (21:07 by `date -u`; the lead's item 3)

The V12 and the flat-plane 4.5 at 5500, 6250 and 7000 rpm, every primary at 0.5, 0.705 (as built: the layout's
default) and 0.9 m, the collector as built; per cycle, cylinder 1 through its overlap (both valves passing gas): the
exhaust valve's flow out and back, the intake's in and back (g), and the mean gauge pressures at the exhaust port,
in the cylinder and at the intake port (the port's pressure read as twice its arriving wave):

```
v12-65 prim 0.5 5500: tq 603.1 ve 0.825 resid 0.026
  prim 0.5   5500: overlap 2.91 ms/cyc; exh valve out 0.0657 back 0.0057 g; intake in 0.0472 back 0.0050 g; mean gauge kPa: exh port -6.7 (min -33.9), cyl -3.4, intake port -4.3
v12-65 prim 0.5 6250: tq 622.4 ve 0.861 resid 0.024
  prim 0.5   6250: overlap 2.05 ms/cyc; exh valve out 0.0518 back 0.0001 g; intake in 0.0718 back 0.0054 g; mean gauge kPa: exh port -7.8 (min -32.7), cyl +2.8, intake port -5.6
v12-65 prim 0.5 7000: tq 549.5 ve 0.791 resid 0.042
  prim 0.5   7000: overlap 1.86 ms/cyc; exh valve out 0.0371 back 0.0016 g; intake in 0.0294 back 0.0068 g; mean gauge kPa: exh port -7.2 (min -32.3), cyl -5.0, intake port -12.8
v12-65 prim 0.705 5500: tq 480.9 ve 0.674 resid 0.080
  prim 0.705 5500: overlap 2.92 ms/cyc; exh valve out 0.0466 back 0.0002 g; intake in 0.0409 back 0.0222 g; mean gauge kPa: exh port +1.7 (min -33.4), cyl +7.2, intake port -6.5
v12-65 prim 0.705 6250: tq 520.6 ve 0.738 resid 0.075
  prim 0.705 6250: overlap 2.05 ms/cyc; exh valve out 0.0254 back 0.0059 g; intake in 0.0445 back 0.0216 g; mean gauge kPa: exh port +10.0 (min -28.6), cyl +17.4, intake port -3.0
v12-65 prim 0.705 7000: tq 487.2 ve 0.715 resid 0.083
  prim 0.705 7000: overlap 1.86 ms/cyc; exh valve out 0.0168 back 0.0139 g; intake in 0.0142 back 0.0265 g; mean gauge kPa: exh port +14.8 (min -15.6), cyl +4.3, intake port -12.5
v12-65 prim 0.9 5500: tq 388.3 ve 0.562 resid 0.144
  prim 0.9   5500: overlap 2.92 ms/cyc; exh valve out 0.0242 back 0.0238 g; intake in 0.0164 back 0.0444 g; mean gauge kPa: exh port +8.8 (min -15.3), cyl +4.1, intake port -6.7
v12-65 prim 0.9 6250: tq 528.2 ve 0.743 resid 0.079
  prim 0.9   6250: overlap 2.05 ms/cyc; exh valve out 0.0153 back 0.0153 g; intake in 0.0252 back 0.0241 g; mean gauge kPa: exh port +14.6 (min -27.6), cyl +9.5, intake port -2.9
v12-65 prim 0.9 7000: tq 541.8 ve 0.776 resid 0.053
  prim 0.9   7000: overlap 1.85 ms/cyc; exh valve out 0.0167 back 0.0176 g; intake in 0.0149 back 0.0218 g; mean gauge kPa: exh port +7.3 (min -51.3), cyl -11.7, intake port -12.1
flat-plane-v8-45 prim 0.5 5500: tq 413.0 ve 0.832 resid 0.035
  prim 0.5   5500: overlap 2.56 ms/cyc; exh valve out 0.0696 back 0.0129 g; intake in 0.0582 back 0.0173 g; mean gauge kPa: exh port -5.3 (min -54.8), cyl -6.8, intake port -2.2
flat-plane-v8-45 prim 0.5 6250: tq 407.4 ve 0.834 resid 0.019
  prim 0.5   6250: overlap 2.38 ms/cyc; exh valve out 0.0699 back 0.0044 g; intake in 0.0944 back 0.0017 g; mean gauge kPa: exh port -9.4 (min -51.0), cyl -5.3, intake port -1.0
flat-plane-v8-45 prim 0.5 7000: tq 391.2 ve 0.819 resid 0.027
  prim 0.5   7000: overlap 2.06 ms/cyc; exh valve out 0.0524 back 0.0019 g; intake in 0.0747 back 0.0021 g; mean gauge kPa: exh port -9.7 (min -37.9), cyl -4.0, intake port -4.6
flat-plane-v8-45 prim 0.705 5500: tq 380.4 ve 0.776 resid 0.048
  prim 0.705 5500: overlap 2.57 ms/cyc; exh valve out 0.0536 back 0.0024 g; intake in 0.0365 back 0.0101 g; mean gauge kPa: exh port -3.2 (min -35.9), cyl -2.5, intake port -6.1
flat-plane-v8-45 prim 0.705 6250: tq 385.7 ve 0.788 resid 0.048
  prim 0.705 6250: overlap 2.44 ms/cyc; exh valve out 0.0387 back 0.0022 g; intake in 0.0359 back 0.0095 g; mean gauge kPa: exh port -1.2 (min -23.6), cyl -3.3, intake port -7.0
flat-plane-v8-45 prim 0.705 7000: tq 372.8 ve 0.780 resid 0.039
  prim 0.705 7000: overlap 2.06 ms/cyc; exh valve out 0.0332 back 0.0009 g; intake in 0.0448 back 0.0064 g; mean gauge kPa: exh port -2.1 (min -14.6), cyl -1.9, intake port -6.1
flat-plane-v8-45 prim 0.9 5500: tq 342.0 ve 0.700 resid 0.079
  prim 0.9   5500: overlap 2.57 ms/cyc; exh valve out 0.0243 back 0.0042 g; intake in 0.0210 back 0.0177 g; mean gauge kPa: exh port +0.5 (min -17.2), cyl -3.2, intake port -6.1
flat-plane-v8-45 prim 0.9 6250: tq 386.6 ve 0.796 resid 0.043
  prim 0.9   6250: overlap 2.44 ms/cyc; exh valve out 0.0230 back 0.0027 g; intake in 0.0286 back 0.0075 g; mean gauge kPa: exh port -3.4 (min -35.8), cyl -6.8, intake port -6.6
flat-plane-v8-45 prim 0.9 7000: tq 396.0 ve 0.820 resid 0.025
  prim 0.9   7000: overlap 2.06 ms/cyc; exh valve out 0.0246 back 0.0014 g; intake in 0.0497 back 0.0032 g; mean gauge kPa: exh port -14.5 (min -53.8), cyl -9.3, intake port -5.0
```

Read across: with primaries that return their suction wave in the overlap (0.5 m on the V12 at 5500 to 6250, on the
flat-plane at 6250 to 7000), the exhaust port sits 5 to 10 kPa below the air through it (troughs to -33 to -55 kPa),
gas leaves by the exhaust valve with little back, the intake draws through, the residual falls to 2 to 3 %, and VE
between the intake's teeth is 0.82 to 0.86. Out of phase (the V12's 0.705 m at 6250 to 7000, its 0.9 m at 5500), the
port is 9 to 15 kPa over the air, gas comes back from the header and pushes up the intake (0.02 to 0.04 g a cycle),
the residual is 8 to 14 %, VE 0.56 to 0.74. The swing is some 20 to 30 % of VE: as large as the intake's tooth.

So the model scavenges at overlap as a tuned header should (G's wave reaches the valve), and the intake's ram is not
asked to do both jobs. The tuned engines' middle is held down by where their primaries are tuned: the layout's default
length (0.705 m on both, not from the targets) puts the V12's suction outside its overlap from 6250 rpm. The lever is the
builds' primaries (content, P), as it is the runners'. The runner-at-a-share-of-redline proposal stays held.

## N3-13. The four-cam V8 and the 3.5 V6: their runners again (21:11 by `date -u`; the lead's item 2)

Both burn and lose as real engines do (brake 0.30 to 0.33, walls 0.12 to 0.13, residual 3 to 4 %); both are short of
air at their torque peaks (VE 0.82 to 0.83). Four-cam (runner 0.38 m, P's modern-composite class; primaries 0.705) at
4500 rpm, N m (VE): as built 458 (0.832); primaries 0.5 m 436 (0.801), 0.9 m 412 (0.757); runner 0.50 m 491 (0.895),
0.60 m **541 (0.969)** (band 520 to 575). The 3.5 V6 (0.38 m, switching to 0.25 m above 4350 rpm): at 4750 / 6350 rpm
as built 302 / 279; second runner 0.38 (or never switching) 311 / 337 (0.848 / 0.965); 0.50 m and never switching **348
/ 358** (0.945 / 1.027) (band 335 to 365 at 4750). Not the model's: the runner class and its switch (P's judgement).

## N3-14. The overrun charge's burn (the lead's item 1; worked, not yet measured: 21:12 by `date -u`)

N1 found H9's fuelled overrun (the V8 preset, overrun fuel 0.25, the spark 11.5 degrees after the top) burning in 53 to
55 degrees, 99 % done by 66 after the top, the exhaust opening at 104: nothing left to light in the pipe.

What the model's burn length holds (`combustion.fm`, charge1): (44 + 11 (0.8 - p_manifold / 1 atm)) x (1 + 2.5 (dilution
- 0.15)) x the mixture's flame (S_L ratio^-0.5) x bore-stroke x CR^-0.3 x (rpm / 3000)^(1/3). Two dependences a real
burn has are missing or weaker:

1. *Where the burn sits.* A charge lit after the top burns in gas that the expansion cools and thins. With the laminar
   flame speed as Metghalchi and Keck have it, S_L ∝ T_u^α p^β (α 2.18, β -0.16 near stoichiometric), the unburned gas
   expanded isentropically (γ 1.35), and the model's own convention (the burn's length as S_L^-0.5), the length goes as
   (V_mid / V_ref)^0.27, V_mid the volume at the burn's middle and V_ref at a best-torque burn's (8 degrees after the
   top). Worked for H9's charge (CR 10, rod ratio 3.5): 54 → about 70 degrees (iterated). A Wiebe burn of 70 degrees lit
   at 11.5 after the top is still more than 99 % done by the exhaust's opening at 104: this alone does not make the
   overrun's crackle. (It does lengthen every late-spark burn by up to a third, the knock retard's and the catalyst-
   heating cases' included, and is worth adding for that.)
2. *The burned gas's dilution.* The model's (1 + 2.5 dx) is linear. Metghalchi and Keck's flame speed falls as (1 -
   2.1 x_b) (linear); Rhodes and Keck's (SAE 850047) as (1 - 2.06 x_b^0.77), much steeper at high dilution. At x_b 0.35
   against 0.15 the burn's length (S_L^-0.5) grows 1.61 times by the first, 2.54 by the second; the model gives 1.50.
   An overrun charge behind a shut throttle carries some 30 to 40 % burned gas; by the steeper form its burn would run
   past the exhaust's opening. Which form, and the overrun charge's own burned share, decide it: the first not
   reachable here (Heywood §9.3 quotes the flame-speed correlations), the second to be read off the overrun.

Proposed (not changed): (1) as above in charge1, model-on only; (2) once the overrun's burned share is read (N1 has the
scenario) and a source for the dilution term's form is open. Both are mine to make (H's path) under card N's rule.

**N3-14, with N1's overrun measured (21:47 by `date -u`).** H9's fuelled overrun (the V8 preset, overrun fuel 0.25, the
spark 11.5 degrees after the top), every 0.1 s from the lift (N1, comb.dilution and intk.pPlenum): burned share 0.05 to
0.19 (falling through the overrun), manifold 15 to 23 kPa, burns of 46 to 55 degrees; on the limiter before the lift
0.23, 38.5 kPa, 66 degrees; the exhaust opens at 104. So the dilution term is not the missing piece: at 5 to 19 % the
steeper published form and the model's differ by a few per cent (the cut firings leave air, not burned gas). The
expansion's term (54 → about 70 degrees) leaves the burn done before the exhaust opens. Neither in-cylinder flame-speed
term carries an overrun's fired charge to the pipe. What real engines show at heavy retard and low density is partial
burns and flame quenching late in the expansion (Heywood ch. 9 on partial burning; not opened here): a share of charges
that stop burning, not a slower whole. That needs its own source (the partial-burn frequency against retard and load)
before anything is built; until then the pop store's spark-time estimate stands. The expansion's term stays proposed
for late burns generally (knock retard, catalyst heating), not for the overrun.

# The boosted fourteen (the lead's next block; from 21:52 by `date -u`, commit 810fbbc)

## B-1. The table

Each boosted archetype at its target's peak-torque and peak-power speeds (settled dyno, 4.5 s settle, tee::Build path),
against the bands' middles, with P's basis (torque/power), the boost measured (tel.boost.pressure, kPa over the air)
and the build's boost target:

```
torque: mean -5.5 %, sd 7.0 %, median -6.4 %, in band 9 of 14
power: mean +0.2 %, sd 11.7 %, median +2.6 %, in band 10 of 14

  supercharged-v8-62           judgement/judgement torque   +8.0 % (810 at 3850 for [720, 780], boost 60 kPa)  power   +0.5 % (417 kW at 6000 for [395, 435], boost 84)  build target 62 kPa
  straight-six-twin-turbo-30   recalled/recalled torque   +6.8 % (561 at 3350 for [450, 600], boost 100 kPa)  power   -2.6 % (287 kW at 5900 for [250, 340], boost 90)  build target 100 kPa
  boxer-four-turbo-unequal     recalled/recalled torque   +1.9 % (402 at 4000 for [380, 410], boost 97 kPa)  power   +0.1 % (223 kW at 6100 for [210, 235], boost 107)  build target 100 kPa
  inline-five-turbo-rally      judgement/judgement torque   -3.1 % (436 at 4500 for [380, 520], boost 116 kPa)  power  +10.7 % (257 kW at 6250 for [200, 265], boost 136)  build target 130 kPa
  turbo-four-20-gdi            recalled/recalled torque   -4.7 % (357 at 3050 for [350, 400], boost 103 kPa)  power   -9.4 % (174 kW at 5750 for [155, 230], boost 86)  build target 120 kPa
  tractor-15-inline-six        recalled/recalled torque   -5.6 % (2257 at 1100 for [2000, 2780], boost 261 kPa)  power  +15.8 % (434 kW at 1800 for [300, 450], boost 165)  build target 260 kPa
  three-cylinder-10-turbo      recalled/recalled torque   -6.4 % (178 at 3000 for [170, 210], boost 101 kPa)  power   -4.2 % (86 kW at 5500 for [74, 105], boost 116)  build target 100 kPa
  twin-turbo-hot-vee-v8-40     recalled/recalled torque   -6.4 % (726 at 3250 for [700, 850], boost 124 kPa)  power   +4.7 % (429 kW at 6050 for [350, 470], boost 110)  build target 120 kPa
  flat-plane-v8-39-twin-turbo  judgement/judgement torque   -7.4 % (725 at 4375 for [760, 805], boost 136 kPa)  power   +8.7 % (536 kW at 7750 for [450, 535], boost 124)  build target 130 kPa
  v6-35-twin-turbo-pickup      recalled/recalled torque   -7.7 % (614 at 3500 for [630, 700], boost 105 kPa)  power   +5.5 % (306 kW at 5500 for [275, 305], boost 115)  build target 100 kPa
  diesel-20-common-rail        recalled/recalled torque   -8.3 % (321 at 2350 for [320, 380], boost 123 kPa)  power  +11.6 % (128 kW at 3850 for [100, 130], boost 158)  build target 140 kPa
  kei-660-triple-turbo         judgement/judgement torque  -11.3 % (93 at 3500 for [100, 110], boost 65 kPa)  power  -11.8 % (41 kW at 6500 for [44, 50], boost 17)  build target 65 kPa
  rotary-twin-13               recalled/recalled torque  -14.1 % (260 at 5000 for [290, 315], boost 59 kPa)  power  -31.8 % (136 kW at 6650 for [190, 210], boost 25)  build target 70 kPa
  pickup-diesel-67-inline-six  recalled/recalled torque  -18.0 % (1012 at 1700 for [1000, 1470], boost 243 kPa)  power   +5.2 % (300 kW at 2887 for [255, 315], boost 154)  build target 200 kPa
```

No common level: torque a little under on the mean, power on it, and the spread half the NA set's. The misses group
by mechanism, not by level:
- *boost that collapses at the power peak:* the rotary (25 kPa at 6650 rpm against its 70 target: power -32 %) and the
  kei (17 against 65 at 6500: -12 %). The turbo runs out at speed: the matching (N2's rework of the design point).
- *boost over its target at low speed, on the diesels:* the pickup 243 kPa at 1700 rpm (target 200), the tractor 261 at
  1100 (260), the 2.0 158 at 3850 (140); their power rows read over (+5 to +16 %).
- *the supercharged V8's torque* +8 % at a boost under its target (60 against 62): the machine's or the build's, to see.
Most bands are recalled or judgement (P is citing them).

## B-2. Four balances (21:54 by `date -u`; the probe's BAL_MEAN over the window, the boost machine's running sums as the
window's increments)

```
turbo-four-20-gdi      3049.16: tq 357  boost 117/120 kPa  PR 2.21 eta_c 0.611 t2 453 K -> cooler out 312 K  turbine inlet 225 kPa abs / plenum 218  knock 11.4 deg  lambda 0.819  Tivc 394
   air kg: compressor 0.1054, drawn 0.1062, throttle 0.1066, valves 0.1067, trapped 0.1090, metered 0.1090; into housings 0.1234, downpipes 0.1234
   energy/fuel: released 0.791 walls 0.102 gross 0.304 pump +0.001 fric 0.019 brake 0.284 exhaust 0.385; turbine 0.036 compressor 0.036; first law +0.002
turbo-four-20-gdi      5750.66: tq 290  boost 92/120 kPa  PR 2.00 eta_c 0.753 t2 372 K -> cooler out 309 K  turbine inlet 241 kPa abs / plenum 194  knock 0.0 deg  lambda 0.832  Tivc 426
   air kg: compressor 0.1660, drawn 0.1666, throttle 0.1656, valves 0.1664, trapped 0.1661, metered 0.1661; into housings 0.1876, downpipes 0.1876
   energy/fuel: released 0.840 walls 0.108 gross 0.340 pump +0.016 fric 0.034 brake 0.290 exhaust 0.406; turbine 0.023 compressor 0.023; first law +0.002
flat-plane-v8-39-twin- 4373.6: tq 725  boost 126/130 kPa  PR 2.32 eta_c 0.691 t2 418 K -> cooler out 315 K  turbine inlet 226 kPa abs / plenum 228  knock 6.9 deg  lambda 0.816  Tivc 400
   air kg: compressor 0.3126, drawn 0.3114, throttle 0.3089, valves 0.3099, trapped 0.3057, metered 0.3057; into housings 0.3090, downpipes 0.3091
   energy/fuel: released 0.797 walls 0.091 gross 0.314 pump +0.001 fric 0.020 brake 0.294 exhaust 0.391; turbine 0.034 compressor 0.033; first law +0.001
flat-plane-v8-39-twin- 7750.98: tq 660  boost 130/130 kPa  PR 2.47 eta_c 0.691 t2 411 K -> cooler out 319 K  turbine inlet 318 kPa abs / plenum 232  knock 0.0 deg  lambda 0.815  Tivc 440
   air kg: compressor 0.5454, drawn 0.5460, throttle 0.5384, valves 0.5393, trapped 0.5395, metered 0.5395; into housings 0.5603, downpipes 0.5604
   energy/fuel: released 0.796 walls 0.089 gross 0.320 pump +0.019 fric 0.033 brake 0.268 exhaust 0.404; turbine 0.034 compressor 0.034; first law +0.002
pickup-diesel-67-inlin 1699.57: tq 1012  boost 196/200 kPa  PR 3.03 eta_c 0.696 t2 442 K -> cooler out 326 K  turbine inlet 413 kPa abs / plenum 298  knock 0.0 deg  lambda 0.000  Tivc 394
   air kg: compressor 0.2949, drawn 0.2863, throttle 0.2642, valves 0.2624, trapped 0.2625, metered 0.0000; into housings 0.3251, downpipes 0.3254
pickup-diesel-67-inlin 2886.97: tq 992  boost 153/200 kPa  PR 2.67 eta_c 0.612 t2 441 K -> cooler out 326 K  turbine inlet 352 kPa abs / plenum 255  knock 0.0 deg  lambda 0.000  Tivc 401
   air kg: compressor 0.3818, drawn 0.3818, throttle 0.3799, valves 0.3798, trapped 0.3816, metered 0.0000; into housings 0.4247, downpipes 0.4248
rotary-twin-13         4999.99: tq 260  boost 70/70 kPa  PR 1.81 eta_c 0.757 t2 358 K -> cooler out 307 K  turbine inlet 199 kPa abs / plenum 171  knock 14.8 deg  lambda 0.845  Tivc 468
   air kg: compressor 0.1424, drawn 0.1424, throttle 0.1394, valves 0.1454, trapped 0.1446, metered 0.1446; into housings 0.1326, downpipes 0.1326
   energy/fuel: released 0.808 walls 0.154 gross 0.288 pump +0.009 fric 0.015 brake 0.264 exhaust 0.360; turbine 0.020 compressor 0.020; first law +0.017
rotary-twin-13         6649.92: tq 196  boost 27/70 kPa  PR 1.39 eta_c 0.732 t2 327 K -> cooler out 299 K  turbine inlet 167 kPa abs / plenum 128  knock 0.0 deg  lambda 0.872  Tivc 463
   air kg: compressor 0.1519, drawn 0.1519, throttle 0.1465, valves 0.1482, trapped 0.1478, metered 0.1478; into housings 0.1266, downpipes 0.1266
   energy/fuel: released 0.844 walls 0.176 gross 0.309 pump +0.018 fric 0.024 brake 0.267 exhaust 0.365; turbine 0.014 compressor 0.013; first law +0.016
```

(The diesel's fuel is not counted by the probe (its charge is chargeDiesel's), so its energy line is left out.)

What is out, and whose:
- *Boost at the top:* 92 of 120 kPa on the turbo four at 5750 rpm, 153 of 200 on the pickup at 2887, 27 of 70 on the
  rotary at 6650; on target at the torque peaks. The turbines run out at speed: the matching (N2's design-point rework).
- *Knock:* 11.4 degrees of retard on the turbo four at 3050 rpm, 6.9 on the twin-turbo V8 at 4375, 14.8 on the rotary at
  5000; brake efficiency 0.26 to 0.29. Its cost is N2's (the knock-limit table) to weigh against the fuels' sources.
- *The cooler:* its effectiveness (compressor out to cooler out against the air at 288 K) 0.85 on the turbo four, 0.79
  on the V8, 0.75 on the pickup, 0.73 on the rotary; Bosch's air-to-air range 0.60 to 0.80: the turbo four's a little
  above it (charge 312 K). The lead's "too cool for an installed engine" holds there, mildly.
- *Back pressure over boost:* turbine inlet over plenum 1.0 to 1.39 (1.24 the turbo four at 5750, 1.37 the V8 at 7750,
  1.39 the pickup at 1700): inside Watson and Janota's 1.0 to 1.5.
- *Energy:* the turbine's work equals the compressor's within 0.001 of the fuel's heat (the shafts steady), 0.013 to
  0.036 of it; released 0.79 to 0.84 at λ 0.82 to 0.87 (the rich mixture's chemistry, N3-2); walls 0.09 to 0.18 (the
  rotary's the most, as its chamber's surface has it); first law within 0.2 % on the pistons, 1.6 to 1.7 % on the
  rotary (its seals' leakage is not in the probe's books).
- *The air's books:* the compressor, the machine's draw, the throttle, the valves and the trapped agree within 1 to 3 %
  on the turbo four, the V8 and the rotary; **the pickup at 1700 rpm parts 8 %** (the compressor 0.2949 kg and the draw
  0.2863 against the throttle 0.2642, the valves and the trapped with the throttle): air the machine draws that the
  intake's throttle book does not see. The gas into the turbines' housings (sumMPipes) against what the cylinders let
  out (trapped air and fuel): +4 % on the turbo four at 3050, -7 % on the V8 at 4375, -8 to -9 % on the rotary: the
  exhaust's books into the housings (G's closing is on the pipes; the housings' side to check).

**B-2's two book gaps measured again** (21:56 by `date -u`; 2 s windows). (a) Withdrawn as first stated: the pickup at 1700
rpm, compressor 0.5545 kg, the machine's draw 0.5466 (-1.4 %), the intake's throttle 0.5452 (the draw's), valves 0.5269
and trapped 0.5278 (-3.4 % from the throttle: the intake's throttle book, N1's known term). The first 8 % was a short
window's estimate. (b) Confirmed, and corrected (the cylinder's mass carries no fuel, so against the exhaust valves'
gas alone): into the turbines' housings over out of the exhaust valves, the turbo four at 3050 rpm 1.14 (0.2433 / 0.2128
kg; first 1.13), the twin-turbo V8 at 4375 1.03. G's given-over-let-out (closed for the pipes by flowClose) apparently
still reaching the housings under boost.model 1; it would over-drive the turbines. Sent to G and N2.

**B-2 attributed with N2 (22:06 by `date -u`).** Boost collapsing at the power peak (the four, the pickup, the rotary, the
kei): N2's design-point matching, measured, not adopted; the rotary's sequential pair separately (its second compressor
barely flows at 6000 to 7200 rpm). Knock retard: C's baked knock-limit table, now on the four; N2's proof runs next. The
diesels over their boost targets low down: the VGT's spring-law vanes and N2's surge guard, after the proof. The cooler's
0.85 on the four: the build's own icEffect (the test cell holds the core at the air), so P's field (sent to P). The
housings' 1.14: with G, N2 to be copied before adopting the design point (it would over-drive every matching by that).

**The cooler, corrected (22:11 by `date -u`).** Withdrawn: "0.85 on the turbo four". The cooler does its stated icEffect
0.75 (every boosted build carries 0.75, P): at 3050 rpm it takes the charge pipe (v1) from 386.8 K to 309.5 K against a
core held at the air's 288.15 K, 0.78 on time means. B-2's "effectiveness" used the compressor outlet's time-mean
temperature (459 K), which is not what the cooler sees. Noted for N2, not a finding: that 459 K is above what PR 2.21
at eta 0.61 gives (about 404 K) and above the charge pipe's 387 K; a time mean weighted by low-flow instants, likely.

**The housings' gap, closed by G (22:13 by `date -u`).** G's flowClose matched the valves' volume; the gas cools from the
valve to the collectors (1396 → 1216 K on the four), so the turbines' housings were given 1.14 of the valves' mass
(the V8 1.03). G's fix ("exhaust: with the turbines fed, stepLet's target closed by mass at the housings"): a slow loop
holds Boost's mass from the pipes equal to the valves' let-out. Checked on the tip, the turbo four at 3050 rpm (2 s):
0.1991 kg into the housings against 0.1995 out of the valves, 0.998; boost 107 kPa (was 117), torque 335 N m (was 357).
N2's matching to be re-taken on it (told).

## Two facts from their owners, and the V12's low-speed curve (22:21 by `date -u`)

- G closed the housings' gap by mass (0.997 on the turbo four, 1.007 on the twin V8, 2 s); N2 re-takes the design point.
- G: the NA tail's mean flow taken by mass at the tail's own state moves torque 0.4 to 0.7 % and the residual by 0.001 at
  most (small block 4000, 3.2 six 7000, four-cam 6500): the tuned four-valves' shortfall is not in the exhaust's mean
  flow. Struck from the candidates.

**The V12 at 2049 rpm (the lead's question for the car work).** On settled steps it is a dip, not a peak: 637 / 457 / 647
/ 670 / 551 / 478 / 712 N m at 1500 / 2049 / 2500 / 3000 / 3500 / 4500 / 6000 rpm, VE 0.911 / 0.656 / 0.878 / 0.897 /
0.744 / 0.660 / 0.961, the residual 13.1 % at 2049 against 0.2 to 0.8 % beside it. The intake cam map is baked (advance
47 / 42 / 36 degrees at 1500 / 2049 / 2500: a large overlap low down), knock retard 8 to 9 degrees below 2100.
One change at a time, at 1500 / 2049 / 2500 rpm (N m, VE, residual):
- primaries 0.5 m: 635 / **650** / 635, VE 0.90 at 2049, residual 0.4 %: the dip gone;
- primaries 0.9 m: 646 / 552 / 509: the dip moves up;
- the runner 0.5 m: 651 / 454 / 630: no change at 2049;
- the phaser parked: 394 / 470 / 459 (VE 0.58 to 0.66): the baked map's overlap is what makes the low end strong, and
  what lets the exhaust's wave in.
So the dip is the 0.705 m primaries' (the layout's default) pressure wave arriving during the baked map's large
low-speed overlap, pushing burned gas back. Not the runner, not knock. A maker's map would take overlap out where its
headers revert; the bake's speeds may straddle 2049 without one there (to N1, whose bake rule it is). The model's light
damping (N3-6) keeps these low-speed teeth sharp (VE swinging 0.66 to 0.91 within 500 rpm): it bears on the open ram
item, as does any published low-speed curve for this engine.

## N3-15. The waves' damping: one item, with a bound from the sources (22:22 by `date -u`; the lead's 22:23)

Three findings point the same way: the intake's ram 12 to 18 % at its tune (N3-6); the V12's reversion dip, VE 0.66 to
0.91 within 500 rpm (above); the tuned four-valves' curves in teeth (N3-10). What the published measurements bound:

| Source (kind) | What it measured | Result |
|---|---|---|
| SI engine, runner length and diameter sweep (ResearchGate 394849080; experiment) | VE against runner length | up to **+8.7 %** at mid speed with a 528 mm runner; +4.2 % at high speed by diameter |
| Turbocharged multi-cylinder diesel, variable-length manifold (ResearchGate 287040569; experiment) | VE | up to **+6 %** at the optimum length |
| Two-step runner length with two valve timings (ScienceDirect S187661021931210X; simulation with test) | VE across the speed range | **+5.4 %** on average; +1.4 % from two lengths alone (another study) |
| Single-cylinder CFR, variable intake length (Colorado State thesis; 1-D model) | VE | about +10 % at low speed against a fixed length |
| 4-2-1 against 4-1 headers, the same engine (Racefix, a maker's bolt-on test; forum dynos) | torque | the 4-2-1 filled a 4-1's mid-range dip by 25 N m at the same peak; forum peaks +8 to +16 % (bases unstated) |
| Winterbone and co. (Int. J. Mech. Sci. 1992); a 1-D exhaust pipe study | pressure traces, predicted against measured | no simple relation between the friction and heat-transfer factors; a straight pipe's exhaust pressure error not cured by the discharge coefficient |

The bound, for the intake: a tuned runner's VE gain at its tune of **6 to 9 %** over an untuned one at the same speed
(the two experimental rows; the simulations at or under 10 %). The model gives 12 to 18 % (the flat six, 0.53 against
0.30 m), so its resonance stands about twice as tall. For the exhaust's reversion dip no measured depth was found (only
makers' and forum dynos): no bound yet.

What the model should then show for the same hardware: the flat six's 0.53 m runner some 6 to 9 % over its 0.30 m one at
4750 rpm (it shows 17.7 %). As a damping: the ram's height goes near the resonance's Q, so the ringing that loses about
10 % a period after the valve shuts (N3-6) should lose some 20 to 25 %.

Where that loss may physically be (for G and N1 to size, each with its source): (1) the friction of an oscillating flow
exceeds the quasi-steady friction the duct takes from the mean speed (the unsteady-friction models of water hammer:
Zielke, Brunone, Vardy and Brown); (2) the port's and the valve seat's losses per pass while the valve is open; (3) the
heat exchanged in the oscillating thermal layer beyond Kirchhoff's (the wall's temperature swing); (4) for the exhaust,
the junctions' loss and the radiation of the open end. Not to be sized to our own targets: the 6 to 9 % is the target,
the archetypes' curves then are the result.

*G on the exhaust's side of N3-15:* no measured trace or VE curve held with its numbers. SAE 2001-01-1797 (Blair et al.,
"Exhaust tuning on a four-stroke engine; experimentation and simulation": measured exhaust pressure diagrams and power
on a YZ400 with three exhaust ends) is the right kind of data; only its abstract was reachable. The model's exhaust
damping today: the junctions' K 0.3 to 0.6 a merge (judgement but the Borda area changes), the open end's radiation and
jet loss, Kirchhoff's quasi-steady wall shelves and a mean-flow friction ξ 0.025; no unsteady friction beyond
Kirchhoff's √f. G will measure the reflected pulse at the port against the junction K and the end loss once a bound
exists. Open until that paper's full text (or another measured trace) is found.

## B-3. The turbo four's low end: matching or model (the lead's 23:10 request; measured 22:52 to 23:00 by `date -u`)

turbo-four-20-gdi on HEAD (G's housing mass closing 15fcdd84 in), the probe on the tip, 4.5 s settle, 2 s window, 96 kHz.
No design-share field exists (machine.fm: nDesign = 0.85 x redRpm; asked N2). The turbine scaled as the rule would:
the nozzle `aNoz x share / 0.85` through `boost.turbAR` (rc = 1.2 x rWheel = 0.0322 m; A/R 0.198 / 0.264 / 0.330 /
0.561 in), the housing's volume following the nozzle as the rule's; the compressor and the gate left as matched. The
energy terms are running sums added to a scratch copy of sim/boost (nothing committed there): the nozzle's isentropic
power m cp T3 (1 - r^(1/4)) per scroll per sample, the enthalpy the pipes bring into the housings, the gas's
mass-weighted T3 and expansion ratio. "etaT" is the turbine's work over that isentropic power; etaC the compressor's
time mean. Residual: trapped burned share at intake closing.

**As built (the phaser on N1's rule)**

| share | rpm | N m | boost kPa | shaft krpm | T3 mass K (time) | p3 kPa | ER mass | etaT | etaC | gate | residual | knock ret. |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0.30 | 1500 | 128 | 14.8 | 54 | 944 (620) | 117 | 1.35 | 0.57 | 0.40 | 0 | 0.173 | 9.9 |
| 0.30 | 1800 | 116 | 20.7 | 63 | 1000 (646) | 121 | 1.46 | 0.58 | 0.42 | 0 | 0.220 | 13.7 |
| 0.30 | 2500 | 153 | 95.0 | 126 | 1168 (758) | 212 | 3.66 | 0.60 | 0.48 | 0 | 0.213 | 15.0 |
| 0.30 | 4000 | 282 | 120.1 | 131 | 1313 | 293 | 3.91 | 0.58 | 0.71 | 0.30 | 0.088 | 8.1 |
| 0.30 | 6000 | 210 | 85.1 | 118 | 1356 | 282 | 3.69 | 0.55 | 0.75 | 0.42 | 0.086 | 0 |
| 0.40 | 1500 | 134 | 10.9 | 47 | 921 | 112 | 1.22 | 0.59 | 0.38 | 0 | 0.155 | 6.7 |
| 0.40 | 1800 | 134 | 17.1 | 58 | 974 | 116 | 1.32 | 0.60 | 0.42 | 0 | 0.184 | 9.1 |
| 0.40 | 2500 | 198 | 94.4 | 125 | 1168 | 178 | 3.01 | 0.62 | 0.51 | 0 | 0.165 | 15.0 |
| 0.40 | 4000 | 313 | 119.9 | 131 | 1314 | 236 | 3.30 | 0.60 | 0.72 | 0.33 | 0.073 | 8.3 |
| 0.40 | 6000 | 235 | 84.9 | 118 | 1337 | 235 | 3.08 | 0.57 | 0.75 | 0.47 | 0.072 | 0 |
| 0.50 | 1500 | 138 | 9.0 | 43 | 918 | 110 | 1.17 | 0.59 | 0.37 | 0 | 0.149 | 4.5 |
| 0.50 | 1800 | 142 | 13.7 | 52 | 957 | 113 | 1.23 | 0.61 | 0.41 | 0 | 0.165 | 6.5 |
| 0.50 | 2500 | 229 | 89.1 | 122 | 1155 | 161 | 2.46 | 0.64 | 0.52 | 0 | 0.131 | 14.9 |
| 0.50 | 4000 | 333 | 119.5 | 131 | 1306 | 209 | 2.79 | 0.63 | 0.72 | 0.37 | 0.063 | 7.9 |
| 0.50 | 6000 | 251 | 85.7 | 119 | 1317 | 209 | 2.65 | 0.60 | 0.76 | 0.56 | 0.063 | 0 |
| 0.85 | 1500 | 137 | 4.1 | 30 | 874 | 106 | 1.09 | 0.52 | 0.35 | 0 | 0.143 | 1.7 |
| 0.85 | 1800 | 140 | 5.2 | 33 | 900 (603) | 107 | 1.10 | 0.56 | 0.36 | 0 | 0.152 | 1.1 |
| 0.85 | 2500 | 235 | 37.9 | 84 | 1068 | 123 | 1.37 | 0.68 | 0.52 | 0 | 0.076 | 3.4 |
| 0.85 | 4000 | 366 | 120.2 | 132 | 1219 | 169 | 2.09 | 0.68 | 0.73 | 0.33 | 0.047 | 6.6 |
| 0.85 | 6000 | 279 | 84.8 | 120 | 1280 | 172 | 1.96 | 0.66 | 0.76 | 0.56 | 0.047 | 0 |

A smaller turbine raises the boost at 1800 only from 5 to 21 kPa and *loses* torque (140 to 116 N m): the residual
rises 0.15 to 0.22, the charge at intake closing 443 to 512 K, the knock retard 1 to 14 degrees (spark 1 BTDC).

**The cause: the intake phaser's advance under boost.** At 1800 N1's scheduleIntake (valvetrain/phaser.fm: the closing
asked 20 degrees after bottom at a quarter of the redline, load 1) advances this cam 48 degrees: the intake opens 38
before top against the exhaust's closing 6 after, 44 degrees of seat overlap, while the turbine's pressure stands at
the plenum's (121 against 122 kPa time mean on the 0.30): the overlap fills the cylinder with exhaust. The rule was
fitted on three NA engines (its comment); nothing in it reads the pressure across the engine. Second measurement, the
phaser parked (`valve.inPhaseRange=0`), one change:

| share | rpm | N m (as built) | boost kPa | residual | knock ret. | etaT | etaC |
|---|---|---|---|---|---|---|---|
| 0.30 | 1500 | 168 (128) | 23.4 | 0.063 | 0 | 0.58 | 0.44 |
| 0.30 | 1800 | 203 (116) | 58.4 | 0.075 | 3.3 | 0.58 | 0.48 |
| 0.30 | 2500 | 246 (153) | 117.9 | 0.087 | 10.1 | 0.60 | 0.53 |
| 0.30 | 4000 | 276 (282) | 120.2 | 0.079 | 3.9 | 0.59 | 0.70 |
| 0.40 | 1800 | 185 (134) | 30.5 | 0.061 | 0 | 0.61 | 0.46 |
| 0.40 | 2500 | 274 (198) | 118.0 | 0.072 | 9.6 | 0.63 | 0.53 |
| 0.50 | 1800 | 174 (142) | 20.2 | 0.057 | 0 | 0.62 | 0.45 |
| 0.50 | 2500 | 291 (229) | 117.6 | 0.064 | 9.1 | 0.65 | 0.53 |
| 0.85 | 1500 | 150 (137) | 4.9 | 0.054 | 0 | 0.53 | 0.34 |
| 0.85 | 1800 | 160 (140) | 8.2 | 0.055 | 0 | 0.60 | 0.40 |
| 0.85 | 2500 | 203 (235) | 29.1 | 0.051 | 0 | 0.67 | 0.50 |
| 0.85 | 4000 | 339 (366) | 121.3 | 0.046 | 4.8 | 0.68 | 0.69 |

Parked, the 1800 point gains 14 % on the matched turbine and 76 % on the 0.30; at 4000 the phaser's advance is worth
7 to 8 % (it helps there). Real engines run this overlap at low speed for scavenging only where the intake's pressure
beats the turbine's through the overlap (twin scroll, pulse separation); here the time means are equal and the overlap
back-fills. Owner: N1 (the rule), told with this table; whether the model's twin scroll gives a low-pressure window at
each cylinder's overlap (the scavenging) is the second half, unmeasured yet (G's pipes, N2's housings).

**The energy book at 1800, 0.30 turbine** (kW; enthalpy above 298 K; 2 s):

| | as built | phaser parked |
|---|---|---|
| fuel (LHV) | 81.3 | 138.1 |
| exhaust out of the valves (the probe's h_out - h_in, the cylinder's own cp) | 29.5 | 51.9 |
| into the turbine's housings (the pipes' q p cp T / R, cp 1148) | 17.5 (T 1003 K) | 33.6 (T 1075 K) |
| through the nozzle, isentropic (what the turbine could take) | 2.26 | 9.89 |
| the turbine takes | 1.31 (0.58) | 5.74 (0.58) |
| the compressor takes | 1.22 (etaC 0.42, PR 1.21, 23 g/s) | 5.57 (etaC 0.48, PR 1.59, 39 g/s) |

The class's boost, about 100 kPa at 1800: some 50 g/s of air at PR about 2.05; isentropic 68 kJ/kg, so 3.4 kW, 4.7 kW
at etaC 0.72 and 7.1 kW at the model's 0.48 here. At etaT 0.58 the turbine must then be offered 8.2 or 12.2 kW
isentropic. The parked 0.30 already offers 9.9 kW at 39 g/s (ER 2.6 mass-weighted): a compressor at a matched map's
efficiency would carry it to about the class's boost; the model's does not.

**What is missing, with its size** (each a measurement, none a tuning):
1. *The phaser under boost* (above): 14 to 76 % of torque at 1800, by turbine. N1's.
2. *The compressor's efficiency at low tip speed*: 0.36 to 0.48 below 2500 against 0.69 to 0.76 at 4000 and over. The
   island's Mach term (0.25 (M - 1.1)^2: judgement, compressor.fm) costs 0.10 to 0.16 at these tip speeds. At 0.72 the
   compressor's need at 1800 falls by a third. Sensitivity (compEta 0.85, one change, 0.30 as built): 20.7 to 25.4 kPa.
   A cited low-speed map is wanted (N2, P).
3. *The turbine's efficiency at low flow*: 0.52 to 0.62 mass-weighted below 2500 (etaMax 0.72; the blade speed ratio off
   its 0.7). turbEta 0.85 (one change, 0.30 as built): 20.7 to 27.0 kPa. Not the first-order cause.
4. *The exhaust's heat before the turbine*: 41 % (as built) and 35 % (parked) of the valves' exhaust enthalpy is gone
   at the housings (out of the valves about 1480 K by enthalpy over mass, an estimate on a mean cp; into the housings 1003 to 1075 K mass-weighted). The port's cooling is about a third
   of it (`exhaust.portHeat=0`: 17.5 to 21.5 kW, boost 20.7 to 25.3 kPa). No cited low-speed figure for a cast or
   integrated manifold: G's walls, open.
5. *Pulse energy*: present in the model (ER mass-weighted 2.6 against 1.5 on time means, parked 0.30); a cited pulse
   factor not found, so not sized against one. Twin scroll is on in this build.
6. *Low-speed scavenging*: absent (p3 = p2 on time means, residual 0.15 to 0.22 with the overlap): item 1's second half.

**Boost from a step** (throttle 0.15 held 3 s at the speed, then 1; time to 50 and 90 % of the last second's boost):

| | 1800: from / to kPa, t50 / t90 s | 2500 |
|---|---|---|
| 0.30 | 19 / 21, at once | 29 / 90, 0.07 / 0.55 |
| 0.40 | 12 / 18, 0.03 | 18 / 94, 0.30 / 0.79 |
| 0.50 | 8 / 14, 0.03 | 11 / 89, 0.42 / 1.17 |
| 0.85 | 3 / 5, 0.03 | 6 / 42, 0.42 / 1.08 |
| 0.30 parked | 20 / 56, 0.11 / 0.89 | 23 / 118, 0.36 / 0.77 |
| 0.85 parked | 3 / 9, 0.03 | 6 / 30, 0.27 / 0.68 |

Where boost comes at all it comes within a second: the low end is the settled balance, not the spool's lag.

**Conclusion.** Both. The first-order cause is the model's: the intake phaser's rule advances the cam into an overlap
the exhaust's pressure back-fills (N1). With it parked, matching matters as expected (5 to 58 kPa at 1800 from the
matched turbine to the 0.30), and the rest of the gap at 1800 is the compressor's low-speed efficiency (model,
judgement), then the turbine's, then the exhaust's heat. A smaller design point alone (P's cited hardware) does not
reach the class's figure on this model while items 1 and 2 stand.

**B-3, measured again on N2's field** (23:06 by `date -u`): `spec.boost.designShare` (N2's, pushed): it sizes the whole
match at that share of the redline (the compressor, the turbine's nozzle and the gate), not the nozzle alone as the
turbAR stand-in above did. Same probe and settle; as built, and the phaser parked (`valve.inPhaseRange=0`). The cited
curve (P's rows, the EA888 gen 3 in the Mk7 GTI): 333 to 368 N m at 1500 / 1800 / 2500 / 4000, 237 to 262 at 6200.

| share | cams | 1500 | 1800 | 2500 | 4000 | 6000 | 6200 (N m; boost kPa) |
|---|---|---|---|---|---|---|---|
| 0.30 | as built | 194; 83 | 215; 120 | 261; 120 | 211; 48 | 112; 11 | 105; 3 |
| 0.30 | parked | 277; 115 | 292; 122 | 296; 118 | 212; 55 | 112; 12 | 105; 3 |
| 0.40 | as built | 164; 33 | 181; 53 | 294; 120 | 289; 96 | 173; 42 | 164; 34 |
| 0.40 | parked | 201; 40 | 293; 114 | 309; 119 | 286; 109 | 173; 44 | 164; 36 |
| 0.50 | as built | 152; 17 | 165; 27 | 317; 120 | 328; 120 | 230; 72 | 220; 64 |
| 0.50 | parked | 169; 19 | 198; 35 | 319; 120 | 316; 121 | 230; 74 | 220; 66 |
| 0.85 | as built | 137; 5 | 142; 6 | 260; 53 | 366; 120 | 278; 85 | 276; 81 |
| 0.85 | parked | 152; 6 | 162; 9 | 215; 36 | 341; 120 | 273; 85 | 271; 81 |

Compressor efficiency (time mean) at 1800: 0.67 / 0.56 / 0.51 / 0.36 at 0.30 / 0.40 / 0.50 / 0.85 as built.
Knock retard at 1800: 15.0 / 14.9 / 9.6 / 1.1 degrees as built, 13.5 / 12.5 / 0 / 0 parked. At the top (0.30, 0.40)
the gate is 0.94 to 0.97 open, the shaft at its limit speed (omegaMax: 317 and 274 krpm) and the compressor's
efficiency 0.07 to 0.40: choked. Turbine efficiency mass-weighted 0.64 to 0.72 throughout on the field's match.

What this changes in B-3's ranking:
1. *The phaser* stands first: on the field's 0.40 it is 181 against 293 N m at 1800 (+62 %); at 0.30, 215 against 292.
2. *The compressor's low-speed efficiency* was mostly matching, not the island: a compressor sized at 0.85 of the
   redline sits far left of its island at 1800 (0.36); sized at 0.30 to 0.40 it runs 0.56 to 0.67 there. The Mach
   term still costs some (0.67 at the 0.30's 223 krpm against a 0.76 peak); a cited low-speed map would settle the rest.
3. *Knock*, newly first-order once the boost arrives: parked, 0.40, 1800: plenum 215 kPa, charge at closing 416 K,
   residual 0.052, lambda 0.82, knock retard 12.5 degrees, spark at about TDC; 293 N m against 333 to 368. C's table
   (fuel "Premium (91)", 9.6:1), with the real engine's direct injection's charge cooling and its scavenged residual
   as the questions. Not sized by me.
4. *The two ends do not meet on one match*: 0.30 to 0.40 hold the low end and choke at the top (105 to 164 N m at
   6200, against 237 to 262); 0.50 is 220 at 6200 but 165 to 198 at 1800; 0.85 is 276 at 6200 (over) and 142 at 1800.
   The real IS20 does both from one wheel: a map wider than the model's family gives (a mixed-flow turbine; the
   compressor's width only through its A/R term, judgement), or a smaller wheel than the 0.85 match with the boost
   map's trim at the top. N2 and P.
5. *The top's excess on the matched turbo* (0.85, 276 N m at 6200 against 237 to 262): the boost map (P's: 120 kPa to
   4500, 85 at 6000, 75 at 6500) is held exactly (81 kPa at 6200, gate 0.61); the torque goes as the charge's density,
   so the cited 250 asks about 60 to 65 kPa at 6200 on this model. No cited GTI boost exists (the S3's "up to 1.2 bar"
   is the larger IS38): content, P's, with that figure as what the model would need, not a target to tune to.
6. *The exhaust's heat before the turbine* (G's measurement, the lead's 23:09): at 1800 (147 N m, 9 kPa) the valves'
   30.8 kW reach the housing as 22.0, 29 % lost (port 16 %, primaries 10 %, collector 4 %); at 4000 13 %. The port's
   drop (104 to 138 K) sits inside the 1976 SAE bound of about 140 K; the softest number is the port wall's 345 K
   (450 K would return about 3 % of the valves' enthalpy). The right order, not bounded closer: third or later.
   *My 35 to 41 % against G's 29 %* is accounting: mine sets the probe's valve enthalpy (the cylinder's own cp(T),
   above the intake's backflow) against the boost machine's housing inflow at its constant cp 1148 J/kg K above 298 K;
   a constant cp below the gas's true one at 1000 to 1450 K understates the housing's side. G's sections are on one
   basis; take G's.

The real engine's features (the lead's list), and the model:
- *Channels kept separate to the turbine* (twin channels, mono-scroll wheel): modelled (scrolls 2: two housings into
  one wheel, each pulse's work taken sample by sample) and on in this build.
- *The manifold integrated in the head* (short, small, water-cooled): in part. Short primaries (0.61 of the layout's
  length) and the port's water-cooled walls (Caton and Heywood) are in; no water-cooled manifold beyond the port.
- *Exhaust valve lift switching* (Audi's AVS, a short low-speed exhaust lobe that keeps the cylinders' pulses apart):
  the model has two-stage cams (valve.stages, a second profile above a speed); this build does not use them.
- *Both cams phased for scavenging*: the model has an exhaust phaser (valve.exPhaseRange); this build has it at 0. The
  intake's rule reads no pressure across the engine (item 1), so nothing schedules scavenging.
- *An electric gate held shut until the target*: equivalent. The model's gate is pneumatic with a duty, and it reads
  0 below the target at every low-speed point measured.

**B-3, the knock limit sized** (the lead's 23:13; measured by 23:11 `date -u`). Base: designShare 0.40, the phaser parked,
1800 rpm full load: 293 N m at 114 kPa (plenum 215), charge at closing 416 K, residual 0.052, lambda 0.82, knock
retard 12.5 degrees, spark about TDC; the cited 333 to 368 N m. One change a run:

| change | N m | boost kPa | residual | T_ivc K | knock ret. | spark |
|---|---|---|---|---|---|---|
| base | 293.1 | 114.3 | 0.052 | 416 | 12.5 | -0.4 |
| air +10 K (env 298 K) | 287.7 | 117.5 | 0.052 | 424 | 12.7 | -1.0 |
| lambda 0.87 (map) | 299.6 | 113.5 | 0.050 | 416 | 13.8 | -2.2 |
| lambda 0.92 | 301.6 | 114.5 | 0.050 | 416 | 14.1 | -2.1 |
| lambda 1.00 | 282.0 | 112.5 | 0.052 | 416 | 13.6 | -0.9 |
| phaser to 10 degrees | 307.8 | 118.4 | 0.055 | 409 | 14.6 | -2.8 |
| phaser to 20 | 308.5 | 122.0 | 0.067 | 411 | 14.8 | -3.4 |
| phaser to 30 | 292.5 | 119.4 | 0.090 | 422 | 15.0 | -3.6 |
| phaser to 40 | 232.3 | 93.8 | 0.133 | 450 | 15.0 | -3.9 |
| RON 93 (grade 1) | 290.3 | 115.9 | 0.051 | 416 | 14.3 | -2.3 |
| RON 104 (grade 3) | 300.9 | 114.8 | 0.052 | 415 | 9.0 | 3.4 |
| compression 10.0 | 298.5 | 119.7 | 0.050 | 415 | 13.5 | -1.4 |
| knock control off | 307.4 | 116.4 | 0.054 | 415 | 0 | 11.3 |
| knock off, the phaser on N1's rule (49 degrees) | 189.4 | 48.8 | 0.160 | 451 | 0 | 11.9 |

The knock model (combustion/knock.fm: Livengood and Wu over Douaud and Eyzat's delay) reads the end gas's pressure and
temperature (T_ivc less the fuel's evaporation, then isentropic) and the octane index (RON: "Premium (91)" is RON 96,
MON 86, K = 0). The residual and lambda have no term of their own in the delay: the residual acts through T_ivc, lambda
through the evaporation's cooling. The controller trades spark for knock (a step on each knock, 15 degrees at most,
2 degrees a second back), so torque moves far less than the retard:
- *per 10 K at intake closing*: about +0.3 degrees and -7 N m (the air +10 K run: +8 K at closing; the boost moved too);
- *per 0.01 of residual*: through T_ivc only; mixing 0.01 of exhaust at about 900 K into a 416 K charge is about
  +5 K, so about +0.15 degrees and -3 to -4 N m on the rule above (the phaser runs move the closing too, so they
  cannot separate it);
- *per 0.05 of lambda*: +6 to +8 N m going 0.82 to 0.87 to 0.92 (the burn nearer its best), then -20 N m from 0.92 to
  1.00; the retard moves less than a degree either way;
- *per RON*: about 0.5 degrees and 1 N m (93 to 104: 14.3 to 9.0 degrees, 290 to 301 N m);
- *the knock's whole cost here*: 14 N m (293 to 307 with the control off).

Against cited values:
- *Direct injection's charge cooling*: Kasseris and Heywood (SAE 2012-01-1275, abstract) measured 14 K for gasoline
  in a turbocharged DI engine (49 K for E85), about 70 to 80 % of the theoretical (second-hand). The model takes
  0.85 of the full evaporation in the cylinder: 28 K at lambda 0.82, 23 K at 1.0. So the model's is twice the measured:
  at the cited 14 K the charge would be about 14 K hotter, about -10 N m. Generous already; it does not close the gap.
- *Lambda at 1800 full load*: the EA888 gen 3 targets lambda 1 over the whole envelope (EcuTek's tuning notes: a
  tuner's guide, not the maker's; VW's Evo4 release says the engine needs no enrichment at high load). The model's
  map formula runs 0.82 at this boost. At 1.0 the model loses 11 N m (282). Unknown from the maker; if cited, it widens
  the gap.
- *Compression and octane*: the build's 9.6:1 is the real engine's (Audi 920243). The real fuel: 95 RON in Europe
  (98 recommended for the Performance's rating, not confirmed by me), US 91 AKI (about 95 to 96 RON). The build's RON 96
  matches. Not a lever here.
- *Residual with scavenging*: a scavenged engine traps nearly none at low speed (no cited figure for this engine found);
  0.052 to 0.02 is about -15 K at closing on the mixing rule, about +10 N m.

**None closes 293 to 350, nor do they together** (at cited values they net about -10 N m: the DI cooling and lambda 1
take, the scavenged residual gives). The gap is air: the fresh charge trapped is 1.82 g per litre per cycle, about
0.77 of the plenum's density (215 kPa, about 2.38 g/L after the cooler); 350 N m at the same efficiency needs about
2.08, about 0.87 of it. The parked cam closes the intake 70 degrees after bottom (pushing charge back at 1800); the
rule's 49 degrees closes it early but its overlap back-fills (fresh 1.14, residual 0.16). Ten to twenty degrees is the
model's best (+5 %). What the real engine has to trap more at an early closing is scavenging through the overlap: the
pressure across the cylinder, item 1, N1's. Knock is the third order here, not the closer.

**Item 5, the top's boost on one hardware set** (6200 rpm, the boost map's 6000 and 6500 rows set for the run):

| match | target kPa | boost kPa | N m | p3 kPa | gate |
|---|---|---|---|---|---|
| 0.85 | 55 | 58.1 | 246 | 159 | 0.64 |
| 0.85 | 62 | 62.1 | 252 | 161 | 0.67 |
| 0.85 | 70 | 69.6 | 262 | 165 | 0.63 |
| 0.85 | 80 (as built) | 80.6 | 276 | 172 | 0.61 |
| 0.50 | 62 or 80 | 63.8 | 220 | 189 | 0.89 |
| 0.40 | 80 (as built) | 34.1 | 164 | 185 | 0.94 |

On the matched turbo the cited 250 N m is 62 kPa at 6200 (p3 161 kPa, gate 0.67). The 0.50 and 0.40 cannot get
there with any taper: their shafts are at the speed limit (omegaMax, 245 and 274 krpm; the gate opened by it, not by
the target), the compressors choked (efficiency 0.54 and 0.33), and the turbine's pressure 28 kPa higher than the
matched one's at the same boost: 220 N m at 64 kPa on the 0.50. On this model's map family no one set holds both ends:
the low end wants 0.30 to 0.40, the top 0.5 or more. For N2 (the map's width, the tip-speed limit of 560 m/s:
judgement) and P (the boost taper: 62 kPa at 6200 on the matched turbo).

**B-3, P's sources tried in the model** (23:16 `date -u`; P's docs/measurements/validation/2026-11-24-2314-ea888-gen3-sources.md).
(a) *The AVS small exhaust lobe* (gen 2's, SSP 922903: 6.35 mm, 180 degrees, closing 2 degrees after TDC, below
about 3100 rpm; whether the 180 is at the seat or at 1 mm is not stated): set in place of the build's exhaust cam
(exLift 6.35 mm, centre 88 degrees before TDC), designShare 0.40, 1800 rpm, one run each:

| exhaust cam, intake | N m | boost kPa | residual | fresh g/L/cycle | T_ivc K | knock ret. | p3 kPa | gate |
|---|---|---|---|---|---|---|---|---|
| the build's (236 seat), N1's rule (49 degrees) | 181 | 53 | 0.157 | | 462 | 14.9 | 128 | 0 |
| 180 at the seat (150 at 1 mm), N1's rule (48) | **316** | 119.7 | 0.073 | 2.065 | 399 | 15.0 | 154 | 0.24 |
| the same, knock control off | 317 | 119.9 | 0.100 | 1.953 | 410 | 0 | 155 | 0.15 |
| 180 at 1 mm (212 seat), N1's rule (49) | 225 | 98.7 | 0.157 | 1.473 | 465 | 15.0 | 147 | 0 |
| 180 at 1 mm, phaser parked | 292 | 118.8 | 0.058 | 1.814 | 423 | 13.1 | 155 | 0.10 |
| 180 at 1 mm, phaser at 20 | 302 | 118.1 | 0.069 | 1.925 | 415 | 14.8 | 155 | 0.18 |

The short lobe at the seat reading scavenges in the model: with the rule's 48 degrees of intake advance the fresh charge
reaches 2.07 g per litre per cycle (the 0.87 of the plenum that 350 N m needs), the boost reaches the target and the
gate opens. 316 N m against 333 to 368; knock at its cap. The lobe read at 1 mm (32 degrees longer at the seat) does
not: it closes the exhaust 16 degrees later, inside the intake's opening. So item 1 has a cited hardware answer the model
already holds (valve.stages: a second lobe by speed) once the lobe's seat duration is known: P's (the build's exhaust
cam), with N1's rule left as is for this engine. Scavenging is not missing from the model; the build's long exhaust
lobe was.

(b) *The top at the real boost*: an owner's stock VCDS log (P: carworklog.com, a transient pull, enthusiast) reads
81 kPa at 6048 rpm and 76 at 6400: the build's held 81 kPa at 6200 is the real boost, so 276 against 237 to 262 N m is
the model's at that boost, not a taper. At 6200, 81 kPa held, by the turbine's size (the turbine's nozzle by turbAR as
the match's at that share; the compressor and gate the 0.85 match's):

| turbine | N m | p3 kPa | gate | residual | etaC |
|---|---|---|---|---|---|
| 0.85 (as built) | 276 | 172 | 0.61 | 0.046 | 0.76 |
| 0.40 | 233 | 238 | 0.57 | 0.071 | 0.76 |
| 0.30 | 207 | 287 | 0.47 | 0.085 | 0.75 |

The top's torque at the real boost is set by the turbine's back pressure; the cited 250 lies between the 0.85 and the
0.40 turbines (about 0.5 to 0.6 of the redline by the match's rule). With the field's whole match at 0.40 or 0.50 the
compressor chokes at the top (the shaft at its limit; with the limit lifted by 1.3 the 0.40 reaches 81 kPa at p3 407 kPa,
etaC 0.18, 101 N m; the 0.50 at p3 249, 0.35, 198 N m). The 0.85 compressor with a 0.40 turbine holds the top (233 at
6200, 313 at 4000) but not the low end (1800 with the short lobe 171 N m at 36 kPa, etaC 0.46; 1500 179 at 22): the
compressor that holds 6200 runs far left of its island at 1800. **The one conflict left is the compressor's map
width**: the low end wants the 0.40 match's wheel (0.67 at 1800), the top the 0.85's. The real IS20's maps are not
published (P). N2's: the model's family's width (A/R term and the island's shape, judgement) against a published
compressor map of a wheel this size.

The ranking after P's sources: the exhaust lobe (content, P; the model has the stage), then the compressor's map width
(N2), then the turbine's size by the cited top (P with N2), then knock (C: at its cap with the lobe in; costs about
nothing in torque at 1800 there, knock off 317). N1's rule stands on this engine with the short lobe.

**B-3, the lobe's reference lift settled, and both cams free** (the lead's 23:21; 23:21 by `date -u`).
*The reference lift.* P's page (SSP 922903, via P's sources file) gives the small lobe 6.35 mm, 180 degrees, closing 2
degrees after TDC, and the large 10 mm, 215 degrees, closing 8 before. The model's generator (valvetrain/cam.fm) takes
both the seat duration (0.15 mm) and the 1 mm duration and fits the ramp between them; its family spans, at the build's
exhaust fatness (0.5):

| lobe | if the figure is at 1 mm: seat | if it is at the seat: 1 mm |
|---|---|---|
| 10 mm, 215 | 234 to 263 (247 on the build's own ramp, k 1.74) | 176 to 198 (187) |
| 6.35 mm, 180 | 204 to 232 (215) | 140 to 159 (151) |

The family alone excludes neither, and the lead's check is not clean: the build's 236-degree seat lobe at 10 mm reads
206 at 1 mm, between the two. Two other things settle it. (1) The page's own events: 180 degrees closing 2 degrees after
TDC opens 2 degrees *before bottom* at its reference lift; at the seat that is an exhaust opening at bottom dead centre
(no blowdown before the stroke turns), not a lobe anyone runs; at 1 mm the seat opening is some 17 degrees earlier.
(2) Audi's practice: its stated timing figures are at 1 mm of lift (AudiWorld and Nefarious Motorsports forum posts,
enthusiast: "all Audi stated numbers are at 1mm valve lift"); P's agent is reading the SSP's own wording. **Taken: at
1 mm.** The small lobe is 180 at 1 mm, about 215 at the seat on the build's ramp (exLift 6.35 mm, exCentre 88): the
225 N m case of the table above under N1's rule. The 316 N m row stands as measured but on a lobe the page does not
describe.

*Both cams free, on that lobe* (the exhaust phaser given 40 degrees: the real range is not published; N1's automatic
schedule asks the exhaust nothing at full load, so the cams were fixed through the ECU's maps, camIn and camEx, one
value each run: a small bake). N m; boost kPa in brackets:

| match, rpm | N1's rule (in, ex) | in 0 | in 20 | in 35 | in 50 | best |
|---|---|---|---|---|---|---|
| 0.40, 1500 | 234 (80) (38, 0) | 193 / 155 / 145 | **253** / 194 / 188 | 247 / 225 / 195 | 229 / 194 / 180 | 253, in 20 ex 0 (85 kPa) |
| 0.40, 1800 | 222 (98) (49, 0) | 283 / 253 / 177 | 299 / 289 / 201 | **319** / 254 / 173 | 220 / 194 / 101 | 319, in 35 ex 0 (122 kPa) |
| 0.50, 1500 | 237 (44) (38, 0) | 162 / 142 / 137 | 204 / 187 / 189 | 236 / 214 / 208 | **242** / 230 / 208 | 242, in 50 ex 0 (47 kPa) |
| 0.50, 1800 | 264 (86) (49, 0) | 214 / 207 / 159 | 272 / 265 / 235 | **282** / 262 / 236 | 261 / 240 / 192 | 282, in 35 ex 0 (97 kPa) |

(each cell: exhaust phase 0 / 20 / 40.) The model's exhaust phaser retards (later events) and every retard costs at full
load here; an exhaust advance (earlier closing, the scavenging direction) is not in the model's phaser to try. The
optimum is narrow: 0.40 at 1800, intake 28 gives 302 and 35 gives 319, 50 gives 220. Knock costs nothing at the best
cells: with the control off, 306.5 (1800, in 35) and 249.7 (1500, in 20), against 319 and 253 with it (the retard's
later burn leaves the turbine more). The build's long lobe on the same match, its intake fixed (the phaser's range
capping N1's rule, earlier in B-3): best 309 at 1800 (in 10 to 20).

*A baked map* (N1's tool, the lead's ruling) on the best case: if the bake sweeps the intake with the stage that is
active at each speed (the short lobe below about 3100), it lands at about in 35 at 1800 and in 20 at 1500 on 0.40:
319 and 253 N m. Baked with the long lobe and run on the short, it would choose 10 to 20 at 1800 and get 299 (6 % less):
the tool must bake each stage on its own lobe. Its step needs to be finer than 15 degrees here.

**The ranking, the lobe at 1 mm** (0.40 match, against 333 to 368 N m):
1. *The intake's phase at boosted low speed*: N1's rule 222 / 181 (short / long lobe) against a bake's 319 / 309 at 1800:
   +44 to +71 %. N1's bake is the answer (ruled); the rule stays the fallback.
2. *The turbo's spool at 1500 and below* (N2): the 0.40 match makes 85 kPa at 1500 at its best phase, the 0.50 47;
   253 and 242 N m against the cited 333 and up. Matching and the compressor's map width as before.
3. *The short exhaust lobe* (P): +3 % at 1800 in the bake (309 to 319); helpful, not decisive, read at 1 mm.
4. *The exhaust phaser*: retard only in the model and it costs at full load; the real one's direction and range unknown.
5. *Knock*: costs nothing at the best cells (control off is no better).
At 1800 the best is 319 against 333 to 368 (-4 to -13 %); at 1500 253 (-24 %).

**B-3, the exhaust cam's phase on the ruled lobe** (the lead's 23:19 ruling: the small lobe 6.35 mm, 180 at 1 mm, about
212 at the seat, centre 88; measured 23:22 to 23:26 by `date -u`). The model's exhaust phaser retards only (phaser.fm:
the exhaust's later); the earlier closing the lead asks for was made by advancing the lobe's centre (exCentre 88 + 0 /
10 / 20 / 30, the phaser's range 0), the intake fixed through camIn at 0 / 15 / 30 / 45 / 60. "Share" is the fresh
charge trapped (g per litre per cycle) over the plenum's density (its mass over its volume, mean over the window).
Each cell: N m / share / residual / boost kPa, the columns the exhaust's advance 0 / 10 / 20 / 30.

0.40 match, 1800 rpm:

| intake | 0 | 10 | 20 | 30 |
|---|---|---|---|---|
| 0 | 292/0.78/0.058/119 | 301/0.80/0.050/117 | 300/0.80/0.048/116 | 300/0.80/0.053/119 |
| 15 | 303/0.82/0.065/120 | 316/0.85/0.055/120 | 325/0.87/0.049/120 | 319/0.87/0.050/117 |
| 30 | **332/0.88/0.059/125** | 310/0.85/0.071/116 | 328/0.88/0.062/120 | 331/0.89/0.059/121 |
| 45 | 231/0.70/0.149/101 | 319/0.86/0.078/120 | 315/0.85/0.085/119 | 316/0.86/0.082/120 |
| 60 | 217/0.68/0.154/108 | 221/0.70/0.156/90 | 233/0.70/0.154/100 | 290/0.80/0.113/120 |

0.40 match, 1500 rpm:

| intake | 0 | 10 | 20 | 30 |
|---|---|---|---|---|
| 0 | 194/0.76/0.064/39 | 258/0.78/0.053/85 | 237/0.79/0.051/64 | 197/0.78/0.057/39 |
| 15 | 254/0.81/0.070/83 | 286/0.83/0.060/104 | **303/0.86/0.053/112** | 289/0.86/0.052/101 |
| 30 | 268/0.79/0.095/102 | 290/0.81/0.089/118 | 302/0.85/0.073/116 | 301/0.88/0.063/108 |
| 45 | 242/0.79/0.105/83 | 253/0.79/0.109/91 | 260/0.81/0.100/91 | 253/0.83/0.089/78 |
| 60 | 224/0.79/0.107/64 | 234/0.77/0.122/82 | 232/0.78/0.118/74 | 229/0.80/0.110/67 |

0.50 match, 1800 rpm (best 320/0.89/0.058/110 at intake 30, exhaust +30); 1500 rpm (best 255/0.92/0.057/57 at
intake 45, exhaust +10; shares 0.84 to 0.92 from intake 15 up, the boost 27 to 57 kPa). Full grids in the scratch run
(80 points); the cells move about 10 N m between neighbours that should be close (0.40, 1800, intake 30: 332 / 310 /
328 / 331), so read the best to about that.

**The 0.87 share is reached**: at 1800 on both matches (0.88 to 0.89), at 1500 on the 0.40 (0.86 to 0.88) and on the 0.50
(0.92, with little boost). The exhaust's earlier closing is what lets the intake open early without the back-fill: at
intake 45 the 0.40 at 1800 goes from 231 N m (residual 0.149) at the lobe's own phase to 316 to 319 (0.08) with 10 to 30
degrees of advance, and at intake 60 from 217 to 290. The best cell needs little of it at intake 30 (the lobe alone
holds the residual near 0.06 there).

**But the share was not the whole of it** (my figure of 0.87 for 350 N m was wrong: it took the torque per gram from
the knock-free run, 169 N m per g/L at spark 11 BTDC; at these cells knock holds the retard at its cap, 14.3 to 15
degrees, and the torque per gram is 158). At the best cells:
- *1800, 0.40*: 332 N m at 2.106 g/L, share 0.88, boost 125 kPa (at its 120 target), knock at the cap: 1 to 21 N m
  short of 333 to 368 (0 to 6 %). 350 at this efficiency needs about 2.22 g/L, share 0.93 at this boost, or a 6 %
  denser plenum (about 135 kPa) or a 6 % better burn (the retard's cost: about 9 % at the cap if the knock-free torque
  per gram held, which it did not in the earlier knock-off runs, where the spark's return cost trapped mass).
- *1500, 0.40*: 303 N m, share 0.86, boost 112 (short of its 120): 30 to 65 N m short (9 to 18 %), of which the boost's
  8 kPa is about 3 % and the share's 0.01 to 0.02 about 2 %; the rest the same efficiency as at 1800.
- *the 0.50 at 1500*: the share is 0.92 but the boost 57 kPa: 255 N m, the turbo's spool (N2) alone.

So with the ruled lobe and an exhaust cam that closes earlier, the trapped share is no longer what is short. At 1800 the
gap is within the grid's noise of the cited curve's bottom; at 1500 the 0.40's boost (spool) and the knock-limited burn
are what remain. Owners: the exhaust phaser's direction (advance: N1, the valvetrain's; today it only retards), the
spool at 1500 (N2), the knock-limited efficiency at this charge (C's table, with the DI cooling note).

**B-3, N1's overlap pressure on both lobes** (23:28 by `date -u`; tel.valve.dpOverlap, 6d531f72; 0.40 match). Under
N1's rule (intake 38 to 49 degrees), 1500 / 1800 rpm: the build's long lobe -28.3 / -28.9 kPa (164 / 190 N m, residual
0.131 / 0.160); the ruled short lobe -8.5 / -15.9 kPa (214 / 233 N m, 0.086 / 0.142). The short lobe cuts the push-back
by a half to two thirds and does not turn it positive: the long lobe is a part of the fault, not all of it. With the
intake fixed through camIn the reading is 0.0 exactly in every run (it is taken over the overlap the rule asks: not a
reading at fixed phases). On this tree (merged at 23:27) the short lobe at intake 30 reads 309 N m at 1800 (332 in the
grid above, run before the merge; with the exhaust +30, 326 against 331): the branch moved between, or the cells
scatter by up to 7 %; read the grid's best to about 20 N m.

**B-3, the top at the cited boost** (23:30 `date -u`). turbo-four-20-gdi as built (0.85 match), 6200 rpm, 81 kPa held (the
real boost, P's VCDS log): 272 N m on this tree (276 earlier) against the cited 237 to 262 (250 for 162 kW). One change
each: lambda 0.84 (the map's formula) to 1.00 (EcuTek: the EA888 gen 3 at lambda 1 everywhere; a tuner's guide) 263
(-9 N m), to 0.90 278; the intercooler's effectiveness 0.75 to 0.60 266 (-6; no cited figure for the real one); the
turbine's back pressure, a 0.40 turbine (p3 238 against 172 kPa) 233 (-15 %, measured above). Friction: FMEP 222 kPa
(Chen-Flynn), pumping 130, gross IMEP 2081; no published FMEP at 6200 for this engine found, not a lever I can size
against a figure. No knock at 6200 (spark 20 BTDC). So the top's 9 % over is the turbine's back pressure first (the real
IS20 is a small turbine: P's wheel sizes, 41.8 / 54.0 mm, no map), then the enrichment if lambda 1 is confirmed (-3 %),
the cooler's effectiveness unknown (-2 % per 0.15).

### B-3 in one table (the turbo four against the EA888 gen 3: 333 to 368 N m at 1500 and 1800, 237 to 262 at 6200)

| cause | at 1500 | at 1800 | at 6200 | owner | sourced? |
|---|---|---|---|---|---|
| The intake phase at boosted low speed: N1's rule against a baked map | rule 164 / 234 against bake 253 (long / short lobe) | rule 181 / 222 against bake 309 / 319: +44 to +71 % | none (the rule helps above 4000) | N1 (the bake; the rule the fallback) | the rule judgement; the bake measured |
| The exhaust lobe and an exhaust that closes earlier (AVS small lobe; the phaser's parked end) | bake 253 to 303 with 20 of advance: +20 % | 309 to 325-332: +5 to +7 % (±20 N m) | none (the large lobe above 3100) | P (the lobe, SSP 922903, ruled at 1 mm; the parked end unknown) | lobe cited; the parked end not |
| The turbo's spool and the compressor's map width | 0.40 match 112 kPa of 120 (about 3 %); the 0.50 57 kPa | at target on the 0.40 | the 0.40 / 0.50 matches choke (164 / 220); the 0.85 holds | N2 | no compressor map near the IS20 found |
| The turbine's back pressure at the top | | | 0.85 turbine 272, 0.40 233 at 81 kPa: the cited 250 between | P with N2 | wheel sizes only |
| The knock-limited burn (retard at its 15 degree cap at the best cells) | about 10 % of what is left | 0 to 6 % left; knock control off is no better | no knock | C (the table), the DI cooling (twice Kasseris and Heywood's measured 14 K) to its owner | the DI cooling cited; the rest the model's |
| Lambda: the map's 0.82 to 0.84 against lambda 1 | | -11 N m at 1 | -9 N m at 1 | the ECU maps' owner | a tuner's guide only |
| The exhaust's heat before the turbine | | 29 % of the valves' enthalpy at 1800 (G's): third order | 13 % at 4000 | G | within a 1976 SAE bound |
| The intercooler | | | -2 % per 0.15 of effectiveness | N2 / P | no figure |
| Friction | | | FMEP 222 kPa | (none named) | no figure found |

Best the model reaches with cited or ruled hardware and a fixed-phase bake: about 303 at 1500 (-9 to -18 %), about
310 to 330 at 1800 (0 to -7 %), and 233 to 272 at 6200 by the turbine's size (the cited 250 inside).

**B-3 after the DI cooling's change** (the steward's 65d0fc35: 14 K at lambda 1; 23:32 by `date -u`). The table's best
cells again, the 0.40 match on the ruled lobe: 1800 (intake 30, exhaust +30) 330 N m at 123 kPa, knock at its cap
(331 before); 1500 (intake 15, exhaust +20) 300.5 at 113 kPa (303); 6200 as built 274 at 81.5 kPa (272). Within the
grid's scatter: the table stands. The cooling now shows as retard where the controller has room (the steward's tee-dyno
runs: 1800 as built 0 to 3.3 degrees), not as torque.

**B-3 on the build as P made it** (95f3f3f: the EA888 gen 3, two-stage exhaust, phasers 60 and 30, N1's baked intake maps;
N1's dpOverlap at fixed phases, 0ad99dde; 23:41 by `date -u`). The build as built (its 0.85 match), full load:

| rpm | N m | boost kPa | residual | knock ret. | intake / exhaust phase | dpOverlap kPa | gate |
|---|---|---|---|---|---|---|---|
| 1500 | 189 | 7.5 | 0.044 | 1.5 | 60 / 0 | -3.2 | 0 |
| 1800 | 218 | 17.4 | 0.027 | 0.8 | 60 / 0 | 0.1 | 0 |
| 2500 | 300 | 68.1 | 0.035 | 8.9 | 41 / 0 | 0.6 | 0 |
| 4000 | 351 | 120.9 | 0.065 | 10.0 | 13 / 0 | -72.8 | 0.28 |
| 6200 | 283 | 82.0 | 0.060 | 0 | 10 / 0 | -84.0 | 0.56 |

Against 333 to 368 / 237 to 262: the breathing at low speed is now clean (residual 3 to 4 %, the overlap's pressure about
0 at the baked phases); what is short at 1500 to 2500 is the 0.85 match's spool (7.5 and 17 kPa: item 3 of the table,
N2's). At 4000 the curve is met; 6200 is 283 (8 % over 262; the turbine's size, as before). The exhaust phaser sits at 0
at full load (the automatic exhaust schedule asks nothing there).

On the 0.40 match on this build (the short lobe is the build's now; intake through camIn):
intake 30: 233 N m at 1500 (64 kPa), 336 at 1800 (122); with the exhaust lobe 30 degrees earlier (exCentre 118):
327 at 1500 (128 kPa), 331 at 1800. dpOverlap at intake 30: -11.4 / -7.2 kPa; with the exhaust earlier -42.3 / -45.4,
while the torque rises at 1500 by 40 %: the reading, taken around the overlap's top, does not rank the exhaust axis (the
exhaust closing earlier shortens the overlap it reads over). A bake should rank by torque, not by it. (My "long lobe"
rows at this merge were on the build's short lobe: the lobe P put in; withdrawn as long-lobe readings.)

**N3-15, N1's reading of the intake's damping** (N1's message, 23:57 by `date -u`). Every sourced loss on the list is in
the model: the port's bend and the seat's loss through the lab's flow-bench discharge coefficients (Idelchik's bend on
top would count them twice); quasi-steady friction from the mean |u| at the ends (swing included); the unsteady friction
and the oscillating thermal layer as Kirchhoff's wall loss, within 2 to 6 % of the textbook coefficient (with turbulent
mean flow the measured damping tends to the quasi-steady value: Peters et al. 1993, N1 from memory). The flat six at
4750 rpm, air through the valves by runner length (my probe): 0.10 1.035, 0.20 1.057, 0.25 1.074, 0.30 1.057,
0.35 1.052, 0.40 1.110, 0.45 1.097, 0.53 1.241, 0.60 1.168, 0.70 1.252. The off-tune floor is flat at 1.04 to 1.07 from
0.10 to 0.35 (0.30 is no trough); the tuned 0.53 and 0.70 stand 16 to 19 % over it, against 6 to 9 % published. So the
excess is the resonance's height, not a missing loss: candidates the port's taper and area change (the model's runner one
bore to the valve), the flow's multidimensional end at the valve, and what the experiments counted as untuned. Mine if
the lead agrees; not started.

**N3-15, the bound's sources read for what they compared, and one reproduced** (the lead's 23:58; 00:02 `date -u`, 2026-11-25).
(1) What each source compared:
- *Harazi et al., Jurnal Teknologi 87:5 (2025) 875-886* (the "+8.7 %, 528 mm" row; ResearchGate 394849080), full text read:
  a **1-D simulation** (Lotus Engine Simulation), not an experiment, of the Proton CamPro 1.6 (76 x 88 mm, rod 180,
  10:1, intake throat 28 mm, IVO 12 BTDC / IVC 48 ABDC, EVO 45 BBDC / EVC 20 ATDC, exhaust lift 8 mm; 148 N m at 4000,
  82 kW at 6000), validated on the stock runner against Mohiuddin and Rahman's dyno VE (average error under 3 %). The
  comparison: the stock runner, 400 mm x 39 mm, against 528 mm (wave theory; 476 mm by Helmholtz); VE (trapped air over
  the swept volume at the inlet's 1 bar, 20 C), relative; "up to 8.17 % at mid-range rpm between 3500 and 4500 rpm and at
  6500" (body; the abstract says 8.71); the 400 mm better between 5500 and 6000. Two lengths near tune, *not tuned against
  untuned*. My table's "experiment" was wrong.
- *ResearchGate 287040569* (turbo diesel, variable length, +6 %), *ScienceDirect S187661021931210X* (two-step length and
  timing, +5.4 % average, +1.4 % from the lengths alone), *the Colorado State CFR thesis* (+10 %): not readable here
  (403 twice; the thesis not found). What each compared is unconfirmed; they stand as abstract-level figures only.
(2) The CamPro pair in the model: hot-hatch-16-16v given the paper's bore, stroke, rod, compression, cam centres and
durations (the paper's events taken as seat timings; 1 mm durations 30 degrees shorter: judgement), exhaust lift 8 mm,
runner bore 39 mm; intake lift, valve heads and plenum (1.28 L) the build's (not in the paper). VE (tel, over ambient),
400 against 528 mm:

| rpm | 1000 | 1500 | 2000 | 2500 | 3000 | 3500 | 4000 | 4500 | 5000 | 5500 | 6000 | 6500 |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 400 mm | 0.684 | 0.650 | 0.694 | 0.797 | 0.830 | 0.744 | 0.837 | 0.903 | 0.984 | 0.952 | 0.972 | 1.020 |
| 528 mm | 0.686 | 0.741 | 0.743 | 0.761 | 0.731 | 0.805 | 0.903 | 0.957 | 1.055 | 1.127 | 1.033 | 0.904 |
| gain | +0.2 | +14.1 | +7.2 | -4.6 | -11.9 | **+8.3** | **+7.9** | **+6.0** | +7.2 | +18.4 | +6.3 | -11.4 |

On the paper's own pair and band (3500 to 4500) the model reads **+6.0 to +8.3 % against the paper's 8.17 %**: by the
lead's test the flat six's 17.6 % (0.30 against 0.53 m, an off-tune floor against a tooth) was never comparable with this
source, and the item closes as far as this source goes. Three cautions, for the record and not reopening it:
(a) the source is itself a 1-D code: agreement with it bounds the model against Lotus, not against an engine; the only
measured anchor in it is the stock runner's VE. (b) Outside the band the two disagree in sign: at 5500 to 6000 the model
prefers 528 mm (+18 and +6 %), the paper 400 mm; at 2500 to 3000 the model's 528 mm dips 5 to 12 %. (c) The level: the
model's VE runs 0.95 to 1.13 from 5000 up on this NA four (torque 164 N m at 5000 with 400 mm against the real engine's
148 at 4000); the paper's grid study reads 0.89. The ram's *height* against a measured engine remains unbounded by any
source read here; the level question sits with the cam timing's reference lift (not stated in the paper) and the intake
lift (not given), so it is not sized.

## N3-16. The big engines' burned share at a warm idle, against a published residual correlation (00:39 `date -u`, 2026-11-25)

The lead's item (from N1's idle work: the V10's and V12's 1 Hz wander at a warm idle, and the 7.0's).
*The source.* Fox, Cheng and Heywood, "A Model for Predicting Residual Gas Fraction in Spark-Ignition Engines", SAE
931025 (1993): exists (SAE Mobilus), calibrated on in-cylinder hydrocarbon sampling at varied intake pressure, speed and
overlap, and it carries the overlap's backflow explicitly. Its equation as reproduced (eq. 2.2 and 2.3) in a University of
Wisconsin thesis, "Residual gas effects on combustion in an air-cooled utility engine" (read in full here):
  y_r = 1.266 (OF / N) (p_i / p_e)^-0.87 sqrt|p_e - p_i| + 0.632 (phi / r_c) (p_i / p_e)^-0.74,
  OF = [D_in * integral(L_in dtheta, IVO to the lifts' crossing) + D_ex * integral(L_ex dtheta, the crossing to EVC)] / V_d,
D the inner seat diameters, L the lifts, V_d the displaced volume; OF in degrees per metre (the thesis's table 4.2). The
units of N and p are not printed there: rpm and kPa taken from Heywood's text as I recall it (a check: p_i 35, p_e 100,
OF 1, 800 rpm gives 0.17, an ordinary idle residual). Fox's data spanned 0.52 < OF < 1.43; the thesis found the
correlation over-predicting well above that (its production cam OF 3.66). The same thesis measured y_r up to about 0.4
on that small engine at large overlap (OF 27). Heywood's own measured residuals against overlap: not read here (N1's
0.15 to 0.25 stays a recollection).
*The definitions.* Fox's y_r is the burned-gas mass fraction of the charge at intake closing, backflow included (the
sampling reads all burned gas in the cylinder). The model's share (my probe: the cylinder's mInert over m at each
intake closing) is the same quantity: the last cycle's products left in, what came back from the header, and what the
intake pushed back during the overlap and drew in again (cylinder.fm's mInert). Lined up; no definition gap.

*At the warm idle* (the probe's new bench mode: started as tee-dyno starts it, then the dyno released and the pedal at 0,
the ECU's idle control holding the speed; 4 s, then 2 s measured; lambda 1, the phasers parked). OF from each build's
own generated lift curves (cam.fm's family, at its fatness), D the valve heads (the inner seats some 10 % less: OF about
10 % high), the valves of one side summed:

| engine | rpm | p_i / p_e kPa | r_c | OF deg/m | Fox: backflow + clearance = y_r | model's share | at q 1.0 |
|---|---|---|---|---|---|---|---|
| v10-52 | 896 | 54.8 / 101.7 | 12.5 | 1.38 | 0.023 + 0.080 = **0.103** | **0.469** | 0.395 |
| v12-65 | 904 | 73.6 / 101.4 | 13.5 | 2.65 | 0.026 + 0.059 = **0.085** | **0.528** | 0.486 |
| big-bore-70-pushrod-v8 | 752 | 65.6 / 101.4 | 11.0 | 2.16 | 0.032 + 0.079 = **0.111** | **0.518** | 0.481 |
| economy-four-15 | 750 | 33.0 / 101.7 | 11.0 | 0.02 | 0.001 + 0.132 = **0.133** | **0.170** | |

(The V12 and the 7.0 sit above Fox's range, where it over-predicts: the published figure for them, if anything, lower.)
The four agrees within 0.04, its share nearly all clearance gas; the three big engines read 4 to 6 times the correlation.

*Where it lives.* One cylinder of the V10 traced over a cycle at idle (grams a cycle, by crank window):

| window (degrees, 360 = overlap TDC) | exhaust out | exhaust back in | intake drawn | intake pushed back |
|---|---|---|---|---|
| 0-300 (exhaust stroke) | 0.173 | 0.121 | | |
| 300-340 | 0.068 | 0 | | |
| **340-380 (the overlap)** | 0.007 | **0.054** | | **0.066** |
| 380-420 | | 0.002 | 0.051 | |
| 420-540 | | | 0.167 | 0.024 |
| 540-640 (after BDC, before the late closing) | | | 0.024 | 0.079 |

The burned gas in the cylinder: 0.020 g at the overlap's TDC (the clearance), 0.108 g at BDC, 0.079 of 0.156 g trapped
at closing (0.51). The overlap takes 0.054 g of exhaust back through the cylinder into the intake runner and the intake
draws it in again: against Fox's backflow term, 0.023 x 0.156 = 0.0036 g, about **15 times**. A plain orifice at the full
manifold difference (47 kPa) across the valves' curtains for the whole overlap (Cd 0.6, exhaust gas at 800 K) gives about
0.05 g: the model's backflow runs at that bound, where the measured correlation says a fifteenth of it. The late closing's
push-back (0.079 g at share 0.5) recirculates the share through the runner, which holds the level up once it is there.

*The three candidates the lead named:*
1. **The backflow through the overlap at a low manifold pressure** (D's cylinder, N1's valvetrain): this is where the gap
   is, 15 times the correlation's backflow. Measured part of it: area.fm's low-lift area is 0.73 of the curtain times
   the port's quality q, and q multiplies the low-lift (curtain-limited) part too: the V10 and V12 carry q 1.65 / 1.8
   (1.2 to 1.3 times the geometric curtain at low lift, more than any valve flows); the 7.0 and the four 1.35 / 1.5.
   At q 1.0 (one change) the shares fall 0.04 to 0.07: a part, not the gap. The rest: how the overlap's flow is driven
   (the full manifold difference seen across both valves in series, with the cylinder between at 92 kPa at TDC), unsized.
2. **The share's definition**: lined up with Fox's y_r; not the gap.
3. **The toll's threshold** (0.56, combustion.fm): with a share at the correlation's 0.09 to 0.11 these engines would
   sit far below it; the threshold is not where the gap is, though the wander lives on the slope near it.
Owners: D (the cylinder's overlap flow) and N1 (area.fm's low-lift area and q). The economy four's agreement is the
control: an engine with almost no lift at TDC reads the correlation.

**N3-16, the correlation's units as the source states them, and the overlap factors** (00:43 `date -u`; the lead's 00:42).
*Units.* The thesis prints eq. 2.2 without units beside it; its nomenclature lists "N Engine Speed", "P Pressure", "kPa
Kilo-Pascal", "RPM Revolutions Per Minute", every pressure in the text is in kPa and every speed in RPM, and its table 4.2
gives OF in "[°/m]". So: OF in degrees per metre, *stated*; N in RPM and p in kPa, the thesis's own convention but *not
printed at the equation*. The other readings, computed: N in revolutions a second makes the V10's backflow term 1.37 (a
residual over 1: impossible), so rpm; p in bar instead of kPa divides the backflow term by 10, the clearance term
unchanged:

| engine | OF deg/m | in Fox's 0.52 to 1.43? | Fox, kPa and rpm | Fox, bar and rpm | model |
|---|---|---|---|---|---|
| v10-52 | 1.38 | inside | 0.023 + 0.080 = 0.103 | 0.002 + 0.080 = 0.082 | 0.469 |
| v12-65 | 2.65 | beyond (over-predicts there: the thesis) | 0.026 + 0.059 = 0.085 | 0.003 + 0.059 = 0.062 | 0.528 |
| big-bore-70-pushrod-v8 | 2.16 | beyond | 0.032 + 0.079 = 0.111 | 0.003 + 0.079 = 0.082 | 0.518 |
| economy-four-15 | 0.02 | below | 0.001 + 0.132 = 0.133 | 0.000 + 0.132 = 0.132 | 0.170 |

Either way the gap stands (bar only widens it). *How OF was computed* (scratchpad fox.py, for D and N1): each lobe the
build's own generated curve (cam.fm's family: the build's seat and 1 mm durations, lift and centre, fatness 0.5, the
phasers parked as at idle), the overlap's angles from the intake's leaving zero lift to the exhaust's reaching it (the
curves' ends, not the 0.15 mm seat); the integral of the intake's lift from its opening to the lifts' crossing, and of the
exhaust's from the crossing to its closing, each times the valve's head diameter times its valves a cylinder (2 on the
four-valves, 1 on the 7.0), over one cylinder's swept volume. The heads stand in for the inner seat diameters, which are
some 10 % smaller: the OFs about 10 % high, and the backflow terms with them.

*Referee:* I hold the four idles (BAL_BENCH=1, 4 s settle, 2 s window) and will re-run them on each change D or N1 sends,
with the share, the plenum's pressure, the overlap's backflow by window (the V10's traced cylinder) and the speed.

**N3-16 corrected: the measured points, and what the fit cannot settle** (00:52 `date -u`; the lead's 00:50, D's overlap
measurement). D measured the overlap from inside the cylinder: the model's backflow runs at 0.4 to 0.6 of a quasi-steady
series orifice at the manifolds' difference, so it is limited by the cylinder between the valves, not at an unlimited
bound as I wrote. And the fit's units are not settled, so my "4 to 6 times" and "15 times" are **withdrawn**.

*The thesis's measured residuals* (its figure 4.6, HMS fuelling, production ignition; read off the plot, about +-0.01;
the intake pressures of its 10 % and 25 % loads are not tabulated; a small air-cooled single):

| cam (OF deg/m) | 1200 rpm, 10 % / 25 % | 1750 rpm, 10 % / 25 % | 3060 rpm, 10 % / 25 % |
|---|---|---|---|
| zero (0.11) | 0.23 / 0.18 | 0.21 / 0.16 | 0.14 / 0.13 |
| production (3.66) | 0.25 / 0.195 | 0.215 / 0.175 | 0.16 / 0.135 |
| intermediate (12.3) | 0.28 / 0.245 | 0.25 / 0.21 | 0.17 / 0.15 |
| max (27.3) | 0.345 / 0.315 | 0.33 / 0.30 | 0.23 / 0.18 |

*The fit against them.* The thesis's own evaluation of the correlation (its figure 4.9) gives 0.4 to 0.8 at the production
cam and 0.6 to 1.2 at the larger ones, where it measured 0.13 to 0.35. In kPa and rpm (my reading) the same points give
0.13 to 0.21 at the production cam (0.066 + about 0.15 at 1200 rpm and 10 % load, taking p_i 40 kPa and r_c 8.5, neither
stated), 0.37 at the intermediate and 0.64 at the max. So the thesis evaluated it at some 3 to 5 times my numbers: either
its units differ from mine (N in rad/s would be about 10 times) or its OF or p_i do. **The thesis does not settle the
units**, and my earlier line "the thesis's finding confirms kPa and rpm" was wrong. In my reading the fit lands near the
measured points at the production cam (0.13 to 0.21 against 0.135 to 0.25) and over them at large overlap, which is what
the thesis says of it; in the thesis's own reading it is over everywhere but the zero cam. Neither proves the units.
P's agent is after SAE 931025's own equation.

*What the nearest measured points imply.* The V10, V12 and 7.0 sit at OF 1.4 to 2.7 (between the thesis's zero and
production cams), 750 to 900 rpm (below its 1200), and 55 to 74 kPa in the plenum (above a 10 % load's). Lower speed
raises the measured residual (+0.03 to +0.04 from 1750 to 1200 at the production cam), the higher manifold pressure lowers
it (25 % against 10 % load: -0.04 to -0.06). By the nearest points, a measured residual of **about 0.2 to 0.27** for these
three; the model reads 0.47 to 0.53, about **twice** that, on a different engine (a small air-cooled single, its compression
not found). The economy four (OF 0.02, like the zero cam; 750 rpm, 33 kPa) against the zero cam's 0.21 to 0.23 at
1200 rpm and 10 %: the model's 0.170 sits inside the measured spread.

| engine | OF | model's share | fit, kPa and rpm (unsettled) | nearest measured (thesis fig. 4.6), adjusted by its own trends |
|---|---|---|---|---|
| v10-52 | 1.38 | 0.469 | 0.103 | about 0.20 to 0.25 |
| v12-65 | 2.65 | 0.528 | 0.085 | about 0.20 to 0.25 |
| big-bore-70-pushrod-v8 | 2.16 | 0.518 | 0.111 | about 0.21 to 0.27 |
| economy-four-15 | 0.02 | 0.170 | 0.133 | about 0.18 to 0.23 |

Standing finding, as the lead put it, with one measured line added: the model's idle share on the big-cam engines is 0.47
to 0.53, on the toll's slope; the nearest measured residuals (one small engine, read off a plot) are about half of it, and
the economy four agrees with them. Whether that factor of two lives in the overlap's backflow (D's measurement: the model
at 0.4 to 0.6 of a series orifice), in the late closing's push-back and re-induction, or in the threshold, is not yet
shown.

**N3-16, a units-free yardstick: Cavina's orifice form, against the measured points and our engines** (00:59 `date -u`;
P's pointer). Maldonado and Kaul, "Evaluation of residual gas fraction estimation methods for cycle-to-cycle combustion
variability analysis and modeling", Int. J. Engine Research (2021), DOI 10.1177/1468087420983087 (OSTI 1756265, the
accepted manuscript read here, its page 4 rendered): Eqs. 6 to 8, "as demonstrated by Cavina et al.":
  X_res,IVO = (1/C)((r_c - 1)/r_c)(V_IVO/V_d)(E/I)^(1/gamma) (1/lambda_in)
  X_res,VO  = sqrt[(1/C)((r_c - 1)/r_c)] * sqrt[R T_int |E - I| / E] * (pi sqrt(2) / 6) * (OF / N) * (E/I)^((gamma+1)/(2 gamma))
  OF = [Z_int D_int integral(L_int dtheta, IVO to the lifts' crossing) + Z_exh D_exh integral(L_exh dtheta, the crossing to EVC)] / V_d
with E and I the exhaust and intake manifold pressures at the crossing, D the inner seat diameters, Z the valves. No fitted
constant; dimensionally whole when theta and N share an angle unit (OF in degrees per metre with N in degrees a second),
so no unit can slip. C = 1 for a normal burn (their eq. 3); V_IVO/V_d taken as 1/(r_c - 1); T_int 300 K; gamma 1.35.

*Against the thesis's measured points* (1200 rpm; its compression and the loads' intake pressures not stated: r_c 8.5 and
I 40 / 65 kPa for 10 / 25 % load assumed): zero cam 0.237 / 0.165 against measured 0.230 / 0.180 (the clearance term
alone, and right); production 0.410 / 0.251 against 0.250 / 0.195 (1.3 to 1.6 times); intermediate 0.83 / 0.46 against
0.28 / 0.245; max 1.56 / 0.83 against 0.345 / 0.315. The quasi-steady orifice over-predicts the measured backflow, more
as the overlap grows: measured overlap backflow runs well under this form.

*Our four at their warm idles:*

| engine | Cavina backflow + clearance = total | model's share | the model's clearance gas at TDC |
|---|---|---|---|
| v10-52 | 0.062 + 0.126 = **0.189** | 0.469 | 0.128 (0.020 of 0.156 g, my trace) |
| v12-65 | 0.071 + 0.094 = **0.165** | 0.528 | |
| big-bore-70-pushrod-v8 | 0.086 + 0.126 = **0.212** | 0.518 | |
| economy-four-15 | 0.002 + 0.209 = **0.211** | 0.170 | |

The model's clearance gas on the V10 (0.128) is Cavina's clearance term (0.126): the model's whole excess is the overlap's
backflow and its re-induction, about 0.34 of the charge against this form's 0.062 (5.5 times), and this form itself runs
over the measured points at overlap. So, by a yardstick with no fitted constant: the model's big-cam idle share (0.47 to
0.53) is about 2.5 to 3 times an orifice estimate that already over-predicts measured residuals, and above the highest
measured residual P found (0.35 to 0.40, a single at 130 degrees of overlap and 10 % load). The economy four (no lift at
TDC) agrees within 0.04.

*For D:* this form's backflow on the V10 is 0.062 x 0.156 = 0.0097 g a cycle; your series orifice gave 0.110 g and the
model 0.055 g. The form's effective area is pi sqrt(2)/6 = 0.74 of D x L, about 0.24 of the curtain pi D L; the model's
low-lift area is 0.73 of the curtain times q (1.65 / 1.8 on the V10): some 5 times larger. Where the two orifices differ
(area, the density taken, the pressure difference across the valves) is the comparison to make.
