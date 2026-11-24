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
