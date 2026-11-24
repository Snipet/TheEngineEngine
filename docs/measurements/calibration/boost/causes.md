# Pattern 2 (every boosted engine too strong): the causes, one a section

Card N2's. Each cause: the ledger line out of range, on which builds, by how much; the one experiment that isolates it;
the source; what was done. Tables: backpressure.md, by-hand.md, gate.md (made by `tee-calib-boost`, README.md).

## 0. How a boosted engine is measured (a cause of the table's misses, not of the engine's)

The table at 463b18f and eb68b0e read peak torque and power from `tee-dyno --predict`. On the 2.0 L turbo four the
predictor gave 437 N m at 2910 rpm and 426 at 5184 where the full engine dyno (`tee::runDyno` steps, 5 s settle, 1 s
mean) gave 401 and 388: +9 to 10 %. Suspected (not proven; card C's): the predictor's few cycles do not settle a
boosted engine's slow states (knock retard builds at 1.5 degrees a knock and settles at 13 degrees at 2000 rpm; the
intercooler's core warms over tens of seconds). P now reads the boosted archetypes from the stepped full dyno.

## 1. No back pressure: the cylinders blew down into the naturally aspirated pipes (fixed by card G, cf267fb)

Ledger line: the exhaust port's mean pressure against the intake's. Before card G's network fed the turbine's
housings, the 2.0 L turbo four's exhaust port stood at 0.61 to 0.87 of its intake port (134 to 188 kPa against 217 to
220) while its turbine's housing, which no pipe saw, stood at 0.98 to 1.32 of the plenum: the pumping loop gave work
(PMEP -0.82 to -0.28 bar) and the cylinder over-scavenged (VE on the manifold 1.02 to 1.04). A matched turbo at full
load runs its turbine inlet at or above the boost, 1.0 to 1.5 (Watson and Janota, *Turbocharging the Internal
Combustion Engine*, ch. 7; Heywood, ch. 6.8). Cause: with boost.model 1 the reference's turbine restriction is off and
F's stand-in handed no pressure back. After G (83a8327): 1.06, 1.13, 1.28, 1.48 at 2000, 3500, 5000, 6500 rpm.
Test: GateTest.cpp, "back pressure against boost on a matched turbo" (between 1.0 and 1.7 at 3500 to 6500 rpm).

## 2. The runaway: a matched wastegate too small to hold its spring (fixed in sim/boost/machine.fm)

**The boosted engine had two states.** The 2.0 L turbo four, spooled at 5271 rpm with F's matched gate (0.85 of the
nozzle's area): the charge pipe at 255 to 290 kPa against its 120 kPa target, the gate wide open, knock retard pinned
at its 15-degree most, the half-burn at 23 degrees after the top, the port gas at 1375 K, 421 to 460 N m (27 bar
BMEP). The same build brought up in steps from 2000 rpm held 120 kPa to 6500. The mechanism is a loop: the boost
overshoots on the spool, the charge knocks, the retard makes a late hot burn, the hotter gas gives the turbine more
power than the open gate can bypass, the boost stays up. Which state a run lands in depends on its history: N1's
ledger (steps from 1500 rpm), P's stepped dyno and the predictor landed in the runaway; my first runs (from 2000)
did not. P's 3.0 twin-turbo six went to 2.99 bar at 4297 rpm (target 1.0) the same way. The kei could not hold its
own 60 kPa spring above 5000 rpm (88 kPa at 6500, 112 at 7800, wide open); the sequential rotary held 70 to 175 kPa
against 70 with its gates wide open.

**Experiment** (one thing changed, spooled at 5271 then 6528 rpm, with G's housings fed): the matched 26.3 mm gate
runs away (255 and 239 kPa); a 32 mm gate (1.5 times the area) holds 126 and 129 kPa, its gate 0.75 and 0.93 open, no
knock: 346 and 334 N m. Knock control off with the matched gate: still runs away (209 and 221 kPa). The gate is the
cause; knock feeds it.

**Source and change**: a wastegate is sized by what it must bypass when the turbine at full flow could make several
times the compressor's need, not by the turbine's size (Bell, *Maximum Boost*, ch. 6; Watson and Janota, ch. 9). The
matched gate now passes, open and alone, the turbo's whole share of the engine's matched flow at the nozzle's design
state (2.4 bar, 1150 K): `aGate = aNoz / 0.65` at size 1 (35 mm on the 2 L four, 22 mm on the kei), the same area
whatever the turbo's size. After: the four holds 120 kPa spooled at the top and stepped from 1500 rpm alike (126 and
129 kPa in both; 355 N m peak, 228 kW at 6528 with engine.wallHeat 0; 402 N m and 260 kW with it 1); the kei holds
60 kPa to 7800 rpm (gate 0.59 to 0.75); the six holds 100 kPa (no runaway). Test: GateTest.cpp, "a matched wastegate
holds its target spooled at the top speed".

## 3. A rotary's turbo matched to half its air (routed to D)

`machine.fm` matches every size to a four-stroke's air (vd x rpm / 120); Engine passes a rotary's vTotal, which it
takes every shaft turn: twice. Asked of D (engine.fm: pass the four-stroke-equivalent displacement); a two-stroke the
same (told T). With every size doubled as a stand-in, the sequential rotary still overboosts (71 to 102 kPa against
70): to be re-measured after D's change and the gate.

## 4. Not boost: what is left after 1 and 2 (leads, not yet causes)

- The 3.0 L twin-turbo six breathes at 0.51 to 0.65 on the manifold at 2 bar (VE air 1.03 to 1.19; the four 0.84):
  404 N m for 450 to 600. A breathing question (N1's, or the build's), not the boost.
- The 2.0 four's power at 6528 rpm with engine.wallHeat 1: 260 kW for 155 to 230, its boost held flat to the
  limiter (P's target map has no taper; production two-litres trim the boost above some 5000 rpm).
- The supercharged V8's boost rises 58 to 135 kPa from 2000 to 5500 rpm (pulley 2.4, no loop).
- engine.wallHeat 1 adds 11 to 13 % on the turbo four: the double count D is fixing (Woschni's walls under fuel.model 1
  with the reference's reduced burn share). Tables say which they used.
