# Q1: where R goes

*Package Q (performance), step 1, 2026-11-24, at c5636c5 (the split simulator as K3b left it: every module the
reference's, the null test identical to the bit). Measured with `tee-bench --profile --engine` (a sampler 4000 times a
second, each sample looked up in FMinus's code ranges by function) and with `docs/measurements/tools/qbench` (two
copies of the simulator, timed side by side; how to build it: [tools/build.sh](tools/build.sh)).*

**The machine**: this cloud container, an Intel Xeon at 2.10 GHz (Emerald Rapids, AVX-512), 4 cores, **quiet**: nothing
else running (load average under 1 between runs), so these numbers are lower than K2's (which were taken beside other
agents' builds). On this quiet machine, `tee-bench --engine` (the least of 3, wall time of the sounding calls):

| build | scenario | simulator, ns a 96 kHz sample | reference, ns | ratio |
|---|---|---:|---:|---:|
| v8 | free-revs | 1841 | 1789 | 1.029 |
| v8 | drive-car | 1897 | 1860 | 1.020 |
| large-diesel | free-revs | 1407 | 1403 | 1.002 |
| large-diesel | drive-car | 1406 | 1394 | 1.008 |

**R on a quiet core of this container is therefore about 1.8 us** (the reference V8), not 2.3: the budgets are in
R, so read every share below as a share, and compare ns only within one table. A run to run spread of 5 to 15 %
(the most over the least of 3 to 5 runs) is normal here even when quiet, so a change under about 2 % needs many
runs before it is believed.

## By module and by function

The profile's ns include the sampler's own interruptions (about +10 % over the plain figure). "V12" is
`content/builds/engines/ref-15-v12-60.json`; "diesel" the Large Diesel (an inline six with twin turbos).

| module, ns a 96 kHz sample | V8 free-revs | V8 drive-car | V12 free-revs | V12 drive-car | diesel free-revs | diesel drive-car |
|---|---:|---:|---:|---:|---:|---:|
| **total ns** | 2098 | 2145 | 2871 | 2604 | 1494 | 1570 |
| Duct | 586 | 606 | 840 | 748 | 314 | 314 |
| Valve | 508 | 501 | 734 | 651 | 341 | 348 |
| Cylinder | 304 | 321 | 474 | 433 | 213 | 226 |
| Exhaust | 288 | 290 | 344 | 331 | 181 | 179 |
| Engine | 127 | 100 | 149 | 141 | 84 | 88 |
| Listen | 108 | 122 | 110 | 111 | 93 | 109 |
| Intake | 56 | 65 | 70 | 73 | 48 | 44 |
| Hall | 41 | 50 | 57 | 41 | 44 | 61 |
| Boost | 1 | 1 | 1 | 1 | 100 | 110 |
| Extras | 32 | 29 | 32 | 27 | 31 | 39 |
| Car | 21 | 25 | 22 | 22 | 18 | 21 |
| Mechanics | 18 | 23 | 26 | 18 | 18 | 19 |
| Heat | 3 | 6 | 3 | 3 | 2 | 3 |
| Pop | 2 | 2 | 3 | 3 | 1 | 1 |
| Ecu | 0 | 1 | 1 | 1 | 1 | 1 |
| Combustion | 0 | 0 | 1 | 1 | 0 | 1 |

| function | V8 free-revs | V8 drive-car | V12 free-revs | V12 drive-car | diesel free-revs | diesel drive-car |
|---|---:|---:|---:|---:|---:|---:|
| Valve::flow | 508 | 501 | 734 | 651 | 341 | 348 |
| Duct::read | 391 | 406 | 583 | 513 | 166 | 161 |
| Cylinder::step | 267 | 281 | 391 | 352 | 183 | 189 |
| Exhaust::step | 253 | 249 | 309 | 304 | 154 | 153 |
| Engine::sampleTick | 117 | 90 | 135 | 131 | 79 | 77 |
| Duct::write | 86 | 104 | 129 | 120 | 63 | 62 |
| Intake::plenum | 54 | 64 | 69 | 72 | 47 | 44 |
| Cylinder::volumeSlope | 38 | 40 | 84 | 81 | 30 | 37 |
| Hall::process | 41 | 50 | 57 | 41 | 44 | 61 |
| Listen::resampled | 45 | 49 | 50 | 50 | 43 | 46 |
| Boost::sound | 1 | 1 | 1 | 1 | 98 | 107 |
| Duct::set | 34 | 27 | 44 | 41 | 24 | 30 |
| Duct::flow16 | 34 | 34 | 40 | 41 | 24 | 24 |
| Extras::tick | 31 | 28 | 32 | 26 | 31 | 38 |
| Listen::finish | 26 | 35 | 30 | 29 | 27 | 27 |
| Duct::back | 27 | 22 | 32 | 25 | 23 | 25 |
| Mechanics::torques | 18 | 23 | 26 | 18 | 18 | 19 |
| Listen::listen | 20 | 22 | 14 | 20 | 11 | 20 |
| Car::step | 11 | 12 | 11 | 14 | 10 | 14 |
| Listen::mix | 13 | 13 | 13 | 9 | 10 | 12 |
| Duct::pack | 13 | 12 | 12 | 8 | 12 | 11 |
| Exhaust::soak | 11 | 13 | 10 | 10 | 9 | 10 |
| Exhaust::bands | 13 | 11 | 13 | 7 | 6 | 5 |
| Exhaust::pipes | 8 | 12 | 10 | 8 | 9 | 8 |

