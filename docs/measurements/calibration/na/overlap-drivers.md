# What drives the overlap's flow at a warm idle (N3-16, card D, 25 Nov 01:50 UTC)

The lead's ask (01:37): at a warm idle on the V10, the V12 and the hot hatch, with the economy four beside them as the
engine that reads right, measure the pressures and the valves' flows through the overlap, the cycle's R, E and F, and E
split by its driver. A measurement only: nothing in `sim/` changed. The probe is a scratch patch,
`overlap-drivers.diff` beside this file: local sums in `Cylinder::stepAreas` and a hidden case `[.drivers]`. Apply it
to reproduce, then revert. Tree: the branch at 01:40 (N1's curtain cap 68e683f1 and N2's a18f1bd4 in).

The method. Each engine starts as tee-dyno starts it, warm, with the pedal at 0 and the ECU's idle holding it. It runs
5 s, then 4 s are measured on cylinder 1. Overlap TDC is at 360 degrees of the cylinder's own cycle. Each valve's flow is
counted by direction. "Port" is the pipe end's pressure the valve is solved against (101325 + 2b + z q). R is the burned
gas in the cylinder at intake opening; X the burned gas at intake closing; F the fresh gas at intake closing (m - burned).

## 1. The pressures and the flows through 330 to 410 degrees (g a cycle)

V10 (900 rpm, sd 1.9 rpm, manifold 43.3 kPa):

| degrees | p exhaust port / cylinder / intake port (kPa) | exhaust out | exhaust in | intake pushed back | intake drawn |
|---|---|---|---|---|---|
| 330-340 | 100.1 / 100.2 / 44.2 | 0.0117 | 0 | 0 | 0 |
| 340-350 | 100.9 / 100.9 / 42.8 | 0.0067 | 0 | 0.0007 | 0 |
| 350-360 | 99.2 / 98.8 / 45.3 | 0.0003 | 0.0045 | 0.0080 | 0 |
| 360-370 | 95.7 / 80.1 / 46.9 | 0 | 0.0166 | 0.0233 | 0 |
| 370-380 | 99.7 / 39.8 / 36.8 | 0 | 0.0081 | 0.0118 | 0 |
| 380-390 | 106.0 / 40.5 / 41.1 | 0 | 0.0010 | 0 | 0.0121 |
| 390-400 | 106.7 / 47.2 / 47.3 | 0 | 0 | 0 | 0.0054 |
| 400-410 | 101.9 / 37.6 / 37.7 | 0 | 0 | 0 | 0.0103 |

V12 (900 rpm, sd 1.4, manifold 56.7 kPa): the same shape, the exhaust in 0.0120 / 0.0303 / 0.0168 g at 350-380 and
the intake pushed back 0.0169 / 0.0355 / 0.0156; the cylinder falls from 96 to 52 kPa while the exhaust port holds 92
to 98.

Hot hatch (848 rpm, sd 15 rpm, manifold 56.3 kPa): the overlap sits some 10 degrees earlier (the exhaust in from 340).
The exhaust in is 0.0058 / 0.0195 / 0.0145 g at 340-370, the cylinder falling from 96 to 57 kPa against 96 to 101 at
the port.

Economy four (750 rpm, sd 7.2, manifold 32.2 kPa): almost no overlap. The exhaust in is 0.0007 g a cycle in all, and the
cylinder stays at 96 to 100 kPa until its intake opens past TDC.

## 2. R, E and F at intake closing (g a cycle, cylinder 1)

| engine | share at IVC | burned at IVO (R) | burned added after | fresh (F) | E: exhaust in while both open |
|---|---|---|---|---|---|
| V10 | 0.404 | 0.0350 | 0.0215 | 0.0835 | 0.0303 |
| V12 | 0.488 | 0.0361 | 0.0491 | 0.0895 | 0.0624 |
| hot hatch | 0.458 | 0.0388 | 0.0298 | 0.0812 | 0.0439 |
| economy four | 0.169 | 0.0167 | 0.0007 | 0.0857 | 0.0007 |

- **R at IVO is not the clearance.** Intake opening comes before TDC, and the piston still pushes gas out after it
  (0.018 g on the V10 at 330-350). The clearance at TDC is some 0.017 g on each, from its volume at 100 kPa and 900 K
  (V10 4.5e-5 m³, V12 4.3e-5, economy four 3.8e-5).
