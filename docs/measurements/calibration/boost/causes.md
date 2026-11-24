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

**The sequential pair** (rotary-twin-13): every collector ends in the first turbo's housing, the second turbine fed
from it only through the transfer valve, so the first gate must bypass the whole engine's flow: sized at its 0.55
share it still ran away at 5000 to 6000 rpm (95 to 101 kPa against 70, both gates wide open, knock retard 15). Sized
for the whole flow: 72, 72, 74, 73, 75 kPa at 3000 to 7000 rpm, gates 0.16 to 0.63 (after D's 2df7ea6).

## 2b. Clicks: the valves moved in stairs, and the nozzle's flow jumped where it chokes (fixed in sim/boost/)

X-010 and X-014 (card X): with the turbine coupled into G's pipes, the output stepped at every throttle change (the
3.0 twin-turbo six 39.1 times the local level against 8.1 with boost.model 0). Two causes, each isolated by removing
it: the gate's lift, the variable vanes and the blow-off valve moved only every 16 samples (and snapped by the
reliefs), so the gas through them saw a staircase (no blow-off valve: 24.3; a 50 mm gate: steps all through the run);
and the nozzle's choked branch left out the housing's own give over the sample that the unchoked passes take, so the
flow jumped at the switch. Now the opening the gas sees follows its actuator every sample through two 1 ms stages, and
the choked branch takes the give. After: the six 7.2 to 8.9; the pickup diesel 26.7 (its own reference 26.0); the 2.0
four 20.3 (reference 8.1: a one-sample step at the throttle's opening from idle, only with a recirculating blow-off
valve). Open: the tractor 53 (reference 25.8): an offset switched in after a lift with its vanes shut at idle (vanes no
more shut than 0.8: 25.1; exhaust.model 0: 17.3); suspected, not proven, the housing solve's clamps.

**The tractor's thump, and why F's map was not reshaped.** The tractor's remaining step (53 against 25.8) was the
compressor inlet's sound: that sound is the inlet flow's rate of change, and at idle with the vanes shut the compressor
surges and its duct's flow kinks (found by silencing one source at a time: the machine's sound off, 27.3; the
exhaust's output off, still 54.7). The map is C1 at zero flow and at the knee (value and slope match); what kinks is
dm/dt, at the duct's semi-implicit denominator clamped at 0.5 (it bites on the rising branch a surge runs through) and
at choke, where the ellipse's slope clamp meets the past-choke line. The shaft and the plenum see m, which stays
continuous: only a signal made of dm/dt stepped. The inlet's sound now passes its duct and filter box (two one-pole
stages at 1 kHz, sim/boost/sound.fm): tractor 27.2, the 2.0 four 7.0 (its recirculating-valve "residual" was the same),
the six 8.9. Noted option, not done: a smooth max for the denominator and a blended slope across choke.

## 3. A rotary's turbo matched to half its air (fixed by D, 2df7ea6)

`machine.fm` matches every size to a four-stroke's air (vd x rpm / 120); Engine passes a rotary's vTotal, which it
takes every shaft turn: twice. Asked of D (engine.fm: pass the four-stroke-equivalent displacement); a two-stroke the
same (told T). With D's x2 and the gates of 2 and 2b the boost is held (above); its torque, 345 to 445 N m against
290 to 315, is then the rotary's own (N1, D).

## 4. Not boost: what is left after 1 and 2 (leads, not yet causes)

- The 3.0 L twin-turbo six breathes at 0.51 to 0.65 on the manifold at 2 bar (VE air 1.03 to 1.19; the four 0.84):
  404 N m for 450 to 600. CAUSE FOUND, in the boost after all: on one bank housingOf sent every collector to turbo 0;
  turbo 1 stood (0 rad/s) and turbo 0 choked. Now group 0 feeds turbo 0, group 1 turbo 1, and sideOf(t) folds an
  outlet onto side 0 on a one-sided exhaust (Boost::exhaustSides, called by G). With primaryTo [0,0,0,1,1,1], two
  collectors, a dual exhaust: 507 / 475 / 442 N m at 2866 / 4297 / 5729 rpm, boost 106 to 113 kPa, both shafts about
  15000 rad/s. Test: GateTest.cpp, "twin turbos on one bank". Listen: two whines at one pitch, no backflow chuff.
- The 2.0 four's power at 6528 rpm with engine.wallHeat 1: 260 kW for 155 to 230, its boost held flat to the
  limiter (P's target map has no taper; production two-litres trim the boost above some 5000 rpm).
- The supercharged V8's boost rises 58 to 135 kPa from 2000 to 5500 rpm (pulley 2.4, no loop).
- engine.wallHeat 1 adds 11 to 13 % on the turbo four: the double count D is fixing (Woschni's walls under fuel.model 1
  with the reference's reduced burn share). Tables say which they used.

## Open: matching a turbo to the build's own ratio (measured, not adopted)

F's matching sizes every turbo for a pressure ratio of 1.9 at the redline, whatever the build asks. Sizing instead for
the build's own ratio at the redline (match-to-target.md, all fourteen boosted archetypes) leaves the one-bar builds
where they were and makes the heavy diesels WORSE at their torque peak (the tractor 1784 -> 1216 N m at 1462 rpm: the
larger compressor no longer spools). A truck's turbo is matched to its peak-torque flow, its vanes giving the low end.
The right rule needs the speed at which the build asks its target (its torque peak), a field no build states today:
with it, the compressor would pass the engine's air at the target ratio at that speed with its surge line short of it
and its choke past the redline's flow at the ratio the boost map asks there. Not today's work.

## 4. The heavy diesels: shut vanes pumping the engine (a limit added, sim/boost/machine.fm, 5eaac6f)

Not the over-boost cut nor the gate (both cleared by switching them). The VGT loop shut the vanes to their stop chasing
a target the matched compressor could not reach (in choke 93 % of the time: matched for a ratio of 1.9 at the redline,
the tractor asks 3.6), and with G's coupled housings the cylinders felt it: PMEP 2.5 to 5 bar, the exhaust port 1.7 to
2.5 times the intake's. A production VGT controller limits the pressure before the turbine: spec.boost.backPressure
(p3/p2, absolute, default 1.4, judgement; bypassed while the ECU commands the vanes, so D2's brake keeps its 406 kW).
On the tip (7e0ce9d, the harness cranking as tee::runDyno does), limit on against off (4.0):

| build | rpm | N m on | N m off | p3/p2 on | p3/p2 off |
|---|---|---|---|---|---|
| tractor-15-inline-six | 1000 / 1300 / 1800 | 427 / 1702 / 1396 | 498 / 1523 / 1067 | 1.41 / 1.42 / 1.40 | 1.85 / 1.81 / 2.12 |
| pickup-diesel-67-inline-six | 1600 / 2600 / 3000 | 679 / 667 / 577 | 709 / 410 / 395 | 1.39 / 1.41 / 1.42 | 1.50 / 2.59 / 2.32 |

At the lowest speeds the limit costs 4 to 14 % (it opens vanes before the boost has built); everywhere above it gains
12 to 63 %. The diesels are still short of their ranges: the fuelling's (the reference's 0.62 kept share under Woschni's
walls, routed to D). Their lambda now reads 1 / phi (1.33): chargeDiesel told the sensor nothing before (71d7869).
