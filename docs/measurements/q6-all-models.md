# Q6: every landed model on, against the 1.6 R budget

*Package Q, 2026-11-24, 16:20 UTC, at the branch tip of 16:10 (G's exhaust builder not yet wired: `exhaust.model` 1 does
nothing yet). Builds: X's archetypes (`content/builds/combo/turbo-v8-dual.json`, `turbo-six.json`) with no switch, with
one switch, and with every switch on as X's tests define it (each section's `model` 1, `engine.custom`,
`engine.wallHeat`): written by a script to `out/q/b/` (not committed). qbench with A and B the same program, 4 runs a
side, so each figure is the least of 8 renders of 8 s; one order only (A = B). R: `tee-bench --engine`, the
reference V8, 5 runs, in the same session: 1921 ns on drive-car, 1865 ns on free-revs. Spreads 4 to 30 %: read the
one-switch rows to about +-0.05 R.*

## The two builds

| Build | Scenario | No switch | Every switch on | In R |
|---|---|---:|---:|---:|
| Twin-turbo V8, dual exhaust with a crossover | drive-car | 1981 ns | 2761 ns | **1.03 R -> 1.44 R** |
| | free-revs | 1993 | 2736 | 1.07 R -> **1.47 R** |
| Turbo six (one turbo, AWD coupe) | drive-car | 1513 | 2116 | 0.79 R -> **1.10 R** |
| | free-revs | 1447 | 2051 | 0.78 R -> **1.10 R** |

CI at 08ae4f8 (X's combo lines) read the turbo six with every switch on at 1.04 R on the Linux runner and 1.20 R on
Windows: this container's 1.10 R sits between.

## One switch at a time (twin-turbo V8, drive-car; least of 8, against no switch's 1981 ns)

| Switch (package) | ns | over no switch | in R |
|---|---:|---:|---:|
| `boost.model` (F, the machine) | 2241 | +260 | +0.14 |
| `sound.model` (L, the listener) | 2145 | +164 | +0.09 |
| `car.model` (I, the vehicle) | 2138 | +157 | +0.08 |
| `fuel.model` (H, the burn, Woschni walls, Chen-Flynn friction) | 2106 | +125 | +0.07 |
| `valve.model` (E, the valvetrain) | 2098 | +117 | +0.06 |
| `voices.model` (M, mechanical voices) | 2097 | +116 | +0.06 |
| `engine.custom` (D, the custom engine) | 2081 | +100 | +0.05 |
| `engine.wallHeat` (D) | 1998 | +17 | +0.01 |
| `ecu.model` (H, maps) | 1996 | +15 | +0.01 |
| `exhaust.model` (G: not wired yet) | 1992 | +11 | 0 (noise) |
| `intake.model` (E) | 1990 | +9 | 0 (noise) |
| sum of the singles | | +1091 | +0.57 |
| **all together** | **2761** | **+780** | **+0.41** |

The singles add to more than all together (switches share work, e.g. the custom engine's geometry and the burn's
walls, and the noise of 4 runs is in every row): read the singles as shares, the "all" row as the cost.

## Where "no margin" stands

- **Today**, every landed model on: the typical build (twin-turbo V8, dual exhaust) **1.44 to 1.47 R against the
  1.6 R budget**: inside, with 0.13 to 0.16 R to spare.
- **When G's builder lands** with its dual X, stubs and valves (+0.31 R, G's own figure from a dev build on a shared
  host): **about 1.75 to 1.78 R: over the budget by 0.15 to 0.2 R**, unless the builder comes in cheaper than that.
