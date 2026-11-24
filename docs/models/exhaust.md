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

## Glide and rebuild (dragging a slider)

These fields glide toward the Spec with a 0.2 s time constant (`XBuild::glide`). The pipes' delays, areas and
losses follow them every control step in `shape()`, with no rebuild and no fade:
- every length: primLen, colLen, mainLen, crossLen, postLen, resLen, linkLen, mufLen, tailLen, gateLen;
- every bore: primD, colD, mainD, crossD, resD, bypassD, tailD, gateD;
- colVol, resHz, resVol, mufSize, primSpread, primStepAt, primStepD.

These rebuild under the exhaust's fade (6 ms down, 12 ms up):
- the kinds: converter, resonator, mufKind, bypass, tips, exitAt, crossKind, crossAt, sides;
- the tables: primaryTo, collectorTo;
- a size moving between 0 (the layout's) and a value of its own (for mufSize, -1);
- a collector's volume appearing or going, and the primaries' step appearing or going.

The bypass valve's position already travels by its own rule over bypassTime, and the valve's rule fields act at once.
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
