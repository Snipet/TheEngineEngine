# The exhaust builder (card G, `exhaust.model = 1`)

*What `sim/exhaust/` computes, the numbers it uses and where each comes from. "Reference" marks a number taken
unchanged from `sim/ref/` (whose comments give their sources); "textbook" one from the standard rules for losses at
changes of area (a sudden expansion's `(1 - a/A)^2` on the narrow side's speed, a contraction's about `0.5 (1 - a/A)`:
Idelchik's *Handbook of Hydraulic Resistance*, as the reference cites them); "judgement" one chosen by reasoning about
how such systems behave, not taken from a source. The tests say what each choice produces. Nobody here has heard any
of it.*

`exhaust.model = 0` is the reference's exhaust, its own code. `model = 1` builds the system from a description
(`XSpec`, `sim/exhaust/buildspec.fm`) out of the reference's own parts. With every field of the description at its
default it builds the reference's three topologies, sizes and junctions, solved in the same arithmetic: measured
identical to the bit on all 17 layouts and four sets of the four dials (`exhaust.model 1 at its defaults is the
reference's exhaust`), once both have their steepening pipes settled (below).

In the engine (`sim/exhaust/exhaust.fm`, `Exhaust`): `spec.exhaust.model` 1 forwards the carved module's entry points
to the builder (sim/README.md says how); with every other field at its default it is **identical to model 0 to the bit
on all 17 layouts** through free revs (`exhaust.model 1 at its defaults is model 0 in the engine, every layout`). The
builder's fields are appended to `spec.exhaust` (`sim/spec/exhaust.fm`); two are renamed there because the reference's
dials hold the names: the crossover's kind is `crossKind`, the muffler's `mufKind`. The standalone harness
(`tests/sim/exhaust/harness.fm`) runs the same files on made-up port flows.

## Files

| File | Module | What |
|---|---|---|
| `network.fm` | `XNet`; `xNode` | the graph: every pipe a `Duct` (the reference's), every meeting of pipes a junction, solved each sample |
| `slots.fm` | `XBuild` | the description resolved into the graph; every pipe's size and every junction's losses, every 16 samples; the seams |
| `ends.fm` | `XEnd`, `XPop` | a radiating end with its position tag; the reference's `Pop`, copied |
| `heat2.fm` | `XHeat` | the reference's `soak()` over the sections as built |
| `buildspec.fm` | `XSpec`, `XEngine` | the description (Spec-shaped); what the builder needs to know of the engine |

## The parts (all the reference's)

- **Pipe**: `Duct` (`sim/ref/pipes.fm`): two delay lines read between samples, the wall's loss (Kirchhoff's, three
  shelves), the flow's loss (from the mean Mach number), optional packing, scattering and a turbine's low-pass; the
  steady flow passes whole. Headers, collectors' pipes and main pipes steepen their outgoing waves. Reference.
- **Node**: `xNode` is the reference's `node()` with one addition: each branch's transmission `t` (a valve): the
  branch's admittance times `t`; at `t` below 1e-4 the branch is shut and its pipe sees a closed end. With `t = 1` it is
  the reference's arithmetic, operation for operation. Losses `K 1/2 rho u|u|` on the drive's slow part
  (quasi-steady below a Strouhal number of about 0.2), from the last sample's: one pass, passive. Reference.
- **Pair**: two pipes in series, one slow drive and one loss for the pair, an optional resistance in series and the
  same transmission: the reference's muffler junctions. Reference.
- **Open end**: Silva et al.'s unflanged reflection for the hot gas inside, the exit's loss (K 1 out, 0.5 in), the
  radiation `dq/dt` and the jet's noise. Reference, now one of a list (each end has a position tag: rear centre, left,
  right, side left, right, front, up), summed into the two channels by tag until wave 2's listener takes the list.
- **Closed end**: the wave comes back whole; the pipe's own wall loss is all that takes it. (New; trivially passive.)
- **Port**: the cylinder's flow in, `z q`, with the valve's jet's hiss. Reference.
- **Housing** and **outside pressure**: card F's seam (below).

## The description

**Headers.** `primaryTo[i]`: the collector port `i`'s primary joins (-2: the layout's: one collector, or one a bank;
-1: an open stack). Each primary's length (0: the layout's, `hdr` times the HEADERS dial, the second bank's times
`hdrBank1`) and bore (0: 0.46 of the cylinder's bore), with the reference's pattern of small differences scaled by
`primSpread` (reference: 6 %). A **step** (`primStepAt` > 0): the primary is two pipes, the second `primStepD` times
the bore, joined by a pair with the textbook expansion and contraction losses; the header's per-pass share (0.878) is
divided between them by length.

**Collectors.** `collectorTo[c]`: the side (0, 1) its pipe joins, or 2 + another collector (a second merge: 4-2-1,
tri-Y); two levels at most (a merge must go to a side; the builder forces it). Each collector's pipe: length (0: 0.38
m, a bike's 0.25: reference), bore (0: the main pipe's area when it is the only collector, 0.85 of it otherwise:
reference). Its node's losses: a header merging K 0.25 in, 0.45 out; a collector's pipe merging into the side K 0.15
in, 0.30 out (reference); the same for a pipe merging into a second collector (textbook-sized; the reference has no
such merge). A **log manifold** (`colVol[c]` > 0): a pipe 0.06 m long of area `V / 0.06`, shut at its far end, at the
collector's node: a volume below its own first mode (a quarter wave in 0.06 m: some 1.4 kHz cold).

**Per-pass losses on new arrangements (judgement).** A pipe of a merge (into a second collector or out of one) and a
log manifold's chamber lose 0.4 dB a pass (`keep` 0.955), as the reference's headers (0.878) and main pipes (0.769) do
for what the model does not resolve: bends, slip joints, the boundary layer, the ports' cross flow. Without it a mode
trapped between a merge's pipes and the chambers rang for seconds (the passivity test found it: 1e-5 of the energy
left after 4 s); with it every random topology tested is quiet within 0.71 s. The reference's own collectors' pipes
keep theirs at none.