- What is in hand against it: the pipes as arrays (exact, `proto/duct-lanes.patch`, waiting for G: about 0.07 R on
  this engine, more with the builder's extra pipes); `TEE_FAST`'s two-pass valves (0.13 to 0.15 R on a V8, waiting
  for the owner's ears). Both together about 0.22 R: back to about 1.55 R, just inside.
- **The owner's core** at R = 40 % (their slower figure): 1.47 R today is 59 %, 1.78 R after the builder 71 %. Only
  Economy (48 kHz inside, 0.49 of the cost: about 0.87 R, 35 %) is certainly under 60 % there.
- **Compile time** has grown with the packages: 3.7 to 4.1 s here for 465 kB of machine code (it was 1.4 s and 158 kB
  for the reference): **at the 4 s limit on this machine**. The -O0-first launch (0.13 s at the reference's size, q2)
  is becoming a need, not a nicety.

## Reproduce

```
python3 (the script in this page's commit message) -> out/q/b/*.json
build-dev/q/qbench --a sim --b sim --builds out/q/b/turbo-v8-dual-none.json,out/q/b/turbo-v8-dual-all.json \
    --scenarios drive-car --repeat 4 --seconds 8
build-dev/bin/tee-bench --engine --builds v8 --scenarios drive-car,free-revs --repeat 5
```

## The exhaust builder in the engine (added at about 16:55 UTC, after G's wiring cf267fb)

qbench, A = B, least of 8 renders of 8 s; R = the reference V8 in the same session (1854 ns free-revs, 1955 drive-car):

| Build | free-revs | drive-car |
|---|---:|---:|
| `exhaust.model` 0 (v8) | 0.99 R | 0.95 R |
| model 1, the reference's topology (`content/builds/exhausts/reference-topology.json`) | 1.13 R (+0.13) | 1.09 R (+0.14) |
| model 1, dual X, quarter-wave stubs, valved mufflers (`dual-x-resonated-valved.json`) | 1.43 R (+0.44) | 1.32 R (+0.37) |

G's own (tee-bench, least of 3, free-revs): +0.17 and +0.44 R: the same within the noise. The twin-turbo V8 with
that exhaust and every model on is therefore about 1.85 R. The builder's overhead at the reference's own topology is
its generic junction path (profile, drive-car: `xNode` 113, `XNet::step` 101, `solveNode` 94, `solvePair` 61,
`solveOpen` 51, `builderStep` 47 ns, the bands, shape and heat some 60, against model 0's specialised `Exhaust::step`
of about 250) plus 115 ns more in the pipes' reads and writes. The pipes as arrays inside the builder (`XNet::read`
over `used[]`, prototyped): exact, 0.981 / 0.986 at the reference topology, 1.023 / 1.001 on the dual X: nothing
measurable; not landed. Compile with the builder: 4.8 to 6.4 s here (586 to 597 kB of code).

**The junction path, first try (about 17:15 UTC)**: the builder's junctions solved grouped by kind (nodes, pairs, open
ends, the rest), each group in the junctions' own order, so the per-junction dispatch goes and the open ends' noise
draws keep their order: exact (reference topology and dual X identical to the bit), and **no faster**: 1.008 to 1.017
by wall time, 1.011 by processor time (7 runs). The dispatch is not the cost; what is left is the general node solve
over its branch lists (`xNode`, its gathers) and the per-pipe work, spread thinly. Not landed. Specialised two- and
three-arm node solves (as the reference unrolls them) are the remaining exact idea, a larger rewrite, unproven.

**X-016 (fuzz seeds at 3 to 5 R)**: seed 1 reduced (X's reducer, its bound lowered to 2 R locally) keeps its cost with
no exhaust field set: a custom engine with four rotors, twelve faces on four shared ports, each face's valves solved
twice a sample (the reference's method for a rotary's shared ports); `Valve::flow` 45 % of its time. Not the builder.

## Rotaries: two, three and four rotors against a V12 (about 17:45 UTC)

P's rotary twin (`archetypes/rotary-twin-13.json`: 654 cc a rotor, its turbos, every model on) scaled to three and
four rotors (the same chamber, ports and exhaust a rotor; 1.96 and 2.62 L; rotors phased 120 degrees, and 0, 180,
270, 90 as `engines/rotary-4.json`: without the phasing the custom engine refuses the layout and plays the twin's
table, which first made every rotor count cost the same); without boost; and model 0 (only the custom engine on).
The V12 archetype (`v12-65.json`, twelve working chambers like the four-rotor) as the control. drive-car, qbench, 5
runs, R = 1936 ns (the reference V8, the same session). "After" is 4845579: a face's valve solve reused when its
inputs repeat the shared-port pass's to the bit (every pair identical to the bit; swapped order 1.06 to 1.25).

| Build | model 0, before -> after | every model on, no boost | every model on, turbos |
|---|---:|---:|---:|
| Twin rotor | 0.72 -> 0.60 R | 1.02 -> 0.88 R | 1.33 -> 1.22 R |
| Three rotors | 0.94 -> 0.75 R | 1.36 -> 1.12 R | 1.69 -> 1.39 R |
| Four rotors | 1.19 -> 0.92 R | 1.49 -> 1.24 R | 1.83 -> 1.58 R |
| V12 (control) | 1.24 -> 1.24 R | 1.86 -> 1.84 R (its own boost off) | - |

A sane four-rotor now costs less than the V12 with every model on (1.24 against 1.84 R) and its turbo build sits
inside the 1.6 R budget. Why it was dear: a rotary's faces share a port, and `Engine::sampleTick` solves each face's
valves twice a sample (for the port's share, then with the others' flows added), which is the reference's method;
with one face open on a port, the most common case, the second solve repeated the first exactly. X-016's 3 to 5 R
seed is a four-rotor with every other field random (its listener's camera, the vehicle's), not this.

P's naturally aspirated rotary (`archetypes/rotary-twin-na-side-port.json`, every model on), scaled to three and four
rotors the same way, after 4845579 (4 runs; R 1840 ns drive-car, 1913 free-revs): twin 0.72 / 0.70 R, three rotors
0.93 / 0.89 R, four rotors 1.13 / 1.07 R (drive-car / free-revs): about 0.2 R a rotor.

**Note (about 18:00 UTC)**: since 6b7e4b7 `boost.model` 1 implies the builder's exhaust (`exhaust.model` 1). The
"one switch at a time" row for `boost.model` above was measured before it (+0.14 R); with it, a boost.model 1 build
also pays the builder's +0.13 to 0.14 R at the reference's topology. The "every switch on" rows are unchanged (they
had the builder on already, though it was not wired then: re-measure when the dual X is the typical build's).

## Re-measured at 19:15 UTC, after D's f89d40c (model switches' decisions taken each call)

At the tip with f89d40c. The host was busy (the reference V8 read 2092 ns, against 1840 to 1955 earlier), so each
figure here is against the simulator's own V8 in the same qbench run, which `tee-bench --engine` read at 1.045 of the
reference in the same session: R = 1801 ns drive-car, 1856 free-revs. 4 runs a side (8 renders), least kept.

| Build | before (16:20 / 16:55) | now, drive-car | now, free-revs |
|---|---:|---:|---:|
| Builder at the reference's topology (default V8's block) | 1.09 / 1.13 R | **1.28 R** | **1.26 R** |
| Builder, dual X, stubs, valved mufflers (default V8's block) | 1.32 / 1.43 R | **1.53 R** | **1.54 R** |
| Twin-turbo V8, every switch on (X's combo, 6.2 L: configured right before too) | 1.44 / 1.47 R | **1.82 R** | **1.84 R** |
| Turbo six, every switch on (its own layout: configured right before too) | 1.10 / 1.10 R | **1.33 R** | **1.37 R** |

The rise is mostly not D's fix: the two combo builds had their own sizes and were configured right before. At 16:20
`exhaust.model` 1 was not yet wired (it did nothing); now the builder runs in every one of these (profile of the
twin-turbo V8, every switch on: `XNet` 322, `XBuild` 115, `Duct` 649 ns, together some 0.3 R over the reference's
exhaust), and the listener's model 1 has grown (`Scene` 239 ns, from about 164). The two exhaust rows on the default
V8's block are the ones D's fix could touch; they moved +0.13 to +0.21 R, of which the builder's own changes since
(passivity gain check, glides) are part: not separable without a second run on the commit before f89d40c.

**The typical build** (twin-turbo V8, every model on, the dual X): 1.82 R + (1.53 - 1.28) = **about 2.1 R** against
the 1.6 R budget. Economy (0.49): **about 1.0 R**: 40 % of the owner's core on their slower figure, 30 % on the faster.

## The overrun by owner (about 19:30 UTC; the host quiet: load average under 0.7)

The typical build (`out/q/b/typical.json`: X's twin-turbo V8 combo with every switch on and the exhaust of
`exhausts/dual-x-resonated-valved.json`), drive-car, `tee-bench --profile --engine` (the sampler's own cost included,
about +10 %), against the simulator at model 0 (the reference, to the bit: 1.045 of the reference's own player by
`tee-bench --engine` in the same session, 1986 against 1901 ns). R in the profile's units: 2050 / 1.045 = 1962 ns.
Whole build: 4390 ns, **2.24 R** in these units (2.1 R by qbench without the sampler).

| Owner (modules) | model 0, ns | typical, ns | over model 0 | in R | card's budget | over budget |
|---|---:|---:|---:|---:|---:|---:|
| G, the exhaust (Duct, XNet, XBuild, Exhaust, XHeat, xNode) | 765 | 1863 | +1098 | +0.56 | +0.25 | **+0.31** |
| F, boost (BoostMachine 282, BoostEnd 127, compressor 43, the rest) | 1 | 463 | +462 | +0.24 | 0.12 | **+0.12** |
| L, the listener (Scene 258, Listen, Hall, Place, CabinBoom) | 129 | 425 | +296 | +0.15 | none stated | (+0.15) |
| M, the voices (GearVoice 56, DriveVoice 33, FuelVoice 28, ...) | 28 | 199 | +171 | +0.09 | none stated | (+0.09) |
| D, engine and mechanics (Cylinder 365, MovingMasses 45, CycleWork 42, WallHeat 11, ...) | 472 | 596 | +124 | +0.06 | 0.06 | 0 |
| E, valvetrain and intake (Intake 94, ValveLab 61, CamShape 7, AirBox) | 55 | 166 | +111 | +0.06 | 0.05 | +0.01 |
| I, the vehicle (Vehicle 102, VehSound 18, ...) | 19 | 124 | +105 | +0.05 | 0.05 | 0 |
| H, combustion and ECU (Combustion, Ecu1, EndGas, idle and launch control) | 10 | 33 | +23 | +0.01 | 0.06 | -0.05 |
| The valves (`Valve::flow`, G's file, the cylinders' calls) | 565 | 524 | -41 | -0.02 | (the reference's) | |
| **total** | 2044 | 4390 | +2346 | **+1.20** | **+0.59** | **+0.61** |

**The three largest overruns, and what each could give:**

1. **G, +0.31 R over its 0.25.** The builder's own path at the reference's topology is +0.13 to 0.14 R before any
   extra pipe (q6 above); the dual X's eleven extra pipes are the rest. *Exact, reachable*: `XBuild::passGain`
   (63 ns, the passivity gain check over 48 frequencies a pipe, every 32 control steps) only when `shape()` changed a
   coefficient beyond a threshold the check itself can bound (0.03 R); the junctions' general solve (`xNode` 236,
   `solveNode` 167, `solvePair` 98 ns) specialised for two and three arms, as the reference unrolls them (unproven;
   grouping by kind gave nothing: perhaps 0.02 to 0.05 R). Together **about 0.05 to 0.08 R**. *For the owner's ears*:
   the wall's shelves off on short pipes (stubs, links, the X: they shape only the highs) and `back`/`flow16` every
   64 samples: some 0.1 R.
2. **F, +0.12 R over its 0.12.** *Exact*: none found (the pipe-end solve's early exit fires too rarely, under 1 %).
   *For the owner's ears*: F's own three (the pipe-end solve started from the last sample's ratio in one pass, about
   0.05 R; the charge path's volumes in float with a double remainder; the sounds' noise at 48 kHz, 0.01 R).
3. **L, +0.15 R, no stated share** (the reference's listener was 0.07 R). *Exact*: `Scene::hear` (195 ns) reads three
   Lagrange taps a source a sample and steps each path's delay and gain by `dS`, `gS`; while a camera stands still
   (`dS` and `gS` 0) the taps' weights are the same every sample and can be computed once a part (the same products in
   the same order): **up to about 0.05 R** with a still camera, nothing with a moving one. *For the owner's ears*: two
   taps (linear) for the far, quiet sources.

**Within budget**: D (the physical cylinder, the moving masses and the walls: +0.06 R against 0.06), I, H; E 0.01 over.

**After G's c4ce28f** (the passivity check skipped while its inputs stand within 1e-4 of the last; 19:21 UTC, quiet
host): `XBuild::passGain` 63 -> 12 ns a sample on the typical build (one profile, drive-car), about 0.026 R saved; the
whole build's profile read 4020 ns against 4390 (one run each: the rest is run-to-run spread, not claimed).

**After L's change** (a still tap's delay and Lagrange weights computed once a part; L's measurement, byte-identical
WAVs on three listening scenarios): `Scene::hear` 167 -> 96 ns on the V8 free-revs at the tailpipe camera, 128 on
drive-car (the retarded time moves the taps while the car accelerates): about 0.02 to 0.04 R. Not re-measured by Q.

**The listener's row re-measured by Q after 5ce0d1b** (19:26 UTC, quiet host, two profiles of the typical build,
drive-car): listener 334 and 369 ns (Scene 212 / 228, of which `Scene::hear` 152 / 156; Listen 79 / 84; Hall 37 / 53),
against 425 (hear 195) at 19:30's table: **+0.10 to +0.12 R over model 0, from +0.15**. The whole build 4021 and 4042
ns in the profile's units (2.05 to 2.06 R).

**The exhaust's row re-measured by Q after G's b861eca (lossHold skipped where the loss is exactly 0) and c4ce28f**
(19:49 UTC, two profiles of the typical build, drive-car): exhaust 1692 and 1707 ns (Duct 891 / 911, XNet 430 / 426,
`xNode` 170 / 176, XBuild 81 / 75 with `passGain` 11, Exhaust 105 / 101) against 1863 at 19:30's table: **+0.48 to
+0.48 R over model 0, from +0.56: over its card's 0.25 R by about 0.23 R now**. The whole build 4111 / 4185 ns in the
profile's units (2.10 / 2.13 R: the container had just restarted, the spread is wider than the exhaust's saving).
G's own figure for the same two: 1.749 -> 1.708 of the reference (drive-car, cmp-identical WAVs).

## First-launch candidates (P's question, 20:06 UTC, at 4eaa5d2)

qbench, A = B, least of 8 renders of 8 s; R = the reference V8 by `tee-bench --engine` in the same session (1866 ns
drive-car, 1834 free-revs; the simulator's own V8 read 1.032 / 1.034 of it); host quiet (just restarted).

| Archetype (every model on, as P built it) | drive-car | free-revs |
|---|---:|---:|
| `straight-six-twin-turbo-30.json` (3.0 L, two twin-scroll turbos, dual exhaust) | 1.46 R | 1.48 R |
| `turbo-four-20-gdi.json` | 0.92 R | 0.92 R |
| `three-cylinder-10-turbo.json` | 0.78 R | 0.80 R |
