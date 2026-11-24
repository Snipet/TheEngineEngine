# Q5: a design note, not code: the engine in parallel parts, and a cheaper fidelity

*Package Q, step 5, 2026-11-24. A study for the lead and the owners of `engine.fm` (D), the exhaust (G), the intake
(E) and the dyno's fast path (C). Nothing here is implemented. The numbers are this container's (Xeon 2.10 GHz,
quiet); the profile's are q1-profile.md's.*

## 1. Advancing parts of the engine 16 samples at a time, side by side

### What links what within a sample today

`Engine::sampleTick` (sim/README.md, "One sample") runs, in order: every pipe's arriving waves (`exh.read`,
`intk.read`); the crank's angle; each cylinder with its header's and runner's near-end waves; the whole exhaust from
the ports to the tails (`exh.step`); the runners' near ends and the plenum; the torques and the crank's speed; the
sounds. Three kinds of link:

1. **Through a pipe.** A wave written at one end of a `Duct` is read at the other `D` samples later. When every
   path between two groups is a pipe with `D >= 16` at its shortest, each group's next 16 samples need only what the
   other wrote at least 16 samples ago: the two can advance a part each, side by side, and the result is the same as
   sample by sample (the same arithmetic in the same order within each group).
2. **Through the crank.** Every cylinder reads the crank's angle step (`dDeg`) and adds its torque to one sum each
   sample; the crank's speed follows that sum. No delay at all.