**Sides and the chain.** `sides` (0: the layout's). Each side, in order:
- **Turbine.** With card F's model, the collectors' pipes end in F's housings and the main pipe begins at the turbine's
  outlet (below). Without it, the reference's: each collector's pipe that ends at a side narrowed and low-passed by the
  turbo's level and size (reference).
- **Main pipe** (0: the layout's length times the PIPE dial, its bore).
- **Converter.** Standard: the reference's monolith (60 Pa s/m at 700 K with its cones, growing with viscosity as
  `(T/700)^0.7`, over the pipe's area, in series at the main pipe's head) and its absorption (2.5 dB above 700 Hz, 5
  more above 4 kHz a pass). High-flow (judgement): 0.4 of the resistance, half the absorption, and the pipe's per-pass
  share raised by 1 dB (it includes the reference's 2.3 dB of the converter's broadband absorption). None (judgement):
  no resistance, no absorption, the per-pass share raised by 2 dB (0.769 to 0.968).
- **Crossover.** H: a pipe `crossD` bore (0: the X-PIPE dial's area share, as the reference's) and `crossLen` long
  (0.25 m: reference) between the sides, sharp-edged (K 0.5 each way at each end: reference). X: the two sides' pipes
  meeting in one four-way node and parting again, each branch K 0.3 each way (judgement: a merge's). `crossAt`: at
  the head (the reference's place) or after the main pipes (then a `postLen` pipe follows, keep 0.96 like the
  reference's tail and H). With card F's turbines a crossover sits after the main pipes.
- **Resonator.** Quarter-wave: a closed stub `c / 4f` long, the main pipe's bore (0) or `resD`, at a tee, then a
  `linkLen` pipe to the muffler. Helmholtz: a neck (`resD`, 0: half the pipe's bore) into a chamber (`resVol`, a pipe
  0.12 m long shut at its end, under a quarter wave at its note so it is a volume), the neck `Leff = An c^2 / (V w^2)`
  long. **Neither carries end corrections**: a plane-wave junction has none, so the model's lengths are the acoustic
  ones, and a real stub or neck for the same note is shorter by them (0.3 d for a stub at a tee, 0.85 r at each end of
  a neck: textbook). Built shorter by them, the Helmholtz rang 1.3 times high. Packed bottle (a glasspack): a pipe the
  main pipe's bore with packing (3 dB above 800 Hz and 7 more above 3.5 kHz a pass for a 0.4 m bottle, by length:
  judgement, the reference's muffler packing's order).
- **Muffler.** Chambered: the reference's two chambers and tube, its MUFFLER dial (or `mufSize`), unchanged.
  Straight-through (judgement): a perforated tube the pipe's bore through a packed shell, as long as the reference's
  can; packing (2 + 6 mu) dB above 900 Hz, (4 + 14 mu) above 4.5 kHz a pass, choking with grazing flow as the
  reference's does; K 0.05 in, the textbook contraction into the tail; the scattering 6 dB. Three-pass (judgement): two
  chambers (`1 + 6 mu` times the pipe's area) and two tubes the pipe's bore, unpacked, each change of area textbook
  plus K 0.5 each way for the gas's turn; the scattering `25 (1 - a/A)` dB. None: the pipe into the tail with its
  textbook change of area.
- **Bypass valve.** A pipe the main pipe's bore (or `bypassD`) round the muffler. It leaves before the can and rejoins
  after it, each through a tube the pipe's bore 0.25 m long (judgement: where a valved system splits and rejoins).
  Those tubes are not decoration: at 200 Hz the first chamber's volume is about a tenth of the pipe's impedance, so a
  bypass joined right at the chamber was barely driven (open measured 0.5 dB *quieter* than shut); the tubes' mass
  lets it carry its share (open +16.7 dB). The valve is the branch's transmission at the split, `t = x^2 (3 - 2x)` of
  its travel `x`, which moves linearly over `bypassTime` (0.3 s); its edge loses K 0.5 each way. Shut, the bypass is a
  dead leg at the join (a quarter-wave branch, as a real one is). Modes: shut, open, by speed and load (open above
  both, shut 10 % below either), by the ECU (`XEngine.ecuValve`).
- **Tail** (0: the layout's length and bore; keep 0.96: reference), one tip or two (two pipes 0.15 m, each half the
  area), each an open end tagged by `exitAt` (the layout's: rear left and right for two sides, rear centre for one).
- **Zoomies**: every primary an open stack (tagged side left or right by bank); a side nothing feeds is not built.

## Heat

The reference's `soak()` over sections: primaries, the first collectors' pipes, the second merges', the main pipes,
the pipes after a crossover or a resonator, the mufflers, the tails; each with the reference's enhancement, shell and
steel (headers 1.5, 1, 9000; collectors 1.2, 1, 6000; main 1, 1, 12000; muffler 2, 4, 10000; tail 1, 1, 5000; the
new sections take their neighbours': judgement). A section a build does not have passes the gas on unchanged, so the
reference's topology soaks as it does. Each section stands for its parallel branches (its length the path's mean,
its bore one branch's); the sides are alike. Not modelled: the two sides differing; a stub's, a chamber's or a
bypass's own stagnant gas (they take their section's).

## Starting at rest (a difference from the reference)

The reference clears its pipes before it sizes them on a new layout. A steepening pipe's arrival times are then those
of its old delay (4 samples after init), and the first pulse down it leaks out as a ramp over the pipe's whole delay:
energy that was never put in (the passivity test, run on the reference's order, saw the energy rise 40-fold in the
first 160 samples). The builder clears, sizes, then settles those timings (`XNet::settle`). `model = 0` keeps the
reference's start; the reference-topology test settles its copy of the reference too, and compares from the first
sample. (Without settling either, the two differ by -45 to -85 dB at full load for as long as the test ran: the
steepening system at 34 kPa pulses forgets a small difference only slowly.)

## Passivity of a packed pipe (card T's finding; model 1)

A pipe's 12 Hz tracker takes its steady flow out before its filters, as the reference's does. With a shelf or a
low-pass after it and little loss a pass, one pass exceeds unity near 25 Hz. The builder computes each used pipe's
exact one-pass gain, |H| at 48 log-spaced frequencies from 4 Hz to 4 kHz (`XBuild::passGain`), at a build and every
32 control steps. Where it exceeds 0.9999 the pipe puts its filters first and its tracker after (`Duct::filtFirst`),
which is passive whatever the filter. Measured over a sweep of keep, gain, packing and a low-pass (180 pipes): the
largest pass 0.99986, where the reference's order would reach 1.0718.

The engine at its defaults is still identical to model 0 on all 17 layouts. In the harness (cold, other dials) a few
of the reference's own pipes pass more than unity: 6 of the 26 reference-test runs. There model 1 leaves the
reference's sound by -47 to -58 dB. The reference test therefore compares with the fix off (`XBuild::passFix = 0`, a
test's switch) and prints the difference the fix makes.

## External wastegates (card F's seam)

With `boost.gate` external, each turbo's gate gets its own pipe (gateLen, gateD; XD_GATE). The pipe runs from an
EXTP junction to an open end tagged front, a screamer pipe; F has no recirculated choice yet. The count comes from
`Boost::externalGates()`, read at every configure (a change rebuilds under the fade). Each sample, `Boost::gatePipe(t,
a, z)` solves turbo t's gate against the pipe's near end, as `downpipe()` does. Measured at 4500 rpm full throttle:
- the 2 L four, gate external: 67.3 kPa of boost, 0.116 kg/s through its gate pipe;
- the V8 with two: 42.1 kPa, 0.013 kg/s;
- both finite and quiet after the key is off.

## Exhaust flap (a diesel's exhaust brake; agreed with D2)

`flapKind` "Butterfly" puts a flap on each side's main-pipe branch at its head: behind the turbine's outlet, or behind
the collectors when there is no turbine.
- **Driver:** card D2's brake calls `Exhaust::setFlap(0 open .. 1 shut)` every 16 samples. `flapFitted()` says
  whether one acts (model 1 and a butterfly); otherwise D2 keeps its stand-in.
- **Travel:** the flap moves over `flapTime` (0.15 s), and the gas sees smoothstep of that travel.
- **Relief:** a spring under the plate gives above `flapRelief` (350 kPa over the air) and is wide open 30 % above
  it; the plate follows in 20 ms (judgement).
- **Model:**
  - an orifice of open area f A, where f goes from 1 to `flapOrifice` (0.03) as it shuts;
  - discharge cd goes from 1 to 0.7 (a sharp plate's, judgement);
  - loss K = (1/cd - f)^2 on the orifice's speed (Borda-Carnot), plus a thin plate's 0.2 wide open (judgement,
    within Idelchik's range);
  - made compressible with the valve's law: the drop for a flow is scaled by (sqrt 2 / t)^2, where t is `Valve::tOf`
    (psi / sqrt(1 - r)) at the mean ratio across the flap; on the gas before it, at its mean pressure.
- **Solve:** at a node, solved every sample as the other junction losses are (lossFlow on the main pipe's slow
  drive). Behind a turbine (an EXTP head), a resistance in series at the mean flow, giving the same mean drop.

Telemetry: `tel.exhaust.flap` (its closing as the gas sees it) and `pFlap` (the pressure before it, over the air).

Measured in the harness: the 2 L four's layout with a source of 0.12 m^3/s, gas at 500 K.
- open: 11.6 kPa before it;
- shut, with the relief at 600 kPa: 655 kPa;
- shut, with the relief at 350 kPa: 382 kPa;
- every sample finite.

In the engine (measured by D2, tee-tests-diesel "*butterfly*"): the 15 L six with twin turbos, motoring at 2000
rpm, pedal up. Open: pFlap 4 kPa, 54 kW retarding. Shut: flap 0.96, pFlap 362 kPa (the relief holding), 130 kW.

## The exhaust port's cooling (`portHeat`, model 1)

Between the valve and the header's flange the gas gives heat to the head's port wall, and the wall gives it to the
coolant.
- **Gas side:** Caton and Heywood's exhaust-port correlation as cited in Heywood ch. 12, Nu = 0.0483 Re^0.783 on the
  port's diameter, with Re from each port's mean mass flow. That correlation is a fit of time-averaged heat transfer
  against time-averaged Re, so the blowdown's spurt is already inside it. Applied per sample, the same law would give
  less, because Re^0.783 is concave.
- **Entrance region:** times Mills' abrupt-entrance factor, 1 + 2.4254 (L/D)^-0.676 = 2.77 at L = 1.6 D. A port is a
  short duct entered through the valve's jet.
- **Gas properties:** viscosity 3.3e-7 T^0.7 Pa s, cp 1150 J/kg K, Pr 0.7.
- **Port:** diameter 0.46 of the bore, length 1.6 diameters (judgement).
- **Wall:** cast iron 6 mm thick (k 50 W/m K) to a water jacket at 10 kW/m^2 K (local boiling; judgement), with heat
  capacity 7200 x 460 J/m^3 K. It warms by what the gas gives less what the coolant takes.

Telemetry: `portTemperature` stays the gas at the valve (the cylinder's). Also `portExitTemperature`, the gas into
the headers, which feeds `headerTemperature`; `portHeat` (W from the gas); `portCoolantHeat` (W to the coolant);
`portWallTemperature`. `portHeat` 0 and model 0 have none, which is how test 2 compares with the reference.

How sure:
- The correlation's form and exponent: as published, to my knowledge. Its coefficient is unverified here.
- Mills' factor: a textbook form for short ducts.
- The wall's numbers: judgement.
- Nothing is tuned to a target.

Measured (`the exhaust port's walls cool the gas, in full`; warm day, full throttle, the heat settled):

| build | rpm | valve | port exit | header | port takes | brake |
|---|---|---|---|---|---|---|
| V8 | 3000 | 1081 K | 983 K | 950 K | 18.5 kW | 161 kW |
| V8 | 5000 | 1175 K | 1084 K | 1050 K | 29.8 kW | 249 kW |
| large diesel | 1800 | 733 K | 695 K | 691 K | 26.3 kW | 362 kW |

- The V8's header is inside 900 to 1100 K.
- The diesel's header is under 700 to 950 K because its gas at the valve is already under it: the cylinder's, not
  the port's.
- Energy: what the gas gave equals what the coolant took plus what the walls stored, to within 0.7 %.

**The enthalpy lost before a turbine** (N3's energy book; the lead's ruling 24 Nov: recorded, no change). The 2.0
turbo four at full throttle, warm day, heat settled, in kW above 298 K (cp 1150), the soak's section outlets read out:

| | 1800 rpm (0.031 kg/s) | 4000 rpm (0.146 kg/s) |
|---|---|---|
| out of the valves | 30.8 | 179.3 |
| the port | -5.0 (16 %); gas 1156 -> 1018 K, wall 345 K | -17.5 (9.8 %); wall 556 K |
| the primaries | -3.2 (10 %); 1018 -> 929 K, wall 641 K, NTU 0.27, 19 ms | -4.6 (2.6 %); wall 1112 K, NTU 0.20, 3 ms |
| the collector | -1.2 (4 %); 929 -> 895 K, wall 655 K, NTU 0.13, 10 ms | -1.7 (0.9 %); wall 1124 K, 1.6 ms |
| into the housing | 22.0 (29 % lost) | 156.7 (13 % lost) |

By cause:
- **The law:** Caton and Heywood with Mills' factor in the port, Dittus-Boelter with an enhancement downstream. Heat
  goes as m^0.8 and enthalpy as m, so the share rises as the flow falls.
- **Cooler walls at low flow.**
- **A longer residence.**

The port is half the loss at 1800 rpm.

Sources:
- Caton and Heywood, Int. J. Heat Mass Transfer 24(4), 581-595 (1981), and SAE 800290: the port's transfer is
  dominated by the valve's jet.
- SAE 760766/760767 (1976): a port designed for minimum heat transfer raises the exhaust gas some 250 F, about 140 K.
  That bounds what a port takes; the model's port drop is 104 to 138 K.
- **No cited share of the enthalpy at low speed was found** (the papers paywalled, an open copy refused).

The softest number is the port's wall, 345 K at 1800 rpm: my judged 6 mm of iron and 10 kW/m^2 K on the jacket.
- **Probe (not adopted, no source for a port's wall in a water-cooled head):** the wall floored at 450 K at 1800 rpm
  takes the port's heat from 4.9 to 4.3 kW. The loss before the turbine goes 28.5 to 26.8 %, the boost 6.5 to 6.8
  kPa, and the torque stays at 147 N m.
- At 4000 rpm the wall is already 543 K, so the floor changes nothing.

## Mass means and thermocouples (telemetry)

`portTemperature`, `portExitTemperature` and `headerTemperature` are mass (enthalpy) means: of the gas the valves pass,
and of that gas after the port and down the primaries. A thermocouple sits in the port all the time while the
blowdown is most of the mass in little of the time, so it reads less. `portThermocouple` and `headerThermocouple`
(`Exhaust::thermocouple`) are a first form of what one reads, and every element of it is judgement:
- **The port's local gas:** a mixing cup of the port's volume. What the valve passes replaces it, backflow fills it
  with port-exit gas, and between pulses it cools toward the port's wall (36 ms).
- **The bead:** the local temperature weighted by the bead's heat transfer, sqrt(u + 2 m/s), with no radiation.
- **The header's thermocouple:** the port's, less the mean drop to the primaries.

Measured at full throttle, heat settled (the hidden case `measure: the exhaust's temperatures at full load, mass means
and thermocouples`):

| build | rpm | port: mass mean exit | port: thermocouple | primaries: mass mean | header: thermocouple |
|---|---|---|---|---|---|
| flat six | 5000 | 1291 K | 1212 K | 1239 K | 1160 K |
| small-block | 4500 | 1346 K | 1282 K | 1270 K | 1206 K |
| 2.0 turbo four | 5500 | 1357 K | 1351 K | 1317 K | 1312 K |
| 6.7 pickup diesel | 2800 | 1002 K | 919 K | 976 K | 893 K |

On the turbo four the two agree because its ports never stop flowing (the turbine's back pressure).

**The bead's radiation error** (`Exhaust::bead`): a bead in hot gas inside a cooler wall reads below the gas. Its
balance is h (tg - tb) = eps sigma (tb^4 - tw^4) (Mills; Incropera's thermocouple example).
- h is from Ranz and Marshall's sphere at the mean speed past the bead.
- The bead is a 1.5 mm sheathed junction with emissivity 0.8 (judgement, an instrument's).
- The walls: the port's wall for the port reading, the primaries' for the header reading.
- The gas and the walls are untouched.

The port's correction is the larger, because its water-cooled wall (460 to 600 K) is far cooler than a primary's
(900 to 1000 K). After a845b07f (the cylinder's new gamma), full throttle, port / header thermocouple, before and after
the radiation error:

| build | rpm | before | after |
|---|---|---|---|
| flat six | 5000 | 1242 / 1190 K | 1147 / 1119 K |
| small-block | 4500 | 1303 / 1227 K | 1196 / 1165 K |
| 3.2 six | 6500 | 1256 / 1211 K | 1173 / 1169 K |
| V12 | 6000 | 1218 / 1157 K | 1128 / 1106 K |
| 2.0 turbo four | 5500 | 1315 / 1287 K | 1230 / 1239 K |
| 6.7 pickup diesel | 2800 | 872 / 858 K | 846 / 842 K |

## What the network costs, by element (the typical build, 19:30)

The build: the twin-turbo V8 with every switch, with `content/builds/exhausts/dual-x-resonated-valved.json`, on
drive-car. Its network has 32 pipes and 33 junctions:
- 9 nodes: six of 3 arms, one of 4, two of 5 or more;
- 8 pairs, 8 ports, 2 open ends, 2 closed, 2 housings, 2 EXTP heads.

From `tee-bench --profile --engine`, ns a 96 kHz sample with the sampler's own cost (about +10 %), R = 1962 ns in
these units:

| element | ns | R |
|---|---:|---:|
| a pipe (Duct: read 24, write 7, the rest 6) | 37 | 0.019 |
| a node (xNode with solveNode) | 51 | 0.026 |
| a pair (two pipes in series, with its loss) | 17 | 0.009 |
| an open end (its radiation filters and jet noise) | 46 | 0.023 |
| fixed: Exhaust's meters, thermocouples and pops | ~200 | ~0.10 |
| fixed: XBuild's control steps (shape, the check, the bands) | ~110 | ~0.06 |
| fixed: XHeat | 20 | 0.01 |

The reference's own exhaust is 765 ns at model 0. The rest is the extra pipes and the general junction solve.

Telemetry gives the counts as built, for a readout of the cost: `tel.exhaust.pipes`, `nodes`, `pairs` and
`openEnds`. The other junctions (ports, closed ends, housings, EXTP heads) sit in the fixed part. They are counted
from the builder's network, which is built at every configure, in model 0 too as the reference's topology. This table
is the source of the per-element costs; the R figures are this machine's, and the host's measured R scales them.

Exact savings taken, each proven by comparing the rendered WAV byte for byte before and after (identical), together
1.749 to 1.708 of the reference (about 120 ns):
- the passivity check skipped while its inputs stand (c4ce28f): passGain 68 to 26 ns;
- lossHold skipped on branches with no loss (exactly 0 there): xNode 291 to 247 ns.

Savings that change the sound, measured but **not** made (Economy / TEE_FAST for the owner's ears). The waveform is
chaotic at load, so a control that changes `xiWall` by 1e-6 relative already leaves the difference only 2.6 dB under
the signal, with octave bands within ±0.1 dB except 63 Hz at -1.2. The bands are therefore read against that floor:

| variant | saved | bands against the floor |
|---|---|---|
| A: the wall's shelves off on pipes under 0.6 m (stubs, links, the X) | some 0.03 R (1.722 to 1.701 of the reference) | +0.1 to 0.3 dB from 2 to 16 kHz: the short pipes lose less of the highs |
| B: the flow's losses and `shape()` every 64 samples instead of 16 | some 0.04 R (to 1.693) | none beyond the floor |

## The valves' mean flow closed (`flowClose`, model 1; the lead's ruling, 24 Nov)

The valve gives its pipe g q, not q (duct.fm, `Valve`: the simple wave's stiffening, g = 1 + (gamma + 1)/4 u/c), and
the cylinder's mass `mE` is reckoned from q. Over a cycle g q is not q: the blowdown's fast gas carries its square term
out.

Measured with each port's sums added in 1024-sample blocks (a first try in plain float over 10^6 samples carried a few
per cent of rounding). Pipes given / let out (Σ qEa / Σ qE):

| build | idle | full throttle |
|---|---|---|
| big-block V8 | 1.046 | 1.128 (2500 rpm), 1.156 (4000) |
| air-cooled flat six | | 1.14 to 1.19 |
| 2.0 turbo four | | 1.18 to 1.22 |

The closing: `Exhaust::stepLet(qe, qeLet, np)` follows each port's surplus (qEa - qE) over 20 ms, as the intake's
plenum does (intake.fm, `mStiff`), and takes it off what that port's pipe is given. The pulse keeps its gain; the
mean is what the valve let out. Measured: 1.000 within 0.2 % in every case.

Before and after, full throttle on the dyno, warm, heat settled (collectors' mean over the air; the port's absolute):

| build, rpm | collectors | port | boost | gate | torque |
|---|---|---|---|---|---|
| big-block, idle | 0.5 -> 0.5 kPa | 101.8 -> 101.8 | | | 18 -> 18 N m |
| big-block, 2500 | 12.4 -> 10.9 | 114.0 -> 112.5 | | | 590 -> 594 |
| big-block, 4000 | 21.7 -> 18.4 | 123.5 -> 120.3 | | | 555 -> 559 |
| flat six, 2500 | 10.2 -> 8.8 | 111.7 -> 110.3 | | | 336 -> 336 |
| flat six, 4500 | 23.4 -> 19.1 | 125.3 -> 120.9 | | | 364 -> 365 |
| flat six, 6500 | 29.0 -> 23.8 | 131.1 -> 125.9 | | | 275 -> 277 |
| turbo four, 2000 | 50.3 -> 28.3 | 151.9 -> 129.8 | 44.0 -> 23.2 kPa | 0 -> 0 | 229 -> 194 |
| turbo four, 3000 | 142.1 -> 134.3 | 243.9 -> 236.5 | 120.6 -> 117.3 | 0.20 -> 0.06 | 336 -> 333 |
| turbo four, 5500 | 153.8 -> 147.4 | 256.0 -> 250.2 | 99.6 -> 100.3 | 0.66 -> 0.47 | 283 -> 283 |

The sound at the tail, drive-car, by octave band, against each build's control (a change of 1e-6 in `xiWall`, which
moves the bands by at most 0.35 dB):
- **big-block V8:** +0.5 to 0.8 dB from 31.5 to 250 Hz; -0.45 to -0.84 dB from 2 to 16 kHz.
- **turbo four:** +1.6 to 1.8 dB at 63 and 125 Hz; -1.2 to -2.6 dB from 2 to 16 kHz; -2.7 dB at 31.5 Hz; the level
  -0.6 dB.

What a person should hear: a little less hiss and rush (the flow noise goes as the mean gas speed squared, and that
speed fell 12 to 20 %), and a slightly fuller low end on the NA engine. On the turbo four, less turbine and jet rush
and a softer low-speed pull, as its boost at 2000 rpm halves.

`flowClose` 0 gives the reference's g q, which is how test 2 compares with the reference.

**Closed by mass at the turbines** (N3's books, 24 Nov). The pipes carry volume, linearly, and mass is volume times
the local pressure over R T. The gas cools from the valve to the collectors, and the cylinders reckon their mass at
the port's pressure in the blowdown while a housing books its own at its own. So the valves' volume, delivered,
put the wrong mass into the turbines:
- turbo four at 3050 rpm: 1.14 of what was let out (the cooling alone gives 1.148);
- twin-turbo V8 at 4375 rpm: 1.03.

With the turbines fed, `Exhaust::bookLet` closes the target by mass where it is booked. Every 50 ms it moves
`tLet` by a fifth of Boost's mass from the pipes (`mach.sumMPipes`) over the valves' less one, with the backflow
reckoned as the cylinder reckons it. The error itself is used, not its log: a window's ratio is noisy, and a log's
zero mean leaves the ratio's mean 2 % high. Measured over 2 s (N3's probe):
- turbo four: 0.9972 (tLet 0.887);
- twin-turbo V8: 1.0071 (tLet 1.004).

Without turbines the target is the valves' volume (tLet 1).

**A known overstatement, left (the lead's ruling, 24 Nov: measure first, then correct only if the torque moves more
than a few per cent).** Without turbines nothing books mass downstream, so the mean volume reaching the tail is
the valves' volume. The real gas there is cooler and at less pressure; its volume is the valves' times
(T_tail / T_valve) (p_port / p_tail). Measured as a local experiment, the tail's mean flow taken that way, at full
throttle, warm, heat settled:

| build | rpm | ratio | port abs | collectors | before the muffler | torque | residual | PMEP |
|---|---|---|---|---|---|---|---|---|
| small-block, two valves | 4000 | 0.917 | 141.0 -> 137.6 kPa | 39.4 -> 36.0 | 23.9 -> 21.5 | 402 -> 405 N m | 0.074 -> 0.075 | 0.88 -> 0.86 bar |
| 3.2 six, four valves | 7000 | 0.776 | 116.2 -> 112.5 | 12.1 -> 8.4 | 4.5 -> 2.4 | 249 -> 250 | 0.063 -> 0.064 | 0.35 -> 0.35 |
| four-cam V8, four valves | 6500 | 0.800 | 128.9 -> 122.7 | 26.3 -> 20.2 | 14.5 -> 10.4 | 432 -> 434 | 0.076 -> 0.076 | 0.59 -> 0.56 |

Torque moves 0.4 to 0.7 % and the residual by at most 0.001, so the tuned engines' shortfall is not here. The back
pressure at the collectors is overstated by 3 to 6 kPa: recorded, not corrected. The ratio came out 0.78 to 0.92,
not the 1.3 to 1.4 first estimated, because the port's higher pressure offsets much of the cooling.

Against a published figure: DieselNet's "Engine Exhaust Back Pressure" (Jääskeläinen; dieselnet.com/tech/
diesel_exh_pres.php) measures at the manifold's outlet on an NA engine (the collectors here). It gives a muffler's
own as some 6 kPa, and limits of 40 kPa under 50 kW to 10 kPa at 500 kW. Those are limits, mainly for diesel and
generator engines, not a stock car system's measured value; I found no published stock figure for a petrol NA engine
at rated power. The small-block's 36 to 39 kPa sits near such a limit's top for its size; the six's 8 to 12 kPa and
the V8's 20 to 26 kPa sit within.

## Glide and rebuild (dragging a slider)

These fields glide toward the Spec with a 0.2 s time constant (`XBuild::glide`). The pipes' delays, areas and
losses follow them every control step in `shape()`, with no rebuild and no fade:
- every length: primLen, colLen, mainLen, crossLen, postLen, resLen, linkLen, mufLen, tailLen, gateLen;
- every bore: primD, colD, mainD, crossD, resD, bypassD, tailD, gateD;
- colVol, resHz, resVol, mufSize, primSpread, primStepAt, primStepD, flapOrifice.

These rebuild under the exhaust's fade (6 ms down, 12 ms up):
- the kinds: converter, resonator, mufKind, bypass, tips, exitAt, crossKind, crossAt, sides;
- the tables: primaryTo, collectorTo;
- a size moving between 0 (the layout's) and a value of its own (for mufSize, -1);
- a collector's volume appearing or going, and the primaries' step appearing or going.

The bypass valve's position already travels by its own rule over bypassTime, and the valve's rule fields act at once. The flap (flapKind) is a loss only and needs no rebuild; its position travels over flapTime.
Measured: the V8 at 3000 rpm, half throttle, its main pipes dragged from 1.5 to 3 m and 60 to 80 mm in 20 steps over
a second. The fade never moved, and the largest step between samples was 0.3865 against 0.3901 in the second before.
A second after the last step the pipes were within 0.06 % of the target. A muffler's kind changed took the fade down to 0.388.

## Seams

In the engine the seams are `Exhaust`'s (sim/README.md). The builder's own, per sample: `read()`; the cylinders
(`net.portWave(i)`, `net.portZ(i)`); `prepare()`; card F's turbines; `step(q)`.

- **Card F** (`Boost` = F's `BoostMachine`): the integrator fills `XEngine.housing[c]` from `Boost::housingOf(bank,
  group, banks)` and `XEngine.gates`, and sets the reference's restriction (`aspUse`) to 0. Each sample, after
  `prepare()`: for each housing `h`, `setHouse(h, Boost::housing(h, houseDrive(h), houseAdmit(h), houseT(h)))`; for
  each side `s` with a downpipe, `setDown(s, Boost::downpipe(s, downWave(s), downZ(s)))`; for each external gate `t`,
  `setGate(t, Boost::gatePipe(t, gateWave(t), gateZ(t)))`. The drive is F's `sum 2 a_i / z_i` less what each pipe's
  junction loss holds back (so the flows F sees are the ones the pipes carry); each pipe then takes `p - a_i` plus its
  loss's share. The downpipe's impedance includes the converter in series; the pressure F returns is behind it.
- **Card H**: `pop(int bank, float amplitude)` (Pa, as the reference's `Pop::fire`), at the collector the bank's first
  port joins; `gasTemperature(int bank)` (K, the collectors' section's gas).

## Telemetry and Scope (fields of `XBuild`, to be copied into `tel.exhaust` and the Scope at integration)

Means over some 0.2 s: `pTurb` (the collectors, before a turbine), `pConv[s]` (each head, before the converter),
`pMuf[s]` (before the muffler: back pressure). `heat.tGas`, `heat.tWall` and `mach` per section; `vPos[s]` the valve's
travel; `net.ends[e].lev` each radiating end's mean square. Scope: `scopeP[64]`, `scopeX[64]`: the pressure along the
path from `scopePort`'s primary to its tail, spaced by length, every 128 samples while `scopeOn`.

## What the tests measured (made-up port flows: blowdown pulses, no cylinders)

| Test | Measured |
|---|---|
| 2: defaults = the reference's topologies | identical to the bit, 17 layouts and three other dial sets |
| 3: passivity, 50 random topologies | the waves' energy never up more than 2.3 % from one 5 ms window to the next (what the filters hold is not in the measure); under 1e-6 of its peak within 0.71 s |
| 4: conservation, 20 element kinds | flow out of the ends against flow in within 0.0073 % (temperatures held; while the gas warms, the volume out runs some 1 % over: the model's flows are volumes) |
| 5: 180-degree headers, crossplane V8, 3000 rpm | off-order lines against firing-order lines at each tail: +2.5 dB with a collector a bank, -24.5 / -21.1 dB with 180-degree headers; a flat four's unequal headers -0.5 dB, equal -16.2 dB |
| 6: X against H (after the main pipes) | the tails' difference over their sum: none +2.4 dB, H -2.3, X: the tails identical (one node, two identical sides after it); above 250 Hz to 4 kHz the X is 1.8 to 2.5 dB above the H |
| 7: resonators | quarter-wave at 200 Hz: the 200 Hz line -38.5 dB (100 and 400 Hz within 0.6 dB); Helmholtz at 150 Hz: -21.2 dB |
| 8: mufflers | insertion loss chambered 22.2, three-pass 15.9, straight-through 1.8 dB; back pressure 3154, 1327, 311, none 272 Pa; bypass open +16.7 dB, its share above 1 kHz +0.1 dB; opening in 0.3 s: no burst above 10 kHz (+0.1 dB against open; snapped open in 10 ms: +4.7 dB); the largest step between samples no more than running open at 1 s of travel (8.40 / 8.38), 1.8 times it at 0.3 s (the level overshoots as the steady flow moves into the bypass) |
| 9: no converter; zoomies | back pressure at the head 3.2 against 4.6 kPa, +3.8 dB above 2 kHz; zoomies on 4, 8 and 12 cylinders: that many radiating ends |
| 11 (standalone): 45 random topologies rebuilt while running, 3 minutes | every output finite; the largest 5636 (dq/dt, m^3/s^2: an open or unmuffled build at speed; the reference's V8 runs some 2 to 40 rms on the same source), 3 s after the flows stop the output 7.5e-22 and the heads' mean pressure 0.0007 Pa |
| 10 (stand-in turbine) | the housing's junction passive (no rise, gone in 1.71 s); the downpipe's head swings 3.0 dB less than the housing |

**Cost** (dev build, this container, R = 2549 ns a 96 kHz sample by `tee-bench --quick`): the reference's own exhaust
some 920 to 1200 ns a sample on the V8 (the harness's copy, noisy with the machine's load); the builder on the same
topology +220 to +300 ns (0.09 to 0.12 R): 21 pipes, 54 to 62 ns a pipe with its junctions. A dual system with an X,
quarter-wave stubs and bypass valves: 32 pipes, +800 ns over the reference's exhaust (0.31 R) standalone; in the engine 1.48 R against 1.04 R (+0.44 R, tee-bench --engine): over the card's 0.25 R,
nearly all of it the 11 more pipes at the reference's own cost a pipe.

## In the engine (measured)

| Test | Measured |
|---|---|
| 1: model 0 is the reference | the null test (tests/sim/NullTest.cpp: every preset and scenario, every layout with a wild cam): identical |
| 2: model 1 at its defaults | identical to model 0 to the bit, 17 layouts, 8 s of free revs (start, idle, blips, limiter, overrun, off, a restart): the builder starts its pipes as the reference does at a configure (it is built at every configure, even in model 0, so its pipes carry the same history) and draws its noise from the engine's stream in the reference's order |
| 10: card F's turbine | a 2 L four with one turbo, the V8 with two (boost.model 1): the collectors fed into the housings, 65.5 and 46.2 kPa of boost at 4500 rpm full throttle; the downpipe's head swings 16.0 and 16.9 dB less than the collector's end; with the key off the pressures fall away (to 3 and 57 Pa when the engine falls silent and stops computing) |
| 11: random topologies while running | ten minutes, the V8 on random pedals, a random description every 4 s (now and then the model switched), under the exhaust's own fade: every sample finite and within full scale, nothing left 6 s after the key is off (numbers printed by the test) |
| `content/builds/exhausts/` | every part build loads with no unknown field, starts and plays finite on the builder |

## Not modelled

The sides' gas differing; directivity (wave 2); a pipe's bends as anything but a share of each pass; the crossover's
own small differences in length (an X with identical sides after it makes identical tails); two-stroke expansion
chambers (wave 2, from these parts).

## Bangs: heat released in the pipe

`spec.exhaust.bang` (the builder only): **Burned in the pipe** (the default since the owner heard both, 25 Nov)
releases a burn's heat in its bank's collector gas. The owner heard the carved pops as
artificial (25 Nov). The inventory found the cause: on first.json's fuel-cut limiter the account lit 25 to 50 bangs a
second, nearly all at its 30 kPa ceiling, each with the same click tail.

- **The source.** A heat release Q̇(t) in a gas expands it: a volume source q = (γ − 1)/(γ p) Q̇. This is the Rayleigh
  term of thermoacoustics (Dowling and Stow, J. Propulsion and Power 19, 2003). The collector's junction takes it
  beside its other sources (`network.fm` `popQ`); the pipes, the junctions and the tail make the sound. γ = 1.33 and
  p is the collector's mean pressure.
- **The release.** A Wiebe curve, x = 1 − exp(−5 (t/τ)³), normalised to reach 1 at τ. It is released by differences
  of x, so the heat released sums to the heat given to the float's rounding. Its steepest rate is 2.02 Q/τ, at
  0.51 τ (BangTest checks both).
- **What bangs.** Only a premixed charge: fuel that left its cylinder with its own air. That is a spark cut, a misfire,
  or an overrun charge burned late. A rich charge's leftover fuel is mostly CO and H₂. Once it meets another charge's
  air it lies far below their lower flammability limits in air: CO 12.5 %, H₂ 4 % (Coward and Jones, US Bureau of
  Mines Bulletin 503, 1952). No flame runs through it; it oxidises where it mixes and is left out of the sound.
- **The burn's time.** S_L is Metghalchi and Keck's correlation (Combustion and Flame 48, 1982). It uses iso-octane's
  constants, 0.2632 m/s and −0.8472 m/s with the fastest mixture at φ 1.13, which the ANSYS CFX Theory Guide's
  Table 7.2 gives. The exponents 2.18 − 0.8(φ − 1) and −0.16 + 0.22(φ − 1) and the burned gas's factor (1 − 2.1 x_b)
  are Heywood's Table 9.3, as recalled and not re-read. S_L is taken at the slug's temperature and the collector's
  pressure.
  - S_T = S_L + u′ (Damköhler's large-scale limit).
  - u′ is a tenth of the gas's speed through the collector as its blowdowns drive it: half its branches' flows over
    the pipe's area, the peak held over some 10 ms (`uHold`). Pipe flows' turbulence intensities run some 5 to 10 %;
    the upper end is used for a pulse's, as judgement.
  - The burned gas's expansion is not counted. A burning velocity is against the unburned gas, so expansion moves the
    front but pushes the unburned gas ahead as fast.
  - Ignition and area: the blowdown that lights a slug runs into it at its own speed, so the hot gas folds into the
    slug along its length, and the flame burns across the bore from that interface. τ = length/u + (bore/2)/S_T, at
    most one front's length/S_T. That is the flame's area a finger flame would need a source for, given here by the
    pulse's mixing; which holds is judgement, and both are kept (`bangTauFront`). A slug carrying its own flame burns
    from within: (bore/2)/S_T.
  - What would still be burning when the gas leaves the pipes (the sections' lengths over their mean speeds) burns
    outside and is not this source.
- **Measured in the engine**, bench, full throttle to the limiter (BangTest's `measure` case; a cut charge at 700 K,
  φ 1.15, burnTime's arithmetic):
  - first.json's collector, 52 mm: its blowdowns drive the gas at 175 to 260 m/s, so S_T is 19 to 27 m/s. A cut
    charge of 0.5 to 1.0 g burns in 2.2 to 3.3 ms folded, or 11 to 19 ms as one front.
  - The rally five's, 57 mm: 160 to 290 m/s, S_T 17 to 30 m/s. 1.5 to 4.0 ms folded, 5.5 to 21 ms as one front.
  - At idle the gas runs at 14 to 17 m/s, and the burn takes 17 to 18 ms folded or about 41 ms as one front.
  - The held rig, its flows a third of the engine's, gave 3 to 4.5 ms folded at 3000 to 6000 rpm, and 17 to 19 ms at
    1000.
- **The account's side** (`sim/ecu/overrun.fm`, `sim/combustion/combustion.fm`: card N1's seam, 991a00d3 to
  682ba1fe, from docs/handoffs/g-exhaust.md, "The pops' seam") feeds it under `ecu.model` 1. Charges are kept by
  cylinder until their exhaust valve opens; premixed slugs are lit by their own flame or by the bank's held blowdown
  while still flammable. **Carved** is the reference's pulse and crackle, kept as the choice. The reference ECU's
  dice (`ecu.model` 0) fire the carved pulse under either setting. The owner heard Carved and Burned of the same runs
  and chose Burned ("sounds better"), so it is the default from 25 Nov. Under it the telemetry's `flame` is a burn in
  the collectors, not the crackle.
- Cost: one Wiebe step per burn alight, about 8 ns while one burns; nothing otherwise. `exhaust.model` 0 never reads
  it (the engine identity test passes with `bang` at its default).

### The burns' gas is hot: what a housing books (26 Nov; the next window's item 1)

A burn's volume source (`bangStep`: (γ−1)/(γp) of its heat's rate) is the expansion its heat makes, so the gas
that reaches a turbine housing is that much hotter at the same pressure. Before this the housing booked the pipes'
volume at the collector's own temperature (`heat.tUse[XH_COLL]`, which never saw the heat), so the burns' volume
became mass the valves never let out.
- **Measured before** (bench, the rally five held on its spark-cut limiter, BangTest's `measure` case, every 0.1 s):
  - the housing booked 3.1 to 3.5 times the valves' mass;
  - `bookLet`'s `tLet` was pinned at its 0.3 floor;
  - the housing was at 6.5 to 7.0 bar abs and 750 K.
  - On the tour (the soak's red, 223f4301): port 7.54 bar, turbine inlet 8.94, `valve.dpWave` −350 kPa against the
    ±300 kPa bound.
- **Now** (`Slots::houseT`): the housing receives T (1 + q_burns / q_ports) over the banks that feed it. That is
  T + Q̇/(ṁ cp) in the mean.
  - Both flows are means over some 20 ms, a bank each: the burns' source `net.popQ`, and the flow its ports let into
    the collector, the valves' alone.
  - At most 2600 K, about a stoichiometric charge's adiabatic flame from a hot start (judgement).
  - The volume source is unchanged. The housing holds that enthalpy as its own `t`, and Boost's turbine expands it.
- **Measured after:**
  - **Bench:** the books average 0.99 (0.46 to 1.49 per window); `tLet` 0.36 to 0.51; the housing 2.0 to 5.2 bar abs
    at 1700 to 2300 K; the port about 2.7 bar; `dpWave` −60 to −133 kPa.
  - **Tour, 7.5 to 8.9 s:** port mean 3.33 bar (max 4.98); turbine inlet mean 3.16 bar (max 6.84) at 1750 K; boost
    mean 1.27 bar (max 1.98); `dpWave` mean −100 kPa (min −155; −174 over the whole tour).
  - **Tour, Carved, for comparison:** port 2.78 (max 3.84), turbine inlet 2.66 (max 4.72) at 633 K, `dpWave` −62
    (min −92).
- **The click at 8.74 s** (0.527 against Carved's 0.098 in 8.6 to 8.9 s) is gone: 0.088 there.
  - The tour's largest steps are now the limiter's bangs, rising over 3 to 4 samples: 0.275 at 7.92 s, 50 ms after
    9.8 MW of heat.
  - X's corpus case (every archetype through the tour, as built): the rally five reads step 12.7 (33.7 at 223f4301),
    and all thirty pass.
- **Sound:**
  - Only builds with a turbine and burns change. Of the five clip pairs the owner chose by, the rally five's is the
    only one that changes: first.json's two burn nothing; the V8s have no turbine.
  - The rally five's limiter, at the Tailpipe camera: the loudest 10 ms windows (99th percentile) are 2.0 dB lower,
    the whole stretch's rms 2.2 dB higher, the peak the same.
- **Not in this change:** builds without a turbine still meet their losses downstream at the cold gas's density. The
  flat-plane V8 on its spark cut reads a port mean of 1.37 bar abs Burned against 1.13 Carved, and `dpWave` −20
  against −11 kPa (min −37): small beside the bound.
- **Whose:** the pipes' and the housing's inlet are G's. What Boost's turbine does with the hot gas, and its books,
  are N2's.
- **The auto level:** L's dense-burn rule (5dc80fc4) keeps the limiter's denser bangs from holding the body down.
  With both in, the rally five's lift reads Burned +0.8 dB against Carved (L's measure; −4.5 without the rule).

## The heat's settle takes a cycle's gas (26 Nov; N1's finding)

Card C's settle (`spec.exhaust.heatSettle`, `Exhaust::settleHeat`) soaks every wall to the gas at the ports and the
mass flow. It took the gas of the last 16 samples (gas(): 0.33 ms, against a 20 ms cycle at 5879 rpm) and a mass flow
smoothed over about 2 ms. So it caught a blowdown or the stroke's cool tail by timing, and soaked the walls to it.
- **Now:** gas() keeps the ports' energy and mass flow through two first-order means in cascade, each over a
  four-stroke cycle at the engine's speed (120/rpm, 10 to 250 ms). That leaves some 2.5 % of the firing's swing. The
  settle takes T = e/m and the flow m from them; the running 0.8 s tPort and the soak are as they were.
- **Measured** (tee-dyno --predict, 48 points, the two-stage, old against new):
  - the port temperature at the points' ends scattered from 1097 to 1606 K point to point; now it rises smoothly from
    1200 to 1489 K, neighbours within about 15 K;
  - 5915 rpm: 181.7 → 189.1 N m. Table P's "notch" is now about 3 N m against its neighbours.
- **first.json, 16 points:**
  - most points move under 1 %;
  - two points at the turbo's spool move: 1733 rpm 249.7 → 382.3 N m and 2833 rpm 353.9 → 385.9;
  - held 20 s at 1733 rpm (the old code's stepped dyno), it read 383.4 N m at its 120 kPa target, and this note
    said the old points were wrong because the old settle left the turbo unspooled. **Withdrawn (26 Nov):** the 383.4
    was not an independent truth: that stepped dyno ran a mass-weighted settle of its own. C's ramps with no settle
    put first.json's one steady state at 1733 rpm on main at about 272 N m and 41 kPa (from above in some 20 s, from
    below over some 4 minutes; not bistable), so both settles read high there (293 at 51 kPa on 9e52a186).
- **Weighted as the running path weights (26 Nov, C's finding):** weighted by mass, the settle landed hot. On the
  two-stage at 5879 rpm its port heat was 5.3 % over a 90 s hold and its torque 3 N m low (187.3 against 190.4). The
  running tPort weighs each 16 samples alike, and the blowdown is both the hottest and the heaviest gas. So the settle
  now takes the time-weighted cycle mean of the ports' gas (the flow stays the cycle's mean):
  - the two-stage at 5879 rpm reads 189.4 N m and 15151 W against 15090 held;
  - first.json at 3200 rpm reads 16047 against 16044.
  - Guarded by "the exhaust's heat settles where the long run lands" (the settle's gas within 2 % of the running
    path's: +0.15 % and +1.48 %; weighted by mass, +4.5 % and +8.1 %).
  - Whether the running path should itself weigh by mass (the heat the gas carries) is the next window's, with a
    prediction across the builds. The physics is below; the measurement is in the next window's queue (item 6).

## Which mean the port's gas should be (26 Nov; the physics for item 6, nothing built)

The running `tPort` (`Exhaust::gas`) is e/m within each 16 samples, then a 0.8 s first-order mean of those values,
each weighed alike: a **time** mean of 16-sample blocks. The blowdown is the hottest and the heaviest gas and passes
in little of the time, so this mean lies below the mass (enthalpy) mean: by 5 to 88 K in the mean port gas across the predictor's curves on six builds (below). The question is what receives `tPort` and which mean each
receiver sees. In model 1, one number, `exitTemperature()` (tPort after the port's wall), goes to all of them:

| receiver | where | what it computes with the gas's T | the mean it needs |
|---|---|---|---|
| the port's wall | `portLoss` | q = m cp (T - T_wall)(1 - e^-NTU), m the mean flow | mass: the heat carried is the sum of m cp T, not of T |
| the pipes' walls and the gas's cooling down the chain | `XHeat::soak` (heat2.fm) | gas out = T_w + (T - T_w) e^-NTU on the mean flow; the wall's gain gIn = NTU m cp | mass, for the same reason: an enthalpy balance on the mean flow |
| the pipes' sound speed and density | `tUse` → `cOf`, the ends' `design` | c = sqrt(gamma R T) of each section's gas | about the mass mean (see below), not the time mean |
| the turbine's inlet | `net.jT[jHouse]` = `tUse[XH_COLL]`, `houseT` | the housing's T in the turbine's flow and energy | mass: a turbine's work is the enthalpy flux, m cp T |
| telemetry `portTemperature` and the "Port gas, mass mean (information)" rows | spec help, this doc, P2's archetypes | read as the mass mean | mass, by its own label |
| the thermocouples | `Exhaust::thermocouple` | their own per-sample local gas; tPort only as the port's mean drop | the bead's heat-transfer weighting is their own; the drop is an enthalpy drop (mass) |

**The walls.** Convection is h (T_gas - T_wall) at each instant, and h rises with the flow (Re^0.8 in a pipe), so a
wall's mean gain weighs the gas by about m^0.8: nearer the mass mean than the time mean. The model takes it one
better already: its gain is NTU m cp (T - T_w) on the mean flow, which is exact only for a mass-mean T.

**The sound speed.** A pipe at any instant holds slugs of gas in the order they passed. A slug of mass m_i at T_i
fills a length proportional to m_i T_i and a wave crosses it in a time proportional to m_i T_i / sqrt(T_i). Over a
pipe, the wave's mean speed is then sum(m T) / sum(m sqrt T): with T = T_m (1 + x), x's mass mean 0, that is
sqrt(T_m) (1 + <x^2>/8 + ...), a temperature T_m (1 + <x^2>/4): the mass mean and a small excess (some 1 % for a
20 % swing), never the time mean below it. (The pressure is near uniform across the slugs; judgement, since a
blowdown's pressure differs.)

**Anything rightly served by the time mean?** Only an instrument in the gas: a bead that sits there all the time.
The model already has that, `portThermocouple` and `headerThermocouple`, weighted as a bead's heat transfer from
the per-sample gas, not from tPort. Nothing else in model 1 takes a time mean on purpose. So the time-weighted
`tPort` is a fault, not a second quantity, and item 6 (`eRun`/`mRun`: the 0.8 s means of the energy and the mass,
tPort their ratio) cures it with one temperature: the thermocouples remain the one time-type reading, as now. The
help of `portThermocouple` ("portTemperature is the mass mean of the gas, hotter") and this doc's "Mass means and
thermocouples" already call it the mass mean, so the change makes those words true rather than changing them.

**What it is not.** The reference (`sim/ref/engine.fm`, model 0) weighs by time and drives its turbine by
mdot tPort / 1100. That is the reference's and stays: item 6 belongs to model 1 only, or model 0 stops being the
reference (the scratch tree measured below changes `gas()` for both models; the code to propose gates it on
`model != 0`, and the null test re-reads it).

**What the measurements say** (scratch tree, main against mass-weighted; all 31 archetypes and the top-level files):
- the port's mean gas rises 1 to 10 % (5 to 88 K by the predictor's curves), the thermocouple rows 1 to 3.5 %;
- no idle row moves more than 1 %;
- five builds cross an edge, four of them the speed of peak torque on a flat-topped curve: the turbo builds' peaks
  move 19 to 32 % as the hotter gas brings their boost in earlier (the rally five's peak leaves its band). The
  turbines' sizes are guessed, so these thresholds say nothing for or against the change; the cited rows cannot tell
  the two means apart. The reason for the change is the table above.

**The order (the lead's ruling, 26 Nov):** item 6 lands in the same window as N2's turbine match (item 9) and before
it, never alone; the settle goes back to the cycle's mass mean with it (`eCyc/mCyc`, as 9e52a186 had), and C's long
hold and "the exhaust's heat settles where the long run lands" are read again on the pair.

## Stated bores: the least, 10 mm (X-038, X-029)

Every pipe bore a person or a build can state (`primD`, `colD`, `mainD`, `crossD`, `resD`, `bypassD`, `tailD`) takes
0 to mean derived. A stated bore under **10 mm** is not used: the builder raises it to 10 mm (`XBuild::pipeBore`, at
every read site). `tel.exhaust.boreRaised` says which kinds were raised, a bit a kind (1 primary, 2 collector, 4 main
pipe, 8 crossover, 16 resonator, 32 bypass, 64 tailpipe), and `tel.exhaust.boresRaised` how many; they are ready for
the EXHAUST workspace's warning. Every bore at 0, or at or above the floor, is bit for bit as before.

- **The floor is the part's, not the solver's.** The smallest road exhausts, on 50 cc scooters and mopeds, run some 16
  to 19 mm headers (judgement from typical parts; no single source), so 10 mm sits under any real engine's pipe.
- **The solver's own limit, measured** (each finding's reproduction, tee-fuzz at its seed for 10 s, the tiny bore
  raised to the floor shown):

  | build | as found | 2 mm | 5 mm | 10 mm |
  |---|---|---|---|---|
  | X-038, a 0.011 mm primary (seed 298) | the guard at 0.41 s | ok | ok | ok |
  | X-038, a 0.072 mm tailpipe (seed 167) | the guard at 0.26 s | ok | ok | ok |
  | X-029, a rotary's 0.62 mm primary (seed 245) | the guard at 0.31 s | the guard at 0.30 s | ok | ok |

  The limit lies between 2 and 5 mm on these three cases; this is not a sweep. The 10 mm floor stands at twice that
  margin. With the floor in, all three run as found: 1.47, 1.33 and 0.56 R.
- **In content/**, only X's finding builds state a bore under the floor: x029 and the two x038 builds (X-040's runner
  is the intake's, not this floor). A person's saved build under it loads and plays with the bore raised, and the
  telemetry says so.
- **Refusal at input** (the loader and the inspector refusing a value in (0, floor), as they refuse one out of its
  range) is the next window's, card C's. The FMinus compiler is the owner's and not ours to change, so it adds no new
  `fminus::` attribute: the rule lives in the host (a table beside the Spec's lint in host/src/Fields.cpp) or in an
  attribute namespace the compiler already passes through.
- The intake's twin (X-040: runner 8 mm, snorkel 10 mm, the same rule) waits for a card whose paths hold
  sim/intake/intake.fm.

## Design note for N2: the back pressure after a turbine (25 Nov; a note, nothing built)

**What sets it.** Downstream of a turbine the builder has the downpipe, the converter, the main pipes, the muffler and
the tail. Its mean pressure at the downpipe's head over the air outside (`XBuild::pConv`, a 0.2 s mean of the head
junction) follows from two kinds of loss:
- **Turbulent losses** (the junctions' sudden expansions and contractions, the muffler's passages, the tail's exit):
  each is K ρu²/2 = K ρQ²/(2A²). For a given mass flow ṁ that is ∝ ṁ² T / p, since ρ = p/(RT).
- **Laminar losses** (a converter's substrate: its many fine channels; the reference's restriction in the pipes):
  ∝ μ(T) Q. That is the volume flow, so ∝ ṁ T / p, and more as μ rises with T.

So Δp ≈ a·Q + b·ρQ² with Q the volume flow at the gas's own state: two terms, not one. A purely quadratic K
misses the low-flow end.

**Measured on first.json** (the 2.0 turbo four; engine dyno; throttle 0.3, 0.6 and 1.0 at 2000 to 6500 rpm; 3 s
each; BangTest's `measure` case):
- The downpipe head runs 4.6 to 35.2 kPa over the air, and the muffler's inlet 2.9 to 23.1 kPa.
- The main pipe's gas is 700 to 1110 K. ṁ is 0.039 to 0.107 kg/s, estimated from the ports' volume flow at the head's
  pressure and the main pipe's temperature, so it is approximate.
- A two-term fit, a = 1.06e5 Pa per m³/s and b = 3.63e5 Pa per (kg/m³)(m³/s)², **first.json's alone** (another build's
  chain gives other values), gives an rms error of 2.2 kPa over the estimated mass flows above. It is
  worst at the lowest flow: 4.6 measured against 8.9 fitted.
- A quadratic alone gives 4.8 kPa rms.

**What I'd recommend** (judgement):
1. In the running engine, a turbine on the builder needs no formula. Its housing's node already sees the downpipe's
   pressure through the network (XJ_HOUSE into the main pipe). The back pressure there is the pipes' own, at the
   gas's temperature, with its pulses.
2. For N2's off-line work (a map's match, a bake), where the turbine needs a quasi-static outlet pressure:
   - use the two-term form, Δp = a·Q + b·ρQ², with a and b fitted per build from a short dyno sweep of `pConv`;
   - or read `pConv` from the running exhaust at the operating point, which is what the bake's runs would do anyway.
   - A single constant or a quadratic alone will be wrong by a few kPa across the map. That is some 2 to 4 % of a
     turbine's expansion ratio at high load: small, but systematic.
3. Not measured: a diesel's or a twin-turbo's chain, a hot-film or particulate filter (none is modelled), and how much
   of the head's mean is the converter's alone. A per-element breakdown needs the junctions' losses read one by one,
   which I can do if N2 wants it.
