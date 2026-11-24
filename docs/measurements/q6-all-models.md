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