- **With E at nothing,** the share would be about R_TDC / (R_TDC + F): 0.17 (V10), 0.16 (V12), 0.15 (economy four).
  So on the big-cam engines E is the larger part of the burned gas, and the share's excess over the measured band is in
  E, as before.
- **F, the fresh charge,** is about the same on all four (0.081 to 0.090 g a cylinder a cycle): what each idle's
  friction asks. On the V10 that is 6.3 g/s of air at 900 rpm. A check against published idle air rates is N1's
  (the lead's ask to N1).

## 3. E split by its driver

Two ways.

**(a) The counterfactual run.** While both valves are open, the exhaust valve is solved against its port's mean
pressure over the previous cycle instead of the arriving wave. The primary's dip and ring are taken out of the overlap,
and everything else runs as it does:

| engine | E as run | E with the port at its mean | share as run | share with the port at its mean |
|---|---|---|---|---|
| V10 | 0.0303 g | 0.0303 g | 0.404 | 0.404 |
| V12 | 0.0624 | 0.0629 | 0.488 | 0.490 |
| hot hatch | 0.0439 | 0.0420 | 0.458 | 0.453 |
| economy four | 0.0007 | 0.0009 | 0.169 | 0.170 |

**The pulse carries at most some 4 % of E.** The port's pressure through the overlap stands a little below its mean,
not above it (the primary's rarefaction half, card G), and holding it at the mean moves E by +0 to -4 %.

**(b) The offline integral.** A plain orifice over the exhaust valve's effective area at the port's own pressure gave
0.053 g on the V10; at the port's cycle mean it gave 0.100 g, 187 %. This overstates the pulse's part: it holds the
cylinder's pressure as it ran, and in the run the cylinder answers the exhaust side. (a) is the one to trust.

**So E is what the steady pressures give.** Once the intake opens past the closing exhaust valve (from about 355 degrees
on the V10, 345 on the hot hatch), the cylinder empties toward the runner and is pulled from about 98 kPa down to the
manifold's 37 to 60 kPa within some 20 degrees. The exhaust side stays at its mean, about 100 kPa, so the whole
manifold difference lands across the exhaust valve's last 1 to 3 mm of lift. Nothing in the overlap is a thing a real
engine lacks at idle. The size of E follows from the areas the valves have there, the manifold's vacuum, and how long
both stay open.

## 4. Whether E returns and whether anything counts it twice

- **It returns.** On the V10 the intake pushes 0.0438 g into its runner at 340-380 (E, 0.0292 in the same window, plus
  some of the cylinder's own gas), and draws it back first at 380-410. The store empties within each cycle and carries
  the cylinder's share ("[.rvstore]" in `tests/sim/core/EngineExplore.cpp`).
- **Nothing counts it twice.**
  - What the intake pushes back leaves the cylinder's burned gas at the cylinder's share and is added back, at the
    store's share, as it is drawn (`Cylinder::stepAreas`, the burned gas's block).
  - The mass books hold at idle under a big overlap: the throttle, the intake valves and the exhaust valves plus the
    cylinders' gain agree within 0.5 % ("the cylinders' books hold at idle under a big overlap").
  - The late closing's push-back cancels from the share in steady state: x = (R + E) / (R + E + F).

## 5. The idle's manifold pressure

V10 43.3 kPa, V12 56.7, hot hatch 56.3, economy four 32.2. No figure with a basis was found for what such engines idle
at. As a recollection only: a road engine's warm idle commonly reads some 30 to 40 kPa on its MAP sensor, and a
long-duration cam idles higher. The economy four sits in that range. The V12's and the hot hatch's 56 kPa are higher.
That follows from their overlap: the gas the overlap returns takes part of the charge's room at a given fresh
charge, so the idle valve opens further for the same F.

## What it says

E is what the honest pressures give through the curtain-line areas. Neither the primary's ring nor the books carry it.
By the lead's rule, the gap goes back to the valves (their effective area at 1 to 3 mm, the flanks, the reverse
discharge) and to the fresh charge an idle needs (N1). Nothing is proposed here.