3. **Through shared state.** Engine's `Noise` stream is drawn by the exhaust (valve, flow, jet noise), by Combustion
   at intake closing (in the cylinders' loop) and by the pops: the order of the draws is part of the sound
   (sim/README.md, "Noise"). Exhaust's `eSum`, `mSum` are summed by the cylinders.

### The pipes' shortest delays at 96 kHz

| Pipe | Length (V8, defaults) | Gas | Delay, samples | Shortest, with steepening (`D / 1.563` at the most) |
|---|---|---|---:|---:|
| headers (`prim`) | 0.4 m (0.61 to 1.63 times by HEADERS) | 600 to 750 m/s | 51 to 64 at 0.4 m | 33 (31 at the dial's shortest: 0.24 m, 20) |
| runners (`run`) | 0.30 m (bike 0.16) | 350 m/s | 82 (44) | no steepening: 82 (44) |
| collectors' pipes (`sec`) | 0.38 m (bike 0.25) | 600 m/s | 61 (40) | 39 (26) |
| the H or X (`cross`) | 0.25 m | 550 m/s | 44 | no steepening: 44 |
| the chambers' link (`mid`) | 0.12 to 0.20 m | 450 m/s | 26 to 43 | 26 |
| tails | 0.40 m (bike 0.10) | 400 m/s | 96 (24) | 96 (24) |

**Every pipe of the reference's systems is longer than 16 samples**, the headers at the HEADERS dial's shortest
included (20 samples). G's builder lets a person make shorter ones (a stub, a short link): the partition is legal
only where the builder reports the link's shortest delay `>= 16`, and a build that breaks it falls back to the serial
order (or the builder refuses a link under 16 samples between two groups, which at 96 kHz is 5.6 cm at 340 m/s).

### The groups

For a V8 with two banks (topologies 1 and 2), and the same shape for a V6, V10, V12, a flat engine:

| Group | What it holds | Its ends | V8 cost today (q1) |
|---|---|---|---:|
| Bank A | its cylinders and valves; the near ends of its headers and runners | headers (>= 20), runners (>= 44) | about 400 ns a sample |
| Bank B | the same | | about 400 |
| Exhaust side A | header far ends, its collector, pipe, converter, chambers, tail; its end of the H or X | headers, the cross (44) | about 440 (half of 880) |
| Exhaust side B | the same | | about 440 |
| Intake | runners' far ends, the plenum, the throttle | runners | about 60 |
| Serial | the crank and mechanics, the boost's control, Extras, the listener, every 16 samples' work | | about 300 |

Each `Duct` is split into its two halves (`readNear`/`write` at the near end by one group, `readFar`/`write` at the
far end by the other: the forward and backward waves already have separate filter states, `wsF`/`wsB`, `mF`/`mB`;
only `qAbs`, the flow's sum for `flow16`, is shared, and becomes one sum a side).

### What breaks the bit

- **The crank.** For the banks to run 16 samples alone they need the crank's angle step for those samples. Take the
  crank's speed as it stood at the part's start (or extrapolated from the last two parts' torques), advance both banks,
  then sum their torques sample by sample into the crank and correct the angle at the part's end. The crank's speed
  changes by `torque / J * dt * 16` over a part: at 500 N m on 0.2 kg m^2 at full throttle in first gear, 0.42 rad/s
  (4 rpm) a part: the angle is out by at most a few thousandths of a degree at a part's end, corrected at once. Not
  exact; physically nothing, but it moves every sample's last bits: judged against the floor (q2-levers.md).
- **The noise.** Each group gets its own stream (the exhaust's sides one each, the banks' combustion scatter one
  each). Not exact; the noise's statistics unchanged.
- **The pops, the scope, the cylinders' `eSum`/`mSum`**: summed per bank and added at the part's end.

So this is the "not to the bit" kind from the start, and it is judged as q2-levers.md proposes: within the floor of
the same engine with its air 0.0001 K warmer, on every pair of the null test, and listened to.

### What it costs to run a region

FMinus's `parallel { }` may sit in a `[[clang::nonblocking]]` `process` (it allocates nothing and takes no lock), and
runs on one pool for the process (a thread for each core but one). Measured here (`parallel { a.run(); b.run(); }`,
two tasks, 6000 regions a second as 16-sample parts at 96 kHz):

| Work in each of the two tasks | serial, us a part | in a region, us a part | over half the serial |
|---|---:|---:|---:|
| none | 0 | 0.61 | 0.61 |
| 1 us | 2.4 | 1.9 to 2.2 | 0.7 to 1.0 |
| 5 us | 9.9 | 5.7 to 6.2 | 0.8 to 1.2 |
| 10 us | 19.9 | 11.0 to 11.2 | 1.0 |
| 20 us | 39.9 | 21.0 to 21.1 | 1.0 to 1.1 |

**About 1 us a region.** A V8's part is 16 x 1.9 us = 30 us today; a region of four tasks (two banks, two exhaust
sides) at about 6.5 us each, then the intake and the serial 5 us: some 13 us a part against 30, **2.3 times
less latency, for 5 to 8 % more processor time in all** (the region's overhead on every thread). Found on the way:
two modules side by side in memory that two tasks write share a cache line at their boundary, and the false sharing
cost up to 40 % (a task's state wants padding to 64 bytes, or the groups' modules kept apart).

### What it is worth

It does **not** lower the cost in "% of a core" that the budgets count: the same work, plus the regions'. What it
lowers is the time a buffer takes, which is what a plugin's meter and a dropout care about; on the owner's Mac (8 or
more cores) a turbo V8 with a dual system at 1.6 R would take some 0.7 R of the audio thread's time. The order of work
in `engine.fm` changes for it (D), every `Duct` gains its two halves (G), the intake's runners likewise (E). **Advice**:
do it after wave 2's integration settles (the order of work is moving every hour now), and only if the single-thread
work below does not bring the heavy builds within budget: it is the larger change and gives up the bit.

## 2. Cheaper per pipe, on one thread: the pipes as arrays

The pipes cost 23 % (reads and writes) plus 12 % (the junctions) of the V8. Each `Duct::read` is 12 to 14 ns: the
Lagrange read, the tracker, three wall shelves each way (one dependent chain), a few branches; FMinus vectorises it
only in pairs (forward and backward). With the pipes' states as arrays across pipes (`float wsF[3][32]` for all of
an engine's pipes, the delay lines as now) the shelves of eight pipes run as one `float8` chain: the same arithmetic
per pipe in the same order, so **exact**, and some 3 to 4 times cheaper for the wall and the tracker (the reads
themselves are gathers and stay scalar). Estimated 8 to 12 % of R on the V8, more on G's 32-pipe builds where it
matters most (G's +0.31 R is nearly all per-pipe cost). G's builder already holds its pipes in a network
(`exhaust/network.fm`); that is where the arrays belong. This is the change I would make first.

## 3. A second fidelity: 48 kHz inside, for the dyno ribbon

The engine's rate is one number (`Engine::setupAll`: `rate = 96000.0f`); every module takes it. Changed to 48000 in
a copy (with the exact valve exit applied; the host at 48 kHz, so no resampler), measured against the same copy at
96 kHz:

**Cost**: 0.48 of 96 kHz's a second of sound (qbench: V8 free-revs 0.536, drive-car 0.476, defaults 0.497 and 0.426).
`tee-dyno` (11 points, 1300 to 6400 rpm, the V8): 11.9 s simulated in 2.29 s at 96 kHz, 1.69 s at 48 kHz (the dyno's
settling is the same number of seconds either way; its overhead between points does not halve).

**The torque curve** (tee-dyno, the same 11 points):

| | 96 kHz, peak | 48 kHz, peak | worst point | typical point |
|---|---:|---:|---:|---:|
| V8 | 670.7 N m at 4354 rpm | 671.3 N m | 1.2 % (1689 rpm) | 0.1 to 0.2 % |
| defaults | 472.9 N m at 3285 rpm | 471.9 N m | 0.6 % (1296 rpm) | 0.0 to 0.3 % |

The worst points are at the lowest speeds, where the big cam's lope makes a point noisy at either rate. **For a curve
the lower rate is good to about 1 %.** The sound is not: the third-octave bands near the top lose 13 to 24 dB (the
engine's own Nyquist is 24 kHz, its filters' highs gone), the levels and the speed stay within the floor.

**Not ready at 48 kHz**: F's boost machine hard-codes 96 kHz (`boost/boost.fm`: `mach.setup(96000.0f, ...)`;
`boost/gas.fm`: `kBoostDt = 1 / 96000`): a turbo build at 48 kHz would run its machine at the wrong time step. F
must take the engine's rate before a 48 kHz tier can carry turbos. And `tee-dyno` on the Large Diesel gave negative
torques at 96 kHz already (from 1180 rpm up: the governor or the settling, card C's to look at), so the diesel's
curve could not be compared.

**For card C's fast path** (a whole curve within a second of an edit): at 48 kHz the V8's 11-point curve takes 1.7 s
on this core with tee-dyno's present settling; the rate halves the engine's part of it, the settling time per point
is the other half of the budget. Fewer pipe filters (the wall's shelves off: they shape the highs, not the tuning)
would take a further 10 to 15 % off at 48 kHz, not measured. The compile is not in it (a Spec edit recompiles nothing).

## 4. In order

1. The exact patches (q3-q4-changes.md): 11 % of R now, no decision needed beyond the owners'.
2. The pipes as arrays (G, exact): 8 to 12 % more, most where the builder adds pipes.
3. The two-pass valve, if a person hears no difference (G): 10 to 13 %.
4. 48 kHz inside for the dyno ribbon (C, D; F's rate first).
5. The parallel parts (D, G, E), only if the heaviest builds still need it after 1 to 3.
