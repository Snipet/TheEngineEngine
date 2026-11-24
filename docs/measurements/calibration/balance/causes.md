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