## Per sample and per 16

Everything that runs every 16 samples (`Engine::control16` and what it calls: `Duct::set`, `back`, `pack`,
`flow16`, `Exhaust::pipes`, `bands`, `soak`, `Heat::control`, `Listen::block`, `Car::drive16` ...) comes to
about **150 ns a sample on the V8 (7 to 8 % of the total)**, nearly all of it the pipes' coefficients (`Duct::set`,
`back`, `pack`, `flow16`: 105 to 120 ns, each a handful of `expf`, `sinf`, `cosf` a pipe every 16 samples,
recomputed whether anything moved or not). The rest, about 92 %, is per sample: the cylinders and their valves (about
40 %), the pipes' reads and writes (25 %), the exhaust's junctions and noise (12 %), the engine's own loop (6 %),
the listener and the resampler (6 to 7 %), the intake's plenum (3 %).

## The five places worth attacking

| # | Where | V8 cost (share) | The change | Could save (of R) | Bit-exact? |
|---|---|---:|---|---:|---|
| 1 | `Valve::flow` (`exhaust/duct.fm`, G's; called from `Cylinder::step`, D's) | 500 ns (24 %) | the three passes a solve where two converge; one `sqrtf` and two divisions a pass are on one dependent chain | 5 to 8 % | no: -80 dB route |
| 2 | `Duct::read` and `write` (G's) | 490 ns (23 %) | the wall's three shelves and the tracker on every pipe each way; per-pipe 21 ns for 29 pipes on the V8 (21 exhaust, 8 runners) | 3 to 6 % | partly |
| 3 | `Cylinder::step` and `volumeSlope` (D's) | 300 ns (15 %) | `sinf`, `cosf`, `sqrtf` a cylinder a sample for the volume (a table by angle); `expf` in the burn | 2 to 4 % | no |
| 4 | `Exhaust::step` (G's) | 250 ns (12 %) | the junctions' `lossFlow` (a `sqrtf` each), the cones' and flow noise's filters | 1 to 3 % | partly |
| 5 | the pipes' coefficients every 16 (`Duct::set`, `back`, `pack`, `flow16`) | 105 to 120 ns (5 to 6 %) | recompute only what moved (bit-exact for `set`'s tracker and `pack`); `back` and `flow16` follow the gas and change every time: a cheaper exponential or every 64 samples | 1 to 4 % | partly |

Beside these: `Listen::resampled` 45 ns (2.4 %; mine, step 3); `Boost::sound` 100 ns on the twin-turbo diesel (7 % of
that engine, F's); `Engine::sampleTick`'s `expf(0.8 logf(...))` for the valves' click level, a function of
`crank.rpm`, which only changes every 16 samples (bit-exact to move: D's).

## How far to trust the attribution

- **Inlining**: FMinus's code ranges are by function, and an inlined callee's time is its caller's. Here
  `Cylinder::step` has swallowed `areaAt`, `gammaAt`, `passed`; `Exhaust::step` has `node`, `coneNoise`,
  `lossFlow`, `lossHold`, the biquads and the noise; `Engine::sampleTick` the crank's, the clicks' and
  `Intake::read`. `Valve::flow` and `Duct::read`/`write` are **not** inlined (their own rows), so the two largest
  rows are clean. `Valve::tOf` (the psi table) is inside `Valve::flow`'s row.
- **perf** could not be used: it is not installed and installing it was refused in this container (and FMinus's JIT
  writes no perf map, so it would see anonymous code anyway).
- **A second way, by adding work instead of removing it** (removing a module changes the engine's dynamics, so its
  time would not be comparable): a copy of the simulator in which every valve solve is done twice (the duplicate on a
  pressure 1e-7 off, its result summed into a field nobody reads, so the real one and the sound are untouched: the
  output stayed identical to the bit). Its extra cost, qbench, 5 runs:

| build, free-revs | sampled share of `Valve::flow` | marginal cost of a second solve |
|---|---:|---:|
| V8 | 508 ns | 293 ns |
| Large Diesel | 341 ns | 167 ns |

  The duplicate costs 50 to 60 % of what the sampler gives the function: the solves are long chains of dependent
  divisions and roots, and an out-of-order core overlaps a second, independent chain with the first. So the profile's
  share is the time the function **occupies**; what removing it would **save** lies between the two columns. Read the
  savings in the table above with that in mind: they use the lower column.

## Reproduce

```
build-dev/bin/tee-bench --profile --engine --build v8 --scenario free-revs --all
build-dev/bin/tee-bench --engine --builds v8,large-diesel --scenarios free-revs,drive-car
docs/measurements/tools/build.sh && build-dev/q/qbench --b <a copy of sim> --builds v8 --scenarios free-revs --repeat 5
```
