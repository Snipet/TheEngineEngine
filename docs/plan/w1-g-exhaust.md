# Card G: the exhaust builder

*Wave 1. Repository: TheEngineEngine, integration branch `claude/awesome-gates-ntdb58`. Read
[01-rules.md](01-rules.md), [02-contracts.md](02-contracts.md), `sim/README.md` (K3's: `Exhaust`, `Duct`,
`Valve`, the junction helpers), then the reference, slowly: all of `sim/ref/pipes.fm` and its header; in
`sim/ref/engine.fm` `pipes()`, `node()`, `soak()`, the exhaust half of `sampleTick` (collectors, the three
topologies, the muffler's four junctions, the open end, the jet), the flow-noise bands and `Pop`. The comments
there record what failed before what works was found; this card depends on them more than any other.*

**OWNS**: `sim/exhaust/**`, `sim/spec/exhaust.fm`, the `tel.exhaust` section and its Scope arrays,
`tests/sim/exhaust/**`, `content/builds/exhausts/**`, `docs/models/exhaust.md`.

**Budget**: the reference's own share, plus 0.25 R for a typical dual system with a crossover, resonators and
valves. A duct that is not in the build costs nothing.

**This card cannot start before K3b has carved `Exhaust` out.** Until then: read, and write the property tests of
passivity and conservation against `Duct` and `node()` as they are in `sim/ref/`, which will serve unchanged.

## What the reference does

Three fixed topologies chosen by the layout (all into one; a collector a bank, then one pipe; two pipes with an H
between), each the same chain: headers, collector pipe, main pipe with a converter, a two-chamber muffler, a tail.
Four dials scale it. It stays, bit for bit, as `exhaust.model = 0`, the default.

## The builder

**Topology as data.** `primaryTo[MAX_PORTS]`: which first-level collector each port's primary joins, or none (an
open stack). `collectorTo[MAX_COLLECTORS]`: which side (0 or 1) each collector's pipe joins, directly or through a
second merge. That one table is 4-into-1, 4-2-1 and tri-Y, a collector a bank, 180-degree headers on a crossplane
V8 (the pairs taken across the banks so each collector fires evenly), a boxer's unequal lengths, a log manifold (a
collector that is a small volume) and zoomies (every primary its own open end). Each primary has its own length
and bore (and a step); each collector its pipe.

**Each side is a chain of slots**, each empty or one of a few kinds, in this order: the turbine (card F's
element: below); the main pipe; the converter (none, standard, high-flow: the reference's resistance and
absorption, scaled); the crossover (none, H: a side pipe between the sides, the reference's; X: the two sides
meeting in one four-way node and parting again); a resonator (none; a quarter-wave side branch, a closed stub
whose length is the note it removes; a Helmholtz chamber on a neck; a packed straight-through bottle); the muffler
(none; straight-through; chambered, the reference's two chambers and tube; three-pass); a bypass valve round the
muffler (shut, open, or opened by a speed and load rule or by the ECU), its area gliding; the tail (length, bore,
tips, where it exits: rear, side, up).

**Everything goes through what exists**: `Duct` for every pipe, `node()` for every meeting of pipes (it takes up
to 14 branches), `lossFlow` and `lossKap` for every change of area, the reference's open end for every pipe that
ends in air. A new kind of element is a new arrangement of these, with its loss coefficients from the same
textbook rules the reference cites (an expansion's `(1 - a/A)^2`, a contraction's half of `1 - a/A`); state each
in `docs/models/exhaust.md`. All ducts' memory is taken in `init` at the maxima; a build uses a subset.

**The turbine's housing** (with card F; agree the seam and write it into `sim/README.md`): the pipes that feed a
turbine end at F's volume: you give F each end's arriving wave, impedance and gas temperature and solve your ends
against the pressure F returns, as the reference solves the runners against the plenum; F gives you the flow and
temperature it puts into your downpipe, and an external wastegate's flow into a short open pipe that is yours.

**Radiating ends** are a list, not two: each tail, each open stack, an external gate's pipe, a cutout. Each gives
a signal (the reference's: the rate the flow changes, plus its jet's noise) and a position tag (rear left, rear
right, side, front, up). Until wave 2's listener takes the list, sum them into the reference's left and right as
their tags suggest.

**Heat**: the reference's `soak` over the chain as built: each section's gas and wall temperatures; the pipes'
lengths in samples follow them slowly, as now. Wall temperatures go to telemetry (a view makes headers glow).

**Bangs**: `pop(collector, energy)` is the hook card H's fuel account calls; it is the reference's pressure pulse
and crackle at that node. A flag in telemetry when one fires near a tail while the gas is hot (a view draws
flame).

**Small differences** between cylinders' primaries (the reference has a fixed pattern) become a field.

## Telemetry and Scope

Mean pressure before the turbine, before the converter and before the muffler (back pressure); each section's gas
and wall temperature and mean Mach number; the valve's position; per radiating end its level. Scope: the
pressure at up to 64 points along one chosen path from a port to a tail, refreshed every few milliseconds, so a
view can show the waves travel.

## Tests

1. **`model = 0`: the null test, for all seventeen layouts.**
2. The assignment table set to mean the reference's three topologies gives the reference's sound within -60 dB
   (the order of work differs; say where).
3. **Passivity, as a property test**: fifty random valid topologies with the engine off; an impulse into a port;
   the energy in the system never rises after the impulse ends, and all of it is gone in a bounded time.
4. **Conservation**: with a steady flow in at the ports, the mass out of the radiating ends equals it within
   0.5 %, for every element kind.
5. 180-degree headers on the crossplane V8: the half-order lines that are its burble fall by a stated number of
   dB against the bank-collector build; the boxer's unequal headers raise theirs.
6. An X in place of an H raises the level above a stated frequency; both lower the difference between the two
   tails.
7. A quarter-wave branch cuts its note at the tail by at least 10 dB at the speed where a firing order sits on
   it; a Helmholtz chamber likewise at its own.
8. Mufflers: insertion loss in the order chambered, three-pass, straight-through, none; back pressure in the same
   order; the bypass valve open is louder and brighter than shut, and its opening makes no click (the largest
   step between samples no more than between any two firings).
9. No converter: less back pressure, more highs. Zoomies: as many radiating ends as cylinders.
10. With card F's made-up turbine (a volume and a nozzle from its harness): the junction is passive, and the
    downpipe is quieter than the collector, by itself.
11. Ten minutes of random topologies changed under the fade while the engine runs: finite, bounded, no stuck
    pressure.

## Not in this card

The turbine's own physics (card F). What decides a bang (card H). Hearing an end from a place (wave 2). Two-stroke
expansion chambers (wave 2, built from your ducts).
